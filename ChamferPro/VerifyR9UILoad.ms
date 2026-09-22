(
    local found = for c in modifier.classes where (c.classID[1] == 0x71293b6b and c.classID[2] == 0x19374a92) collect c
    if found.count != 1 then throw "R9 class not uniquely registered"
    local ctor = found[1]
    local testModifier = ctor()
    #(ctor as string, testModifier.chamfer1 != undefined, testModifier.chamfer2 != undefined, "No modifier added to scene")
)