#pragma once
#include <vector>
// Conservative value inference, NOT unique-parent provenance. An unresolved
// connected region is filled only when every resolved boundary agrees exactly.
// Never change resolved edges, smoothing groups, normals or geometry.
inline int ResolveWeightRegionConsensus(MNMesh& dst, const float* weights,
    std::vector<int>& parents) {
    std::vector<std::vector<int>> incident(dst.numv);
    for (int e=0;e<dst.nume;++e) {
        incident[dst.e[e].v1].push_back(e);
        incident[dst.e[e].v2].push_back(e);
    }
    const std::vector<int> initial=parents;
    std::vector<bool> visited(dst.nume,false);
    int filled=0;
    for(int seed=0;seed<dst.nume;++seed) {
        if(initial[seed]>=0 || visited[seed]) continue;
        std::vector<int> region(1,seed);
        visited[seed]=true;
        int candidate=-1;
        bool conflict=false;
        for(size_t i=0;i<region.size();++i) {
            const auto& edge=dst.e[region[i]];
            for(int v : {edge.v1,edge.v2}) for(int next:incident[v]) {
                int p=initial[next];
                if(p>=0) {
                    if(candidate<0) candidate=p;
                    else if(weights[candidate]!=weights[p]) conflict=true;
                } else if(!visited[next]) {
                    visited[next]=true;
                    region.push_back(next);
                }
            }
        }
        if(candidate>=0 && !conflict) for(int e:region) {
            parents[e]=candidate;
            ++filled;
        }
    }
    return filled;
}