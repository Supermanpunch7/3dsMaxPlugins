# CenterWeight: current Object003 failure

Live MCP inspection: 2026-09-21. Original modifier stack was not modified.
Both inspection scripts operated on temporary copies, cleaned them up, and
restored node selection. MCP responses reported successful completion.

## Reproduction

Active pair: ChamferProCenterWeight. R5Defaults below it is disabled.
Chamfer 1: scale 0.1, Inset enabled, amount 0.2, offset -0.2.
Chamfer 2: enabled, scale 0.0.
Selected output edges: 9192, 9195, 9852, 9853; all Weight 1.
Their matching geometric edges still have Weight 1 when the upper Edit Poly is
removed, and when a fresh Edit Poly is added instead. This is not an upper
Edit Poly-only data-loss problem.

## Exact rejected conditions

WeightCenterContinuation.h builds a path from source edges 1116 and 4255
(both Weight 5) at source vertex 17. Their direction dot product is -0.979872,
so the parent continuation test accepts this pair.

However the path radius is only 0.101946. The four output edges have maximum
endpoint distances from the parent junction of 0.224811 to 0.230002. All fail
the radius test.

The corridor tolerance is radius * 0.10 = approximately 0.010195. The measured
perpendicular corridor distances are 0.219537 to 0.221106. All also fail this
test.

Edges 9192 and 9852 have direction alignment approximately 0.99874 (passes).
Edges 9195 and 9853 have alignment 0.741491 and 0.770683 (fails threshold 0.97).
Increasing radius alone therefore cannot recover all four edges.

## Implication

The existing algorithm recognizes near-center, straight continuation only;
it does not account for the displaced Inset boundary or its bent connecting
segments. The earlier successful four-edge test used different output edges
and did not record complete settings or endpoint correspondence; it does not
establish correctness for this case.

Do not simply enlarge all tolerances or assign every nearby default-valued
edge Weight 5. Nearby perpendicular parents carry Weight 0.3, and Weight 1 is
also a valid authored value. A replacement needs to recognize the offset
boundary as a connected path and distinguish its side connections before
assigning parent data. Preserve hard-edge behavior and non-default weights.

No replacement DLL was built in this diagnostic step. Current CenterWeight
remains a failing trial, not an approved fix.

Evidence: center-stages-current.log, center-rejection-current.log and their
corresponding MCP response JSON files. Scripts: InspectCenterStages.ms and
InspectCenterRejection.ms.