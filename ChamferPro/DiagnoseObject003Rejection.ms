(
    local n = getNodeByName "Object003"
    if n == undefined do throw "Object003 not found"
    local env = dotNetClass "System.Environment"
    local old = env.GetEnvironmentVariable "CHAMFERPRO_CORNER_LOG"
    local savedSelection = selection as array
    local temp = undefined
    local result = "OK"
    env.SetEnvironmentVariable "CHAMFERPRO_CORNER_LOG" @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\object003-rejection.log"
    try
    (
        temp = copy n
        temp.name = uniqueName "__ChamferPro_RejectionDiagnostic_"
        while temp.modifiers.count > 3 do deleteModifier temp 1
        convertToPoly temp
    )
    catch (result = getCurrentException())
    if isValidNode temp do delete temp
    if old == undefined then env.SetEnvironmentVariable "CHAMFERPRO_CORNER_LOG" "" else env.SetEnvironmentVariable "CHAMFERPRO_CORNER_LOG" old
    select savedSelection
    result
)