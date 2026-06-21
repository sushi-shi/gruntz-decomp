# Large branchy /GX function: objdiff fuzzy% is unreliable (alignment desyncs) — read the per-instruction diff_kind histogram, not the rollup
tags: topic:wall topic:tooling topic:scoring-artifact | cpp:eh
symptoms: a 3000-B SEH function with many member-chain call sites + multi-way top-level paths rolls up 0.0% fuzzy despite a faithful carcass; INSERT/DELETE pairs dominate the rollup
confidence: 7/10

A large `/GX` function with multi-way top-level dispatch and many nested member-chain call sites
plateaus LOW (even 0.0% rollup) not because the carcass is wrong but because objdiff's GLOBAL
alignment DESYNCS at a multi-way branch: once your reconstructed path-lengths differ from the
target's, alignment slips and can't re-anchor across the long divergent paths, so the bulk is
scored as INSERT/DELETE pairs. Do NOT read the rollup as fidelity. Gauge the real carcass quality
from the per-instruction `diff_kind` histogram (count `MATCH` + `DIFF_ARG_MISMATCH` directly):

```
objdiff-cli diff … --format json   # then count diff_kind, don't trust the fuzzy rollup
```
WALL (scoring desync until byte-perfect path-length reconstruction). The deliverable for such a
function is the carcass (control flow + every offset + the ordered call sequence); keep the unit
`wip`, do NOT claim it matched. Evidence: CPlay::Render @0xc8cf0 (3092 B, 3-way SEH dispatch) —
~106 opcode-exact + ~184 reloc-arg-only instrs but fuzzy rolls up 0.0% (only ~23% of rows MATCH).
related: tu-completeness-rva-order.md, eh-state-numbering-base.md.
