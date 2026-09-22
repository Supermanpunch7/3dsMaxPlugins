-- Separate batch process only. Compare stage-one geometry, not just output EData.
(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local log = createFile (d + "weight-geometry.log")
    fn points n = (for v = 1 to polyop.getNumVerts n collect polyop.getVert n v)
    fn difference a b =
    (
        if a.count != b.count then 1e30 else
        (
            local delta = 0.0
            for i = 1 to a.count do delta = amax delta (distance a[i] b[i])
            delta
        )
    )
    fn setup c limitOn =
    (
        c.amountType = 3; c.scale = 0.1; c.depthType = 1
        c.miteringType = 1; c.segments = 1
        c.selectionoption = 5; c.limiteffect = limitOn
    )
    try
    (
        loadDllsFromDir (d + @"bin\CornerWeightR2\") "ChamferProCornerWeightR2.dlm"
        local cls = undefined
        for c in modifier.classes where c.classID[1] == 0x71293b62 and c.classID[2] == 0x19374a83 do cls = c
        if cls == undefined do throw "R2 missing"
        local failed = 0
        for limitOn in #(false,true) do
        (
            local low = undefined
            for weight in #(1.0,18.834) do
            (
                local n = box length:100 width:100 height:100
                convertToPoly n
                polyop.setEDataChannelSupport n 1 true
                polyop.setEDataValue n 1 #{1..12} weight
                local r = copy n
                local c = Chamfer()
                setup c limitOn
                addModifier n c
                local m = cls()
                setup m.chamfer1 limitOn
                m.chamfer2.enabled = false
                addModifier r m
                local a = points n
                local b = points r
                local delta = difference a b
                if delta > 0.00001 do failed += 1
                format "CASE limit=% weight=% nativeVerts=% r2Verts=% nativeVsR2=%\n" limitOn weight a.count b.count delta to:log
                if low == undefined then low = b else
                (
                    local response = difference low b
                    format "WEIGHT_RESPONSE limit=% displacement=%\n" limitOn response to:log
                    if response < 0.00001 do failed += 1
                )
                delete n; delete r
                flush log
            )
        )
        format "COMPLETE failed=% scope=stage1-box-by-weight-no-opensubdiv\n" failed to:log
    )
    catch (format "ERROR %\n" (getCurrentException()) to:log)
    close log
)