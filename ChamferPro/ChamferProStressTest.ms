(
    local logPath = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\ChamferProStressTest.log"
    local fileClass = dotNetClass "System.IO.File"

    fn logLine text =
    (
        fileClass.AppendAllText logPath ((text as string) + "\r\n")
    )

    fn forceEvaluation obj iteration =
    (
        local evaluated = snapshotAsMesh obj
        if evaluated == undefined then throw ("snapshotAsMesh failed at iteration " + (iteration as string))
        if evaluated.numverts < 1 or evaluated.numfaces < 1 then throw ("empty mesh at iteration " + (iteration as string))
        local result = #(evaluated.numverts, evaluated.numfaces)
        free evaluated
        result
    )

    fileClass.WriteAllText logPath "START\r\n"

    try
    (
        resetMaxFile #noPrompt
        local obj = box length:100.0 width:100.0 height:100.0
        convertToPoly obj
        local mod = ChamferPro()
        addModifier obj mod

        local amounts = #(0.0, 0.0000001, 0.00001, 0.001, 0.01, 0.1, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 40.0, 20.0, 10.0, 5.0, 2.0, 1.0, 0.5, 0.1, 0.01, 0.001, 0.0)
        local tensions = #(0.0, 0.25, 0.5, 0.75, 1.0)
        local iteration = 0

        for segmentCount = 1 to 3 do
        (
            mod.chamfer1Segments = segmentCount
            mod.chamfer2Segments = segmentCount

            for tensionValue in tensions do
            (
                mod.chamfer1Tension = tensionValue
                mod.chamfer2Tension = 1.0 - tensionValue

                for amountValue in amounts do
                (
                    iteration += 1
                    mod.chamfer1Amount = amountValue
                    mod.chamfer2Amount = amountValue * 0.5
                    local counts = forceEvaluation obj iteration

                    if mod.chamfer1Amount != amountValue then throw ("parameter readback mismatch at iteration " + (iteration as string))
                    logLine ("OK " + (iteration as string) + " amount=" + (amountValue as string) + " segments=" + (segmentCount as string) + " tension=" + (tensionValue as string) + " verts=" + (counts[1] as string) + " faces=" + (counts[2] as string))
                )
            )
        )

        logLine ("PASS iterations=" + (iteration as string))
    )
    catch
    (
        logLine ("MAXSCRIPT_ERROR=" + ((getCurrentException()) as string))
    )
)