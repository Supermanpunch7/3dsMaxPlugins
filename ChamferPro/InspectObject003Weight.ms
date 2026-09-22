(
    local source = getNodeByName "Object003"
    if source == undefined do throw "Object003 not found"
    local log = createFile @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\object003-weight-stages.log"
    local temp = undefined
    local savedSelection = selection as array
    try
    (
        for i = 1 to source.modifiers.count do
        (
            local m = source.modifiers[i]
            format "MOD % % enabled=% viewport=%\n" i m.name m.enabled m.enabledInViews to:log
            if isProperty m #chamfer1 do
                for cm in #(m.chamfer1,m.chamfer2) do
                (
                    format " CHILD enabled=%\n" cm.enabled to:log
                    for p in getPropNames cm do format "  %=%\n" p (getProperty cm p) to:log
                )
        )
        flush log
        for retained = 0 to source.modifiers.count do
        (
            temp = copy source
            temp.name = uniqueName "__ChamferPro_WeightDiagnostic_"
            while temp.modifiers.count > retained do deleteModifier temp 1
            convertToPoly temp
            local count = polyop.getNumEdges temp
            local support = polyop.getEDataChannelSupport temp 1
            local nonDefault = 0
            local low = 1e30
            local high = -1e30
            local samples = #()
            if support do for e = 1 to count do
            (
                local v = polyop.getEDataValue temp 1 e
                low = amin low v
                high = amax high v
                if abs (v-1.0) > 0.00001 do
                (
                    nonDefault += 1
                    if samples.count < 12 do append samples #(e,v)
                )
            )
            format "STAGE retainedBottom=% edges=% supported=% nonDefault=% min=% max=% samples=%\n" retained count support nonDefault low high samples to:log
            flush log
            delete temp
            temp = undefined
        )
        format "COMPLETE\n" to:log
    )
    catch (format "ERROR %\n" (getCurrentException()) to:log)
    if isValidNode temp do delete temp
    select savedSelection
    close log
    "Object003 stage diagnostics finished; source unchanged"
)