# Screening jcc_sieve POLARITY rows: same destination = predicate bug, different KIND of destination = layout

tags: cpp:branch cpp:return | asm:jcc asm:jmp | topic:tooling topic:codegen-idiom
symptoms: `jcc_sieve` reports a POLARITY row (base `jne` where retail has `je`), but the source
predicate reads correct and the two sides are semantically identical
confidence: 9/10

## The claim

`jcc_sieve` compares mnemonics positionally, so it flags two very different things under one label:

1. **A real inverted predicate** - the guarded code runs on the opposite condition.
2. **An epilogue/block-placement difference** - identical semantics, but one side reaches a
   `return` through a *shared* exit and the other through a *local copy*, which flips the
   mnemonic of the branch that gets there.

(2) is common enough to dominate the board. Screen with the branch TARGETS, which the row already
prints:

| both sides' branch goes to... | verdict |
|---|---|
| the **same logical destination** - same block role, and for a skip, the same distance | **predicate bug** - open it |
| destinations of **different kind** - one short/local, one far/shared exit | layout - the tail-merge family |

The second column of a row (`rets N->M`) corroborates: unequal ret counts, or `DUP-EXIT`, is
almost always (2). But equal ret counts do NOT imply (1) - see `RouteUnitTo` below.

## Measured, 2026-08-01 (seven POLARITY rows opened)

**Real predicate bugs - both found by the same-destination test.**
`CBattlezMapConfig::ClaimCellFromRow` @0x30730 (rets 5->5), two of them:

* `#5` - both sides jump to **blk6**, ours on `jne`, retail on `je`. Retail 0x307ae is
  `cmp ecx,edx / je 0x307bc`, so it skips the `return 0` when the arrival column MATCHES
  `m_curCell`; we returned 0 when it matched. Inverted: `if (sx != m_curCell) return 0;`.
* `#17` - both sides skip the **same 2-byte `xor ebp,ebp`**, ours on `jle`, retail on `jg`. So
  retail clears `ok` when the squared distance is `<= 0x19` - the candidate is rejected for being
  too CLOSE to its level-record marker, not too far. (The dossier comment asserted the opposite;
  a sibling site in the same unit already tested `<= 0x190`, which corroborates.)

88.51 -> 89.35, and the function's branch sequence now AGREES.

**Layout, not predicates** - every one of these has the flipped branches going to destinations of
different kind:

| function | row | what it really is |
|---|---|---|
| `CProjectile::ScanTargets` 0xe0b10 | `#14 jge->jl` | loop back-edge; retail's epilogue A sits after the loop so `jl top` falls through to it, ours is tail-merged far away. Three spellings already measured byte-identical |
| `CBattlezMapConfig::StepRowSpawn` 0x26470 | `#9 jge->jl` | same loop-exit shape (rets 4->5) |
| `CMulti::SetupTcpIpConfig` 0xbc460 | `#3 jne->je` | retail materializes `!ok` (`neg/sbb/inc`) and tests the byte; we compare `ok` against a pinned zero |
| `CDDSurface::SaveTga` 0x144900 | `#1`, `#4` | zero-register-pinning, MIRRORED - retail spells the per-gate form at `#1` and folds at `#4`, we do the exact opposite |
| `CGrunt::PathScan` 0x57db0 | `#46 jne->je` | two `Clip(0); return 0` guards; retail routes both to the shared end block, we emit one local copy |
| `CDDrawSubMgrPages::TransEnter` 0x158e40 | `#0 jne->je` | DUP-EXIT: retail shares one `xor eax,eax / pop / ret`, we emit two (the second needs no `xor` because eax is already 0) |
| `CBattlezMapConfig::RouteUnitTo` 0x300c0 | `#1,2,3,5 jne->je` | rets 1->1 yet still layout - the early bail reaches retail's shared exit and our local `~CPtrList; return 0` copy |

`RouteUnitTo` is the one that proves ret-count equality is not sufficient on its own: four pure
`jne->je` flips with equal ret counts, all layout.

## Corollary: read the whole row, not the flagged index

The sieve names targets by branch INDEX and only reports MNEMONIC flips, so a branch whose
mnemonic matches but whose **target** differs is not flagged at all. On `PathScan` the reported
`#46` was the small half - the unreported `#44` (`je` on both sides) went to the far exit in
retail and to a short local block in ours. Always print both sides' full branch lists
(`gruntz walls diagnose <rva> --target` / `--base`) before concluding.
