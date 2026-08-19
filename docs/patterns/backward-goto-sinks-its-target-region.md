# A backward `goto` sinks its whole target region to the end of the function

> **2026-08-19 — scope correction.** The topological rule below describes the CFG
> entering layout. A backward edge in final retail code can instead be introduced by
> later value factoring, so it is not proof that retail source contained a cycle or
> `goto`. `StepArrivalDrop`'s emitted `pathGate` is acyclic; full-function loop and
> triplication controls fail the retail size/instruction census. See the corrected
> worked example in
> [`../relevations/wall-reasons-layout.md`](../relevations/wall-reasons-layout.md) §2.
>
> **2026-08-18 — the mechanism is named and this doc's EH half is refuted for the
> minimal shape.** cl 5.0 lays blocks out in TOPOLOGICAL order: a block is emitted
> immediately after its LAST predecessor, and only a real CYCLE exempts it. Measured
> matrix and minimal probe:
> [`../relevations/wall-reasons-layout.md`](../relevations/wall-reasons-layout.md) §2.
> In that probe `goto A` from B only gives **B A C**, not B C A — the region lands
> after its last predecessor, not at the end — and adding a destructible local (in
> region A or at function scope) does **not** move the order. Read that entry before
> re-deriving anything below.
tags: cpp:goto cpp:branch | asm:jmp asm:jcc | topic:wall
symptoms: two macro-regions of a big function appear in the OPPOSITE order to retail; retail's
early `jcc` reaches far forward past a `ret` while ours reaches a few bytes; the jump table's
arms sit at the very end of our object; score roughly halves on a function that is otherwise
block-for-block reconstructed
confidence: 8/10

For `if (cond) { A ... return; } B` where `B` later does `goto <label inside A>`, cl 5.0 moves
the ENTIRE `A` region below `B` so that every `goto` becomes a forward jump. Retail keeps `A`
inline and lets the gotos be backward jumps. The rotation re-encodes every branch displacement
in both regions and re-orders the instruction stream, so it is worth far more than its two
instructions.

```cpp
if (Probe(a, b) != 0) {
    if (Count()) Pop();
pathGate:              // <- target of the two gotos below
    /* region A ... */
    return 1;
}
/* region B */
if (nudged) goto pathGate;      // backward in retail, forward in ours
/* region C/D */
goto pathGate;                  // same
```

**The `goto` is only half of it: the other half is a /GX EH scope inside the target
region, and that half is decisive.** `StepArrivalDrop`'s region A contains a
`CPtrList probe(10)` - a destructible local, so a `__ehunwind` state range. Two
independent layout-only probes flip cl back to retail's order while leaving both
gotos in place: (a) delete the probe block, (b) keep every statement but give the
list a HEAP home (`CPtrList* p = new CPtrList(10)`) so no local is destructible.
Restore the stack local and A sinks again. So the rule is: *a backward `goto` into a
region that owns an EH state range sinks that region*; a backward `goto` into a plain
region does not have to. This does NOT yet give a legal fix - retail has the SAME
scope, the same size (state 0 at 0x4b651 through state -1 at 0x4b73f, 0xee bytes;
ours 0x962..0xa50, also 0xee), with its own `je 0x4b74e` jumping the whole scope - so
some third difference decides it. Refuted as that difference, one build each:
spelling the battlez arm `if (state == BATTLEZ) reinit = 0; else { CPtrList
probe(10); ... }` instead of `goto commitEntrance`; and hoisting the declaration to
FUNCTION scope so that no `goto` crosses a scope boundary at all. Neither moves the
ctor out of the sunk region, so it is not the scope's POSITION and not the crossing
- the mere presence of a destructible local in the function is enough to trigger it.

Measured on `CGrunt::StepArrivalDrop` @0x4b370: with both gotos present cl emits
prologue/B/C/D/A and scores 33.37%; with both replaced by a plain `return` (semantically wrong,
probe only) it emits prologue/A/B/C/D — retail's order — and scores 66.15%. Removing only ONE
of the two gotos does not flip it, so it is not a predecessor-count effect. Neither hoisting
region A to file scope with an explicit `goto nudgeStart`, nor wrapping A in a `for (;;)` and
turning the re-entries into `continue`, changes the decision (31.89% both). Wall so far — record
the residue, do not re-derive it.

After the original experiment, relocation-count analysis recovered three missing
scoped `CMapMgr*` locals (see
`global-reload-runs-prove-scoped-pointer-locals.md`). That structural correction
made the frame and all seven `g_gameReg` relocations agree and left the candidate
only eight bytes shorter than retail, but did not change the region order. A
64-trial parser-state request produced 56 legal states; every result stayed at
the linear scorer's 0.00 floor. The remaining referent-count difference is only
the known cross-jump copy (`RemoveHead` 3/4 and `g_coordPool` 18/21). This is new
evidence that the rotation is independent of the missing cache locals rather
than evidence against those locals.

2026-08-13, two more refuted levers plus the retail edge map, one build each:

- UN-NESTING region A (`if (SearchEdge(..) == 0) goto nudgeStart;` with A as
  top-level straight-line code, no if-block at all) still sinks A. The trigger
  is not the lexical region attached to the `if`.
- Spelling the AI_NONE bail as a forward `goto arrivalBail` with the block at
  the function end (retail's end placement, 0x4be5b) gets the target HOISTED to
  the goto site — the forward-goto-hoists behavior wins over end placement in
  our TU, so retail's end block does not come from that spelling here.
- Retail's complete backward-edge map into A (in-edges from the late regions):
  `0x4b4ff` (pathGate head) from a bare `jne` at nudgeDone plus a threaded
  `je`/`jmp` pair in reProbe; `0x4b605` (`xor eax,eax` return-0 tail) and
  `0x4b787`/`0x4b78c` (`mov eax,1`/epilogue) are ordinary cross-jump tail
  merges. These edges are compatible with the same two-goto structure, but do
  not prove it: an SCC walk shows that final retail `pathGate` cannot reach any
  late predecessor, so the edges may have been introduced by factoring after
  placement. The deciding input difference remains unfound; the
  declaration-probe panel (wall-break 2026-08-13) proves it is not reachable
  by parser-state handles. Untested residue hypothesis: TU body-set parity
  (a sibling body present/absent changes C2 layout state).
- Real structure recovered while mapping: both late commit tails (reCommit
  0x4bd6c, reProbe-fail 0x4be44) store `m_arrivalPhase` then return
  `arrivalPhase != 0` (test + branch into the shared `mov eax,1`; fallthrough
  reuses eax=0), NOT the unconditional `return 1` we had. cl if-converts every
  local spelling of this to a shared `setne` block, so the branch shape itself
  is layout-state residue, but the SEMANTICS (`return arrivalPhase != 0`) are
  retail ground truth and are kept.

2026-08-19 full-function controls close the two generic-lever guesses. A
`pathFound` loop (including variants with retail's `reinit` and `CoordCount` prefix
at its head) restores the 26-call/68-relocation census but remains path-last and
grows to `0xdd0`-`0xde8` through rotated scan-prefix copies. Three written-out path
regions grow to `0x1088` with three ctor/dtor pairs, 44 calls and 106 relocations;
the per-copy `CPtrList` EH states block the desired merge. Neither spelling models
retail. Keep the single real EH scope and do not retry those broad transformations.
