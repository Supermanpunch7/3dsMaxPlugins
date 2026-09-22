-- Non-destructive live regression for ChamferProCornerWeightR9.
-- Object003 is only copied. Every temporary node is deleted and selection is restored.
(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local log = createFile (d + "object003-r9-mixed-junction-auto.log")
    local savedSelection = selection as array
    local temporaryNodes = #()
    local failure = undefined

    fn r9IsPlusDirections directions =
    (
        if directions.count != 4 then return false
        local found = false
        for a = 1 to 3 do for b = a + 1 to 4 where (dot directions[a] directions[b]) <= -0.75 do
        (
            for c = 1 to 3 do for e = c + 1 to 4 where (dot directions[c] directions[e]) <= -0.75 do
            (
                if a != c and a != e and b != c and b != e and abs (dot directions[a] directions[c]) < 0.85 do
                    found = true
            )
        )
        found
    )

    fn r9HasMixedWeights weights epsilon:0.00001 =
    (
        if weights.count < 2 then return false
        local mixed = false
        for i = 2 to weights.count where abs (weights[i] - weights[1]) > epsilon do mixed = true
        mixed
    )

    fn r9CollectJunctions sourceNode logStream =
    (
        local result = #()
        local sourceVertCount = polyop.getNumVerts sourceNode
        for vertex = 1 to sourceVertCount do
        (
            local incident = polyop.getEdgesUsingVert sourceNode #{vertex}
            if incident.numberSet == 4 do
            (
                local edgeIDs = #()
                local directions = #()
                local weights = #()
                local position = polyop.getVert sourceNode vertex
                local radius = 1.0e30
                local valid = true
                for edge in incident do
                (
                    local edgeVerts = polyop.getEdgeVerts sourceNode edge
                    local other = if edgeVerts[1] == vertex then edgeVerts[2] else edgeVerts[1]
                    local delta = (polyop.getVert sourceNode other) - position
                    local edgeLength = length delta
                    if edgeLength <= 0.000001 then valid = false
                    else
                    (
                        append edgeIDs edge
                        append directions (normalize delta)
                        append weights (polyop.getEDataValue sourceNode 1 edge)
                        if edgeLength < radius do radius = edgeLength
                    )
                )
                if valid and r9IsPlusDirections directions and r9HasMixedWeights weights do
                (
                    append result #(vertex, position, radius, edgeIDs, directions, weights)
                    format "CANDIDATE vertex=% radius=% edges=% weights=%\n" vertex radius edgeIDs weights to:logStream
                )
            )
        )
        result
    )

    try
    (
        local source = getNodeByName "Object003"
        if source == undefined do throw "Object003 missing"

        local r9Class = undefined
        for c in modifier.classes where c.classID[1] == 0x71293b6b and c.classID[2] == 0x19374a92 do r9Class = c
        if r9Class == undefined do throw "ChamferProCornerWeightR9 is not loaded"

        local settingsIndex = 0
        local lowestChamferPro = 0
        for i = 1 to source.modifiers.count do
        (
            local candidate = source.modifiers[i]
            if isProperty candidate #chamfer1 do
            (
                lowestChamferPro = i
                if settingsIndex == 0 and candidate.enabled do settingsIndex = i
            )
        )
        if settingsIndex == 0 do throw "No enabled ChamferPro settings source"
        if lowestChamferPro == 0 do throw "No ChamferPro stack boundary"
        local settingsModifier = source.modifiers[settingsIndex]
        format "SETTINGS index=% name=% classID=% lowest=%\n" settingsIndex settingsModifier.name settingsModifier.classID lowestChamferPro to:log

        local inputMesh = copy source
        append temporaryNodes inputMesh
        inputMesh.name = uniqueName "__R9MixedInput_"
        for i = 1 to lowestChamferPro do deleteModifier inputMesh 1
        convertToPoly inputMesh
        if not (polyop.getEDataChannelSupport inputMesh 1) do throw "Input has no Edge Properties Weight channel"

        local junctions = r9CollectJunctions inputMesh log
        if junctions.count == 0 do throw "No automatic mixed-weight + junction candidate in current input"

        local reference = copy inputMesh
        append temporaryNodes reference
        reference.name = uniqueName "__R9MixedReference_"
        addModifier reference (copy settingsModifier)
        convertToPoly reference

        local output = copy inputMesh
        append temporaryNodes output
        output.name = uniqueName "__R9MixedOutput_"
        local r9 = r9Class()
        r9.chamfer1 = copy settingsModifier.chamfer1
        r9.chamfer2 = copy settingsModifier.chamfer2
        addModifier output r9
        convertToPoly output

        local sourceEdges = polyop.getNumEdges inputMesh
        local outputEdges = polyop.getNumEdges output
        if outputEdges <= sourceEdges do throw "R9 produced no new edges"
        if not (polyop.getEDataChannelSupport output 1) do throw "R9 output has no Edge Properties Weight channel"
        if outputEdges != polyop.getNumEdges reference or polyop.getNumVerts output != polyop.getNumVerts reference or polyop.getNumFaces output != polyop.getNumFaces reference do
            throw "Topology count changed against current ChamferPro"

        local geometryMismatch = 0
        local smoothingMismatch = 0
        for vertex = 1 to polyop.getNumVerts output where distance (polyop.getVert output vertex) (polyop.getVert reference vertex) > 0.000001 do geometryMismatch += 1
        for face = 1 to polyop.getNumFaces output where polyop.getFaceSmoothGroup output face != polyop.getFaceSmoothGroup reference face do smoothingMismatch += 1
        if geometryMismatch != 0 or smoothingMismatch != 0 do throw "Geometry or smoothing changed against current ChamferPro"

        local checked = 0
        local mismatches = 0
        local ambiguous = 0
        local checkedAxes = for j = 1 to junctions.count collect #()
        local epsilon = 0.00001

        for outputEdge = 1 to outputEdges do
        (
            local edgeVerts = polyop.getEdgeVerts output outputEdge
            local a = polyop.getVert output edgeVerts[1]
            local b = polyop.getVert output edgeVerts[2]
            local delta = b - a
            local edgeLength = length delta
            if edgeLength > epsilon do
            (
                local midpoint = (a + b) * 0.5
                local bestJunction = 0
                local bestDistance = 1.0e30
                local junctionTie = false
                for j = 1 to junctions.count do
                (
                    local center = junctions[j][2]
                    local radius = junctions[j][3]
                    local midpointDistance = distance midpoint center
                    local endpointA = distance a center
                    local endpointB = distance b center
                    if midpointDistance <= radius * 0.67 + epsilon and endpointA <= radius * 0.75 + epsilon and endpointB <= radius * 0.75 + epsilon and endpointA > epsilon and endpointB > epsilon and edgeLength < radius * 0.75 do
                    (
                        local tieDistance = radius * 0.00001 + epsilon
                        if midpointDistance + tieDistance < bestDistance then
                        (
                            bestJunction = j
                            bestDistance = midpointDistance
                            junctionTie = false
                        )
                        else if abs (midpointDistance - bestDistance) <= tieDistance do junctionTie = true
                    )
                )

                if bestJunction != 0 and not junctionTie do
                (
                    local junction = junctions[bestJunction]
                    local center = junction[2]
                    local directions = junction[5]
                    local parentWeights = junction[6]
                    local outputDirection = normalize delta
                    local bestAlignment = -1.0
                    local aligned = #()
                    for parent = 1 to directions.count do
                    (
                        local alignment = abs (dot outputDirection directions[parent])
                        if alignment >= 0.70 and alignment > bestAlignment do bestAlignment = alignment
                    )
                    if bestAlignment >= 0.70 do
                    (
                        for parent = 1 to directions.count where bestAlignment - abs (dot outputDirection directions[parent]) <= 0.002 do append aligned parent
                        local fromCenter = midpoint - center
                        local midpointDistance = length fromCenter
                        if aligned.count > 1 and midpointDistance > epsilon do
                        (
                            local midpointDirection = normalize fromCenter
                            local bestSide = -2.0
                            for parent in aligned do
                            (
                                local side = dot midpointDirection directions[parent]
                                if side > bestSide do bestSide = side
                            )
                            local sided = #()
                            for parent in aligned where bestSide - dot midpointDirection directions[parent] <= 0.002 do append sided parent
                            aligned = sided
                        )

                        if aligned.count > 0 do
                        (
                            local expected = parentWeights[aligned[1]]
                            local sameExpected = true
                            for index = 2 to aligned.count where abs (parentWeights[aligned[index]] - expected) > epsilon do sameExpected = false
                            if aligned.count == 1 or sameExpected then
                            (
                                checked += 1
                                local axis = directions[aligned[1]]
                                local knownAxis = false
                                for oldAxis in checkedAxes[bestJunction] where abs (dot axis oldAxis) >= 0.90 do knownAxis = true
                                if not knownAxis do append checkedAxes[bestJunction] axis
                                local actual = polyop.getEDataValue output 1 outputEdge
                                if abs (actual - expected) > epsilon do
                                (
                                    mismatches += 1
                                    format "MISMATCH junctionVertex=% outputEdge=% expected=% actual=% midpoint=% direction=% parentEdges=%\n" junction[1] outputEdge expected actual midpoint outputDirection junction[4] to:log
                                )
                            )
                            else ambiguous += 1
                        )
                    )
                )
            )
        )

        local verifiedJunctions = 0
        for j = 1 to checkedAxes.count where checkedAxes[j].count >= 2 do verifiedJunctions += 1
        format "SUMMARY sourceEdges=% outputEdges=% candidates=% checked=% ambiguous=% verifiedJunctions=% mismatches=% geometryMismatch=% smoothingMismatch=%\n" sourceEdges outputEdges junctions.count checked ambiguous verifiedJunctions mismatches geometryMismatch smoothingMismatch to:log
        if checked == 0 do throw "No generated junction-internal edge could be matched"
        if verifiedJunctions == 0 do throw "No + junction was checked in two parent direction families"
        if mismatches != 0 do throw "Directional parent-weight mismatch"
        format "PASS mixed-weight + junction directional inheritance\n" to:log
    )
    catch
    (
        failure = getCurrentException()
        format "ERROR %\n" failure to:log
    )

    for node in temporaryNodes where isValidNode node do delete node
    select savedSelection
    format "Temporary objects cleaned; Object003 untouched\n" to:log
    close log
    if failure != undefined do throw failure
    "R9 mixed-junction regression passed"
)
