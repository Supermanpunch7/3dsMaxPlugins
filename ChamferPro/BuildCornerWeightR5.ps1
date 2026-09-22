$ErrorActionPreference='Stop'
$p=$PSScriptRoot
$h=[IO.File]::ReadAllText("$p\CornerWeightR4.h")
$hook='    // Weight is an independent edge channel.'
if(!$h.Contains($hook)){throw 'Missing hook'}
$h=$h.Replace('#include "WeightFaceCandidates.h"', '#include "WeightFaceCandidates.h"' + "`r`n" + '#include "WeightRegionConsensus.h"')
$h=$h.Replace($hook,'    if (sourceWeight) ResolveWeightRegionConsensus(dst, sourceWeight, weightParent);' + "`r`n" + $hook)
[IO.File]::WriteAllText("$p\CornerWeightR5.h",$h)
$s=[IO.File]::ReadAllText("$p\CornerWeightR4.cpp").Replace('CornerWeightR4.h','CornerWeightR5.h').Replace('0x71293b64, 0x19374a85','0x71293b66, 0x19374a87').Replace('ChamferProCornerWeightR4','ChamferProCornerWeightR5')
[IO.File]::WriteAllText("$p\CornerWeightR5.cpp",$s)
$v=[IO.File]::ReadAllText("$p\CornerWeightR4.vcxproj").Replace('CornerWeightR4','CornerWeightR5')
[IO.File]::WriteAllText("$p\CornerWeightR5.vcxproj",$v)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\CornerWeightR5.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\corner-weight-r5-build.log"
[IO.File]::WriteAllText("$p\corner-weight-r5-build.result","$LASTEXITCODE")
if($LASTEXITCODE -ne 0){throw 'Build failed'}
foreach($suffix in @('', 'Five')) {
 $t=[IO.File]::ReadAllText("$p\TestObject003R4$suffix.ms").Replace('CornerWeightR4','CornerWeightR5').Replace('0x71293b64','0x71293b66').Replace('0x19374a85','0x19374a87').Replace('object003-r4','object003-r5').Replace('R4','R5').Replace('#(3,4)','#(3,5)').Replace('version == 4','version == 5')
 [IO.File]::WriteAllText("$p\TestObject003R5$suffix.ms",$t)
}