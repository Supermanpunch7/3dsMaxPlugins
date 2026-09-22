(
 local nodes=#()
 local saved=selection as array
 local log=stringStream ""
 local failure=undefined
 try (
  loadDllsFromDir @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\bin\Stable\" "ChamferProNativeTest.dlm"
  local cls=undefined
  for c in modifier.classes where c.classID[1]==0x71293b41 and c.classID[2]==0x19374a62 do cls=c
  if cls==undefined do throw "Stable class missing"
  local a=box length:100 width:100 height:100
  append nodes a
  convertToPoly a
  local b=copy a
  append nodes b
  local p=cls()
  format "NAME=% CLASS=%\n" p.name cls to:log
  if p.name!="ChamferPro" do throw "Display name mismatch"
  addModifier a p
  local first=copy p.chamfer1
  local second=copy p.chamfer2
  addModifier b first
  addModifier b second
  for k=0 to 8 do (
   p.chamfer1.amount=first.amount=k*0.1
   p.chamfer2.amount=second.amount=k*0.05
   p.chamfer2.segments=second.segments=1+mod k 3
   local x=snapshotAsMesh a
   local y=snapshotAsMesh b
   if x.numverts!=y.numverts or x.numfaces!=y.numfaces do throw "Topology mismatch"
   for i=1 to x.numverts where distance (getVert x i) (getVert y i)>0.00001 do throw "Position mismatch"
   for i=1 to x.numfaces where (getFace x i)!=(getFace y i) do throw "Face mismatch"
   free x
   free y
   format "MATCH %\n" k to:log
  )
  local cloned=copy p
  if cloned.chamfer1==p.chamfer1 or cloned.chamfer2==p.chamfer2 do throw "Shared clone stages"
  format "CLONE_OK PASS\n" to:log
 ) catch (failure=getCurrentException())
 for n in nodes where isValidNode n do delete n
 if saved.count>0 then select saved else clearSelection()
 format "failure=% remaining=%\n" failure objects.count to:log
 log as string
)