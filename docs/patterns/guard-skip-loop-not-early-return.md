# A mid-function guard before a loop: write `if (cond) { loop }` (fall through to the single return), NOT `if (!cond) return acc;`

tags: cpp:branch cpp:return cpp:loop | asm:jcc asm:jmp asm:ret | topic:codegen-idiom
confidence: 8/10
variants: identical-return-epilogue-tailmerge.md, nested-if-success-deepest-error-tail.md, void-vs-bool-return-epilogue-split.md

## Symptom

A function accumulates a result, then has a guard that can skip the rest of the
work and return the accumulator-so-far, then a loop, then `return acc;`. Spelled as
an **early return**:

```cpp
int acc = 0;
... ;
if (lo > hi) {          // guard
    return acc;         // early-out
}
do { ... } while (...);
return acc;
```

the recompile emits a SECOND full pop/ret epilogue at the guard site, where retail
emits a single `jg <shared-exit>` that jumps to the one `mov eax,[acc]; pop…; ret`
tail. The body is otherwise byte-identical, but the duplicated epilogue + the
slightly different live-range around it drag a clean function down (~95%).

## Fix — invert the guard so it SKIPS the loop and falls through to the one return

```cpp
int acc = 0;
... ;
if (lo <= hi) {         // guard now GATES the work
    do { ... } while (...);
}
return acc;             // the SINGLE exit both paths reach
```

Now the guard is a `jg`/`jcc` over the loop to the shared `return acc;` — exactly
retail's `jg <shared-exit>`. No duplicate epilogue. The accumulator's live range
also matches (it stays in its [esp] slot / register across the skipped loop instead
of being finalized at two sites).

Evidence: `CWwdGrid::Query` (0x1918c0) 95.0%→99.7% on this single rewrite — the
early `if (colA > colB) return fired;` (which duplicated the
`mov eax,[esp+0x10]; pop…; ret 0x14` tail) became `if (colA <= colB) { <nested
loops> }` with the lone `return fired;` after. The residual 0.3% is pure
addressing-mode/schedule entropy (`lea ebp,[ecx*8+0]` vs `[8*ecx]`).

This is the mirror of identical-return-epilogue-tailmerge (where the recompile
*tail-merges* and that's a wall): here the recompile *duplicates* and the C
spelling controls it. Use it whenever a guard sits BEFORE a loop and returns a
running accumulator — gate the loop, don't early-return.
