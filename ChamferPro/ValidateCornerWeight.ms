-- Run only in a separate 3dsmaxbatch process; never changes the interactive scene.
(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local log = createFile (d + "corner-weight-r2-validation.log")
    fn hardAt n e =
    (
        local f = (polyop.getFacesUsingEdge n #{e}) as array
        f.count != 2 or (bit.and (polyop.getFaceSmoothGroup n f[1]) (polyop.getFaceSmoothGroup n f[2])) == 0
    )
    local cases = 0; local failed = 0
    try
    (
        loadDllsFromDir (d + @"bin\CornerWeightR2\") "ChamferProCornerWeightR2.dlm"
        local cls = undefined
        for c in modifier.classes where c.classID[1] == 0x71293b62 and c.classID[2] == 0x19374a83 do cls = c
        if cls == undefined do throw "Weight trial class missing"
        for insetOn in #(false,true) do for secondOn in #(false,true) do for smoothOn in #(false,true) do
        (
            local n = box length:100 width:100 height:100
            convertToPoly n
            polyop.setFaceSmoothGroup n #{1..6} 0
            polyop.setEDataChannelSupport n 1 true
            polyop.setEDataValue n 1 #{1..12} 1.83
            local m = cls()
            m.chamfer1.amount = 0.2
            m.chamfer1.segments = 1
            m.chamfer1.smooth = smoothOn
            m.chamfer1.addinset = insetOn
            m.chamfer1.insetamount = 0.2
            m.chamfer1.insetsegments = 0
            m.chamfer1.insetoffset = -0.4
            m.chamfer2.enabled = secondOn
            m.chamfer2.amount = 0.05
            m.chamfer2.segments = 0
            m.chamfer2.smooth = smoothOn
            addModifier n m
            local weightBad = 0; local hardBad = 0
            local count = polyop.getNumEdges n
            for e = 1 to count do
            (
                if abs ((polyop.getEDataValue n 1 e)-1.83) > 0.00001 do weightBad += 1
                if not (hardAt n e) do hardBad += 1
            )
            addModifier n (Edit_Poly())
            local downstreamBad = 0
            for e = 1 to polyop.getNumEdges n do
                if abs ((polyop.getEDataValue n 1 e)-1.83) > 0.00001 or not (hardAt n e) do downstreamBad += 1
            cases += 1
            if weightBad != 0 or hardBad != 0 or downstreamBad != 0 or count <= 12 do failed += 1
            format "CASE inset=% second=% smooth=% edges=% weightBad=% hardBad=% downstreamBad=%\n" insetOn secondOn smoothOn count weightBad hardBad downstreamBad to:log
            flush log
            delete n
        )
        format "COMPLETE cases=% failed=% scope=uniform-weight-all-hard-box\n" cases failed to:log
    )
    catch (format "ERROR %\n" (getCurrentException()) to:log)
    close log
)