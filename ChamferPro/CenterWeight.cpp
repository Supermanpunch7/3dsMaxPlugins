#include "CornerWeightR5.h"
#include "WeightCenterContinuation.h"
#include <max.h>
#include <iparamm2.h>
#include <buildver.h>

// Isolated prototype: never shares the legacy ChamferPro class ID.
static HINSTANCE instance;
static const Class_ID wrapperID(0x71293bb2, 0x19374ad2);
static const Class_ID nativeID(858559760, 951800972);
class NativePair;
class PairDesc : public ClassDesc2 {
public:
    int IsPublic() override { return TRUE; }
    void* Create(BOOL loading = FALSE) override;
    const TCHAR* ClassName() override { return _T("ChamferProCenterWeight"); }
    const TCHAR* NonLocalizedClassName() override { return ClassName(); }
    const TCHAR* InternalName() override { return _T("ChamferProCenterWeight"); }
    SClass_ID SuperClassID() override { return OSM_CLASS_ID; }
    Class_ID ClassID() override { return wrapperID; }
    const TCHAR* Category() override { return _T("Supermanpunch Development"); }
    HINSTANCE HInstance() override { return instance; }
};
static PairDesc desc;
static ParamBlockDesc2 params(0, _T("nativeStages"), 0, &desc, P_AUTO_CONSTRUCT, 0,
    0, _T("chamfer1"), TYPE_REFTARG, 0, 0, p_end,
    1, _T("chamfer2"), TYPE_REFTARG, 0, 0, p_end,
    p_end);

class PairContext : public LocalModData {
public:
    ModContext stages[2];
    LocalModData* Clone() override { return new PairContext(); }
};

// Apply only to new native stages; loading and cloning preserve stored settings.
template<class T> static void SetInitialValue(Modifier* m, const TCHAR* name, T value) {
    for (int b=0; b<m->NumParamBlocks(); ++b) {
        auto* p=m->GetParamBlock(b);
        if (!p) continue;
        for (int j=0; j<p->NumParams(); ++j) {
            ParamID id=p->IndextoID(j);
            const auto& def=p->GetParamDef(id);
            if (def.int_name && _tcsicmp(def.int_name,name)==0) {
                p->SetValue(id,0,value); return;
            }
        }
    }
}
static void ApplyInitialSettings(Modifier* m, int stage) {
    if (!m) return;
    if (stage == 0) SetInitialValue(m, _T("amount"), 1.0f);
    if (stage == 0) SetInitialValue(m, _T("segments"), 1);
    if (stage == 0) SetInitialValue(m, _T("tension"), 1.0f);
    if (stage == 0) SetInitialValue(m, _T("openchamfer"), 0);
    if (stage == 0) SetInitialValue(m, _T("invert"), 0);
    if (stage == 0) SetInitialValue(m, _T("selectionoption"), 5);
    if (stage == 0) SetInitialValue(m, _T("smoothingoption"), 2);
    if (stage == 0) SetInitialValue(m, _T("materialoption"), 1);
    if (stage == 0) SetInitialValue(m, _T("setmaterial"), 0);
    if (stage == 0) SetInitialValue(m, _T("materialID"), 1);
    if (stage == 0) SetInitialValue(m, _T("smooth"), 1);
    if (stage == 0) SetInitialValue(m, _T("SmoothType"), 1);
    if (stage == 0) SetInitialValue(m, _T("smooththreshold"), 20.0f);
    if (stage == 0) SetInitialValue(m, _T("chamfertype"), 0);
    if (stage == 0) SetInitialValue(m, _T("limiteffect"), 1);
    if (stage == 0) SetInitialValue(m, _T("useminangle"), 0);
    if (stage == 0) SetInitialValue(m, _T("minangle"), 20.0f);
    if (stage == 0) SetInitialValue(m, _T("usemaxangle"), 0);
    if (stage == 0) SetInitialValue(m, _T("maxangle"), 90.0f);
    if (stage == 0) SetInitialValue(m, _T("smoothtoadjacent"), 1);
    if (stage == 0) SetInitialValue(m, _T("quadIntersectionMode"), 1);
    if (stage == 0) SetInitialValue(m, _T("miteringType"), 1);
    if (stage == 0) SetInitialValue(m, _T("amountType"), 3);
    if (stage == 0) SetInitialValue(m, _T("minAmount"), 0.0f);
    if (stage == 0) SetInitialValue(m, _T("maxAmount"), 1.0f);
    if (stage == 0) SetInitialValue(m, _T("addinset"), 1);
    if (stage == 0) SetInitialValue(m, _T("insetamount"), 0.2f);
    if (stage == 0) SetInitialValue(m, _T("insetsegments"), 0);
    if (stage == 0) SetInitialValue(m, _T("insetoffset"), -0.2f);
    if (stage == 0) SetInitialValue(m, _T("forcePositiveOffset"), 0);
    if (stage == 0) SetInitialValue(m, _T("miterEndBias"), 0.5f);
    if (stage == 0) SetInitialValue(m, _T("useConstantOffset"), 0);
    if (stage == 0) SetInitialValue(m, _T("depth"), 0.5f);
    if (stage == 0) SetInitialValue(m, _T("radiusBias"), 0.0f);
    if (stage == 0) SetInitialValue(m, _T("biasEndPoints"), 1);
    if (stage == 0) SetInitialValue(m, _T("scale"), 0.1f);
    if (stage == 0) SetInitialValue(m, _T("depthType"), 1);
    if (stage == 0) SetInitialValue(m, _T("insetType"), 0);
    if (stage == 0) SetInitialValue(m, _T("angleFactorVertex"), 0.5f);
    if (stage == 1) SetInitialValue(m, _T("amount"), 1.0f);
    if (stage == 1) SetInitialValue(m, _T("segments"), 1);
    if (stage == 1) SetInitialValue(m, _T("tension"), 1.0f);
    if (stage == 1) SetInitialValue(m, _T("openchamfer"), 0);
    if (stage == 1) SetInitialValue(m, _T("invert"), 0);
    if (stage == 1) SetInitialValue(m, _T("selectionoption"), 5);
    if (stage == 1) SetInitialValue(m, _T("smoothingoption"), 2);
    if (stage == 1) SetInitialValue(m, _T("materialoption"), 0);
    if (stage == 1) SetInitialValue(m, _T("setmaterial"), 0);
    if (stage == 1) SetInitialValue(m, _T("materialID"), 1);
    if (stage == 1) SetInitialValue(m, _T("smooth"), 1);
    if (stage == 1) SetInitialValue(m, _T("SmoothType"), 1);
    if (stage == 1) SetInitialValue(m, _T("smooththreshold"), 180.0f);
    if (stage == 1) SetInitialValue(m, _T("chamfertype"), 0);
    if (stage == 1) SetInitialValue(m, _T("limiteffect"), 1);
    if (stage == 1) SetInitialValue(m, _T("useminangle"), 0);
    if (stage == 1) SetInitialValue(m, _T("minangle"), 20.0f);
    if (stage == 1) SetInitialValue(m, _T("usemaxangle"), 0);
    if (stage == 1) SetInitialValue(m, _T("maxangle"), 90.0f);
    if (stage == 1) SetInitialValue(m, _T("smoothtoadjacent"), 1);
    if (stage == 1) SetInitialValue(m, _T("quadIntersectionMode"), 1);
    if (stage == 1) SetInitialValue(m, _T("miteringType"), 1);
    if (stage == 1) SetInitialValue(m, _T("amountType"), 3);
    if (stage == 1) SetInitialValue(m, _T("minAmount"), 0.0f);
    if (stage == 1) SetInitialValue(m, _T("maxAmount"), 1.0f);
    if (stage == 1) SetInitialValue(m, _T("addinset"), 0);
    if (stage == 1) SetInitialValue(m, _T("insetamount"), 0.5f);
    if (stage == 1) SetInitialValue(m, _T("insetsegments"), 0);
    if (stage == 1) SetInitialValue(m, _T("insetoffset"), 0.0f);
    if (stage == 1) SetInitialValue(m, _T("forcePositiveOffset"), 0);
    if (stage == 1) SetInitialValue(m, _T("miterEndBias"), 0.5f);
    if (stage == 1) SetInitialValue(m, _T("useConstantOffset"), 0);
    if (stage == 1) SetInitialValue(m, _T("depth"), 0.5f);
    if (stage == 1) SetInitialValue(m, _T("radiusBias"), 0.0f);
    if (stage == 1) SetInitialValue(m, _T("biasEndPoints"), 1);
    if (stage == 1) SetInitialValue(m, _T("scale"), 0.04f);
    if (stage == 1) SetInitialValue(m, _T("depthType"), 1);
    if (stage == 1) SetInitialValue(m, _T("insetType"), 0);
    if (stage == 1) SetInitialValue(m, _T("angleFactorVertex"), 0.5f);
}

class NativePair : public Modifier {
    IParamBlock2* pb = nullptr;
    IObjParam* editIP = nullptr;
    HWND selector = nullptr;
    int activeStage = 0;
    ULONG editFlags = 0;
    static INT_PTR CALLBACK PanelProc(HWND window, UINT message, WPARAM w, LPARAM l) {
        auto* self = reinterpret_cast<NativePair*>(GetWindowLongPtr(window, GWLP_USERDATA));
        if (message == WM_INITDIALOG) {
            self = reinterpret_cast<NativePair*>(l);
            SetWindowLongPtr(window, GWLP_USERDATA, l);
            CheckRadioButton(window, 1001, 1002, 1001 + self->activeStage);
            return TRUE;
        }
        if (message == WM_COMMAND && self && HIWORD(w) == BN_CLICKED &&
            (LOWORD(w) == 1001 || LOWORD(w) == 1002)) {
            // Defer editing-page replacement until the button notification returns.
            PostMessage(window, WM_APP + 1, LOWORD(w) - 1001, 0);
            return TRUE;
        }
        if (message == WM_APP + 1 && self && self->editIP && self->activeStage != int(w)) {
            if (auto* old = self->Stage(self->activeStage)) old->EndEditParams(self->editIP, self->editFlags, nullptr);
            self->activeStage = int(w);
            if (auto* next = self->Stage(self->activeStage)) next->BeginEditParams(self->editIP, self->editFlags, nullptr);
            return TRUE;
        }
        return FALSE;
    }
public:
    void BeginEditParams(IObjParam* ip, ULONG flags, Animatable* previous) override {
        editIP = ip;
        editFlags = flags;
        selector = ip->AddRollupPage(instance, MAKEINTRESOURCE(201), PanelProc,
            _T("ChamferProCenterWeight"), reinterpret_cast<LPARAM>(this));
        if (auto* m = Stage(activeStage)) m->BeginEditParams(ip, flags, previous);
    }
    void EndEditParams(IObjParam* ip, ULONG flags, Animatable* next) override {
        if (auto* m = Stage(activeStage)) m->EndEditParams(ip, flags, next);
        if (selector) ip->DeleteRollupPage(selector);
        selector = nullptr;
        editIP = nullptr;
    }
    explicit NativePair(BOOL loading) {
        desc.MakeAutoParamBlocks(this);
        if (!loading) {
            for (int i = 0; i < 2; ++i)
                { auto* m = static_cast<Modifier*>(CreateInstance(OSM_CLASS_ID, nativeID)); ApplyInitialSettings(m, i); pb->SetValue(i, 0, static_cast<ReferenceTarget*>(m)); }
        }
    }
    ~NativePair() override { DeleteAllRefsFromMe(); }
    Modifier* Stage(int i) {
        auto* ref = pb ? pb->GetReferenceTarget(i) : nullptr;
        return ref && ref->SuperClassID() == OSM_CLASS_ID && ref->ClassID() == nativeID
            ? static_cast<Modifier*>(ref) : nullptr;
    }
    void DeleteThis() override { delete this; }
    Class_ID ClassID() override { return wrapperID; }
    SClass_ID SuperClassID() override { return OSM_CLASS_ID; }
    void GetClassName(MSTR& s, bool) const override { s = _T("ChamferProCenterWeight"); }
    const TCHAR* GetObjectName(bool) const override { return _T("ChamferProCenterWeight"); }
    CreateMouseCallBack* GetCreateMouseCallBack() override { return nullptr; }
    int NumRefs() override { return 1; }
    RefTargetHandle GetReference(int i) override { return i == 0 ? pb : nullptr; }
    void SetReference(int i, RefTargetHandle r) override { if (i == 0) pb = static_cast<IParamBlock2*>(r); }
    int NumSubs() override { return 1; }
    Animatable* SubAnim(int i) override { return i == 0 ? pb : nullptr; }
    TSTR SubAnimName(int, bool) override { return _T("Native Chamfer Stages"); }
    int NumParamBlocks() override { return 1; }
    IParamBlock2* GetParamBlock(int i) override { return i == 0 ? pb : nullptr; }
    IParamBlock2* GetParamBlockByID(BlockID id) override { return id == 0 ? pb : nullptr; }
    RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL) override {
        return REF_SUCCEED;
    }
    RefTargetHandle Clone(RemapDir& remap) override {
        auto* result = new NativePair(TRUE);
        result->ReplaceReference(0, remap.CloneRef(pb));
        BaseClone(this, result, remap);
        return result;
    }
    ChannelMask ChannelsUsed() override { return GEOM_CHANNEL | TOPO_CHANNEL | SELECT_CHANNEL | SUBSEL_TYPE_CHANNEL | TEXMAP_CHANNEL | VERTCOLOR_CHANNEL; }
    ChannelMask ChannelsChanged() override { return ChannelsUsed(); }
    Class_ID InputType() override { auto* m = Stage(0); return m ? m->InputType() : defObjectClassID; }
    Interval LocalValidity(TimeValue t) override {
        Interval valid = FOREVER;
        for (int i = 0; i < 2; ++i) if (auto* m = Stage(i)) valid &= m->LocalValidity(t);
        return valid;
    }
    void NotifyInputChanged(const Interval& interval, PartID part, RefMessage msg, ModContext* mc) override {
        if (!mc || !mc->localData) return;
        auto* data = static_cast<PairContext*>(mc->localData);
        for (int i = 0; i < 2; ++i)
            if (auto* m = Stage(i)) m->NotifyInputChanged(interval, part, msg, &data->stages[i]);
    }
    void ModifyObject(TimeValue t, ModContext& mc, ObjectState* os, INode* node) override {
        if (!os || !os->obj) return;
        if (!mc.localData) mc.localData = new PairContext();
        auto* data = static_cast<PairContext*>(mc.localData);
        MNMesh originalMesh;
        bool haveOriginal = os->obj->IsSubClassOf(polyObjectClassID) != 0;
        if (haveOriginal) originalMesh = static_cast<PolyObject*>(os->obj)->GetMesh();
        for (int i = 0; i < 2; ++i) {
            auto* m = Stage(i);
            if (!m || !m->IsEnabled()) continue;
            // Each native modifier owns a separate persistent context.
            // No topology editing or parameter mutation occurs in this wrapper.
            m->NotifyInputChanged(FOREVER, PART_ALL, REFMSG_CHANGE, &data->stages[i]);
            MNMesh sourceMesh;
bool haveSource = os->obj->IsSubClassOf(polyObjectClassID) != 0;
if (haveSource) sourceMesh = static_cast<PolyObject*>(os->obj)->GetMesh();
m->ModifyObject(t, data->stages[i], os, node);
if (haveSource && os->obj->IsSubClassOf(polyObjectClassID))
    LogNativeCornerTrial(i, ApplyNativeCornerTrial(sourceMesh, static_cast<PolyObject*>(os->obj)->GetMesh()));
// The first chamfer creates short edges whose matching radii can be too small
// for stage two. Reapply only correspondences resolved against original input.
if (i == 1 && haveOriginal && os->obj->IsSubClassOf(polyObjectClassID))
    LogNativeCornerTrial(2, ApplyNativeCornerTrial(originalMesh, static_cast<PolyObject*>(os->obj)->GetMesh()));
        }
        if (haveOriginal && os->obj->IsSubClassOf(polyObjectClassID))
            RestoreCenterContinuation(originalMesh, static_cast<PolyObject*>(os->obj)->GetMesh());
    }
};
void* PairDesc::Create(BOOL loading) { return new NativePair(loading); }
BOOL WINAPI DllMain(HINSTANCE h, ULONG reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) { instance = h; DisableThreadLibraryCalls(h); }
    return TRUE;
}
extern "C" __declspec(dllexport) const TCHAR* LibDescription() { return _T("Native Chamfer pair evaluation prototype"); }
extern "C" __declspec(dllexport) int LibNumberClasses() { return 1; }
extern "C" __declspec(dllexport) ClassDesc* LibClassDesc(int i) { return i == 0 ? &desc : nullptr; }
extern "C" __declspec(dllexport) ULONG LibVersion() { return VERSION_3DSMAX; }
extern "C" __declspec(dllexport) ULONG CanAutoDefer() { return 1; }