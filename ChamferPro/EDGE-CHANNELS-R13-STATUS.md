# Edge channels R13 — 2026-09-21

Built with the installed 3ds Max 2027 SDK; MSBuild exit code 0.
Class: ChamferProEdgeChannelsR13, ID 0x71293b71 / 0x19374a98.
Loaded into the running Max session. Original Object003 stack was not replaced.
R9 source and installed DLL were not changed. FaceLineageWeight.h now accepts
an optional scalar-channel argument; its default remains EDATA_KNOT.

R13 invokes the same lineage algorithm separately for EDATA_KNOT,
EDATA_CREASE and EDATA_DEPTH after each native stage. The temporary map tag
is removed only after all three passes. Missing source channels are skipped.
Uniform-value and ambiguous-candidate consensus are evaluated per channel;
equal weights cannot incorrectly resolve conflicting crease/depth values.
Unknown parents retain the native output, as in R9. Inset hard-edge behavior
and smoothing repair are unchanged.

Live test: TestEdgeChannelsR13.ms, edge-channels-r13-test.json.
Uniform box: stage1 108 edges, stage2 972 edges, all channel ratios passed.
Mixed box: stage1 84 checked / 24 default-weight edges excluded;
stage2 508 checked / 464 default-weight edges excluded. Zero ratio mismatches
among checked edges. These are consistency checks, not full parent-coverage
proofs. Arbitrary independently mixed channels, user production geometry,
save/reload and downstream Edit Poly/OpenSubdiv remain unverified.
Temporary test boxes were deleted and selection restored.
No startup registration or automatic migration of existing R9 instances.