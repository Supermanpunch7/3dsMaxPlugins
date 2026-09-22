(
 local n=getNodeByName "Object003"
 if n==undefined do throw "Object003 missing"
 local log=createFile @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\selected-weight.log"
 local tmp=undefined
 local saved=selection as array
 local failure=undefined
 try (
  local chosen=undefined
  tmp=copy n
  convertToPoly tmp
  chosen=polyop.getEdgeSelection tmp
  format "SELECTED %\n" chosen to:log
  if chosen.numberSet==0 do throw "No evaluated selected edges; select the failing edges in top Edit Poly"
  local targets=#()
  for e in chosen do (
   local ends=polyop.getEdgeVerts tmp e
   local a=polyop.getVert tmp ends[1]; local b=polyop.getVert tmp ends[2]
   append targets #(a,b)
   format "OUT % weight=% a=% b=%\n" e (polyop.getEDataValue tmp 1 e) a b to:log
  )
  delete tmp; tmp=copy n
  local idx=0
  for i=1 to tmp.modifiers.count where idx==0 and isProperty tmp.modifiers[i] #chamfer1 do idx=i
  if idx==0 do throw "Chamfer missing"
  for i=1 to idx do deleteModifier tmp 1
  convertToPoly tmp
  for target in targets do (
   local midpoint=(target[1]+target[2])*0.5
   local candidates=#()
   for e=1 to polyop.getNumEdges tmp do (
    local ends=polyop.getEdgeVerts tmp e
    local a=polyop.getVert tmp ends[1]; local b=polyop.getVert tmp ends[2]
    local ab=b-a
    local t= if dot ab ab > 1e-12 then (dot (midpoint-a) ab)/(dot ab ab) else 0
    t=amax 0 (amin 1 t)
    local faces=(polyop.getFacesUsingEdge tmp #{e}) as array
    local hard=faces.count!=2
    if faces.count==2 do hard=(bit.and (polyop.getFaceSmoothGroup tmp faces[1]) (polyop.getFaceSmoothGroup tmp faces[2]))==0
    append candidates #((distance midpoint (a+ab*t)),e,a,b,polyop.getEDataValue tmp 1 e,hard)
   )
   fn compare a b = if a[1]<b[1] then -1 else if a[1]>b[1] then 1 else 0
   qsort candidates compare
   for i=1 to amin 8 candidates.count do format "CANDIDATE %\n" candidates[i] to:log
  )
  format "COMPLETE\n" to:log
 ) catch (failure=getCurrentException(); format "ERROR %\n" failure to:log)
 if isValidNode tmp do delete tmp
 select saved
 close log
 if failure!=undefined do throw failure
 "Selected edge diagnosis complete"
)