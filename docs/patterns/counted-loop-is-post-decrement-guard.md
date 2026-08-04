# A counted loop whose guard materializes `n-1` is `while (n-- > 0)`

**Symptom.** The loop body is instruction-for-instruction right, but the loop
ENTRY differs by two instructions and every stack offset in the function is
shifted:

```
retail   mov  esi,edx        ; copy of n
         dec  edx            ; n-1  -- apparently dead
         test esi,esi
         jle  <skip>
         ...
         lea  edi,[edx+1]    ; trip count = (n-1)+1
base     test edx,edx
         jle  <skip>
         ...
         mov  edi,edx        ; trip count = n
```

The `dec` looks pointless (nothing reads `n-1`) and the `lea +1` looks like it
undoes it, so it is easy to file the pair as scheduling noise. It is not: it is
the signature of the loop's induction variable being the *post-decrement* of the
guard expression.

**Cause.** cl5 lowers the guard and the induction of a counted loop from the
same expression node. The four spellings are NOT interchangeable:

| source | guard | trip count |
|---|---|---|
| `for (i = n; i > 0; i--)` | `test n; jle` | `n` |
| `for (i = 0; i < n; i++)` | `test n; jle` | `n` |
| `for (i = n - 1; i >= 0; i--)` | `dec; test(n-1); jl` | `(n-1)+1` |
| `while (n-- > 0)` | copy, `dec`, `test(copy); jle` | `(n-1)+1` |
| `while (n--)` | copy, `dec`, `test(copy); je` | `(n-1)+1` |

Only the two post-decrement forms materialize `n-1` *and* keep the guard on the
pre-decrement value; `> 0` gives the signed `jle`, the bare `n--` gives `je`.
The decrement survives even though `n` is dead afterwards, because it IS the
induction variable — writing `i--` in the third clause lets cl fold it away and
count from `n` directly.

**Fix.** Read the guard's jcc and whether `n-1` is materialized, then pick the
row above. `n` is normally the function's own count parameter or a local copy
of it, decremented in place — no separate index variable exists in retail, so
the `i32 i;` at the top of the function goes too (it is a spill slot that shifts
every later offset).

**Evidence.** `CDDrawShadeBlit::ConvertRow` 66.7 -> 76.0, `ConvertRowFlip`
62.7 -> 67.7, and the same idiom in the row loops inlined into
`BlitShadedMirrored` 44.0 -> 52.6 / `BlitShadedForward` 51.6 -> 53.2 and
`BlitCopyMirrored` 40.6 -> 42.4. The `je` variant is the one already recorded
for the `m_grown` teardown loop in
[act-registrar-counter-cse-and-freeloop.md](act-registrar-counter-cse-and-freeloop.md);
`CGrunt::FinalizeStep`'s inlined `GruntPosScratchTeardown` needed it too.

**Trap.** The guard alone is not enough to identify the spelling — `i > 0` and
`i < n` produce the *same* entry code. The discriminator is whether `n-1` is
computed before the test.
