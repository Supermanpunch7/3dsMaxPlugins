(
 local root = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
 local report = createFile (root + "defaults-release-validation.log")
 try (
  pluginManager.loadClass ChamferPro
  local m = ChamferPro()
  local a = m.chamfer1
  local b = m.chamfer2
  local expected = #(
   #(#miteringType,1,1), #(#miterEndBias,0.5,0.5),
   #(#amountType,3,3), #(#scale,units.decodeValue "0.02cm",units.decodeValue "0.05cm"),
   #(#segments,1,0), #(#depthType,1,1), #(#radiusBias,0.0,0.0),
   #(#limiteffect,true,true), #(#addinset,true,false), #(#insetType,0,0),
   #(#insetamount,units.decodeValue "0.2cm",units.decodeValue "0.5cm"),
   #(#insetsegments,0,0), #(#insetoffset,units.decodeValue "-0.2cm",0.0),
   #(#selectionoption,5,5), #(#smoothingoption,2,2), #(#materialoption,1,2),
   #(#useminangle,false,false), #(#usemaxangle,false,false), #(#setmaterial,false,false),
   #(#smooth,false,true), #(#SmoothType,1,1), #(#smoothtoadjacent,true,true),
   #(#smooththreshold,30.0,180.0), #(#openchamfer,false,false), #(#invert,false,false))
  for row in expected do (
   for i = 1 to 2 do (
    local actual = getProperty (if i == 1 then a else b) row[1]
    local want = row[i+1]
    local ok = if classof want == Float then abs(actual-want) < 0.00001 else actual == want
    format "stage=% property=% value=% expected=% OK=%\n" i row[1] actual want ok to:report
    if not ok do throw ("Default mismatch: " + row[1] as string)
   )
  )
  a.scale = units.decodeValue "0.37cm"
  local cloned = copy m
  if abs(cloned.chamfer1.scale-a.scale) > 0.00001 do throw "Clone settings overwritten"
  format "PASS: defaults and clone preservation\n" to:report
 ) catch (format "FAIL: %\n" (getCurrentException()) to:report)
 close report
)