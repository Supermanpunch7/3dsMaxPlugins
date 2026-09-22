-- Run only in an isolated 3dsmaxbatch process; never in the user's scene.
(
    local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
    local env = dotNetClass "System.Environment"
    local io = dotNetClass "System.IO.File"
    local variant = env.GetEnvironmentVariable "CHAMFERPRO_SEAM_VARIANT"
    if variant != "NativeHardTrial" and variant != "HardSeamValidation" do throw "Invalid test variant"
    local suite = io.ReadAllText (d + "VerifyHardSmooth.ms")
    suite = substituteString suite @"bin\NativeHardTrial\" ("bin\\" + variant + "\\")
    suite = substituteString suite "hard-smooth-verification.log" ("seam-" + variant + "-matrix.log")
    execute suite
    local b = box name:"Box001" length:100 width:100 height:100
    convertToPoly b
    for f = 1 to polyop.getNumFaces b do polyop.setFaceSmoothGroup b #{f} 0
    local junction = io.ReadAllText (d + "TestJunctionRead.ms")
    junction = substituteString junction @"bin\NativeHardTrial\" ("bin\\" + variant + "\\")
    junction = substituteString junction "junction-baseline.log" ("seam-" + variant + "-junction.log")
    junction = substituteString junction "#(0,1,2)" "#(0,1)"
    execute junction
    delete b
)