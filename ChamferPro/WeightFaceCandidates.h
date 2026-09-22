#pragma once
#include <vector>
#include <algorithm>

// Experimental face adjacency evidence, not exact native chamfer provenance.
// Only use the ORIGINAL resolved parents; no recursive flood across junctions.
inline int ResolveWeightFaceCandidates(MNMesh& src, MNMesh& dst,
    const std::vector<int>& owner, std::vector<int>& parents) {
    std::vector<std::vector<int>> faces(dst.numf);
    for (int e=0; e<dst.nume; ++e) if (parents[e]>=0) {
        for (int f : {dst.e[e].f1, dst.e[e].f2}) if (f>=0) {
            auto& candidates=faces[f];
            if (std::find(candidates.begin(), candidates.end(), parents[e])==candidates.end())
                candidates.push_back(parents[e]);
        }
    }
    int added=0;
    for (int e=0; e<dst.nume; ++e) {
        if (parents[e]>=0) continue;
        const auto& edge=dst.e[e];
        if (edge.f1<0 || edge.f2<0) continue;
        int a=owner[edge.v1], b=owner[edge.v2];
        if (a<0 || a!=b) continue;
        int match=-1;
        for (int p:faces[edge.f1]) {
            const auto& source=src.e[p];
            if (source.v1!=a && source.v2!=a) continue;
            if (std::find(faces[edge.f2].begin(),faces[edge.f2].end(),p)==faces[edge.f2].end()) continue;
            if (match>=0) { match=-2; break; }
            match=p;
        }
        if (match>=0) { parents[e]=match; ++added; }
    }
    return added;
}