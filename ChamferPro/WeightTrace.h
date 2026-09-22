#pragma once
#include <polyobj.h>
#include <vector>
#include <map>
#include <utility>
#include <algorithm>
#include <cstdio>
#include "WeightAssignmentTrace.h"
#include "WeightFaceCandidates.h"
#include "WeightRegionConsensus.h"
#include "WeightQuadStrips.h"

// Experimental small-offset correspondence, NOT general topology provenance.
// Never included by Stable. Unknown junction edges keep their native boundary.
inline bool TrialHard(MNMesh& m, int i) {
    auto& e = m.e[i];
    return e.f1 < 0 || e.f2 < 0 || !(m.f[e.f1].smGroup & m.f[e.f2].smGroup);
}
inline int ApplyNativeCornerTrial(MNMesh& src, MNMesh& dst) {
    const bool preserveNormals = src.GetSpecifiedNormals() || dst.GetSpecifiedNormals();
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
    // A corner has a set of incident parents, not a unique nearest edge.
// Only unanimous incident states are inherited; mixed corners remain native.
std::vector<int> cornerState(src.numv, -1);
for (int e=0; e<src.nume; ++e) {
    int state = TrialHard(src,e) ? 1 : 0;
    for (int v : {src.e[e].v1, src.e[e].v2}) {
        if (cornerState[v] == -1) cornerState[v] = state;
        else if (cornerState[v] != state) cornerState[v] = 2;
    }
}
// Edge Properties Weight (EDATA_KNOT); crease/depth channels are untouched.
const float* sourceWeight = src.edgeFloat(EDATA_KNOT);
std::vector<int> weightParent(dst.nume, -1);
std::vector<int> cornerWeightParent(src.numv, -1);
if (sourceWeight) for (int e=0; e<src.nume; ++e) {
    for (int v : {src.e[e].v1, src.e[e].v2}) {
        int& parent = cornerWeightParent[v];
        if (parent == -1) parent = e;
        else if (parent >= 0 && sourceWeight[parent] != sourceWeight[e]) parent = -2;
    }
}
int resolved = 0;
    std::vector<bool> hard(dst.nume);
    std::vector<std::vector<int>> forbidden(dst.numf);
    for (int e=0; e<dst.nume; ++e) {
        auto& edge = dst.e[e];
        hard[e] = TrialHard(dst,e);
        int a = owner[edge.v1], b = owner[edge.v2];
        if (a>=0 && b>=0 && a!=b) {
            auto found = sourceEdges.find(std::minmax(a,b));
            if (found != sourceEdges.end()) { hard[e]=TrialHard(src,found->second); weightParent[e]=found->second; ++resolved; }
        }
        if (a>=0 && a==b && cornerState[a]>=0 && cornerState[a]<2) {
    hard[e] = cornerState[a] == 1;
    ++resolved;
}
// A corner has multiple candidate parents. Copy only an exactly unanimous value.
if (sourceWeight && a>=0 && a==b && cornerWeightParent[a]>=0)
    weightParent[e] = cornerWeightParent[a];
if (hard[e] && edge.f1>=0 && edge.f2>=0) {
            forbidden[edge.f1].push_back(edge.f2);
            forbidden[edge.f2].push_back(edge.f1);
        }
    }
    if (sourceWeight) ResolveWeightFaceCandidates(src, dst, owner, weightParent);
    if (sourceWeight) ResolveWeightQuadStrips(dst, sourceWeight, weightParent);
    if (sourceWeight) ResolveWeightRegionConsensus(dst, sourceWeight, weightParent);
    TraceWeightAssignments(src,dst,owner,weightParent);
    // Weight is an independent edge channel. Do not suppress it because normals
// exist or because a subsequent smoothing-group allocation cannot succeed.
if (sourceWeight) {
    dst.setEDataSupport(EDATA_KNOT);
    float* targetWeight = dst.edgeFloat(EDATA_KNOT);
    if (!targetWeight) return -8;
    for (int e=0; e<dst.nume; ++e)
        if (weightParent[e]>=0) targetWeight[e]=sourceWeight[weightParent[e]];
}
// Keep specified normals and all native smoothing groups unchanged.
if (preserveNormals) return resolved;
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
    // Commit only after the existing hard/soft validation succeeds.
if (sourceWeight) {
    dst.setEDataSupport(EDATA_KNOT);
    float* targetWeight = dst.edgeFloat(EDATA_KNOT);
    if (!targetWeight) return -8;
    for (int e=0; e<dst.nume; ++e)
        if (weightParent[e]>=0) targetWeight[e]=sourceWeight[weightParent[e]];
}
for (int f=0; f<dst.numf; ++f) dst.f[f].smGroup=groups[f];
    dst.InvalidateGeomCache();
    return resolved;
}
inline void LogNativeCornerTrial(int stage, int status) {
    wchar_t path[2048];
    DWORD n=GetEnvironmentVariableW(L"CHAMFERPRO_CORNER_LOG",path,2048);
    if (!n || n>=2048) return;
    FILE* f=nullptr;
    if (!_wfopen_s(&f,path,L"a") && f) {
        fprintf(f,"stage=%d resolved_or_rejection=%d\n",stage,status); fclose(f);
    }
}