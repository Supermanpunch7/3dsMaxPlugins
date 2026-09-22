$ErrorActionPreference = 'Stop'
$p = $PSScriptRoot
$source = [IO.File]::ReadAllText("$p\NativePrototype.cpp")
$source = '#include "NativeHardTrial.h"' + "`r`n" + $source
$source = $source.Replace('0x71293b41, 0x19374a62', '0x71293b45, 0x19374a66').Replace('ChamferProNativeTest', 'ChamferProHardTrial').Replace('_T("ChamferPro")', '_T("ChamferProHardTrial")')
$needle = 'm->ModifyObject(t, data->stages[i], os, node);'
if (-not $source.Contains($needle)) { throw 'Native hook missing' }
$hook = @'
MNMesh sourceMesh;
bool haveSource = os->obj->IsSubClassOf(polyObjectClassID) != 0;
if (haveSource) sourceMesh = static_cast<PolyObject*>(os->obj)->GetMesh();
m->ModifyObject(t, data->stages[i], os, node);
if (haveSource && os->obj->IsSubClassOf(polyObjectClassID))
    LogNativeHardTrial(i, ApplyNativeHardTrial(sourceMesh, static_cast<PolyObject*>(os->obj)->GetMesh()));
'@
$source = $source.Replace($needle, $hook)
[IO.File]::WriteAllText("$p\NativeHardTrial.cpp", $source)
$project = [IO.File]::ReadAllText("$p\Stable.vcxproj").Replace('\Stable\', '\NativeHardTrial\').Replace('ChamferProNativeTest', 'ChamferProHardTrial').Replace('NativePrototype.cpp', 'NativeHardTrial.cpp')
[IO.File]::WriteAllText("$p\NativeHardTrial.vcxproj", $project)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\NativeHardTrial.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\native-hard-trial-build.log"
$code = $LASTEXITCODE
[IO.File]::WriteAllText("$p\native-hard-trial-build.result", "$code")
if ($code -ne 0) { throw "Hard trial build failed: $code" }