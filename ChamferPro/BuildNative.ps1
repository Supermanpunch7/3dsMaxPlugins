$ErrorActionPreference = 'Stop'
$d = $PSScriptRoot
$s = [IO.File]::ReadAllText("$d\ChamferPro.vcxproj")
$s = $s.Replace('ChamferPro.cpp', 'NativePrototype.cpp').Replace('<TargetName>ChamferPro</TargetName>', '<TargetName>ChamferProNativeTest</TargetName>').Replace('\bin\$(Configuration)\', '\bin\NativeTest\').Replace('\obj\$(Platform)\$(Configuration)\', '\obj\NativeTest\')
$s = $s.Replace('ChamferPro.rc', 'NativePrototype.rc')
[IO.File]::WriteAllText("$d\NativePrototype.vcxproj", $s)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$d\NativePrototype.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$d\native-build.log"
$LASTEXITCODE | Set-Content "$d\native-build.exitcode" -Encoding ASCII
$log = Get-Content "$d\native-build.log" -Raw
Set-Content "$d\native-build.log" $log -Encoding UTF8