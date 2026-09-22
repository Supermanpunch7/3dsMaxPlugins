(
    local targetID = #(0x71293b6b, 0x19374a92)
    local existing = for c in modifier.classes where (c.classID[1] == targetID[1] and c.classID[2] == targetID[2]) collect c
    if existing.count != 0 then
        throw "R9 already registered. Restart required; no duplicate module loaded."
    loadDllsFromDir @"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\bin\R9UIFix\" "ChamferProCornerWeightR9.dlm"
    local loaded = for c in modifier.classes where (c.classID[1] == targetID[1] and c.classID[2] == targetID[2]) collect c
    if loaded.count != 1 then throw "R9 UI fix class registration failed."
    #(loaded[1] as string, loaded[1].classID, "Loaded only. Scene stack unchanged.")
)