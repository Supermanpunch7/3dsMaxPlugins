(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local log = createFile (d + "junction-baseline.log")
    local original = getNodeByName "Box001"
    local n = undefined
    local savedSelection = selection as array
    fn isHard n e =
    (
        local fs = (polyop.getFacesUsingEdge n #{e}) as array
        fs.count != 2 or (bit.and (polyop.getFaceSmoothGroup n fs[1]) (polyop.getFaceSmoothGroup n fs[2])) == 0
    )
    try
    (
        if original == undefined do throw "Box001 missing"
        loadDllsFromDir (d + @"bin\NativeHardTrial\") "ChamferProHardTrial.dlm"
        local hardClass = undefined
        for c in modifier.classes where c.classID[1] == 0x71293b45 and c.classID[2] == 0x19374a66 do hardClass = c
        if hardClass == undefined do throw "HardTrial class missing"
        for kind in #(0,1,2) do for smoothOn in #(true,false) do
        (
            n = copy original
            while n.modifiers.count > 0 do deleteModifier n 1
            convertToPoly n
            local corners = for v = 1 to polyop.getNumVerts n collect polyop.getVert n v
            local radius = 1e30
            for a = 1 to corners.count do for b = a+1 to corners.count do radius = amin radius (distance corners[a] corners[b] * 0.2)
            local m = if kind == 0 then Chamfer() else if kind == 1 then hardClass() else copy original.modifiers[original.modifiers.count]
            local stage = if kind == 0 then m else m.chamfer1
            if kind != 0 do m.chamfer2.enabled = false
            stage.amount = radius * 0.1
            stage.segments = 2
            stage.smooth = smoothOn
            stage.addinset = true
            stage.insetamount = radius * 0.01
            stage.insetoffset = -radius * 0.01
            stage.insetsegments = 0
            addModifier n m
            local owner = for v = 1 to polyop.getNumVerts n collect
            (
                local result = 0
                for a = 1 to corners.count where distance (polyop.getVert n v) corners[a] < radius do result = a
                result
            )
            local junctionHard = 0; local junctionSoft = 0; local longitudinalHard = 0; local longitudinalSoft = 0; local unknown = 0
            for e = 1 to polyop.getNumEdges n do
            (
                local vs = polyop.getEdgeVerts n e
                local a = owner[vs[1]]; local b = owner[vs[2]]
                if a == 0 or b == 0 then unknown += 1
                else if a == b then (if isHard n e then junctionHard += 1 else junctionSoft += 1)
                else (if isHard n e then longitudinalHard += 1 else longitudinalSoft += 1)
            )
            format "kind=% smooth=% verts=% junctionHard=% junctionSoft=% longitudinalHard=% longitudinalSoft=% unknown=%\n" kind smoothOn (polyop.getNumVerts n) junctionHard junctionSoft longitudinalHard longitudinalSoft unknown to:log
            flush log
            delete n
            n = undefined
        )
        format "COMPLETE\n" to:log
    )
    catch (format "ERROR %\n" (getCurrentException()) to:log)
    if isValidNode n do delete n
    select savedSelection
    close log
    "junction-baseline.log"
)