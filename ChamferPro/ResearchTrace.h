#pragma once
#include <mnmesh.h>
#include <cstdio>
inline void ResearchTrace(const char* label, MNMesh& mesh) {
    FILE* f=nullptr;
    _wfopen_s(&f, L"C:\\Users\\ksi\\AppData\\Local\\Autodesk\\3dsMax\\2027 - 64bit\\ENU\\3dsMaxPlugins\\ChamferPro\\research-stages.log", L"a");
    if (!f) return;
    fprintf(f,"%s V=%d E=%d\n",label,int(mesh.numv),int(mesh.nume));
    for (int ch : {0,1,3}) {
        const float* values=mesh.edgeFloat(ch);
        int count=0;
        if(values) for(int e=0;e<mesh.nume;++e) if(values[e]>0.001f && values[e]<0.4f) ++count;
        fprintf(f," ch=%d matched=%d ptr=%p first=",ch,count,static_cast<const void*>(values));
        if(values) for(int e=0;e<mesh.nume && e<6;++e) fprintf(f,"%g,",values[e]);
        fprintf(f,"\n");
    }
    fprintf(f," parents=");
    for(int e=0;e<mesh.nume && e<6;++e) fprintf(f,"%d,",mesh.e[e].track);
    fprintf(f,"\n"); fclose(f);
}