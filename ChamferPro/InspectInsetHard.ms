(
 local s = stringStream ""
 format "scene=% objects=% selection=%\n" maxFileName objects.count selection.count to:s
 for n in selection do
 (
  format "node=%\n" n.name to:s
  for m in n.modifiers do
  (
   format "modifier=% class=%\n" m.name (classof m) to:s
   if isProperty m #chamfer1 do
   (
    for c in #(m.chamfer1,m.chamfer2) do
     for p in getPropNames c do format "%=%\n" p (getProperty c p) to:s
   )
  )
 )
 s as string
)