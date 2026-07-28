# Retail's `sub`/`add` pair that cancels algebraically means the source DERIVED one value from the other

tags: cpp:local cpp:int | asm:add asm:sub asm:lea asm:inc | topic:codegen-idiom topic:mis-model
symptoms: retail computes a value in 3 steps whose constants cancel (`lea eax,[b+c+1]` …
`sub eax,c` … `add eax,d`, or `sub eax,edx; dec eax` after an `inc`) while your compile
emits the folded 1-2 step form; an extra epilogue in retail that your build tail-merged
away; one fewer `return` block on your side
confidence: 9/10

cl5 folds `(y + 1) - b - 1` to `y - b` and `(top + cur + 1) - cur + sy` to
`top + 1 + sy` **inside one expression**. Retail's un-folded 3-step form therefore proves
the source did NOT write one expression: it wrote a NAMED value and derived the second
FROM it. The tell is doubly useful because a folded pair also lets cl tail-merge two
`return` blocks that retail keeps separate.

```cpp
// before - cl folds; the two returns then tail-merge into one epilogue
i32 want = (t->m_extent.bottom + cursor + 1) - cursor + t->m_screenY;
SpanCheck(a1, want, t->m_extent.bottom + cursor + 1, &probe);

// after - `top` is a value, `want` is derived from it (retail's sub/add pair)
i32 top = t->m_extent.bottom + cursor + 1;
SpanCheck(a1, top - cursor + t->m_screenY, top, &probe);
```

The mirror case is a cursor bump: retail's `inc eax; …; sub eax,bottom; dec eax` is
`++cur;` as a STATEMENT followed by `cur - bottom - 1`, not `(cur + 1) - bottom - 1`
(which folds to `cur - bottom`).

```asm
base:   lea eax,[esi+0x1] | mov edx,[ecx+0x140] | sub eax,edx              ; folded
target: lea eax,[esi+0x1] | mov edx,[ecx+0x140] | sub eax,edx | dec eax    ; not folded
```

STEERABLE where the intermediate is a real local; NOT steerable when the fold happens
inside a single expression (parenthesising `(y+1) - (b+1)` still folds). Evidence
(2026-07-28, `src/Gruntz/GameLevel.cpp`): `MoveHandlerD` 83.00 -> 96.33 (the SpanCheck
span floor), `ResolveMoveDown` 83.49 -> **100 EXACT** (span floor + `++cur`),
`AltStepValidate` 71.97 -> ~90 (`cmpHi = tHi - a2 + sy`, which took the whole body to
instruction-for-instruction identity). `AdvanceB` @0x15ede0 is the un-steerable half:
its two `return`s differ only by the fold, so cl merges them and retail's third epilogue
is unreachable from any spelling tried.
