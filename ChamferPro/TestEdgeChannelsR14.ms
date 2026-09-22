(
 local s = stringStream ""
 local nodes = #()
 local saved = selection as array
 local passed = 0
 local failed = 0
 try
 (
  local cls = undefined
  for c in modifier.classes where c.classID[1] == 0x71293b72 and c.classID[2] == 0x19374a99 do cls = c
  if cls == undefined do
  (
   loadDllsFromDir @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\bin\EdgeChannelsR14\" "*.dlm"
   scanForNewPlugins()
   for c in modifier.classes where c.classID[1] == 0x71293b72 and c.classID[2] == 0x19374a99 do cls = c
  )
   if cls == undefined do throw "R14 not loaded"
  with undo off with redraw off
  (
   for mixed in #(false,true) do for stages in #(1,2) do
   (
    local n = box length:100 width:100 height:100
    append nodes n
    convertToPoly n
    polyop.setNumEDataChannels n 4
    for ch in #(1,2,4) do polyop.setEDataChannelSupport n ch true
    for e = 1 to polyop.getNumEdges n do
    (
     local w = if mixed then (0.1 + e*0.05) else 0.4
     polyop.setEDataValue n 1 #{e} w
     polyop.setEDataValue n 2 #{e} (w*0.5)
     polyop.setEDataValue n 4 #{e} (w*0.25)
    )
    local m = cls()
    for c in #(m.chamfer1,m.chamfer2) do
    (
     c.amount = 1
     c.amountType = 0
     c.depthType = 0
     c.segments = 1
     c.selectionoption = 5
     c.smoothingoption = 0
    )
    m.chamfer2.enabled = stages == 2
    addModifier n m
    local count = polyop.getNumEdges n
    if count <= 12 do throw "No chamfer topology"
    local mismatch = 0
    local checked = 0
    local unresolved = 0
     local invalidWeight = 0
    for e = 1 to count do
    (
     local w = polyop.getEDataValue n 1 e
     if mixed and abs (w-1.0) < 0.00001 then unresolved += 1 else
     (
      checked += 1
      local belongsToSource = false
      if mixed then
      (
       for sourceEdge = 1 to 12 where abs (w-(0.1+sourceEdge*0.05)) < 0.00001 do belongsToSource = true
      )
      else belongsToSource = abs (w-0.4) < 0.00001
      if not belongsToSource do invalidWeight += 1
      if abs ((polyop.getEDataValue n 2 e)-w*0.5) > 0.00001 do mismatch += 1
      if abs ((polyop.getEDataValue n 4 e)-w*0.25) > 0.00001 do mismatch += 1
     )
    )
    format "mixed=% stages=% edges=% checked=% unresolvedWeight=% channelMismatch=%\n" mixed stages count checked unresolved mismatch to:s
     local casePassed = checked > 0 and mismatch == 0 and unresolved == 0 and invalidWeight == 0
     format "sourceValueMismatch=% casePassed=%\n" invalidWeight casePassed to:s
     if casePassed then passed += 1 else failed += 1
    delete n
   )
  )
   format "SUMMARY passed=% failed=%\n" passed failed to:s
   if failed > 0 do throw "Inheritance regression failed; see per-case counts"
   format "PASS: value coverage only; spatial parent identity is not verified by this test.\n" to:s
 )
 catch (format "ERROR %\n" (getCurrentException()) to:s)
 for n in nodes where isValidNode n do with undo off delete n
 select saved
 s as string
)