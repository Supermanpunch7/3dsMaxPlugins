#pragma once
#include <cstdio>
// Opt-in diagnostic: no mutation of geometry, normals or edge data.
inline void TraceWeightAssignments(MNMesh& src, MNMesh& dst,
    const std::vector<int>& owner, const std::vector<int>& parents) {
    wchar_t path[2048];
    DWORD n=GetEnvironmentVariableW(L"CHAMFERPRO_ASSIGNMENT_TRACE",path,2048);
    if(!n || n>=2048) return;
    FILE* f=nullptr;
    if(_wfopen_s(&f,path,L"a") || !f) return;
    const float* sw=src.edgeFloat(EDATA_KNOT);
    const float* dw=dst.edgeFloat(EDATA_KNOT);
    fprintf(f,"STAGE srcE=%d dstE=%d\n",int(src.nume),int(dst.nume));
    if(sw && dw) for(int e=0;e<dst.nume;++e) {
        int p=parents[e];
        if(p<0 || sw[p]==dw[e]) continue;
        const auto& edge=dst.e[e];
        fprintf(f,"ASSIGN edge=%d native=%g proposed=%g parent=%d owners=%d,%d faces=%d,%d\n",
            e+1,double(dw[e]),double(sw[p]),p+1,owner[edge.v1],owner[edge.v2],int(edge.f1),int(edge.f2));
        if(dw[e]!=1.0f) {
            for(int face : {edge.f1,edge.f2}) if(face>=0) {
                fprintf(f,"FACE %d",face);
                for(int j=0;j<dst.f[face].deg;++j) {
                    int ne=dst.f[face].edg[j];
                    fprintf(f," e%d:w%g:p%d",ne+1,double(dw[ne]),parents[ne]+1);
                }
                fprintf(f,"\n");
            }
        }
    }
    fclose(f);
}