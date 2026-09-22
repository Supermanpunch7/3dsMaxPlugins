# Center continuation Weight trial

## Verified in live Object003 via MCP
- Baseline: ChamferProR5Defaults, existing upper stack preserved on copies.
- Selected output edges: 9221, 9224, 9881, 9882.
- All four baseline values 1; all four trial values 5.
- Input incident continuation edges 1116 and 4255 carry 5; another crossing direction carries 0.3.
- Total changed output weights: 16. Other 12 changes not individually ancestry-verified.
- Non-default baseline weights overwritten: 0.
- Vertex position and face smoothing-group mismatches: 0.
- Temporary copies deleted; original modifier stack not replaced.
- Build exit code 0; MCP test passed (center-weight-live.log).

## Scope
ChamferProCenterWeight retains R5Defaults settings/UI and adds final-output Weight repair only.
No hard/smoothing, crease or depth changes in the new helper.
Finds near-straight incident hard parent pairs with identical weights, and short output wires
inside a narrow corridor around their shared vertex. Conflicting weights reject assignment.
This remains geometric inference, not native topology ancestry. No edge IDs or value 5
are hardcoded in the plugin. The test asserts 5 for the currently selected known case.
Does not repair weights between Chamfer 1 and Chamfer 2; only after both existing stages.
General offset parallel wires, differing-weight continuation pairs, other meshes and
animation/save-load have not been validated. Specified normals not separately compared.

## Files
- WeightCenterContinuation.h: new helper.
- BuildCenterWeight.ps1: isolated build generated from R5Defaults.cpp/vcxproj.
- TestCenterWeight.ms: live copy-only regression for selected four wires.
- bin/CenterWeight/ChamferProCenterWeight.dlm: loaded trial class.