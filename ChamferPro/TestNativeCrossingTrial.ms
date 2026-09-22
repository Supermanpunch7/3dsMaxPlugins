-- Batch-only mixed four-way intersection regression. Not a general lineage proof.
(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local log = createFile (d + "native-crossing-test.log")
    local failed = 0
    local cases = 0
    local n = undefined
    fn hardAt n e =
    (
        local fs = (polyop.getFacesUsingEdge n #{e}) as array
        fs.count != 2 or (bit.and (polyop.getFaceSmoothGroup n fs[1]) (polyop.getFaceSmoothGroup n fs[2])) == 0
    )
    try
    (
        loadDllsFromDir (d + @"bin\NativeCrossingTrial\") "ChamferProCrossingTrial.dlm"
        local cls = undefined
        for c in modifier.classes where c.classID[1] == 0x71293b48 and c.classID[2] == 0x19374a69 do cls = c
        if cls == undefined do throw "Crossing class missing"
        for seg in #(0,1,2,4) do for inset in #(false,true) do
        (
            n = plane length:100 width:100 lengthsegs:2 widthsegs:2
            convertToPoly n
            -- Fold the soft crossing direction: a flat plane with a hard-only
            -- filter never generated the transverse intersection being tested.
            for v = 1 to polyop.getNumVerts n do
            (
                local p = polyop.getVert n v
                p.z = abs p.x * 0.5
                polyop.setVert n v p
            )
            for f = 1 to polyop.getNumFaces n do
                polyop.setFaceSmoothGroup n #{f} (if (polyop.getFaceCenter n f).y < 0 then 1 else 2)
            local m = cls()
            m.chamfer1.selectionoption = 5
            m.chamfer1.smoothingoption = 0
            m.chamfer1.useminangle = false
            m.chamfer1.usemaxangle = false
            m.chamfer1.amountType = 0
            m.chamfer1.amount = 2
            m.chamfer1.segments = seg
            m.chamfer1.addinset = inset
            m.chamfer1.insetamount = 0.5
            if m.chamfer1.selectionoption != 5 or m.chamfer1.smoothingoption != 0 do throw "Selection readback failed"
            m.chamfer2.enabled = false
            addModifier n m
            local checked = 0
            local bad = 0
            local softChecked = 0
            for e = 1 to polyop.getNumEdges n do
            (
                local vs = polyop.getEdgeVerts n e
                local a = polyop.getVert n vs[1]
                local b = polyop.getVert n vs[2]
                local delta = b-a
                if length a < 10 and length b < 10 and length delta > 0.00001 do
                (
                    format "CENTER seg=% inset=% a=% b=% hard=%\n" seg inset a b (hardAt n e) to:log
                    if abs delta.x > 0.00001 and abs delta.y < 0.00001 do
                    (
                        checked += 1
                        if not (hardAt n e) do bad += 1
                    )
                    if abs delta.y > 0.00001 and abs delta.x < 0.00001 do
                    (
                        softChecked += 1
                        if hardAt n e do bad += 1
                    )
                )
            )
            local before = for e = 1 to polyop.getNumEdges n collect hardAt n e
            local ends = for e = 1 to before.count collect polyop.getEdgeVerts n e
            addModifier n (Edit_Poly())
            if polyop.getNumEdges n != before.count do throw "Downstream topology count changed"
            for e = 1 to before.count do
            (
                local ev = polyop.getEdgeVerts n e
                if ev[1] != ends[e][1] or ev[2] != ends[e][2] do throw "Downstream topology changed"
                if hardAt n e != before[e] do bad += 1
            )
            if checked == 0 or softChecked == 0 or bad != 0 do failed += 1
            cases += 1
            format "CASE seg=% inset=% hardChecked=% softChecked=% mismatch=%\n" seg inset checked softChecked bad to:log
            flush log
            delete n
            n = undefined
        )
    )
    catch (failed += 1; format "ERROR %\n" (getCurrentException()) to:log)
    if isValidNode n do delete n
    format "COMPLETE cases=% failed=%\n" cases failed to:log
    close log
)