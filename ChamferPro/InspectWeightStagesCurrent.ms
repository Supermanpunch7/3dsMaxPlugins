(
 local n=getNodeByName "Object003"
 if n==undefined do throw "Object003 missing"
 local log=createFile @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\weight-stages-current.log"
 local saved=selection as array
 local tmp=undefined
 local failure=undefined
 try (
  tmp=copy n
  convertToPoly tmp
  local targets=#()
  for e in polyop.getEdgeSelection tmp do (
   local ev=polyop.getEdgeVerts tmp e
   append targets #(polyop.getVert tmp ev[1],polyop.getVert tmp ev[2])
  )
  delete tmp; tmp=undefined
  if targets.count==0 do throw "Select failing output edges"
  local idx=0
  for i=1 to n.modifiers.count where idx==0 and isProperty n.modifiers[i] #chamfer1 do idx=i
  if idx==0 do throw "ChamferPro missing"
  for removeCount=0 to idx do (
   tmp=copy n
   for i=1 to removeCount do deleteModifier tmp 1
   format "STAGE removedTop=% remaining=%\n" removeCount (for m in tmp.modifiers collect m.name) to:log
   convertToPoly tmp
   for target in targets do (
    local candidates=#()
    for e=1 to polyop.getNumEdges tmp do (
     local ev=polyop.getEdgeVerts tmp e
     local a=polyop.getVert tmp ev[1]; local b=polyop.getVert tmp ev[2]
     local score=amin ((distance a target[1])+(distance b target[2])) ((distance b target[1])+(distance a target[2]))
     append candidates #(score,e,polyop.getEDataValue tmp 1 e,a,b)
    )
    fn cmp a b=if a[1]<b[1] then -1 else if a[1]>b[1] then 1 else 0
    qsort candidates cmp
    for k=1 to amin 4 candidates.count do format "MATCH %\n" candidates[k] to:log
   )
   delete tmp; tmp=undefined
  )
  format "COMPLETE\n" to:log
 ) catch (failure=getCurrentException(); format "ERROR %\n" failure to:log)
 if isValidNode tmp do delete tmp
 select saved
 close log
 if failure!=undefined do throw failure
 "Stage comparison complete"
)