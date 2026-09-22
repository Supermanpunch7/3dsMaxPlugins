#pragma once

#include <polyobj.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <map>
#include <utility>
#include <vector>

// Native Chamfer does not preserve MNEdge::track/MNVert::orig.  A temporary,
// otherwise unused map channel is therefore used to carry the source face id
// through the native operation.  The channel is removed before evaluation
// returns, so it never becomes part of the user's mesh.
struct WeightFaceLineageTag {
    int channel = -1;
    int originalMapCount = 0;
    bool appended = false;
};

inline WeightFaceLineageTag BeginWeightFaceLineage(MNMesh& mesh) {
    WeightFaceLineageTag tag;
    tag.originalMapCount = mesh.MNum();

    // Append instead of borrowing a live channel.  When all channels are in
    // use, leave the mesh untouched; uniform-weight propagation still works.
    if (tag.originalMapCount < MAX_MESHMAPS) {
        tag.channel = (std::max)(2, tag.originalMapCount);
        if (tag.channel >= MAX_MESHMAPS) return tag;
        mesh.SetMapNum(tag.channel + 1);
        tag.appended = true;
    }
    if (tag.channel < 0) return tag;

    MNMap* tags = mesh.M(tag.channel);
    if (!tags) {
        // SetMapNum may have grown the mesh before allocation failed.
        // Restore the exact incoming map-channel count on this path too.
        if (tag.appended && mesh.MNum() > tag.originalMapCount)
            mesh.SetMapNum(tag.originalMapCount);
        tag.channel = -1;
        tag.appended = false;
        return tag;
    }

    tags->ClearFlag(MN_DEAD);
    tags->setNumVerts(mesh.numf);
    tags->setNumFaces(mesh.numf);
    for (int face = 0; face < mesh.numf; ++face) {
        tags->v[face] = UVVert(float(face + 1), 12345.25f, -54321.5f);
        tags->f[face].SetSize(mesh.f[face].deg);
        for (int corner = 0; corner < mesh.f[face].deg; ++corner)
            tags->f[face].tv[corner] = face;
    }
    return tag;
}

inline void EndWeightFaceLineage(MNMesh& mesh, const WeightFaceLineageTag& tag) {
    if (tag.channel < 0) return;
    if (tag.appended && mesh.MNum() > tag.originalMapCount)
        mesh.SetMapNum(tag.originalMapCount);
}

inline int DecodeWeightFaceTag(const UVVert& value, int sourceFaceCount) {
    if (std::fabs(value.y - 12345.25f) > 0.01f ||
        std::fabs(value.z + 54321.5f) > 0.01f)
        return -1;
    const int encoded = int(std::floor(value.x + 0.5f));
    if (encoded < 1 || encoded > sourceFaceCount ||
        std::fabs(value.x - float(encoded)) > 0.01f)
        return -1;
    return encoded - 1;
}

inline void WeightLineageAddUnique(std::vector<int>& values, int value) {
    if (value >= 0 && std::find(values.begin(), values.end(), value) == values.end())
        values.push_back(value);
}

inline bool WeightLineageContains(const std::vector<int>& superset,
                                  const std::vector<int>& subset) {
    return std::includes(superset.begin(), superset.end(),
                         subset.begin(), subset.end());
}

inline bool WeightLineageTouchesOwner(const MNEdge& edge,
                                      const std::vector<int>& owners) {
    return std::binary_search(owners.begin(), owners.end(), edge.f1) ||
           std::binary_search(owners.begin(), owners.end(), edge.f2);
}

// Copy the Edge Properties Weight channel after one native Chamfer stage.
//
// 1. Edges shared by two tagged source-face regions have an exact parent.
// 2. Junction/miter edges are assigned to the incident source edge parallel
//    to them.  At a '+' junction, midpoint side selects between the two
//    opposite collinear arms, so horizontal children follow the horizontal
//    parent and vertical children follow the vertical parent.
// 3. If every source edge has the same weight, every output edge can inherit
//    it without ambiguity (including all corner and multi-segment edges).
inline int ApplyWeightFaceLineage(const MNMesh& source, MNMesh& output,
                                  const WeightFaceLineageTag& tag,
                                  bool removeTag = true,
                                  int dataChannel = EDATA_KNOT) {
    // Run the identical lineage policy independently for each scalar channel.
    // Equal Weight values must not imply equal Crease or Depth values at ties.
    const float* sourceWeight = source.edgeFloat(dataChannel);
    if (!sourceWeight) {
        if (removeTag) EndWeightFaceLineage(output, tag);
        return 0;
    }

    int firstLiveEdge = -1;
    for (int edge = 0; edge < source.nume; ++edge) {
        if (!source.e[edge].GetFlag(MN_DEAD)) {
            firstLiveEdge = edge;
            break;
        }
    }
    if (firstLiveEdge < 0) {
        if (removeTag) EndWeightFaceLineage(output, tag);
        return 0;
    }

    output.setEDataSupport(dataChannel);
    float* targetWeight = output.edgeFloat(dataChannel);
    if (!targetWeight) {
        if (removeTag) EndWeightFaceLineage(output, tag);
        return 0;
    }

    bool uniform = true;
    const float uniformValue = sourceWeight[firstLiveEdge];
    for (int edge = firstLiveEdge + 1; edge < source.nume && uniform; ++edge) {
        if (!source.e[edge].GetFlag(MN_DEAD) &&
            std::fabs(sourceWeight[edge] - uniformValue) > 1.e-6f)
            uniform = false;
    }
    if (uniform) {
        int resolved = 0;
        for (int edge = 0; edge < output.nume; ++edge) {
            if (output.e[edge].GetFlag(MN_DEAD)) continue;
            targetWeight[edge] = uniformValue;
            ++resolved;
        }
        if (removeTag) EndWeightFaceLineage(output, tag);
        return resolved;
    }

    MNMap* tags = tag.channel >= 0 && tag.channel < output.MNum()
        ? output.M(tag.channel) : nullptr;
    if (!tags || tags->GetFlag(MN_DEAD) || tags->numf != output.numf) {
        if (removeTag) EndWeightFaceLineage(output, tag);
        return 0;
    }

    std::vector<int> faceOwner(output.numf, -1);
    for (int face = 0; face < output.numf; ++face) {
        if (output.f[face].GetFlag(MN_DEAD) ||
            tags->f[face].deg != output.f[face].deg || tags->f[face].deg < 1)
            continue;
        int owner = -1;
        bool valid = true;
        for (int corner = 0; corner < tags->f[face].deg; ++corner) {
            const int mapVertex = tags->f[face].tv[corner];
            if (mapVertex < 0 || mapVertex >= tags->numv) {
                valid = false;
                break;
            }
            const int decoded = DecodeWeightFaceTag(tags->v[mapVertex], source.numf);
            if (decoded < 0 || (owner >= 0 && owner != decoded)) {
                valid = false;
                break;
            }
            owner = decoded;
        }
        if (valid) faceOwner[face] = owner;
    }

    std::vector<std::vector<int>> outputVertexOwners(output.numv);
    for (int face = 0; face < output.numf; ++face) {
        if (faceOwner[face] < 0) continue;
        for (int corner = 0; corner < output.f[face].deg; ++corner) {
            const int vertex = output.f[face].vtx[corner];
            if (vertex >= 0 && vertex < output.numv)
                WeightLineageAddUnique(outputVertexOwners[vertex], faceOwner[face]);
        }
    }
    for (auto& owners : outputVertexOwners)
        std::sort(owners.begin(), owners.end());

    std::vector<std::vector<int>> sourceVertexFaces(source.numv);
    for (int face = 0; face < source.numf; ++face) {
        if (source.f[face].GetFlag(MN_DEAD)) continue;
        for (int corner = 0; corner < source.f[face].deg; ++corner) {
            const int vertex = source.f[face].vtx[corner];
            if (vertex >= 0 && vertex < source.numv)
                WeightLineageAddUnique(sourceVertexFaces[vertex], face);
        }
    }
    for (auto& faces : sourceVertexFaces)
        std::sort(faces.begin(), faces.end());

    std::vector<std::vector<int>> sourceVertexEdges(source.numv);
    std::vector<float> sourceVertexRadius(
        source.numv, (std::numeric_limits<float>::max)());
    std::map<std::pair<int, int>, int> sourceByFaces;
    for (int edge = 0; edge < source.nume; ++edge) {
        const MNEdge& sourceEdge = source.e[edge];
        if (sourceEdge.GetFlag(MN_DEAD) || sourceEdge.v1 < 0 ||
            sourceEdge.v1 >= source.numv || sourceEdge.v2 < 0 ||
            sourceEdge.v2 >= source.numv)
            continue;

        sourceVertexEdges[sourceEdge.v1].push_back(edge);
        sourceVertexEdges[sourceEdge.v2].push_back(edge);
        const float length = Length(source.v[sourceEdge.v2].p - source.v[sourceEdge.v1].p);
        if (length > 1.e-7f) {
            sourceVertexRadius[sourceEdge.v1] =
                (std::min)(sourceVertexRadius[sourceEdge.v1], length);
            sourceVertexRadius[sourceEdge.v2] =
                (std::min)(sourceVertexRadius[sourceEdge.v2], length);
        }

        if (sourceEdge.f1 >= 0 && sourceEdge.f2 >= 0 &&
            sourceEdge.f1 != sourceEdge.f2) {
            const auto key = (std::minmax)(sourceEdge.f1, sourceEdge.f2);
            auto inserted = sourceByFaces.emplace(key, edge);
            if (!inserted.second) inserted.first->second = -2;
        }
    }

    std::vector<int> parent(output.nume, -1);

    // Exact longitudinal descendants: both endpoints still touch the two
    // source face regions that met on their parent edge.
    for (int edge = 0; edge < output.nume; ++edge) {
        const MNEdge& outputEdge = output.e[edge];
        if (outputEdge.GetFlag(MN_DEAD) || outputEdge.v1 < 0 ||
            outputEdge.v1 >= output.numv || outputEdge.v2 < 0 ||
            outputEdge.v2 >= output.numv)
            continue;

        const auto& left = outputVertexOwners[outputEdge.v1];
        const auto& right = outputVertexOwners[outputEdge.v2];
        std::vector<int> common;
        std::set_intersection(left.begin(), left.end(), right.begin(), right.end(),
                              std::back_inserter(common));

        int exactParent = -1;
        bool ambiguous = false;
        for (size_t first = 0; first < common.size(); ++first) {
            for (size_t second = first + 1; second < common.size(); ++second) {
                const auto found = sourceByFaces.find(
                    std::make_pair(common[first], common[second]));
                if (found == sourceByFaces.end() || found->second < 0) continue;
                if (exactParent >= 0 && exactParent != found->second)
                    ambiguous = true;
                else
                    exactParent = found->second;
            }
        }
        if (!ambiguous && exactParent >= 0) parent[edge] = exactParent;
    }

    // Resolve corner/miter descendants by their local direction at the source
    // junction.  This is the important fallback for '+' shaped intersections.
    constexpr float kVertexNeighborhood = 0.67f;
    constexpr float kMinimumAlignment = 0.70f;
    constexpr float kAlignmentTie = 0.002f;
    constexpr float kSideTie = 0.002f;
    for (int edge = 0; edge < output.nume; ++edge) {
        if (parent[edge] >= 0) continue;
        const MNEdge& outputEdge = output.e[edge];
        if (outputEdge.GetFlag(MN_DEAD) || outputEdge.v1 < 0 ||
            outputEdge.v1 >= output.numv || outputEdge.v2 < 0 ||
            outputEdge.v2 >= output.numv)
            continue;

        std::vector<int> owners;
        std::set_union(outputVertexOwners[outputEdge.v1].begin(),
                       outputVertexOwners[outputEdge.v1].end(),
                       outputVertexOwners[outputEdge.v2].begin(),
                       outputVertexOwners[outputEdge.v2].end(),
                       std::back_inserter(owners));
        if (owners.empty()) continue;

        const Point3 midpoint =
            (output.v[outputEdge.v1].p + output.v[outputEdge.v2].p) * 0.5f;
        int junction = -1;
        float bestDistance = (std::numeric_limits<float>::max)();
        bool junctionTie = false;
        // Every valid junction must be a corner of every tagged owner face.
        // Seed from the lowest-degree owner face instead of scanning every
        // source vertex (important for production meshes with many vertices).
        int seedFace = -1;
        int seedDegree = (std::numeric_limits<int>::max)();
        for (int owner : owners) {
            if (owner < 0 || owner >= source.numf ||
                source.f[owner].GetFlag(MN_DEAD))
                continue;
            if (source.f[owner].deg < seedDegree) {
                seedFace = owner;
                seedDegree = source.f[owner].deg;
            }
        }
        if (seedFace < 0) continue;
        for (int corner = 0; corner < source.f[seedFace].deg; ++corner) {
            const int vertex = source.f[seedFace].vtx[corner];
            if (vertex < 0 || vertex >= source.numv ||
                source.v[vertex].GetFlag(MN_DEAD) ||
                sourceVertexEdges[vertex].empty() ||
                !WeightLineageContains(sourceVertexFaces[vertex], owners))
                continue;
            const float radius = sourceVertexRadius[vertex];
            if (radius == (std::numeric_limits<float>::max)()) continue;
            const float distance = Length(midpoint - source.v[vertex].p);
            if (distance > radius * kVertexNeighborhood + 1.e-5f) continue;
            const float tieDistance = (std::max)(1.e-5f, radius * 1.e-5f);
            if (distance + tieDistance < bestDistance) {
                junction = vertex;
                bestDistance = distance;
                junctionTie = false;
            } else if (std::fabs(distance - bestDistance) <= tieDistance) {
                junctionTie = true;
            }
        }
        if (junction < 0 || junctionTie) continue;

        Point3 outputDirection = output.v[outputEdge.v2].p - output.v[outputEdge.v1].p;
        const float outputLength = Length(outputDirection);
        if (outputLength <= 1.e-7f) continue;
        outputDirection /= outputLength;

        struct DirectionCandidate {
            int edge;
            float alignment;
            float side;
        };
        std::vector<DirectionCandidate> candidates;
        float bestAlignment = -1.0f;
        Point3 midpointDirection = midpoint - source.v[junction].p;
        const float midpointDistance = Length(midpointDirection);
        if (midpointDistance > 1.e-7f) midpointDirection /= midpointDistance;

        for (int sourceEdgeIndex : sourceVertexEdges[junction]) {
            const MNEdge& sourceEdge = source.e[sourceEdgeIndex];
            if (!WeightLineageTouchesOwner(sourceEdge, owners)) continue;
            const int other = sourceEdge.v1 == junction ? sourceEdge.v2 : sourceEdge.v1;
            if (other < 0 || other >= source.numv) continue;
            Point3 sourceDirection = source.v[other].p - source.v[junction].p;
            const float sourceLength = Length(sourceDirection);
            if (sourceLength <= 1.e-7f) continue;
            sourceDirection /= sourceLength;
            const float alignment = std::fabs(DotProd(outputDirection, sourceDirection));
            if (alignment < kMinimumAlignment) continue;
            const float side = midpointDistance > 1.e-7f
                ? DotProd(midpointDirection, sourceDirection) : 0.0f;
            candidates.push_back({ sourceEdgeIndex, alignment, side });
            bestAlignment = (std::max)(bestAlignment, alignment);
        }
        if (candidates.empty()) continue;

        std::vector<DirectionCandidate> aligned;
        for (const auto& candidate : candidates)
            if (bestAlignment - candidate.alignment <= kAlignmentTie)
                aligned.push_back(candidate);

        if (aligned.size() > 1 && midpointDistance > 1.e-7f) {
            float bestSide = -2.0f;
            for (const auto& candidate : aligned)
                bestSide = (std::max)(bestSide, candidate.side);
            std::vector<DirectionCandidate> sided;
            for (const auto& candidate : aligned)
                if (bestSide - candidate.side <= kSideTie)
                    sided.push_back(candidate);
            aligned.swap(sided);
        }

        if (aligned.empty()) continue;
        const float chosenWeight = sourceWeight[aligned.front().edge];
        bool sameWeight = true;
        for (size_t candidate = 1; candidate < aligned.size(); ++candidate) {
            if (std::fabs(sourceWeight[aligned[candidate].edge] - chosenWeight) > 1.e-6f) {
                sameWeight = false;
                break;
            }
        }
        if (aligned.size() == 1 || sameWeight)
            parent[edge] = aligned.front().edge;
    }

    int resolved = 0;
    for (int edge = 0; edge < output.nume; ++edge) {
        if (parent[edge] < 0) continue;
        targetWeight[edge] = sourceWeight[parent[edge]];
        ++resolved;
    }

    if (removeTag) EndWeightFaceLineage(output, tag);
    return resolved;
}




