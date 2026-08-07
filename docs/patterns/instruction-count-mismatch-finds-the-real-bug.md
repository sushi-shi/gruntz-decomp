# Diff the INSTRUCTION COUNT first: a count mismatch is a source bug, an equal count is regalloc

tags: cpp:expr | asm:mov | topic:method topic:wall
symptoms: a function in the high 80s/90s whose `sema disasm --diff` shows only register-name swaps and one-instruction transpositions, so it gets filed as a regalloc wall; nothing in the masked view says whether the two sides even have the same NUMBER of instructions
confidence: 10/10

Before reading a single diff line, count the instructions on both sides. The
count is a **hard invariant of the source shape** and it partitions the whole
worklist in one pass:

* **counts differ** — the source is WRONG (a missing reload, a cached local retail
  does not have, an un-sequenced comparison operand, a missing statement). There
  is a real fix and it is usually worth several points.
* **counts agree** — every remaining difference is register naming, operand-role
  choice or a schedule transposition. That is the documented regalloc/scheduling
  wall; `@early-stop` it and move on.

**Do not hand-roll the count — `python -m gruntz.audit.insn_count` is the whole
sweep** (`--summary`, `--unit X`, `--min/--max`, `--tsv`, `--eh`). It reads
`build/gen/residual_function_queue.tsv` and does the two subtractions that a hand
`llvm-objdump | wc -l` gets wrong:

* the COMDAT alignment `nop`s cl emits and the delinker does not — untrimmed they
  put spikes of 56/73/114 functions at exactly delta +4/+8/+12;
* the switch JUMP TABLE, which decodes linearly and DIFFERENTLY on the two sides
  (base entries are all-zero reloc slots, target entries hold real addresses).
  Found exactly, from the run of DIR32 relocations at a 4-byte stride — the
  "cut after the last `ret`" rule fails on `CTileActionEvent::Process`, whose
  table bytes happen to decode a `retl $0x0`.

Between them those accounted for 39 of the 304 mismatches a naive count reports,
including the two largest: `CSpriteRef::Build` (-33, really 275 vs 275) and
`CTileActionEvent::Process` (-40, really 261 vs 261). Both regalloc walls.

Use `llvm-objdump`, not `sema disasm --diff`: the latter truncates at the first
`$L` jump-table label, and the delinker packs a jump table INTO the owning
function's symbol on the target side while the base keeps it in separate `$L`
symbols — so a switch-heavy function shows a huge bogus count delta unless you
account for the split labels.

**Two things move the count that are NOT a missing construct.** A SPILL costs
`sub esp,N` + `add esp,N` + the store/reload pair, so a function that ran out of
registers on one side only reads as ±4 with no statement missing
(`CStatusBarMgr::UpdateFallingItemStatusBar` -4 is exactly this). And the /GX
EH frame is ~10 instructions of pure prologue/epilogue; `--eh` reports that side
mismatch separately so it does not masquerade as a body difference.

Measured on the `??0C*@@QAE@PAUCGameObject@@@Z` constructor family (61 partial):
26 had a count mismatch and every one investigated was a genuine source bug —
CTileTrigger/CBrickz cached `m_object` in a local where retail reloads it before
each of three stores (106 -> 108, exactly retail); seven candy ctors were one
instruction short per big-act comparison
([compare-lhs-materialized-before-its-call.md](compare-lhs-materialized-before-its-call.md)).
The 24 that already matched on count were all pure register rotation.

**A count fix can LOWER the current percent** — restoring CTileTrigger's reloads
took it 95.58 -> 92.90 because the extra instructions shift objdiff's alignment
onto a different register rotation. The count is the evidence; the percent is
not. Keep the change (MAX holds).

related: [walls-are-broken-not-documented](../gotchas.md),
[masked-diff-hides-branch-target.md](masked-diff-hides-branch-target.md) (the
same blind spot for control flow).
