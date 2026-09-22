#pragma once
#include <polyobj.h>
#include <vector>
#include <map>
#include <utility>
#include <algorithm>
#include <cstdio>

// Experimental small-offset correspondence, NOT general topology provenance.
// Never included by Stable. Unknown junction edges keep their native boundary.
inline bool TrialHard(MNMesh& m, int i) {
    auto& e = m.e[i];
    return e.f1 < 0 || e.f2 < 0 || !(m.f[e.f1].smGroup & m.f[e.f2].smGroup);
}
inline int ApplyNativeFourTrial(MNMesh& src, MNMesh& dst) {
    if (src.GetSpecifiedNormals() || dst.GetSpecifiedNormals()) return -1;
    if (src.numv > 4096 || dst.numv > 100000) return -2;
    std::vector<float> radius(src.numv, 1.e30f);
    for (int a=0; a<src.numv; ++a) for (int b=a+1; b<src.numv; ++b) {
        float d = Length(src.v[a].p-src.v[b].p)*0.2f;
        if (d < 1.e-7f) return -3;
        radius[a] = (std::min)(radius[a], d);
        radius[b] = (std::min)(radius[b], d);
    }
    std::vector<int> owner(dst.numv, -1);
    for (int v=0; v<dst.numv; ++v) for (int a=0; a<src.numv; ++a)
        if (Length(dst.v[v].p-src.v[a].p) < radius[a]) {
            if (owner[v] != -1) return -4;
            owner[v] = a;
        }
    std::map<std::pair<int,int>, int> sourceEdges;
    for (int e=0; e<src.nume; ++e) {
        auto key = std::minmax(src.e[e].v1, src.e[e].v2);
        if (!sourceEdges.emplace(key,e).second) return -5;
    }
    int resolved = 0; std::vector<int> parents(dst.nume, -1);
    std::vector<bool> hard(dst.nume);
    std::vector<std::vector<int>> forbidden(dst.numf);
    for (int e=0; e<dst.nume; ++e) {
        auto& edge = dst.e[e];
        hard[e] = TrialHard(dst,e);
        int a = owner[edge.v1], b = owner[edge.v2];
        if (a>=0 && b>=0 && a!=b) {
            auto found = sourceEdges.find(std::minmax(a,b));
            if (found != sourceEdges.end()) { parents[e]=found->second; hard[e]=TrialHard(src,found->second); ++resolved; }
        }
        if (hard[e] && edge.f1>=0 && edge.f2>=0) {
            forbidden[edge.f1].push_back(edge.f2);
            forbidden[edge.f2].push_back(edge.f1);
        }
    }
    if (!resolved) return 0;
    // Allocate shared bits for soft face pairs without joining any hard pair.
    // Validate every boundary before committing; failure leaves native output intact.
    std::vector<DWORD> groups(dst.numf,0);
    for (int e=0; e<dst.nume; ++e) {
        auto& edge=dst.e[e];
        int a=edge.f1,b=edge.f2;
        if (hard[e] || a<0 || b<0 || (groups[a]&groups[b])) continue;
        DWORD blocked=0;
        for (int f: forbidden[a]) blocked |= groups[f];
        for (int f: forbidden[b]) blocked |= groups[f];
        DWORD bit=1;
        while (bit && (bit&blocked)) bit <<= 1;
        if (!bit) return -6;
        groups[a] |= bit; groups[b] |= bit;
    }
    for (int e=0; e<dst.nume; ++e) {
        auto& edge=dst.e[e];
        if (edge.f1>=0 && edge.f2>=0 &&
            (!(groups[edge.f1]&groups[edge.f2])) != hard[e]) return -7;
    }
    // Stage on a copy so allocation/validation failure cannot partially commit.
MNMesh working(dst);
for (int ch : {0,1,3}) {
    if (!src.eDataSupport(ch)) continue;
    working.setEDataSupport(ch, TRUE);
    float* input = static_cast<float*>(src.edgeData(ch));
    float* output = static_cast<float*>(working.edgeData(ch));
    if (!input || !output) return -8;
    for (int e=0; e<working.nume; ++e)
        if (parents[e]>=0) output[e]=input[parents[e]];
}
for (int f=0; f<working.numf; ++f) working.f[f].smGroup=groups[f];
dst = working;
    dst.InvalidateGeomCache();
    return resolved;
}
inline void LogNativeFourTrial(int stage, int status) {
    wchar_t path[2048];
    DWORD n=GetEnvironmentVariableW(L"CHAMFERPRO_FOUR_LOG",path,2048);
    if (!n || n>=2048) return;
    FILE* f=nullptr;
    if (!_wfopen_s(&f,path,L"a") && f) {
        fprintf(f,"stage=%d resolved_or_rejection=%d\n",stage,status); fclose(f);
    }
}