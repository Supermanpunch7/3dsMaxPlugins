# Object003 Weight R3 — 2026-09-21

## Confirmed cause
The live R2 diagnostic returned -1 for both native stages. The corresponding
guard rejects any source/destination specified-normal data and skipped Weight
copying along with smoothing-group reconstruction.

## Isolated change
BuildCornerWeightR3.ps1 snapshots R2 into a separate class and binary. Weight
copying now precedes the specified-normal/smoothing-group guards. Specified
normals and native smoothing groups are not modified on that guarded path.
Existing CornerTrial and R2 binaries were not overwritten.

## Live MCP test
TestObject003R3.ms used temporary copies of Object003 and copied both native
Chamfer parameter sets. It added a fresh Edit Poly before converting copies
for measurement. The existing top Edit Poly edits were not part of this test.

| Version | Second stage | Edges | Nondefault weights | Range |
|---|---|---:|---:|---|
| R2 | off | 3284 | 56 | 1–18.8337 |
| R2 | on | 4411 | 0 | 1–1 |
| R3 | off | 3284 | 66 | 0.3–18.8337 |
| R3 | on | 4411 | 116 | 0.3–18.8337 |

The log completed successfully. This proves partial channel recovery, NOT
exact inheritance for every child edge. Parent matching remains the limited
small-offset geometric heuristic. Hard-boundary equivalence, shape equivalence,
mixed-weight junctions, and all-child coverage still need verification.

R3 was loaded into the interactive Max session for testing only; Object003's
original stack was not replaced. Temporary test nodes are deleted by the script.

## Follow-up coverage test on the current live settings
TestObject003WeightCoverage.ms completed through MCP. The live source now uses
R3, first scale 0.021 and second scale 0.0. These are different conditions from
the earlier R2/R3 comparison; do not compare their raw counts as a regression.
The lower evaluated stack was collapsed on temporary copies only.

With all input edges assigned 1.83 on a temporary copy, output had 3219 edges:
3203 retained 1.83 and 16 were 1.0. The log contains IDs and world-space endpoints
of all 16 failures. Several failures share endpoints in four spatial clusters.
This is evidence of incomplete correspondence, not proof of a particular
topological parent for those edges. No global overwrite workaround was applied.

Second-stage ON/OFF produced identical counts with scale 0.0; this follow-up
must NOT be reported as successful testing of a nonzero second chamfer.
Original-weight output contained 1.0, 0.3, 2.0, 1.83 and 5.0, but individual
parent-child correctness has not yet been proved. Test copies were cleaned up
and MCP returned success. No DLL changes were made during this follow-up.