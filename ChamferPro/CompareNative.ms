(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local f = createFile (d + "CompareNative.log")
    try
    (
        format "VERSION=%\n" (maxVersion()) to:f
        loadDllsFromDir (d + @"bin\NativeTest\") "ChamferProNativeTest.dlm"
        local cls = undefined
        for c in modifier.classes where c.classID[1] == 0x71293b41 and c.classID[2] == 0x19374a62 do cls = c
        if cls == undefined do throw "Prototype class missing"
        local p = cls()
        format "STAGES=% %\n" p.chamfer1 p.chamfer2 to:f
        local a = box length:100 width:100 height:100
        convertToPoly a
        local b = copy a
        addModifier a p
        local first = copy p.chamfer1
        local second = copy p.chamfer2
        addModifier b first
        addModifier b second
        for k = 0 to 20 do
        (
            p.chamfer1.amount = first.amount = k * 0.1
            p.chamfer2.amount = second.amount = k * 0.05
            p.chamfer2.segments = second.segments = 1 + mod k 3
            local x = snapshotAsMesh a
            local y = snapshotAsMesh b
            if x.numverts != y.numverts or x.numfaces != y.numfaces do throw "Topology count mismatch"
            for i = 1 to x.numverts where distance (getVert x i) (getVert y i) > 0.00001 do throw "Vertex mismatch"
            for i = 1 to x.numfaces where (getFace x i) != (getFace y i) do throw "Face mismatch"
            format "MATCH % verts=% faces=%\n" k x.numverts x.numfaces to:f
            flush f
            free x
            free y
        )
        select a
        max modify mode
        modPanel.setCurrentObject p
        completeRedraw()
        format "UI_OPEN_OK\n" to:f
        local cloned = copy a
        cloned.name = "NativePairClone"
        if cloned.modifiers[1].chamfer1 == p.chamfer1 do throw "Clone shares first stage"
        format "CLONE_OK\n" to:f
        saveMaxFile (d + "NativePairRoundtrip.max") quiet:true
        loadMaxFile (d + "NativePairRoundtrip.max") quiet:true
        local restored = getNodeByName "NativePairClone"
        if restored == undefined do throw "Saved node missing"
        local restoredMesh = snapshotAsMesh restored
        format "RELOAD_OK verts=% faces=%\n" restoredMesh.numverts restoredMesh.numfaces to:f
        free restoredMesh
        format "PASS\n" to:f
    )
    catch (format "FAIL=%\n" (getCurrentException()) to:f)
    close f
)