# Chamferpro Lineage Lab — experimental, not production

## 2026-09-21: independent numeric fixture — SCOPED PASS ONLY

TestLineageExact.ms ran in a separate 3dsmaxbatch process. Result:
SCOPED_PASS cases=24 failed=0 (lineage-exact.result and lineage-exact.log).
Fixture: 100-unit box, amount 2, one stage, SDK segments 1/2/4/8,
three independently permuted numeric assignments, all-hard/all-soft input groups.
Input values are read back and checked before evaluation. Output endpoint
positions are uniquely classified within disjoint 10-unit source-corner regions;
distinct corner pairs independently identify the longitudinal source edge.
This is a fixture-only oracle, NOT a general nearest-edge mapping algorithm.
Each identified edge matches its own parent's Weight, Crease and Depth within
1e-5. Per-parent coverage is segments+1, not merely an aggregate range count.
Unresolved counts per case: 24/80/288/1088 for segments 1/2/4/8 respectively.

This verifies numeric output on longitudinal descendants only. It does NOT
verify junction ownership, output hard boundaries, mixed normals, segments 0,
two stages, save/reload or downstream modifiers. No production source/binary
was changed in this test step. Previous Depth range-count failures are not
evidence of failed Depth transfer on these independently identified edges.

## Follow-up 2026-09-21: endpoint identity restoration — UNVALIDATED

LineageLab.cpp now restores edge owners after FillInMesh using compacted
endpoint pairs, rejects duplicate pairs, and checks supported numeric channels
and independent hard state against each resolved parent after normal creation.
These checks do not independently prove parent correctness and skip unresolved
parents. Junction ownership remains unsolved. Stable was NOT changed.

Build confirmed: lineage-owner-build.exitcode = 0.
Batch completed: lineage-owner-test.exitcode = 0, but lab-test.log reports
FAIL assertions=4. Process success is NOT test success. Tail confirms segments=8
Crease range count improved to 108; Depth range count is 1196 versus 108.
Depth range counting remains an invalid inheritance oracle. No full pass claimed.

## Latest attempt: 2026-09-21 — REJECTED, DO NOT DEPLOY

Added LineageNormals.h: independent shading-boundary detection and explicit
corner-normal island construction. This regenerates normal directions; it does
not preserve authored normal vectors. Conflicting hard/soft fan constraints
reject the pass. No downstream hard-edge test has passed.

Changed numeric data transfer to run after dead-element compaction and removed
the final topology-cache rebuild. Build completed with exit code 0, but runtime
results regressed: observed Weight/Crease matching counts 0/12/12/12 for SDK
segments 1/2/4/8 (expected 24/36/60/108). Latest log reports 12 assertions failed.
The current source is an unsuccessful experiment, NOT a working fix.

Installed mnmesh.h defines EDATA_DEPTH as SDK index 3, so MAXScript channel 4
is now used in the test and input support was observed. The range-count test
is NOT a valid Depth inheritance oracle because the default can fall inside
the accepted range. Its reported counts must not be interpreted as inheritance.

Source changes and the new header compiled; runtime success, correct parent
mapping after compaction, hard-edge preservation, and two-pass correctness are
unproven. Existing production/native-pair binaries were not replaced.
Historical observations below describe earlier revisions, not this build.

Date: 2026-09-18

Separate Class_ID: (0x71293b42, 0x19374a63). Existing native pair remains unchanged.

## Confirmed preservation requirement (2026-09-21)

The user requires FOUR independent edge properties: hard-edge shading state,
Weight, Crease, and Depth. Hard edge means the smoothing-group/normal boundary,
NOT a Crease value. Never infer one property from another or replace one with
another. Do not assume all four use identical storage or channel APIs.

Each surviving edge and each identifiable descendant (including extended edges,
both chamfer boundaries and longitudinal segment edges) must inherit its own
source edge's four properties. Soft source edges must remain soft; hard source
edges must carry their hard boundary state. Do not average values or choose a
nearest edge at junctions. Unresolved ownership remains an explicit blocker.

Preservation applies after EACH stage: input -> Chamfer 1 -> Chamfer 2 -> output
available to the next modifier. Verify evaluated output, not just internal
temporary meshes. A downstream modifier that intentionally changes/discards
data is outside this output-preservation guarantee.

Hard-edge preservation is NOT implemented or tested yet. Determine how to
preserve both smoothing-group boundaries and explicit-normal discontinuities
without conflating them with the three numeric attributes. Existing normal
clearing and smoothing behavior require review before claiming support.

Acceptance tests must vary the four inputs independently, compare each known
descendant against its own parent, and cover segments 0/1/many, mixed hard/soft
edges, junctions, two stages, and a non-destructive downstream inspection.
Vertex attributes remain a separate requirement; do not treat an edge shading
boundary as a single scalar vertex property.

This experiment uses the SDK's low-level ChamferEdges topology operation and
MNChamferData10 geometric displacement, not the native Chamfer modifier.
Before displacement, exact vertex coordinates identify source vertices; endpoint
parent pairs identify source edges. Ambiguous coincident input vertices are NOT
resolved with nearest-distance guesses. This is not yet explicit generation-time
lineage and does not establish general correctness.

Observed box results (12 original edges):

| SDK segments | Matched longitudinal edges | Unresolved edges | Unresolved vertices |
|---|---:|---:|---:|
| 1 | 24 | 24 | 0 |
| 2 | 36 | 80 | 0 |
| 4 | 60 | 288 | 0 |
| 8 | 108 | 1088 | 0 |

Build succeeded (lab-build.exitcode = 0). Runtime test showed EData channels
1 and 2 in the expected input-value range on 36/60/108 output edges for segments
2/4/8. Segments 1 yielded ZERO matching values despite correct values inside the
pass: unresolved output/pipeline preservation bug. Counts alone do not prove
correct ownership. The original smoke test incorrectly called this a pass;
assertions now mark this discrepancy as a failure.

Limitations:
- Not installed as a replacement for the working plugin.
- Linear experimental profile; tension UI is inherited but NOT implemented.
- SDK segment numbering only; user-facing segments 0 not implemented.
- Junction/connector edge lineage unresolved.
- Vertex attribute inheritance code exists but input VData was empty in the test.
- Depth channel not tested: MaxScript test channel 3 was not supported on input.
- Second pass, mixed selections, coincident vertices, UVs and save/reload not validated.
- No claim of native-equivalent or better shape quality.

Next work: fix single-segment output data loss; verify each edge's three values
against independently known parents; add vertex data fixtures; determine actual
Depth channel access from installed SDK; implement explicit topology ownership
for junctions; then test a second stage and profile controls.