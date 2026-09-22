$ErrorActionPreference='Stop'
$p=$PSScriptRoot
$rows=[IO.File]::ReadAllLines("$p\default-settings-live.log")
$stage=-1
$calls=@()
foreach($row in $rows) {
 if($row -eq 'STAGE') {$stage++; continue}
 if($row -notmatch '^#([^=]+)=(.+)$'){continue}
 $name=$Matches[1]; $value=$Matches[2]
 if($name -eq 'presetOptionsComboBox'){continue}
 if($value -eq 'true'){$value='1'} elseif($value -eq 'false'){$value='0'} elseif($value.Contains('.')){$value+='f'}
 $calls+='    if (stage == '+$stage+') SetInitialValue(m, _T("'+$name+'"), '+$value+');'
}
if($stage -ne 1){throw 'Expected two captured stages'}
$helper=@'
// Apply only to new native stages; loading and cloning preserve stored settings.
template<class T> static void SetInitialValue(Modifier* m, const TCHAR* name, T value) {
    for (int b=0; b<m->NumParamBlocks(); ++b) {
        auto* p=m->GetParamBlock(b);
        if (!p) continue;
        for (int j=0; j<p->NumParams(); ++j) {
            ParamID id=p->IndextoID(j);
            const auto& def=p->GetParamDef(id);
            if (def.int_name && _tcsicmp(def.int_name,name)==0) {
                p->SetValue(id,0,value); return;
            }
        }
    }
}
static void ApplyInitialSettings(Modifier* m, int stage) {
    if (!m) return;
'@
$helper+="`r`n"+($calls -join "`r`n")+"`r`n}`r`n"
$s=[IO.File]::ReadAllText("$p\CornerWeightR5.cpp")
$s=$s.Replace('0x71293b66, 0x19374a87','0x71293ba1, 0x19374ac1').Replace('ChamferProCornerWeightR5','ChamferProR5Defaults')
$s=$s.Replace('class NativePair : public Modifier {',$helper+"`r`nclass NativePair : public Modifier {")
$old='pb->SetValue(i, 0, static_cast<ReferenceTarget*>(CreateInstance(OSM_CLASS_ID, nativeID)));'
if(!$s.Contains($old)){throw 'Constructor hook missing'}
$s=$s.Replace($old,'{ auto* m = static_cast<Modifier*>(CreateInstance(OSM_CLASS_ID, nativeID)); ApplyInitialSettings(m, i); pb->SetValue(i, 0, static_cast<ReferenceTarget*>(m)); }')
[IO.File]::WriteAllText("$p\R5Defaults.cpp",$s)
$v=[IO.File]::ReadAllText("$p\CornerWeightR5.vcxproj").Replace('CornerWeightR5','R5Defaults')
[IO.File]::WriteAllText("$p\R5Defaults.vcxproj",$v)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\R5Defaults.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\r5-defaults-build.log"
[IO.File]::WriteAllText("$p\r5-defaults-build.result","$LASTEXITCODE")
if($LASTEXITCODE -ne 0){throw 'Build failed'}