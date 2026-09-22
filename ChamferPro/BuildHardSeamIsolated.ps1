$ErrorActionPreference = 'Stop'
$p = $PSScriptRoot
$source = [IO.File]::ReadAllText("$p\NativeHardTrial.cpp")
if (-not $source.Contains('0x71293b45, 0x19374a66')) { throw 'HardTrial class ID missing' }
$source = $source.Replace('0x71293b45, 0x19374a66', '0x71293b59, 0x19374a79').Replace('ChamferProHardTrial', 'ChamferProSeamIsolated')
[IO.File]::WriteAllText("$p\HardSeamIsolated.cpp", $source)
$project = [IO.File]::ReadAllText("$p\NativeHardTrial.vcxproj").Replace('\NativeHardTrial\', '\HardSeamIsolated\').Replace('ChamferProHardTrial', 'ChamferProSeamIsolated').Replace('NativeHardTrial.cpp', 'HardSeamIsolated.cpp')
[IO.File]::WriteAllText("$p\HardSeamIsolated.vcxproj", $project)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\HardSeamIsolated.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\seam-isolated-build.log"
if ($LASTEXITCODE -ne 0) { throw "Isolated build failed: $LASTEXITCODE" }