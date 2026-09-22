(
    local folder = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\bin\Hybrid"
    if (execute "Chmferpro") == undefined do loadDllsFromDir (folder + "\\") "Chmferpro.dlm"
    scanForNewPlugins()
    local m = execute "Chmferpro()"
    if m.chamfer1 == undefined or m.chamfer2 == undefined do throw "Missing native stages"
    #(classOf m as string, m.chamfer1 != undefined, m.chamfer2 != undefined)
)