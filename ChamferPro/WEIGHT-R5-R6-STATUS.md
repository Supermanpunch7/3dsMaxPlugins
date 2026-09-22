# Weight R5/R6 live experiments

Existing CornerTrial/R3/R4 binaries and Object003 stack were not replaced.

## Diagnostic
Current input differed from the previous 853-edge evaluation. New outputs have
2766 edges. Instrumented stage: 1160 source vertices / 2236 source edges;
2101 assigned output edges and 665 unresolved, ALL involving an unowned endpoint.
Specified normals present. Old 0.2-distance ownership radius misses these points.
Not every unresolved edge has default weight; native output retains some values.

## R5: unanimous connected-region boundary weights
Weight-only fallback on unresolved edge components. Copies a value only when all
resolved surrounding candidates have exactly that value; conflicting regions stay
unresolved. This is value inference, NOT verified individual-parent provenance.

Live Object003 copy, all INPUT edge weights=5:
- R3: 484 wrong out of 2766.
- R5: 0 wrong out of 2766.
- Vertex positions and face smoothing groups identical to R3.
- Edit Poly included above tested modifier.

Original mixed input: R3 and R5 identical, 268 non-default edges. No demonstrated
mixed-parent improvement. Nonzero stage-two geometry, arbitrary insets, explicit
normal values, save/reload and every individual parent remain unverified.

## R6: rejected opposite-quad propagation
Build succeeded. Live mixed comparison incorrectly changed four existing 5 values
to 1 (edges 264,482,1452,1666). DO NOT DEPLOY. Opposite-quad connectivity is not
sufficient evidence for an original parent. R6 is isolated and not auto-registered.

## Test hygiene
Diagnostic compile fixed explicit int casts for SDK count wrappers.
Diagnostic script failed restoring an absent .NET environment value AFTER cleanup;
VerifyWeightCleanup.ms explicitly clears the diagnostic-only variable.
Read weight-final-cleanup.log to confirm no temporary objects remain.

R5 is partial progress, not the requested complete solution. Do not promote it
based on all-input-same-value coverage alone.