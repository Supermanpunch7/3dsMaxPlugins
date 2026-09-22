(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local f = createFile (d + "NativeGuiTest.log")
    try
    (
        format "VERSION=%\n" (maxVersion()) to:f
        loadDllsFromDir (d + @"bin\NativeTest\") "ChamferProNativeTest.dlm"
        local cls = undefined
        for c in modifier.classes where c.classID[1] == 0x71293b41 and c.classID[2] == 0x19374a62 do cls = c
        if cls == undefined do throw "Native prototype missing"
        local p = cls()
        local obj = box name:"ChamferPro_Native_UI_Test" length:100 width:100 height:100
        convertToPoly obj
        p.chamfer1.amount = 2
        p.chamfer2.amount = 0.5
        addModifier obj p
        select obj
        max modify mode
        modPanel.setCurrentObject p
        completeRedraw()
        format "PANEL_OPEN\n" to:f
        flush f
        local buttons = for w in windows.getChildrenHWND (windows.getMAXHWND()) where w[5] == "Chamfer 2" collect w[1]
        if buttons.count != 1 do throw ("Expected one Chamfer 2 button, found " + buttons.count as string)
        UIAccessor.PressButton buttons[1]
        windows.processPostedMessages()
        completeRedraw()
        format "STAGE_2_SWITCH_OK\n" to:f
        flush f
        buttons = for w in windows.getChildrenHWND (windows.getMAXHWND()) where w[5] == "Chamfer 1" collect w[1]
        if buttons.count != 1 do throw "Chamfer 1 button missing"
        UIAccessor.PressButton buttons[1]
        windows.processPostedMessages()
        completeRedraw()
        format "STAGE_1_SWITCH_OK\n" to:f
        for i = 1 to 30 do
        (
            p.chamfer1.amount = i * 0.1
            p.chamfer2.amount = i * 0.025
            completeRedraw()
            local meshResult = snapshotAsMesh obj
            free meshResult
        )
        format "PARAMETER_REDRAW_OK\n" to:f
        format "PASS_SCRIPTED_GUI_CHECKS\n" to:f
    )
    catch (format "FAIL=%\n" (getCurrentException()) to:f)
    close f
)