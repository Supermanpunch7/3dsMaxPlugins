# Native wrapper integration baseline — 2026-09-21

Production requirement: keep NativePrototype.cpp native Chamfer 1/2 UI and
geometry. Do not substitute the LineageLab geometry engine.

Added ChamferPro/TestNativeInheritance.ms and ran it in a separate Max batch.
Final marker: FAIL cases=16 failed=16. This is a regression baseline, not a fix.
Cases: segments 0/1/2/4, second stage disabled/enabled, Edit Poly absent/present.
Three numeric channels use different per-source values, verified on input.
The oracle identifies longitudinal box descendants only; junctions are excluded.
Coverage requires each source edge to have at least one identified descendant.
It does not yet assert the exact expected descendant count.

Confirmed stage-one example: segments=0, amount=2, stage two disabled,
24 identified descendants, 24 unresolved junction edges, and 24 mismatches
in EACH of Weight, Crease and Depth. Same result with downstream Edit Poly.
Consequently downstream Edit Poly alone cannot explain the failure.
Hard/soft output and authored normals have NOT been tested in this baseline.

The preliminary run used amount=0 rather than disabling stage two. Its output
had additional topology. The final test explicitly disables stage two and logs
parameter readbacks, avoiding the assumption that zero amount is a bypass.

No production C++ source, UI, INI registration or DLM was changed.
No integrated candidate build has been produced. Native provenance recovery
remains a blocker; the low-level lab's pre-displacement mapping cannot simply
be applied after the native modifier has already generated displaced geometry.
Do not copy by output edge index or nearest-edge guesses to hide failures.

Next acceptance work: independently verify native input/output provenance,
preserve numeric data between stages, test independent hard/soft constraints,
then compare geometry against unmodified native stages and test save/reload.