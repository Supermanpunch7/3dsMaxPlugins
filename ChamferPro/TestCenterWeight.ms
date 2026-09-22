(
 local d=@"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
 local log=createFile (d+"center-weight-live.log")
 local saved=selection as array
 local baseline=undefined
 local trial=undefined
 local failure=undefined
 try (
  local n=getNodeByName "Object003"
  if n==undefined do throw "Object003 missing"
  local idx=0
  for i=1 to n.modifiers.count where idx==0 and n.modifiers[i].enabled and isProperty n.modifiers[i] #chamfer1 do idx=i
  if idx==0 do throw "No active ChamferPro"
  baseline=copy n
  convertToPoly baseline
  local targets=polyop.getEdgeSelection baseline
  format "targets=% source=%\n" targets n.modifiers[idx].name to:log
  if targets.numberSet!=4 do throw "Expected four selected problem wires"
  loadDllsFromDir (d+@"bin\CenterWeight\") "ChamferProCenterWeight.dlm"
  local cls=undefined
  for c in modifier.classes where c.classID[1]==0x71293bb2 and c.classID[2]==0x19374ad2 do cls=c
  if cls==undefined do throw "Test class not loaded"
  trial=copy n
  local m=cls()
  m.chamfer1=copy n.modifiers[idx].chamfer1
  m.chamfer2=copy n.modifiers[idx].chamfer2
  deleteModifier trial idx
  addModifier trial m before:(idx-1)
  convertToPoly trial
  if polyop.getNumEdges trial!=polyop.getNumEdges baseline or polyop.getNumVerts trial!=polyop.getNumVerts baseline or polyop.getNumFaces trial!=polyop.getNumFaces baseline do throw "Topology count mismatch"
  local geometry=0; local smoothing=0; local changed=0; local overwritten=0
  for v=1 to polyop.getNumVerts baseline where distance (polyop.getVert baseline v) (polyop.getVert trial v)>0.000001 do geometry+=1
  for f=1 to polyop.getNumFaces baseline where polyop.getFaceSmoothGroup baseline f!=polyop.getFaceSmoothGroup trial f do smoothing+=1
  for e=1 to polyop.getNumEdges baseline do (
   local a=polyop.getEDataValue baseline 1 e
   local b=polyop.getEDataValue trial 1 e
   if abs(a-b)>0.00001 do (changed+=1; if a!=1 do overwritten+=1)
  )
  local failed=0
  for e in targets do (
   local a=polyop.getEDataValue baseline 1 e
   local b=polyop.getEDataValue trial 1 e
   format "TARGET % before=% after=%\n" e a b to:log
   if abs(b-5)>0.00001 do failed+=1
  )
  format "geometryMismatch=% smoothingMismatch=% changed=% overwrittenNonDefault=% targetFailures=%\n" geometry smoothing changed overwritten failed to:log
  if geometry!=0 or smoothing!=0 or overwritten!=0 or failed!=0 do throw "Regression or target failure"
  format "PASS selected four wires\n" to:log
 ) catch (failure=getCurrentException(); format "ERROR %\n" failure to:log)
 if isValidNode baseline do delete baseline
 if isValidNode trial do delete trial
 select saved
 format "Temporary objects cleaned\n" to:log
 close log
 if failure!=undefined do throw failure
 "Center weight comparison passed"
)