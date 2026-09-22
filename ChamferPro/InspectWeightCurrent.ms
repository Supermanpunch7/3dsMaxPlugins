(
 local n=getNodeByName "Object003"
 if n==undefined do throw "Object003 missing"
 local log=createFile @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\weight-current.log"
 local saved=selection as array
 local tmp=undefined
 local failure=undefined
 try (
  format "TIME %\n" localTime to:log
  for i=1 to n.modifiers.count do format "STACK % % %\n" i n.modifiers[i].name n.modifiers[i].classID to:log
  tmp=copy n
  convertToPoly tmp
  local selected=polyop.getEdgeSelection tmp
  local targets=#()
  format "SELECTED %\n" selected to:log
  for e in selected do (
   local ev=polyop.getEdgeVerts tmp e
   local a=polyop.getVert tmp ev[1]; local b=polyop.getVert tmp ev[2]
   append targets #(a,b)
   format "OUTPUT edge=% weight=% a=% b=%\n" e (polyop.getEDataValue tmp 1 e) a b to:log
  )
  delete tmp
  tmp=copy n
  local idx=0
  for i=1 to tmp.modifiers.count where idx==0 and isProperty tmp.modifiers[i] #chamfer1 do idx=i
  if idx==0 do throw "Chamfer missing"
  for i=1 to idx do deleteModifier tmp 1
  convertToPoly tmp
  for target in targets do (
   local mid=(target[1]+target[2])*0.5
   local nearest=0; local dist=1e30
   for v=1 to polyop.getNumVerts tmp do (
    local dd=distance mid (polyop.getVert tmp v)
    if dd<dist do (dist=dd; nearest=v)
   )
   local incident=polyop.getEdgesUsingVert tmp #{nearest}
   format "CORNER vertex=% distance=% incident=%\n" nearest dist incident to:log
   for e in incident do (
    local ev=polyop.getEdgeVerts tmp e
    local a=polyop.getVert tmp ev[1]; local b=polyop.getVert tmp ev[2]
    local faces=polyop.getFacesUsingEdge tmp #{e}
    format "INPUT edge=% weight=% a=% b=% faces=%\n" e (polyop.getEDataValue tmp 1 e) a b faces to:log
   )
  )
  format "COMPLETE\n" to:log
 ) catch (failure=getCurrentException(); format "ERROR %\n" failure to:log)
 if isValidNode tmp do delete tmp
 select saved
 close log
 if failure!=undefined do throw failure
 "Current weight capture complete"
)