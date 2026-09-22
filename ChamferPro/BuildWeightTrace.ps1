$ErrorActionPreference='Stop'
$p=$PSScriptRoot
$h=[IO.File]::ReadAllText("$p\CornerWeightR6.h")
$hook='    // Weight is an independent edge channel.'
if(!$h.Contains($hook)){throw 'Missing trace hook'}
$h=$h.Replace('#include <cstdio>','#include <cstdio>' + "`r`n" + '#include "WeightAssignmentTrace.h"')
$h=$h.Replace($hook,'    TraceWeightAssignments(src,dst,owner,weightParent);' + "`r`n" + $hook)
[IO.File]::WriteAllText("$p\WeightTrace.h",$h)
$s=[IO.File]::ReadAllText("$p\CornerWeightR6.cpp").Replace('CornerWeightR6.h','WeightTrace.h').Replace('0x71293b67, 0x19374a88','0x71293b68, 0x19374a89').Replace('ChamferProCornerWeightR6','ChamferProWeightTrace')
[IO.File]::WriteAllText("$p\WeightTrace.cpp",$s)
$v=[IO.File]::ReadAllText("$p\CornerWeightR6.vcxproj").Replace('CornerWeightR6','WeightTrace')
[IO.File]::WriteAllText("$p\WeightTrace.vcxproj",$v)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\WeightTrace.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\weight-trace-build.log"
[IO.File]::WriteAllText("$p\weight-trace-build.result","$LASTEXITCODE")
if($LASTEXITCODE -ne 0){throw 'Build failed'}
$t=[IO.File]::ReadAllText("$p\TestObject003R6.ms").Replace('CornerWeightR6','WeightTrace').Replace('0x71293b67','0x71293b68').Replace('0x19374a88','0x19374a89').Replace('object003-r6-test.log','weight-trace-live.log')
$t=$t.Replace(' local saved = selection as array',' local saved = selection as array' + "`r`n" + ' local env=dotNetClass "System.Environment"' + "`r`n" + ' local previous=env.GetEnvironmentVariable "CHAMFERPRO_ASSIGNMENT_TRACE"' + "`r`n" + ' if previous==undefined do previous=""' + "`r`n" + ' env.SetEnvironmentVariable "CHAMFERPRO_ASSIGNMENT_TRACE" (d+"weight-assignment-trace.log")')
$t=$t.Replace(' close log',' env.SetEnvironmentVariable "CHAMFERPRO_ASSIGNMENT_TRACE" previous' + "`r`n" + ' close log')
[IO.File]::WriteAllText("$p\TestWeightTrace.ms",$t)