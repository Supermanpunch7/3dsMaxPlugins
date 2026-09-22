#pragma once
#include <polyobj.h>
#include <cstdio>

// Diagnostic only. Never included by the stable build. Nonzero sentinels avoid
// mistaking default track/orig values for valid provenance.
inline void NativeProbe(ObjectState* os, int stage, bool before) {
    if (!os || !os->obj || !os->obj->IsSubClassOf(polyObjectClassID)) return;
    auto& mesh = static_cast<PolyObject*>(os->obj)->GetMesh();
    wchar_t path[2048];
    DWORD length = GetEnvironmentVariableW(L"CHAMFERPRO_PROBE_LOG", path, 2048);
    if (!length || length >= 2048) return;
    FILE* out = nullptr;
    if (_wfopen_s(&out, path, L"a") || !out) return;
    if (before) {
        for (int v = 0; v < mesh.numv; ++v) mesh.v[v].orig = 100000 + v;
        for (int e = 0; e < mesh.nume; ++e) mesh.e[e].track = 200000 + e;
    }
    fprintf(out, "STAGE %d %s V %d E %d F %d\n", stage,
        before ? "INPUT" : "OUTPUT", int(mesh.numv), int(mesh.nume), int(mesh.numf));
    for (int v = 0; v < mesh.numv; ++v) {
        const Point3 p = mesh.v[v].p;
        fprintf(out, "V %d orig %d p %.9g %.9g %.9g\n", v, int(mesh.v[v].orig), p.x, p.y, p.z);
    }
    for (int e = 0; e < mesh.nume; ++e) {
        fprintf(out, "E %d track %d ends %d %d", e, mesh.e[e].track, mesh.e[e].v1, mesh.e[e].v2);
        for (int ch : {0, 1, 3}) {
            float* values = static_cast<float*>(mesh.edgeData(ch));
            if (values) fprintf(out, " ch%d=%.9g", ch, values[e]);
            else fprintf(out, " ch%d=UNSUPPORTED", ch);
        }
        fprintf(out, "\n");
    }
    fclose(out);
}