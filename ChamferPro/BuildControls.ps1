$ErrorActionPreference = 'Stop'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$PSScriptRoot\Chmferpro.vcxproj" /t:Build /p:Configuration=Hybrid /p:Platform=x64 /v:minimal /nologo 2>&1 | Out-File "$PSScriptRoot\controls-build.log" -Encoding utf8 -Width 300
$LASTEXITCODE | Set-Content "$PSScriptRoot\controls-build.result"