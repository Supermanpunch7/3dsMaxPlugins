$ErrorActionPreference = 'Stop'
$p = $PSScriptRoot
# Generate isolated experiments; never write HardTrial or Stable artifacts.
$h = [IO.File]::ReadAllText("$p\NativeHardTrial.h")
$h = $h.Replace('ApplyNativeHardTrial', 'ApplyNativeFourTrial').Replace('LogNativeHardTrial', 'LogNativeFourTrial').Replace('CHAMFERPRO_HARD_LOG', 'CHAMFERPRO_FOUR_LOG')
$h = $h.Replace('int resolved = 0;', 'int resolved = 0; std::vector<int> parents(dst.nume, -1);')
$h = $h.Replace('hard[e]=TrialHard(src,found->second);', 'parents[e]=found->second; hard[e]=TrialHard(src,found->second);')
$needle = 'for (int f=0; f<dst.numf; ++f) dst.f[f].smGroup=groups[f];'
if (-not $h.Contains($needle)) { throw 'Commit hook missing' }
$commit = @'
// Stage on a copy so allocation/validation failure cannot partially commit.
MNMesh working(dst);
for (int ch : {0,1,3}) {
    if (!src.eDataSupport(ch)) continue;
    working.setEDataSupport(ch, TRUE);
    float* input = static_cast<float*>(src.edgeData(ch));
    float* output = static_cast<float*>(working.edgeData(ch));
    if (!input || !output) return -8;
    for (int e=0; e<working.nume; ++e)
        if (parents[e]>=0) output[e]=input[parents[e]];
}
for (int f=0; f<working.numf; ++f) working.f[f].smGroup=groups[f];
dst = working;
'@
$h = $h.Replace($needle, $commit)
[IO.File]::WriteAllText("$p\NativeFourTrial.h", $h)
$source = [IO.File]::ReadAllText("$p\NativeHardTrial.cpp").Replace('NativeHardTrial.h','NativeFourTrial.h').Replace('0x71293b45, 0x19374a66','0x71293b46, 0x19374a67').Replace('ChamferProHardTrial','ChamferProFourTrial').Replace('ApplyNativeHardTrial','ApplyNativeFourTrial').Replace('LogNativeHardTrial','LogNativeFourTrial')
[IO.File]::WriteAllText("$p\NativeFourTrial.cpp", $source)
$project = [IO.File]::ReadAllText("$p\NativeHardTrial.vcxproj").Replace('NativeHardTrial','NativeFourTrial').Replace('ChamferProHardTrial','ChamferProFourTrial')
[IO.File]::WriteAllText("$p\NativeFourTrial.vcxproj", $project)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\NativeFourTrial.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\native-four-trial-build.log"
$code = $LASTEXITCODE
[IO.File]::WriteAllText("$p\native-four-trial-build.result", "$code")
if ($code -ne 0) { throw "Four trial build failed: $code" }