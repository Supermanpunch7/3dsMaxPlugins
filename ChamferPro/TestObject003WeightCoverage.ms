(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local source = getNodeByName "Object003"
    if source == undefined do throw "Object003 missing"
    local saved = selection as array
    local temp = undefined
    local log = createFile (d + "object003-weight-coverage.log")
    local failure = undefined
    try
    (
        loadDllsFromDir (d + @"bin\CornerWeightR3\") "ChamferProCornerWeightR3.dlm"
        local cls = undefined
        for c in modifier.classes where c.classID[1] == 0x71293b63 and c.classID[2] == 0x19374a84 do cls = c
        if cls == undefined do throw "R3 missing"
        local index = 0
        for i = 1 to source.modifiers.count do
        (
            format "SOURCE % % enabled=%\n" i source.modifiers[i].name source.modifiers[i].enabled to:log
            if index == 0 and isProperty source.modifiers[i] #chamfer1 do index = i
        )
        if index == 0 do throw "Reference chamfer missing"
        local reference = source.modifiers[index]
        for uniform in #(false,true) do for secondOn in #(false,true) do
        (
            temp = copy source
            temp.name = uniqueName "__ChamferPro_CoverageDiagnostic_"
            for i = 1 to index do deleteModifier temp 1
            convertToPoly temp
            if uniform do
            (
                polyop.setEDataChannelSupport temp 1 true
                polyop.setEDataValue temp 1 #{1..(polyop.getNumEdges temp)} 1.83
            )
            local m = cls()
            m.chamfer1 = copy reference.chamfer1
            m.chamfer2 = copy reference.chamfer2
            m.chamfer2.enabled = secondOn
            format "PARAM firstEnabled=% firstAmount=% firstScale=% secondEnabled=% secondAmount=% secondScale=%\n" m.chamfer1.enabled m.chamfer1.amount m.chamfer1.scale m.chamfer2.enabled m.chamfer2.amount m.chamfer2.scale to:log
            addModifier temp m
            addModifier temp (Edit_Poly())
            convertToPoly temp
            local values = #()
            local counts = #()
            local total = polyop.getNumEdges temp
            local wrong = 0
            for e = 1 to total do
            (
                local v = polyop.getEDataValue temp 1 e
                if abs (v-1.83) > 0.00001 do wrong += 1
                if uniform and abs (v-1.83) > 0.00001 do
                (
                    local ends = polyop.getEdgeVerts temp e
                    format " MISSING edge=% weight=% p1=% p2=%\n" e v (polyop.getVert temp ends[1]) (polyop.getVert temp ends[2]) to:log
                )
                local key = (floor (v*100000+0.5)) as integer
                local slot = findItem values key
                if slot == 0 then (append values key; append counts 1) else counts[slot] += 1
            )
            format "CASE uniform=% second=% edges=% differingFrom183=%\n" uniform secondOn total wrong to:log
            for i = 1 to values.count do format " WEIGHT % COUNT %\n" (values[i]/100000.0) counts[i] to:log
            flush log
            delete temp
            temp = undefined
        )
        format "COMPLETE\n" to:log
    )
    catch (failure = getCurrentException(); format "ERROR %\n" failure to:log)
    if isValidNode temp do delete temp
    select saved
    close log
    if failure != undefined do throw failure
    "Coverage test complete; temporary copies deleted"
)