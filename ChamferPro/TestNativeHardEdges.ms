-- Separate batch scene only. Smoothing-group boundaries, not explicit normals.
(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local log = createFile (d + "native-hard-edges.log")
    local node = undefined
    fn hardAt n e =
    (
        local faces = (polyop.getFacesUsingEdge n #{e}) as array
        if faces.count != 2 then true else
            (bit.and (polyop.getFaceSmoothGroup n faces[1]) (polyop.getFaceSmoothGroup n faces[2])) == 0
    )
    fn ownerCorner p corners =
    (
        local found = 0
        for k = 1 to corners.count where distance p corners[k] < 10 do
        (
            if found != 0 do throw "Ambiguous corner"
            found = k
        )
        found
    )
    local cases = 0
    try
    (
        local cls = undefined
        for c in modifier.classes where c.classID[1] == 0x71293b41 and c.classID[2] == 0x19374a62 do cls = c
        if cls == undefined do throw "Stable class missing"
        for seg in #(0,1,2,4) do for mode in #(0,1,2) do for input in #(0,1,2) do
        (
            node = box length:100 width:100 height:100
            convertToPoly node
            for f = 1 to 6 do
            (
                local group = if input == 0 then 0 else if input == 1 then 1 else if f <= 3 then 1 else 2
                polyop.setFaceSmoothGroup node #{f} group
            )
            local corners = for v = 1 to polyop.getNumVerts node collect polyop.getVert node v
            local edges = for e = 1 to polyop.getNumEdges node collect polyop.getEdgeVerts node e
            local expected = for e = 1 to edges.count collect hardAt node e
            local m = cls()
            m.chamfer1.amount = 2
            m.chamfer1.segments = seg
            m.chamfer2.enabled = false
            if mode == 1 do m.chamfer1.smooth = false
            if mode == 2 do (m.chamfer1.smooth = true; m.chamfer1.smoothtype = 1; m.chamfer1.smoothtoadjacent = false)
            addModifier node m
            local counts = for e = 1 to edges.count collect 0
            local mismatch = 0
            local unresolved = 0
            local hardCount = 0
            local before = for e = 1 to polyop.getNumEdges node collect hardAt node e
            for e = 1 to before.count do
            (
                if before[e] do hardCount += 1
                local ev = polyop.getEdgeVerts node e
                local a = ownerCorner (polyop.getVert node ev[1]) corners
                local b = ownerCorner (polyop.getVert node ev[2]) corners
                local parent = 0
                if a != 0 and b != 0 and a != b do
                    for k = 1 to edges.count where ((edges[k][1] == a and edges[k][2] == b) or (edges[k][1] == b and edges[k][2] == a)) do parent = k
                if parent == 0 then unresolved += 1 else
                (
                    counts[parent] += 1
                    if before[e] != expected[parent] do mismatch += 1
                )
            )
            -- Check exact topology identity before comparing downstream indices.
            local topology = for e = 1 to before.count collect polyop.getEdgeVerts node e
            addModifier node (Edit_Poly())
            if polyop.getNumEdges node != before.count do throw "Downstream edge count changed"
            local downstreamMismatch = 0
            for e = 1 to before.count do
            (
                local ev = polyop.getEdgeVerts node e
                if ev[1] != topology[e][1] or ev[2] != topology[e][2] do throw "Downstream topology changed"
                if hardAt node e != before[e] do downstreamMismatch += 1
            )
            local coverage = true
            for n in counts where n == 0 do coverage = false
            format "CASE seg=% mode=% input=% hard=% parentMismatch=% unresolved=% coverage=% downstreamMismatch=%\n" seg mode input hardCount mismatch unresolved coverage downstreamMismatch to:log
            flush log
            cases += 1
            delete node
            node = undefined
        )
        format "COMPLETE cases=% (not a full inheritance pass)\n" cases to:log
    )
    catch (format "ERROR cases=% %\n" cases (getCurrentException()) to:log)
    if isValidNode node do delete node
    close log
)