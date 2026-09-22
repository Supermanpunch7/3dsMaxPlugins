#pragma once
#include <polyobj.h>
#include <vector>
#include <algorithm>
#include <cmath>

// Conservative geometric continuation, not exact native chamfer ancestry.
// Only short junction wires following two incident, nearly straight hard
// parents with identical weights qualify. Conflicting values reject the wire.
inline int RestoreCenterContinuation(MNMesh& src, MNMesh& dst) {
    const float* weights = src.edgeFloat(EDATA_KNOT);
    if (!weights) return 0;
    struct Path { Point3 center, a, b; float radius, weight; };
    std::vector<std::vector<int>> incident(src.numv);
    for (int e=0; e<src.nume; ++e) {
        if (src.e[e].GetFlag(MN_DEAD)) continue;
        if (!TrialHard(src,e)) continue;
        incident[src.e[e].v1].push_back(e);
        incident[src.e[e].v2].push_back(e);
    }
    std::vector<Path> paths;
    for (int v=0; v<src.numv; ++v) {
        auto& edges=incident[v];
        for (size_t i=0;i<edges.size();++i) for (size_t j=i+1;j<edges.size();++j) {
            int x=edges[i], y=edges[j];
            if (weights[x]!=weights[y] || !std::isfinite(weights[x])) continue;
            const auto& ex=src.e[x]; const auto& ey=src.e[y];
            Point3 a=src.v[ex.v1==v?ex.v2:ex.v1].p-src.v[v].p;
            Point3 b=src.v[ey.v1==v?ey.v2:ey.v1].p-src.v[v].p;
            float la=Length(a), lb=Length(b);
            if (la<1.e-6f || lb<1.e-6f) continue;
            a/=la; b/=lb;
            if (DotProd(a,b)>-0.95f) continue;
            paths.push_back({src.v[v].p,a,b,(std::min)(la,lb)*0.2f,weights[x]});
        }
    }
    dst.setEDataSupport(EDATA_KNOT);
    float* out=dst.edgeFloat(EDATA_KNOT);
    if (!out) return 0;
    int changed=0;
    for (int e=0;e<dst.nume;++e) {
        if (dst.e[e].GetFlag(MN_DEAD) || out[e]!=1.0f) continue;
        Point3 p=dst.v[dst.e[e].v1].p, q=dst.v[dst.e[e].v2].p;
        Point3 direction=q-p;
        float length=Length(direction);
        if (length<1.e-7f) continue;
        direction/=length;
        bool found=false, conflict=false;
        float value=1.0f;
        for (const auto& path:paths) {
            if (Length(p-path.center)>path.radius || Length(q-path.center)>path.radius) continue;
            if ((std::max)(std::fabs(DotProd(direction,path.a)),std::fabs(DotProd(direction,path.b)))<0.97f) continue;
            bool withinCorridor=true;
            for (const Point3& point:{p,q}) {
                Point3 d=point-path.center;
                float da=Length(d-path.a*(std::max)(0.0f,DotProd(d,path.a)));
                float db=Length(d-path.b*(std::max)(0.0f,DotProd(d,path.b)));
                if ((std::min)(da,db)>path.radius*0.10f) withinCorridor=false;
            }
            if (!withinCorridor) continue;
            if (found && value!=path.weight) { conflict=true; break; }
            found=true; value=path.weight;
        }
        if (found && !conflict && value!=1.0f) { out[e]=value; ++changed; }
    }
    return changed;
}