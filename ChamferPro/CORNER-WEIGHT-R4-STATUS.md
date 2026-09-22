# R4 experimental weight adjacency

- Separate R4 binary built successfully (corner-weight-r4-build.result = 0).
- Existing R3/CornerTrial binaries and original scene were not replaced.
- Added WeightFaceCandidates.h: intersect parent candidates on adjacent faces for unresolved same-corner edges. Requires one candidate, no recursive propagation.
- This remains a heuristic, NOT native topology provenance. Do not claim exact inheritance.
- Hard-boundary code is unchanged. Weight can influence stage-two geometry, so regression comparison remains required.
- First live test stopped before copying any node: R3 lookup used array equality. Changed lookup to compare both class-ID elements.
- Retry failed connecting to MCP at 127.0.0.1:60640 (connection refused).
- R4 has NOT been live validated or confirmed loaded. No successful 5-to-5 result yet.
- Resume with TestObject003R4.ms after MCP is available; inspect log for COMPLETE, changed weights, geometry and smoothing mismatches. Test different parent values and nonzero second-stage amount before approving.

## Live retest (supersedes the connection-blocked status above)
- MCP responded successfully. Current Object003 has CornerTrial, R3 and R4 entries; activation flags were not recorded, so do not assume all are enabled.
- Revised TestObject003R4.ms to remove all trial entries on temporary copies and compare one R3 or R4 on the same underlying input, using copied R3 stage parameters.
- Isolated output: 853 edges, 138 non-default weights for each version; changed weights=0, vertex-position mismatches=0, smoothing-group mismatches=0. COMPLETE received.
- TestObject003R4Five.ms sets all INPUT edges to Weight 5 on collapsed temporary copies only. Each version outputs 853 edges, of which 577 retain 5 and 276 fail the expected value. R4 offers no improvement for this case.
- Thus mixed parent weights are NOT the sole failure mode. Existing vertex ownership/radius guards or unresolved endpoints must be diagnosed next; the new face-candidate pass still depends on that ownership and does not fix it.
- No production binary or original node was replaced. Tests contain cleanup and completed; separate post-cleanup node-count confirmation remains pending.