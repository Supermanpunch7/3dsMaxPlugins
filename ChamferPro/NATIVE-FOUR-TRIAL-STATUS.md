# Four-property native trial — 2026-09-21

Separate class (0x71293b46,0x19374a67), ChamferProFourTrial.dlm.
Build succeeded (native-four-trial-build.result=0).
HardTrial and Stable artifacts and registration untouched.

Generated from the native hard trial. Reuses its heuristic parent mapping and
smoothing-group boundary assignment; copies supported SDK edge channels 0/1/3
(MAXScript 1/2/4) from each resolved parent on a working mesh before commit.
Unknown junction numeric values retain native output; no full preservation claim.
Explicit normals still reject the entire transfer pass.

TestNativeFourTrial.ms finished: FAIL cases=16 failed=8.
Single stage: all eight numeric cases passed on independently classified box
longitudinal descendants, segments 0/1/2/4, with and without upper Edit Poly.
Per-parent counts were respectively 2/3/5/9; every checked Weight/Crease/Depth
matched its original parent within 1e-5.
Two stages: all eight cases failed. Per-channel mismatch counts at segments
0/1/2/4 were 72/83/79/72 with or without Edit Poly.
This test does NOT inspect hard boundaries in the combined binary.

Next blocker: investigate second-stage correspondence on dense first-stage
corners. Do not increase matching radii blindly or infer correctness from coverage.
Need mixed hard/soft and forced-soft chamfer tests, combined boundary validation,
geometry regression and UI/save/reload checks. Trial is not deployable as Stable.