# LINK 5.10 `.text` layout: how the linker orders, splits, and never splits a TU

Reverse-engineered from the toolchain's own `link.exe` (5.10.7303, the retail
linker, Ghidra on `$MSVC_DIR/bin/link.exe`, image base 0x400000) and validated
by probes linked with that exact binary under wine. Companion to
`docs/compiler-data-layout.md` (cl's side of the same question) and the reason
`gruntz verify tu-order` now models one legitimate exception instead of
failing on 80 interleaving TU-pairs.

**The verdict up front: the one-contiguous-ascending-block invariant SURVIVES.**
Nothing in link 5.10 splits or reorders an obj's `.text` contribution within its
`$`-group. What looked like counter-evidence is one precise, provable exception —
a **multi-defined COMDAT is kept at the FIRST obj on the link line that defines
it** — plus our own partition defects. The 80 pairs decompose exactly: **47
legitimate (kept-COMDAT exiles, now modeled by the gate), 24 partition defects
(4 fixed in this change), 9 undecided with the deciding evidence named.**

## Architecture: PSEC / PGRP / CON (read from link.exe)

The linker's layout model is three levels, and all three are visible in the
binary:

* **CON** (contribution) — one record per input section. Created by
  `FUN_0040e850` (0x40e850): allocates the record, then **appends it at the tail
  of its group's linked list** (`grp[7]`=head, `grp[8]`=tail, `tail->next=new` —
  the function's last block). *Append-at-tail is the whole ordering story:
  contributions are laid out in arrival order.*
* **PGRP** (group) — one per distinct **full** section name (`.text`,
  `.text$AFX_CORE1`, `.text$x`). Find-or-create in `FUN_0040e700` (0x40e700):
  on create, the group is inserted into its section's group list by **ascending
  byte-wise `strcmp` of the full name** (the unrolled two-byte compare loop at
  0x40e77d, insert-before on the first `new < existing`). *Group order is name
  order; the empty suffix sorts first.*
* **PSEC** (output section) — one per **base** name. `FUN_0040e5e0` (0x40e5e0)
  copies the name into a static buffer (`DAT_00467560`) and **truncates at
  `'$'`** (`strchr(buf, 0x24)`); `FUN_0040ef90` finds-or-creates the PSEC keyed
  on that base name. *The `$`-suffix is an ordering key inside the section, gone
  from the image.*

The Pass-1 driver is `FUN_00442f50` (0x442f50; it sets the phase string
`"Pass1"`). The incremental-link thunk table is created by `FUN_00451940`
(0x451940) as a CON named **`.text$_glue`** (global `DAT_00494574`), sized
`n*0x18+0x14` or `n*8+0x28` depending on mode plus up to `0xfff0` growth slack —
and it is placed at the **head** of `.text` by the incremental writer, *despite*
`$_glue` sorting after the empty suffix: the observed image (probe and retail
both) puts the E9-thunk band first, so the glue CON's placement is
special-cased, not name-sorted.

So the layout law for `.text` is:

    [.text$_glue thunk band]           (incremental link only, special-cased)
    for each group, ascending by name:  ".text" < ".text$AFX_*" < ".text$x"
        for each CON in arrival order:
            command-line objs, in command-line order;   then
            library members, in pull (resolution) order
    each obj's CONs inside one group: obj section-table order = cl emission
    order = source order

## Probe record (all linked with the real link.exe under wine)

Probe sources/harness: this session's scratchpad `linkprobe/` (three TUs `a,b,c`
with a shared header-inline pair, forced out-of-line by address-taking; flags
`/O2 /MT /GX`; link `/INCREMENTAL:YES /OPT:NOREF /OPT:NOICF /NODEFAULTLIB`).

| probe | question | result |
|---|---|---|
| `b a c` vs `c a b` | contribution order | map order follows the command line exactly; each obj one contiguous run, functions in file order |
| shared inline in all 3 TUs | duplicate-COMDAT selection | **kept from the FIRST obj on the link line, discarded from the rest; the kept copy sits INSIDE the keeper's contiguous run** (moves from `b.obj` to `c.obj` when the line is reversed) |
| `/INCREMENTAL:NO` | does ilink change order | no — identical order, only padding (each contribution 16-aligned + slack under ilink; packed without) |
| `#pragma code_seg(".text$B"/".text$A")` in 2 TUs | `$`-group ordering | `.text` (no suffix) < `.text$A` < `.text$B`; within a group, obj order |
| `lib(b,c)` before vs after `a.obj` | library placement | **library members always follow ALL command-line objs regardless of the lib's position on the line**; member order = resolution order (entry pulled `c`, then `c`'s dep `b`); members packed (no ilink padding) — matches the measured retail 19.97%-vs-0% padding split |
| grow `b2`, re-link over exe+ilk | what an ilink RE-link does | **`b2` moved to the END of `.text`** (past `c.obj`), its old slot tombstoned, `b1`/`b3` unmoved — the one true contribution splitter; measured at **3 functions in all of retail** (demo-oracle result, `docs/` exe-layout notes) |

The first-definer-wins probe is the load-bearing one. Its retail-scale
consequence: a header-inline member (`inline` in a class body, a macro-generated
`Serialize`, a compiler-generated `??1`/`??_G`) is emitted by **every** TU that
includes the header, and its single surviving copy sits wherever the **first
linking** of those objs put it — at that obj's emission point (mid-run at first
use, or the deferred-inline tail at the obj's seam). That is what every "COMDAT
pool" in this project actually is: **not a linker pool — some obj's ordinary
contribution.**

## Worked example: MenuItem and its four "intruders"

The worst case in the tu_order_check report was:

    MenuItem  [0x184610-0x185a0e]  INTERLEAVES  RezColl        [0x1848b0-0x184b5d]
    MenuItem  [0x184610-0x185a0e]  INTERLEAVES  DebugPrintf    [0x184ba0-0x1851d3]
    MenuItem  [0x184610-0x185a0e]  INTERLEAVES  RezList        [0x1851e0-0x185315]
    MenuItem  [0x184610-0x185a0e]  INTERLEAVES  WapCompress  [0x1853b0-0x185456]

Walked through the mechanism, byte by byte (library band, so pull order):

    0x1832d0-0x1845a2  MenuPage.obj      33 out-of-line CMenuPage methods, ascending
    0x1845b0-0x1848aa  MenuPage.obj      its KEPT-COMDAT tail: CMenuItem::GetItemName /
                       (deferred-inline   GetLeftItemName / GetRightItemName (unpinned, found by
                        emission tail)    prediction - see below) / GetUpItemName /
                                          GetDownItemName / SetState / UsesStateAnimations /
                                          ??_GCMenuItem / ??1CMenuItem / Reset /
                                          CAnimatedMenuItem::SetState / SetFramePeriod /
                                          UsesStateAnimations /
                                          ??_GCAnimatedMenuItem / ??1CAnimatedMenuItem / Reset
    0x1848b0-0x184b5d  RezColl.obj       CBaseHash* - a real, separate TU
    0x184ba0-0x1851d3  DebugPrintf.obj   CRangeSet + CDebugConfig - real TU
    0x1851e0-0x185315  RezList.obj       CObjList/CRezList - real TU
    0x1853b0-0x185456  WapCompress.obj real TU
    0x185460-0x185a0e  MenuItem.obj      the real MenuItem.cpp: Init, Cleanup,
                                          GetFrameWidth ... AdvanceFrame, ascending

Every body in the 0x1845b0-0x1848aa block is inline-shaped (32-byte CString
getters, 3-10 byte setters/stubs, the dtor pair with its `??_G` adjacent) and
starts flush at MenuPage's padded end. These are `MenuItem.h` inline members,
emitted by every including TU and **kept by MenuPage.obj — the first obj in
link order that included the header**. Our tree homes nine of them as `RVA()`
bodies in `MenuItem.cpp` (class-homing convention), which stretched "MenuItem"'s
span over four innocent TUs.

**Prediction check:** before disassembling, the mechanism predicted the three
unpinned 32-byte bodies at 0x1845b0/0x1845d0/0x1845f0 and the unpinned
0x184730/0x1847c0 would be more CMenuItem-family inline members. They are:
`?GetItemName@CMenuItem@@QAE?AVCString@@XZ`, `GetLeftItemName`, `GetRightItemName`
(byte-identical shapes, `this+0x10/0x4c/0x50`), `?Reset@CMenuItem@@UAEXXZ`, and
`??_GCAnimatedMenuItem@@UAEPAXI@Z`.

Verdict: the four "intruders" are **real retail TUs, correctly partitioned**;
the defect was in the *invariant*, which treated MenuPage's kept-COMDAT tail as
MenuItem.cpp's contribution. The nine bodies are now rows in the exile ledger,
and all four pairs are gone without touching a single source file.

## What the gate now models: the kept-COMDAT exile ledger

`config/retail/kept-comdat-exiles.tsv` — one row per body our tree homes with
its class while retail's copy was kept inside another obj's contribution: `rva,
owner_unit, host_unit, name, evidence`. `tu_order_check`:

* excludes ledger bodies from both the intra file-order check and the inter
  span/contiguity check,
* prints the count explicitly (`63 kept-COMDAT exile bodies excluded`), and
* **re-proves every row on every run**: a row whose rva is no longer pinned in
  its owner unit, or no longer lies within its host unit's span ±0x1800 (the
  measured seam slack — deferred-inline tails start just past the host's last
  pinned fn, MenuPage's at +0x2ee), fails the audit. The ledger cannot silently
  rot into a mute list, and each row can be re-argued from its evidence column.

This is deliberately NOT a band cut (`tu_layout.POOLS` stays for the two big
low bands): the exiles sit *inside live TUs' runs*, so only a per-body,
host-verified exemption states the truth.

## Adjudication of the 80 pairs

**(b) Legitimate — kept-COMDAT exiles: 47 pairs**, dissolved by 63 ledger rows.
The big groups: MenuPage's tail (4 pairs), GruntCombat's proven mid-run group
0x58b60-0x58f6c (3), the gamelevel/levelplane seam group 0x161330-0x161558
holding all three CImageSet variants' inline members (21, the whole ImageSet
tangle), DDrawSurfacePair's woven AniRecord view dtors (7 incl. span
collapses), NetMgr's tail band (2), the compconai tail pocket 0x310f0-0x31314
(3), FrontCandyAni's kept candy ctors (3), WwdFactoryObject's two stray inline
bodies (4), MenuState's kept `~CMenuTree` (1), Play's CImage accessor pocket
(1, partially - see below), GruntEntranceMove's tail seam trio (2).

**(a) Partition defects — 24 pairs** (two units are one retail TU, or a body
is homed in the wrong unit). **4 fixed in this change:**

* `TitleAppStart` held two `CSplashState` methods sitting exactly in
  `SplashState.cpp`'s hole → merged into SplashState.cpp, unit deleted (1 pair).
* `ButeTailEncode`/`BitStreamBlowfish` held `CButeTail::Encode`/`Decode` at
  0x16f6e0/0x16f760, bracketed by `Blowfish.cpp`'s own functions (InitKey
  0x16f6c0, encipher 0x16f7f0) → one crypto TU; merged, units deleted (2 pairs).
* `StatusBarUpdaters` held `CTileTriggerSwitchLogic::SwitchDown`/`SwitchUp`
  inside `TileTriggerSwitchLogic.cpp`'s own class and span → merged, unit
  deleted (1 pair).

**Ranked worklist for the remaining 20 (a) pairs** (all are re-partition work,
frozen in the tu-order ratchet baseline):

| rank | pairs | work |
|---|---|---|
| 1 | 8 | **statusbar.cpp reunification** — SBI_RectOnly × {StatusBarItem, StatusBarMgr, SBI_StatzTabArrowEh, SBI_SideTabBuild, WarpStoneFly, MgrSettings, SBI_TabzDialogEh} + WarpStoneFly × MgrSettings. Compiland proven (credits name + bracketing, `docs/tu-partition-brief.md`). Note the sbi_tabzdialog_eh base-ctor-spelling objection in the units archaeology must be re-tested — per-site inliner variance within one TU may void it. |
| 2 | 4 | **booty.cpp** — BootyStateActivate × {GameMode, BootyWalkAnim, IconLoaders, GameText}. Compiland proven. |
| 3 | 3 | **PathHazard family** — PathHazard × Ufo × RainCloud: dense bidirectional weave of real bodies (PathHazard.cpp even pins a CRainCloud method; Ufo.cpp pins CPathHazard::SerializeDispatch) = one compiland. |
| 4 | 2 | **DDPageMgr palette pocket** — CMoviePlayer::UploadPalette/ResetPalette embedded in the DDPageMgr obj (same conclusion the palettesnapshot archaeology already reached). Fold MoviePaletteUpload + PaletteReset (+ siblings) in. |
| 5 | 2 | **Blowfish** — done (see above). |
| 6 | 1 | **Projectile × Boomerang** — bidirectional weave (CBoomerang::AdvanceMotion pinned in Projectile.cpp between Boomerang.cpp's pins) = one compiland. |
| 7 | 1 | **BattlezMapConfig × GruntMoveStep** — proven (bracketed) compconai member; fold the one fn in. |
| 8 | 1 | **MovingLogic × MotionStateStep** — CMotionState::Step + ArrivalVelX/Y between two CMovingLogic bodies = one TU; MovingLogic's head pair (0x16cdd0/0x16d000, unnamed) additionally needs a rehome decision. |

**(c) Undecided — 9 pairs**, with the evidence that would decide each:

* **Play region (5)** — Play × {PlayerCommandStep, LevelTileValidation,
  PlayPlaneScan, CImageComdats} + LevelTileValidation × PlayPlaneScan. The
  0xc7ec0-0xda4a0 region is >1 retail obj (the XCU oracle already cut
  gruntzplayer/playassetload out but sees only initializer-carrying TUs).
  Deciding evidence: interval dossiers / private-static extents for the
  remaining cuts; `CState::DrawScreenTextImage` 0xd5c10 is thunk-proven
  link-line code inside the region and carries an `@identity-TODO` already.
* **NetSessionMgr × {BuildGruntzCrcInfo, PacketPool} (2)** — same class family
  either side of the holes; needs the netcmdslot/netsessionmgr interval work.
* **MovingLogic × TypeKeyColl (1)** — is `CUserLogic::SerializeDispatch` 0x16e7f0 a
  kept inline of the z-collections TU (plausible: that TU also holds
  `CUserBaseLink`), or does the z-band split into >1 obj around it? Deciding
  evidence: whether any z-band TU includes UserLogic.h, or an initializer/
  private-static cut inside 0x16d190-0x16ea11.
* **DDrawSurfacePair × LogicRecord (1)** — the CLogicRecord block
  0x164830-0x16520d (real 0x400-byte bodies) inside the surface-pair run: one
  multi-class TU (merge) or header-heavy record class kept as COMDATs
  (ledger)? Deciding evidence: whether retail duplicates any CLogicRecord
  body's shape elsewhere (multi-emission ⇒ inline), or a seam/dossier cut.

The 7 TUs with intra-order violations (GruntzMgr 22, GameLevel 10,
CRegMgr 2, AdvancedOptions, ButeNode, ScatterSamples, ToobSpikez) are the
same phenomenon seen from inside: merged files whose parts retail interleaved
with other objs, or genuinely misordered pins. They stay frozen in the ratchet.

## What this means for the project

1. **The invariant is real and stays.** An obj's ordinary `.text` is one
   contiguous ascending block — probe-proven, code-located. A unit that
   interleaves another after exile-exclusion is a partition defect, full stop.
2. **"COMDAT pools" are contributions.** Every pooled body belongs to a specific
   obj (the first-linking includer). The pool bands are not linker magic and
   are in principle attributable — the `POOLS` band cut remains a pragmatic
   approximation for the two big low bands.
3. **A kept-COMDAT exile is a proof its body was header-defined in the original
   source.** MenuItem's nine exiles say `MenuItem.h` originally defined those
   members inline. Whether to migrate such bodies into headers is a separate
   (matching-sensitive) decision — the inline-member-crater lesson stands — so
   the ledger records the fact without forcing the migration.
4. **Link order is recoverable — and now RECOVERED.** Command-line objs in line
   order, lib members in pull order, kept COMDATs at first includer: retail
   `.text` order encodes the original project's file list and link line.
   `config/retail/link-order.tsv` (derived + re-proven by
   the retired link-line derivation, see [tooling-map](tooling-map.md)) is that recovery: 238
   thunk-proven command-line objs in order, 89 lib members in pull order, and a
   relink with the derived order reproduces the cross-TU layout exactly
   (Kendall 0.0000 vs 0.4508 for the alphabetical baseline). See
   `docs/link-order-investigation.md` § "The derived link line".
5. **Ilink moves are negligible** — the one genuine splitter is bounded at 3
   functions in the whole EXE, and none of the 80 pairs involved one.

## Reproduce

    gruntz verify tu-order            # gate + exile accounting
    # probes: scratchpad linkprobe/run.sh (cl+link under wine, see Probe record)
    # link.exe reversing: Ghidra project on $MSVC_DIR/bin/link.exe; addresses above
