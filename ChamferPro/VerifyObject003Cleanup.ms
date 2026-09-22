(
    local n = getNodeByName "Object003"
    local text = stringStream ""
    format "temporaryNodes=%\n" (for o in objects where matchPattern o.name pattern:"__ChamferPro_R3Diagnostic_*" collect o.name) to:text
    if n != undefined do for m in n.modifiers do format "% enabled=%\n" m.name m.enabled to:text
    text as string
)