$ErrorActionPreference='Stop'
$p=$PSScriptRoot
$h=[IO.File]::ReadAllText("$p\CornerWeightR4.h")
$hook='    // Weight is an independent edge channel.'
if (!$h.Contains($hook)) { throw 'Missing hook' }
$diag=@'
    {
        wchar_t path[2048];
        if (GetEnvironmentVariableW(L"CHAMFERPRO_WEIGHT_DIAG",path,2048)) {
            FILE* f=nullptr;
            if (!_wfopen_s(&f,path,L"a") && f) {
                int noOwner=0, same=0, different=0, assigned=0;
                for (int e=0;e<dst.nume;++e) {
                    if(weightParent[e]>=0) { ++assigned; continue; }
                    int a=owner[dst.e[e].v1],b=owner[dst.e[e].v2];
                    if(a<0||b<0) ++noOwner; else if(a==b) ++same; else ++different;
                }
                fprintf(f,"srcV=%d srcE=%d dstV=%d dstE=%d assigned=%d noOwner=%d same=%d different=%d normals=%d\n",int(src.numv),int(src.nume),int(dst.numv),int(dst.nume),assigned,noOwner,same,different,int(preserveNormals));
                fclose(f);
            }
        }
    }
'@
$h=$h.Replace($hook,$diag+"`r`n"+$hook)
[IO.File]::WriteAllText("$p\WeightDiagnostic.h",$h)
$s=[IO.File]::ReadAllText("$p\CornerWeightR4.cpp").Replace('CornerWeightR4.h','WeightDiagnostic.h').Replace('0x71293b64, 0x19374a85','0x71293b65, 0x19374a86').Replace('ChamferProCornerWeightR4','ChamferProWeightDiagnostic')
[IO.File]::WriteAllText("$p\WeightDiagnostic.cpp",$s)
$v=[IO.File]::ReadAllText("$p\CornerWeightR4.vcxproj").Replace('CornerWeightR4','WeightDiagnostic')
[IO.File]::WriteAllText("$p\WeightDiagnostic.vcxproj",$v)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\WeightDiagnostic.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\weight-diagnostic-build.log"
[IO.File]::WriteAllText("$p\weight-diagnostic-build.result","$LASTEXITCODE")
if($LASTEXITCODE -ne 0) {throw 'Build failed'}
$t=[IO.File]::ReadAllText("$p\TestObject003R4Five.ms").Replace('CornerWeightR4','WeightDiagnostic').Replace('0x71293b64','0x71293b65').Replace('0x19374a85','0x19374a86').Replace('object003-r4-five.log','weight-diagnostic-live.log')
$t=$t.Replace(' local saved = selection as array', ' local saved = selection as array' + "`r`n" + ' local env=dotNetClass "System.Environment"' + "`r`n" + ' local previousEnv=env.GetEnvironmentVariable "CHAMFERPRO_WEIGHT_DIAG"' + "`r`n" + ' env.SetEnvironmentVariable "CHAMFERPRO_WEIGHT_DIAG" (d+"weight-diagnostic-details.log")')
$t=$t.Replace(' close log', ' env.SetEnvironmentVariable "CHAMFERPRO_WEIGHT_DIAG" previousEnv' + "`r`n" + ' close log')
[IO.File]::WriteAllText("$p\TestWeightDiagnostic.ms",$t)