(
 local d=@"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
 local log=createFile (d+"center-stages-current.log")
 local n=getNodeByName "Object003"
 local saved=selection as array
 local tmp=undefined
 local failure=undefined
 try (
  if n==undefined do throw "Object003 missing"
  local idx=0
  for i=1 to n.modifiers.count do (
   format "STACK % % enabled=% class=%\n" i n.modifiers[i].name n.modifiers[i].enabled (classof n.modifiers[i]) to:log
   if idx==0 and n.modifiers[i].enabled and isProperty n.modifiers[i] #chamfer1 do idx=i
  )
  if idx==0 do throw "Active pair missing"
  for m in #(n.modifiers[idx].chamfer1,n.modifiers[idx].chamfer2) do (
   format "STAGE % enabled=%\n" (classof m) m.enabled to:log
   for prop in getPropNames m do format "%=%\n" prop (getProperty m prop) to:log
  )
  tmp=copy n
  convertToPoly tmp
  local chosen=polyop.getEdgeSelection tmp
  local targets=#()
  for e in chosen do (
   local vv=polyop.getEdgeVerts tmp e
   append targets #(polyop.getVert tmp vv[1],polyop.getVert tmp vv[2])
   format "TOP % weight=%\n" e (polyop.getEDataValue tmp 1 e) to:log
  )
  delete tmp
  for fresh in #(false,true) do (
   tmp=copy n
   for i=1 to idx-1 do deleteModifier tmp 1
   if fresh do addModifier tmp (Edit_Poly())
   convertToPoly tmp
   format "PAIR freshEditPoly=% edges=%\n" fresh (polyop.getNumEdges tmp) to:log
   for k=1 to targets.count do (
    local hits=0
    for e=1 to polyop.getNumEdges tmp do (
     local vv=polyop.getEdgeVerts tmp e
     local a=polyop.getVert tmp vv[1]; local b=polyop.getVert tmp vv[2]
     local t=targets[k]
     if ((distance a t[1]<0.0001 and distance b t[2]<0.0001) or (distance b t[1]<0.0001 and distance a t[2]<0.0001)) do (
      hits+=1
      format "MATCH target=% edge=% weight=%\n" k e (polyop.getEDataValue tmp 1 e) to:log
     )
    )
    format "target=% matches=%\n" k hits to:log
   )
   delete tmp
  )
  format "COMPLETE\n" to:log
 ) catch (failure=getCurrentException(); format "ERROR %\n" failure to:log)
 if isValidNode tmp do delete tmp
 select saved
 close log
 if failure!=undefined do throw failure
 "Stage comparison completed"
)