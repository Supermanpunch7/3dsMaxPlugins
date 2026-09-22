(
 local n = getNodeByName "Object003"
 local f = createFile @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\default-settings-live.log"
 if n != undefined do for m in n.modifiers where matchPattern (classof m as string) pattern:"*CornerWeightR5*" do
 (
  for c in #(m.chamfer1,m.chamfer2) do
  (
   format "STAGE\n" to:f
   for p in getPropNames c do try(format "%=%\n" p (getProperty c p) to:f)catch()
  )
 )
 close f
 "Read only defaults inspection complete"
)