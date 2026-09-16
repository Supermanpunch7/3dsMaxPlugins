@echo off
del "C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\ChamferProStressTest.log" 2>nul
"C:\Program Files\Autodesk\3ds Max 2027\3dsmaxbatch.exe" "C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\ChamferProStressTest.ms" -v 4 >"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\ChamferProStressBatchOutput.txt" 2>&1
echo %ERRORLEVEL%>"C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\ChamferPro\ChamferProStressTest.exitcode"