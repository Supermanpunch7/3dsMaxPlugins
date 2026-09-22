(
 local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
 local log = createFile (d + "object003-r5-test.log")
 local saved = selection as array
 local temp = undefined
 local reference = undefined
 try
 (
  local source = getNodeByName "Object003"
  if source == undefined do throw "Object003 missing"
  local idx = 0
  local lowestTrial = 0
  for i=1 to source.modifiers.count do
  (
   local cid=source.modifiers[i].classID
   format "SOURCE % % id=%\n" i source.modifiers[i].name cid to:log
   if cid[1]==0x71293b63 and cid[2]==0x19374a84 do idx=i
    if matchPattern ((classOf source.modifiers[i]) as string) pattern:"ChamferPro*" do lowestTrial=i
  )
  if idx == 0 do throw "R3 modifier missing"
  loadDllsFromDir (d + @"bin\CornerWeightR5\") "ChamferProCornerWeightR5.dlm"
  local cls = undefined
  for c in modifier.classes where c.classID[1]==0x71293b66 and c.classID[2]==0x19374a87 do cls=c
  if cls == undefined do throw "R5 class missing"
  local r3cls=undefined
  for c in modifier.classes where c.classID[1]==0x71293b63 and c.classID[2]==0x19374a84 do r3cls=c
  if r3cls == undefined do throw "R3 class missing"
  format "ISOLATED input below modifier %; settings from R3 index %\n" lowestTrial idx to:log
  for version in #(3,5) do
  (
   temp=copy source
   temp.name=uniqueName "__R5WeightCheck_"
   if true do
   (
    local old=source.modifiers[idx]
    local m=if version == 5 then cls() else r3cls()
    m.chamfer1=copy old.chamfer1
    m.chamfer2=copy old.chamfer2
    for i=1 to lowestTrial do deleteModifier temp 1
    addModifier temp m
   )
   addModifier temp (Edit_Poly())
   convertToPoly temp
   local n=polyop.getNumEdges temp
   local nonDefault=0
   for e=1 to n where abs ((polyop.getEDataValue temp 1 e)-1)>0.00001 do nonDefault+=1
   format "R% edges=% nonDefault=%\n" version n nonDefault to:log
   if version == 3 then (reference=temp; temp=undefined) else
   (
    if n != polyop.getNumEdges reference do throw "Topology count changed"
    local changed=0
    for e=1 to n do
    (
     local a=polyop.getEDataValue reference 1 e
     local b=polyop.getEDataValue temp 1 e
     if abs(a-b)>0.00001 do (changed+=1; format "CHANGE edge=% R3=% R5=%\n" e a b to:log)
    )
    local geometryMismatch=0
    local smoothingMismatch=0
    if polyop.getNumVerts reference != polyop.getNumVerts temp do throw "Vertex count changed"
    for v=1 to polyop.getNumVerts temp where distance (polyop.getVert temp v) (polyop.getVert reference v)>0.000001 do geometryMismatch+=1
    for f=1 to polyop.getNumFaces temp where polyop.getFaceSmoothGroup temp f != polyop.getFaceSmoothGroup reference f do smoothingMismatch+=1
    format "changed=% geometryMismatch=% smoothingMismatch=%\n" changed geometryMismatch smoothingMismatch to:log
    if n>=2666 do format "target2666 R3=% R5=%\n" (polyop.getEDataValue reference 1 2666) (polyop.getEDataValue temp 1 2666) to:log
   )
  )
  format "COMPLETE\n" to:log
 ) catch (format "ERROR %\n" (getCurrentException()) to:log)
 if isValidNode temp do delete temp
 if isValidNode reference do delete reference
 select saved
 close log
 "R5 comparison finished"
)