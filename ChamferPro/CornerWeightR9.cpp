#include "CornerWeightR4.h"
#include "FaceLineageWeight.h"

#include <max.h>
#include <iparamm2.h>
#include <buildver.h>

#include <algorithm>
#include <vector>

static HINSTANCE instance;
static const Class_ID wrapperID(0x71293b6b, 0x19374a92);
static const Class_ID nativeID(858559760, 951800972);

class NativePair;
class PairDesc : public ClassDesc2 {
public:
    int IsPublic() override { return TRUE; }
    void* Create(BOOL loading = FALSE) override;
    const TCHAR* ClassName() override { return _T("ChamferProCornerWeightR9"); }
    const TCHAR* NonLocalizedClassName() override { return ClassName(); }
    const TCHAR* InternalName() override { return _T("ChamferProCornerWeightR9"); }
    SClass_ID SuperClassID() override { return OSM_CLASS_ID; }
    Class_ID ClassID() override { return wrapperID; }
    const TCHAR* Category() override { return _T("Supermanpunch Development"); }
    HINSTANCE HInstance() override { return instance; }
};

static PairDesc desc;
static ParamBlockDesc2 params(
    0, _T("nativeStages"), 0, &desc, P_AUTO_CONSTRUCT, 0,
    0, _T("chamfer1"), TYPE_REFTARG, 0, 0, p_end,
    1, _T("chamfer2"), TYPE_REFTARG, 0, 0, p_end,
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

class NativePair : public Modifier {
    IParamBlock2* pb = nullptr;
    IObjParam* editIP = nullptr;
    HWND selector = nullptr;
    int activeStage = 0;
    ULONG editFlags = 0;

    static INT_PTR CALLBACK PanelProc(HWND window, UINT message,
                                      WPARAM w, LPARAM l) {
        auto* self = reinterpret_cast<NativePair*>(
            GetWindowLongPtr(window, GWLP_USERDATA));
        if (message == WM_INITDIALOG) {
            self = reinterpret_cast<NativePair*>(l);
            SetWindowLongPtr(window, GWLP_USERDATA, l);
            CheckRadioButton(window, 1001, 1002, 1001 + self->activeStage);
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
        selector = ip->AddRollupPage(
            instance, MAKEINTRESOURCE(201), PanelProc,
            _T("ChamferProCornerWeightR9"), reinterpret_cast<LPARAM>(this));
        if (auto* modifier = Stage(activeStage))
            modifier->BeginEditParams(ip, flags, nullptr);
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
        name = _T("ChamferProCornerWeightR9");
    }
    const TCHAR* GetObjectName(bool) const override {
        return _T("ChamferProCornerWeightR9");
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
    ChannelMask ChannelsChanged() override { return ChannelsUsed(); }
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
            if (!modifier || !modifier->IsEnabled()) continue;

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
                ApplyWeightFaceLineage(sourceMesh, outputMesh, weightTag));
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






