# Count the relocation MULTISET before you read its ordered diff

tags: topic:method topic:triage topic:mis-model | asm:call | cpp:call
symptoms: `insn_seq --seq` reports the two sides with the SAME number of
relocations and narrates the difference as "these calls moved" / "this group is
transposed"; the function looks like a pure block-placement job; hand-reordering
the source statements changes nothing
confidence: 10/10

`insn_seq --seq` difflibs the two ordered `(mnemonic, symbol)` lists. difflib
must produce an *alignment*, so when we call the WRONG FUNCTION at a site it has
no choice but to pair our extra `A` against retail's extra `B` somewhere and
report a move. The story it tells ("Retune/TickVolumeRamps sit ahead of
VisitVisible in retail, after it here") is then true *and* useless: reordering
the source cannot fix a callee that does not exist in retail's list at all.

**Count first.** A `collections.Counter` over each side's relocation symbols is
order-free, so it answers a different and stronger question - *does retail even
call this?*

```python
tgt  = [reloc symbols of build/objdiff/target/<unit>.c.obj, in order]
base = [reloc symbols of build/objdiff/base/<unit>.obj,     in order]
from collections import Counter
ct, cb = Counter(tgt), Counter(base)
for k in sorted(set(ct) | set(cb)):
    if ct[k] != cb[k]:
        print(f"{k:70} tgt={ct[k]} base={cb[k]}")
```

On `CPlay::Render` (0xc8cf0) the ordered diff had been read as a block-placement
problem for several sessions. The multiset closed it in one line:

    ?LoadDestructButtonSprite@CStatusBarMgr@@QAEHH@Z    tgt=0 base=3
    ?LoadMainStatusBarSprite@CStatusBarMgr@@QAEHXZ      tgt=4 base=1
    ?FilterList2@CTileTriggerContainer@@QAEHH@Z         tgt=2 base=4
    ?AdvanceCursorAnimation@CPlay@@QAEHH@Z                        tgt=3 base=1

A `tgt=0` row is decisive: retail never calls it, so every one of our sites is
wrong, and a complementary pair (`0/3` against `4/1`) names the replacement for
free. `_g_frameDelta tgt=11 base=13` fell out of the same swap - the wrong
callee took an argument the right one does not.

Read the rows in this order:
1. **`tgt=0` / `base=0`** - a callee that exists on one side only. Wrong callee,
   or a whole statement missing. Fix before anything else.
2. **complementary pairs** summing equal - a rename/overload swap, as above.
3. **off-by-one on a repeated callee** - one duplicated arm cl cross-jumped, or
   one arm retail duplicates and we share
   (`identical-arms-need-distinct-locals.md`).
4. **only then** the ordered diff, for genuine block placement.

Measured this run: CPlay::Render 71.84 -> 76.06 (three wrong callees + two
more), CGrunt::UpdateArrival's tail (`SetEntrancePos` where retail calls
`CTriggerMgr::ApplyTriggerA`), CBattlezMapConfig::ResolveArrival 56.73 -> 82.34
(the `+ SearchEdge / + TileSwitch / + AddTail` runs the counts exposed).
