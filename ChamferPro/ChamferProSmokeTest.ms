(
    local logPath = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\ChamferProSmokeTest.log"
    local fileClass = dotNetClass "System.IO.File"

    fn logLine text =
    (
        fileClass.AppendAllText logPath ((text as string) + "\r\n")
    )

    fileClass.WriteAllText logPath "START\r\n"

    try
    (
        resetMaxFile #noPrompt
        logLine "RESET_OK"

        local modifierClass = ChamferPro
        logLine ("CLASS_OK=" + (modifierClass as string))

        local mod = ChamferPro()
        logLine "CREATE_MODIFIER_OK"

        local propertyNames = getPropNames mod
        logLine ("PROPERTIES=" + (propertyNames as string))
        logLine ("DEFAULTS=" + (mod.chamfer1Amount as string) + "," + (mod.chamfer1Segments as string) + "," + (mod.chamfer1Tension as string) + "," + (mod.chamfer2Amount as string) + "," + (mod.chamfer2Segments as string) + "," + (mod.chamfer2Tension as string) + "," + (mod.limitEffect as string))

        local obj = box length:100.0 width:100.0 height:100.0
        convertToPoly obj
        logLine "OBJECT_OK"

        addModifier obj mod
        logLine "ADD_MODIFIER_OK"

        local evaluated1 = snapshotAsMesh obj
        logLine ("EVALUATE_DEFAULT_OK verts=" + (evaluated1.numverts as string) + " faces=" + (evaluated1.numfaces as string))
        free evaluated1

        mod.chamfer1Amount = 1.0
        mod.chamfer1Segments = 1
        mod.chamfer1Tension = 0.5
        mod.chamfer2Amount = 0.5
        mod.chamfer2Segments = 2
        mod.chamfer2Tension = 0.5
        mod.limitEffect = true
        logLine "SET_PARAMETERS_OK"

        local evaluated2 = snapshotAsMesh obj
        logLine ("EVALUATE_CHANGED_OK verts=" + (evaluated2.numverts as string) + " faces=" + (evaluated2.numfaces as string))
        free evaluated2

        logLine "PASS"
    )
    catch
    (
        logLine ("MAXSCRIPT_ERROR=" + ((getCurrentException()) as string))
    )
)