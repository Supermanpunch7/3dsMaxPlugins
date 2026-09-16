#include <Max.h>
#include <iparamm2.h>
#include <polyobj.h>
#include <mnmesh.h>
#include <istdplug.h>
#include <plugapi.h>
#include <buildver.h>

#include <cfloat>

#include "resource.h"

HINSTANCE hInstance = nullptr;

static const Class_ID CHAMFERPRO_CLASS_ID(0x6f2a4d31, 0x41d07c92);
static const short CHAMFERPRO_PBLOCK_REF = 0;

class ChamferProClassDesc;

enum ChamferProParamBlockID
{
    chamferpro_params = 0,
};

enum ChamferProParamID
{
    pb_chamfer1_amount = 0,
    pb_chamfer1_segments,
    pb_chamfer1_tension,
    pb_chamfer2_amount,
    pb_chamfer2_segments,
    pb_chamfer2_tension,
    pb_limit_effect,
};

struct ChamferPassSettings
{
    float amount = 0.007f;
    int segments = 1;
    float tension = 0.5f;
};

class ChamferProMod : public Modifier
{
public:
    ChamferProMod();

    void DeleteThis() override { delete this; }

    void GetClassName(MSTR& s, bool localized) const override
    {
        s = _T("ChamferPro");
    }

    Class_ID ClassID() override { return CHAMFERPRO_CLASS_ID; }
    SClass_ID SuperClassID() override { return OSM_CLASS_ID; }
    const TCHAR* GetObjectName(bool localized) const override { return _T("ChamferPro"); }

    void BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev) override;
    void EndEditParams(IObjParam* ip, ULONG flags, Animatable* next) override;

    ChannelMask ChannelsUsed() override
    {
        return GEOM_CHANNEL | TOPO_CHANNEL | SELECT_CHANNEL | SUBSEL_TYPE_CHANNEL | TEXMAP_CHANNEL | VERTCOLOR_CHANNEL;
    }

    ChannelMask ChannelsChanged() override
    {
        return GEOM_CHANNEL | TOPO_CHANNEL | SELECT_CHANNEL | TEXMAP_CHANNEL | VERTCOLOR_CHANNEL;
    }

    Class_ID InputType() override { return defObjectClassID; }
    Interval LocalValidity(TimeValue t) override
    {
        Interval valid = FOREVER;
        if (pblock)
            pblock->GetValidity(t, valid);
        return valid;
    }
    CreateMouseCallBack* GetCreateMouseCallBack() override { return nullptr; }

    int NumRefs() override { return 1; }
    RefTargetHandle GetReference(int i) override { return (i == CHAMFERPRO_PBLOCK_REF) ? pblock : nullptr; }
    void SetReference(int i, RefTargetHandle rtarg) override
    {
        if (i == CHAMFERPRO_PBLOCK_REF)
            pblock = static_cast<IParamBlock2*>(rtarg);
    }

    int NumSubs() override { return 1; }
    Animatable* SubAnim(int i) override { return (i == 0) ? pblock : nullptr; }
    TSTR SubAnimName(int i, bool localized) override { return (i == 0) ? TSTR(_T("ChamferPro Parameters")) : TSTR(_T("")); }

    int NumParamBlocks() override { return 1; }
    IParamBlock2* GetParamBlock(int i) override { return (i == 0) ? pblock : nullptr; }
    IParamBlock2* GetParamBlockByID(BlockID id) override { return (id == chamferpro_params) ? pblock : nullptr; }

    RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL) override
    {
        // ReferenceMaker propagates ParamBlock changes to dependents automatically.
        // Calling NotifyDependents(REFMSG_CHANGE) from this callback re-enters the
        // reference notification system and can crash 3ds Max.
        return REF_SUCCEED;
    }

    RefTargetHandle Clone(RemapDir& remap) override
    {
        ChamferProMod* m = new ChamferProMod();
        m->ReplaceReference(CHAMFERPRO_PBLOCK_REF, remap.CloneRef(pblock));
        BaseClone(this, m, remap);
        return m;
    }

    void ModifyObject(TimeValue t, ModContext& mc, ObjectState* os, INode* node) override;

private:
    IParamBlock2* pblock = nullptr;

    ChamferPassSettings GetPassSettings(TimeValue t, int pass) const;
    bool GetLimitEffect(TimeValue t) const;

    static bool PrepareChamferFlags(MNMesh& mesh, DWORD flag);
    static bool PrepareSecondPassFlags(MNMesh& mesh, DWORD flag);
    static bool ApplyOneChamferPass(MNMesh& mesh, DWORD flag, const ChamferPassSettings& settings, bool limitEffect);
    bool ApplyChamferPro(TimeValue t, MNMesh& mesh);
};

class ChamferProClassDesc : public ClassDesc2
{
public:
    int IsPublic() override { return TRUE; }
    void* Create(BOOL loading = FALSE) override { return new ChamferProMod(); }
    const TCHAR* ClassName() override { return _T("ChamferPro"); }
    const TCHAR* NonLocalizedClassName() override { return _T("ChamferPro"); }
    SClass_ID SuperClassID() override { return OSM_CLASS_ID; }
    Class_ID ClassID() override { return CHAMFERPRO_CLASS_ID; }
    const TCHAR* Category() override { return _T("Supermanpunch"); }
    const TCHAR* InternalName() override { return _T("ChamferPro"); }
    HINSTANCE HInstance() override { return hInstance; }
};

static ChamferProClassDesc gChamferProDesc;

static ParamBlockDesc2 gChamferProParamBlock(
    chamferpro_params,
    _T("ChamferProParameters"),
    0,
    &gChamferProDesc,
    P_AUTO_CONSTRUCT | P_AUTO_UI,
    CHAMFERPRO_PBLOCK_REF,
    IDD_CHAMFERPRO_PANEL,
    IDS_CHAMFERPRO_ROLLOUT,
    0,
    0,
    nullptr,

    pb_chamfer1_amount,
    _T("chamfer1Amount"),
    TYPE_FLOAT,
    P_ANIMATABLE,
    IDS_CHAMFER1_AMOUNT,
    p_default, 0.007f,
    p_range, 0.0f, 999999.0f,
    p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CHAMFER1_AMOUNT_EDIT, IDC_CHAMFER1_AMOUNT_SPIN, SPIN_AUTOSCALE,
    p_end,

    pb_chamfer1_segments,
    _T("chamfer1Segments"),
    TYPE_INT,
    P_ANIMATABLE,
    IDS_CHAMFER1_SEGMENTS,
    p_default, 1,
    p_range, 1, 100,
    p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CHAMFER1_SEGMENTS_EDIT, IDC_CHAMFER1_SEGMENTS_SPIN, 1.0f,
    p_end,

    pb_chamfer1_tension,
    _T("chamfer1Tension"),
    TYPE_FLOAT,
    P_ANIMATABLE,
    IDS_CHAMFER1_TENSION,
    p_default, 0.5f,
    p_range, 0.0f, 1.0f,
    p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_CHAMFER1_TENSION_EDIT, IDC_CHAMFER1_TENSION_SPIN, 0.01f,
    p_end,

    pb_chamfer2_amount,
    _T("chamfer2Amount"),
    TYPE_FLOAT,
    P_ANIMATABLE,
    IDS_CHAMFER2_AMOUNT,
    p_default, 0.007f,
    p_range, 0.0f, 999999.0f,
    p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_CHAMFER2_AMOUNT_EDIT, IDC_CHAMFER2_AMOUNT_SPIN, SPIN_AUTOSCALE,
    p_end,

    pb_chamfer2_segments,
    _T("chamfer2Segments"),
    TYPE_INT,
    P_ANIMATABLE,
    IDS_CHAMFER2_SEGMENTS,
    p_default, 1,
    p_range, 1, 100,
    p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_CHAMFER2_SEGMENTS_EDIT, IDC_CHAMFER2_SEGMENTS_SPIN, 1.0f,
    p_end,

    pb_chamfer2_tension,
    _T("chamfer2Tension"),
    TYPE_FLOAT,
    P_ANIMATABLE,
    IDS_CHAMFER2_TENSION,
    p_default, 0.5f,
    p_range, 0.0f, 1.0f,
    p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_CHAMFER2_TENSION_EDIT, IDC_CHAMFER2_TENSION_SPIN, 0.01f,
    p_end,

    pb_limit_effect,
    _T("limitEffect"),
    TYPE_BOOL,
    0,
    IDS_LIMIT_EFFECT,
    p_default, TRUE,
    p_ui, TYPE_SINGLECHEKBOX, IDC_LIMIT_EFFECT,
    p_end,

    p_end);

ChamferProMod::ChamferProMod()
{
    gChamferProDesc.MakeAutoParamBlocks(this);
}

void ChamferProMod::BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev)
{
    gChamferProDesc.BeginEditParams(ip, this, flags, prev);
}

void ChamferProMod::EndEditParams(IObjParam* ip, ULONG flags, Animatable* next)
{
    gChamferProDesc.EndEditParams(ip, this, flags, next);
}

ChamferPassSettings ChamferProMod::GetPassSettings(TimeValue t, int pass) const
{
    ChamferPassSettings settings;
    if (!pblock)
        return settings;

    Interval valid = FOREVER;

    if (pass == 1)
    {
        pblock->GetValue(pb_chamfer1_amount, t, settings.amount, valid);
        pblock->GetValue(pb_chamfer1_segments, t, settings.segments, valid);
        pblock->GetValue(pb_chamfer1_tension, t, settings.tension, valid);
    }
    else
    {
        pblock->GetValue(pb_chamfer2_amount, t, settings.amount, valid);
        pblock->GetValue(pb_chamfer2_segments, t, settings.segments, valid);
        pblock->GetValue(pb_chamfer2_tension, t, settings.tension, valid);
    }

    settings.segments = (settings.segments < 1) ? 1 : settings.segments;
    return settings;
}

bool ChamferProMod::GetLimitEffect(TimeValue t) const
{
    BOOL limitEffect = TRUE;
    if (pblock)
    {
        Interval valid = FOREVER;
        pblock->GetValue(pb_limit_effect, t, limitEffect, valid);
    }
    return limitEffect != FALSE;
}

bool ChamferProMod::PrepareChamferFlags(MNMesh& mesh, DWORD flag)
{
    mesh.ClearEFlags(flag);

    bool any = false;

    if (mesh.selLevel == MNM_SL_EDGE)
    {
        for (int i = 0; i < mesh.nume; ++i)
        {
            if (mesh.e[i].GetFlag(MN_DEAD))
                continue;
            if (mesh.e[i].GetFlag(MN_SEL))
            {
                mesh.e[i].SetFlag(flag);
                any = true;
            }
        }
    }
    else
    {
        for (int i = 0; i < mesh.nume; ++i)
        {
            if (mesh.e[i].GetFlag(MN_DEAD))
                continue;
            mesh.e[i].SetFlag(flag);
            any = true;
        }
    }

    return any;
}

bool ChamferProMod::PrepareSecondPassFlags(MNMesh& mesh, DWORD flag)
{
    mesh.ClearEFlags(flag);

    bool any = false;
    for (int i = 0; i < mesh.nume; ++i)
    {
        if (mesh.e[i].GetFlag(MN_DEAD))
            continue;

        // QuadChamfer marks the two result edges that replace each processed
        // input edge.  These are the safe equivalent of passing the first
        // chamfer's edge result to a second chamfer operation.
        if (mesh.e[i].GetFlag(MN_EDGE_WAS_PROCESSED))
        {
            mesh.e[i].SetFlag(flag);
            any = true;
        }
    }

    return any;
}

bool ChamferProMod::ApplyOneChamferPass(MNMesh& mesh, DWORD flag, const ChamferPassSettings& settings, bool limitEffect)
{
    if (settings.amount <= FLT_EPSILON)
        return true;

    const DWORD options = limitEffect ? QCHAM_OPTION_LIMIT_EFFECT : 0;
    return mesh.QuadChamfer(settings.amount, settings.segments, settings.tension, options, QCHAM_CHAMFEREDOBJECT, flag, QCHAM_VERSION_OLYMPUS);
}

bool ChamferProMod::ApplyChamferPro(TimeValue t, MNMesh& mesh)
{
    // Work transactionally.  An unsuccessful pass must never leave a partly
    // modified topology in the object being evaluated by 3ds Max.
    MNMesh working(mesh);

    if (!working.GetFlag(MN_MESH_FILLED_IN))
        working.FillInMesh();

    working.ClearSpecifiedNormals();

    constexpr DWORD chamferFlag = MN_USER;
    if (!PrepareChamferFlags(working, chamferFlag))
        return false;

    const bool limitEffect = GetLimitEffect(t);
    const ChamferPassSettings pass1 = GetPassSettings(t, 1);
    const ChamferPassSettings pass2 = GetPassSettings(t, 2);

    bool haveSecondPassTargets = true;
    if (pass1.amount > FLT_EPSILON)
    {
        working.ClearEFlags(MN_EDGE_WAS_PROCESSED);
        if (!ApplyOneChamferPass(working, chamferFlag, pass1, limitEffect))
            return false;

        working.CollapseDeadStructs();
        working.FillInMesh();
        if (!working.CheckAllData())
            return false;

        haveSecondPassTargets = PrepareSecondPassFlags(working, chamferFlag);
    }

    if (pass2.amount > FLT_EPSILON && haveSecondPassTargets)
    {
        working.ClearEFlags(MN_EDGE_WAS_PROCESSED);
        if (!ApplyOneChamferPass(working, chamferFlag, pass2, limitEffect))
            return false;

        working.CollapseDeadStructs();
        working.FillInMesh();
        if (!working.CheckAllData())
            return false;
    }

    working.ClearEFlags(chamferFlag | MN_EDGE_WAS_PROCESSED);
    working.InvalidateTopoCache();
    working.FillInMesh();

    mesh = working;
    return true;
}

void ChamferProMod::ModifyObject(TimeValue t, ModContext& mc, ObjectState* os, INode* node)
{
    if (!os || !os->obj)
        return;

    PolyObject* polyObj = nullptr;
    bool converted = false;

    if (os->obj->IsSubClassOf(polyObjectClassID))
    {
        polyObj = static_cast<PolyObject*>(os->obj);
    }
    else if (os->obj->CanConvertToType(polyObjectClassID))
    {
        polyObj = static_cast<PolyObject*>(os->obj->ConvertToType(t, polyObjectClassID));
        converted = true;
    }

    if (!polyObj)
        return;

    ApplyChamferPro(t, polyObj->GetMesh());

    const Interval validity = LocalValidity(t);
    polyObj->UpdateValidity(GEOM_CHAN_NUM, validity);
    polyObj->UpdateValidity(TOPO_CHAN_NUM, validity);
    polyObj->UpdateValidity(VERT_COLOR_CHAN_NUM, validity);
    polyObj->UpdateValidity(TEXMAP_CHAN_NUM, validity);
    polyObj->UpdateValidity(SELECT_CHAN_NUM, validity);

    if (converted)
        os->obj = polyObj;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        hInstance = hinstDLL;
        DisableThreadLibraryCalls(hInstance);
    }
    return TRUE;
}

extern "C" __declspec(dllexport) const TCHAR* LibDescription()
{
    return _T("ChamferPro - one modifier, two internal native SDK chamfer passes");
}

extern "C" __declspec(dllexport) int LibNumberClasses()
{
    return 1;
}

extern "C" __declspec(dllexport) ClassDesc* LibClassDesc(int i)
{
    return (i == 0) ? &gChamferProDesc : nullptr;
}

extern "C" __declspec(dllexport) ULONG LibVersion()
{
    return VERSION_3DSMAX;
}

extern "C" __declspec(dllexport) ULONG CanAutoDefer()
{
    return 1;
}