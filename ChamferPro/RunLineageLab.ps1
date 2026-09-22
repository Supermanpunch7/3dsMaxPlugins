$env:CHAMFERPRO_LAB_LOG = "$PSScriptRoot\lab-ancestry.log"
& 'C:\Program Files\Autodesk\3ds Max 2027\3dsmaxbatch.exe' "$PSScriptRoot\TestLineageLab.ms" -v 4 *> "$PSScriptRoot\lab-batch.log"
$LASTEXITCODE | Set-Content "$PSScriptRoot\lab-test.exitcode"