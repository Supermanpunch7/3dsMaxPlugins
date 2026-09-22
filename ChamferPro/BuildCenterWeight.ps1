$ErrorActionPreference='Stop'
$p=$PSScriptRoot
$s=[IO.File]::ReadAllText("$p\R5Defaults.cpp")
$s=$s.Replace('0x71293ba1, 0x19374ac1','0x71293bb2, 0x19374ad2').Replace('ChamferProR5Defaults','ChamferProCenterWeight')
$include='#include "CornerWeightR5.h"'
if (!$s.Contains($include)) {throw 'Missing R5 header'}
$s=$s.Replace($include, $include+"`r`n"+'#include "WeightCenterContinuation.h"')
$hook="        }`r`n    }`r`n};`r`nvoid* PairDesc::Create"
if (!$s.Contains($hook)) {
    $s=$s.Replace("`r`n","`n").Replace("`n","`r`n")
}
if (!$s.Contains($hook)) {throw 'Missing final-output hook'}
$replacement="        }`r`n        if (haveOriginal && os->obj->IsSubClassOf(polyObjectClassID))`r`n            RestoreCenterContinuation(originalMesh, static_cast<PolyObject*>(os->obj)->GetMesh());`r`n    }`r`n};`r`nvoid* PairDesc::Create"
$s=$s.Replace($hook,$replacement)
[IO.File]::WriteAllText("$p\CenterWeight.cpp",$s)
$v=[IO.File]::ReadAllText("$p\R5Defaults.vcxproj").Replace('R5Defaults','CenterWeight')
[IO.File]::WriteAllText("$p\CenterWeight.vcxproj",$v)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\CenterWeight.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\center-weight-build.log"
[IO.File]::WriteAllText("$p\center-weight-build.result","$LASTEXITCODE")
if ($LASTEXITCODE -ne 0) {throw 'Build failed'}