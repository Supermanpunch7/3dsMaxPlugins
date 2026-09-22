(
 local root = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
 local log = createFile (root + "controls-validation.log")
 local n = undefined
 try (
  loadDllsFromDir (root + "bin\\Controls\\") "ChamferPro.dlm"
  pluginManager.loadClass ChamferPro
  local m = ChamferPro()
  if not (isProperty m #chamfer1Enabled) do throw "Old binary loaded"
  if not m.chamfer1Enabled or not m.chamfer2Enabled do throw "Stage defaults"
  n = box length:10 width:10 height:10
  convertToPoly n
  addModifier n m
  m.chamfer1Enabled = false
  m.chamfer2Enabled = false
  local snap = copy n
  convertToPoly snap
  local count = polyop.getNumEdges snap
  delete snap
  if count != 12 do throw "Both disabled must bypass chamfer"
  m.displayHardEdges = true
  m.hardEdgeColor = color 255 80 0
  local c = copy m
  if c.chamfer1Enabled or c.chamfer2Enabled or not c.displayHardEdges do throw "Clone state"
  if c.hardEdgeColor != m.hardEdgeColor do throw "Clone color"
  m.chamfer1Enabled = true
  m.chamfer2Enabled = true
  snap = copy n
  convertToPoly snap
  format "enabled edge count=%\n" (polyop.getNumEdges snap) to:log
  delete snap
  delete n
  n = undefined
  format "PASS: controls defaults, bypass, clone, evaluation\n" to:log
 ) catch (
  format "FAIL: %\n" (getCurrentException()) to:log
  if isValidNode n do delete n
 )
 close log
)