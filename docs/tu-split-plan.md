# Splitting the conflated TUs — plan

**Status: PLAN ONLY. Nothing here is applied.** Written 2026-08-03 from the cluster
census (`gruntz verify vtable-scan`-adjacent analysis over `build/gen/bindings.tsv` +
retail RVAs). The rule this rests on is measured, not assumed:

> The linker lays each object's `.text` down as **one contiguous run**. Measured on our
> own link: 94.7% of 646 objects perfectly contiguous, mean 1.07 fragments — *identical*
> under `/INCREMENTAL:YES` and `:NO`. So if one of our units' functions land in two
> distant places in retail, either the linker **folded** a body there (shared inline /
> template / `??` special, placed at first-referencer) or **our unit is two objects**.

After subtracting everything foldable (`??` specials, dtors, dynamic-init, and the
shared families the data names itself — `FireActivation` 8 classes, `RegisterActs` 6,
`AdvanceAnim` 5, `GetDisplayedValue` 3), **17 units** have own-class body code sitting in
a far cluster. Those are below.

## Do not split mechanically

The RVA gap tells you *that* a unit is two objects; it does not tell you *where the
seam is*. Cutting at the gap produces `Foo.cpp` + `Foo2.cpp`, which is a fossil, not a
reconstruction. Each split below has to end with a named TU whose contents are a
coherent responsibility — the same standard `docs/tu-partition-brief.md` applies to
every other partition decision. Where the seam is not yet known, this plan says so and
names the evidence to go get.

---

## Tier 1 — not splits: a stray body to re-home (6 units)

The minor cluster is **one or two functions**. A one-function "TU" is never right; these
are misattributions. Find the real owner by xref / vtable slot (`gruntz sema xref
--tree`), move the body there, and the unit becomes contiguous.

| unit | stray | lives at | main block |
|---|---|---|---|
| `ddrawsubmgr` | `SoundCueRegistry::PlayCueIfElapsed` | `0x114120` | `0x156cb0..0x1591d0` (118 fns) |
| `animationregistry` | `AnimationRegistry::FindAnimation` | `0x06b2a0` | `0x152640..0x152e30` |
| `wwdgameobject` | `CWwdGameObjectA::ApplyGeometryDirect` | `0x058b60` | `0x1504d0..0x1525c0` |
| `levelpreview` | `CPreviewState::LoadScreen` | `0x0fab90` | `0x0de030..0x0de590` |
| `gamelevel` | 2 strays (`0x06b330`, `0x082600`) | — | `0x15ccd0..0x1614e0` (73 fns) |
| `ddrawsurfacepair` | 2 strays (`0x03a1d0`, `0x06b270`) | — | `0x163bc0..0x1660b0` |

**`0x06b2a0` and `0x06b330` are 0x90 apart** — `animationregistry`'s stray and one of
`gamelevel`'s sit side by side. This was first read as an unidentified third object;
**that is wrong**. The region is fully attributed — five functions, five *different*
units, packed back to back with a 0x2e6 gap after:

```
0x06b260   5B  gruntentrancemove      CGrunt::StepAttackAction
0x06b270  27B  ddrawsurfacepair       CAniElement::AtChecked
0x06b2a0  35B  animationregistry       AnimationRegistry::FindAnimation
0x06b2e0  57B  gruntentrancearrival   CWapX::ApplyAnimation
0x06b330  42B  gamelevel              CGameLevel::PointInBounds
```

That is a pooled band — small bodies from many TUs placed together — not a compiland.

### Do NOT try to fix these by moving the body to a header — measured, it fails

The obvious hypothesis is that a body landing in a pooled band was a header inline in the
original, folded to its first referencer. **Tested; it does not hold for any of these.**

* `AnimationRegistry::FindAnimation` (35 B) moved into its header (24 includers) was emitted
  in **0** base objs — MSVC inlines a trivial non-virtual away entirely — while the target
  still defines it, so it became an unmatched target symbol. Reverted.
* Of the **110** bodies already defined in `include/`, **101 are virtual** (the vtable
  forces emission) and the remaining **9 all return `CString` by value** (an NRV temporary
  MSVC5 will not elide). Those are the only two shapes that keep a header body emitted.
  **All ten strays here are non-virtual returning `int`/`void`/pointer** — neither shape.
* Size is not the competing explanation: the largest, `CPreviewState::LoadScreen` (170 B),
  will not even compile in a header — it needs `IMGTAG_XCP`, `CRezArchiveEntry` and
  `m_world->m_drawTarget->LoadPageImage`, i.e. half the engine hoisted into a header.

So these strays are **mis-homed**, and re-homing by xref is the only route open.

`gamelevel` is the priority: its `/GR` verdict is currently **contaminated** by the
conflation (it emits 6 RTTI descriptors retail lacks), so it is parked on `cpp` and
cannot be scoped correctly until this is done.

## Tier 2 — genuine two-object splits (8 units)

Both clusters carry real bodies. Each needs a semantic boundary, not an address cut.

| unit | cluster A | cluster B | likely seam |
|---|---|---|---|
| `cimage` | `0x152e90` — CImage×20, pure image ops | `0x0d5c10` — CImage×3, CWapObj×2, CState×1, free×2 | core bitmap vs **load/state glue** — B mixes in `CWapObj`/`CState`, a different layer |
| `gameapp` | `0x13d590` — CGameApp×12, CGameMgr×8 | `0x080cf0` — CGameApp×3, free×2 | app/manager core vs a small app-side helper obj |
| `gamewnd` | `0x13cf00` — CGameWnd×8 | `0x094c10` — CGameWnd×5 | same class twice — creation/registration vs message handling |
| `secretteleportertrigger` | `0x010a10` — free×5, CSecretLevelTrigger×2, CSecretTeleporterTrigger×1 | `0x041e90` — CSecretTeleporterTrigger×3, CSecretLevelTrigger×3, free×2 | both clusters mix both classes → seam is **behavioural**, not per-class |
| `ufo` | `0x0b4330` — CUFO×2, CPathHazard×1 | `0x013230` — CPathHazard×1, CUFO×1 | as above; both mix |
| `sbi_wellgoo` | `0x0e6020` — CSBI_WellGoo×4 | `0x104b80` — free×2, CSBI_WellGoo×1 | status-bar item vs its free helpers |
| `menustate` | `0x09fe50` — CMenuState×12 | `0x08ce10` — CMenuState×1, free×1 | small tail obj |
| `attractstate` | `0x013fb0` — CAttract×10 | `0x08cd40` — free×2, CAttract×1 | small tail obj |

`menustate`/`attractstate`/`sbi_wellgoo` sit on the Tier-1/Tier-2 boundary — 2–3
functions in B. Decide by whether B's free functions are a coherent helper set (split) or
orphans of another object (re-home).

## Tier 3 — investigate before proposing anything (3 units)

- **`play`** — 111 fns at `0x0c8700..0x0ddaa0`, 4 at `0x083260..0x08c970`. Also **20
  intra-order violations**. The B cluster spans 0x9000, so it is not a stray; it is a
  real object we have folded in. Biggest single unit in the tree, so do it last.
- **`wwdfile`** — 3 functions, 3 clusters, and *all three* are `CDDrawWorkerHost`, which
  is not this file's class at all. Probably not a split: the whole unit may be mis-homed.
  Check ownership before touching layout.
- **`motionstate`** — 3 fns at `0x16ecd0` vs 2 at `0x058bc0`, all `CMotionState`. Small
  enough to resolve by disassembly directly.

---

## Method, per split

1. **Prove the seam semantically.** Read both clusters with `gruntz sema disasm --blocks`.
   The new TU needs a one-sentence responsibility. If you cannot write that sentence, you
   do not yet know where the seam is — keep reading, do not cut.
2. **Name it for what it does**, matching the neighbours' convention. Never `<unit>2`.
3. **Move every body** — a half-migrated TU breaks the contiguity invariant worse than
   the conflation did (`docs/tu-partition-brief.md`: re-home breaks until the TU is
   complete).
4. **Register the new unit** in `config/units.toml` (profile `cpp`, or `cpp-rtti` if it
   is a Gruntz-project TU — check against the RTTI descriptor oracle, below).
5. **Verify, in this order:**
   - `gruntz verify tu-order` — the new unit must be contiguous, and the
     interleaving-pair count must go **down**;
   - the RTTI descriptor oracle (diff `.?AV`/`.?AU` in `.data`, candidate vs retail) —
     must stay at 2 extra / 1 missing or improve. For `gamelevel` this is the *point*:
     the split should let one half take `/GR` and drop its 6 spurious descriptors;
   - `gruntz build` — gate on BUILD, not on current %. A dip is expected and is not a
     reason to revert (MAX preserves best-ever).

## Sequence

1. Tier-1 strays, by xref (cheap, low risk, each removes a row). Not header moves —
   see the measured negative above.
2. `gamelevel` — unblocks its `/GR` scope.
4. Tier-2, smallest first (`motionstate`, `attractstate`, `menustate`, `sbi_wellgoo`).
5. `cimage`, `gameapp`, `gamewnd` — clearest semantic seams of the larger ones.
6. `secretteleportertrigger`, `ufo` — need a behavioural seam, hardest to name.
7. `play` last.
