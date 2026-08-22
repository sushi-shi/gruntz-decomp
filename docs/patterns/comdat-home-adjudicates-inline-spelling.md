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
* SURVIVORS (dev-source duplication is the faithful model; each carries the
  refutation in its header comment): `AnimWorkerObjCtorInline.h` (the call and
  the expansion sit inside ONE spelling of CGameObject::AttachToOwner, and the
  slice-coupling row above closes every single-spelling escape),
  `DDrawWorkerCacheFindInline.h` (keeper theorem + depth-3 no-emission; the
  ASSERT inside both twins is what lets the point ctors decline at all),
  `LogicTypeTableInline.h` (keeper theorem + first-definer + inverse-size).

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
