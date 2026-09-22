$ErrorActionPreference='Stop'
$p=$PSScriptRoot
$s=[IO.File]::ReadAllText("$p\LineageLab.cpp")
$s=$s.Replace('0x71293b42, 0x19374a63','0x71293b43, 0x19374a64').Replace('ChamferproLineageLab','ChamferproResearch')
$s='#include "ResearchTrace.h"'+"`r`n"+$s
$s=$s.Replace('MNMesh source(mesh);','ResearchTrace("pass-input",mesh); MNMesh source(mesh);')
$s=$s.Replace('using Position =','ResearchTrace("after-native-topology",mesh); ResearchTrace("source-after-topology",source); using Position =')
$s=$s.Replace('mesh.CollapseDeadStructs();','ResearchTrace("before-compact",mesh); mesh.CollapseDeadStructs(); ResearchTrace("after-compact",mesh);')
$s=$s.Replace('transferData();','transferData(); ResearchTrace("after-transfer",mesh); ResearchTrace("source-after-transfer",source);')
$s=$s.Replace('if (!LineageBuildNormals(mesh, outputHard)) return false;','if (!LineageBuildNormals(mesh, outputHard)) { ResearchTrace("normals-FAILED",mesh); return false; } ResearchTrace("after-normals",mesh);')
$s=$s.Replace('mesh = working;','ResearchTrace("before-assign",working); mesh = working; ResearchTrace("after-assign",mesh);')
$s=$s.Replace('ApplyChamferPro(t, polyObj->GetMesh());','bool success = ApplyChamferPro(t, polyObj->GetMesh()); ResearchTrace(success ? "modifier-SUCCESS" : "modifier-FAILED",polyObj->GetMesh());')
[IO.File]::WriteAllText("$p\Research.cpp",$s)
$v=[IO.File]::ReadAllText("$p\LineageLab.vcxproj").Replace('LineageLab.cpp','Research.cpp').Replace('ChamferproLineageLab','ChamferproResearch').Replace('\LineageLab\','\Research\')
[IO.File]::WriteAllText("$p\Research.vcxproj",$v)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\Research.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal 2>&1 | Out-File "$p\research-build.log" -Encoding utf8 -Width 300
$LASTEXITCODE | Set-Content "$p\research-build.exitcode"
$t=[IO.File]::ReadAllText("$p\McpCompareRead.ms").Replace('0x71293b42','0x71293b43').Replace('0x19374a63','0x19374a64').Replace('\LineageLab\','\Research\').Replace('ChamferproLineageLab','ChamferproResearch')
[IO.File]::WriteAllText("$p\McpResearchStages.ms",$t)