#pragma once
#include <mnmesh.h>
#include <MNNormalSpec.h>
#include <vector>

inline int LineageCorner(MNMesh& mesh, int face, int vertex) {
    if (face < 0 || face >= mesh.numf) return -1;
    for (int c = 0; c < mesh.f[face].deg; ++c)
        if (mesh.f[face].vtx[c] == vertex) return c;
    return -1;
}

// Shading boundary, independent of Weight, Crease and Depth.
inline bool LineageHard(MNMesh& mesh, int edge) {
    const auto& e = mesh.e[edge];
    if (e.f1 < 0 || e.f2 < 0) return true;
    if ((mesh.f[e.f1].smGroup & mesh.f[e.f2].smGroup) == 0) return true;
    auto* normals = mesh.GetSpecifiedNormals();
    if (!normals || normals->GetNumFaces() != mesh.numf) return false;
    for (int v : {e.v1, e.v2}) {
        int a = LineageCorner(mesh, e.f1, v), b = LineageCorner(mesh, e.f2, v);
        if (a < 0 || b < 0) continue;
        auto& fa = normals->Face(e.f1);
        auto& fb = normals->Face(e.f2);
        if (a >= fa.GetDegree() || b >= fb.GetDegree()) continue;
        int na = fa.GetNormalID(a), nb = fb.GetNormalID(b);
        if (na >= 0 && nb >= 0 && na != nb && fa.GetSpecified(a) && fb.GetSpecified(b)) return true;
    }
    return false;
}

// Construct corner-normal islands without crossing inherited hard edges.
// A hard boundary reconnected through a soft fan is not silently smoothed:
// reject the transaction until that topology has a defined split policy.
inline bool LineageBuildNormals(MNMesh& mesh, const std::vector<bool>& hard) {
    std::vector<int> offset(int(mesh.numf) + 1, 0);
    for (int f = 0; f < mesh.numf; ++f) offset[f+1] = offset[f] + mesh.f[f].deg;
    std::vector<int> parent(offset.back());
    for (int i = 0; i < int(parent.size()); ++i) parent[i] = i;
    auto root = [&](int x) {
        while (parent[x] != x) { parent[x] = parent[parent[x]]; x = parent[x]; }
        return x;
    };
    for (int e = 0; e < mesh.nume; ++e) {
        const auto& edge = mesh.e[e];
        if (hard[e] || edge.f1 < 0 || edge.f2 < 0) continue;
        for (int v : {edge.v1, edge.v2}) {
            int a = LineageCorner(mesh, edge.f1, v), b = LineageCorner(mesh, edge.f2, v);
            if (a < 0 || b < 0) return false;
            parent[root(offset[edge.f1]+a)] = root(offset[edge.f2]+b);
        }
    }
    for (int e = 0; e < mesh.nume; ++e) {
        const auto& edge = mesh.e[e];
        if (!hard[e] || edge.f1 < 0 || edge.f2 < 0) continue;
        for (int v : {edge.v1, edge.v2}) {
            int a = LineageCorner(mesh, edge.f1, v), b = LineageCorner(mesh, edge.f2, v);
            if (a < 0 || b < 0 || root(offset[edge.f1]+a) == root(offset[edge.f2]+b)) return false;
        }
    }
    std::vector<Point3> sums(parent.size(), Point3(0,0,0));
    for (int f = 0; f < mesh.numf; ++f) {
        Point3 normal = mesh.GetFaceNormal(f, false);
        for (int c = 0; c < mesh.f[f].deg; ++c) sums[root(offset[f]+c)] += normal;
    }
    mesh.ClearSpecifiedNormals();
    mesh.SpecifyNormals();
    auto* normals = mesh.GetSpecifiedNormals();
    if (!normals || !normals->SetNumFaces(mesh.numf)) return false;
    std::vector<int> ids(parent.size(), -1);
    for (int f = 0; f < mesh.numf; ++f) {
        // Explicit normals, rather than a 32-bit smoothing-group coloring,
        // encode the per-edge boundaries in this experiment.
        mesh.f[f].smGroup = 1;
        normals->Face(f).SetDegree(mesh.f[f].deg);
        for (int c = 0; c < mesh.f[f].deg; ++c) {
            int r = root(offset[f]+c);
            if (ids[r] < 0) {
                if (Length(sums[r]) < 1.0e-12f) return false;
                ids[r] = normals->NewNormal(sums[r], true);
            }
            normals->SetNormalIndex(f, c, ids[r]);
        }
    }
    normals->SetAllExplicit(true);
    return true;
}