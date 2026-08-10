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

Measured on `CGrunt::StepArrivalDrop` @0x4b370: with both gotos present cl emits
prologue/B/C/D/A and scores 33.37%; with both replaced by a plain `return` (semantically wrong,
probe only) it emits prologue/A/B/C/D — retail's order — and scores 66.15%. Removing only ONE
of the two gotos does not flip it, so it is not a predecessor-count effect. Neither hoisting
region A to file scope with an explicit `goto nudgeStart`, nor wrapping A in a `for (;;)` and
turning the re-entries into `continue`, changes the decision (31.89% both). Wall so far — record
the residue, do not re-derive it.
