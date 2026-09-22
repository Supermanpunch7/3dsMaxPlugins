-- Batch-only regression: native pair, evaluated poly data, independent box oracle.
-- Junctions and explicit normals are deliberately not claimed as verified.
(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local log = createFile (d + "native-four-trial.log")
    local result = createFile (d + "native-four-trial.result")
    format "RUNNING\n" to:result
    close result
    local failures = 0
    local cases = 0
    local node = undefined
    fn ownerCorner p corners =
    (
        local found = 0
        for k = 1 to corners.count where distance p corners[k] < 10 do
        (
            if found != 0 do throw "Ambiguous fixture corner"
            found = k
        )
        found
    )
    try
    (
        loadDllsFromDir (d + @"bin\NativeFourTrial\") "ChamferProFourTrial.dlm"; local cls = undefined
        for c in modifier.classes where c.classID[1] == 0x71293b46 and c.classID[2] == 0x19374a67 do cls = c
        if cls == undefined do throw "Stable native class missing"
        for seg in #(0,1,2,4) do for second in #(false,true) do for downstream in #(false,true) do
        (
            cases += 1
            node = box length:100 width:100 height:100
            convertToPoly node
            local corners = for v = 1 to polyop.getNumVerts node collect polyop.getVert node v
            local edges = for e = 1 to polyop.getNumEdges node collect polyop.getEdgeVerts node e
            local channels = #(1,2,4)
            local values = #()
            for j = 1 to channels.count do
            (
                polyop.setEDataChannelSupport node channels[j] true
                local row = #()
                for e = 1 to edges.count do
                (
                    local value = 0.05 + (mod (e*(2*j+1)) 17)*0.02
                    polyop.setEDataValue node channels[j] e value
                    if abs((polyop.getEDataValue node channels[j] e)-value) > 0.00001 do throw "Input roundtrip failed"
                    append row value
                )
                append values row
            )
            local m = cls()
            m.chamfer1.amount = 2
            m.chamfer1.segments = seg
            m.chamfer2.amount = if second then 0.25 else 0
            m.chamfer2.segments = 1
            if not second do m.chamfer2.enabled = false
            format "PARAMS firstAmount=% firstSegments=% secondAmount=% secondEnabled=%\n" m.chamfer1.amount m.chamfer1.segments m.chamfer2.amount m.chamfer2.enabled to:log
            addModifier node m
            if downstream do addModifier node (Edit_Poly())
            local counts = for e = 1 to edges.count collect 0
            local mismatches = #(0,0,0)
            local unresolved = 0
            for e = 1 to polyop.getNumEdges node do
            (
                local ev = polyop.getEdgeVerts node e
                local a = ownerCorner (polyop.getVert node ev[1]) corners
                local b = ownerCorner (polyop.getVert node ev[2]) corners
                local parent = 0
                if a != 0 and b != 0 and a != b do
                    for k = 1 to edges.count where
                        ((edges[k][1] == a and edges[k][2] == b) or
                         (edges[k][1] == b and edges[k][2] == a)) do parent = k
                if parent == 0 then unresolved += 1 else
                (
                    counts[parent] += 1
                    for j = 1 to channels.count do
                    (
                        if not (polyop.getEDataChannelSupport node channels[j]) then mismatches[j] += 1
                        else if abs((polyop.getEDataValue node channels[j] e)-values[j][parent]) > 0.00001 do mismatches[j] += 1
                    )
                )
            )
            local coverage = true
            for n in counts where n == 0 do coverage = false
            if not coverage or mismatches[1] != 0 or mismatches[2] != 0 or mismatches[3] != 0 do failures += 1
            format "CASE seg=% second=% downstream=% descendants=% unresolved=% mismatches(W,C,D)=% coverage=%\n" seg second downstream counts unresolved mismatches coverage to:log
            flush log
            delete node
            node = undefined
        )
    )
    catch
    (
        failures += 1
        format "ERROR %\n" (getCurrentException()) to:log
    )
    if isValidNode node do delete node
    format "SUMMARY cases=% failed=% scope=box-longitudinal-numeric; hard-state-NOT-tested\n" cases failures to:log
    close log
    result = createFile (d + "native-four-trial.result")
    format "% cases=% failed=%\n" (if cases == 16 and failures == 0 then "SCOPED_PASS" else "FAIL") cases failures to:result
    close result
)