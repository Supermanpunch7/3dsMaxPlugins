(
 local report = stringStream ""
 local original = getNodeByName "Object003"
 local temp = undefined
 local savedSelection = selection as array
 fn collectHard n =
 (
  local result = #{}
  for e = 1 to polyop.getNumEdges n do
  (
   local fs = (polyop.getFacesUsingEdge n #{e}) as array
   if fs.count == 2 do
    if bit.and (polyop.getFaceSmoothGroup n fs[1]) (polyop.getFaceSmoothGroup n fs[2]) == 0 do result[e] = true
  )
  result
 )
 try
 (
  if original == undefined do throw "Object003 missing"
  with undo off with redraw off
  (
   temp = copy original
   temp.name = uniqueName "__ChamferProInsetDiagnostic"
   while temp.modifiers.count > 0 and not isProperty temp.modifiers[1] #chamfer1 do deleteModifier temp 1
   if temp.modifiers.count == 0 do throw "ChamferPro missing"
   local pair = temp.modifiers[1]
   local native = copy pair.chamfer1
   pair.chamfer2.enabled = false
   local wrappedHard = collectHard temp
   local wrappedVerts = for v = 1 to polyop.getNumVerts temp collect polyop.getVert temp v
   local wrappedEdges = for e = 1 to polyop.getNumEdges temp collect polyop.getEdgeVerts temp e
   format "R9 stage1 verts=% edges=% hard=%\n" wrappedVerts.count wrappedEdges.count wrappedHard.numberSet to:report
   deleteModifier temp 1
   addModifier temp native
   local nativeHard = collectHard temp
   local same = wrappedVerts.count == polyop.getNumVerts temp and wrappedEdges.count == polyop.getNumEdges temp
   if same do
   (
    for v = 1 to wrappedVerts.count where distance wrappedVerts[v] (polyop.getVert temp v) > 0.00001 do same = false
    for e = 1 to wrappedEdges.count do
    (
     local ev = polyop.getEdgeVerts temp e
     if ev[1] != wrappedEdges[e][1] or ev[2] != wrappedEdges[e][2] do same = false
    )
   )
   format "Native stage1 verts=% edges=% hard=% exactTopology=%\n" (polyop.getNumVerts temp) (polyop.getNumEdges temp) nativeHard.numberSet same to:report
   if same do format "Lost native hard=% Added R9 hard=%\n" (nativeHard-wrappedHard) (wrappedHard-nativeHard) to:report
   delete temp
   temp = undefined
  )
 )
 catch (format "ERROR: %\n" (getCurrentException()) to:report)
 if isValidNode temp do with undo off delete temp
 select savedSelection
 report as string
)