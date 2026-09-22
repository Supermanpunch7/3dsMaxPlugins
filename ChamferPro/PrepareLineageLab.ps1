$ErrorActionPreference = 'Stop'
$p = $PSScriptRoot
$s = [IO.File]::ReadAllText("$p\ChamferPro.cpp")
$s = $s.Replace('0x6f2a4d31, 0x41d07c92', '0x71293b42, 0x19374a63').Replace('_T("ChamferPro")', '_T("ChamferproLineageLab")')
if (Test-Path "$p\LineageLab.cpp") { throw 'Lab already exists; refusing overwrite' }
[IO.File]::WriteAllText("$p\LineageLab.cpp", $s)
$v = [IO.File]::ReadAllText("$p\ChamferPro.vcxproj")
$v = $v.Replace('ChamferPro.cpp', 'LineageLab.cpp').Replace('<TargetName>ChamferPro</TargetName>', '<TargetName>ChamferproLineageLab</TargetName>').Replace('\bin\$(Configuration)\', '\bin\LineageLab\').Replace('\obj\$(Platform)\$(Configuration)\', '\obj\LineageLab\')
[IO.File]::WriteAllText("$p\LineageLab.vcxproj", $v)
Select-String -Path 'C:\Program Files\Autodesk\3ds Max 2027 SDK\maxsdk\include\mnmesh.h' -Pattern 'EDataSupport|VDataSupport|EDNum|VDNum|edgeFloat|vertexFloat' | Out-File "$p\lineage-api.txt" -Encoding utf8