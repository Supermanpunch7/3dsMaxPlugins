(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local source = getNodeByName "Object003"
    if source == undefined do throw "Object003 missing"
    local log = createFile (d + "object003-r3-test.log")
    local saved = selection as array
    local temp = undefined
    try
    (
        loadDllsFromDir (d + @"bin\CornerWeightR3\") "ChamferProCornerWeightR3.dlm"
        local cls = undefined
        for c in modifier.classes where c.classID[1] == 0x71293b63 and c.classID[2] == 0x19374a84 do cls = c
        if cls == undefined do throw "R3 class missing"
        local index = 0
        for i = 1 to source.modifiers.count do
        (
            local cid = source.modifiers[i].classID
            format "SOURCE % % id=%\n" i source.modifiers[i].name cid to:log
            if cid[1] == 0x71293b62 and cid[2] == 0x19374a83 do index = i
        )
        if index == 0 do for i = 1 to source.modifiers.count do
        (
            local cid = source.modifiers[i].classID
            if cid[1] == 0x71293b47 and cid[2] == 0x19374a68 do index = i
        )
        if index == 0 do throw "Reference chamfer not found"
        loadDllsFromDir (d + @"bin\CornerWeightR2\") "ChamferProCornerWeightR2.dlm"
        local r2Class = undefined
        for c in modifier.classes where c.classID[1] == 0x71293b62 and c.classID[2] == 0x19374a83 do r2Class = c
        if r2Class == undefined do throw "R2 class missing"
        for useR3 in #(false,true) do for secondOn in #(false,true) do
        (
            temp = copy source
            temp.name = uniqueName "__ChamferPro_R3Diagnostic_"
            for i = 1 to index-1 do deleteModifier temp 1
            local old = temp.modifiers[1]
            local m = old
            if true do
            (
                m = if useR3 then cls() else r2Class()
                m.chamfer1 = copy old.chamfer1
                m.chamfer2 = copy old.chamfer2
                deleteModifier temp 1
                addModifier temp m
            )
            m.chamfer2.enabled = secondOn
            addModifier temp (Edit_Poly())
            convertToPoly temp
            local count = polyop.getNumEdges temp
            local nonDefault = 0
            local low = 1e30
            local high = -1e30
            for e = 1 to count do
            (
                local v = polyop.getEDataValue temp 1 e
                low = amin low v
                high = amax high v
                if abs (v-1) > 0.00001 do nonDefault += 1
            )
            format "R3=% second=% edges=% nonDefault=% min=% max=%\n" useR3 secondOn count nonDefault low high to:log
            flush log
            delete temp
            temp = undefined
        )
        format "COMPLETE\n" to:log
    )
    catch (format "ERROR %\n" (getCurrentException()) to:log)
    if isValidNode temp do delete temp
    select saved
    close log
    "R3 live comparison finished"
)