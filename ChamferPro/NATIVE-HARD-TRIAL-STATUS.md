# Native hard inheritance trial — 2026-09-21

Built separate ChamferProHardTrial.dlm (build.result=0), class ID
(0x71293b45,0x19374a66). Stable source, binary and plugin registration unchanged.
Generated from NativePrototype.cpp, retaining native stage editing pages.
UI has NOT been visually checked. No claim of production readiness.

NativeHardTrial.h snapshots the input before each enabled native stage, then
assigns smoothing-group boundaries on the output. Positions/topology and native
parameters are not edited by this routine. Geometry equivalence was not tested.
It uses disjoint 20%-nearest-vertex-spacing neighborhoods to infer longitudinal
parents. This is a small-offset heuristic, NOT trustworthy general provenance.
Unknown junction edges retain their native hard/soft boundary states.
Coincident vertices, explicit normals, excessive mesh sizes and exhausted group
bits reject the pass without changing groups. Rejections can be logged through
CHAMFERPRO_HARD_LOG; no user-facing rejection UI exists yet.

TestNativeHardTrial.ms completed 36 diagnostic cases, failed=0:
segments 0/1/2/4, three smoothing modes, hard/soft/mixed source groups.
All identified longitudinal edges matched parent hard state and an empty
downstream Edit Poly retained every output boundary. Default all-hard results
include 24 edges at segments 0 and 36 at segments 1 (12 source edges).

LIMITATIONS: all-soft fixture did not chamfer soft edges; coverage checks only
require at least one descendant per parent. Second stage was disabled in tests.
No second-stage consumer, explicit-normal, complex-junction, save/reload, UI,
performance or geometry regression acceptance yet. Weight/Crease/Depth deferred.
Do not replace Stable with this trial.