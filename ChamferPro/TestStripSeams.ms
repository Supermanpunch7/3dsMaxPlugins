-- Isolated batch only: no user scene is loaded or modified.
(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local log = createFile (d + "strip-seams-test.log")
    local n = undefined
    local cases=0; local failures=0; local seams=0; local changed=0
    fn hardAt n e =
    (
        local fs=(polyop.getFacesUsingEdge n #{e}) as array
        if fs.count!=2 then true else (bit.and (polyop.getFaceSmoothGroup n fs[1]) (polyop.getFaceSmoothGroup n fs[2]))==0
    )
    try
    (
        loadDllsFromDir (d + @"bin\HardSeamValidation\") "ChamferProHardTrial.dlm"
        local cls=undefined
        for c in modifier.classes where c.classID[1]==0x71293b45 and c.classID[2]==0x19374a66 do cls=c
        if cls==undefined do throw "HardTrial missing"
        for seg in #(0,1,2,4) do for insetOn in #(false,true) do for smoothOn in #(false,true) do for mixed in #(false,true) do
        (
            n=box length:100 width:100 height:100
            convertToPoly n
            for f=1 to polyop.getNumFaces n do polyop.setFaceSmoothGroup n #{f} (if mixed and f<=3 then 1 else 0)
            local points=for v=1 to polyop.getNumVerts n collect polyop.getVert n v
            local parents=for e=1 to polyop.getNumEdges n collect polyop.getEdgeVerts n e
            local states=for e=1 to parents.count collect hardAt n e
            local m=cls()
            m.chamfer2.enabled=false
            m.chamfer1.amount=2; m.chamfer1.segments=seg
            m.chamfer1.smooth=smoothOn; m.chamfer1.addinset=insetOn
            m.chamfer1.insetamount=0.2; m.chamfer1.insetoffset=-0.2; m.chamfer1.insetsegments=0
            addModifier n m
            local owners=for v=1 to polyop.getNumVerts n collect
            (
                local a=0
                for k=1 to points.count where distance (polyop.getVert n v) points[k]<20 do a=k
                a
            )
            local strip=for f=1 to polyop.getNumFaces n collect
            (
                local ids=#()
                for v in polyop.getFaceVerts n f do appendIfUnique ids owners[v]
                ids.count==2 and findItem ids 0==0
            )
            local bad=0; local count=0; local parentCount=0; local unknownHard=0
            local before=for e=1 to polyop.getNumEdges n collect hardAt n e
            local topology=for e=1 to before.count collect polyop.getEdgeVerts n e
            for e=1 to before.count do
            (
                local vs=topology[e]; local a=owners[vs[1]]; local b=owners[vs[2]]
                local fs=(polyop.getFacesUsingEdge n #{e}) as array
                if a>0 and a==b and fs.count==2 and strip[fs[1]] and strip[fs[2]] then
                (
                    count+=1
                    if before[e] do bad+=1
                )
                else if a>0 and b>0 and a!=b then
                (
                    for p=1 to parents.count where ((parents[p][1]==a and parents[p][2]==b) or (parents[p][1]==b and parents[p][2]==a)) do
                    (
                        parentCount+=1
                        if before[e]!=states[p] do bad+=1
                    )
                )
                else if before[e] do unknownHard+=1
            )
            addModifier n (Edit_Poly())
            if polyop.getNumEdges n!=before.count do throw "Edit Poly topology changed"
            for e=1 to before.count do if (polyop.getEdgeVerts n e)!=topology[e] or (hardAt n e)!=before[e] do bad+=1
            format "seg=% inset=% smooth=% mixed=% seams=% parents=% unknownHard=% mismatches=%\n" seg insetOn smoothOn mixed count parentCount unknownHard bad to:log
            seams+=count; cases+=1
            if bad>0 do failures+=1
            flush log
            delete n; n=undefined
        )
        format "COMPLETE cases=% failures=% classifiedSeams=%\n" cases failures seams to:log
    )
    catch (format "ERROR %\n" (getCurrentException()) to:log)
    if isValidNode n do delete n
    close log
)