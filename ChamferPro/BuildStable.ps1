$ErrorActionPreference='Stop'
$p=$PSScriptRoot
$v=[IO.File]::ReadAllText("$p\NativePrototype.vcxproj").Replace('\bin\NativeTest\','\bin\Stable\').Replace('\obj\NativeTest\','\obj\Stable\')
[IO.File]::WriteAllText("$p\Stable.vcxproj",$v)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\Stable.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal 2>&1 | Out-File "$p\stable-build.log" -Encoding utf8 -Width 300
$LASTEXITCODE | Set-Content "$p\stable-build.exitcode"
if ($LASTEXITCODE -ne 0) { throw 'Stable build failed' }