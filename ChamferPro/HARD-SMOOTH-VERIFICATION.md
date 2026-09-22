# HardTrial Smooth comparison — 2026-09-21

VerifyHardSmooth.ms completed 96 cases in a separate 3dsmaxbatch scene.
See hard-smooth-verification.log: COMPLETE cases=96 failed=8.
Existing HardTrial binary used; no production source or binary changed.

Matrix: box, segments 0/1/2/4, Smooth on/off (both stages), all-hard,
all-soft or mixed face groups, stage-1 Inset off/on (amount .2, offset -.2,
segments 0, type 0), second stage disabled/enabled (amount .1, segments 0).
Native default selection=5, smoothingFilter=2. Soft-only fixtures remain
unchamfered; do NOT claim newly generated soft-edge coverage.

Longitudinal oracle compares output endpoints in disjoint source corner
regions. Junctions are counted separately, NOT validated against an owner.
The failed counter excludes junction semantics and is NOT full acceptance.
All 96 cases preserved output smoothing boundaries through empty Edit Poly.
No longitudinal soft-to-hard mismatches observed within this limited fixture.

All-hard input: Smooth OFF removes remaining soft junctions, including Inset
and two stages. Example segments=4 with Inset, stage 1 only: ON has 24 hard /
864 soft junctions; OFF has 888 hard / 0 soft junctions.
Mixed input is NOT fixed universally: segments=4, Smooth OFF, two stages
has 20 hard-to-soft longitudinal mismatches without Inset and 26 with Inset.
Mixed junction ownership remains unverified; many junctions stay soft.

This supports the user's observation but not universal correctness. Actual
user mesh, plus-shaped crossing, explicit normals, and selectively changing
only one stage's Smooth were not tested. No claim of full plugin validation.