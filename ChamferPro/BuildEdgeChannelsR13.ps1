$ErrorActionPreference = 'Stop'
$p = $PSScriptRoot
$s = [IO.File]::ReadAllText("$p\CornerWeightR9.cpp")
$s = $s.Replace('0x71293b6b, 0x19374a92', '0x71293b71, 0x19374a98').Replace('ChamferProCornerWeightR9', 'ChamferProEdgeChannelsR13')
$old = 'ApplyWeightFaceLineage(sourceMesh, outputMesh, weightTag));'
if (!$s.Contains($old)) { throw 'Missing lineage hook' }
$new = @'
ApplyWeightFaceLineage(sourceMesh, outputMesh, weightTag, false));
            LogNativeCornerTrial(
                20 + stage,
                ApplyWeightFaceLineage(sourceMesh, outputMesh, weightTag, false, EDATA_CREASE));
            LogNativeCornerTrial(
                30 + stage,
                ApplyWeightFaceLineage(sourceMesh, outputMesh, weightTag, false, EDATA_DEPTH));
            EndWeightFaceLineage(outputMesh, weightTag);
'@
$s = $s.Replace($old, $new)
[IO.File]::WriteAllText("$p\EdgeChannelsR13.cpp", $s)
$v = [IO.File]::ReadAllText("$p\CornerWeightR9.vcxproj").Replace('CornerWeightR9', 'EdgeChannelsR13')
[IO.File]::WriteAllText("$p\EdgeChannelsR13.vcxproj", $v)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\EdgeChannelsR13.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\edge-channels-r13-build.log"
[IO.File]::WriteAllText("$p\edge-channels-r13-build.result", "$LASTEXITCODE")
if ($LASTEXITCODE -ne 0) { throw 'R13 build failed; see build log' }