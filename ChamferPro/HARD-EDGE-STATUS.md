# Hard-edge priority — 2026-09-21

User deferred Weight, Crease and Depth. Preserve the stable native pair UI and
geometry. Do not substitute the low-level lab or globally harden all edges.

TestNativeHardEdges.ms executed in a separate Max batch after correcting a
MAXScript case-expression syntax error. Final log: COMPLETE cases=36.
This is diagnostic completion, NOT 36 passing inheritance tests.

Matrix: segments 0/1/2/4; native defaults / smooth=false /
smooth=true, smoothtype=1, smoothtoadjacent=false; all-hard/all-soft/mixed
smoothing-group box inputs. Second native stage explicitly disabled.

Every case reported downstreamMismatch=0 after adding an empty Edit Poly.
Edge count and endpoint-index pairs were checked before comparing indices.
Thus existing output smoothing-group boundary bits survived this downstream
operation in these fixtures. Explicit normals were not inspected.

With native defaults, segments=0 had zero longitudinal parent mismatches
for all three inputs. Segments=1/2/4 had 12/36/84 mismatches for all-hard
input and 8/24/56 for mixed input. Junctions excluded from parent comparison.
Smooth=false did NOT fix mixed input: 16/16/32/64 parent mismatches.
Do not deploy a blanket smooth=false workaround.

Important coverage limitation: native selection defaults were not overridden.
The all-soft input had no unresolved corner edges and does not demonstrate
soft-edge chamfer inheritance. Later tests must explicitly select soft edges,
assert native selection parameter readback and require changed topology.
Coverage currently only requires at least one descendant per source edge.

No stable C++ or DLM changed. No hard-preservation implementation is complete.
Next: separate original chamfer boundary preservation from inheritance onto
internal segment edges; inspect explicit-normal boundaries; exercise mixed
selected edges and both stages before integrating a preservation algorithm.