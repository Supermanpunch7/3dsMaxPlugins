$ErrorActionPreference = 'Stop'
$p = $PSScriptRoot
$h = [IO.File]::ReadAllText("$p\CornerWeightR3.h")
$hook = '    // Weight is an independent edge channel.'
if (!$h.Contains($hook)) { throw 'Weight hook missing' }
$h = $h.Replace('#include <cstdio>', "#include <cstdio>`r`n#include `"WeightFaceCandidates.h`"")
$h = $h.Replace($hook, "    if (sourceWeight) ResolveWeightFaceCandidates(src, dst, owner, weightParent);`r`n" + $hook)
[IO.File]::WriteAllText("$p\CornerWeightR4.h", $h)
$s = [IO.File]::ReadAllText("$p\CornerWeightR3.cpp")
if (!$s.Contains('0x71293b63, 0x19374a84')) { throw 'Expected R3 ID missing' }
$s = $s.Replace('CornerWeightR3.h','CornerWeightR4.h').Replace('0x71293b63, 0x19374a84','0x71293b64, 0x19374a85').Replace('ChamferProCornerWeightR3','ChamferProCornerWeightR4')
[IO.File]::WriteAllText("$p\CornerWeightR4.cpp", $s)
$v = [IO.File]::ReadAllText("$p\CornerWeightR3.vcxproj").Replace('CornerWeightR3','CornerWeightR4')
[IO.File]::WriteAllText("$p\CornerWeightR4.vcxproj", $v)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\CornerWeightR4.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\corner-weight-r4-build.log"
$code=$LASTEXITCODE
[IO.File]::WriteAllText("$p\corner-weight-r4-build.result", "$code")
if ($code -ne 0) { throw "R4 build failed: $code" }