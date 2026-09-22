(
 local d = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
 local f = createFile (d + "lab-test.log")
 try (
  loadDllsFromDir (d + @"bin\LineageLab\") "ChamferproLineageLab.dlm"
  local cls = undefined
  for c in modifier.classes where c.classID[1] == 0x71293b42 and c.classID[2] == 0x19374a63 do cls = c
  if cls == undefined do throw "Lab class missing"
  local failures = 0
  for seg in #(1,2,4,8) do (
   local n = box length:100 width:100 height:100
   convertToPoly n
    for ch in #(1,2,4) do (
    polyop.setEDataChannelSupport n ch true
    for e = 1 to polyop.getNumEdges n do polyop.setEDataValue n ch e (e * 0.01 + ch * 0.1)
   )
   local p = cls()
    for ch in #(1,2,4) do format "INPUT % support=% first=%\n" ch (polyop.getEDataChannelSupport n ch) (polyop.getEDataValue n ch 1) to:f
   p.chamfer1Amount = 2
   p.chamfer1Segments = seg
   p.chamfer2Amount = 0
   addModifier n p
   -- Read evaluated PolyObject without collapsing through a conversion.
   format "SEG=% V=% E=%\n" seg (polyop.getNumVerts n) (polyop.getNumEdges n) to:f
   if polyop.getNumVerts n <= 8 do throw "Chamfer did not change topology"
    for ch in #(1,2,4) do (
    format "SUPPORT %=%\n" ch (polyop.getEDataChannelSupport n ch) to:f
    local count = 0
    for e = 1 to polyop.getNumEdges n do (
     local value = polyop.getEDataValue n ch e
     if value > ch*0.1 and value < ch*0.1+0.13 do count += 1
    )
    format "CH=% inheritedValues=%\n" ch count to:f
    if count != 12 * (seg + 1) do (
     failures += 1
     format "ASSERT_FAIL expected=% actual=%\n" (12*(seg+1)) count to:f
    )
   )
   delete n
   flush f
  )
  if failures > 0 then format "FAIL assertions=%\n" failures to:f
  else format "EDGE_COUNT_PASS (not full ancestry validation)\n" to:f
 ) catch (format "FAIL=%\n" (getCurrentException()) to:f)
 close f
)