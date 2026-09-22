(
    (dotNetClass "System.Environment").SetEnvironmentVariable "CHAMFERPRO_CORNER_LOG" ""
    local n = getNodeByName "Object003"
    if n != undefined do select n
    for obj in objects where matchPattern obj.name pattern:"__ChamferPro_RejectionDiagnostic_*" collect obj.name
)