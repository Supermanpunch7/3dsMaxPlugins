# Hard seam validation — 2026-09-21

Status: NOT a verified fix. Do not deploy HardSeamValidation as a solution.

Inspected NativeHardTrial.h contains a same-owner, strip-to-strip seam
softening candidate. Existing hard-seam-build.result is 0. No production
DLL or user scene was replaced in this validation session.

Added ValidateHardSeam.ms and ran two separate 3dsmaxbatch processes,
requesting bin/NativeHardTrial and bin/HardSeamValidation respectively.
Both matrix logs completed 96 cases with 8 failures. Both junction logs
completed with identical counts:

| Requested trial | Smooth | Junction Hard | Junction Soft | Longitudinal Hard |
| --- | --- | ---: | ---: | ---: |
| NativeHardTrial | ON | 24 | 240 | 84 |
| HardSeamValidation | ON | 24 | 240 | 84 |
| NativeHardTrial | OFF | 264 | 0 | 84 |
| HardSeamValidation | OFF | 264 | 0 | 84 |

These observations do not demonstrate an improvement. The harness selects
the modifier by class ID after loadDllsFromDir; it does not yet assert the
loaded module path. A preloaded duplicate class is therefore not excluded.
Do not interpret identical outputs as proof that the candidate was executed.

The matrix tests longitudinal smoothing-group boundaries and downstream
Edit Poly preservation, not exact parent provenance at junctions. Its eight
failures include mixed-hard/soft, two-stage chamfer cases. The junction test
counts all same-source-vertex edges together and does not independently label
which ones are unwanted seams. The user's pictured model was not tested.

Next required validation: assert loaded module identity, dump each residual
hard junction edge with adjacent face/source-owner classifications, then
build independently labeled seam tests before expanding the softening rule.
Do not indiscriminately soften all same-owner edges.