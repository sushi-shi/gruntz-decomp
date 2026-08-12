# A backward `goto` sinks its whole target region to the end of the function
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
