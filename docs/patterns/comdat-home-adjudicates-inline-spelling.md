# The kept COMDAT's home adjudicates a dual-spelled inline: three retail proofs and one depth-bounded emission rule

tags: cpp:inline cpp:ctor cpp:call | asm:call | topic:codegen-idiom topic:layout
confidence: 9/10 (every claim below is a 2026-08-15 A/B on the pinned cl 5.0 against
the real tree; the dissolution campaign's six `*Inline.h` devices are the corpus)
symptoms: one body appears in retail as an out-of-line copy AND as expansions in
other TUs; the reconstruction spells it twice (an `.inl` view plus an OOL twin);
"dissolve it into one in-class definition" either loses the retail RVA (no obj
emits the COMDAT) or craters the calling TU (cl expands what retail called)

## The adjudication, in the order that decides fastest

1. **Who emits the keeper?** If the retail copy sits in a TU with NO local
   reference to it (no intra-band caller, no data reloc), that TU can only have
   emitted it as a plain out-of-line definition. An in-class/inline spelling
   CANNOT reproduce that copy - a TU that merely sees an inline definition and
   never references it emits nothing.
   * `?Find@CDDrawWorkerCache` 0x9cab0: last function of streamrecordloaders
     (seq 107), zero intra-band referrers -> strong OOL def in
     StreamRecordLoaders.cpp. Measured confirmation: with the body in-class,
     the four point-logic ctors' declined nested sites left `U ?Find@...` in
     statedispatch/waypoint/guardpoint/lightfx.obj and NO obj defined it -
     0x9cab0 unhomed, the exact lost-COMDAT failure two-shapes warns about.
   * Same theorem: `?BuildLogicTypeTable` 0x8a40 (userlogic, seq 2, no local
     caller), `?SetZ@CMotionState` 0x58ca0 / `??0CUserLogic@..PAU..` 0x58cd0
     (gruntcombat, seq 68, no local callers).
2. **First-definer versus the caller seqs** (link-order-first-definer): a retail
   caller in an obj EARLIER than the keeper proves that caller's TU never saw a
   definition. CActionArea (actionarea, seq 0) calls 0x8a40 homed at seq 2;
   CGrunt::CGrunt (grunt, 63) calls 0x58ca0/0x58bc0/0x58cd0 homed at 68.
3. **Budget-shape tests on the expansion side.** If the call/expand split runs
   INVERSE to caller size (retail's ~45 mostly-large ctors CALL 0x8a40 while
   the four small point-logic ctors expand the body), no budget arithmetic
   produces it - it is per-TU visibility, i.e. the devs' own include split.
   A per-SITE mixed shape inside one function is the opposite tell:
   `??0CLightFx` (0x9cf00) expands its first Find site and calls the other two
   - only sequential budget spend does that, so lightfx's TU DID see Find's
   body (and the .inl device therefore belongs in LightFx.cpp's include set).

## Emission on decline is DEPTH-BOUNDED (measured, corrects two older claims)

A declined site makes cl emit the callee's COMDAT only when the decline happens
at shallow inline depth. Both cells measured today on real TUs:

| declined site | depth | COMDAT emitted? |
|---|---|---|
| `??0AnimWorkerObj@..HH@Z` inside the creators' expansion of `??0CGameObject(.., EInlineBase)` (wwdobjmgr) | 2 | **yes** - `T` in wwdobjmgr.obj, homed exactly where retail keeps 0x15b300 |
| `?Find@CDDrawWorkerCache` inside ctor -> RegisterLogicTypesOnce -> BuildLogicTypeTable (statedispatch/waypoint/guardpoint/lightfx) | 3 | **no** - `U`, call planted, nothing emitted |

Corrections this measurement forces:
* zero-emission-statements-cross-the-ob1-cb-exemption.md's in-tree note ("one
  ASSERT ... put 0x9cab0 at 100% emitted by waypoint.obj") does NOT reproduce on
  the current tree: at depth 3 the decliner emits nothing. Keep that doc's cb
  mechanism (proven, reused below); its emission anecdote came from a TU state
  that no longer exists.
* link-order-first-definer-adjudicates-comdat-homes.md's premise "any TU that
  sees a header-inline definition and declines to expand it" emits the copy is
  true at depth <= 2 only. Its worked conclusions all stand (they concluded
  OOL-at-keeper, which the depth-3 rule reaches too).
* nested-ctor-call-vs-expansion-is-a-tu-visibility-split.md's "making them
  inline makes ... their COMDATs vanish from every base obj" is not universal:
  wwdobjmgr's depth-2 decline of the visible `??0AnimWorkerObj` DID emit and
  home the COMDAT correctly.  And its "a base sub-object ... is not
  [steerable]" is refuted by the CResolveNode row below - a TAGGED base-ctor
  overload in the pinned body's mem-init list steers a base sub-object fine;
  what stays unsteerable is a site inside the SHARED helper text
  (AttachToOwner's `new AnimWorkerObj`).

## Budget slices at sibling ctor sites are COUPLED - a rejected site spends nothing

Inside one expansion, rejecting site A frees A's cb for the sites after it.
Measured on the creators (wwdobjmgr), natural cb: `??0CResolveNode` 84-90,
`??0AnimWorkerObj` 56-58 (both via `inline-model --measure-cb`):

| CResolveNode spelling | CResolveNode outcome | AnimWorkerObj outcome |
|---|---|---|
| visible, natural cb | expanded (spends ~87) | declined + emitted (retail's call shape) |
| visible, +1..2 ASSERT (cb ~93-96) | still expanded | declined |
| out of line (spends 0) | called (retail) | **expanded** - the freed slice absorbs it |

So retail's both-called creators are unreachable from any two-visible-bodies
state: one of the pair must be genuinely invisible (OOL elsewhere), and the
other's decline then needs the slice-coupling to land the same way retail's
did. Titrate before concluding; do not stack ASSERTs past ~2 - if the gap is
bigger than a plausible compiled-out statement, the body was not visible.

## What the six-device dissolution campaign landed (2026-08-15)

* DISSOLVED `MotionStateSetZInline.h` + `MovingLogicGruntScaleInline.h`: SetZ
  stays OOL in GruntCombat.cpp (keeper theorem, cb 48-50 measured); the
  CMovingLogic owner-ctor pair became true two-entity siblings in
  MovingLogic.h - the untagged (CProjectile) ctor spells the three m_maxStep
  stores, the EGruntScale (CGrunt) ctor calls SetZ, both share
  InitOwner(0.001)/BeginMotion. The two fabricated named 0.001 globals
  (g_motionZScale, address-named g_val_1e9738) dissolved into the one literal:
  each expanding TU pools its own copy (retail 0x1eaa88 / 0x1e9738), for the
  FP oracle to claim. Byte-neutral at every touched function.
* DISSOLVED `ResolveNodeCtorInline.h`: OOL keeper 0x15b2c0 in WwdObjMgr.cpp +
  the tagged inline sibling `CResolveNode(.., EInlineSeed)` defined in
  WwdFactoryObject.h (a header only the two Wwd TUs include), used by
  0x15b390's init list. two-shapes ctor recipe; byte-exact on both sides.
* SURVIVORS as of 2026-08-15 (the justifications below are SUPERSEDED - see the
  2026-08-22 audit section): `AnimWorkerObjCtorInline.h`,
  `DDrawWorkerCacheFindInline.h`, `LogicTypeTableInline.h`.

## NEGATIVE CONTROL 2026-08-21: LightFx must NOT opt into the Find inline

`DDrawWorkerCacheFindInline.h`'s own header comment reads as if `CLightFx`'s
constructor is the intended consumer ("is why CLightFx's ctor can afford
exactly ONE of its three sites"). It is not - the ctor is the WITNESS for the
mixed shape, not a TU that should include the header.

The byte evidence that invites the change is real: a whole-image screen of
`?Find@CDDrawWorkerCache@@QAEPAVCObject@@PBD@Z` against
`?Lookup@CMapStringToOb@@QBEHPBDAAPAVCObject@@@Z` finds exactly ONE unit where
the counts disagree - `lightfx`, base 3 Find / 1 Lookup against retail's
2 Find / 2 Lookup - and `walls semdiff` names it as a referent REPLACE, which
normally means a wrong callee.

Adding `#include <DDrawMgr/DDrawWorkerCacheFindInline.h>` to `src/Gruntz/LightFx.cpp`
does flip that site to `Lookup`, and takes `??0CLightFx@@QAE@PAUCGameObject@@@Z`
from 96.46 to **0.00**: with the body visible cl 5.0 stops emitting the
constructor's COMDAT at all. Same failure mode as the depth-3 no-emission
result above, one level up. Reverted.

Read-across: on this device a referent REPLACE between a wrapper and the
function it wraps is an inline-VISIBILITY reading, and the visibility split is
adjudicated by which COMDATs each obj still HOMES - never by the call alone.

## 2026-08-22 RETEST: the "no emitter" premise is FALSE, and the device is a workaround

The user challenged the opt-in-inline device on principle ("we shouldn't have
an opt-in variant; this is usually the inline limit in the function itself").
Tested directly on the smallest instance - `CDDrawWorkerCache::Find` collapsed
to ONE entity: an in-class body in `DDrawWorkerCache.h` carrying
`RVA(0x0009cab0, 0x23)`, the out-of-line definition in StreamRecordLoaders.cpp
deleted, both `DDrawWorkerCacheFindInline.h` includes removed.

Result: **0x9cab0 was still emitted - by `guardpoint.obj` - and still scored
100.00 EXACT.** So a TU that declines the expansion DOES emit the COMDAT, and
the header's "an in-class body leaves 0x9cab0 with NO emitter" justification,
plus the depth-3 "emits nothing" cell above, do not hold for the
single-entity configuration (they were measured with the split in place, where
the decliners had another definition available to call).

What the split actually buys is narrower: it keeps `lightfx.cpp` from SEEING
the body. With one visible body `??0CLightFx@@QAE@PAUCGameObject@@@Z` goes
94.87 -> **0.00** (plus four sub-0.1 ripples in levelplane/wwdobjmgr), because
cl expands a different number of its three `Find` tests than retail did.

That makes the device a WORKAROUND FOR CALLER-SIDE MODELLING ERROR, not a
reconstruction of era structure - retail's own lightfx expands one of the
three, so era source had visibility in that TU and the header is the wrong
shape for it; it merely scores better while our ctor's `cb` differs from
retail's. REMOVAL CONDITION: model `CLightFx::CLightFx` accurately enough that
its own budget declines sites 2 and 3, and all three entities collapse to one
visible body. Until then the device stays, labelled, with this measurement.

## 2026-08-22 AUDIT: every *Inline.h device collapsed and measured

The user's principle objection ("a header that makes one function inline-visible
to SOME TUs and not others is not something a developer writes") was tested on
all ten remaining devices by the same method: collapse to ONE entity - the body
moved in-class or out-of-class-inline into the owning class header carrying its
`RVA(...)` pin, the separate out-of-line definition deleted, every include of
the opt-in header removed, the header deleted - then a full `gruntz build`, and
read (a) does the pinned RVA still get an emitter and which unit emits it, and
(b) which functions moved.

**Three devices were not devices at all and are GONE.** Where the helper had NO
out-of-line twin, the opt-in header was only narrowing visibility for no
structural reason:

| collapsed | model | measured |
|---|---|---|
| `DDrawSubWorkerDirtyRectInline.h` | body to the bottom of DDrawSurfacePair.h | CPlay::StepScroll 88.03 -> 100.00 EXACT, HandleDragMove 93.58 -> 97.41, CDDrawWorker::GetMemoryUsage -> 100.00 |
| `GruntArrivalRerollInline.h` | three CGrunt members in Grunt.h | +23.18 total, exact unchanged |
| `AmbientSoundInline.h` | CAmbientSound::ScaleVolume in AmbientSound.h | byte-neutral (0 changed rows) |

Two of the three IMPROVED the score, which is the direct refutation: the device
was costing match, not buying it.

**The "an in-class body leaves the RVA with NO emitter" premise is FALSE in
three of the seven survivors.** Collapsed, `Find` was still homed (by
guardpoint, then droppedobject) at 100.00 EXACT, `SoundCue::PlayIfElapsed`
rehomed bootystateactivate -> sbi_rectonly at 100.00 EXACT, and
`CUserLogic::BuildLogicTypeTable` was homed by actionarea - at 65.58 alone, and
at **100.00 EXACT** when the Find device is collapsed at the same time. The
keeper theorem holds only where no TU declines at all; that is an empirical
per-case fact, not a rule, and it must be re-measured rather than cited.

**What the surviving devices actually buy is hiding the body from the TUs retail
CALLS it from**, whose ctors/callers expand it once they can see it. That is a
caller-side modelling error, so every survivor now carries its measurement and a
REMOVAL CONDITION in its own header:

| kept | pinned RVA under collapse | the cost |
|---|---|---|
| `AnimWorkerObjCtorInline.h` | 0x15b300 loses every emitter (unique-names FATAL) | the three wwdobjmgr creators expand it: 100.00 -> 86.14 / 84.92 / 83.04 plus ten unwind funclets |
| `LogicTypeTableInline.h` | homed by actionarea | ~11 point-logic ctors expand it: CBehindCandy 99.83 -> 66.44, CTileTriggerTransition 100.00 -> 44.76 (-676) |
| `SoundCueInline.h` | rehomed, 100.00 EXACT | ~15 callers expand it: CRezImage::FillRectAt 100.00 -> 66.44, CSBI_MenuItem::Render -> 74.04 (-221, -12 exact) |
| `AniElementInline.h` | 0x6b270 loses every emitter | CAniAdvanceCursor::Advance 92.75 -> 77.28 (-118, -2 exact) |
| `GruntMovementInline.h` | 0x29a80 and 0x343f0 both lose every emitter | -122 and -111; all 86 Grunt.h TUs expand, none declines |
| `FreeNodePoolInline.h` | 0x311b0 loses every emitter | ~20 grunt/battlez steps drop 1-10 points (-175) |
| `AniAdvanceCursorInline.h` | no retail entity exists | -31.55, -3 exact of pure /O2 ripple; blocked by the LEDGER, not the bytes (see below) |

**COUPLING.** `LogicTypeTableInline.h` and `DDrawWorkerCacheFindInline.h` hide
the same body from the same TUs at two depths. Collapsed together, both pinned
RVAs land at 100.00 EXACT in earlier units, but ??0CLightFx goes 94.87 -> 0.00
and the point-logic ctors drop further (-815 total). Neither can be adjudicated
without the other.

**A LEDGER effect that constrains this whole class of cleanup.** Collapsing a
device rewrites its call sites, which changes those functions' per-function
source hashes, so `best` RESETS to the current value. If a sibling is currently
dipped on unrelated ripple, the rewrite BANKS the dip. That is what blocks
`AniAdvanceCursorInline.h`: CTeleporter::Update is banked at 100.00 but sits at
98.91 on ripple from other lanes, and the collapse would reset it. Do a
call-site-rewriting collapse in a build where the touched TUs read their banked
current values.

**Mid-file includes are NOT part of the device.** Twenty-two `#include
<...Inline.h>` directives sat below code in fifteen TUs, gating visibility by
line number as well as by TU. Hoisting all of them into the canonical include
block moved exactly one function by +0.005 - the placement was lazy, not
load-bearing.

**`FreeNodePoolInline.h` is a different animal and should not be read as a
visibility device.** Retail's Push split is per SITE: BattlezUnitStep.cpp calls
0x311b0 through `RECYCLE_GRUNT_COORDS_IF_ANY` and expands it through
`RECYCLE_GRUNT_COORDS_INLINE_PUSH_IF_ANY` a few lines apart. A visibility header
cannot express per-site at all; two spellings can, so two entities are what the
evidence supports.

## 2026-08-22 AUDIT: the macro side of the same device, collapsed the same way

`FreeNodePoolInline.h` survived the *Inline.h sweep because retail's `Push`
split is per SITE, and the thing that expressed the per-site split was
`include/Gruntz/GruntCoordRecycleMacros.h` - **ten macro variants of one loop,
named for the inline expansion each produced** (`..._POSITION_INLINE_POOL_IF_ANY`
and friends) across 50 sites in 15 TUs. Same audit method as the headers: fold
each pair, full build, read the per-function rows.

**Four axes; only two were real.** The census is the whole finding:

| axis | fold | measured | verdict |
|---|---|---|---|
| walk: `m_coordList.GetHeadPosition()`/`GetNext` vs `CoordHead()`/`m_next` | rewrite the 3 POSITION macros to the node walk | **0 rows moved** (6 sites) | DELETED |
| guard: `if (CoordCount() != 0)` inside the macro (`*_IF_ANY`) | hoist to all 21 call sites | **0 rows moved** | DELETED |
| push: `g_coordPool.Push(...)` (call 0x311b0) vs expanded | collapse the call arm into the expansion | **-26.85 over 9 fns** (CheckQueuedSpawnTile -6.61, RouteUnitTo -4.51, PathToNearestCandidate -3.94) | KEPT |
| walk: `CGruntCoordList::NextData` (0x29a30) vs inlined step | collapse into the inline walk | **-16.37 over 4 fns** (TrackAssignedEnemy -9.43, Step -4.10) | KEPT |

Ten variants to three, net compare effect one unrelated row (+0.0089).

**The walk axis was never a codegen choice, and the reason generalizes.**
`<MfcNoInline.h>` cannot suppress `CPtrList::GetNext`: `afxcoll.inl` is parsed
inside `<Mfc.h>` itself, so the `#undef _AFX_ENABLE_INLINES` that follows is a
no-op for every `afx`/`afxcoll` accessor - the same mechanism that
`mfcnoinline-is-inert-when-the-own-header-pulls-mfcwin.md` records for
`afxwin1.inl`. `CoordHead()` is `MfcNodeFromPosition<CoordNode>(GetHeadPosition())`
and `node->m_next` is `GetNext`'s body, so the two spellings are the same IL.
**A macro variant that exists to choose between an MFC accessor and its
hand-transcribed body is always deletable.** `CGrunt::SetEntrancePos` (0x4d060,
100.00 EXACT) is the golden reference for the loop retail actually emits:
`GetCount` test, `GetHeadPosition`, then the hoisted `m_freeHead` splice.

**A guard hidden in a macro NAME is worse than a codegen-named macro.**
`RECYCLE_GRUNT_COORDS_IF_ANY` buried a live conditional in an identifier, and
the same tree already wrote `if (unit->CoordCount() != 0) { RECYCLE_... }`
explicitly at other sites - one source construct with two appearances. Hoisting
it is byte-free and makes the branch readable; check the call sites for
dangling-`else` first (all 21 here were plain statements).

**A spelling that is byte-identical at every site can still be blocked by the
LEDGER.** The expanded arm can be written `PushFreeNode(&g_coordPool, ...)`
instead of the three-line splice; it is identical code at all 24 sites and
*zero* macro-host functions move. But losing the `slot` local rotates cl 5.0's
allocation cursor for the NEXT function in `Projectile.cpp`, so
`CBoomerang::AdvanceMotion` goes 86.25 -> **84.58** - a fresh MAX regression on
a function whose source never changed, reproducible across two different TU
states. The longhand stays with the measurement and a removal condition
(break AdvanceMotion's regalloc wall first). Related: a single unused
`#include <Gruntz/FreeNodePoolInline.h>` in `GruntSteps.cpp` restores
`CGrunt::StepCompassMove` to its banked 63.2990 from 62.0438 - a real
declaration-count-window datum for that wall, and exactly the fitted artifact
that must NOT be left in the tree.
