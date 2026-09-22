(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local f = createFile (d + "TraceInheritance.log")
    try
    (
        loadDllsFromDir (d + @"bin\InheritanceTest\") "ChamferProNativeTest.dlm"
        local cls = undefined
        for c in modifier.classes where c.classID[1] == 0x71293b41 and c.classID[2] == 0x19374a62 do cls = c
        if cls == undefined do throw "Class missing"
        local n = box length:100 width:100 height:100
        convertToPoly n
        local p = cls()
        p.chamfer1.amount = 2
        p.chamfer1.segments = 3
        p.chamfer2.enabled = false
        addModifier n p
        local m = snapshotAsMesh n
        format "NAME=% V=% F=%\n" (classof p) m.numverts m.numfaces to:f
        free m
        delete n
        format "PASS\n" to:f
    )
    catch (format "FAIL=%\n" (getCurrentException()) to:f)
    close f
)