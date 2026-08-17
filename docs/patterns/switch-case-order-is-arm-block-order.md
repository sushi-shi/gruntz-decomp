# Switch arm-block layout IS the source case order - and grouped cases change the dispatch

tags: cpp:switch | asm:jmp asm:mov | topic:codegen-idiom
symptoms: a big switch scores mid-90s with size/branches/relocs exact but the per-arm
store immediates appear in a PERMUTED order vs retail (extract both sides'
`mov $imm,(reg)` sequences and compare); OR the reloc count collapses (54 -> 28) and
the xlat+table dispatch disappears after "cleaning up" duplicate case bodies into
shared `case A: case B:` groups
confidence: 9/10

Two facts, measured on `CMapMgr::ComputeCellFlags` 0x77790 (50 cases, 18 unique
bodies, two-level xlat byte-map + 19-entry table):

1. **cl 5.0 emits one arm block per unique body, placed in SOURCE ORDER of the case
   that carries the body** (for per-case duplicate bodies, the LAST case of each
   merged group). Retail's arm order is therefore the retail case order - read the
   `mov $imm` sequence off the target and reorder the source cases to match. Here the
   retail order was thematic (water group, hazards, pyramids, rocks, bricks, ...) and
   nothing like the enum's ascending order. 97.10 -> 97.59 with the jump table + xlat
   map bytes snapping.

2. **Collapsing per-case duplicate bodies into shared-body case groups is NOT
   codegen-neutral**: with `case A: case B: ... body` groups cl abandoned the
   xlat+table for a compare-chain dispatch (relocs 54 -> 28, size 0x4ea -> 0x47a,
   +2 branches). Retail's 50 per-case duplicate one-store bodies are load-bearing:
   keep `case X: cell->m_flags = K; break;` per case even when K repeats.

Screen: dump both sides' arm stores in address order
(`movl $imm,(%esi)` regex over llvm-objdump) and diff the two value sequences.
