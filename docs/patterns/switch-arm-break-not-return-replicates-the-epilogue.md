# A switch arm ending `break;` gets its epilogue replicated; one ending `return <const>;` gets merged
tags: cpp:switch cpp:branch cpp:return | asm:ret asm:jmp asm:jcc | topic:codegen-idiom
symptoms: a big `switch` whose arms all end `return <same const>;`, base short by N-1 copies of the 5-6 instruction epilogue, `gruntz walls diagnose` reports fewer rets on the base with identical branch counts, every arm body byte-identical
confidence: 9/10
variants: retail-duplicates-small-return-epilogues.md, switch-arm-tail-crossjump-vs-duplicate.md, single-predecessor-tail-block-gets-replicated.md

cl 5.0 /O2 runs two passes that move exits in opposite directions:

* an early **cross-jump / tail-merge** over source-identical `return` statements,
  which fires BEFORE layout and collapses every arm onto one far epilogue;
* a late **layout replication** that copies a small `return`-terminated join block
  back into its predecessors.

Which one you get is decided by how the arm ENDS in the source. `return <const>;`
in every arm feeds the first pass — the returns are statement-identical, they merge,
the join now has N predecessors and the replicator declines. `break;` in every arm
plus ONE trailing `return <const>;` after the switch never presents the first pass
with anything to merge, and the layout pass then replicates the trailing block into
each arm, which is retail's shape.

```cpp
// BASE: cl merges all 15 arms onto one epilogue - 88.53
case SBICMD_TAB_STATZ:
    if (m_hlBusy) { return 1; }
    m_tabSprite0->SetState(state, 1);
    ...
    return 1;                      // <-- statement-identical in every arm
...
}
return 1;

// TARGET shape: each arm carries its own `mov eax,1; pop...; ret 8` - 100.00 EXACT
case SBICMD_TAB_STATZ:
    if (m_hlBusy) { return 1; }    // an early-exit guard INSIDE an arm keeps its return
    m_tabSprite0->SetState(state, 1);
    ...
    break;                         // <-- every arm
...
}
return 1;
```

`CStatusBarMgr::SetTabState` 0x100d70 **88.53 -> 100.00 EXACT** on that edit alone
(15 arms). The guards inside the arms keep their own `return 1;` — they are not
arm-terminal, they are not statement-identical to the arm tail, and converting them
too is not needed.

## When it does NOT apply - the discriminator, measured 2026-08-08

The lever only reaches sites whose ENTIRE merged content is the epilogue. Two
near-neighbours in the same family were re-measured and neither moves:

* `CTriggerMgr::ResetGroup` 0x79520 — arms end `Activate(...); return 1;` with a
  DIFFERENT literal argument per arm. Break form is **byte-neutral** (90.7352 before
  and after): cl had already merged on the shared `Activate` suffix, which the
  break form does not touch.
* `CDDrawSurfaceChildA::SetGeometry` 0x1644a0 — arms each store a different
  `WORLDERR_*` and return. Break form **91.37 -> 69.81**: retail's arms return
  directly, so routing them through the trailing block is the wrong shape as well
  as a worse score.

`SetGeometry`'s real residue is not a merge-policy difference at all. Retail emits
the `WORLDERR_CREATE_DEVICE` block TWICE — once for the switch default and once for
the `m_lastError == 0` else — and the two copies differ in exactly one byte:

```asm
1645b4: 8b 76 0c   mov esi,[esi+0xc]
1645b7: 8b 46 38   mov eax,[esi+0x38]
1645ba: 85 c0      test eax,eax
1645bc: 75 74      jne 0x164632          ; <-- displacement 0x74
1645be: c7 46 38 b9 0b 00 00  mov [esi+0x38],0xbb9
        ... identical ...
1645ce: 8b 76 0c   mov esi,[esi+0xc]
1645d1: 8b 46 38   mov eax,[esi+0x38]
1645d4: 85 c0      test eax,eax
1645d6: 75 5a      jne 0x164632          ; <-- displacement 0x5a
1645d8: c7 46 38 b9 0b 00 00  mov [esi+0x38],0xbb9
```

cl's cross-jumper compares ENCODED BYTES, so a pair of semantically identical exit
blocks merges or not depending on whether their branch displacements happen to
encode the same. Retail's did not; ours did. That is an accident of layout with no
source spelling behind it — do not chase it, and do not read it as a missing
statement. The same accident is visible inside
`CStatusBarMgr::UpdateStatusBarTabHighlight` 0x000fe910, where retail keeps the
DIALOG_SECONDARY and DIALOG_YES `else` arms apart only because one reaches its
shared tail through a near `0f 84` and the other through a short `74`.

**Screen for the lever, not the family:** the arms must end in the same
`return <const>;` and contain no other shared suffix. Otherwise you are in
`switch-arm-tail-crossjump-vs-duplicate.md` (periodic arm deficit driven by the
register rotation), which this does not fix.

## The precondition is NECESSARY but NOT SUFFICIENT - full sweep, 2026-08-08

A tree-wide sweep settles this: the recipe is a **one-off** until a second
positive exists. Screened all 830 sub-100% functions in two stages - parse every
`RVA()`-pinned body, split each `switch` into top-level arms, classify each arm's
terminator (**19 sites** have >=2 arms ending in one identical `return <const>;`,
`default` excluded from the identity test), then test legality: the recipe only
preserves semantics when the switch's fall-out is itself `return <same const>;`.
**6 of 19 are legal. All 6 were applied and measured. None improved** - three
byte-neutral, three worse.

| fn | rva | before -> after |
|---|---|---|
| `CGrunt::ScanNearestTarget` | 0xf42f0 | 68.28 -> 67.73 |
| `CProjectile::SerializeMove` | 0xe0d40 | byte-neutral |
| `CStatusBarMgr::LoadTabSprites` | 0x102250 | 76.12 -> 73.75 |
| `CSBI_WellGoo::SerializeFields` | 0xe64c0 | byte-neutral |
| `CMulti::DispatchRecvMsg` | 0xb9750 | 99.05 -> 98.21 |
| `CPlay::OnKeyDown` | 0xcbcc0 | byte-neutral |

The other 13 are **illegal**: the arm returns deliberately bypass real
post-switch work (a `goto` label - `seek:`/`resetState:`/`reportError:`/
`timeout:` - a differing `return 0;`, or a whole trailing block).
`CGrunt::LoadGruntTypeTable` 0x4dd50 is the clearest, its 17 arm-returns skipping
the `BuildAssetNamespacePrefixes`/`ReadConfigFromButeMgr` tail. This is also why
`CTriggerMgr::ResetGroup` measured byte-neutral: a `reportError:` label sits after
its switch.

**Two things this retires.**

1. **"Looks like the exemplar" is not a predictor.** `CPlay::OnKeyDown`'s numpad
   switch is *shape-identical* to `SetTabState` - 13 arms, one call per arm, 12
   arm-terminal `return 1;` plus one already-`break;`, trailing `return 1;` - and
   converting all 12 is byte-identical output.
2. **Exit-count parity is not a proxy for bytes.** `ScanNearestTarget`'s break
   form fired the layout replicator and took its ret count 12 -> 13, *exactly*
   matching retail's 13, while the score went DOWN. `LoadTabSprites` moved 17 ->
   14 *toward* retail's 13 and lost 2.37.

The OVER+LEGAL screen has produced **zero** positives in three attempts. It has
ruling-out power only; no positive predictive power. The 39-row OVER-MERGE bucket
is not reachable by this lever at all - only 5 of its rows contain a qualifying
switch.
