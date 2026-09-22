$ErrorActionPreference = 'Stop'
$p = $PSScriptRoot
$h = [IO.File]::ReadAllText("$p\NativeHardTrial.h")
$h = $h.Replace('ApplyNativeHardTrial','ApplyNativeCornerTrial').Replace('LogNativeHardTrial','LogNativeCornerTrial').Replace('CHAMFERPRO_HARD_LOG','CHAMFERPRO_CORNER_LOG')
$needle = 'int resolved = 0;'
if (-not $h.Contains($needle)) { throw 'Source corner hook missing' }
$corner = @'
// A corner has a set of incident parents, not a unique nearest edge.
// Only unanimous incident states are inherited; mixed corners remain native.
std::vector<int> cornerState(src.numv, -1);
for (int e=0; e<src.nume; ++e) {
    int state = TrialHard(src,e) ? 1 : 0;
    for (int v : {src.e[e].v1, src.e[e].v2}) {
        if (cornerState[v] == -1) cornerState[v] = state;
        else if (cornerState[v] != state) cornerState[v] = 2;
    }
}
int resolved = 0;
'@
$h = $h.Replace($needle,$corner)
$needle = 'if (hard[e] && edge.f1>=0 && edge.f2>=0) {'
if (-not $h.Contains($needle)) { throw 'Output corner hook missing' }
$h = $h.Replace($needle, @'
if (a>=0 && a==b && cornerState[a]>=0 && cornerState[a]<2) {
    hard[e] = cornerState[a] == 1;
    ++resolved;
}
if (hard[e] && edge.f1>=0 && edge.f2>=0) {
'@)
[IO.File]::WriteAllText("$p\NativeCornerTrial.h",$h)
$s = [IO.File]::ReadAllText("$p\NativeHardTrial.cpp").Replace('NativeHardTrial.h','NativeCornerTrial.h').Replace('0x71293b45, 0x19374a66','0x71293b47, 0x19374a68').Replace('ChamferProHardTrial','ChamferProCornerTrial').Replace('ApplyNativeHardTrial','ApplyNativeCornerTrial').Replace('LogNativeHardTrial','LogNativeCornerTrial')
[IO.File]::WriteAllText("$p\NativeCornerTrial.cpp",$s)
$v = [IO.File]::ReadAllText("$p\NativeHardTrial.vcxproj").Replace('NativeHardTrial','NativeCornerTrial').Replace('ChamferProHardTrial','ChamferProCornerTrial')
[IO.File]::WriteAllText("$p\NativeCornerTrial.vcxproj",$v)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\NativeCornerTrial.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\native-corner-build.log"
$code = $LASTEXITCODE
[IO.File]::WriteAllText("$p\native-corner-build.result","$code")
if ($code -ne 0) { throw "Corner build failed: $code" }