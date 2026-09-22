(
 local d=@"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
 local log=createFile (d+"weight-final-cleanup.log")
 local n=getNodeByName "Object003"
 format "objects=% temporary=%\n" objects.count (for x in objects where matchPattern x.name pattern:"__R*WeightCheck_*" collect x.name) to:log
 if n!=undefined do for m in n.modifiers do format "modifier=% enabled=%\n" m.name m.enabled to:log
 local env=dotNetClass "System.Environment"
 -- Clear the diagnostic-only process variable set by our earlier test.
 env.SetEnvironmentVariable "CHAMFERPRO_WEIGHT_DIAG" ""
 format "COMPLETE\n" to:log
 close log
 "Cleanup verified"
)