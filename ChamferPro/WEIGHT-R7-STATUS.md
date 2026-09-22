# R7 actual-scene test

- MCP trace ran on Object003 copies. Current input includes Weight 20.
- Trace confirms R6 assigns 1 over native retained 20 on eight edges, including
  264,482,678,749,1452,1666,1860,1928. Opposite quad is not parent provenance.
- R7 is derived from R5, not R6: seeds unresolved regions with retained non-default
  native values only when the same value exists in input. This is value evidence,
  NOT proof of a unique parent. Conflicting boundary values remain unresolved.
- Build result=0; R7 live mixed output equals R3: 2766 edges, 324 non-default;
  changed=0; geometryMismatch=0; smoothingMismatch=0; edge2666 remains 1.
- All-input-5 control: 484 previously-default values restored; geometry and face
  smoothing groups match R3; COMPLETE recorded. Does not solve mixed-parent case.
- Original Object003 stack not replaced. Cleanup: objects=16 temporary=#().
  Current active trial is R5; R3/R4/CornerTrial disabled (user's stack preserved).
- R7 is NOT an approved replacement. No demonstrated mixed-weight improvement.