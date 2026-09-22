$ErrorActionPreference='Stop'
$p=$PSScriptRoot
$h=[IO.File]::ReadAllText("$p\CornerWeightR5.h")
$hook='    if (sourceWeight) ResolveWeightRegionConsensus(dst, sourceWeight, weightParent);'
if(!$h.Contains($hook)){throw 'Missing hook'}
$replacement=@'
    // A native non-default value is value evidence, not a unique edge owner.
    // Only seed unresolved edges, and only if that exact value exists in input.
    // Keep the original native value if evidence is absent or conflicting.
    if (sourceWeight) {
        const float* nativeWeight=dst.edgeFloat(EDATA_KNOT);
        if(nativeWeight) for(int e=0;e<dst.nume;++e) {
            if(weightParent[e]>=0 || nativeWeight[e]==1.0f) continue;
            for(int p=0;p<src.nume;++p) if(sourceWeight[p]==nativeWeight[e]) {
                weightParent[e]=p;
                break;
            }
        }
        ResolveWeightRegionConsensus(dst, sourceWeight, weightParent);
    }
'@
$h=$h.Replace($hook,$replacement)
[IO.File]::WriteAllText("$p\CornerWeightR7.h",$h)
$s=[IO.File]::ReadAllText("$p\CornerWeightR5.cpp").Replace('CornerWeightR5.h','CornerWeightR7.h').Replace('0x71293b66, 0x19374a87','0x71293b69, 0x19374a90').Replace('ChamferProCornerWeightR5','ChamferProCornerWeightR7')
[IO.File]::WriteAllText("$p\CornerWeightR7.cpp",$s)
$v=[IO.File]::ReadAllText("$p\CornerWeightR5.vcxproj").Replace('CornerWeightR5','CornerWeightR7')
[IO.File]::WriteAllText("$p\CornerWeightR7.vcxproj",$v)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' "$p\CornerWeightR7.vcxproj" /p:Configuration=Hybrid /p:Platform=x64 /nologo /v:minimal *> "$p\corner-weight-r7-build.log"
[IO.File]::WriteAllText("$p\corner-weight-r7-build.result","$LASTEXITCODE")
if($LASTEXITCODE -ne 0){throw 'Build failed'}
foreach($suffix in @('', 'Five')) {
 $t=[IO.File]::ReadAllText("$p\TestObject003R5$suffix.ms").Replace('CornerWeightR5','CornerWeightR7').Replace('0x71293b66','0x71293b69').Replace('0x19374a87','0x19374a90').Replace('object003-r5','object003-r7').Replace('R5','R7').Replace('#(3,5)','#(3,7)').Replace('version == 5','version == 7')
 [IO.File]::WriteAllText("$p\TestObject003R7$suffix.ms",$t)
}