(
 local report = stringStream ""
 local nodes = #()
 local savedSelection = selection as array
 local failure = undefined
 try (
  loadDllsFromDir @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\bin\LineageLab\" "ChamferproLineageLab.dlm"
  local cls = undefined
  for c in modifier.classes where c.classID[1] == 0x71293b42 and c.classID[2] == 0x19374a63 do cls = c
  if cls == undefined do throw "Lab not loaded"
  for seg in #(1,2) do (
   local n = box name:(uniqueName "__ChamferproResearch_") length:100 width:100 height:100
   append nodes n
   convertToPoly n
   for ch in #(1,2,4) do (
    polyop.setEDataChannelSupport n ch true
    for e=1 to 12 do polyop.setEDataValue n ch e (0.02*e+0.03*ch)
   )
   local p=cls()
   p.chamfer1Amount=2
   p.chamfer1Segments=seg
   p.chamfer2Amount=0
   addModifier n p
   for mode=1 to 2 do (
    if mode==2 do convertToPoly n
    format "seg=% mode=% edges=%\n" seg mode (polyop.getNumEdges n) to:report
    for ch in #(1,2,4) do (
     local values=for e=1 to (amin 18 (polyop.getNumEdges n)) collect polyop.getEDataValue n ch e
     format "channel=% values=%\n" ch values to:report
    )
   )
  )
 ) catch (failure=getCurrentException())
 for n in nodes where isValidNode n do delete n
 if savedSelection.count>0 then select savedSelection else clearSelection()
 format "failure=% remainingNodes=%\n" failure objects.count to:report
 report as string
)