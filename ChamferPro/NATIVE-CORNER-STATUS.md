# Corner hard-only trial — 2026-09-21

ChamferProCornerTrial, class (0x71293b47,0x19374a68), built successfully.
Stable, HardTrial and FourTrial were not overwritten or registered differently.
No Weight/Crease/Depth transfer code is included in this variant.

Adds unanimous incident-parent state for generated edges whose two endpoints
map to the same original vertex neighborhood. All incident parents hard -> hard;
all soft -> soft; mixed -> retain native boundary (UNSOLVED ownership).
Still uses HardTrial's small-offset positional heuristic, not general provenance.

Separate batch TestNativeCornerTrial.ms: COMPLETE cases=36 failed=0.
All-hard box corner checks per case at segments 0/1/2/4: 24/72/240/864,
all with zero soft-edge mismatches. Longitudinal parent mismatches and upper
Edit Poly boundary mismatches were zero in all 36 conditions.
Test corner classification counts all non-longitudinal edges in all-hard fixture.
All-soft fixture does not force chamfer, so generated-soft coverage unproven.

Not validated: mixed-corner ownership, explicit normals, large offsets, complex
meshes, two-stage output inheritance, geometry regression, UI or save/reload.
Native UI code retained, but no visual UI test. Not a production Stable replacement.