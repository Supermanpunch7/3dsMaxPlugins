// Release snapshot: 2026-09-21, R14 lineage baseline; known partial inheritance limitation.
#include "CornerWeightR4.h"
#include "FaceLineageWeightR14.h"

#include <max.h>
#include <iparamm2.h>
#include <buildver.h>
#include <units.h>
#include <custcont.h>

#include <algorithm>
#include <vector>

static HINSTANCE instance;
static const Class_ID wrapperID(0x71293b73, 0x19374a9a);
static const Class_ID nativeID(858559760, 951800972);

class NativePair;
class PairDesc : public ClassDesc2 {
public:
    int IsPublic() override { return TRUE; }
    void* Create(BOOL loading = FALSE) override;
    const TCHAR* ClassName() override { return _T("ChamferPro"); }
    const TCHAR* NonLocalizedClassName() override { return ClassName(); }
    const TCHAR* InternalName() override { return _T("ChamferPro"); }
    SClass_ID SuperClassID() override { return OSM_CLASS_ID; }
    Class_ID ClassID() override { return wrapperID; }
    const TCHAR* Category() override { return _T("Supermanpunch"); }
    HINSTANCE HInstance() override { return instance; }
};

static PairDesc desc;
static ParamBlockDesc2 params(
    0, _T("nativeStages"), 0, &desc, P_AUTO_CONSTRUCT, 0,
    0, _T("chamfer1"), TYPE_REFTARG, 0, 0, p_end,
    1, _T("chamfer2"), TYPE_REFTARG, 0, 0, p_end,
    2, _T("chamfer1Enabled"), TYPE_BOOL, 0, 0, p_default, TRUE, p_end,
    3, _T("chamfer2Enabled"), TYPE_BOOL, 0, 0, p_default, TRUE, p_end,
    4, _T("displayHardEdges"), TYPE_BOOL, 0, 0, p_default, FALSE, p_end,
    5, _T("hardEdgeColor"), TYPE_RGBA, 0, 0, p_default, Color(0.0f, 0.0f, 0.0f), p_end,
    p_end);

// ApplyNativeCornerTrial also repairs smoothing groups, but its older geometric
// weight guesses must not leak into edges for which exact lineage is unknown.
// Preserve the native Chamfer weight result around that smoothing-only pass.
struct NativeEdgeWeightState {
    bool supported = false;
    std::vector<float> values;
};

static NativeEdgeWeightState CaptureNativeEdgeWeights(const MNMesh& mesh) {
    NativeEdgeWeightState state;
    const float* values = mesh.edgeFloat(EDATA_KNOT);
    if (!values) return state;
    state.supported = true;
    state.values.assign(values, values + mesh.nume);
    return state;
}

static void RestoreNativeEdgeWeights(MNMesh& mesh,
                                     const NativeEdgeWeightState& state) {
    if (!state.supported) {
        mesh.freeEData(EDATA_KNOT);
        return;
    }
    mesh.setEDataSupport(EDATA_KNOT);
    float* values = mesh.edgeFloat(EDATA_KNOT);
    if (!values) return;
    const int edgeCount = mesh.nume;
    const int count = (std::min)(edgeCount, int(state.values.size()));
    std::copy(state.values.begin(), state.values.begin() + count, values);
}

class PairContext : public LocalModData {
public:
    ModContext stages[2];
    LocalModData* Clone() override { return new PairContext(); }
};

// Defaults revision 2026-09-21b: stage 1 Smooth off; stage 2 Same Materials.
// Applied to new instances only; scene loading and cloning preserve settings.
template<class T> static void SetInitialValue(Modifier* m, const TCHAR* name, T value) {
    for (int b = 0; b < m->NumParamBlocks(); ++b) {
        auto* block = m->GetParamBlock(b);
        if (!block) continue;
        for (int j = 0; j < block->NumParams(); ++j) {
            const ParamID id = block->IndextoID(j);
            const auto& def = block->GetParamDef(id);
            if (def.int_name && _tcsicmp(def.int_name, name) == 0) {
                block->SetValue(id, 0, value);
                return;
            }
        }
    }
}

static void ApplyInitialSettings(Modifier* m, int stage) {
    if (!m) return;
    const float cm = float(1.0 / GetSystemUnitScale(UNITS_CENTIMETERS));
    SetInitialValue(m, _T("miteringType"), 1);
    SetInitialValue(m, _T("miterEndBias"), 0.5f);
    SetInitialValue(m, _T("amountType"), 3);
    SetInitialValue(m, _T("scale"), (stage == 0 ? 0.02f : 0.05f) * cm);
    SetInitialValue(m, _T("segments"), stage == 0 ? 1 : 0);
    SetInitialValue(m, _T("depthType"), 1);
    SetInitialValue(m, _T("radiusBias"), 0.0f);
    SetInitialValue(m, _T("limiteffect"), 1);
    SetInitialValue(m, _T("addinset"), stage == 0 ? 1 : 0);
    SetInitialValue(m, _T("insetType"), 0);
    SetInitialValue(m, _T("insetamount"), (stage == 0 ? 0.2f : 0.5f) * cm);
    SetInitialValue(m, _T("insetsegments"), 0);
    SetInitialValue(m, _T("insetoffset"), (stage == 0 ? -0.2f : 0.0f) * cm);
    SetInitialValue(m, _T("selectionoption"), 5);
    SetInitialValue(m, _T("smoothingoption"), 2);
    SetInitialValue(m, _T("materialoption"), stage == 0 ? 1 : 2);
    SetInitialValue(m, _T("useminangle"), 0);
    SetInitialValue(m, _T("minangle"), 20.0f);
    SetInitialValue(m, _T("usemaxangle"), 0);
    SetInitialValue(m, _T("maxangle"), 90.0f);
    SetInitialValue(m, _T("setmaterial"), 0);
    SetInitialValue(m, _T("materialID"), 1);
    SetInitialValue(m, _T("smooth"), stage == 0 ? 0 : 1);
    SetInitialValue(m, _T("SmoothType"), 1);
    SetInitialValue(m, _T("smoothtoadjacent"), 1);
    SetInitialValue(m, _T("smooththreshold"), stage == 0 ? 30.0f : 180.0f);
    SetInitialValue(m, _T("openchamfer"), 0);
    SetInitialValue(m, _T("invert"), 0);
}

class NativePair : public Modifier {
    IParamBlock2* pb = nullptr;
    IObjParam* editIP = nullptr;
    HWND selector = nullptr;
    int activeStage = 0;
    ULONG editFlags = 0;
    IColorSwatch* hardSwatch = nullptr;

    void RefreshControls() {
        if (!selector || !pb) return;
        for (int i = 0; i < 2; ++i)
            CheckDlgButton(selector, 1011 + i, pb->GetInt(2 + i) ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(selector, 1013, pb->GetInt(4) ? BST_CHECKED : BST_UNCHECKED);
        if (hardSwatch) hardSwatch->SetColor(pb->GetColor(5));
    }

    static INT_PTR CALLBACK PanelProc(HWND window, UINT message,
                                      WPARAM w, LPARAM l) {
        auto* self = reinterpret_cast<NativePair*>(
            GetWindowLongPtr(window, GWLP_USERDATA));
        if (message == WM_INITDIALOG) {
            self = reinterpret_cast<NativePair*>(l);
            SetWindowLongPtr(window, GWLP_USERDATA, l);
            CheckRadioButton(window, 1001, 1002, 1001 + self->activeStage);
            self->selector = window;
            self->hardSwatch = GetIColorSwatch(GetDlgItem(window, 1014),
                self->pb->GetColor(5), _T("Hard Edge Color"));
            self->RefreshControls();
            return TRUE;
        }
        if (message == WM_DESTROY && self) {
            if (self->hardSwatch) ReleaseIColorSwatch(self->hardSwatch);
            self->hardSwatch = nullptr;
        }
        if (message == WM_COMMAND && self && HIWORD(w) == BN_CLICKED &&
            LOWORD(w) >= 1011 && LOWORD(w) <= 1013) {
            theHold.Begin();
            self->pb->SetValue(LOWORD(w) - 1009, 0,
                IsDlgButtonChecked(window, LOWORD(w)) == BST_CHECKED);
            theHold.Accept(_T("ChamferPro Controls"));
            GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
            return TRUE;
        }
        if (message == CC_COLOR_CHANGE && self && self->hardSwatch) {
            theHold.Begin();
            self->pb->SetValue(5, 0, Color(self->hardSwatch->GetColor()));
            theHold.Accept(_T("ChamferPro Hard Edge Color"));
            GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
            return TRUE;
        }
        if (message == WM_COMMAND && self && HIWORD(w) == BN_CLICKED &&
            (LOWORD(w) == 1001 || LOWORD(w) == 1002)) {
            PostMessage(window, WM_APP + 1, LOWORD(w) - 1001, 0);
            return TRUE;
        }
        if (message == WM_APP + 1 && self && self->editIP &&
            self->selector == window && w <= 1 &&
            self->activeStage != int(w)) {
            if (auto* old = self->Stage(self->activeStage))
                old->EndEditParams(self->editIP, END_EDIT_REMOVEUI, nullptr);
            self->activeStage = int(w);
            if (auto* next = self->Stage(self->activeStage))
                next->BeginEditParams(self->editIP, self->editFlags, nullptr);
            return TRUE;
        }
        return FALSE;
    }

public:
    void BeginEditParams(IObjParam* ip, ULONG flags,
                         Animatable* previous) override {
        // Max can begin editing again while this instance still owns its UI.
        // Never overwrite the only handle to an existing selector rollout.
        if (editIP == ip && selector && IsWindow(selector)) return;
        if (editIP) EndEditParams(editIP, END_EDIT_REMOVEUI, nullptr);
        editIP = ip;
        editFlags = flags;
        if (auto* modifier = Stage(activeStage))
            modifier->BeginEditParams(ip, flags, nullptr);
        // UI revision 2026-09-21: create selector after the native UI and
        // keep it outside native rollout auto-collapse; retain the scene Class_ID.
        selector = ip->AddRollupPage(
            instance, MAKEINTRESOURCE(202), PanelProc,
            _T("ChamferPro - Chamfer 1 / Chamfer 2"),
            reinterpret_cast<LPARAM>(this), DONTAUTOCLOSE, ROLLUP_CAT_SYSTEM);
    }

    void EndEditParams(IObjParam* ip, ULONG flags, Animatable* next) override {
        if (!editIP) return;
        IObjParam* owner = editIP;
        HWND page = selector;
        editIP = nullptr;
        selector = nullptr;
        if (auto* modifier = Stage(activeStage))
            modifier->EndEditParams(owner, END_EDIT_REMOVEUI, nullptr);
        if (page && IsWindow(page)) owner->DeleteRollupPage(page);
    }

    explicit NativePair(BOOL loading) {
        desc.MakeAutoParamBlocks(this);
        if (!loading) {
            for (int stage = 0; stage < 2; ++stage) {
                pb->SetValue(stage, 0, static_cast<ReferenceTarget*>(
                    CreateInstance(OSM_CLASS_ID, nativeID)));
                ApplyInitialSettings(Stage(stage), stage);
            }
        }
    }

    ~NativePair() override { DeleteAllRefsFromMe(); }

    Modifier* Stage(int index) {
        auto* reference = pb ? pb->GetReferenceTarget(index) : nullptr;
        return reference && reference->SuperClassID() == OSM_CLASS_ID &&
                       reference->ClassID() == nativeID
            ? static_cast<Modifier*>(reference)
            : nullptr;
    }

    void DeleteThis() override { delete this; }
    Class_ID ClassID() override { return wrapperID; }
    SClass_ID SuperClassID() override { return OSM_CLASS_ID; }
    void GetClassName(MSTR& name, bool) const override {
        name = _T("ChamferPro");
    }
    const TCHAR* GetObjectName(bool) const override {
        return _T("ChamferPro");
    }
    CreateMouseCallBack* GetCreateMouseCallBack() override { return nullptr; }
    int NumRefs() override { return 1; }
    RefTargetHandle GetReference(int index) override {
        return index == 0 ? pb : nullptr;
    }
    void SetReference(int index, RefTargetHandle reference) override {
        if (index == 0) pb = static_cast<IParamBlock2*>(reference);
    }
    int NumSubs() override { return 1; }
    Animatable* SubAnim(int index) override {
        return index == 0 ? pb : nullptr;
    }
    TSTR SubAnimName(int, bool) override {
        return _T("Native Chamfer Stages");
    }
    int NumParamBlocks() override { return 1; }
    IParamBlock2* GetParamBlock(int index) override {
        return index == 0 ? pb : nullptr;
    }
    IParamBlock2* GetParamBlockByID(BlockID id) override {
        return id == 0 ? pb : nullptr;
    }
    RefResult NotifyRefChanged(const Interval&, RefTargetHandle,
                               PartID&, RefMessage, BOOL) override {
        RefreshControls();
        return REF_SUCCEED;
    }
    RefTargetHandle Clone(RemapDir& remap) override {
        auto* result = new NativePair(TRUE);
        result->ReplaceReference(0, remap.CloneRef(pb));
        BaseClone(this, result, remap);
        return result;
    }
    ChannelMask ChannelsUsed() override {
        return GEOM_CHANNEL | TOPO_CHANNEL | SELECT_CHANNEL |
               SUBSEL_TYPE_CHANNEL | TEXMAP_CHANNEL | VERTCOLOR_CHANNEL;
    }
    ChannelMask ChannelsChanged() override { return ChannelsUsed() | DISP_ATTRIB_CHANNEL; }
    Class_ID InputType() override {
        auto* modifier = Stage(0);
        return modifier ? modifier->InputType() : defObjectClassID;
    }
    Interval LocalValidity(TimeValue time) override {
        Interval valid = FOREVER;
        for (int stage = 0; stage < 2; ++stage)
            if (auto* modifier = Stage(stage))
                valid &= modifier->LocalValidity(time);
        return valid;
    }
    void NotifyInputChanged(const Interval& interval, PartID part,
                            RefMessage message, ModContext* context) override {
        if (!context || !context->localData) return;
        auto* data = static_cast<PairContext*>(context->localData);
        for (int stage = 0; stage < 2; ++stage)
            if (auto* modifier = Stage(stage))
                modifier->NotifyInputChanged(
                    interval, part, message, &data->stages[stage]);
    }

    void ModifyObject(TimeValue time, ModContext& context,
                      ObjectState* state, INode* node) override {
        if (!state || !state->obj) return;
        if (!context.localData) context.localData = new PairContext();
        auto* data = static_cast<PairContext*>(context.localData);

        MNMesh originalMesh;
        const bool haveOriginal =
            state->obj->IsSubClassOf(polyObjectClassID) != 0;
        if (haveOriginal)
            originalMesh = static_cast<PolyObject*>(state->obj)->GetMesh();

        for (int stage = 0; stage < 2; ++stage) {
            auto* modifier = Stage(stage);
            if (!modifier || !modifier->IsEnabled() || !pb->GetInt(2 + stage)) continue;

            modifier->NotifyInputChanged(
                FOREVER, PART_ALL, REFMSG_CHANGE, &data->stages[stage]);

            MNMesh sourceMesh;
            WeightFaceLineageTag weightTag;
            const bool haveSource =
                state->obj->IsSubClassOf(polyObjectClassID) != 0;
            if (haveSource) {
                auto& inputMesh =
                    static_cast<PolyObject*>(state->obj)->GetMesh();
                sourceMesh = inputMesh;
                weightTag = BeginWeightFaceLineage(inputMesh);
            }

            modifier->ModifyObject(
                time, data->stages[stage], state, node);

            if (!haveSource ||
                !state->obj->IsSubClassOf(polyObjectClassID))
                continue;

            auto& outputMesh =
                static_cast<PolyObject*>(state->obj)->GetMesh();
            const NativeEdgeWeightState nativeWeights =
                CaptureNativeEdgeWeights(outputMesh);

            LogNativeCornerTrial(
                stage, ApplyNativeCornerTrial(sourceMesh, outputMesh));
            if (stage == 1 && haveOriginal) {
                LogNativeCornerTrial(
                    2, ApplyNativeCornerTrial(originalMesh, outputMesh));
            }

            RestoreNativeEdgeWeights(outputMesh, nativeWeights);
            LogNativeCornerTrial(
                10 + stage,
                ApplyWeightFaceLineage(sourceMesh, outputMesh, weightTag, false));
            LogNativeCornerTrial(
                20 + stage,
                ApplyWeightFaceLineage(sourceMesh, outputMesh, weightTag, false, EDATA_CREASE));
            LogNativeCornerTrial(
                30 + stage,
                ApplyWeightFaceLineage(sourceMesh, outputMesh, weightTag, false, EDATA_DEPTH));
            EndWeightFaceLineage(outputMesh, weightTag);
        }
        // SDK editablepoly/polyedops.cpp UpdateEdgeColorDisplay convention:
        // EDATA_COLOR black means smooth; hard-edge colors must be nonzero.
        // Display only: never change smoothing groups or modeling channels here.
        if (state->obj->IsSubClassOf(polyObjectClassID)) {
            auto& mesh = static_cast<PolyObject*>(state->obj)->GetMesh();
            mesh.ClearDispFlag(MNDISP_USE_EDGE_COLORS);
            if (pb->GetInt(4)) {
                Color c = pb->GetColor(5);
                auto component = [](float x) -> unsigned char {
                    return static_cast<unsigned char>((std::max)(1, (std::min)(255, int(x * 255.0f))));
                };
                Color24 hard(component(c.r), component(c.g), component(c.b));
                mesh.setEDataSupport(EDATA_COLOR);
                auto* colors = static_cast<Color24*>(mesh.edgeData(EDATA_COLOR));
                if (colors) {
                    for (int i = 0; i < mesh.nume; ++i) {
                        const auto& e = mesh.e[i];
                        colors[i] = Color24(0, 0, 0);
                        if (!e.GetFlag(MN_DEAD) && e.f1 >= 0 && e.f2 >= 0 &&
                            !(mesh.f[e.f1].smGroup & mesh.f[e.f2].smGroup)) colors[i] = hard;
                    }
                    mesh.SetDispFlag(MNDISP_USE_EDGE_COLORS);
                }
            }
            mesh.InvalidateHardwareMesh();
        }
    }
};

void* PairDesc::Create(BOOL loading) { return new NativePair(loading); }

BOOL WINAPI DllMain(HINSTANCE handle, ULONG reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        instance = handle;
        DisableThreadLibraryCalls(handle);
    }
    return TRUE;
}

extern "C" __declspec(dllexport) const TCHAR* LibDescription() {
    return _T("Native Chamfer pair with directional edge-weight lineage");
}
extern "C" __declspec(dllexport) int LibNumberClasses() { return 1; }
extern "C" __declspec(dllexport) ClassDesc* LibClassDesc(int index) {
    return index == 0 ? &desc : nullptr;
}
extern "C" __declspec(dllexport) ULONG LibVersion() { return VERSION_3DSMAX; }
extern "C" __declspec(dllexport) ULONG CanAutoDefer() { return 1; }






