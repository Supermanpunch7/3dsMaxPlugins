(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local log = createFile (d + "native-probe-test.log")
    local env = dotNetClass "System.Environment"
    local a = undefined
    local b = undefined
    local cases = 0
    try
    (
        loadDllsFromDir (d + @"bin\NativeProbe\") "ChamferProNativeProbe.dlm"
        local cls = undefined
        for c in modifier.classes where c.classID[1] == 0x71293b44 and c.classID[2] == 0x19374a65 do cls = c
        if cls == undefined do throw "Probe missing"
        for seg in #(0,1,2,4) do
        (
            local trace = d + "native-probe-seg" + seg as string + ".log"
            local f = createFile trace
            close f
            env.SetEnvironmentVariable "CHAMFERPRO_PROBE_LOG" trace
            a = box length:100 width:100 height:100
            convertToPoly a
            for ch in #(1,2,4) do
            (
                polyop.setEDataChannelSupport a ch true
                for e = 1 to polyop.getNumEdges a do polyop.setEDataValue a ch e (0.05+e*0.02)
            )
            b = copy a
            local m = cls()
            m.chamfer1.amount = 2
            m.chamfer1.segments = seg
            m.chamfer2.enabled = false
            addModifier a m
            addModifier b (copy m.chamfer1)
            local x = snapshotAsMesh a
            local y = snapshotAsMesh b
            if x.numverts != y.numverts or x.numfaces != y.numfaces do throw "Probe changes topology"
            for v = 1 to x.numverts where distance (getVert x v) (getVert y v) > 0.00001 do throw "Probe changes positions"
            for f = 1 to x.numfaces where getFace x f != getFace y f do throw "Probe changes faces"
            format "GEOMETRY_MATCH seg=% verts=% faces=%\n" seg x.numverts x.numfaces to:log
            free x
            free y
            delete a
            delete b
            a = b = undefined
            cases += 1
            flush log
        )
        format "PASS cases=% scope=probe-geometry-only\n" cases to:log
    )
    catch (format "FAIL cases=% error=%\n" cases (getCurrentException()) to:log)
    env.SetEnvironmentVariable "CHAMFERPRO_PROBE_LOG" ""
    if isValidNode a do delete a
    if isValidNode b do delete b
    close log
)