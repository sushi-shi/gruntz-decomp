# Brief: the TU-partition lane gates BOTH .text layout AND the data loop

For the dedicated lane. Written by the DATA lane after the contribution-manifest
attempt was measured and found blocked. Companion: `docs/data-attribution.md`.

> **MECHANISM SETTLED (lane/band-interleave, 2026-08-09):** how link.exe 5.10
> actually lays out `.text` is now read out of the linker binary itself and
> probe-proven — `docs/link-text-layout.md`. The contiguity invariant SURVIVES;
> the one legitimate exception (multi-defined COMDATs kept at the FIRST linking
> obj) is modeled explicitly by `tu_order_check` via
> `config/retail/kept-comdat-exiles.tsv` (per-body, host-verified, counted).
> The 80 interleave pairs adjudicated: 47 legitimate exiles, 24 partition
> defects (4 fixed), 9 undecided — ranked worklist in that doc.

> **CORRECTION (matcher-6, 2026-07-17) — READ THIS FIRST. The premise below,
> "ANY interleaving proves our TU partition != retail's compilands", is FALSE, and
> the 12134-pair headline is ~98% an artifact of it.**
>
> A compiland's contribution is contiguous **only for its ordinary (non-COMDAT)
> .text**. MSVC5 emits inline members, macro-expanded members, and compiler-generated
> ctor/dtor/`??_G` bodies as **COMDATs**, and the linker groups those into pools far
> from the owning obj's run. So a pooled body's RVA says nothing about which obj owns
> it, and `tu_order_check`'s INTER-TU invariant is over-strict for it. (`tu_layout.py`
> already knew this - `POOLS`, `--exclude-pools` - but `POOLS` is a hardcoded guess:
> its `(0x10000,0x14000)` misses the real pool's head at ~0xf2f0, and its
> `(0x80000,0x90000)` overlaps genuine compilands.)
>
> **Proof (not inference):** 0xf470 `CAniCycle::Serialize`, 0x10a10
> `CSecretLevelTrigger::Serialize` and 0x12bc0 `CToobSpikez::Serialize` are all
> **exactly 71 bytes and byte-for-byte identical** (all three `call 0x16e7f0`, zero
> relocations). retail was linked without ICF (measured, `docs/linker-flags.md`), so they are three separate COMDAT copies of
> one macro-generated member that the linker pooled - not one compiland. The band
> 0xf2f0..0x13d00 is dozens of these: `Serialize` at +0x47 and `~X` at +0x44, over and
> over, for ~20 unrelated classes.
>
> **Measured, all-pairs:** of 190 outlier functions (5% of 3722) that cause **98.4%**
> of the interleaving (12099 pairs -> **195** if every TU kept only its largest
> cluster), **161 have a class that ALREADY lives in their TU** - i.e. they are pooled
> COMDAT residue, and the partition for them is *correct*. Only **20** had a class
> whose plurality home was a different TU (real rehomes; matcher-6 landed the provable
> ones), and 9 are classless.
>
> **So the data loop is NOT blocked on a wholesale re-partition.** What it needs is a
> pool-aware contribution model: measure the real COMDAT-pool bands empirically
> (replace the hardcoded `POOLS`), exclude pooled bodies from the contiguity
> invariant, and derive contributions from each TU's *ordinary* run. Homing a pooled
> body with its class is already the tree's convention and is correct
> (`~CMenuSparkle` 0x101b0 lives in MenuSparkle.cpp) - it is not a defect to "fix".
>
> Owner attribution is by **xref / vtable-data-ref / class identity**, never by RVA
> proximity - and that still works fine for pooled bodies, because the *class* is
> recoverable even when the *obj* is not.
>
> **REFINEMENT (2026-08-02, OOL ctor/dtor campaign): the contributing obj IS
> recoverable for realization groups.** Header-inline ctors/dtors/??_G/inline
> virtuals emit as one COMDAT group from the TU that constructs the class
> (vtable-realization); the linker keeps the first-linking copy and lays the group
> at that compiland's seam, alignment-padded, ??_G adjacent to ??1. The emitting
> TU is identified by (a) base-obj emission census (`llvm-nm` over
> `build/objdiff/base`), (b) seam adjacency (`gruntz sema map range`), (c) retail
> xref (`gruntz sema xref <ctor rva>`). When all three agree, the `RVA_COMPGEN`
> pin goes in that TU - class-homing is only the fallback when the contributor is
> genuinely unrecoverable. Proven cases: CPlay/CState group -> gruntzmgr
> (0x8c4xx-0x8c9xx), SBI dtor clusters -> sbi_rectonly / statusbargamemenu /
> statusbarmgr, CMovingLogic/CMotionState group -> serialobjectfactory
> (0x136d0-0x13c40). A pin in a NON-emitting TU silently loses its label (the
> labels manifest ratchet catches the count drop) - never place one unverified.

## What a contribution range is, and why we need it

Per (compiland, storage) a **contiguous** `[rva, size)` interval. A linker lays each
object's **ordinary** contribution to a section down in one piece (COMDATs are the
documented exception - see the correction above), so real contributions never
interleave. homm2 reads them free from CodeView NB09 `sstModule`; **GRUNTZ.EXE has no
debug stream**, so ours must be derived from our own TU partition. They gate:

- `--contribution-manifest` → owner attribution for data we cannot otherwise place:
  the 290 COMDAT string payloads (296 retail copies) whose retail owner is currently
  unprovable, plus the `$T`/`$S` constant pools;
- `--data-section-manifest` → placing definitions at their candidate `section_offset`
  (candidate-shaped target sections), which is the rest of `matched_data`;
- absolute-RVA layout: `exe-diff` §B abs-rva ≈ 0, §E 0/771 at the correct
  section-relative offset.

## Measured state: our TU partition != retail's compilands

- `gruntz verify tu-order`: **GATE FAIL — 49 TUs with intra-order violations,
  10832 interleaving TU-pairs** (was 52 / 12134 when this brief was written; matcher-6
  landed the first drain — and see the correction at the top: ~98% of the remainder is
  COMDAT-pool placement, not a partition defect) (e.g. `Fader [0x17e450-0x182935]` interleaves
  `RezBufferObjectDtor`/`RecordFill`/`CircleShadeBlit`; `MenuItem [0x184610-0x185a0e]`
  interleaves `RezColl`/`DebugPrintf`/`RezList`/`WapUncompress`).
- Independent check (hull of each unit's labelled symbols per storage, **all-pairs**
  overlap): `.text` **134/259** unit hulls interleave. Data is no better once measured
  correctly: only **8 of 86** bands overlap no other band — rdata **2/15**, data
  **5/17**, bss **1/54**.
- Consequence, measured: **0 of 296** ambiguous COMDAT string copies can be attributed
  by clean-band containment. The data contribution manifest is NOT independently
  recoverable; it waits on this lane.

CAUTION for whoever measures this next: check overlap **all-pairs**, not just adjacent
bands in a sorted list. Adjacent-only is blind to one band swallowing many others and
reports ~70-80% clean where the truth is ~9%. That error is why this brief exists.

## Worst .text offenders by span (a single compiland cannot do this)

| unit | fns | .text hull span |
|---|---:|---:|
| motionstate | 6 | 0x15bd60 (1.4 MB) |
| movinglogic | 6 | 0x15bb60 |
| gametext | 16 | 0x155280 |
| butemgr | 54 | 0x1539a0 |
| gruntzrandom | 5 | 0x14fee0 |
| movieplayer | 3 | 0x143730 |
| ddrawsurfacepair | 44 | 0x12bee0 |
| gruntvoice | 19 | 0x107530 |

Aggregates (`globals`, `vtables`) span by construction and are not compilands — exclude
them from band analysis, but they must drain eventually too. (`src/Stub/All.cpp` /
`engine_label_stubs` is **gone** — that backlog is fully drained; only provenance
prose survives in `config/units.toml` comments. Excluding it changes nothing: 8/86
clean either way.)

## Six more retail compiland names, from the shipped credits (2026-08-08)

`GRUNTZ.EXE`'s own string table leaks nine `.cpp` paths, exactly one of them
under `C:\Proj\Gruntz\` (`GruntzMgr.cpp`). The shipped credits resource
`STATEZ\CREDITZ\CREDITZ` names six more, two of them with line numbers, in a
pasted multiplayer desync log and a joke list:

| Name | Evidence |
|---|---|
| `C:\Proj\Gruntz\Grunt_State.cpp` line **922** | 12 occurrences in the log |
| `C:\Proj\Gruntz\Grunt_Combat.cpp` line **411** | 1 occurrence in the log |
| `booty.cpp` · `compconai.cpp` · `statusbar.cpp` · `nakedchix.cpp` | joke list |

None of the six appears anywhere in `src/`, `include/`, `config/` or `docs/`.
Our tree splits the same code across many small units — `GruntCombat.cpp`,
`GruntStateStep.cpp` / `GruntStateRec.cpp`, `StatusBarMgr.cpp` and siblings,
`BootyStateActivate.cpp` and siblings — so these are the *original* compilands
those units were carved out of, and merging toward them is a candidate
consolidation. Full quotation and the rest of the credits' contents:
[`formats/game-data-strings.md`](formats/game-data-strings.md#2a-retail-source-file-names--tu-partition-evidence).

## Instruments already in place

- `gruntz link` → candidate EXE + `.map` (392 objs, 4886 unresolved externs under
  the candidate `.map` (`gruntz link`) for build order.
- `gruntz verify tu-order` — the one-contiguous-block invariant gate.
- `gruntz verify link-tier` — `.text` intra-TU order / block-exact / abs-RVA + a ranked
  reorder worklist; §E — data static-storage: 771/925 data symbols defined+placed,
  715/771 in the right storage class, 0/771 at the right section-relative offset, with
  the first section-relative divergence per section.

## Method (homm2's, and it is the right one here)

Fix the **earliest** divergence in a section, relink, re-read: later rows are cumulative
consequences, not independent defects. Split holding TUs by RVA gap and chase the real
owner by xref (see the holding-TU-drain notes).

**Revised metric (see the correction at the top).** Driving `tu_order_check`'s raw
interleave count to 0 is NOT achievable and not the goal: ~98% of it is COMDAT-pool
placement, which no source arrangement can express. The tractable targets are:

1. **Measure the real COMDAT-pool bands** and replace `tu_layout.POOLS`' hardcoded
   guess; then make `--exclude-pools` the gate. That is the honest invariant.
2. **Drain the holding TUs** - files that group unrelated classes by method name or by
   "orphan" rather than by compiland. matcher-6 dissolved seven
   (FlashRect, MenuStateAssets, GruntSpawnLevel, LogicActReg646010, OrphanLeaves,
   OrphanMethods, plus MoviePlayer's slice) and every move was byte-neutral or better
   (`CBootyState::FormatHudText` 88.64 -> 97.50 just from being homed to its real TU).
3. **The ~20 real rehomes** the class-identity check finds (outliers whose class's
   plurality home is another TU) - not the 161 pooled-COMDAT outliers.

Then `exe-diff` §B/§E start moving and the data lane can generate contributions from
each TU's *ordinary* run.

## Known adjacent defect, worth a dedicated lane (matcher-6, proven)

`CUserLogic`'s **vtable slot 4 is modelled with the wrong arity tree-wide**:
`include/Gruntz/UserLogic.h:360` declares `virtual i32 UserLogicVfunc2()` (no args),
but retail's own base body (ILT thunk 0x246e -> **0x8b70**) and every override
(e.g. 0xade60) end in **`ret 4`** - the slot takes one 4-byte arg.

Consequence: ~40 leaf classes cannot declare their REAL slot-4 body as the override, so
each parks it as a plain non-virtual `RunAct`/`FireActivation` *beside a declared-only
placeholder virtual* - the exact "body-less placeholder virtual" anti-pattern the
matcher brief bans, replicated ~40 times from one wrong signature. Many lanes hit it and
documented it in place (`Grunt.h:881`, `Projectile.h:77`, `PathHazard.cpp:232`,
`ActionArea.h:25`, `SpotLight.h`, `Teleporter.h`, ...). Fixing the base arity would let
every one become a real `OVERRIDE` and delete its placeholder. The sites are almost all
declared-only, so the blast radius is small; it needs one lane that owns UserLogic.h.

---

## Six retail compiland names from the game's own credits (lane/compiland-names, 2026-08-08)

`STATEZ\CREDITZ\CREDITZ` (REZ entry at file offset `0x04485fc1`, 0x25a7 B, plain
CRLF text) carries six `.cpp` names that appear nowhere else in the shipped data
or either EXE. **They come from two different places and are NOT equally strong
evidence** - a distinction worth keeping:

1. **A pasted dev-build desync log** (the tail of the entry) prints, over and
   over, `C:\Proj\Gruntz\Grunt_State.cpp, 922` and once
   `C:\Proj\Gruntz\Grunt_Combat.cpp, 411`. Full `C:\Proj\Gruntz\` paths with
   line numbers, in the project's CamelCase - the same shape as the one path
   retail itself leaks (`C:\Proj\Gruntz\GruntzMgr.cpp`, referenced by
   `CGruntzMgr::InitializeLobbyConnectionSettings` @0x0008eca0).
2. **A joke "we would like to acknowledge" list** (~70 in-jokes: `CTRL+Y`,
   `Subway`, `MPKELLY`, `ButeMgr`, `MoveGruntAroundObstacle()`,
   `ActuallyRemoveGrunt()`, ...) contains **`booty.cpp`, `compconai.cpp`,
   `statusbar.cpp`, `nakedchix.cpp`** - bare, all-lowercase, no path, no line.
   Two of that list's function names (`MoveGruntAroundObstacle`,
   `ActuallyRemoveGrunt`) exist nowhere in our tree either, so the list names
   real code; but nothing dates it, and lowercase is not the project's style.

`.cpp` occurs exactly 17 times in the whole REZ - 4 (list) + 12 + 1 (log) - so
these six are the complete set. `GruntDem.exe` leaks the same paths as retail and
adds nothing.

### What each maps to

Method: the linker lays one obj's *ordinary* `.text` down contiguously, so a
function of TU *B* sitting strictly between two functions of TU *A* proves A and
B are one compiland (COMDAT-shaped bodies - `??0`/`??1`/`??_G`/`$E`/inline
accessors - are excluded; the linker pools those).

| credits name | retail band | strength |
|---|---|---|
| `booty.cpp` | `0x00018830`-`0x0001f928` | **strong** |
| `statusbar.cpp` | `0x000fdc00`-`~0x0010bc30` | **strong** |
| `compconai.cpp` | `0x00024dc0`-`0x00036051` | plausible |
| `Grunt_Combat.cpp` | `0x00056f80`-`0x0005d084` | plausible |
| `Grunt_State.cpp` | undetermined | - |
| `nakedchix.cpp` | undetermined | - |

- **booty.cpp** - the band is 44 functions and **every one is a `CBootyState` or
  `CMultiBootyState` method**, no interlopers. Our tree splits it over six TUs:
  `BootyCheatState.cpp`, `BootyStateActivate.cpp`, `GameMode.cpp`,
  `BootyWalkAnim.cpp`, `IconLoaders.cpp`, `GameText.cpp`. `IconLoaders.cpp`
  (0x1c070 and 0x1e720) is *bracketed* by `BootyStateActivate.cpp` functions on
  both sides, as are `GameMode.cpp` and `GameText.cpp` - contiguity therefore
  **proves** they are one compiland. Three of those six TU names
  (`GameMode`, `IconLoaders`, `GameText`) hold nothing but booty-screen methods
  and are actively misleading.
- **statusbar.cpp** - 135 functions, effectively all `CStatusBarMgr` plus
  `CWarpStoneFly`/`CStatusBarSprite` and the pooled `??1`/`??_G` group of every
  `CSBI_*` item class (which is exactly the vtable-realization group of the TU
  that constructs them). `StatusBarMgr.cpp` (0x102250-0x104cb0),
  `SBI_TabzDialogEh.cpp`, `SBI_SideTabBuild.cpp`, `WarpStoneFly.cpp` and
  `MgrSettings.cpp` are each bracketed by `SBI_RectOnly.cpp` functions - one
  compiland, ~14 of our TUs. Note our `StatusBarMgr.cpp` is a 4-function
  fragment of it while `SBI_RectOnly.cpp` carries the bulk.
  A **separate, earlier** band `0x000e6020`-`0x000ebae4` holds only the
  `CSBI_*` item classes' own methods (WellGoo/Image/ImageSet/ImageSetAni/
  MenuItem/GruntMachine/SideTab/StatzTabArrow/StatzTabGruntBar/WarlordHead) -
  a sibling compiland, unnamed; it cannot be the same one (not contiguous).
- **compconai.cpp** = "computer-controlled AI". The band is one class,
  `CBattlezMapConfig` - `ChooseIdleBehavior`, `RouteToNearbyEnemy`,
  `PathToNearestGoal`, `RetargetIdleUnit`, `TrackAssignedEnemy`,
  `AdvanceToEnemyBase`, `StepDefenderUnit`, ... i.e. an RTS opponent - split
  over eleven of our TUs (`BattlezMapConfig.cpp`, `GruntMoveStep.cpp`,
  `BattlezUnitStep.cpp`, `GruntTileScan.cpp`, `GruntStateStep.cpp`,
  `BattlezSpecialAnim.cpp`, `BattlezSpawnCheck.cpp`, `BattlezRepath.cpp`,
  `BattlezReservePlace.cpp`, `BattlezRetarget.cpp`, `TileScan.cpp`).
  `GruntMoveStep.cpp`'s single function is bracketed by `BattlezMapConfig.cpp`,
  proving at least that pair. Battlez is where the game itself says "Computer
  (easy/normal/difficult)", which is what makes this the better reading; the
  rival reading is the enemy-grunt behaviour band below. Not proven.
- **Grunt_Combat.cpp** - `0x00056f80`-`0x0005d084` is our `GruntCombat.cpp`
  almost exactly: `OnStruck`, `BeginAttack`, `CommitNeighbor`, `FindGridNeighbor`,
  `EnsureStruckSlot/Voice`, `LoadGruntCombatAnimations`, `PathScan`, plus
  `CreateGrunt`/`RegisterGruntActions`/`Activate`. Its COMDAT group sits at
  `0x58b60`-`0x58f6c` *bracketed by its own functions* (incl.
  `??0CUserLogic@@QAE@PAUCGameObject@@@Z`, which is exactly what a TU containing
  `CreateGrunt` emits) - so the band is one compiland. That it is *the* file
  called `Grunt_Combat.cpp` is inference from content, not proof; the adjoining
  `0x0005d210`-`0x0005fe6e` (`StepBehavior`, the 17-way AI tick, + `FinalizeStep` +
  `AdvanceMotion`) is contiguous with it and could belong to either side.
- **Grunt_State.cpp** - **undetermined.** The file has >=922 lines and contains a
  per-grunt trace that prints act name, goal position, a `tg` flag and a random
  number for all twelve gruntz at once. The two structural candidates are
  (a) `0x000ec670`-`0x000f87f9` - sixteen consecutive one-function TUs of ours,
  every one a `CGrunt` behaviour step that switches on `m_defenderState`
  (`WanderStep`, `ChargeStep`, `SeekTarget`, `ScanNearestTarget`, `PhaseStep`,
  `StepArrivalDefense{,Alt,Lean}`, `StepBrickLayerBehavior`,
  `StepGooSuckerBehavior`, `StepDiggerBehavior`, ... plus the file-static
  `_CellTargetable`), ~49 KB, all called from the single 17-way dispatch at
  0x0005d210 - and (b) the `CGrunt` core at `0x00047a10`-`0x00055160`. Against
  (a): the log's five state names are act codes (see below), not the
  SEEK/CHASE/ATTACK/RETURN/COOLDOWN/RETREAT of `m_defenderState`. No TU in (a)
  brackets another - each holds exactly one function - so contiguity proves
  nothing there. **Do not rename on this.**
- **nakedchix.cpp** - **undetermined, and possibly not in the shipped build.**
  The token occurs exactly once in the whole REZ (in this list); `chick`,
  `bikini` and any asset-side variant occur zero times; the movie-player theory
  dies because playback lives inside `CGruntzMgr::MakeRezPath`, not a compiland
  of its own. The four list names being lowercase, when every path the binaries
  leak is CamelCase, is weak evidence that they are older files.

### Partition defects this exposes

Three retail compilands are shattered across our tree, and in two of them the
fragment names mislead: **booty** (6 TUs), **statusbar** (14 TUs, with the bulk
under `SBI_RectOnly.cpp`), **compconai/Battlez** (11 TUs). None of this changes
matching; it changes where a body should live and what a unit should be called.

### Negative result: string literals cannot attribute compilands here

Worth recording so nobody re-derives it. `.data` holds exactly **one** copy of
each game string, and single addresses (e.g. `LogicBump` @0x0020a470) are
referenced from 58 functions spread over the whole image, in compilands that
provably differ. Retail therefore folded string literals across objects, so the
"each TU pools its own strings, so a string address names its owner" test - the
obvious second axis for the contribution manifest - **does not work**. (Not
universal folding: `'1252'` still has 27 copies and one save-game message has 2.)

---

## Per-unit partition notes (migrated out of `config/units.toml`, 2026-08-03)

These were inline comments in the manifest. They are **archaeology**, not build
configuration: why a TU exists, was split out of a conflation, or absorbed another.
They moved here because `config/units.toml` is machine-edited in bulk (profile
renames, flag sweeps) and prose in its `[[unit]]` blocks both rots and sits in the
blast radius - a comment-stripping pass ate the `[build]`/`[flags]` tables and four
whole unit blocks the day this was written. The manifest is now pure data.

**Stale-by-construction warning:** these predate the 2026-08-03 profile rename, so
references to `flags="base"` / `"eh"` / `"mfc"` name profiles that no longer exist
(the set is now `c` / `cpp` / `cpp-rtti`; see `docs/linker-flags.md` for why).

### sbi_rectonlybase
The thin RTTI CSBI_RectOnly intermediate's own obj (dossier #16): slot-2 Setup @0xe86e0.

### sbi_tabzdialog_eh
CTabzBuilder::BuildTabzDialog (0x10a340) - its own retail obj, un-merged back out of
SBI_RectOnly.cpp. It needs the OUT-OF-LINE CStatusBarItem base ctor (retail emits
`call ??0CStatusBarItem`) while SBI_RectOnly.cpp's builders need the INLINE one, and
MSVC5 has exactly one base-ctor spelling per TU - so they cannot share a TU.

### sbi_sidetab_build
--- the rest of the wave1-E (c62cfaf8d) one-file merge, un-merged (2026-07-13) ---
A TU is the unit of BOTH MSVC5's /O2 budget and its compiler flags. Two of these were
flags="base" (no /GX) yet had been folded into the flags="eh" SBI_RectOnly.cpp, which
cannot be right: one TU compiles with one flag set.

### sbi_imagesetani
CSBI_ImageSetAni::Serialize (0xe7cd0), the frameless slot-1 serialize re-attributed
from SBI_WarlordHead.cpp (vtable proof: slot 1 = thunk 0x2829, shared w/ StatzTabArrow).

### triggermgrgrid
CTriggerMgr grid/placement obj [0x6b640..0x6eb25] - split off the triggermgr
conflation (own init-frag run 10@0x6b370; original filename unknown).

### triggermgrhittest
CTriggerMgr hit-test/probe obj [0x759e0..0x75e1a] - split off the triggermgr
conflation (tail of the unreconstructed FUN_6f2f0 megafn's TU).

### menuitem
ONE original TU (dossier 0x1832d0: MenuItem.cpp [0x185460..0x185a0e]): absorbed menuitem2.

### ingameicon
ONE original TU (wave3-J): the 0x095b10-0x099b46 text is an I-T-I sandwich +
contiguous private-.data extents; absorbs the ex ingametext unit.

### statusbartabbuilders
ONE original /GX TU (dossier 0x0e8a70-0x0ea3ea): absorbed sbi_gruntmachine +
sbi_sidetab + the SetDirection seam pair (ex statusbarmgr).

### projectile
ONE original TU (wave3-J): the 0x0dec60-0x0e2213 text is a P-T-P sandwich +
contiguous private-.data extents; absorbs the ex timebomb unit.

### gamelevelmove
The movement/collision-resolution module (original TU at 0x167130-0x168276,
bracketed by the ImageSet TU before and CWwdSpatialMgr after; filename unknown -
candidates Move.cpp/Collide.cpp). CGameLevel's methods span several original
files; these 12 movement fns are birth-positioned in their own obj (wave1-C).

### levelplane
The plane/render module (original TU at [0x161350..0x163a00], @identity-TODO;
interval dossier 0x15ccd0, wave1-C): CLevelPlane + CPlaneRender + the WwdFile
plane methods (RebuildPlanes/ReadPlaneObjects) + the CImageSet3-helper and
CDDrawWorkerHost bodies woven into that range. The imageset1/2/3g TUs below
carry the CImageSet variant records' out-of-line bodies; their vtables are
still emitted (and VTBL-bound) in gamelevel via ReadImageSet's `new CImageSetN`.

### imageset3g
imageset3g: the small (size-0x18) CImageSet3 variant record's bodies. Named
distinctly from the existing `imageset3` unit (src/Image/ImageSet3.cpp), which
models a DIFFERENT grid-owning object under the same reconstruction name.

### savegame
ONE original TU (wave3-J): the 0x0e3690 interval's savegame+levelinfodlg text is
an L-S-L-S sandwich + contiguous private-.data extents; absorbs the ex
levelinfodlg + savegamemenu units (DrawSaveGameMenu 0xe3f40 is text-contained).

### fileimage
wave4-K: the 0x143cf0-0x145dff surface file-CODEC obj (dossier #14I; original
name unknown - no __FILE__ anchor; a DIRFILE.CPP-like DDrawMgr sibling).

### gruntzmgr
ONE original TU: C:\Proj\Gruntz\GruntzMgr.cpp (__FILE__-anchored). wave3-J merged
the ex playdtor+appdialogs units in (FLAGS group 0x08b8c0-0x093ce7, unified /GX).

### gruntzmgr2
CGruntzMgr::SetCellHeight (0x111ec0) - a lone CGruntzMgr retail-object contribution
far from the GruntzMgr.cpp block (interleaved with tile-trigger-switch-logic .text).
Same class, split into its own TU so each src file maps to one contiguous retail obj.

### serializesyncmarker
SerializeSyncMarker (0x13610) - the free WAP32 serialize round validator, a separate
retail object far from GruntzMgr.cpp (among the CVoiceTrigger serialize .text).

### butetree
CButeTree::Find/Insert - the string-keyed crit-bit (PATRICIA) trie. No
destructible locals -> /base (no /GX EH frame); strlen/strcmp/strcpy come from
/O2's /Oi intrinsics.

### butenode
CButeNodeBase / CButeNodeEntry ctors (0x16df70 / 0x16dff0) - the .bute config-tree
node base + its embedded keyed-store entry. Re-homed from src/Stub/CButeNodeBase.cpp
into its own unit (its self-contained class model conflicts with ButeMgr.h's minimal
CButeNodeBase decl). flags="eh" == the engine_label_stubs base+/GX it came from.

### gruntassetloaders
The mechanical CGrunt asset/sprite/tuning loaders (re-homed from ApiCallers).
No destructible locals -> /base (no /GX EH frame).

### gruntpickupload
CGrunt::LoadPickupSprites (0x65e80) - the pickup/powerup entrance-sprite loader
(re-homed from ApiCallers). No destructible locals -> /base.

### gruntzrandom
The MS-CRT-style LCG random helpers (0xcd00/0xcd70/0x19f50/0x15cbe0), reached
through ILT jmp-thunks (re-homed from ApiCallers). No destructible locals -> /base.

### palettesnapshot
CDDScreen::Snapshot (0x17cd90) - system-palette capture, split out of the ex
ResourceLoaders.cpp holding TU. Its obj-mate Apply (0x1775f0 = CImagePaletteNode)
folded into PaletteBmp.cpp; this is a stray CDDScreen-palette method (sibling of
PaletteReset/PaletteCopy, embedded in the DDPageMgr obj). No destructible locals -> /base.

### gruntzmgrcmd
CGruntzMgr::HandleCommand (0x862f0) - the WM_COMMAND / accelerator + cheat-code
dispatcher (the binary's single largest function). Re-homed from the
src/Stub/ApiCallers.cpp winapi grab-bag; a destructible /GX config-command
local forces the EH frame -> "eh" (== engine_label_stubs' base+/GX).

### serialobjectfactory
SerialObjectFactory (0xd2a0) + its ParseSerial helper (0xd210) - the game's
(de)serialize object dispatch, a separate obj (contiguous 0xd210-0xec24 block).
Carved out of GruntzMgrCmd.cpp (REHOME D9); kept "eh" to preserve ParseSerial's
exact build flags (byte-neutral - no destructible locals in either fn yet).

### savefrontbuffershot
SaveFrontBufferShot (0x114ec0) + ...Impl (0x114f00) - the front-buffer screenshot
forwarder pair, a separate obj (0x114ec0-0x114f3e block). Carved out of
GruntzMgrCmd.cpp (REHOME D9). No destructible locals; "eh" == its old base+/GX.

### grunttargetscan
CGrunt nearest-enemy / arrival-target scan (0xf42f0) - sibling of the three
GruntArrivalScan steps (re-homed from ApiCallers). No destructible locals -> /base.

### gruntdatarecord
GruntDataRecord::SerializeStrings (0x56da0) - the per-record string/field writer
the big grunt-data Serialize (0x53f90) calls; non-EH (plain stack buffer).

### boomerang
--- src/Stub/ FULLY DRAINED + DELETED (2026-07-10): the labeled-but-unmatched
backlog (engine_label_stubs / All.cpp) is empty; every stub was re-homed to its
real class TU. The re-homed units below keep flags "eh" (== the old base+/GX). ---
CBoomerang projectile ctor (0xe0650); no other CBoomerang method reconstructed
yet, so this is the class's first/real TU.

### splashstate
CSplashState::LoadSounds (0xf9780); first/real CSplashState TU.

### unknownfileioctor
??0CFecFile emit TU: the retail copy at 0x8fea0 is gruntzmgr's COMDAT emission
(ChangeState constructs CMoviePlayer whose m_decodeStore member is a CFecFile),
but converting to a header inline makes our cl flatten the ctor into ChangeState
and lose the label (inline-depth wall). Kept until that caller converges.

### bootystateactivate
CBootyState::vfunc_9 (slot 9) + CMultiBootyState::OnActivate2 (slot 8) - vtable-proven.

### checkpointswitchbuild
eh: CMultiBootyState::Render (0x1f480) has the /GX frame for its CString HUD temp.
CCheckpointTriggerSwitchLogic slot-1 builder (0x112a50) - vtable-proven.

### loadgamemenu
GruntzLoadGameDlgProc (0x9dff0) + LoadGameCommand (0x9e390) - the "GAME_LOAD" dialog
(load-side sibling of savegamemenu; opened by CGruntzMgr::RunLoadGameDialog).

### ingametextupdate
CInGameText::Update (0x997c0) per-frame tick - Ghidra cluster attribution.

### gameinfostring
CGameInfo::FormatGameInfoString (0x1183b0) save-game info query builder.

### cspawnentry
CSpawnEntry ctor (by-value CString name + data -> /GX); the named spawn/voice
record (<Gruntz/SpawnList.h>, the CVoiceSound/CSpawnEntryN/CObjResNode/Obj09a260
unification). GetName (ex-obj09a260 unit) re-homed to areamgr, its retail band.

### objecttracker
CObjectTracker per-tick game-object tracker (ex-"Obj0f7d90"); Update re-homed from Discovered.cpp.

### bitstreamblowfish
__stdcall Blowfish bit-decode loop (0x16f760), trace mis-attributed to
ClassUnknown_4; re-homed from Discovered.cpp.

### spotlight
CSpotLight::Update (0x0b1ee0) x87 rotation re-homed from Discovered.cpp.

### spotlightctor
CSpotLight 1-arg ctor (0x0b1200); /GX EH frame (throwing CUserBaseLink base).

### creditsstate
CCreditsState (credits/attract CState leaf) + CCreditzOwner (SetupTitle), split out of
the GameMode.cpp god-TU. eh: ~CCreditsState / DrawScrollingCredits (CString temp) /
SetupTitle (operator new/delete + CString) carry the /GX SEH frame.

### statereleaseresources
CState::ReleaseResources (0x0fa150), the base game-state teardown (CState vtable
slot 2's default body; ex "CGameModeBase::BaseCleanup" - that class was a this-view
of CState, folded). Its own retail .obj (re-homed from Stub).

### attract
The attract state-services interval [0x0fa1f0..0x0fb328]: woven CAttract title/
fade + CSoundFxEmitter + CState helpers + the CState header serialize (one obj).
eh: CState::InputVirtual's CString splash + the draw temps carry the /GX frame.

### attractstate
CAttract state-machine CORE obj [0x013fb0..0x014819] + the COMDAT ??1 dtor
(0x08cd90); split out of the conflated Attract.cpp (REHOME D5). The class-identity
TU: 10 CState vtable-slot overrides + the vtable/??_G emission anchor. eh: Vslot09's
CString format local + the ??1 EH frame.

### play
The 0x0d5960 interval is ONE original TU (dossier: one 20-frag init run; the
channelslots frag INSIDE the play run): wave3-J folded the ex channelslots+
gruntzplayer+gamemodeobjlifecycle units here, /GX unified.

### demo
The demo/attract feature obj [0x3bfa0..0x3dee1] (dossier #16): CDemo methods +
demo setup/camera + bute debug editors + the anim-worker dispatch family. /GX.

### rockbreakeffectupdate
Ghidra-missed 0x476b0: per-frame rock-break effect Update (identity-TODO).

### directionclassify
Ghidra-missed 0x4a780: 8-way direction classifier (identity-TODO, FP).

### bootymessages
Booty/secret-state HUD message overlays (string-xref cluster): a destructible
MFC CString temp per banner -> /GX exception frame.

### startupprompt
Launch CD-ROM / Spawn-Mode prompt (StartUpPrompt): two CString temps + the
BeginWaitCursor/EndWaitCursor scope -> /GX exception frame.

### gameiconflasheffect
base -> eh (wave3-I: the 0x616e0 interval has 2 EH sites)

### donothing
eh: ~CMenuState / ReleaseResources (operator delete + ~CChatBox) / BuildVersionString
(CString temp) carry the /GX SEH frame; the base dialog helpers are unwind-free (neutral).

### secretteleportertrigger
ONE original TU (wave3-J): the 0x041e90-0x042cd3 text is a T-L-T sandwich
(adjacent frag run i40/i41); absorbs the ex secretleveltrigger unit.

### leafcueplay
Foreign lone methods carved out of ddrawsubmgr (operation REHOME D8): LeafCue and
CQueueDrainHost sit ~1.2 MB before the DDraw submgr block - each its own obj.

### ddrawptrcollections
wave4-K: now the 0x148840-0x148cd8 surface-extras pocket obj only (dossier #14);
the in-band 0x141cc0-0x143c20 pool/factory methods live in DirectDrawMgr.cpp
(the DDRAWMGR.CPP obj). Unit name kept for delinker packing.

### ddpalette
ONE original TU: C:\Proj\DDrawMgr\DIRPAL.CPP (__FILE__-anchored; wave3-J merge of
the ex ddpalette+dirpal+palettelerp units + the PalLoad_1479e0 stray). Unit name
kept "ddpalette" (unit renames disturb delinker packing).

### directinputmgr2
TWO original files (__FILE__-anchored; wave1-E split at 0x134cb0): DinMgr2.cpp
(absorbed fixedptrarray32) + InputDevice.cpp below.

### directsoundmgr
The two original Dsndmgr TUs (docs/exe-map/interval-dossiers.md, split @0x137330):
directsoundmgr == C:\Proj\Dsndmgr\DSNDMGR.CPP (absorbed sounddevice, dsoundvoice,
gruntcmdpercent, statusbarmgrgetitem + the DSoundList::RemoveMatching /
WAVE-resource seam re-homes); soundstream == C:\Proj\Dsndmgr\DSndMgSR.cpp
(absorbed streamfeeder, streamvoice, soundstreamfree, soundstreamteardown +
TickSubManagers + the error-reporting tail).

### soundvoicelist
The five 0x1390e0+ list primitives - a separate retail obj PAST the DSndMgSR.cpp
interval end (0x13848b), so it stays its own unit (RemoveMatching @0x136f60 moved
to directsoundmgr, whose obj span owns it).

### chatbox_eh
eh: /GX is what makes Clear CALL the header-inline ~CMenuPage instead of
folding it (the inliner declines to create an EH frame in a frameless fn) and
what emits the standalone ??1 COMDAT this obj contributes at 0x183250 (retail
link position: right after HitTest2 0x183230, before menupage's GetKey).
Lone dissenter: Find (0x182be0) destructs a CString temp with NO fs:0 frame
in retail (non-/GX shape) - see the @early-stop on Find (suspected retail TU
split around it; /O1 disproven empirically - mass craters).

### frontcandyani
ONE original TU (wave3-J): the 0x0abfa0-0x0ad527 text is an F-E-F sandwich
(frontcandyani frag @0xad110 in the tail); absorbs the ex eyecandyani unit.

### ddrawblterrthunk
/GX: retail's RemoveAll CALLS ??_M (the `eh vector destructor iterator') and Construct
calls ??_P + carries the EH try-frame those throwing iterators need. cl only emits the
??_M/??_P forms under /GX - without it, an array delete/new lowers to a plain inline
loop (no EH frame), which is what this unit used to produce. So retail compiled this TU
with exception handling on.

### ddpagemgr
/GX: AddFile/ExtractArchive have destructible CFile/CString locals (retail's SEH
frame). Neutral for the frameless leaf methods (no destructible locals).

### netlobbydialogs
NetMgr lobby / in-game network DialogProcs + their init helpers, re-homed from
src/Stub/ApiCallers.cpp. A destructible CString status-banner local (0xbe2f0)
forces the /GX EH frame -> "eh" (== the engine_label_stubs base+/GX it came from).

### imagepool
ONE original TU (dossier 0x174e90-0x177476): absorbed rezimage/scanlinesurface/
imagevflip/scanlinesurfacesave/imagerectfill. /GX (CFileIO/CFile stack dtors).

### gruntstaterec
Carved out of streamrecordloaders (operation REHOME D8): the grunt-state /
projectile record loaders each sit at their own far-away .text block (separate objs).
(The ex "triggerloadrec" unit is GONE: its one body 0x9bb0 was
CActionOptionsMenuBar::Deserialize - re-homed into actionoptionsmenubar,
whose obj it retail-contiguously ends.)

### bsecobj10fctor
The CButeSection ctor (0x170210) has 8 destructible embedded members (CString +
CButeNode streams) -> /GX EH frame + 8-trylevel member-unwind state machine.

### spotlightactreg
CBSecObj10f::CBSecObj10f (0x16f680) - orphan 3-byte empty-ctor COMDAT (returns this);
dedicated 1-fn TU so CButeSection's ctor CALL binds to the real RVA.
LogicActRegistrars.cpp + LogicActReg.cpp DRAINED + DELETED (REHOME D1): each was a
pure holding TU conflating unrelated per-class activation registrars. Each
Construct+Register pair split into its own RVA-block .cpp, named by the CRACKED
owner class where the xref pinned it (registry table -> the class that walks it):
0x646188 -> CSpotLight       (SpotLightActReg.cpp)
0x646250 -> CPathHazard      (PathHazardActReg.cpp)
0x6514d8 -> CGruntVoice      (GruntVoiceActReg.cpp)
0x62bfa0 -> CCursorSnapSprite (CursorSnapActReg.cpp)
0x646010 -> uncracked (scattered leaf pool) -> LogicActReg646010.cpp

### arrayserialize
eh: the retail-kept ??1?$CArray@PAUPLAYLISTINFOSTRUCT... COMDAT (0x39f20) carries a
/GX EH frame (push -1 / push 0x5d9b08 / fs:0), so the instantiation host compiles /GX.

### titleappstart
CTitleApp::OnStart (0xf9880) - title-sequence starter, split out of the ex
AppHelpers.cpp holding TU. Its orphan-mates re-homed to their real objs: Handle
(0xb4cb0) -> CUFO::Method_b4cb0 in Ufo.cpp; Unmatched_be030 (0xbe030) ->
NetLobby::Init_2522 in LobbyDialogs.cpp. OnStart's CAttract owner is deferred
(+0x1b8 layout conflict). No destructible locals -> /base.

### CRect
The rectangle functions at 0x29ac0, 0x8c380, 0x115b30, and 0x17b500 are emitted
MFC 4.2 `CRect` inline bodies, not a WAP32-owned rectangle class or source TU.
Consumers use the SDK definition through `<MfcWin.h>`; the retail bodies are
classified as MFC rather than reconstructed as a project-local shadow.

### debugprintf
ONE original TU (dossier 0x1832d0 pocket, oracle-proven): absorbed rangeset.

### grunttoytimesprite
trace-discovered leaf dtors re-homed from src/Stub/Discovered.cpp (matcher-1):

### customleveldlg
matcher-3: custom-level dialog populate/select (thiscall, magic-static CString + walk).

### modeobjinit
matcher-3: game-mode/object initializer (four owned sub-objects + /GX teardown).

## Static-init band cuts (the GruntDirStatics oracle, 2026-08-06)

A header-defined internal-linkage object family with ctors (the nine
GruntDirectionCell statics, include/Gruntz/GruntDirStatics.h) is emitted at
the INCLUDE POINT of every including TU: nine .data cells + nine `$E`
funclets, contiguously, at the TU's head (includes-at-top convention; the
one XCU-buddy-decidable case confirmed head). Each band is therefore a TU
CUT with an orientation: the band's first byte starts a compiland. One band
per TU is unconditional (a TU cannot emit two copies), so band COUNT floors
the TU count and band order fixes link order. Full derivation:
docs/data-attribution.md (access-map section) + the sidecar
config/retail/data_compgen.tsv (class=copy) (rva-suffixed names = holding-unit copies
awaiting their split).

Cuts inside CURRENT units (band setter rvas; each starts a new TU, span
runs to the next cut):
  gruntarrivalscan (>=16 TUs): 0x0ec3c0 0x0ec9e0 0x0ed740 0x0ee550 0x0ef400
    0x0efe80 0x0f0b00 0x0f19c0 0x0f2440 0x0f2870 0x0f33f0 0x0f4040 0x0f5e40
    0x0f6f10 0x0f7ae0 0x0f7f90
  battlezmapconfig (>=5, up to 8 with edge bands): 0x0349c0 0x034e20
    0x0352a0 0x0355f0 0x035c60 (+edges 0x024b10 0x031360 0x032a30)
  dialogs (2 copies -> >=2 TUs), demo (2), statusbartabbuilders (3),
  ghidra-carved regions (3 bands).

Generalization (unswept): the CRT `$XCU` table enumerates EVERY TU with any
dynamic-initialized static - zero-separated per-obj pointer groups in link
order, each pointing at that TU's funclet band. Walking the whole table
drops cut points across the binary, not just where this family traveled.

### Executed (2026-08-07)

The band cuts above are DONE: 26 new compilands on main.
  battlezmapconfig -> +6 (battlezunitstep, battlezspecialanim, battlezspawncheck,
    battlezrepath, battlezreserveplace, battlezretarget)
  gruntarrivalscan -> +15 (one AI-step behavior each: bricklayerstep, wanderstep,
    reticlescan, chargestep, arrivalupdate, goosuckerstep, defensealt,
    arrivalneighbor, defensestep, diggerstep, scantarget, phasestep, seektarget,
    peertracking, defenselean)
  dialogs -> +1 (battlezdlgcolors), demo -> +1 (dircellmethods),
  statusbartabbuilders -> +2 (sbi_sidetab, sbi_statztabarrow)
Names are DESCRIPTIVE placeholders from each TU's reconstructed content, not
recovered compiland names (retail's own names are unknown; see the naming rule
in [[fossil-tu-dissolution-method]]).

Procedure that worked, per cut: move the annotated blocks whose RVA falls in
the cut's span into a new .cpp with the parent's prelude (includes + file-local
macros the moved code uses), add `#include <Gruntz/GruntDirStatics.h>` so the
new TU emits its own copy, add the [[unit]] entry right after the parent (link
order = band order), canonicalize that copy's nine sidecar pins to the new
unit, then build; the function census (`config/retail/gruntz_functions.tsv`)
keys on RVAs, so the cross-unit move passes on its own and the rows re-home on
the next `gruntz verify check --tier full`.

Traps hit: (1) blocks must be re-emitted DATA-first then ascending-RVA - the
tu-order gate is per-TU and a plain source-order copy trips it; (2) an
interleaved bare `RVA_COMPGEN` between a label and its body must stay GLUED to
that block or the label orphans; (3) a global used by a moved fn needs its
`extern` in the owner header (g_spawnCfg).

Still suffixed in the sidecar (uncertain, by design): the ghidra-region copies
(3 - no TU exists to split; they need TUs carved for unreconstructed regions)
and the split-half families whose halves straddle a cross-TU interleaver
(0x229eb8, 0x22b588, 0x245168, 0x22c1c0, 0x244f50).

### The generalized oracle: the CRT `$XCU` table (2026-08-07)

The direction-cell family is one tracer; the CRT initializer table is the
GENERAL one. Every TU with any dynamic-initialized static contributes its own
group of funclet pointers to `.CRT$XCU`, groups concatenated in link order, so
walking the table enumerates those TUs and points at each one's funclet band
(= its head). In GRUNTZ.EXE the table is 0x205e54..0x20a3dc = **115 groups**.

Six claimed units carried >1 group (= >1 compiland); all six cuts were
corroborated independently by a CLASS boundary at the same address, and all
six are now SPLIT (commit aac379521):
    play -> +gruntzplayer +playassetload   savegame -> +savegamedialogs
    netcmdslot -> +netsessionmgr           netlobbydialogs -> +multihelpdlg
The remaining 13 groups attribute to the `retail` pseudo-unit - unreconstructed
regions where no TU exists to split; they mark future TU births, not defects.

Measured effect on the linker-order metric (config/cleanliness/tu-order-baseline.tsv):
    intra-order violations   70 -> 44      TUs carrying them   11 -> 9
    interleaving TU-pairs    95 -> 89
`Dialogs` (6) and `Play` (20) fell to zero: a merged file cannot be ordered
ascending when retail interleaved other objects between its parts, so those
violations were never real defects - they were the merge showing through.
Still-open intra offenders (GruntzMgr 22, GameLevel 10, Fader 5, ...) carry no
initializer group, so this oracle cannot cut them; they need xref/class
analysis or another tracer.

New traps beyond the band-cut list: a namespace-scoped TU (LobbyDialogs.cpp)
has INDENTED labels a column-0 splitter silently misses - check the annotated-
block count against the Model before trusting a split; and copying a
parent's prelude drags its file-local enums/constants along, which the
cleanliness ratchet catches (delete what the new TU does not use).
