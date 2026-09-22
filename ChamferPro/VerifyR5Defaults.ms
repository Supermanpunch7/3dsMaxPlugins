(
 local root = @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\"
 loadDllsFromDir (root + @"bin\R5Defaults\") "ChamferProR5Defaults.dlm"
 local cls = undefined
 for c in modifier.classes where c.classID[1] == 0x71293ba1 and c.classID[2] == 0x19374ac1 do cls=c
 if cls == undefined do throw "Defaults plugin not loaded"
 local m=cls()
 local input=openFile (root+"default-settings-live.log")
 local output=createFile (root+"r5-defaults-validation.log")
 local stages=#(m.chamfer1,m.chamfer2)
 local stage=0
 local checked=0
 local failed=0
 while not eof input do
 (
  local row=readLine input
  if row == "STAGE" then stage+=1 else
  (
   local fields=filterString row "="
   if fields.count==2 and fields[1]!="#presetOptionsComboBox" do
   (
    local key=execute fields[1]
    local expected=execute fields[2]
    local actual=getProperty stages[stage] key
    local same=if classof expected==Float then abs(actual-expected)<0.00001 else actual==expected
    checked+=1
    if not same do (failed+=1; format "FAIL stage=% property=% expected=% actual=%\n" stage key expected actual to:output)
   )
  )
 )
 local duplicate=copy m
 if duplicate.chamfer1.scale != m.chamfer1.scale do failed+=1
 format "CHECKED=% FAILURES=%\n" checked failed to:output
 close input
 close output
 if failed>0 do throw "Default settings validation failed"
 "PASS - new plugin defaults match both captured stages; scene unchanged"
)