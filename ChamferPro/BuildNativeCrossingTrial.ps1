$ErrorActionPreference = 'Stop'
$p = $PSScriptRoot
$h = [IO.File]::ReadAllText("$p\NativeCornerTrial.h")
$h = '#include "CrossingDirection.h"' + "`r`n" + $h
$h = $h.Replace('NativeCornerTrial','NativeCrossingTrial').Replace('CHAMFERPRO_CORNER_LOG','CHAMFERPRO_CROSSING_LOG')
$needle = 'if (hard[e] && edge.f1>=0 && edge.f2>=0) {'
if (-not $h.Contains($needle)) { throw 'Crossing hook missing' }
$replacement = @'
// Mixed junction: follow only unambiguous parallel incident parent directions.
if (a>=0 && a==b && cornerState[a]==2) {
    Point3 delta = dst.v[edge.v2].p-dst.v[edge.v1].p;
    double direction[3] = {delta.x,delta.y,delta.z};
    std::vector<double> rays;
    std::vector<int> states;
    for (int p=0; p<src.nume; ++p) {
        const auto& parent=src.e[p];
        if (parent.v1!=a && parent.v2!=a) continue;
        Point3 ray=src.v[parent.v2].p-src.v[parent.v1].p;
        rays.push_back(ray.x); rays.push_back(ray.y); rays.push_back(ray.z);
        states.push_back(TrialHard(src,p)?1:0);
    }
    int state=CrossingDirectionState(direction,rays.data(),states.data(),int(states.size()));
    if (state>=0) { hard[e]=state==1; ++resolved; }
}
if (hard[e] && edge.f1>=0 && edge.f2>=0) {
'@
$h = $h.Replace($needle,$replacement)
[IO.File]::WriteAllText("$p\NativeCrossingTrial.h",$h)
$s = [IO.File]::ReadAllText("$p\NativeCornerTrial.cpp").Replace('NativeCornerTrial','NativeCrossingTrial').Replace('ChamferProCornerTrial','ChamferProCrossingTrial').Replace('0x71293b47, 0x19374a68','0x71293b48, 0x19374a69')
[IO.File]::WriteAllText("$p\NativeCrossingTrial.cpp",$s)
$v = [IO.File]::ReadAllText("$p\NativeCornerTrial.vcxproj").Replace('NativeCornerTrial','NativeCrossingTrial').Replace('ChamferProCornerTrial','ChamferProCrossingTrial')
[IO.File]::WriteAllText("$p\NativeCrossingTrial.vcxproj",$v)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\NativeCrossingTrial.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\native-crossing-build.log"
$code = $LASTEXITCODE
[IO.File]::WriteAllText("$p\native-crossing-build.result","$code")
if ($code -ne 0) { throw "Crossing build failed: $code" }