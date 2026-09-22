$ErrorActionPreference = 'Stop'
$p = $PSScriptRoot
try {
 $dst = "$p\bin\Hybrid"
 $backup = "$p\backup-controls-$(Get-Date -Format yyyyMMdd-HHmmss)"
 New-Item -ItemType Directory -Path "$backup\en-US" -Force | Out-Null
 Copy-Item "$dst\ChamferPro.dlm" "$backup\ChamferPro.dlm"
 Copy-Item "$dst\en-US\ChamferPro.dlm.mui" "$backup\en-US\ChamferPro.dlm.mui"
 Copy-Item "$p\bin\Controls\ChamferPro.dlm" "$dst\ChamferPro.dlm" -Force
 Copy-Item "$p\bin\Controls\en-US\ChamferPro.dlm.mui" "$dst\en-US\ChamferPro.dlm.mui" -Force
 $a = (Get-FileHash "$p\bin\Controls\ChamferPro.dlm").Hash
 $b = (Get-FileHash "$dst\ChamferPro.dlm").Hash
 if ($a -ne $b) { throw 'Installed hash mismatch' }
 "PASS installed; backup=$backup; SHA256=$b" | Set-Content "$p\controls-install.log"
 Start-Process 'C:\Program Files\Autodesk\3ds Max 2027\3dsmaxbatch.exe' -ArgumentList ('"'+$p+'\VerifyControls.ms"') -RedirectStandardOutput "$p\controls-batch.log" -RedirectStandardError "$p\controls-error.log"
} catch { $_ | Out-String | Set-Content "$p\controls-install.log"; throw }