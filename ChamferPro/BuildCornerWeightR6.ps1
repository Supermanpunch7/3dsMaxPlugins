$ErrorActionPreference='Stop'
$p=$PSScriptRoot
$h=[IO.File]::ReadAllText("$p\CornerWeightR5.h")
$hook='    if (sourceWeight) ResolveWeightRegionConsensus(dst, sourceWeight, weightParent);'
if(!$h.Contains($hook)){throw 'Missing hook'}
$h=$h.Replace('#include "WeightRegionConsensus.h"','#include "WeightRegionConsensus.h"' + "`r`n" + '#include "WeightQuadStrips.h"')
$h=$h.Replace($hook,'    if (sourceWeight) ResolveWeightQuadStrips(dst, sourceWeight, weightParent);' + "`r`n" + $hook)
[IO.File]::WriteAllText("$p\CornerWeightR6.h",$h)
$s=[IO.File]::ReadAllText("$p\CornerWeightR5.cpp").Replace('CornerWeightR5.h','CornerWeightR6.h').Replace('0x71293b66, 0x19374a87','0x71293b67, 0x19374a88').Replace('ChamferProCornerWeightR5','ChamferProCornerWeightR6')
[IO.File]::WriteAllText("$p\CornerWeightR6.cpp",$s)
$v=[IO.File]::ReadAllText("$p\CornerWeightR5.vcxproj").Replace('CornerWeightR5','CornerWeightR6')
[IO.File]::WriteAllText("$p\CornerWeightR6.vcxproj",$v)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\CornerWeightR6.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\corner-weight-r6-build.log"
[IO.File]::WriteAllText("$p\corner-weight-r6-build.result","$LASTEXITCODE")
if($LASTEXITCODE -ne 0){throw 'Build failed'}
foreach($suffix in @('', 'Five')) {
 $t=[IO.File]::ReadAllText("$p\TestObject003R5$suffix.ms").Replace('CornerWeightR5','CornerWeightR6').Replace('0x71293b66','0x71293b67').Replace('0x19374a87','0x19374a88').Replace('object003-r5','object003-r6').Replace('R5','R6').Replace('#(3,5)','#(3,6)').Replace('version == 5','version == 6')
 [IO.File]::WriteAllText("$p\TestObject003R6$suffix.ms",$t)
}