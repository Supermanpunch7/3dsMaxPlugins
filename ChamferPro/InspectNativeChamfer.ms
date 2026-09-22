(
    local path = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\InspectNativeChamfer.log"
    local f = createFile path
    try
    (
        format "VERSION=%\n" (maxVersion()) to:f
        local candidates = for c in modifier.classes where matchPattern (c as string) pattern:"*Chamfer*" collect c
        format "CANDIDATES=%\n" candidates to:f
        local nativeClass = undefined
        for c in candidates do
        (
            format "CANDIDATE=% ID=%\n" c c.classID to:f
            if c.classID[1] == 858559760 and c.classID[2] == 951800972 do nativeClass = c
        )
        if nativeClass == undefined do throw "Native Chamfer class not found"
        local m = nativeClass()
        format "CLASSID=%\n" m.classID to:f
        showProperties m to:f
        showInterfaces m to:f
        format "PASS\n" to:f
    )
    catch (format "FAIL=%\n" (getCurrentException()) to:f)
    close f
)