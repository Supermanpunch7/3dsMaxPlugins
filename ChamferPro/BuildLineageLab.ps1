$ErrorActionPreference = 'Stop'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$PSScriptRoot\LineageLab.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal 2>&1 | Out-File "$PSScriptRoot\lab-build.log" -Encoding utf8 -Width 300
$LASTEXITCODE | Set-Content "$PSScriptRoot\lab-build.exitcode"