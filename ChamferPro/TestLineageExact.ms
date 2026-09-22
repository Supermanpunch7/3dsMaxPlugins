(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local log = createFile (d + "lineage-exact.log")
    local marker = createFile (d + "lineage-exact.result")
    format "RUNNING\n" to:marker
    close marker
    local node = undefined
    local failures = 0
    local cases = 0
    fn cornerOwner p corners =
    (
        local owner = 0
        for i = 1 to corners.count where distance p corners[i] < 10.0 do
        (
            if owner != 0 do throw "Ambiguous fixture corner"
            owner = i
        )
        owner
    )
    try
    (
        loadDllsFromDir (d + @"bin\LineageLab\") "ChamferproLineageLab.dlm"
        local cls = undefined
        for c in modifier.classes where c.classID[1] == 0x71293b42 and c.classID[2] == 0x19374a63 do cls = c
        if cls == undefined do throw "Lab class missing"
        for seg in #(1,2,4,8) do for seed in #(0,1,2) do for soft in #(false,true) do
        (
            cases += 1
            node = box length:100 width:100 height:100
            convertToPoly node
            polyop.setFaceSmoothGroup node #{1..6} (if soft then 1 else 0)
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
                    local value = 0.05 + (mod (e*(2*j+1)+seed*7) 17)*0.02
                    polyop.setEDataValue node channels[j] e value
                    local actual = polyop.getEDataValue node channels[j] e
                    if abs(actual-value) > 0.00001 do throw "Input value not representable"
                    append row actual
                )
                append values row
            )
            local m = cls()
            m.chamfer1Amount = 2
            m.chamfer1Segments = seg
            m.chamfer2Amount = 0
            addModifier node m
            local counts = for e = 1 to edges.count collect 0
            local checked = 0
            local unresolved = 0
            local mismatches = 0
            for e = 1 to polyop.getNumEdges node do
            (
                local ev = polyop.getEdgeVerts node e
                local a = cornerOwner (polyop.getVert node ev[1]) corners
                local b = cornerOwner (polyop.getVert node ev[2]) corners
                local owner = 0
                if a != 0 and b != 0 and a != b do
                    for k = 1 to edges.count where
                        ((edges[k][1] == a and edges[k][2] == b) or
                         (edges[k][1] == b and edges[k][2] == a)) do owner = k
                if owner == 0 then unresolved += 1
                else
                (
                    checked += 1
                    counts[owner] += 1
                    for j = 1 to channels.count do
                    (
                        local actual = polyop.getEDataValue node channels[j] e
                        if abs(actual-values[j][owner]) > 0.00001 do
                        (
                            mismatches += 1
                            format "VALUE_FAIL seg=% seed=% soft=% edge=% parent=% channel=% expected=% actual=%\n" seg seed soft e owner channels[j] values[j][owner] actual to:log
                        )
                    )
                )
            )
            local coverageOK = (polyop.getNumVerts node > 8)
            for count in counts where count != seg+1 do coverageOK = false
            if not coverageOK or mismatches != 0 do failures += 1
            format "CASE seg=% seed=% soft=% checked=% unresolved=% mismatches=% coverage=%\n" seg seed soft checked unresolved mismatches coverageOK to:log
            delete node
            node = undefined
            flush log
        )
    )
    catch
    (
        failures += 1
        format "ERROR %\n" (getCurrentException()) to:log
    )
    if isValidNode node do delete node
    format "SUMMARY cases=% failed=% scope=single-stage-box-longitudinal-numeric-only\n" cases failures to:log
    close log
    marker = createFile (d + "lineage-exact.result")
    format "% cases=% failed=%\n" (if failures == 0 and cases == 24 then "SCOPED_PASS" else "FAIL") cases failures to:marker
    close marker
)