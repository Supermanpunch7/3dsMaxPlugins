$ErrorActionPreference = 'Stop'
$p = $PSScriptRoot
# Snapshot the approved CornerTrial implementation; never regenerate it from HardTrial.
$h = [IO.File]::ReadAllText("$p\NativeCornerTrial.h")
$needle = 'int resolved = 0;'
if (-not $h.Contains($needle)) { throw 'Corner weight hook missing' }
$h = $h.Replace($needle, @'
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
'@)
$needle = 'hard[e]=TrialHard(src,found->second); ++resolved;'
if (-not $h.Contains($needle)) { throw 'Parent weight hook missing' }
$h = $h.Replace($needle, 'hard[e]=TrialHard(src,found->second); weightParent[e]=found->second; ++resolved;')
$needle = 'if (hard[e] && edge.f1>=0 && edge.f2>=0) {'
$h = $h.Replace($needle, @'
// A corner has multiple candidate parents. Copy only an exactly unanimous value.
if (sourceWeight && a>=0 && a==b && cornerWeightParent[a]>=0)
    weightParent[e] = cornerWeightParent[a];
if (hard[e] && edge.f1>=0 && edge.f2>=0) {
'@)
$needle = 'for (int f=0; f<dst.numf; ++f) dst.f[f].smGroup=groups[f];'
if (-not $h.Contains($needle)) { throw 'Commit hook missing' }
$h = $h.Replace($needle, @'
// Commit only after the existing hard/soft validation succeeds.
if (sourceWeight) {
    dst.setEDataSupport(EDATA_KNOT);
    float* targetWeight = dst.edgeFloat(EDATA_KNOT);
    if (!targetWeight) return -8;
    for (int e=0; e<dst.nume; ++e)
        if (weightParent[e]>=0) targetWeight[e]=sourceWeight[weightParent[e]];
}
for (int f=0; f<dst.numf; ++f) dst.f[f].smGroup=groups[f];
'@)
[IO.File]::WriteAllText("$p\CornerWeightTrial.h",$h)
$s = [IO.File]::ReadAllText("$p\NativeCornerTrial.cpp").Replace('NativeCornerTrial.h','CornerWeightTrial.h').Replace('0x71293b47, 0x19374a68','0x71293b61, 0x19374a82').Replace('ChamferProCornerTrial','ChamferProCornerWeightTrial')
$needle = 'auto* data = static_cast<PairContext*>(mc.localData);'
# Only ModifyObject has the immediately following stage loop.
$hook = $needle + "`r`n        for (int i = 0; i < 2; ++i) {"
if (-not $s.Contains($hook)) { $hook = $needle + "`n        for (int i = 0; i < 2; ++i) {" }
if (-not $s.Contains($hook)) { throw 'Original input hook missing' }
$s = $s.Replace($hook, @'
auto* data = static_cast<PairContext*>(mc.localData);
        MNMesh originalMesh;
        bool haveOriginal = os->obj->IsSubClassOf(polyObjectClassID) != 0;
        if (haveOriginal) originalMesh = static_cast<PolyObject*>(os->obj)->GetMesh();
        for (int i = 0; i < 2; ++i) {
'@)
$needle = 'LogNativeCornerTrial(i, ApplyNativeCornerTrial(sourceMesh, static_cast<PolyObject*>(os->obj)->GetMesh()));'
if (-not $s.Contains($needle)) { throw 'Final inheritance hook missing' }
$s = $s.Replace($needle, $needle + @'

// The first chamfer creates short edges whose matching radii can be too small
// for stage two. Reapply only correspondences resolved against original input.
if (i == 1 && haveOriginal && os->obj->IsSubClassOf(polyObjectClassID))
    LogNativeCornerTrial(2, ApplyNativeCornerTrial(originalMesh, static_cast<PolyObject*>(os->obj)->GetMesh()));
'@)
$s = $s.Replace('0x71293b61, 0x19374a82','0x71293b62, 0x19374a83').Replace('ChamferProCornerWeightTrial','ChamferProCornerWeightR2')
[IO.File]::WriteAllText("$p\CornerWeightTrial.cpp",$s)
$v = [IO.File]::ReadAllText("$p\NativeCornerTrial.vcxproj").Replace('NativeCornerTrial','CornerWeightTrial').Replace('ChamferProCornerTrial','ChamferProCornerWeightTrial')
$v = $v.Replace('bin\CornerWeightTrial\','bin\CornerWeightR2\').Replace('obj\CornerWeightTrial\','obj\CornerWeightR2\').Replace('ChamferProCornerWeightTrial','ChamferProCornerWeightR2')
[IO.File]::WriteAllText("$p\CornerWeightTrial.vcxproj",$v)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\CornerWeightTrial.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\corner-weight-build.log"
$code = $LASTEXITCODE
[IO.File]::WriteAllText("$p\corner-weight-build.result","$code")
if ($code -ne 0) { throw "Weight build failed: $code" }