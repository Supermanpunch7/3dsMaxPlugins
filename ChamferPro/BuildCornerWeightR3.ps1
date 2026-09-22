$ErrorActionPreference = 'Stop'
$p = $PSScriptRoot
# Separate snapshot: never overwrite the approved CornerTrial or loaded R2.
$h = [IO.File]::ReadAllText("$p\CornerWeightTrial.h")
$guard = 'if (src.GetSpecifiedNormals() || dst.GetSpecifiedNormals()) return -1;'
if (!$h.Contains($guard)) { throw 'Expected normal guard missing' }
$h = $h.Replace($guard, 'const bool preserveNormals = src.GetSpecifiedNormals() || dst.GetSpecifiedNormals();')
$hook = 'if (!resolved) return 0;'
if (!$h.Contains($hook)) { throw 'Expected weight separation hook missing' }
$h = $h.Replace($hook, @'
// Weight is an independent edge channel. Do not suppress it because normals
// exist or because a subsequent smoothing-group allocation cannot succeed.
if (sourceWeight) {
    dst.setEDataSupport(EDATA_KNOT);
    float* targetWeight = dst.edgeFloat(EDATA_KNOT);
    if (!targetWeight) return -8;
    for (int e=0; e<dst.nume; ++e)
        if (weightParent[e]>=0) targetWeight[e]=sourceWeight[weightParent[e]];
}
// Keep specified normals and all native smoothing groups unchanged.
if (preserveNormals) return resolved;
if (!resolved) return 0;
'@)
[IO.File]::WriteAllText("$p\CornerWeightR3.h", $h)
$s = [IO.File]::ReadAllText("$p\CornerWeightTrial.cpp")
if (!$s.Contains('0x71293b62, 0x19374a83')) { throw 'Expected R2 class ID missing' }
$s = $s.Replace('CornerWeightTrial.h','CornerWeightR3.h').Replace('0x71293b62, 0x19374a83','0x71293b63, 0x19374a84').Replace('ChamferProCornerWeightR2','ChamferProCornerWeightR3')
[IO.File]::WriteAllText("$p\CornerWeightR3.cpp", $s)
$v = [IO.File]::ReadAllText("$p\CornerWeightTrial.vcxproj").Replace('CornerWeightR2','CornerWeightR3').Replace('CornerWeightTrial.cpp','CornerWeightR3.cpp')
[IO.File]::WriteAllText("$p\CornerWeightR3.vcxproj", $v)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\CornerWeightR3.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\corner-weight-r3-build.log"
$code = $LASTEXITCODE
[IO.File]::WriteAllText("$p\corner-weight-r3-build.result", "$code")
if ($code -ne 0) { throw "R3 build failed: $code" }