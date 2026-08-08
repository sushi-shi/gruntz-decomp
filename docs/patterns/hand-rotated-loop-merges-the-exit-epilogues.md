# A hand-rotated loop merges the two exit epilogues retail keeps apart

tags: cpp:loop cpp:while cpp:for cpp:goto | asm:jcc asm:ret | topic:codegen-idiom
symptoms: the loop latch reads `jcc <exit>` + `jmp <top>` where retail has a single `jcc <top>` falling into the exit; the base has one FEWER `ret` than the target; the target's loop-exit return block omits an `xor eax,eax` that the base's shared one has; every block index after the latch is shifted
confidence: 9/10

## Symptom

`--blocks --diff --lite` shows the latch inverted and the exit block replaced by a jump:

```
  B12    2i [jcc B28 | fall B13]   !!   2i [jcc B10^ | fall B13]     <- retail's back-edge
  B13    1i [jmp B10^]             !!   6i [ret]                     <- retail's own exit copy
```

and the two `return 0` sites collapse to one epilogue that both reach.

## Cause

A loop written pre-rotated by hand —

```cpp
POSITION pos = list.GetHeadPosition();
if (pos == NULL) {
    return 0;                 // guard copy, written out
}
do {
    ...
    if (hit) goto found;
} while (pos != NULL);
return 0;                     // exit copy, written out
```

— hands cl **two textually identical `return 0` statements** and lets it cross-jump them into a
single epilogue. Retail's compiler was given the *un*-rotated loop and rotated it itself, which
emits a separate exit block per exit edge. Those copies are not identical, so nothing merges:
the fall-through copy knows `pos` is already 0 in `eax` and **omits the `xor eax,eax`** that the
guard copy needs.

## The fix

Write the loop the natural way and put the hit-body inside it:

```cpp
POSITION pos = m_recList.GetHeadPosition();
while (pos != NULL) {
    POSITION cur = pos;
    Coord* p = static_cast<Coord*>(m_recList.GetNext(pos));
    if (p->m_x == x && p->m_y == y) {
        ...
        return 1;
    }
}
return 0;
```

`CTriggerMgr::RemoveCellRecord` 0x78260: **84.63 -> 89.59**, latch and both exit copies exact.

The counted form is the same story — a `do { ... } while (i < 10)` over a known-nonzero trip
count still merges, because the trailing `return result` is a second written-out statement:

```cpp
for (i32 i = 0; i < 10; i++, list++) { ... }
return result;
```

`CTriggerMgr::SelectionListFind` 0x7d2a0: **84.58 -> 100.00 EXACT**. Note the increments: put
BOTH the counter and the cursor bump in the `for` clause. Leaving `list++` as the last statement
of the body emits `add ebx,0x1c` before `inc esi`; retail has `inc esi` first, and that two-
instruction swap was the whole remaining gap.

## Do not over-apply

The lever is the SOURCE loop form, not "epilogues should be duplicated". Where retail itself
has the guarded form — `test esi,esi / jle exit` then a preheader then a bottom-tested loop, as
in `CDDSurface::ShadeBlt` 0x13f020 — `if (n > 0) { do {...} while (--n != 0); }` is already
correct and rewriting it is a regression. Read `--blocks --lite --target` first: if retail's
latch is `jcc <top>` with the exit as its fall-through you have this bug; if retail has an
explicit top guard AND a preheader, you do not.

## Related

- [`retail-duplicates-small-return-epilogues`](retail-duplicates-small-return-epilogues.md)
- [`do-while-is-an-echo-write-while`](do-while-is-an-echo-write-while.md)
- [`allocate-check-then-body-is-the-then-block`](allocate-check-then-body-is-the-then-block.md)
  — the same merge symptom with NO loop, where no source spelling is known to fix it.
