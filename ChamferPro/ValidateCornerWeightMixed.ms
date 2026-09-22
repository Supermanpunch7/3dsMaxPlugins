-- Independent batch only; unresolved corner weights are explicitly counted.
(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local log = createFile (d + "corner-weight-r2-mixed.log")
    fn hardAt n e =
    (
        local fs = (polyop.getFacesUsingEdge n #{e}) as array
        if fs.count != 2 then true else (bit.and (polyop.getFaceSmoothGroup n fs[1]) (polyop.getFaceSmoothGroup n fs[2])) == 0
    )
    fn owner p corners =
    (
        local found = 0
        for i = 1 to corners.count where distance p corners[i] < 10 do
        (
            if found != 0 do throw "Ambiguous test owner"
            found = i
        )
        found
    )
    local cases = 0; local failed = 0
    try
    (
        loadDllsFromDir (d + @"bin\CornerWeightR2\") "ChamferProCornerWeightR2.dlm"
        local cls = undefined
        for c in modifier.classes where c.classID[1] == 0x71293b62 and c.classID[2] == 0x19374a83 do cls = c
        if cls == undefined do throw "R2 missing"
        for insetOn in #(false,true) do for secondOn in #(false,true) do for smoothOn in #(false,true) do
        (
            local n = box length:100 width:100 height:100
            convertToPoly n
            for f = 1 to 6 do polyop.setFaceSmoothGroup n #{f} (if f <= 3 then 1 else 2)
            polyop.setEDataChannelSupport n 1 true
            local weights = for e = 1 to 12 collect (0.25 + e*0.19)
            for e = 1 to 12 do polyop.setEDataValue n 1 #{e} weights[e]
            local corners = for v = 1 to 8 collect polyop.getVert n v
            local edges = for e = 1 to 12 collect polyop.getEdgeVerts n e
            local hard = for e = 1 to 12 collect hardAt n e
            local m = cls()
            m.chamfer1.amount = 0.2; m.chamfer1.segments = 1
            m.chamfer1.smooth = smoothOn; m.chamfer1.addinset = insetOn
            m.chamfer1.insetamount = 0.2; m.chamfer1.insetsegments = 0; m.chamfer1.insetoffset = -0.4
            m.chamfer2.enabled = secondOn; m.chamfer2.amount = 0.05
            m.chamfer2.segments = 0; m.chamfer2.smooth = smoothOn
            addModifier n m
            local bad = 0; local resolved = 0; local unresolved = 0
            local counts = for e = 1 to 12 collect 0
            for e = 1 to polyop.getNumEdges n do
            (
                local ev = polyop.getEdgeVerts n e
                local a = owner (polyop.getVert n ev[1]) corners
                local b = owner (polyop.getVert n ev[2]) corners
                local parent = 0
                if a != 0 and b != 0 and a != b do
                    for k = 1 to 12 where (edges[k][1] == a and edges[k][2] == b) or (edges[k][1] == b and edges[k][2] == a) do parent = k
                if parent == 0 then unresolved += 1 else
                (
                    resolved += 1; counts[parent] += 1
                    if (hardAt n e) != hard[parent] or abs ((polyop.getEDataValue n 1 e)-weights[parent]) > 0.00001 do bad += 1
                )
            )
            local missing = 0
            for count in counts where count == 0 do missing += 1
            if bad != 0 or missing != 0 do failed += 1
            cases += 1
            format "CASE inset=% second=% smooth=% resolved=% unresolved=% bad=% missingParents=%\n" insetOn secondOn smoothOn resolved unresolved bad missing to:log
            flush log
            delete n
        )
        format "COMPLETE cases=% failed=% scope=resolved-longitudinal-edges-only\n" cases failed to:log
    )
    catch (format "ERROR %\n" (getCurrentException()) to:log)
    close log
)