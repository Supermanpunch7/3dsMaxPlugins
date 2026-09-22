#pragma once
#include <vector>
#include <algorithm>
// Experimental topology inference. Opposite edges in a quad form a strip.
// Collect ALL existing boundary seeds before assigning; conflicting strips
// remain unresolved. Do not recursively treat inferred parents as new seeds.
inline int ResolveWeightQuadStrips(MNMesh& dst, const float* weights,
    std::vector<int>& parents) {
    std::vector<std::vector<int>> opposite(dst.nume);
    for (int f=0;f<dst.numf;++f) {
        const auto& face=dst.f[f];
        if(face.deg!=4) continue;
        for(int i=0;i<2;++i) {
            int a=face.edg[i], b=face.edg[i+2];
            if(a<0 || b<0 || a>=dst.nume || b>=dst.nume) continue;
            opposite[a].push_back(b); opposite[b].push_back(a);
        }
    }
    const std::vector<int> initial=parents;
    std::vector<bool> seen(dst.nume,false);
    int filled=0;
    for(int seed=0;seed<dst.nume;++seed) {
        if(initial[seed]>=0 || seen[seed]) continue;
        std::vector<int> region(1,seed);
        seen[seed]=true;
        int parent=-1;
        bool conflict=false;
        for(size_t i=0;i<region.size();++i) for(int next:opposite[region[i]]) {
            int p=initial[next];
            if(p>=0) {
                if(parent<0) parent=p;
                else if(weights[parent]!=weights[p]) conflict=true;
            } else if(!seen[next]) { seen[next]=true; region.push_back(next); }
        }
        if(parent>=0 && !conflict) for(int e:region) { parents[e]=parent; ++filled; }
    }
    return filled;
}