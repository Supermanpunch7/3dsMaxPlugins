(
 local ini = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\Plugin.UserSettings.ini"
 local backup = ini + ".before-stable.bak"
 if not doesFileExist backup do
     if not copyFile ini backup do throw "Plugin settings backup failed"
 if not setINISetting ini "Directories" "ChamferPro Native Stable" @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\bin\Stable\" do throw "Plugin registration failed"
 fileIn @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\usermacros\Supermanpunch-9-ChamferPro.mcr"
 #(getINISetting ini "Directories" "ChamferPro Native Stable", "MACRO_COMPILED")
)