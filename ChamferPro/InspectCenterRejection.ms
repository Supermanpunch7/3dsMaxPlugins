(
 local d=@"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
 local log=createFile (d+"center-rejection-current.log")
 local saved=selection as array
 local tmp=undefined
 local failure=undefined
 try (
  local n=getNodeByName "Object003"
  if n==undefined do throw "Object003 missing"
  tmp=copy n
  convertToPoly tmp
  local targets=#()
  for e in polyop.getEdgeSelection tmp do (
   local vv=polyop.getEdgeVerts tmp e
   append targets #(e,polyop.getVert tmp vv[1],polyop.getVert tmp vv[2])
  )
  delete tmp
  tmp=copy n
  local idx=0
  for i=1 to tmp.modifiers.count where idx==0 and tmp.modifiers[i].enabled and isProperty tmp.modifiers[i] #chamfer1 do idx=i
  if idx==0 do throw "Active pair missing"
  for i=1 to idx do deleteModifier tmp 1
  convertToPoly tmp
  fn hard obj e = (
   local fs=(polyop.getFacesUsingEdge obj #{e}) as array
   fs.count!=2 or (bit.and (polyop.getFaceSmoothGroup obj fs[1]) (polyop.getFaceSmoothGroup obj fs[2]))==0
  )
  fn sortDistance a b = if a[1]<b[1] then -1 else if a[1]>b[1] then 1 else 0
  for target in targets do (
   local p=target[2]; local q=target[3]; local dir=normalize(q-p)
   local candidates=#()
   for v=1 to polyop.getNumVerts tmp do (
    local center=polyop.getVert tmp v
    local radial=amax (distance p center) (distance q center)
    if radial<2.0 do (
     local es=for e in polyop.getEdgesUsingVert tmp #{v} where hard tmp e collect e
     for i=1 to es.count-1 do for j=i+1 to es.count do (
      local x=es[i]; local y=es[j]
      local w=polyop.getEDataValue tmp 1 x
      if w==polyop.getEDataValue tmp 1 y do (
       local ev=polyop.getEdgeVerts tmp x
       local a=(polyop.getVert tmp (if ev[1]==v then ev[2] else ev[1]))-center
       ev=polyop.getEdgeVerts tmp y
       local b=(polyop.getVert tmp (if ev[1]==v then ev[2] else ev[1]))-center
       local radius=(amin (length a) (length b))*0.2
       a=normalize a; b=normalize b
       local bend=dot a b
       if bend< -0.8 do (
        local corridor=0.0
        for point in #(p,q) do (
         local dv=point-center
         local da=length(dv-a*(amax 0.0 (dot dv a)))
         local db=length(dv-b*(amax 0.0 (dot dv b)))
         corridor=amax corridor (amin da db)
        )
        append candidates #(radial,v,x,y,w,bend,radius,(amax (abs(dot dir a)) (abs(dot dir b))),corridor)
       )
      )
     )
    )
   )
   qsort candidates sortDistance
   format "TARGET % columns=radial,vertex,parentA,parentB,weight,bend,radius,alignment,corridor\n" target[1] to:log
   for i=1 to amin 8 candidates.count do format "%\n" candidates[i] to:log
  )
  format "COMPLETE\n" to:log
 ) catch (failure=getCurrentException(); format "ERROR %\n" failure to:log)
 if isValidNode tmp do delete tmp
 select saved
 close log
 if failure!=undefined do throw failure
 "Rejection inspection completed"
)