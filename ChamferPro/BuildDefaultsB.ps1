$ErrorActionPreference = 'Stop'
$p = $PSScriptRoot
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\Chmferpro.vcxproj" /t:Build /p:Configuration=Hybrid /p:Platform=x64 /v:minimal /nologo 2>&1 | Out-File "$p\defaults-b-build.log" -Encoding utf8
$LASTEXITCODE | Set-Content "$p\defaults-b-build.result"
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
try {
    $dst = "$p\bin\Hybrid"
    $backup = "$p\backup-defaults-b-$(Get-Date -Format yyyyMMdd-HHmmss)"
    New-Item -ItemType Directory -Path $backup | Out-Null
    if (Test-Path "$dst\ChamferPro.dlm") { Move-Item "$dst\ChamferPro.dlm" "$backup\ChamferPro.dlm" }
    Copy-Item "$p\bin\DefaultsB\ChamferPro.dlm" "$dst\ChamferPro.dlm"
    Copy-Item "$p\bin\DefaultsB\en-US\ChamferPro.dlm.mui" "$dst\en-US\ChamferPro.dlm.mui" -Force
    Get-FileHash "$dst\ChamferPro.dlm" | Format-List | Out-File "$p\defaults-b-install.log"
    Start-Process 'C:\Program Files\Autodesk\3ds Max 2027\3dsmaxbatch.exe' -ArgumentList ('"'+$p+'\VerifyReleaseDefaults.ms"') -RedirectStandardOutput "$p\defaults-b-batch.log" -RedirectStandardError "$p\defaults-b-error.log"
} catch {
    $_ | Out-String | Set-Content "$p\defaults-b-install.log"
    throw
}