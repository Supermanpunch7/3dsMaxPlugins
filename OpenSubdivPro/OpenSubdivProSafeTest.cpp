#include "max.h"
#include "iparamb2.h"
#include "polyobj.h"
#include "MNNormalSpec.h"
#include "resource.h"

#include <Qt/QMaxParamBlockWidget.h>
#include <Qt/QmaxSpinBox.h>
#include <Qt/QMaxColorSwatch.h>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <opensubdiv/far/primvarRefiner.h>
#include <opensubdiv/far/topologyDescriptor.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>

namespace
{
// OpenSubdivPro owns a distinct persistent class ID. Do not reuse the former
// MeshSmoothPro ID: both binaries can briefly coexist during an upgrade.
constexpr Class_ID kOpenSubdivProClassId(0x38a75c31, 0x71e946b7);
constexpr int kPBlockRef = 0;

HINSTANCE g_instance = nullptr;

const TCHAR* GetString(int id)
{
    static TCHAR buffer[256];
    if (g_instance && LoadString(g_instance, id, buffer, _countof(buffer)))
        return buffer;
    return _T("");
}

enum ParamBlockIds
{
    mesh_smooth_pro_params
};

enum ParamMapIds
{
    map_general_controls,
    map_opensubdiv_controls,
    map_weighted_normals
};

enum ParamIds
{
    pb_iterations,
    pb_smoothness,
    pb_use_vertex_crease,
    pb_use_edge_crease,
    pb_smooth_result,
    pb_use_render_iterations,
    pb_render_iterations,
    pb_isoline_display,
    pb_vertex_boundary,
    pb_fvar_boundary,
    pb_smooth_triangles,
    pb_crease_method,
    pb_wn_use_area_weight,
    pb_wn_use_angle_weight,
    pb_wn_use_convex_angle,
    pb_wn_snap_to_largest_face,
    pb_wn_blending_coeff,
    pb_wn_use_smoothing_groups,
    pb_wn_use_uv_seams,
    pb_wn_uv_channel,
    pb_wn_use_hard_edge_angle,
    pb_wn_hard_edge_angle,
    pb_wn_smoothing_coeff,
    pb_wn_boundary_coeff,
    pb_wn_iterations,
    pb_wn_relaxation_coeff,
    pb_wn_use_total_coplanar_area,
    // Appended to preserve the ParamID values stored by existing scenes.
    pb_wn_display_normals,
    pb_wn_normal_length,
    pb_display_hard_edges,
    pb_hard_edge_color,
    pb_native_weighted_normals
};

constexpr float kOpenSubdivInfiniteSharpness = 10.0f;

struct OsdVec3
{
    float value[3] = { 0.0f, 0.0f, 0.0f };

    OsdVec3() = default;
    explicit OsdVec3(const Point3& point)
    {
        value[0] = point.x;
        value[1] = point.y;
        value[2] = point.z;
    }

    void Clear()
    {
        value[0] = value[1] = value[2] = 0.0f;
    }

    void AddWithWeight(const OsdVec3& source, float weight)
    {
        value[0] += source.value[0] * weight;
        value[1] += source.value[1] * weight;
        value[2] += source.value[2] * weight;
    }
};

struct FaceVaryingInput
{
    int mapChannel = 0;
    std::vector<UVVert> values;
    std::vector<OpenSubdiv::Far::Index> indices;
};

struct IntVectorHash
{
    std::size_t operator()(const std::vector<int>& values) const noexcept
    {
        std::size_t hash = values.size();
        for (const int value : values)
        {
            hash ^= std::hash<int>{}(value) + 0x9e3779b9u +
                (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

float Clamp01(float value)
{
    return std::max(0.0f, std::min(1.0f, value));
}

std::uint64_t EdgeKey(int vertexA, int vertexB)
{
    const std::uint32_t low = static_cast<std::uint32_t>(std::min(vertexA, vertexB));
    const std::uint32_t high = static_cast<std::uint32_t>(std::max(vertexA, vertexB));
    return (static_cast<std::uint64_t>(low) << 32) | high;
}

std::vector<float> PropagateEdgeData(
    const OpenSubdiv::Far::TopologyRefiner& refiner,
    std::vector<float> values,
    int maxLevel,
    float generatedEdgeDefault = 0.0f)
{
    for (int level = 1; level <= maxLevel; ++level)
    {
        const OpenSubdiv::Far::TopologyLevel& parentLevel = refiner.GetLevel(level - 1);
        const OpenSubdiv::Far::TopologyLevel& childLevel = refiner.GetLevel(level);
        // A subdivided mesh contains direct children of source edges and newly
        // generated face-to-edge spokes. Max edge Weight and Chamfer Depth use
        // 1.0 and 0.5 as their neutral values, not zero.
        std::vector<float> childValues(childLevel.GetNumEdges(), generatedEdgeDefault);

        const int parentCount = std::min(parentLevel.GetNumEdges(), static_cast<int>(values.size()));
        for (int edge = 0; edge < parentCount; ++edge)
        {
            const OpenSubdiv::Far::ConstIndexArray children = parentLevel.GetEdgeChildEdges(edge);
            for (int child = 0; child < children.size(); ++child)
            {
                const int childIndex = children[child];
                if (childIndex >= 0 && childIndex < static_cast<int>(childValues.size()))
                    childValues[childIndex] = values[edge];
            }
        }
        values.swap(childValues);
    }
    return values;
}

std::vector<float> PropagateVertexData(
    const OpenSubdiv::Far::TopologyRefiner& refiner,
    std::vector<float> values,
    int maxLevel,
    float generatedVertexDefault = 0.0f)
{
    for (int level = 1; level <= maxLevel; ++level)
    {
        const OpenSubdiv::Far::TopologyLevel& parentLevel = refiner.GetLevel(level - 1);
        const OpenSubdiv::Far::TopologyLevel& childLevel = refiner.GetLevel(level);
        std::vector<float> childValues(childLevel.GetNumVertices(), generatedVertexDefault);

        const int parentCount = std::min(parentLevel.GetNumVertices(), static_cast<int>(values.size()));
        for (int vertex = 0; vertex < parentCount; ++vertex)
        {
            const int childIndex = parentLevel.GetVertexChildVertex(vertex);
            if (childIndex >= 0 && childIndex < static_cast<int>(childValues.size()))
                childValues[childIndex] = values[vertex];
        }
        values.swap(childValues);
    }
    return values;
}

std::vector<int> PropagateFaceSources(
    const OpenSubdiv::Far::TopologyRefiner& refiner,
    int baseFaceCount,
    int maxLevel)
{
    std::vector<int> sources(baseFaceCount);
    for (int face = 0; face < baseFaceCount; ++face)
        sources[face] = face;

    for (int level = 1; level <= maxLevel; ++level)
    {
        const OpenSubdiv::Far::TopologyLevel& childLevel = refiner.GetLevel(level);
        std::vector<int> childSources(childLevel.GetNumFaces(), 0);
        for (int face = 0; face < childLevel.GetNumFaces(); ++face)
        {
            const int parentFace = childLevel.GetFaceParentFace(face);
            if (parentFace >= 0 && parentFace < static_cast<int>(sources.size()))
                childSources[face] = sources[parentFace];
        }
        sources.swap(childSources);
    }
    return sources;
}

std::vector<int> SortedFaceKey(const int* vertices, int count)
{
    std::vector<int> key(vertices, vertices + count);
    std::sort(key.begin(), key.end());
    return key;
}

bool BuildNativeComponentMaps(
    const OpenSubdiv::Far::TopologyLevel& osdLevel,
    const MNMesh& nativeMesh,
    const std::vector<int>& osdToNativeVertex,
    std::vector<int>& osdToNativeFace,
    std::vector<int>& osdToNativeEdge)
{
    if (static_cast<int>(osdToNativeVertex.size()) != osdLevel.GetNumVertices())
        return false;

    std::unordered_map<std::uint64_t, int> nativeEdges;
    nativeEdges.reserve(nativeMesh.nume);
    for (int edge = 0; edge < nativeMesh.nume; ++edge)
    {
        if (nativeMesh.e[edge].GetFlag(MN_DEAD))
            continue;
        nativeEdges[EdgeKey(nativeMesh.e[edge].v1, nativeMesh.e[edge].v2)] = edge;
    }

    osdToNativeEdge.assign(osdLevel.GetNumEdges(), -1);
    for (int edge = 0; edge < osdLevel.GetNumEdges(); ++edge)
    {
        const OpenSubdiv::Far::ConstIndexArray vertices = osdLevel.GetEdgeVertices(edge);
        if (vertices.size() != 2)
            return false;
        const int nativeVertexA = osdToNativeVertex[vertices[0]];
        const int nativeVertexB = osdToNativeVertex[vertices[1]];
        if (nativeVertexA < 0 || nativeVertexA >= nativeMesh.numv ||
            nativeVertexB < 0 || nativeVertexB >= nativeMesh.numv)
        {
            return false;
        }
        const auto found = nativeEdges.find(EdgeKey(nativeVertexA, nativeVertexB));
        if (found == nativeEdges.end())
            return false;
        osdToNativeEdge[edge] = found->second;
    }

    std::unordered_map<std::vector<int>, int, IntVectorHash> nativeFaces;
    nativeFaces.reserve(nativeMesh.numf);
    for (int face = 0; face < nativeMesh.numf; ++face)
    {
        const MNFace& nativeFace = nativeMesh.f[face];
        if (nativeFace.GetFlag(MN_DEAD) || nativeFace.deg < 3)
            continue;
        nativeFaces[SortedFaceKey(nativeFace.vtx, nativeFace.deg)] = face;
    }

    osdToNativeFace.assign(osdLevel.GetNumFaces(), -1);
    for (int face = 0; face < osdLevel.GetNumFaces(); ++face)
    {
        const OpenSubdiv::Far::ConstIndexArray vertices = osdLevel.GetFaceVertices(face);
        std::vector<int> key(vertices.size());
        for (int corner = 0; corner < vertices.size(); ++corner)
        {
            const int nativeVertex = osdToNativeVertex[vertices[corner]];
            if (nativeVertex < 0 || nativeVertex >= nativeMesh.numv)
                return false;
            key[corner] = nativeVertex;
        }
        std::sort(key.begin(), key.end());
        const auto found = nativeFaces.find(key);
        if (found == nativeFaces.end())
            return false;
        osdToNativeFace[face] = found->second;
    }

    return true;
}

float SanitizeVertexWeight(float value)
{
    if (!std::isfinite(value))
        return 1.0f;
    return std::max(MIN_WEIGHT, std::min(MAX_WEIGHT, value));
}

float SanitizeEdgeKnot(float value)
{
    if (!std::isfinite(value))
        return 1.0f;
    // Zero knot intervals create a discontinuity in NURMS. Keep Weight a
    // spacing control and leave hard corners to the independent Crease data.
    return std::max(MIN_WEIGHT, std::min(MAX_WEIGHT, value));
}

void PrepareNativeWeightMesh(MNMesh& mesh)
{
    for (int vertex = 0; vertex < mesh.numv; ++vertex)
    {
        if (!mesh.v[vertex].GetFlag(MN_DEAD))
            mesh.v[vertex].SetFlag(MN_VERT_SUBDIVISION_CORNER);
    }
    for (int edge = 0; edge < mesh.nume; ++edge)
    {
        if (!mesh.e[edge].GetFlag(MN_DEAD))
            mesh.e[edge].SetFlag(MN_EDGE_SUBDIVISION_BOUNDARY);
    }

    // Keep the original vertex and edge crease channels in both the weighted
    // and neutral native meshes. NURMS Weight is nonlinear around creases, so
    // measuring the delta after stripping crease data does not reproduce the
    // MeshSmooth result. Since both streams share the same crease channels,
    // their difference isolates Weight without replacing the OSD crease base.

    mesh.ClearVFlags(MN_TARG);
    mesh.TargetVertsBySelection(MNM_SL_OBJECT);
    mesh.ClearEFlags(MN_EDGE_NOCROSS | MN_EDGE_MAP_SEAM);
    mesh.SetMapSeamFlags();
    mesh.FenceOneSidedEdges();
}

bool BuildNativeVertexWeightDeltas(
    const MNMesh& source,
    const OpenSubdiv::Far::TopologyRefiner& refiner,
    int maxLevel,
    std::vector<Point3>& deltas)
{
    deltas.clear();
    if (maxLevel <= 0)
        return false;

    MNMesh weighted(source);
    const bool sourceHasVertexWeight = weighted.vDataSupport(VDATA_WEIGHT);
    if (weighted.VDNum() <= VDATA_WEIGHT)
        weighted.setNumVData(VDATA_WEIGHT + 1, TRUE);
    if (weighted.EDNum() <= EDATA_KNOT)
        weighted.setNumEData(EDATA_KNOT + 1, TRUE);
    weighted.setVDataSupport(VDATA_WEIGHT);
    weighted.setEDataSupport(EDATA_KNOT);

    bool hasNonNeutralWeight = false;
    float* vertexWeights = weighted.vertexFloat(VDATA_WEIGHT);
    std::vector<float> requestedVertexWeights(weighted.numv, 1.0f);
    for (int vertex = 0; vertex < weighted.numv; ++vertex)
    {
        const float userValue = sourceHasVertexWeight
            ? SanitizeVertexWeight(vertexWeights[vertex])
            : 1.0f;
        // OpenSubdivPro uses 1 as neutral and smaller values as a tighter
        // gather. Native MeshSmooth expresses the same effect in the opposite
        // direction, so feed its reciprocal into CubicNURMS.
        requestedVertexWeights[vertex] = SanitizeVertexWeight(
            1.0f / std::max(userValue, 1.0e-4f));
        hasNonNeutralWeight =
            hasNonNeutralWeight || std::fabs(userValue - 1.0f) > 1.0e-6f;
    }

    for (int vertex = 0; vertex < weighted.numv; ++vertex)
        vertexWeights[vertex] = requestedVertexWeights[vertex];

    // Edge Weight is a data channel for modifiers above OpenSubdivPro. It must
    // not change this modifier's surface. Keep native NURMS edge knots neutral
    // while evaluating the Vertex Weight-only spacing delta.
    std::fill(
        weighted.edgeFloat(EDATA_KNOT),
        weighted.edgeFloat(EDATA_KNOT) + weighted.nume,
        1.0f);

    if (!hasNonNeutralWeight)
        return false;

    MNMesh neutral(weighted);
    std::fill(
        neutral.vertexFloat(VDATA_WEIGHT),
        neutral.vertexFloat(VDATA_WEIGHT) + neutral.numv,
        1.0f);
    std::fill(
        neutral.edgeFloat(EDATA_KNOT),
        neutral.edgeFloat(EDATA_KNOT) + neutral.nume,
        1.0f);

    PrepareNativeWeightMesh(weighted);
    PrepareNativeWeightMesh(neutral);

    const OpenSubdiv::Far::TopologyLevel& baseLevel = refiner.GetLevel(0);
    if (weighted.numv != baseLevel.GetNumVertices())
        return false;

    std::vector<int> osdToNativeVertex(baseLevel.GetNumVertices());
    for (int vertex = 0; vertex < baseLevel.GetNumVertices(); ++vertex)
        osdToNativeVertex[vertex] = vertex;

    std::vector<int> osdToNativeFace;
    std::vector<int> osdToNativeEdge;
    if (!BuildNativeComponentMaps(
            baseLevel,
            weighted,
            osdToNativeVertex,
            osdToNativeFace,
            osdToNativeEdge))
    {
        return false;
    }

    for (int level = 1; level <= maxLevel; ++level)
    {
        const OpenSubdiv::Far::TopologyLevel& parentLevel = refiner.GetLevel(level - 1);
        const OpenSubdiv::Far::TopologyLevel& childLevel = refiner.GetLevel(level);
        const int oldVertexCount = weighted.numv;
        const int oldFaceCount = weighted.numf;
        const int oldEdgeCount = weighted.nume;

        std::vector<int> childVertexMap(childLevel.GetNumVertices(), -1);
        for (int vertex = 0; vertex < parentLevel.GetNumVertices(); ++vertex)
        {
            const int child = parentLevel.GetVertexChildVertex(vertex);
            if (child < 0 || child >= childLevel.GetNumVertices())
                return false;
            childVertexMap[child] = osdToNativeVertex[vertex];
        }
        for (int face = 0; face < parentLevel.GetNumFaces(); ++face)
        {
            const int child = parentLevel.GetFaceChildVertex(face);
            if (child < 0 || child >= childLevel.GetNumVertices() ||
                osdToNativeFace[face] < 0)
            {
                return false;
            }
            childVertexMap[child] = oldVertexCount + osdToNativeFace[face];
        }
        for (int edge = 0; edge < parentLevel.GetNumEdges(); ++edge)
        {
            const int child = parentLevel.GetEdgeChildVertex(edge);
            if (child < 0 || child >= childLevel.GetNumVertices() ||
                osdToNativeEdge[edge] < 0)
            {
                return false;
            }
            childVertexMap[child] =
                oldVertexCount + oldFaceCount + osdToNativeEdge[edge];
        }

        weighted.OrderVerts();
        neutral.OrderVerts();
        weighted.CubicNURMS(nullptr, nullptr, MN_SUBDIV_NEWMAP);
        neutral.CubicNURMS(nullptr, nullptr, MN_SUBDIV_NEWMAP);
        weighted.CollapseDeadFaces();
        neutral.CollapseDeadFaces();
        if (!weighted.GetFlag(MN_MESH_FILLED_IN))
            weighted.FillInMesh();
        if (!neutral.GetFlag(MN_MESH_FILLED_IN))
            neutral.FillInMesh();

        const int expectedVertexCount = oldVertexCount + oldFaceCount + oldEdgeCount;
        if (weighted.numv != expectedVertexCount ||
            neutral.numv != expectedVertexCount ||
            weighted.numv != childLevel.GetNumVertices() ||
            weighted.numf != childLevel.GetNumFaces() ||
            weighted.nume != childLevel.GetNumEdges() ||
            neutral.numv != weighted.numv ||
            neutral.numf != weighted.numf ||
            neutral.nume != weighted.nume)
        {
            return false;
        }

        for (const int nativeVertex : childVertexMap)
        {
            if (nativeVertex < 0 || nativeVertex >= weighted.numv)
                return false;
        }
        osdToNativeVertex.swap(childVertexMap);

        weighted.SetMapSeamFlags();
        neutral.SetMapSeamFlags();
        weighted.FenceOneSidedEdges();
        neutral.FenceOneSidedEdges();
        if (!BuildNativeComponentMaps(
                childLevel,
                weighted,
                osdToNativeVertex,
                osdToNativeFace,
                osdToNativeEdge))
        {
            return false;
        }
    }

    const OpenSubdiv::Far::TopologyLevel& finalLevel = refiner.GetLevel(maxLevel);
    deltas.resize(finalLevel.GetNumVertices());
    for (int vertex = 0; vertex < finalLevel.GetNumVertices(); ++vertex)
    {
        const int nativeVertex = osdToNativeVertex[vertex];
        const Point3 delta = weighted.P(nativeVertex) - neutral.P(nativeVertex);
        if (!std::isfinite(delta.x) ||
            !std::isfinite(delta.y) ||
            !std::isfinite(delta.z))
        {
            deltas.clear();
            return false;
        }
        deltas[vertex] = delta;
    }
    return true;
}

Point3 GetOsdPoint(const OsdVec3& value)
{
    return Point3(value.value[0], value.value[1], value.value[2]);
}

void SetOsdPoint(OsdVec3& value, const Point3& point)
{
    value.value[0] = point.x;
    value.value[1] = point.y;
    value.value[2] = point.z;
}

bool IsFinitePoint(const Point3& point)
{
    return std::isfinite(point.x) &&
        std::isfinite(point.y) &&
        std::isfinite(point.z);
}

float SpacingStrength(float value)
{
    const float weight = SanitizeVertexWeight(value);
    // 1.0 is neutral. Reciprocal values have matching opposite effects:
    // 2.0 spreads by the same amount that 0.5 gathers.
    return (weight - 1.0f) / (weight + 1.0f);
}

bool InitializeBaseSpacingWeights(
    const MNMesh& source,
    const OpenSubdiv::Far::TopologyLevel& baseLevel,
    std::vector<float>& vertexWeights,
    std::vector<float>& edgeWeights)
{
    constexpr float kNeutralEpsilon = 1.0e-6f;
    bool hasNonNeutralWeight = false;

    vertexWeights.assign(baseLevel.GetNumVertices(), 1.0f);
    if (source.vDataSupport(VDATA_WEIGHT))
    {
        const float* values = source.vertexFloat(VDATA_WEIGHT);
        const int count = std::min(
            static_cast<int>(source.numv),
            baseLevel.GetNumVertices());
        for (int vertex = 0; vertex < count; ++vertex)
        {
            const float value = SanitizeVertexWeight(values[vertex]);
            vertexWeights[vertex] = value;
            hasNonNeutralWeight =
                hasNonNeutralWeight || std::fabs(value - 1.0f) > kNeutralEpsilon;
        }
    }

    edgeWeights.assign(baseLevel.GetNumEdges(), 1.0f);
    if (source.eDataSupport(EDATA_KNOT))
    {
        const float* values = source.edgeFloat(EDATA_KNOT);
        std::unordered_map<std::uint64_t, int> sourceEdgeByVertices;
        sourceEdgeByVertices.reserve(source.nume);
        for (int edge = 0; edge < source.nume; ++edge)
        {
            if (!source.e[edge].GetFlag(MN_DEAD))
            {
                sourceEdgeByVertices[
                    EdgeKey(source.e[edge].v1, source.e[edge].v2)] = edge;
            }
        }

        for (int edge = 0; edge < baseLevel.GetNumEdges(); ++edge)
        {
            const OpenSubdiv::Far::ConstIndexArray vertices =
                baseLevel.GetEdgeVertices(edge);
            if (vertices.size() != 2)
                continue;

            const auto found = sourceEdgeByVertices.find(
                EdgeKey(vertices[0], vertices[1]));
            if (found == sourceEdgeByVertices.end())
                continue;

            const float value = SanitizeEdgeKnot(values[found->second]);
            edgeWeights[edge] = value;
            hasNonNeutralWeight =
                hasNonNeutralWeight || std::fabs(value - 1.0f) > kNeutralEpsilon;
        }
    }

    return hasNonNeutralWeight;
}

void ApplyFinalEdgeWeightSpacing(
    const MNMesh& source,
    const OpenSubdiv::Far::TopologyRefiner& refiner,
    int maxLevel,
    const OpenSubdiv::Far::TopologyLevel& finalLevel,
    const std::vector<int>& faceSources,
    const OsdVec3* pureFinalPositions,
    OsdVec3* finalPositions)
{
    constexpr float kNeutralEpsilon = 1.0e-6f;
    constexpr float kDirectionEpsilon = 1.0e-12f;
    constexpr float kBoundaryEpsilon = 1.0e-4f;
    constexpr float kMaximumExponent = 5.0f;

    if (!pureFinalPositions || !finalPositions || !source.eDataSupport(EDATA_KNOT))
        return;

    const float* edgeWeights = source.edgeFloat(EDATA_KNOT);
    const int finalVertexCount = finalLevel.GetNumVertices();
    std::vector<Point3> deltas(finalVertexCount, Point3(0.0f, 0.0f, 0.0f));
    std::vector<int> counts(finalVertexCount, 0);

    const OpenSubdiv::Far::TopologyLevel& baseLevel = refiner.GetLevel(0);
    std::unordered_map<std::uint64_t, int> baseEdgeByVertices;
    baseEdgeByVertices.reserve(baseLevel.GetNumEdges());
    for (int edge = 0; edge < baseLevel.GetNumEdges(); ++edge)
    {
        const OpenSubdiv::Far::ConstIndexArray vertices =
            baseLevel.GetEdgeVertices(edge);
        if (vertices.size() == 2)
            baseEdgeByVertices[EdgeKey(vertices[0], vertices[1])] = edge;
    }

    auto pinFinalEdgeDescendants =
        [&](int baseEdge, std::vector<unsigned char>& pinned)
    {
        if (baseEdge < 0)
            return;
        std::vector<int> descendantEdges(1, baseEdge);
        for (int level = 0; level < maxLevel; ++level)
        {
            const OpenSubdiv::Far::TopologyLevel& parentLevel =
                refiner.GetLevel(level);
            std::vector<int> childEdges;
            childEdges.reserve(descendantEdges.size() * 2);
            for (const int edge : descendantEdges)
            {
                const OpenSubdiv::Far::ConstIndexArray children =
                    parentLevel.GetEdgeChildEdges(edge);
                for (int child = 0; child < children.size(); ++child)
                {
                    if (children[child] >= 0)
                        childEdges.push_back(children[child]);
                }
            }
            descendantEdges.swap(childEdges);
        }

        for (const int edge : descendantEdges)
        {
            if (edge < 0 || edge >= finalLevel.GetNumEdges())
                continue;
            const OpenSubdiv::Far::ConstIndexArray vertices =
                finalLevel.GetEdgeVertices(edge);
            for (int endpoint = 0; endpoint < vertices.size(); ++endpoint)
            {
                const int vertex = vertices[endpoint];
                if (vertex >= 0 && vertex < finalVertexCount)
                    pinned[vertex] = 1;
            }
        }
    };

    auto addFaceStrip = [&](int sourceEdge, int sourceFace, float edgeWeight)
    {
        if (sourceFace < 0 || sourceFace >= source.numf)
            return;
        const MNFace& face = source.f[sourceFace];
        if (face.GetFlag(MN_DEAD) || face.deg != 4)
            return;

        const int selectedA = source.e[sourceEdge].v1;
        const int selectedB = source.e[sourceEdge].v2;
        int selectedCorner = -1;
        for (int corner = 0; corner < face.deg; ++corner)
        {
            const int next = (corner + 1) % face.deg;
            if (EdgeKey(face.vtx[corner], face.vtx[next]) ==
                EdgeKey(selectedA, selectedB))
            {
                selectedCorner = corner;
                break;
            }
        }
        if (selectedCorner < 0)
            return;

        const int i0 = selectedCorner;
        const int i1 = (selectedCorner + 1) % 4;
        const int i2 = (selectedCorner + 2) % 4;
        const int i3 = (selectedCorner + 3) % 4;
        const Point3 pointA = source.P(face.vtx[i0]);
        const Point3 pointB = source.P(face.vtx[i1]);
        const Point3 pointC = source.P(face.vtx[i2]);
        const Point3 pointD = source.P(face.vtx[i3]);
        const Point3 edgeAxis = pointB - pointA;
        const float edgeAxisLengthSquared = DotProd(edgeAxis, edgeAxis);
        if (!IsFinitePoint(edgeAxis) ||
            !std::isfinite(edgeAxisLengthSquared) ||
            edgeAxisLengthSquared <= kDirectionEpsilon)
        {
            return;
        }

        const float strength = SpacingStrength(edgeWeight);
        const float exponent =
            1.0f + std::fabs(strength) * (kMaximumExponent - 1.0f);
        if (!std::isfinite(exponent))
            return;

        // Build an exact face-local subdivision coordinate.  Geometry-space
        // projection gives slightly different values at the two ends of the
        // same generated wire on a curved surface, which shears a straight
        // row into a diagonal.  Topological midpoint propagation gives every
        // vertex on that row the identical dyadic coordinate.
        std::vector<float> wireCoordinate(
            baseLevel.GetNumVertices(),
            std::numeric_limits<float>::quiet_NaN());
        wireCoordinate[face.vtx[i0]] = 0.0f;
        wireCoordinate[face.vtx[i1]] = 0.0f;
        wireCoordinate[face.vtx[i2]] = 1.0f;
        wireCoordinate[face.vtx[i3]] = 1.0f;
        std::vector<float> alongCoordinate(
            baseLevel.GetNumVertices(),
            std::numeric_limits<float>::quiet_NaN());
        alongCoordinate[face.vtx[i0]] = 0.0f;
        alongCoordinate[face.vtx[i1]] = 1.0f;
        alongCoordinate[face.vtx[i2]] = 1.0f;
        alongCoordinate[face.vtx[i3]] = 0.0f;

        std::vector<int> levelFaceSources(source.numf);
        for (int faceIndex = 0; faceIndex < source.numf; ++faceIndex)
            levelFaceSources[faceIndex] = faceIndex;

        for (int level = 0; level < maxLevel; ++level)
        {
            const OpenSubdiv::Far::TopologyLevel& parentLevel =
                refiner.GetLevel(level);
            const OpenSubdiv::Far::TopologyLevel& childLevel =
                refiner.GetLevel(level + 1);
            std::vector<float> childCoordinate(
                childLevel.GetNumVertices(),
                std::numeric_limits<float>::quiet_NaN());
            std::vector<float> childAlongCoordinate(
                childLevel.GetNumVertices(),
                std::numeric_limits<float>::quiet_NaN());

            for (int vertex = 0; vertex < parentLevel.GetNumVertices(); ++vertex)
            {
                if (vertex >= static_cast<int>(wireCoordinate.size()) ||
                    !std::isfinite(wireCoordinate[vertex]) ||
                    !std::isfinite(alongCoordinate[vertex]))
                {
                    continue;
                }
                const int child = parentLevel.GetVertexChildVertex(vertex);
                if (child >= 0 && child < static_cast<int>(childCoordinate.size()))
                {
                    childCoordinate[child] = wireCoordinate[vertex];
                    childAlongCoordinate[child] = alongCoordinate[vertex];
                }
            }

            for (int edge = 0; edge < parentLevel.GetNumEdges(); ++edge)
            {
                const OpenSubdiv::Far::ConstIndexArray vertices =
                    parentLevel.GetEdgeVertices(edge);
                if (vertices.size() != 2 ||
                    vertices[0] >= static_cast<int>(wireCoordinate.size()) ||
                    vertices[1] >= static_cast<int>(wireCoordinate.size()) ||
                    !std::isfinite(wireCoordinate[vertices[0]]) ||
                    !std::isfinite(wireCoordinate[vertices[1]]) ||
                    !std::isfinite(alongCoordinate[vertices[0]]) ||
                    !std::isfinite(alongCoordinate[vertices[1]]))
                {
                    continue;
                }
                const int child = parentLevel.GetEdgeChildVertex(edge);
                if (child >= 0 && child < static_cast<int>(childCoordinate.size()))
                {
                    childCoordinate[child] =
                        0.5f * (wireCoordinate[vertices[0]] +
                                wireCoordinate[vertices[1]]);
                    childAlongCoordinate[child] =
                        0.5f * (alongCoordinate[vertices[0]] +
                                alongCoordinate[vertices[1]]);
                }
            }

            for (int parentFace = 0;
                 parentFace < parentLevel.GetNumFaces();
                 ++parentFace)
            {
                if (parentFace >= static_cast<int>(levelFaceSources.size()) ||
                    levelFaceSources[parentFace] != sourceFace)
                {
                    continue;
                }
                const OpenSubdiv::Far::ConstIndexArray vertices =
                    parentLevel.GetFaceVertices(parentFace);
                float sum = 0.0f;
                float alongSum = 0.0f;
                bool valid = !vertices.empty();
                for (int corner = 0; corner < vertices.size(); ++corner)
                {
                    if (vertices[corner] >= static_cast<int>(wireCoordinate.size()) ||
                        !std::isfinite(wireCoordinate[vertices[corner]]) ||
                        !std::isfinite(alongCoordinate[vertices[corner]]))
                    {
                        valid = false;
                        break;
                    }
                    sum += wireCoordinate[vertices[corner]];
                    alongSum += alongCoordinate[vertices[corner]];
                }
                if (!valid)
                    continue;
                const int child = parentLevel.GetFaceChildVertex(parentFace);
                if (child >= 0 && child < static_cast<int>(childCoordinate.size()))
                {
                    childCoordinate[child] = sum / static_cast<float>(vertices.size());
                    childAlongCoordinate[child] =
                        alongSum / static_cast<float>(vertices.size());
                }
            }

            std::vector<int> childFaceSources(childLevel.GetNumFaces(), -1);
            for (int childFace = 0; childFace < childLevel.GetNumFaces(); ++childFace)
            {
                const int parentFace = childLevel.GetFaceParentFace(childFace);
                if (parentFace >= 0 &&
                    parentFace < static_cast<int>(levelFaceSources.size()))
                {
                    childFaceSources[childFace] = levelFaceSources[parentFace];
                }
            }
            wireCoordinate.swap(childCoordinate);
            alongCoordinate.swap(childAlongCoordinate);
            levelFaceSources.swap(childFaceSources);
        }

        std::vector<unsigned char> pinnedBoundary(finalVertexCount, 0);
        const auto selectedBaseEdge = baseEdgeByVertices.find(
            EdgeKey(face.vtx[i0], face.vtx[i1]));
        if (selectedBaseEdge != baseEdgeByVertices.end())
            pinFinalEdgeDescendants(selectedBaseEdge->second, pinnedBoundary);

        const auto oppositeBaseEdge = baseEdgeByVertices.find(
            EdgeKey(face.vtx[i2], face.vtx[i3]));
        if (oppositeBaseEdge != baseEdgeByVertices.end())
            pinFinalEdgeDescendants(oppositeBaseEdge->second, pinnedBoundary);

        auto evaluatePurePatch =
            [&](float queryS, float queryT, Point3& sampledPoint)
        {
            constexpr float kParameterEpsilon = 1.0e-5f;
            struct CurveSample
            {
                float parameter;
                Point3 point;
            };
            std::vector<CurveSample> samples;
            std::vector<unsigned char> visited(finalVertexCount, 0);

            for (int patchFace = 0;
                 patchFace < finalLevel.GetNumFaces();
                 ++patchFace)
            {
                if (patchFace >= static_cast<int>(faceSources.size()) ||
                    faceSources[patchFace] != sourceFace)
                {
                    continue;
                }
                const OpenSubdiv::Far::ConstIndexArray patchVertices =
                    finalLevel.GetFaceVertices(patchFace);
                for (int patchCorner = 0;
                     patchCorner < patchVertices.size();
                     ++patchCorner)
                {
                    const int patchVertex = patchVertices[patchCorner];
                    if (patchVertex < 0 ||
                        patchVertex >= finalVertexCount ||
                        visited[patchVertex] ||
                        patchVertex >= static_cast<int>(wireCoordinate.size()) ||
                        !std::isfinite(wireCoordinate[patchVertex]) ||
                        !std::isfinite(alongCoordinate[patchVertex]))
                    {
                        continue;
                    }
                    if (std::fabs(alongCoordinate[patchVertex] - queryS) <=
                        kParameterEpsilon)
                    {
                        visited[patchVertex] = 1;
                        samples.push_back({
                            wireCoordinate[patchVertex],
                            GetOsdPoint(pureFinalPositions[patchVertex]) });
                    }
                }
            }

            if (samples.empty())
                return false;
            std::sort(
                samples.begin(),
                samples.end(),
                [](const CurveSample& first, const CurveSample& second)
                {
                    return first.parameter < second.parameter;
                });

            if (queryT <= samples.front().parameter + kParameterEpsilon)
            {
                sampledPoint = samples.front().point;
                return IsFinitePoint(sampledPoint);
            }
            if (queryT >= samples.back().parameter - kParameterEpsilon)
            {
                sampledPoint = samples.back().point;
                return IsFinitePoint(sampledPoint);
            }

            int upper = 1;
            while (upper < static_cast<int>(samples.size()) &&
                   samples[upper].parameter < queryT)
            {
                ++upper;
            }
            if (upper >= static_cast<int>(samples.size()))
                return false;
            const int lower = upper - 1;
            if (std::fabs(queryT - samples[lower].parameter) <=
                kParameterEpsilon)
            {
                sampledPoint = samples[lower].point;
                return IsFinitePoint(sampledPoint);
            }
            if (std::fabs(queryT - samples[upper].parameter) <=
                kParameterEpsilon)
            {
                sampledPoint = samples[upper].point;
                return IsFinitePoint(sampledPoint);
            }

            const float interval =
                samples[upper].parameter - samples[lower].parameter;
            if (interval <= kParameterEpsilon)
                return false;
            const float u = std::clamp(
                (queryT - samples[lower].parameter) / interval,
                0.0f,
                1.0f);
            const Point3& p1 = samples[lower].point;
            const Point3& p2 = samples[upper].point;
            const Point3& p0 = samples[std::max(0, lower - 1)].point;
            const Point3& p3 =
                samples[std::min(static_cast<int>(samples.size()) - 1,
                                 upper + 1)].point;
            const Point3 tangent1 = (p2 - p0) * 0.5f;
            const Point3 tangent2 = (p3 - p1) * 0.5f;
            const float u2 = u * u;
            const float u3 = u2 * u;
            Point3 curved =
                p1 * (2.0f * u3 - 3.0f * u2 + 1.0f) +
                tangent1 * (u3 - 2.0f * u2 + u) +
                p2 * (-2.0f * u3 + 3.0f * u2) +
                tangent2 * (u3 - u2);

            // Retain the smooth tangent but prohibit cubic overshoot beyond
            // the current subdivision segment.
            curved.x = std::clamp(curved.x, std::min(p1.x, p2.x), std::max(p1.x, p2.x));
            curved.y = std::clamp(curved.y, std::min(p1.y, p2.y), std::max(p1.y, p2.y));
            curved.z = std::clamp(curved.z, std::min(p1.z, p2.z), std::max(p1.z, p2.z));
            sampledPoint = curved;
            return IsFinitePoint(sampledPoint);
        };

        for (int finalFace = 0; finalFace < finalLevel.GetNumFaces(); ++finalFace)
        {
            if (finalFace >= static_cast<int>(faceSources.size()) ||
                faceSources[finalFace] != sourceFace)
            {
                continue;
            }

            const OpenSubdiv::Far::ConstIndexArray vertices =
                finalLevel.GetFaceVertices(finalFace);
            for (int corner = 0; corner < vertices.size(); ++corner)
            {
                const int vertex = vertices[corner];
                if (vertex < 0 || vertex >= finalVertexCount)
                    continue;

                const Point3 purePoint = GetOsdPoint(pureFinalPositions[vertex]);
                if (pinnedBoundary[vertex])
                    continue;

                float s = vertex < static_cast<int>(alongCoordinate.size())
                    ? alongCoordinate[vertex]
                    : std::numeric_limits<float>::quiet_NaN();
                if (!std::isfinite(s))
                    continue;
                s = std::clamp(s, 0.0f, 1.0f);

                float t = vertex < static_cast<int>(wireCoordinate.size())
                    ? wireCoordinate[vertex]
                    : std::numeric_limits<float>::quiet_NaN();
                if (!std::isfinite(t) ||
                    t <= kBoundaryEpsilon ||
                    t >= 1.0f - kBoundaryEpsilon)
                    continue;

                t = std::clamp(t, 0.0f, 1.0f);
                float desiredT = t;
                if (edgeWeight <= kNeutralEpsilon)
                {
                    // Weight zero is the exact gather limit: every generated
                    // interior row lands on the selected-edge reference and
                    // must never pass through it.
                    desiredT = 0.0f;
                }
                else if (strength < -kNeutralEpsilon)
                {
                    desiredT = std::pow(t, exponent);
                }
                else if (strength > kNeutralEpsilon)
                {
                    desiredT = 1.0f - std::pow(1.0f - t, exponent);
                }
                else
                {
                    continue;
                }
                desiredT = std::clamp(desiredT, 0.0f, 1.0f);
                Point3 desired;
                if (!evaluatePurePatch(s, desiredT, desired))
                    continue;
                const Point3 correction = desired - GetOsdPoint(finalPositions[vertex]);
                if (!IsFinitePoint(correction))
                    continue;

                deltas[vertex] += correction;
                ++counts[vertex];
            }
        }
    };

    for (int edge = 0; edge < source.nume; ++edge)
    {
        if (source.e[edge].GetFlag(MN_DEAD))
            continue;
        const float weight = SanitizeEdgeKnot(edgeWeights[edge]);
        if (std::fabs(weight - 1.0f) <= kNeutralEpsilon)
            continue;
        addFaceStrip(edge, source.e[edge].f1, weight);
        addFaceStrip(edge, source.e[edge].f2, weight);
    }

    for (int vertex = 0; vertex < finalVertexCount; ++vertex)
    {
        if (counts[vertex] <= 0)
            continue;
        const Point3 result =
            GetOsdPoint(finalPositions[vertex]) +
            deltas[vertex] / static_cast<float>(counts[vertex]);
        if (IsFinitePoint(result))
            SetOsdPoint(finalPositions[vertex], result);
    }
}

void PropagateSpacingWeightsOneLevel(
    const OpenSubdiv::Far::TopologyLevel& parentLevel,
    const OpenSubdiv::Far::TopologyLevel& childLevel,
    std::vector<float>& vertexWeights,
    std::vector<float>& edgeWeights)
{
    std::vector<float> childVertexWeights(childLevel.GetNumVertices(), 1.0f);
    const int parentVertexCount =
        std::min(parentLevel.GetNumVertices(), static_cast<int>(vertexWeights.size()));
    for (int vertex = 0; vertex < parentVertexCount; ++vertex)
    {
        const int child = parentLevel.GetVertexChildVertex(vertex);
        if (child >= 0 && child < static_cast<int>(childVertexWeights.size()))
            childVertexWeights[child] = vertexWeights[vertex];
    }

    // Edge Weight is an original-edge strip control, not a recursively
    // inherited weight. Re-applying it to generated child edges lets the
    // influence leak into neighboring original strips and changes the shape.
    std::vector<float> childEdgeWeights(childLevel.GetNumEdges(), 1.0f);

    vertexWeights.swap(childVertexWeights);
    edgeWeights.swap(childEdgeWeights);
}

struct SpacingDelta
{
    Point3 edge = Point3(0.0f, 0.0f, 0.0f);
    int edgeCount = 0;
};

void ApplyWeightSpacing(
    const OpenSubdiv::Far::TopologyLevel& parentLevel,
    const OpenSubdiv::Far::TopologyLevel& childLevel,
    int subdivisionLevel,
    const std::vector<float>& vertexWeights,
    const std::vector<float>& edgeWeights,
    const OsdVec3* pureChildPositions,
    OsdVec3* childPositions)
{
    constexpr float kNeutralEpsilon = 1.0e-6f;
    (void)subdivisionLevel;

    const int childVertexCount = childLevel.GetNumVertices();
    if (childVertexCount <= 0 || !pureChildPositions || !childPositions)
        return;

    auto validChildVertex = [childVertexCount](int vertex)
    {
        return vertex >= 0 && vertex < childVertexCount;
    };

    // hardPinned points never move. Every weighted vertex and every point on
    // a weighted edge is an exact center anchor; Weight may reposition only
    // the surrounding wire rows, never the user's selected center itself.
    std::vector<unsigned char> hardPinned(childVertexCount, 0);
    std::vector<unsigned char> resetToPure(childVertexCount, 0);

    const int parentVertexCount =
        std::min(parentLevel.GetNumVertices(), static_cast<int>(vertexWeights.size()));
    for (int vertex = 0; vertex < parentVertexCount; ++vertex)
    {
        if (std::fabs(vertexWeights[vertex] - 1.0f) <= kNeutralEpsilon)
            continue;
        const int anchor = parentLevel.GetVertexChildVertex(vertex);
        if (validChildVertex(anchor))
        {
            hardPinned[anchor] = 1;
            resetToPure[anchor] = 1;
        }
    }

    const int parentEdgeCount =
        std::min(parentLevel.GetNumEdges(), static_cast<int>(edgeWeights.size()));
    for (int edge = 0; edge < parentEdgeCount; ++edge)
    {
        if (std::fabs(edgeWeights[edge] - 1.0f) <= kNeutralEpsilon)
            continue;

        const int edgePoint = parentLevel.GetEdgeChildVertex(edge);
        if (validChildVertex(edgePoint))
        {
            hardPinned[edgePoint] = 1;
            resetToPure[edgePoint] = 1;
        }

        const OpenSubdiv::Far::ConstIndexArray vertices =
            parentLevel.GetEdgeVertices(edge);
        for (int endpoint = 0; endpoint < vertices.size(); ++endpoint)
        {
            const int vertexPoint =
                parentLevel.GetVertexChildVertex(vertices[endpoint]);
            if (validChildVertex(vertexPoint))
            {
                hardPinned[vertexPoint] = 1;
                resetToPure[vertexPoint] = 1;
            }
        }
    }

    // A true OpenSubdiv corner remains an exact anchor. Edge/face child
    // points around it are still free to receive the uniform spacing ratio.
    for (int vertex = 0; vertex < parentLevel.GetNumVertices(); ++vertex)
    {
        if (parentLevel.GetVertexSharpness(vertex) <= 0.0f)
            continue;
        const int vertexPoint = parentLevel.GetVertexChildVertex(vertex);
        if (validChildVertex(vertexPoint))
        {
            hardPinned[vertexPoint] = 1;
            resetToPure[vertexPoint] = 1;
        }
    }

    for (int vertex = 0; vertex < childVertexCount; ++vertex)
    {
        if (resetToPure[vertex])
            childPositions[vertex] = pureChildPositions[vertex];
    }
}
}

class MeshSmoothPro;

class MeshSmoothProClassDesc final : public ClassDesc2
{
public:
    int IsPublic() override { return TRUE; }
    void* Create(BOOL loading = FALSE) override;
    const TCHAR* ClassName() override { return _T("OpenSubdivProSafeTest"); }
    const TCHAR* NonLocalizedClassName() override { return _T("OpenSubdivProSafeTest"); }
    SClass_ID SuperClassID() override { return OSM_CLASS_ID; }
    Class_ID ClassID() override { return kOpenSubdivProClassId; }
    const TCHAR* Category() override { return GetString(IDS_CATEGORY); }
    const TCHAR* InternalName() override { return _T("OpenSubdivProSafeTest"); }
    bool UseOnlyInternalNameForMAXScriptExposure() override { return true; }
    HINSTANCE HInstance() override { return g_instance; }
    MaxSDK::QMaxParamBlockWidget* CreateQtWidget(
        ReferenceMaker& owner,
        IParamBlock2& paramBlock,
        const MapID paramMapID,
        MSTR& rollupTitle,
        int& rollupFlags,
        int& rollupCategory) override;
};

static MeshSmoothProClassDesc g_classDesc;

class NativeNormalsContext final : public LocalModData
{
public:
    ModContext normals;
    MNMesh displayMesh;
    bool evaluated = false;
    LocalModData* Clone() override { return new NativeNormalsContext(); }
};

class MeshSmoothPro final : public Modifier
{
public:
    MeshSmoothPro()
    {
        g_classDesc.MakeAutoParamBlocks(this);
    }

    ~MeshSmoothPro() override { DeleteAllRefsFromMe(); }

    Modifier* NativeNormals()
    {
        auto* target = m_pblock ? m_pblock->GetReferenceTarget(pb_native_weighted_normals) : nullptr;
        return target && target->SuperClassID() == OSM_CLASS_ID &&
            target->ClassID() == WEIGHTED_NORMALS_MOD_ID ? static_cast<Modifier*>(target) : nullptr;
    }

    Modifier* EnsureNativeNormals(TimeValue t)
    {
        auto* native = NativeNormals();
        const bool created = native == nullptr;
        if (created)
            native = static_cast<Modifier*>(CreateInstance(OSM_CLASS_ID, WEIGHTED_NORMALS_MOD_ID));
        if (!native)
            return nullptr;
        IParamBlock2* params = native->GetParamBlock(0);
        if (!params)
        {
            native->DeleteThis();
            return nullptr;
        }
        struct Mapping { ParamID legacy; const TCHAR* name; bool integer; };
        const Mapping mappings[] = {
            {pb_wn_use_area_weight, _T("useAreaWeight"), true},
            {pb_wn_use_angle_weight, _T("useAngleWeight"), true},
            {pb_wn_use_convex_angle, _T("useConvexAngle"), true},
            {pb_wn_snap_to_largest_face, _T("snapToLargestFace"), true},
            {pb_wn_blending_coeff, _T("blendingCoeff"), false},
            {pb_wn_use_smoothing_groups, _T("useSmoothingGroups"), true},
            {pb_wn_use_uv_seams, _T("useUVSeams"), true},
            {pb_wn_uv_channel, _T("uvChannelIndex"), true},
            {pb_wn_use_hard_edge_angle, _T("useHardEdgeAngle"), true},
            {pb_wn_hard_edge_angle, _T("hardEdgeAngle"), false},
            {pb_wn_smoothing_coeff, _T("smoothingCoeff"), false},
            {pb_wn_boundary_coeff, _T("boundaryCoeff"), false},
            {pb_wn_iterations, _T("smoothingIterLimit"), true},
            {pb_wn_relaxation_coeff, _T("relaxationCoeff"), false},
            {pb_wn_use_total_coplanar_area, _T("useTotalCoplanarArea"), true},
            {pb_wn_display_normals, _T("displayNormals"), true},
            {pb_wn_normal_length, _T("normalLength"), false}
        };
        for (const auto& mapping : mappings)
        {
            if (mapping.integer)
                params->SetValueByName(mapping.name, m_pblock->GetInt(mapping.legacy, t), t);
            else
            {
                float value = m_pblock->GetFloat(mapping.legacy, t);
                if (mapping.legacy == pb_wn_hard_edge_angle)
                    value = DegToRad(value);
                params->SetValueByName(mapping.name, value, t);
            }
            // Preserve legacy controllers except the degree-valued angle.
            // That controller remains in the old block and is sampled below.
            if (created && mapping.legacy != pb_wn_hard_edge_angle)
                if (Control* controller = m_pblock->GetControllerByID(mapping.legacy))
                    for (int index = 0; index < params->NumParams(); ++index)
                    {
                        const ParamID id = params->IndextoID(index);
                        const auto& def = params->GetParamDef(id);
                        if (def.int_name && _tcscmp(def.int_name, mapping.name) == 0)
                            params->SetControllerByID(id, 0,
                                static_cast<Control*>(CloneRefHierarchy(controller)), FALSE);
                    }
        }
        if (created)
            m_pblock->SetValue(pb_native_weighted_normals, 0, static_cast<ReferenceTarget*>(native));
        return native;
    }

    void DeleteThis() override { delete this; }
    void GetClassName(MSTR& name, bool localized) const override
    {
        name = _T("OpenSubdivProSafeTest");
    }
    Class_ID ClassID() override { return kOpenSubdivProClassId; }
    SClass_ID SuperClassID() override { return OSM_CLASS_ID; }
    const TCHAR* GetObjectName(bool localized) const override
    {
        return _T("OpenSubdivProSafeTest");
    }
    CreateMouseCallBack* GetCreateMouseCallBack() override { return nullptr; }
    void BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev) override
    {
        if (m_editIP)
            return;
        m_editIP = ip;
        SetAFlag(A_MOD_BEING_EDITED);
        Modifier::BeginEditParams(ip, flags, prev);
        g_classDesc.BeginEditParams(ip, this, flags, prev);
        NotifyDependents(FOREVER, PART_ALL, REFMSG_BEGIN_EDIT);
        NotifyDependents(FOREVER, PART_ALL, REFMSG_MOD_DISPLAY_ON);
    }
    void EndEditParams(IObjParam* ip, ULONG flags, Animatable* next) override
    {
        if (!m_editIP)
            return;
        ClearAFlag(A_MOD_BEING_EDITED);
        NotifyDependents(FOREVER, PART_ALL, REFMSG_END_EDIT);
        NotifyDependents(FOREVER, PART_ALL, REFMSG_MOD_DISPLAY_OFF);
        Modifier::EndEditParams(ip, flags, next);
        g_classDesc.EndEditParams(ip, this, flags, next);
        m_editIP = nullptr;
    }

    int Display(TimeValue t, INode* node, ViewExp* view, int flags, ModContext* context) override
    {
        auto* data = context ? static_cast<NativeNormalsContext*>(context->localData) : nullptr;
        if (!m_editIP || !node || !view || !data || !data->evaluated ||
            !m_pblock->GetInt(pb_smooth_result, t) || !m_pblock->GetInt(pb_wn_display_normals, t))
            return 0;
        auto* normals = data->displayMesh.GetSpecifiedNormals();
        auto* gw = view->getGW();
        if (!normals || !gw)
            return 0;
        const DWORD limits = gw->getRndLimits();
        gw->setRndLimits(limits & ~GW_ILLUM);
        Matrix3 tm = node->GetObjectTM(t);
        gw->setTransform(tm);
        normals->SetParent(&data->displayMesh);
        normals->SetDisplayLength(m_pblock->GetFloat(pb_wn_normal_length, t));
        normals->Display(gw, false);
        gw->setRndLimits(limits);
        return 0;
    }

    void GetWorldBoundBox(TimeValue t, INode* node, ViewExp* view, Box3& box, ModContext* context) override
    {
        box.Init();
        auto* data = context ? static_cast<NativeNormalsContext*>(context->localData) : nullptr;
        if (node && data && data->evaluated && m_pblock->GetInt(pb_smooth_result, t) &&
            m_pblock->GetInt(pb_wn_display_normals, t))
        {
            const float length = m_pblock->GetFloat(pb_wn_normal_length, t);
            const Point3 extent(length, length, length);
            const Matrix3 tm = node->GetObjectTM(t);
            for (int v = 0; v < data->displayMesh.numv; ++v)
                for (int corner = 0; corner < 8; ++corner)
                {
                    const Point3 offset((corner & 1) ? extent.x : -extent.x,
                        (corner & 2) ? extent.y : -extent.y, (corner & 4) ? extent.z : -extent.z);
                    box += (data->displayMesh.v[v].p + offset) * tm;
                }
        }
    }

    void NotifyInputChanged(const Interval& interval, PartID part, RefMessage message, ModContext* context) override
    {
        auto* data = context ? static_cast<NativeNormalsContext*>(context->localData) : nullptr;
        if (data)
        {
            data->evaluated = false;
            if (auto* native = NativeNormals())
                native->NotifyInputChanged(interval, part, message, &data->normals);
        }
    }

    int NumRefs() override { return 1; }
    RefTargetHandle GetReference(int index) override { return index == kPBlockRef ? m_pblock : nullptr; }
    int NumSubs() override { return 1; }
    Animatable* SubAnim(int index) override { return index == 0 ? m_pblock : nullptr; }
    TSTR SubAnimName(int, bool localized) override { return localized ? GetString(IDS_PARAMETERS) : _T("Parameters"); }
    RefTargetHandle Clone(RemapDir& remap) override
    {
        auto* clone = new MeshSmoothPro();
        clone->ReplaceReference(kPBlockRef, remap.CloneRef(m_pblock));
        BaseClone(this, clone, remap);
        return clone;
    }
    RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL) override
    {
        return REF_SUCCEED;
    }

    int NumParamBlocks() override { return 1; }
    IParamBlock2* GetParamBlock(int) override { return m_pblock; }
    IParamBlock2* GetParamBlockByID(BlockID id) override
    {
        return m_pblock && m_pblock->ID() == id ? m_pblock : nullptr;
    }

    ChannelMask ChannelsUsed() override { return ALL_CHANNELS; }
    ChannelMask ChannelsChanged() override { return ALL_CHANNELS; }
    Class_ID InputType() override { return polyObjectClassID; }
    BOOL ChangeTopology() override { return TRUE; }
    BOOL DependOnTopology(ModContext&) override { return TRUE; }
    Interval LocalValidity(TimeValue t) override
    {
        if (TestAFlag(A_MOD_BEING_EDITED))
            return NEVER;
        return EvaluationValidity(t);
    }

    Interval EvaluationValidity(TimeValue t)
    {
        Interval validity = FOREVER;
        int i = 0;
        float f = 0.0f;
        m_pblock->GetValue(pb_iterations, t, i, validity);
        m_pblock->GetValue(pb_smoothness, t, f, validity);
        m_pblock->GetValue(pb_use_vertex_crease, t, i, validity);
        m_pblock->GetValue(pb_use_edge_crease, t, i, validity);
        m_pblock->GetValue(pb_smooth_result, t, i, validity);
        m_pblock->GetValue(pb_use_render_iterations, t, i, validity);
        m_pblock->GetValue(pb_render_iterations, t, i, validity);
        m_pblock->GetValue(pb_isoline_display, t, i, validity);
        m_pblock->GetValue(pb_vertex_boundary, t, i, validity);
        m_pblock->GetValue(pb_fvar_boundary, t, i, validity);
        m_pblock->GetValue(pb_smooth_triangles, t, i, validity);
        m_pblock->GetValue(pb_crease_method, t, i, validity);
        m_pblock->GetValue(pb_wn_use_area_weight, t, i, validity);
        m_pblock->GetValue(pb_wn_use_angle_weight, t, i, validity);
        m_pblock->GetValue(pb_wn_use_convex_angle, t, i, validity);
        m_pblock->GetValue(pb_wn_snap_to_largest_face, t, i, validity);
        m_pblock->GetValue(pb_wn_blending_coeff, t, f, validity);
        m_pblock->GetValue(pb_wn_use_smoothing_groups, t, i, validity);
        m_pblock->GetValue(pb_wn_use_uv_seams, t, i, validity);
        m_pblock->GetValue(pb_wn_uv_channel, t, i, validity);
        m_pblock->GetValue(pb_wn_use_hard_edge_angle, t, i, validity);
        m_pblock->GetValue(pb_wn_hard_edge_angle, t, f, validity);
        m_pblock->GetValue(pb_wn_smoothing_coeff, t, f, validity);
        m_pblock->GetValue(pb_wn_boundary_coeff, t, f, validity);
        m_pblock->GetValue(pb_wn_iterations, t, i, validity);
        m_pblock->GetValue(pb_wn_relaxation_coeff, t, f, validity);
        m_pblock->GetValue(pb_wn_use_total_coplanar_area, t, i, validity);
        m_pblock->GetValue(pb_wn_display_normals, t, i, validity);
        m_pblock->GetValue(pb_wn_normal_length, t, f, validity);
        m_pblock->GetValue(pb_display_hard_edges, t, i, validity);
        Color hardEdgeColor;
        m_pblock->GetValue(pb_hard_edge_color, t, hardEdgeColor, validity);
        if (auto* native = NativeNormals())
            validity &= native->LocalValidity(t);
        return validity;
    }

    void ModifyObject(TimeValue t, ModContext& context, ObjectState* os, INode* node) override
    {
        if (!context.localData)
            context.localData = new NativeNormalsContext();
        auto* normalsData = static_cast<NativeNormalsContext*>(context.localData);
        normalsData->evaluated = false;
        if (!os || !os->obj || !os->obj->IsSubClassOf(polyObjectClassID))
            return;

        auto* polyObject = static_cast<PolyObject*>(os->obj);
        MNMesh source = polyObject->GetMesh();
        source.CollapseDeadStructs();
        if (!source.GetFlag(MN_MESH_FILLED_IN))
            source.FillInMesh();

        if (source.numv == 0 || source.numf == 0)
            return;

        int maxLevel = 1;
        int useVertexCrease = TRUE;
        int useEdgeCrease = TRUE;
        int smoothResult = TRUE;
        int useRenderIterations = FALSE;
        int renderIterations = 0;
        int isolineDisplay = FALSE;
        int vertexBoundary = 0;
        int fvarBoundary = 4;
        int smoothTriangles = TRUE;
        int creaseMethod = 0;
        int wnUseAreaWeight = FALSE;
        int wnUseAngleWeight = TRUE;
        int wnUseConvexAngle = TRUE;
        int wnSnapToLargestFace = FALSE;
        float wnBlendingCoeff = 1.0f;
        int wnUseSmoothingGroups = TRUE;
        int wnUseUvSeams = FALSE;
        int wnUvChannel = 1;
        int wnUseHardEdgeAngle = FALSE;
        float wnHardEdgeAngle = 30.0f;
        float wnSmoothingCoeff = 1.0f;
        float wnBoundaryCoeff = 0.0f;
        int wnIterations = 10;
        float wnRelaxationCoeff = 0.5f;
        int wnUseTotalCoplanarArea = TRUE;
        int wnDisplayNormals = FALSE;
        float wnNormalLength = 10.0f;
        m_pblock->GetValue(pb_iterations, t, maxLevel, FOREVER);
        m_pblock->GetValue(pb_use_vertex_crease, t, useVertexCrease, FOREVER);
        m_pblock->GetValue(pb_use_edge_crease, t, useEdgeCrease, FOREVER);
        m_pblock->GetValue(pb_smooth_result, t, smoothResult, FOREVER);
        m_pblock->GetValue(pb_use_render_iterations, t, useRenderIterations, FOREVER);
        m_pblock->GetValue(pb_render_iterations, t, renderIterations, FOREVER);
        m_pblock->GetValue(pb_isoline_display, t, isolineDisplay, FOREVER);
        m_pblock->GetValue(pb_vertex_boundary, t, vertexBoundary, FOREVER);
        m_pblock->GetValue(pb_fvar_boundary, t, fvarBoundary, FOREVER);
        m_pblock->GetValue(pb_smooth_triangles, t, smoothTriangles, FOREVER);
        m_pblock->GetValue(pb_crease_method, t, creaseMethod, FOREVER);
        m_pblock->GetValue(pb_wn_use_area_weight, t, wnUseAreaWeight, FOREVER);
        m_pblock->GetValue(pb_wn_use_angle_weight, t, wnUseAngleWeight, FOREVER);
        m_pblock->GetValue(pb_wn_use_convex_angle, t, wnUseConvexAngle, FOREVER);
        m_pblock->GetValue(pb_wn_snap_to_largest_face, t, wnSnapToLargestFace, FOREVER);
        m_pblock->GetValue(pb_wn_blending_coeff, t, wnBlendingCoeff, FOREVER);
        m_pblock->GetValue(pb_wn_use_smoothing_groups, t, wnUseSmoothingGroups, FOREVER);
        m_pblock->GetValue(pb_wn_use_uv_seams, t, wnUseUvSeams, FOREVER);
        m_pblock->GetValue(pb_wn_uv_channel, t, wnUvChannel, FOREVER);
        m_pblock->GetValue(pb_wn_use_hard_edge_angle, t, wnUseHardEdgeAngle, FOREVER);
        m_pblock->GetValue(pb_wn_hard_edge_angle, t, wnHardEdgeAngle, FOREVER);
        m_pblock->GetValue(pb_wn_smoothing_coeff, t, wnSmoothingCoeff, FOREVER);
        m_pblock->GetValue(pb_wn_boundary_coeff, t, wnBoundaryCoeff, FOREVER);
        m_pblock->GetValue(pb_wn_iterations, t, wnIterations, FOREVER);
        m_pblock->GetValue(pb_wn_relaxation_coeff, t, wnRelaxationCoeff, FOREVER);
        m_pblock->GetValue(pb_wn_use_total_coplanar_area, t, wnUseTotalCoplanarArea, FOREVER);
        m_pblock->GetValue(pb_wn_display_normals, t, wnDisplayNormals, FOREVER);
        m_pblock->GetValue(pb_wn_normal_length, t, wnNormalLength, FOREVER);
        if (useRenderIterations && GetCOREInterface7()->IsRenderActive())
            maxLevel = renderIterations;
        maxLevel = std::max(0, std::min(6, maxLevel));
        vertexBoundary = std::max(0, std::min(1, vertexBoundary));
        fvarBoundary = std::max(0, std::min(5, fvarBoundary));
        creaseMethod = std::max(0, std::min(1, creaseMethod));

        std::vector<int> verticesPerFace(source.numf);
        std::vector<OpenSubdiv::Far::Index> faceVertexIndices;
        faceVertexIndices.reserve(source.numf * 4);
        for (int face = 0; face < source.numf; ++face)
        {
            const MNFace& sourceFace = source.f[face];
            if (sourceFace.deg < 3)
                return;

            verticesPerFace[face] = sourceFace.deg;
            for (int corner = 0; corner < sourceFace.deg; ++corner)
                faceVertexIndices.push_back(sourceFace.vtx[corner]);
        }

        std::vector<FaceVaryingInput> fvarInputs;
        for (int mapChannel = -NUM_HIDDENMAPS; mapChannel < source.MNum(); ++mapChannel)
        {
            MNMap* map = source.M(mapChannel);
            if (!map || map->GetFlag(MN_DEAD) || map->numv <= 0 || map->numf < source.numf)
                continue;

            FaceVaryingInput input;
            input.mapChannel = mapChannel;
            input.values.assign(map->v, map->v + map->numv);
            input.indices.reserve(faceVertexIndices.size());

            bool valid = true;
            for (int face = 0; face < source.numf && valid; ++face)
            {
                const MNMapFace& mapFace = map->f[face];
                if (mapFace.deg != source.f[face].deg)
                {
                    valid = false;
                    break;
                }
                for (int corner = 0; corner < mapFace.deg; ++corner)
                {
                    if (mapFace.tv[corner] < 0 || mapFace.tv[corner] >= map->numv)
                    {
                        valid = false;
                        break;
                    }
                    input.indices.push_back(mapFace.tv[corner]);
                }
            }

            if (valid)
                fvarInputs.push_back(std::move(input));
        }

        const float* sourceEdgeCrease =
            source.eDataSupport(EDATA_CREASE) ? source.edgeFloat(EDATA_CREASE) : nullptr;
        const float* sourceVertexCrease =
            source.vDataSupport(VDATA_CREASE) ? source.vertexFloat(VDATA_CREASE) : nullptr;

        std::vector<OpenSubdiv::Far::Index> creaseVertexPairs;
        std::vector<float> creaseSharpness;
        if (useEdgeCrease && sourceEdgeCrease)
        {
            creaseVertexPairs.reserve(source.nume * 2);
            creaseSharpness.reserve(source.nume);
            for (int edge = 0; edge < source.nume; ++edge)
            {
                const float crease = Clamp01(sourceEdgeCrease[edge]);
                if (crease <= 0.0f)
                    continue;

                creaseVertexPairs.push_back(source.e[edge].v1);
                creaseVertexPairs.push_back(source.e[edge].v2);
                creaseSharpness.push_back(crease * kOpenSubdivInfiniteSharpness);
            }
        }

        std::vector<OpenSubdiv::Far::Index> cornerVertexIndices;
        std::vector<float> cornerSharpness;
        if (useVertexCrease && sourceVertexCrease)
        {
            cornerVertexIndices.reserve(source.numv);
            cornerSharpness.reserve(source.numv);
            for (int vertex = 0; vertex < source.numv; ++vertex)
            {
                const float crease = Clamp01(sourceVertexCrease[vertex]);
                if (crease <= 0.0f)
                    continue;

                cornerVertexIndices.push_back(vertex);
                cornerSharpness.push_back(crease * kOpenSubdivInfiniteSharpness);
            }
        }

        std::vector<OpenSubdiv::Far::TopologyDescriptor::FVarChannel> fvarChannels(fvarInputs.size());
        for (std::size_t channel = 0; channel < fvarInputs.size(); ++channel)
        {
            fvarChannels[channel].numValues = static_cast<int>(fvarInputs[channel].values.size());
            fvarChannels[channel].valueIndices = fvarInputs[channel].indices.data();
        }

        OpenSubdiv::Far::TopologyDescriptor descriptor;
        descriptor.numVertices = source.numv;
        descriptor.numFaces = source.numf;
        descriptor.numVertsPerFace = verticesPerFace.data();
        descriptor.vertIndicesPerFace = faceVertexIndices.data();
        descriptor.numCreases = static_cast<int>(creaseSharpness.size());
        descriptor.creaseVertexIndexPairs =
            creaseVertexPairs.empty() ? nullptr : creaseVertexPairs.data();
        descriptor.creaseWeights = creaseSharpness.empty() ? nullptr : creaseSharpness.data();
        descriptor.numCorners = static_cast<int>(cornerSharpness.size());
        descriptor.cornerVertexIndices =
            cornerVertexIndices.empty() ? nullptr : cornerVertexIndices.data();
        descriptor.cornerWeights = cornerSharpness.empty() ? nullptr : cornerSharpness.data();
        descriptor.numFVarChannels = static_cast<int>(fvarChannels.size());
        descriptor.fvarChannels = fvarChannels.empty() ? nullptr : fvarChannels.data();

        OpenSubdiv::Sdc::Options schemeOptions;
        schemeOptions.SetVtxBoundaryInterpolation(
            vertexBoundary == 0
                ? OpenSubdiv::Sdc::Options::VTX_BOUNDARY_EDGE_ONLY
                : OpenSubdiv::Sdc::Options::VTX_BOUNDARY_EDGE_AND_CORNER);
        schemeOptions.SetFVarLinearInterpolation(
            static_cast<OpenSubdiv::Sdc::Options::FVarLinearInterpolation>(fvarBoundary));
        schemeOptions.SetCreasingMethod(
            creaseMethod == 0
                ? OpenSubdiv::Sdc::Options::CREASE_UNIFORM
                : OpenSubdiv::Sdc::Options::CREASE_CHAIKIN);
        schemeOptions.SetTriangleSubdivision(
            smoothTriangles
                ? OpenSubdiv::Sdc::Options::TRI_SUB_SMOOTH
                : OpenSubdiv::Sdc::Options::TRI_SUB_CATMARK);

        using DescriptorFactory = OpenSubdiv::Far::TopologyRefinerFactory<
            OpenSubdiv::Far::TopologyDescriptor>;
        std::unique_ptr<OpenSubdiv::Far::TopologyRefiner> refiner(
            DescriptorFactory::Create(
                descriptor,
                DescriptorFactory::Options(OpenSubdiv::Sdc::SCHEME_CATMARK, schemeOptions)));
        if (!refiner)
            return;

        if (maxLevel > 0)
        {
            OpenSubdiv::Far::TopologyRefiner::UniformOptions refineOptions(maxLevel);
            refineOptions.fullTopologyInLastLevel = true;
            refiner->RefineUniform(refineOptions);
        }

        const int totalVertexCount = refiner->GetNumVerticesTotal();
        std::vector<OsdVec3> positions(totalVertexCount);
        for (int vertex = 0; vertex < source.numv; ++vertex)
            positions[vertex] = OsdVec3(source.P(vertex));

        OpenSubdiv::Far::PrimvarRefiner primvarRefiner(*refiner);
        OsdVec3* positionSource = positions.data();
        for (int level = 1; level <= maxLevel; ++level)
        {
            const OpenSubdiv::Far::TopologyLevel& parentLevel =
                refiner->GetLevel(level - 1);
            OsdVec3* positionDestination =
                positionSource + parentLevel.GetNumVertices();
            primvarRefiner.Interpolate(level, positionSource, positionDestination);
            positionSource = positionDestination;
        }

        std::vector<std::vector<OsdVec3>> fvarBuffers(fvarInputs.size());
        for (std::size_t channel = 0; channel < fvarInputs.size(); ++channel)
        {
            std::vector<OsdVec3>& buffer = fvarBuffers[channel];
            buffer.resize(refiner->GetNumFVarValuesTotal(static_cast<int>(channel)));
            for (std::size_t value = 0; value < fvarInputs[channel].values.size(); ++value)
                buffer[value] = OsdVec3(fvarInputs[channel].values[value]);

            OsdVec3* sourceValues = buffer.data();
            for (int level = 1; level <= maxLevel; ++level)
            {
                OsdVec3* destinationValues =
                    sourceValues +
                    refiner->GetLevel(level - 1).GetNumFVarValues(static_cast<int>(channel));
                primvarRefiner.InterpolateFaceVarying(
                    level,
                    sourceValues,
                    destinationValues,
                    static_cast<int>(channel));
                sourceValues = destinationValues;
            }
        }

        const OpenSubdiv::Far::TopologyLevel& finalLevel = refiner->GetLevel(maxLevel);
        const int finalVertexCount = finalLevel.GetNumVertices();
        const int finalFaceCount = finalLevel.GetNumFaces();
        const int finalVertexOffset = totalVertexCount - finalVertexCount;
        const std::vector<int> faceSources =
            PropagateFaceSources(*refiner, source.numf, maxLevel);

        // Only Vertex Weight affects this modifier's surface. Edge Weight is
        // propagated unchanged for modifiers above OpenSubdivPro.

        // Preserve the Pixar OpenSubdiv surface (including all crease data)
        // and add only the native MeshSmooth weighted-vs-neutral difference.
        // Weighted and neutral NURMS meshes contain the same crease channels,
        // so changing Weight cannot replace or globally alter the crease shape.
        std::vector<Point3> nativeVertexWeightDeltas;
        if (BuildNativeVertexWeightDeltas(
                source,
                *refiner,
                maxLevel,
                nativeVertexWeightDeltas) &&
            nativeVertexWeightDeltas.size() ==
                static_cast<std::size_t>(finalVertexCount))
        {
            for (int vertex = 0; vertex < finalVertexCount; ++vertex)
            {
                OsdVec3& position = positions[finalVertexOffset + vertex];
                const Point3 result =
                    GetOsdPoint(position) + nativeVertexWeightDeltas[vertex];
                if (IsFinitePoint(result))
                    SetOsdPoint(position, result);
            }
        }

        MNMesh output;
        output.dispFlags = source.dispFlags;
        output.setNumVerts(finalVertexCount);
        for (int vertex = 0; vertex < finalVertexCount; ++vertex)
        {
            const OsdVec3& position = positions[finalVertexOffset + vertex];
            output.P(vertex) = Point3(position.value[0], position.value[1], position.value[2]);
        }

        output.setNumFaces(finalFaceCount);
        for (int face = 0; face < finalFaceCount; ++face)
        {
            const OpenSubdiv::Far::ConstIndexArray faceVertices =
                finalLevel.GetFaceVertices(face);
            std::vector<int> maxFaceVertices(faceVertices.size());
            for (int corner = 0; corner < faceVertices.size(); ++corner)
                maxFaceVertices[corner] = faceVertices[corner];

            output.f[face].MakePoly(
                static_cast<int>(maxFaceVertices.size()),
                maxFaceVertices.data());

            const int sourceFaceIndex =
                face < static_cast<int>(faceSources.size()) ? faceSources[face] : 0;
            output.f[face].material = source.f[sourceFaceIndex].material;
            output.f[face].smGroup = 1u;
        }

        output.SetMapNum(source.MNum());
        for (std::size_t channel = 0; channel < fvarInputs.size(); ++channel)
        {
            const int mapChannel = fvarInputs[channel].mapChannel;
            MNMap* outputMap = output.M(mapChannel);
            if (!outputMap)
                continue;

            outputMap->ClearFlag(MN_DEAD);
            const int finalValueCount =
                finalLevel.GetNumFVarValues(static_cast<int>(channel));
            const int finalValueOffset =
                refiner->GetNumFVarValuesTotal(static_cast<int>(channel)) - finalValueCount;
            outputMap->setNumVerts(finalValueCount);
            for (int value = 0; value < finalValueCount; ++value)
            {
                const OsdVec3& refinedValue = fvarBuffers[channel][finalValueOffset + value];
                outputMap->v[value] =
                    UVVert(refinedValue.value[0], refinedValue.value[1], refinedValue.value[2]);
            }

            outputMap->setNumFaces(finalFaceCount);
            for (int face = 0; face < finalFaceCount; ++face)
            {
                const OpenSubdiv::Far::ConstIndexArray values =
                    finalLevel.GetFaceFVarValues(face, static_cast<int>(channel));
                outputMap->f[face].SetSize(values.size());
                for (int corner = 0; corner < values.size(); ++corner)
                    outputMap->f[face].tv[corner] = values[corner];
            }
        }

        output.FillInMesh();

        const bool sourceHasVertexWeight = source.vDataSupport(VDATA_WEIGHT);
        const bool sourceHasVertexCrease = sourceVertexCrease != nullptr;
        std::vector<float> baseVertexWeight(source.numv, 1.0f);
        std::vector<float> baseVertexCrease(source.numv, 0.0f);
        if (sourceHasVertexWeight)
        {
            const float* values = source.vertexFloat(VDATA_WEIGHT);
            std::copy(values, values + source.numv, baseVertexWeight.begin());
        }
        if (sourceVertexCrease)
            std::copy(sourceVertexCrease, sourceVertexCrease + source.numv, baseVertexCrease.begin());

        const std::vector<float> finalVertexWeight =
            PropagateVertexData(*refiner, std::move(baseVertexWeight), maxLevel, 1.0f);
        const std::vector<float> finalVertexCrease =
            PropagateVertexData(*refiner, std::move(baseVertexCrease), maxLevel);
        const std::vector<float> finalVertexCage =
            PropagateVertexData(
                *refiner,
                std::vector<float>(source.numv, 1.0f),
                maxLevel);

        std::unordered_map<std::uint64_t, int> sourceEdgeByVertices;
        sourceEdgeByVertices.reserve(source.nume);
        for (int edge = 0; edge < source.nume; ++edge)
            sourceEdgeByVertices[EdgeKey(source.e[edge].v1, source.e[edge].v2)] = edge;

        const OpenSubdiv::Far::TopologyLevel& baseLevel = refiner->GetLevel(0);
        std::vector<float> baseEdgeWeight(baseLevel.GetNumEdges(), 1.0f);
        std::vector<float> baseEdgeCrease(baseLevel.GetNumEdges(), 0.0f);
        std::vector<float> baseEdgeDepth(baseLevel.GetNumEdges(), 0.5f);
        std::vector<float> baseEdgeHard(baseLevel.GetNumEdges(), 0.0f);
        std::vector<float> baseEdgeCage(baseLevel.GetNumEdges(), 0.0f);
        const float* sourceEdgeWeight =
            source.eDataSupport(EDATA_KNOT) ? source.edgeFloat(EDATA_KNOT) : nullptr;
        const float* sourceEdgeDepth =
            source.eDataSupport(EDATA_DEPTH) ? source.edgeFloat(EDATA_DEPTH) : nullptr;

        for (int edge = 0; edge < baseLevel.GetNumEdges(); ++edge)
        {
            const OpenSubdiv::Far::ConstIndexArray vertices = baseLevel.GetEdgeVertices(edge);
            if (vertices.size() != 2)
                continue;

            const auto found =
                sourceEdgeByVertices.find(EdgeKey(vertices[0], vertices[1]));
            if (found == sourceEdgeByVertices.end())
                continue;

            const int sourceEdgeIndex = found->second;
            const MNEdge& sourceMeshEdge = source.e[sourceEdgeIndex];
            if (sourceEdgeWeight)
                baseEdgeWeight[edge] = sourceEdgeWeight[sourceEdgeIndex];
            if (sourceEdgeCrease)
                baseEdgeCrease[edge] = sourceEdgeCrease[sourceEdgeIndex];
            if (sourceEdgeDepth)
                baseEdgeDepth[edge] = sourceEdgeDepth[sourceEdgeIndex];
            if (sourceMeshEdge.f1 >= 0 && sourceMeshEdge.f2 >= 0)
            {
                const DWORD smoothingA = source.f[sourceMeshEdge.f1].smGroup;
                const DWORD smoothingB = source.f[sourceMeshEdge.f2].smGroup;
                baseEdgeHard[edge] = (smoothingA & smoothingB) == 0 ? 1.0f : 0.0f;
            }
            baseEdgeCage[edge] = 1.0f;
        }

        const std::vector<float> finalEdgeWeight =
            PropagateEdgeData(*refiner, std::move(baseEdgeWeight), maxLevel, 1.0f);
        const std::vector<float> finalEdgeCrease =
            PropagateEdgeData(*refiner, std::move(baseEdgeCrease), maxLevel);
        const std::vector<float> finalEdgeDepth =
            PropagateEdgeData(*refiner, std::move(baseEdgeDepth), maxLevel, 0.5f);
        const std::vector<float> finalEdgeHard =
            PropagateEdgeData(*refiner, std::move(baseEdgeHard), maxLevel);
        const std::vector<float> finalEdgeCage =
            PropagateEdgeData(*refiner, std::move(baseEdgeCage), maxLevel);

        std::unordered_map<std::uint64_t, int> finalEdgeByVertices;
        finalEdgeByVertices.reserve(finalLevel.GetNumEdges());
        for (int edge = 0; edge < finalLevel.GetNumEdges(); ++edge)
        {
            const OpenSubdiv::Far::ConstIndexArray vertices = finalLevel.GetEdgeVertices(edge);
            if (vertices.size() == 2)
                finalEdgeByVertices[EdgeKey(vertices[0], vertices[1])] = edge;
        }

        output.setNumVData(VDATA_CREASE + 1);
        if (sourceHasVertexWeight)
            output.setVDataSupport(VDATA_WEIGHT);
        if (sourceHasVertexCrease)
            output.setVDataSupport(VDATA_CREASE);
        float* outputVertexWeight =
            sourceHasVertexWeight ? output.vertexFloat(VDATA_WEIGHT) : nullptr;
        float* outputVertexCrease =
            sourceHasVertexCrease ? output.vertexFloat(VDATA_CREASE) : nullptr;
        for (int vertex = 0; vertex < output.numv; ++vertex)
        {
            if (outputVertexWeight)
                outputVertexWeight[vertex] =
                    vertex < static_cast<int>(finalVertexWeight.size()) ? finalVertexWeight[vertex] : 1.0f;
            if (outputVertexCrease)
                outputVertexCrease[vertex] =
                    vertex < static_cast<int>(finalVertexCrease.size()) ? finalVertexCrease[vertex] : 0.0f;
            output.v[vertex].SetFlag(
                MN_VERT_SUBDIVISION_CORNER,
                vertex < static_cast<int>(finalVertexCage.size()) &&
                    finalVertexCage[vertex] > 0.5f);
        }

        output.setNumEData(EDATA_DEPTH + 1);
        const bool sourceHasEdgeWeight = sourceEdgeWeight != nullptr;
        const bool sourceHasEdgeCrease = sourceEdgeCrease != nullptr;
        const bool sourceHasEdgeDepth = sourceEdgeDepth != nullptr;
        if (sourceHasEdgeWeight)
            output.setEDataSupport(EDATA_KNOT);
        if (sourceHasEdgeCrease)
            output.setEDataSupport(EDATA_CREASE);
        if (sourceHasEdgeDepth)
            output.setEDataSupport(EDATA_DEPTH);
        float* outputEdgeWeight =
            sourceHasEdgeWeight ? output.edgeFloat(EDATA_KNOT) : nullptr;
        float* outputEdgeCrease =
            sourceHasEdgeCrease ? output.edgeFloat(EDATA_CREASE) : nullptr;
        float* outputEdgeDepth =
            sourceHasEdgeDepth ? output.edgeFloat(EDATA_DEPTH) : nullptr;
        for (int edge = 0; edge < output.nume; ++edge)
        {
            float weight = 1.0f;
            float crease = 0.0f;
            float depth = 0.5f;
            bool hard = false;
            bool cage = false;
            const auto found =
                finalEdgeByVertices.find(EdgeKey(output.e[edge].v1, output.e[edge].v2));
            if (found != finalEdgeByVertices.end())
            {
                const int osdEdge = found->second;
                if (osdEdge < static_cast<int>(finalEdgeWeight.size()))
                    weight = finalEdgeWeight[osdEdge];
                if (osdEdge < static_cast<int>(finalEdgeCrease.size()))
                    crease = finalEdgeCrease[osdEdge];
                if (osdEdge < static_cast<int>(finalEdgeDepth.size()))
                    depth = finalEdgeDepth[osdEdge];
                if (osdEdge < static_cast<int>(finalEdgeHard.size()))
                    hard = finalEdgeHard[osdEdge] > 0.5f;
                if (osdEdge < static_cast<int>(finalEdgeCage.size()))
                    cage = finalEdgeCage[osdEdge] > 0.5f;
            }
            if (outputEdgeWeight)
                outputEdgeWeight[edge] = weight;
            if (outputEdgeCrease)
                outputEdgeCrease[edge] = crease;
            if (outputEdgeDepth)
                outputEdgeDepth[edge] = depth;
            output.e[edge].SetFlag(MN_USER, hard);
            output.e[edge].SetFlag(MN_EDGE_SUBDIVISION_BOUNDARY, cage);
        }

        // Match 3ds Max's native OpenSubdiv/Edit Poly isoline implementation.
        // This display-only flag hides generated subdivision interiors while
        // preserving real edge visibility, shading, and downstream channels.
        if (isolineDisplay)
            output.SetDispFlag(MNDISP_HIDE_SUBDIVISION_INTERIORS);
        else
            output.ClearDispFlag(MNDISP_HIDE_SUBDIVISION_INTERIORS);

        // Editable Poly's Hard/Smooth buttons are represented by the smoothing
        // relationship of adjacent faces. Rebuild those groups from the direct
        // descendants of the original hard edges, just as MeshSmooth does.
        output.SmoothByCreases(MN_USER);
        for (int edge = 0; edge < output.nume; ++edge)
            output.e[edge].ClearFlag(MN_USER);

        output.InvalidateGeomCache();
        polyObject->GetMesh() = output;
        polyObject->GetMesh().InvalidateHardwareMesh();

        if (smoothResult)
        {
            auto* weightedNormals = EnsureNativeNormals(t);
            if (weightedNormals)
            {
                // Old degree-valued animated angles stay driven by the legacy
                // controller; other parameters are owned by the native UI.
                if (Control* angle = m_pblock->GetControllerByID(pb_wn_hard_edge_angle);
                    angle && angle->IsAnimated())
                    weightedNormals->GetParamBlock(0)->SetValueByName(
                        _T("hardEdgeAngle"), DegToRad(wnHardEdgeAngle), t);
                ModContext& normalsContext = normalsData->normals;
                delete normalsContext.tm;
                normalsContext.tm = nullptr;
                delete normalsContext.box;
                normalsContext.box = nullptr;
                if (context.tm)
                    normalsContext.tm = new Matrix3(*context.tm);
                if (context.box)
                    normalsContext.box = new Box3(*context.box);
                weightedNormals->NotifyInputChanged(FOREVER, PART_ALL, REFMSG_CHANGE, &normalsContext);
                weightedNormals->ModifyObject(t, normalsContext, os, node);
                if (os->obj && os->obj->IsSubClassOf(polyObjectClassID))
                    normalsData->displayMesh = static_cast<PolyObject*>(os->obj)->GetMesh();
                normalsData->evaluated = true;
            }
        }

        // Native Editable Poly display path: see SDK sample
        // editablepoly/polyedops.cpp, EditPolyObject::UpdateEdgeColorDisplay.
        if (!os->obj || !os->obj->IsSubClassOf(polyObjectClassID))
            return;
        polyObject = static_cast<PolyObject*>(os->obj);
        MNMesh& displayMesh = polyObject->GetMesh();
        displayMesh.ClearDispFlag(MNDISP_USE_EDGE_COLORS);
        if (m_pblock->GetInt(pb_display_hard_edges, t))
        {
            const Color color = m_pblock->GetColor(pb_hard_edge_color, t);
            const auto component = [](float value) -> unsigned char
            {
                if (!std::isfinite(value))
                    return 1;
                return static_cast<unsigned char>(std::clamp(value * 255.0f, 1.0f, 255.0f));
            };
            const Color24 hardColor(component(color.r), component(color.g), component(color.b));
            const Color24 smoothColor(0, 0, 0);
            displayMesh.setEDataSupport(EDATA_COLOR);
            auto* colors = reinterpret_cast<Color24*>(displayMesh.edgeData(EDATA_COLOR));
            if (colors)
            {
                for (int edge = 0; edge < displayMesh.nume; ++edge)
                {
                    colors[edge] = smoothColor;
                    const MNEdge& e = displayMesh.e[edge];
                    if (e.GetFlag(MN_DEAD) || e.f1 < 0 || e.f2 < 0 ||
                        e.f1 >= displayMesh.numf || e.f2 >= displayMesh.numf)
                        continue;
                    if (!(displayMesh.f[e.f1].smGroup & displayMesh.f[e.f2].smGroup))
                        colors[edge] = hardColor;
                }
                displayMesh.SetDispFlag(MNDISP_USE_EDGE_COLORS);
            }
        }
        displayMesh.InvalidateHardwareMesh();

        // Editing invalidates the upstream cache, not the evaluated output.
        const Interval validity = EvaluationValidity(t);
        polyObject->UpdateValidity(GEOM_CHAN_NUM, validity);
        polyObject->UpdateValidity(TOPO_CHAN_NUM, validity);
        polyObject->UpdateValidity(SELECT_CHAN_NUM, validity);
        polyObject->UpdateValidity(TEXMAP_CHAN_NUM, validity);
        polyObject->UpdateValidity(VERT_COLOR_CHAN_NUM, validity);
    }

private:
    void SetReference(int index, RefTargetHandle target) override
    {
        if (index == kPBlockRef)
            m_pblock = static_cast<IParamBlock2*>(target);
    }

    IParamBlock2* m_pblock = nullptr;
    IObjParam* m_editIP = nullptr;
};

namespace
{
QLabel* MakeRightLabel(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return label;
}

template <class SpinBox>
void ConfigureSpinBox(SpinBox* spinBox)
{
    spinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    spinBox->setMinimumWidth(86);
}

class GeneralControlsRollout final : public MaxSDK::QMaxParamBlockWidget
{
public:
    GeneralControlsRollout()
    {
        setObjectName(QStringLiteral("OpenSubdivProGeneralControls"));

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(4, 4, 4, 5);
        root->setSpacing(0);

        auto* mainGroup = new QGroupBox(tr("Main"), this);
        auto* grid = new QGridLayout(mainGroup);
        grid->setContentsMargins(10, 5, 10, 7);
        grid->setHorizontalSpacing(5);
        grid->setVerticalSpacing(3);
        grid->setColumnStretch(0, 1);
        grid->setColumnStretch(1, 1);

        auto* iterations = new MaxSDK::QmaxSpinBox(mainGroup);
        iterations->setObjectName(QStringLiteral("iterations"));
        ConfigureSpinBox(iterations);
        grid->addWidget(MakeRightLabel(tr("Iterations:"), mainGroup), 0, 0);
        grid->addWidget(iterations, 0, 1);

        m_useRenderIterations = new QCheckBox(tr("Render Iters:"), mainGroup);
        m_useRenderIterations->setObjectName(QStringLiteral("useRenderIterations"));
        m_renderIterations = new MaxSDK::QmaxSpinBox(mainGroup);
        m_renderIterations->setObjectName(QStringLiteral("renderIterations"));
        ConfigureSpinBox(m_renderIterations);
        grid->addWidget(m_useRenderIterations, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
        grid->addWidget(m_renderIterations, 1, 1);

        auto* isoline = new QCheckBox(tr("Isoline Display"), mainGroup);
        isoline->setObjectName(QStringLiteral("isolineDisplay"));
        grid->addWidget(isoline, 2, 0, 1, 2, Qt::AlignHCenter);

        QObject::connect(
            m_useRenderIterations,
            &QCheckBox::toggled,
            m_renderIterations,
            &QWidget::setEnabled);

        root->addWidget(mainGroup);
    }

    void PostConnectUI(const MapID) override
    {
        m_renderIterations->setEnabled(m_useRenderIterations->isChecked());
    }

private:
    QCheckBox* m_useRenderIterations = nullptr;
    MaxSDK::QmaxSpinBox* m_renderIterations = nullptr;
};

class OpenSubdivControlsRollout final : public MaxSDK::QMaxParamBlockWidget
{
public:
    OpenSubdivControlsRollout()
    {
        setObjectName(QStringLiteral("OpenSubdivProControls"));

        auto* grid = new QGridLayout(this);
        grid->setContentsMargins(10, 5, 10, 8);
        grid->setHorizontalSpacing(6);
        grid->setVerticalSpacing(5);
        grid->setColumnStretch(0, 1);
        grid->setColumnStretch(1, 1);

        auto* vertexBoundary = new QComboBox(this);
        vertexBoundary->setObjectName(QStringLiteral("vertexBoundary"));
        vertexBoundary->addItem(tr("Interp. Edges"), 0);
        vertexBoundary->addItem(tr("Interp. Edges And Corners"), 1);
        vertexBoundary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        grid->addWidget(MakeRightLabel(tr("Vertex Boundary:"), this), 0, 0);
        grid->addWidget(vertexBoundary, 0, 1);

        auto* uvBoundary = new QComboBox(this);
        uvBoundary->setObjectName(QStringLiteral("fvarBoundary"));
        uvBoundary->addItem(tr("Smooth All"), 0);
        uvBoundary->addItem(tr("Sharpen Corners"), 1);
        uvBoundary->addItem(tr("Sharpen Corners +1"), 2);
        uvBoundary->addItem(tr("Sharpen Corners +2"), 3);
        uvBoundary->addItem(tr("Sharpen All Boundaries"), 4);
        uvBoundary->addItem(tr("Bilinear All"), 5);
        uvBoundary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        grid->addWidget(MakeRightLabel(tr("UV Boundary:"), this), 1, 0);
        grid->addWidget(uvBoundary, 1, 1);

        auto* smoothTriangles = new QCheckBox(tr("Smooth Triangles"), this);
        smoothTriangles->setObjectName(QStringLiteral("smoothTriangles"));
        grid->addWidget(smoothTriangles, 2, 0, 1, 2, Qt::AlignHCenter);

        auto* creaseMethod = new QComboBox(this);
        creaseMethod->setObjectName(QStringLiteral("creaseMethod"));
        creaseMethod->addItem(tr("Uniform"), 0);
        creaseMethod->addItem(tr("Chaikin"), 1);
        creaseMethod->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        grid->addWidget(MakeRightLabel(tr("Crease:"), this), 3, 0);
        grid->addWidget(creaseMethod, 3, 1);
    }
};

class WeightedNormalsRollout final : public MaxSDK::QMaxParamBlockWidget
{
public:
    WeightedNormalsRollout()
    {
        setObjectName(QStringLiteral("OpenSubdivProWeightedNormals"));
        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(4, 4, 4, 4);
        auto* header = new QGridLayout();
        header->addWidget(new QLabel(tr("Weighted Normals"), this), 0, 0);
        header->setColumnStretch(0, 1);
        m_enable = new QCheckBox(tr("OFF"), this);
        m_enable->setObjectName(QStringLiteral("smoothResult"));
        m_enable->setToolTip(tr("Enable weighted normal calculation and normal display. Settings are preserved when OFF."));
        header->addWidget(m_enable, 0, 1, Qt::AlignRight | Qt::AlignVCenter);
        QObject::connect(m_enable, &QCheckBox::toggled, m_enable,
            [this](bool enabled) { m_enable->setText(enabled ? tr("ON") : tr("OFF")); });
        root->addLayout(header);
        auto* settings = new QWidget(this);
        auto* grid = new QGridLayout(settings);
        grid->setContentsMargins(4, 2, 4, 2);
        grid->setColumnStretch(1, 1);
        int line = 0;
        const auto check = [&](const char* name, const char* label)
        {
            auto* widget = new QCheckBox(tr(label), settings);
            widget->setObjectName(QString::fromLatin1(name));
            grid->addWidget(widget, line++, 0, 1, 2);
            return widget;
        };
        const auto number = [&](const char* name, const char* label, bool integer)
        {
            grid->addWidget(MakeRightLabel(tr(label), settings), line, 0);
            QWidget* widget;
            if (integer)
                widget = new MaxSDK::QmaxSpinBox(settings);
            else
                widget = new MaxSDK::QmaxDoubleSpinBox(settings);
            widget->setObjectName(QString::fromLatin1(name));
            grid->addWidget(widget, line++, 1);
            return widget;
        };
        grid->addWidget(new QLabel(tr("Weighting"), settings), line++, 0, 1, 2);
        auto* weighting = new QGridLayout();
        weighting->setContentsMargins(0, 0, 0, 0);
        const auto weightButton = [&](const char* name, const char* label, int column)
        {
            auto* button = new QToolButton(settings);
            button->setObjectName(QString::fromLatin1(name));
            button->setText(tr(label));
            button->setCheckable(true);
            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            weighting->addWidget(button, 0, column);
            weighting->setColumnStretch(column, 1);
        };
        weightButton("wnUseAreaWeight", "Area", 0);
        weightButton("wnUseAngleWeight", "Angle", 1);
        grid->addLayout(weighting, line++, 0, 1, 2);
        auto* convex = check("wnUseConvexAngle", "Use Convex Corner Angle");
        auto* angleWeight = settings->findChild<QToolButton*>(QStringLiteral("wnUseAngleWeight"));
        QObject::connect(angleWeight, &QToolButton::toggled, convex, &QWidget::setEnabled);
        check("wnSnapToLargestFace", "Snap to Largest Face");
        number("wnBlendingCoeff", "Blending:", false);
        check("wnUseSmoothingGroups", "Use Smoothing Groups");
        auto* seams = check("wnUseUVSeams", "Use UV Seams");
        auto* channel = number("wnUVChannel", "UV Channel:", true);
        auto* angle = check("wnUseHardEdgeAngle", "Use Hard Edge Angle");
        auto* angleValue = number("wnHardEdgeAngle", "Hard Edge Angle:", false);
        number("wnSmoothingCoeff", "Smoothing:", false);
        number("wnBoundaryCoeff", "Boundary Blending:", false);
        number("wnIterations", "Iterations:", true);
        number("wnRelaxationCoeff", "Relaxation:", false);
        // Keep the serialized parameter for scene compatibility, but do not expose it in the UI.
        auto* normals = check("wnDisplayNormals", "Display Normals");
        auto* length = number("wnNormalLength", "Normal Length:", false);
        m_dependencies = {{seams, channel}, {angle, angleValue}, {normals, length}};
        m_angleWeight = angleWeight;
        m_convex = convex;
        for (const auto& dependency : m_dependencies)
            QObject::connect(dependency.first, &QCheckBox::toggled, dependency.second, &QWidget::setEnabled);
        m_settings = settings;
        QObject::connect(m_enable, &QCheckBox::toggled, settings, &QWidget::setEnabled);
        root->addWidget(settings);
        auto* row = new QGridLayout();
        auto* color = new MaxSDK::QMaxColorSwatch(this);
        color->setObjectName(QStringLiteral("hardEdgeColor"));
        color->setFixedSize(32, 18);
        color->setTitle(tr("Hard Edge Color"));
        auto* display = new QCheckBox(tr("Display Hard Edges"), this);
        display->setObjectName(QStringLiteral("displayHardEdges"));
        row->addWidget(color, 0, 0);
        row->addWidget(display, 0, 1);
        row->setColumnStretch(1, 1);
        root->addLayout(row);
    }
    void PostConnectUI(const MapID) override
    {
        RefreshEnabledState();
    }
    void UpdateUI(const TimeValue) override
    {
        RefreshEnabledState();
    }
    void UpdateParameterUI(const TimeValue, const ParamID, const int) override
    {
        RefreshEnabledState();
    }

private:
    void RefreshEnabledState()
    {
        m_enable->setText(m_enable->isChecked() ? tr("ON") : tr("OFF"));
        m_settings->setEnabled(m_enable->isChecked());
        for (const auto& dependency : m_dependencies)
            dependency.second->setEnabled(dependency.first->isChecked());
        m_convex->setEnabled(m_angleWeight->isChecked());
    }
    QToolButton* m_angleWeight = nullptr;
    QCheckBox* m_convex = nullptr;
    QCheckBox* m_enable = nullptr;
    QWidget* m_settings = nullptr;
    std::vector<std::pair<QCheckBox*, QWidget*>> m_dependencies;
};
}

MaxSDK::QMaxParamBlockWidget* MeshSmoothProClassDesc::CreateQtWidget(
    ReferenceMaker&,
    IParamBlock2& paramBlock,
    const MapID paramMapID,
    MSTR& rollupTitle,
    int& rollupFlags,
    int& rollupCategory)
{
    if (paramBlock.ID() != mesh_smooth_pro_params)
        return nullptr;

    rollupFlags = 0;
    rollupCategory = ROLLUP_CAT_STANDARD;
    switch (paramMapID)
    {
    case map_general_controls:
        rollupTitle = _T("General Controls");
        return new GeneralControlsRollout();
    case map_opensubdiv_controls:
        rollupTitle = _T("OpenSubdiv Controls");
        return new OpenSubdivControlsRollout();
    case map_weighted_normals:
        rollupTitle = _T("Weighted Normals");
        return new WeightedNormalsRollout();
    default:
        return nullptr;
    }
}

static ParamBlockDesc2 g_paramBlock(
    mesh_smooth_pro_params, _T("meshSmoothProParams"), IDS_PARAMETERS, &g_classDesc,
    P_AUTO_CONSTRUCT | P_AUTO_UI_QT | P_MULTIMAP, kPBlockRef,
    3,
    map_general_controls,
    map_opensubdiv_controls,
    map_weighted_normals,

    pb_iterations, _T("iterations"), TYPE_INT, P_ANIMATABLE | P_RESET_DEFAULT, IDS_ITERATIONS,
        p_default, 1,
        p_range, 0, 6,
        p_end,

    pb_smoothness, _T("smoothness"), TYPE_FLOAT, P_ANIMATABLE | P_RESET_DEFAULT, IDS_SMOOTHNESS,
        p_default, 1.0f,
        p_range, 0.0f, 1.0f,
        p_end,

    pb_use_vertex_crease, _T("useVertexCrease"), TYPE_BOOL, P_RESET_DEFAULT, IDS_USE_VERTEX_CREASE,
        p_default, TRUE,
        p_end,

    pb_use_edge_crease, _T("useEdgeCrease"), TYPE_BOOL, P_RESET_DEFAULT, IDS_USE_EDGE_CREASE,
        p_default, TRUE,
        p_end,

    pb_smooth_result, _T("smoothResult"), TYPE_BOOL, P_RESET_DEFAULT, IDS_SMOOTH_RESULT,
        p_default, TRUE,
        p_end,

    pb_use_render_iterations, _T("useRenderIterations"), TYPE_BOOL, P_RESET_DEFAULT, IDS_USE_RENDER_ITERS,
        p_default, FALSE,
        p_end,

    pb_render_iterations, _T("renderIterations"), TYPE_INT, P_ANIMATABLE | P_RESET_DEFAULT, IDS_RENDER_ITERS,
        p_default, 0,
        p_range, 0, 6,
        p_end,

    pb_isoline_display, _T("isolineDisplay"), TYPE_BOOL, P_RESET_DEFAULT, IDS_ISOLINE_DISPLAY,
        p_default, FALSE,
        p_end,

    pb_vertex_boundary, _T("vertexBoundary"), TYPE_INT, P_RESET_DEFAULT, IDS_VERTEX_BOUNDARY,
        p_default, 0,
        p_range, 0, 1,
        p_end,

    pb_fvar_boundary, _T("fvarBoundary"), TYPE_INT, P_RESET_DEFAULT, IDS_FVAR_BOUNDARY,
        p_default, 4,
        p_range, 0, 5,
        p_end,

    pb_smooth_triangles, _T("smoothTriangles"), TYPE_BOOL, P_RESET_DEFAULT, IDS_SMOOTH_TRIANGLES,
        p_default, TRUE,
        p_end,

    pb_crease_method, _T("creaseMethod"), TYPE_INT, P_RESET_DEFAULT, IDS_CREASE_METHOD,
        p_default, 0,
        p_range, 0, 1,
        p_end,

    pb_wn_use_area_weight, _T("wnUseAreaWeight"), TYPE_BOOL, P_RESET_DEFAULT, IDS_WN_AREA,
        p_default, FALSE,
        p_end,

    pb_wn_use_angle_weight, _T("wnUseAngleWeight"), TYPE_BOOL, P_RESET_DEFAULT, IDS_WN_ANGLE,
        p_default, TRUE,
        p_end,

    pb_wn_use_convex_angle, _T("wnUseConvexAngle"), TYPE_BOOL, P_RESET_DEFAULT, IDS_WN_CONVEX,
        p_default, TRUE,
        p_end,

    pb_wn_snap_to_largest_face, _T("wnSnapToLargestFace"), TYPE_BOOL, P_RESET_DEFAULT, IDS_WN_SNAP,
        p_default, FALSE,
        p_end,

    pb_wn_blending_coeff, _T("wnBlendingCoeff"), TYPE_FLOAT, P_ANIMATABLE | P_RESET_DEFAULT, IDS_WN_BLENDING,
        p_default, 1.0f,
        p_range, 0.0f, 1.0f,
        p_end,

    pb_wn_use_smoothing_groups, _T("wnUseSmoothingGroups"), TYPE_BOOL, P_RESET_DEFAULT, IDS_WN_SMOOTH_GROUPS,
        p_default, TRUE,
        p_end,

    pb_wn_use_uv_seams, _T("wnUseUVSeams"), TYPE_BOOL, P_RESET_DEFAULT, IDS_WN_UV_SEAMS,
        p_default, FALSE,
        p_end,

    pb_wn_uv_channel, _T("wnUVChannel"), TYPE_INT, P_RESET_DEFAULT, IDS_WN_UV_CHANNEL,
        p_default, 1,
        p_range, 1, 99,
        p_end,

    pb_wn_use_hard_edge_angle, _T("wnUseHardEdgeAngle"), TYPE_BOOL, P_RESET_DEFAULT, IDS_WN_HARD_ANGLE_USE,
        p_default, FALSE,
        p_end,

    pb_wn_hard_edge_angle, _T("wnHardEdgeAngle"), TYPE_FLOAT, P_ANIMATABLE | P_RESET_DEFAULT, IDS_WN_HARD_ANGLE,
        p_default, 30.0f,
        p_range, 0.0f, 180.0f,
        p_end,

    pb_wn_smoothing_coeff, _T("wnSmoothingCoeff"), TYPE_FLOAT, P_ANIMATABLE | P_RESET_DEFAULT, IDS_WN_SMOOTHING,
        p_default, 1.0f,
        p_range, 0.0f, 1.0f,
        p_end,

    pb_wn_boundary_coeff, _T("wnBoundaryCoeff"), TYPE_FLOAT, P_ANIMATABLE | P_RESET_DEFAULT, IDS_WN_BOUNDARY_BLEND,
        p_default, 0.0f,
        p_range, 0.0f, 1.0f,
        p_end,

    pb_wn_iterations, _T("wnIterations"), TYPE_INT, P_ANIMATABLE | P_RESET_DEFAULT, IDS_WN_ITERATIONS,
        p_default, 10,
        p_range, 0, 100,
        p_end,

    pb_wn_relaxation_coeff, _T("wnRelaxationCoeff"), TYPE_FLOAT, P_ANIMATABLE | P_RESET_DEFAULT, IDS_WN_RELAXATION,
        p_default, 0.5f,
        p_range, 0.0f, 1.0f,
        p_end,

    pb_wn_use_total_coplanar_area, _T("wnUseTotalCoplanarArea"), TYPE_BOOL, P_RESET_DEFAULT, IDS_WN_COPLANAR_AREA,
        p_default, TRUE,
        p_end,

    pb_wn_display_normals, _T("wnDisplayNormals"), TYPE_BOOL, P_RESET_DEFAULT, IDS_WN_DISPLAY_NORMALS,
        p_default, FALSE,
        p_end,

    pb_wn_normal_length, _T("wnNormalLength"), TYPE_FLOAT, P_ANIMATABLE | P_RESET_DEFAULT, IDS_WN_NORMAL_LENGTH,
        p_default, 10.0f,
        p_range, 0.0f, 1000000.0f,
        p_end,

    pb_display_hard_edges, _T("displayHardEdges"), TYPE_BOOL, P_RESET_DEFAULT, IDS_DISPLAY_HARD_EDGES,
        p_default, FALSE,
        p_end,

    pb_hard_edge_color, _T("hardEdgeColor"), TYPE_RGBA, P_RESET_DEFAULT, IDS_HARD_EDGE_COLOR,
        p_default, Color(0.0f, 0.0f, 0.0f),
        p_end,

    pb_native_weighted_normals, _T("nativeWeightedNormals"), TYPE_REFTARG, 0, IDS_WN_ROLLOUT,
        p_end,

    p_end);

void* MeshSmoothProClassDesc::Create(BOOL)
{
    return new MeshSmoothPro();
}

extern "C" __declspec(dllexport) const TCHAR* LibDescription()
{
    return GetString(IDS_LIBDESCRIPTION);
}

extern "C" __declspec(dllexport) int LibNumberClasses()
{
    return 1;
}

extern "C" __declspec(dllexport) ClassDesc* LibClassDesc(int index)
{
    return index == 0 ? &g_classDesc : nullptr;
}

extern "C" __declspec(dllexport) ULONG LibVersion()
{
    return VERSION_3DSMAX;
}

extern "C" __declspec(dllexport) ULONG CanAutoDefer()
{
    return 1;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_instance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}












