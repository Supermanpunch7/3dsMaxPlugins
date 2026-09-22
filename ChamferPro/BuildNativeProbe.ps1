$ErrorActionPreference = 'Stop'
$p = $PSScriptRoot
$source = [IO.File]::ReadAllText("$p\NativePrototype.cpp")
$source = '#include <initializer_list>' + "`r`n" + '#include "NativeProbe.h"' + "`r`n" + $source
$source = $source.Replace('0x71293b41, 0x19374a62', '0x71293b44, 0x19374a65').Replace('ChamferProNativeTest', 'ChamferProNativeProbe').Replace('_T("ChamferPro")', '_T("ChamferProNativeProbe")')
$needle = 'm->ModifyObject(t, data->stages[i], os, node);'
if (-not $source.Contains($needle)) { throw 'Native evaluation hook missing' }
$source = $source.Replace($needle, 'NativeProbe(os, i, true); ' + $needle + ' NativeProbe(os, i, false);')
[IO.File]::WriteAllText("$p\NativeProbe.cpp", $source)
$project = [IO.File]::ReadAllText("$p\Stable.vcxproj").Replace('\Stable\', '\NativeProbe\').Replace('ChamferProNativeTest', 'ChamferProNativeProbe').Replace('NativePrototype.cpp', 'NativeProbe.cpp')
[IO.File]::WriteAllText("$p\NativeProbe.vcxproj", $project)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\NativeProbe.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\native-probe-build.log"
$code = $LASTEXITCODE
[IO.File]::WriteAllText("$p\native-probe-build.result", "$code")
if ($code -ne 0) { throw "Probe build failed: $code" }