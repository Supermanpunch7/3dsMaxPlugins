-- Isolated batch only. Unique class prevents deferred-DLL collisions.
(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local io = dotNetClass "System.IO.File"
    local env = dotNetClass "System.Environment"
    env.SetEnvironmentVariable "CHAMFERPRO_HARD_LOG" (d + "seam-isolated-status.log")
    loadDllsFromDir (d + @"bin\HardSeamIsolated\") "ChamferProSeamIsolated.dlm"
    local cls = undefined
    for c in modifier.classes where c.classID[1] == 0x71293b59 and c.classID[2] == 0x19374a79 do cls = c
    if cls == undefined do throw "Isolated class failed to load"
    local proof = createFile (d + "seam-isolated-loaded.log")
    format "class=% id=%\n" cls cls.classID to:proof
    local proc = (dotNetClass "System.Diagnostics.Process").GetCurrentProcess()
    for i = 0 to proc.Modules.Count-1 do
    (
        local m = proc.Modules.Item[i]
        if matchPattern m.ModuleName pattern:"*SeamIsolated*" do format "%\n" m.FileName to:proof
    )
    close proof
    fn rewriteSuite text =
    (
        text = substituteString text @"bin\NativeHardTrial\" @"bin\HardSeamIsolated\"
        text = substituteString text "ChamferProHardTrial.dlm" "ChamferProSeamIsolated.dlm"
        text = substituteString text "0x71293b45" "0x71293b59"
        text = substituteString text "0x19374a66" "0x19374a79"
        text
    )
    local suite = rewriteSuite (io.ReadAllText (d + "VerifyHardSmooth.ms"))
    suite = substituteString suite "hard-smooth-verification.log" "seam-isolated-matrix.log"
    execute suite
    local b = box name:"Box001" length:100 width:100 height:100
    convertToPoly b
    for f = 1 to polyop.getNumFaces b do polyop.setFaceSmoothGroup b #{f} 0
    local junction = rewriteSuite (io.ReadAllText (d + "TestJunctionRead.ms"))
    junction = substituteString junction "junction-baseline.log" "seam-isolated-junction.log"
    junction = substituteString junction "#(0,1,2)" "#(0,1)"
    execute junction
    delete b
)