# A hand-rotated loop merges the two exit epilogues retail keeps apart

tags: cpp:loop cpp:while cpp:for cpp:goto | asm:jcc asm:ret | topic:codegen-idiom
symptoms: the loop latch reads `jcc <exit>` + `jmp <top>` where retail has a single `jcc <top>` falling into the exit; the base has one FEWER `ret` than the target; the target's loop-exit return block omits an `xor eax,eax` that the base's shared one has; every block index after the latch is shifted
confidence: 9/10 for the ASM fingerprint; 1/10 for the SOURCE shape (see "The screen is the asm, never the source")

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

## The screen is the ASM, never the source (measured tree-wide, 2026-08-08)

A whole-tree sweep A/B'd this transform against objdiff on **every** site that matches it in
source, over the 754 sub-100% functions outside the units another lane held. The result is
decisive and inverts the obvious reading of this page:

| screen | sites | in a sub-100% fn | kept | rate |
|---|---|---|---|---|
| `if (C) { [init] do { B } while (C); }`, guard **textually equal** to the latch | 90 | 24 | **1** | 4% |
| `if (!C) { return R; } do { B } while (C); ... return R;` (the shape in "Cause") | 6 | 1 | **0** | 0% |
| BASE has a lone `jmp <top>` **and** TARGET has a `jcc <top> \| fall <ret>` latch (the asm fingerprint above) | 2 | 2 | **1** | 50% |

The 8 sites that scored *identically* either way, and the 15 that scored **worse** (down to
`_CellTargetable` 89.47 -> 73.22, `CellHitTest` 80.71 -> 71.17, `RebuildSelectionList`
85.78 -> 77.24, `ResolveCeilingCollision` 97.40 -> 90.26), say the same thing the five
already-EXACT sites do:

**Retail's own programmers wrote the guarded `do/while`.** These are all **100.00 EXACT**
carrying the exact shape this page's "Cause" section calls the bug:

- `CMapMgr::Find` 0x9f500 — `if (p == NULL) return 0; do { ... } while (p != NULL); return 0;`
- `CSpawnList::ClearFlags` 0x9a420, `CMenuState::StopMusicChain` 0xa0640 (both `void`)
- `CWormhole::SpawnPartners` 0x403b0, `CAttract::LeaveState` 0x14340
- `CDDrawChildGroup::RenderChildren` / `BltDirtyChildren` / `BltDirtyChildrenEx`,
  `CDDrawSubMgrLeafScan::RemoveKeysEqual` and its two twins — the MFC `POSITION` walk,
  `if (pos != NULL) { do { GetNext(pos); ... } while (pos != NULL); }`, verbatim

So **do not grep for the source shape**. Screen only on the asm fingerprint at the top of this
page, and only when BOTH halves hold at once:

```
BASE   has a block that is a lone unconditional BACK-jump   [jmp Bk^]
TARGET has a latch                                          [jcc Bk^ | fall Bm]  with Bm == [ret]
```

Tree-wide that pair fires on exactly **two** functions. One of them,
`CProjectile::ScanTargets` 0xe0b10, closed **95.37 -> 100.00 EXACT** on the `do` -> `while`
edit alone (its outer `do { ... } while (rowBase < 0x3c)` handed cl two identical `void`
epilogues, the loop exit and three early `return;`s, and cl cross-jumped them). The other,
`CTriggerMgr::PlaceObjectFull` 0x78a50 at 40.66%, diverges in 115-vs-131 blocks and needs a
re-read, not a loop edit.

### The variant that IS still worth grepping for

Not the exit merge — the **preheader**. When our source spells the guard by hand, cl emits the
init as its own 1-instruction preheader block; when retail's source is a plain `while`, cl peels
the guard and folds the init INTO the guard block. So `base N+1 blocks vs target N` with a lone
small `[fall Bk]` block ahead of the loop head is a real hit:

`CDDrawWorkerHost::InitGeometry` 0x1619f0's two `if (tileW > 1) { i32 v = tileW; do { v >>= 1;
m_shift++; } while (v > 1); }` log2 loops — **94.02 -> 95.89**, and all 11 blocks then agree
instruction-for-instruction. `WwdFile::GetMapBaseName` 0x3bb50's backslash scan — **83.29 ->
88.24**, the only keep out of the 24-site A/B.

### Trap

Deleting the guard and KEEPING the `do/while` is a different (and wrong) edit — the body then
runs once on the empty case. On `GetMapBaseName` that spelling scores **82.00** where the honest
`while` scores 88.24. The lever is promoting the test to the loop CONDITION, never deleting a
guard.

## Related

- [`retail-duplicates-small-return-epilogues`](retail-duplicates-small-return-epilogues.md)
- [`do-while-is-an-echo-write-while`](do-while-is-an-echo-write-while.md)
- [`allocate-check-then-body-is-the-then-block`](allocate-check-then-body-is-the-then-block.md)
  — the same merge symptom with NO loop, where no source spelling is known to fix it.
