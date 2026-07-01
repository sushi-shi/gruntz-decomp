# Class-metadata sweep log

Per-class `SIZE`/`SIZE_UNKNOWN`/`VTBL` annotation sweep (infra: `include/rva.h`,
tests `python -m gruntz.match.class_sizes` + `class_vtables`). Goal: every class
carries a size annotation and every vtabled class a `VTBL` catalog entry —
**per-class verified** because annotating a class in a hot header can reschedule
a neighbour's MSVC5 codegen (measured on the infra proof: `SIZE(CGrunt)` cost
`StepCompassMove` -0.52%). Procedure: annotate in small groups, `gruntz build` +
snapshot-diff every function's fuzzy%; a NEUTRAL group is kept, a REGRESSING
class is reverted and logged here as a hot-header casualty.

This file records the SKIPPED classes (hot-header casualties + un-catalogable
vtables) so the next sweep `@early-stop`s on recognition instead of re-grinding.

---

## Net module — pilot (2026-07-01)

Scope: classes defined in `include/Net/*.h` + `src/Net/*.cpp` (CNetMgr already
had SIZE_UNKNOWN+VTBL from the infra proof; skipped).

Coverage delta (whole-tree counters, Net worklist drained):
- SIZE: 55/3264 → 118/3264 annotated names (**63 Net classes annotated**; Net
  SIZE violators 63 → 0).
- VTBL: Net vtable violators 5 → 3 (2 catalogued, 3 skipped below).

Annotated: **57 SIZE_UNKNOWN + 6 SIZE(exact) + 2 VTBL**.
- Exact `SIZE`: `CNetCmdSlot2`(0x64), `CNetResyncEntry`(0x410),
  `CNetSession2`(0x20bb0), `CNetJoinPacket`(0x28), `CNetConfigBlob`(0x11c),
  `CMsgPacket`(0x38), `CNetCmdSlot`(0x64), `CNetCmdBuf`(0x238),
  `CNetSession`(0x20bb0) — array-element strides, RE'd fixed packets, or a
  byte-matched struct-copy prove these are the *complete* retail object size.
  (Note: `SIZE(T,N)` asserts `sizeof(myStruct)==N`, i.e. it is self-consistent —
  it only encodes a real retail size when the modeled struct is COMPLETE. Every
  partial pad-to-last-touched-field view therefore takes `SIZE_UNKNOWN`.)
- `VTBL`: `CNetPlayerListNode`(0x5f0760), `CNetSessionNode`(0x5f0778) — own
  most-derived vtables, real-polymorphic in NetSessionNode.cpp; those RVAs were
  not yet named in symbol_names.csv, so the catalog rows are new (no collision).

### Hot-header casualties: NONE.
All 63 SIZE + 2 VTBL annotations were matching-NEUTRAL: three build groups
(A = all `src/Net/*.cpp` local classes + LatencyList.h; B1 = `NetMgr.h`
110–347; B2 = `NetMgr.h` 422–962), each snapshot-diffed across all 3394
functions → **0 REGRESS, 0 IMPROVE** every time. No class annotation had to be
reverted. `NetMgr.h` is a genuinely hot header (included by every Net TU plus
several Gruntz TUs — CMulti/Dialogs/NetGameDlg), yet the no-code `char[1]`
typedef the macros lower to under MSVC5 perturbed nothing here.

### VTBL skipped (un-catalogable, NOT casualties — logged for the final sweep):
- `CNetNodeBase` (NetSessionNode.cpp) and `InterfaceObjectBase`
  (InterfaceObject.cpp): base subobjects whose transient construction vtable is
  the SHARED CObject-base dtor vtable **0x5e8cb4**, already catalogued as
  `?g_severusBaseDtorVtbl@@3PAXA`. A `VTBL(...,0x5e8cb4)` would collide on that
  rva (dup-DATA guard) and mis-attribute one shared vtable to a modeling-base
  name. These are the KEEP-HAND-ROLLED / shared-base-vtable exception.
- `CNetPlayerObj` (NetMgr.h): a slot-dispatch modeling view — its virtuals are
  declared-but-undefined so cl emits no vtable in this TU, and its concrete
  retail vtable RVA is not confidently pinned. Left un-catalogued (no wrong RVA
  guessed) pending the matcher that pins its concrete class.

---

## SBI / status-bar family (2026-07-01)

Scope: classes in `include/Gruntz/SBI_*.h`, `CStatusBarMgr.h`, `StatusBarItem.h`,
`MenuItem.h`, `MenuItem2.h`, `MenuPage.h`, and in `src/Gruntz/SBI_*.cpp` /
`StatusBar*.cpp` / `CStatusBarMgr*.cpp` / `MenuPage.cpp`. Excluded (per matcher
instructions): `SBI_Image.cpp` (the CSBI_Image stub — do-not-touch),
`ApiCallers.cpp`, `RezSync.cpp`. `SBI_RectOnly.cpp` was annotate-only (its 54
`.cpp`-local classes got SIZE, no function bodies touched).

Coverage delta (whole-tree SIZE counter):
- SIZE: 118/3292 → 301/3292 annotated names (**183 SBI-family names annotated**;
  SBI SIZE violators drained to 0 except the do-not-touch stub file, below).
- VTBL: no new catalog rows (all SBI vtable violators are the un-catalogable
  modeling-view case, below). VTBL total unchanged (310 not-catalogued).

Annotated: **183 SIZE_UNKNOWN + 0 SIZE(exact) + 0 VTBL**.
- ALL 183 are `SIZE_UNKNOWN`: no SBI class was a provably-complete retail object
  (no array-element stride / RE'd fixed packet / byte-matched struct-copy in the
  models — they are pad-to-last-touched-field views), so per doctrine none took an
  exact `SIZE`. Exact-size upgrades left to the owning matcher.
- Two build groups: **G1** = all SBI `.cpp`-local classes (independent TUs;
  CStatusBarMgr/CStatusBarMgrGetItem/CStatusBarSpriteActs/SBI_ImageSet/SBI_MenuItem/
  SBI_RectOnly(+Eh)/SBI_SideTab(+Build)/SBI_StatzTabArrowEh/SBI_ImageSetAniEh/
  SBI_WellGooEh/SBI_TabzDialogEh/StatusBarGameMenu/StatusBarUpdaters/MenuPage/
  SBI_ImageEh). **G2** = all 10 SBI headers (each also gained `#include <rva.h>`).
- Cross-file dedup notes: `CSBI_Image` (rep is the excluded SBI_Image.cpp) was
  annotated instead at its per-TU view in `SBI_ImageEh.cpp`; `CMenuPage` (rep is
  the out-of-scope CAttract.h) was annotated at its `MenuPage.h` def. Names whose
  first-seen rep is a non-SBI file (CSprite/CTileGrid/CGameReg/CMenuMap/CMenuNode/
  SbView/CStatusBarSurface/EngineLabelBacklog/CButeMgr) were NOT annotated here —
  they belong to their owning module's sweep.

### Hot-header casualties: NONE.
Both groups snapshot-diffed across all 3394 functions → **0 REGRESS**. Net effect
was matching-NEUTRAL-or-POSITIVE: **+3 exact fns / 4 improved**
(`CSBI_RectOnly::LoadMainStatusBarSprite` 88.6→95.6, `CSBI_MenuItem::DecCounter`
92.0→100, `CMenuPage::Layout` 99.98→100, `CMenuPage::LayoutOne` 99.98→100) — a
scoring/reloc-naming side effect of completing the annotated types; all kept.
Adding `#include <rva.h>` to the 10 SBI headers (none had it) perturbed nothing.

### SIZE deferred (do-not-touch stub, NOT casualties):
- `CImageCfgValue` / `CImageCfgRecord` / `CImageCfgMap` / `CImageCfgHost` — defined
  ONLY in `src/Gruntz/SBI_Image.cpp`, the CSBI_Image stub file this matcher was
  instructed not to touch. Left for that file's owner. (`CStatusBarSurface` in
  SpriteResource.cpp and `StatusBarItem` in Stub/Backlog.cpp are non-SBI files.)

### VTBL skipped (un-catalogable modeling views, NOT casualties — for final sweep):
Every SBI vtable violator is a **declared-but-undefined polymorphic view struct**
used only for a `mov eax,[this]; call [eax+slot]` slot dispatch (the WAP `((View*)
this)->Slot()` idiom). cl emits NO `??_7<View>@@6B@` for a class with no defined
virtuals, and the concrete retail vtable they alias belongs to an engine/MFC class
whose real name is unknown — so a `VTBL(...)` would bind our name to a datum that
is not that class's vtable. This is the same case as the Net pilot's `CNetPlayerObj`.
Skipped: `CSBI` (CStatusBarMgr.h), `CMenuItemView` (MenuItem.h), `CImageSetStream`
+ `CStatzSelf` (SBI_WarlordHead.h / SBI_StatzTabGruntBar.h), `CMiArchive` / `CMiSelf`
(SBI_MenuItem.cpp), `CTabWidget` (StatusBarUpdaters.cpp), and the SBI_RectOnly.cpp
views `CSbiStream` / `CSbiSprite` / `CSbiRect` / `CSbiSlotPtr` / `CSbiNotifyTarget` /
`CSbiNotifyPayload` / `CSbiGaugeNotify` / `CSbiSeqObj` / `CSbiHiWidget`.
The SBI *leaf* classes (CSBI_RectOnly / CSBI_Image / CSBI_ImageSet / CSBI_MenuItem /
CSBI_WarlordHead / CSBI_WellGoo / CSBI_GruntMachine / CSBI_StatzTabArrow /
CSBI_ImageSetAni / CSBI_SideTab / CSBI_StatzTabGruntBar) are already catalogued —
their RTTI vtables are in config/vtable_names.csv AND they carry the manual
vptr-stamp device — so no VTBL was added (would collide on the named rva).
## CDDraw family (2026-07-01)

Scope: classes defined in `include/Gruntz/CDDraw{ShadeBlit,SurfacePair}.h` +
`CDirectDrawMgr.h`, and in `src/Gruntz/CDDraw*.cpp` / `CDirectDrawMgr.cpp` /
`CDDSurfaceDtor.cpp` (18 src + 3 headers). All src files already `#include <rva.h>`.

Coverage delta (whole-tree counters, CDDraw worklist drained):
- SIZE: 118/3296 → 253/3296 annotated names (**135 CDDraw classes annotated**;
  CDDraw SIZE violators 135 → 0). All `SIZE_UNKNOWN` (every class is a partial
  pad-to-last-touched-field / interface / view; none provably == the full retail
  object, so no exact `SIZE` upgrades).
- VTBL: CDDraw vtable violators 35 → 34 (1 catalogued, 34 skipped below).
  - `VTBL(CDDrawSurfacePair, 0x005eff30)` — the one clean own most-derived
    real-polymorphic vtable: pure C++ virtuals (no manual `g_*Vtbl` stamp),
    in-TU dtor (0x1590f0) so cl emits `??_7CDDrawSurfacePair@@6B@`; RVA 0x5eff30
    was unnamed (no collision). Directly analogous to the Net pilot's SessionNode.

### Hot-header casualty: CDDraw headers are HOT (NEW finding — method fix).
Unlike the Net pilot (0 casualties), placing a `SIZE_UNKNOWN`/`VTBL` macro (a
no-code `char[1]` typedef) **inside** a CDDraw header REGRESSED a neighbour in
every includer TU tested — MSVC5 reschedules codegen when a file-scope typedef
appears mid-parse (before the function bodies). Measured, per header:
- `CDDrawShadeBlit.h` → `ConvertRowDouble` **-1.30%** (hot RLE blit inner loop).
- `CDirectDrawMgr.h` → `ddrawpolyfill FillPolygon` **-0.09%**.
- `CDDrawSurfacePair.h` → `DrawBox` **-0.03%** (also reproduced by a mid-`.cpp`
  insert, i.e. any pre-function-body typedef, not just headers).

**Fix (generalizable):** the completeness checks text-scan `SIZE`/`VTBL` macros
tree-wide, so the annotation need not live next to the class. Host every
header-class annotation at the **EOF of the owning `.cpp`** (the one that
`#include`s the header — types are complete there, and appending at EOF shifts no
existing line, so it is `__LINE__`-neutral and mid-parse-neutral). Verified: all
135 SIZE + 1 VTBL are matching-NEUTRAL (report.json snapshot-diff over all 3394
functions → 0 REGRESS / 0 IMPROVE; build "regressions vs baseline" clean). The
`.cpp`-local classes were likewise appended at their own EOF (a mid-`.cpp`
after-each-class insert cost DrawBox -0.03%, so EOF is the rule for `.cpp` too).
`#if 0`-guarded WIP classes (`AlbusMapValue`, `UnknownAlbusTeardown` in
`CDDrawWorkerMapSmall.cpp`) get their `SIZE_UNKNOWN` INSIDE the disabled block:
the preprocessor-unaware text-scan counts them while the compiler skips them.

### VTBL skipped (34 — KEEP-HAND-ROLLED / uncertain, logged for the final sweep):
The CDDraw family is dominated by hand-rolled vtables; none of the 34 is a clean
own-`??_7` at an unnamed RVA without a foreign stamp. Categories:
- **Foreign `g_*Vtbl` worker/element stamps** (ctor stamps a reloc-masked extern,
  not the class body): `SiriusWorker` (g_siriusWorkerVtbl), `HagridWorker`
  (g_hagridWorkerVtblA/B), `AlbusWorker`/`CDDrawWorkerMapSmall`/`AlbusMapBase`
  (g_albus*), `SeverusWorker`/`SeverusValue`/`UnknownSeverusVtableView`
  (g_severus*), `CWwdWorker`/`CWwdFactoryObject`/`CWwdObject` (g_wwd*),
  `CAniElemView` (g_aniElemVtbl), `LeafElement`/`LeafScanValue` (g_leafElemVtbl).
  The task's KEEP-HAND-ROLLED rule names these explicitly — the g_* name reflects
  hand-rolled uncertainty; do not migrate to VTBL here.
- **Shared grand-base dtor vtable 0x5e8cb4** (g_remusBaseDtorVtbl): the CObject-
  like base subobjects `CSurfacePairBase`, `CDDrawSubMgrBase`, `LeafScanBase`,
  `CCatalogNode`, `HagridChild`, `AlbusMapValue`(#if0). A VTBL here would collide
  on / mis-attribute the one shared vtable (same exception as Net's CNetNodeBase).
- **Declared-but-undefined virtual "views"** (no vtable emitted in-TU, concrete
  RVA not confidently pinned): `CDDrawSurfaceMgr`, `CDDrawSurfaceMgrBase`,
  `HermionaChild`/`CDDrawChildGroup`, `DracoChild`, `DDChildSlot0`/`DDChildSlot1`,
  `CQueueProbeData`, `CWwdArchive`, `Serializer`, `CDDAttachedSurface`,
  `CDDrawWorkerDisp`, `CDDrawSubMgr`, `CDDrawSubMgrLucius`. No RVA guessed.

---

## Scale recommendation (pilot → full 3160 SIZE / 313 VTBL worklist)

The per-class-verify sweep is **efficient enough to scale**, with this shape:

1. **SIZE_UNKNOWN is the bulk, scalable default.** ~90% of classes are partial
   pad-to-last-touched-field views whose modeled `sizeof` is NOT the retail size;
   `SIZE_UNKNOWN` (completeness-only) is the correct, zero-risk annotation and
   the only one a bulk fan-out can apply without per-class RE. Exact `SIZE` is an
   opportunistic upgrade the owning matcher adds when the object is provably
   complete (array-element stride, RE'd fixed packet, byte-matched struct-copy) —
   do NOT try to auto-derive exact sizes in a fan-out (wrong number silently
   passes the self-consistent assert).
2. **The neutrality risk is real but rare; batch by header, not by class.** A
   `.cpp`-local class only touches its own TU (do them in one big group). A
   header class touches every includer — do a header in ≤2 line-range groups and
   snapshot-diff. In this pilot NONE regressed, so a per-header (not per-class)
   verify granularity is enough; only split a header further if its group shows a
   REGRESS.
3. **Trust the tooling for the gate.** `gruntz build`'s "regressions vs baseline"
   plus a report.json fuzzy% snapshot-diff (all functions) catches both the
   auto-check's cases and the clangd-fallback-hashed edits. One snapshot-diff per
   build group is the whole verification cost.
4. **Duplicate class NAMES across TUs are common** (a full view in the header +
   a partial same-named view in a `.cpp`). The tests dedupe by name, so annotate
   ONCE at any complete definition; annotating the complete one lets you use
   exact `SIZE`.

Estimated fan-out: one worker per module (or per hot header + its `.cpp` set),
each draining its SIZE + VTBL violators in ~2–3 build groups. Budget per worker
is dominated by reads/edits, not builds (3 builds sufficed for 63+2 here).
Per-module log of the SIZE/SIZE_UNKNOWN/VTBL completeness sweep
(`python -m gruntz.match.class_sizes` / `class_vtables`). Records hot-header
casualties (an annotation that rescheduled a neighbor and was reverted) and the
per-module hotness verdict, so the full fan-out can decide where to be careful.

## Dsndmgr module (2026-07-01)

Scope: classes defined under `src/Dsndmgr/*` + `include/Dsndmgr/*`.

Coverage delta (both checks, Dsndmgr classes only):
- SIZE completeness: 1/36 annotated (only `DSoundCloneCtor`) -> 36/36. All
  Dsndmgr class names now carry SIZE or SIZE_UNKNOWN.
- VTBL completeness: 4 Dsndmgr violators remain (see "VTBL skips" below); all are
  already bound via a manual `g_*Vtbl` DATA extern or have no in-module vtable
  RVA, so no VTBL was added (adding one would collide on the RVA / has no target).

Annotated (this sweep):
- Exact `SIZE`: `DSoundVoice` 0x28, `SampleVoice` 0x60, `StreamVoice` 0xb0,
  `StreamFeeder` 0x44, `SoundDevice` 0x98, `SoundStream` 0x9c,
  `CGruntzSoundInnerZ` 0x60, `CGruntzSoundZ` 0x2c, `DSBCAPS` 0x14,
  `DSBUFFERDESC` 0x14. (`CGruntzSoundZ` 0x2c is MSVC5-verified: its `SIZE`
  static_assert compiled, confirming `sizeof(CMapStringToOb)==0x1c`.)
- `SIZE_UNKNOWN` (view / partial / opaque tail / C++ sizeof != retail alloc):
  `DSoundVoiceList`, `DSoundCloneList`, `DSoundBaseSub`, `DSoundCloneInst`,
  `DirectSoundMgr`, `IDirectSoundZ`, `IDirectSoundBufferZ`, `DSoundCloneBase`,
  `DSoundLink`, `DSoundElem`, `DSoundList`, `WaveFormatX`, `SoundBuf`,
  `SoundSample`, `SoundVoiceList`, `SoundBufList`, `ParseFmt`, `RezFile`,
  `CSoundBank`, `StreamList`, `StreamVoiceList`, `StreamSource`, `FeederSource`,
  `FeederOwner`, `FeederBuf`, `VoiceOwner`.

VTBL skips (KEEP-HAND-ROLLED / external; each logged, none added):
- `CGruntzSoundInnerZ` — vtable 0x1ef700 already bound to `g_innerSoundVtbl`
  (`DATA(0x001ef700)` in CGruntzSoundZ.cpp); VTBL would dup the RVA.
- `DSoundCloneBase` — clone-base vtable 0x1ef6c0 already targeted by
  `DATA(0x001ef6c0) g_DirectSoundBaseVtbl` (DirectSoundMgr.cpp); VTBL would dup.
- `CSoundBank` — virtuals declared-not-defined; its vtable is external, no RVA in
  this module.
- `SoundSample` — cached-sample node view; its live vtable RVA is not modeled
  (only the pure/free-restamp 0x5ef6c8, itself bound as `g_PureVtbl`/`g_vtbl_pure6c8`).

Hot-header casualties: NONE. Every group built with "no regressions vs baseline";
overall stayed 1870/3394 exact / 63.27% fuzzy throughout.

Hotness verdict: **Dsndmgr headers are CLEAN (not regression-prone).** Even the
most-included header (`DirectSoundMgr.h`, pulled by SoundDevice.h -> SoundStream.h
and by Gruntz's CAmbientSound.h / CRandomAmbientSound.h / CDDrawSubMgrLeafScan.cpp)
took all six annotations with zero neighbor movement. Contrast Grunt.h, where a
lone `SIZE_UNKNOWN(CGrunt)` reschedules a neighbor. Dsndmgr is safe to annotate in
one shot; per-class build verification was still done here and confirmed neutral.

## Boundary + Tile-trigger family (2026-07-01)

Scope: `include/Gruntz/Tile*.h` + `CExitTrigger.h`/`CTimeBomb.h`/`TriggerMgr.h`
(+ `CCheckpointTrigger.h`) and the `src/Gruntz/Boundary*.cpp` / `Tile*.cpp` /
`CExitTrigger.cpp` / `CVoiceTrigger.cpp` / `CCheckpointTrigger.cpp` /
`CSecretLevelTrigger.cpp` / `CTimeBomb.cpp` / `TriggerMgr*.cpp` local classes.
(No `Boundary*.h` / `CVoiceTrigger.h` / `CSecretLevelTrigger.h` headers exist —
those classes are `.cpp`-local.)

Coverage delta:
- SIZE completeness: family violators **413 -> 0**; tree-wide annotated names
  **154/3296 -> 567/3296** (+413 = the whole family).
- VTBL completeness: family violators **49 -> 49** (all skipped, see below); none
  catalogued (every add would collide on a shared RVA or target an absent/foreign
  vtable).

Annotated: **413 SIZE_UNKNOWN + 0 SIZE(exact) + 0 VTBL.** All 413 family classes
are partial pad-to-last-touched-field modeling views (the RTTI-unattributable
engine_boundary backlog + the placeholder-named tile-trigger view structs), so the
scalable `SIZE_UNKNOWN` default is the only honest annotation — no object here is
provably == its full retail alloc size, so no exact `SIZE` was guessed. (The owning
matcher can upgrade e.g. `CTrigParam` (by-value 16-byte block) to exact later.)

### Hot-header casualties + placement fix
- NONE among the 7 headers (all 1-2 includers; the `char[1]` typedef the macros
  lower to under MSVC5 perturbed nothing). Added `#include <rva.h>` to the 3
  headers lacking it (TileGridCommand.h / TileTriggerContainer.h /
  TileTriggerSwitchLogic.h); matching-neutral.
- **`TriggerMgr.cpp` IS codegen-sensitive (a `.cpp`, not a header).** The default
  interspersed placement (a `SIZE_UNKNOWN(...)` right after each class) rescheduled
  two same-TU methods: `CTriggerMgr::ResetGroup` -0.18% and `HitTestCell` -0.02%
  (an interspersed no-code typedef nudges MSVC5's per-function COMDAT ordering in
  this dense TU). **Fix: place all of TriggerMgr.cpp's 46 `SIZE_UNKNOWN` at
  end-of-TU (after every function body) — verified matching-NEUTRAL** (0 regress).
  Lesson for the full sweep: for a large/dense reconstructed `.cpp`, prefer
  end-of-TU placement; interspersed is fine for headers and small TUs.
- Two incidental IMPROVEMENTS (interspersed placement in their own TUs, no
  regression, kept): `triggermgr_eh CTriggerMgr::DestroyGroup` 64.80->66.75 and
  `boundarytail CSnd788d0::PositionUpdate` 64.83->65.34.

### VTBL skipped (all 49; un-catalogable, NOT casualties):
Two exhaustive buckets, identical to the Net/Dsndmgr pilots' skip cases:
1. **Interface / archive / slot-dispatch VIEW structs** — declare `virtual` slots
   ONLY to lower a slot-indexed virtual call (`p->Read()` -> `call [vptr+0x2c]`)
   with no cast; NEVER constructed here, so cl emits no `??_7` in this TU and the
   real vtable lives in a foreign engine class. No own RVA to bind. E.g. the
   serializer views TgcStream/TtcStream/CSerialStream/CTileActionArchive/
   CTileTransitionState/CTrigReader/CTileObj/CTmSerReader/CTmSerMapObj/TgcTickView,
   and the dispatch views Cea170/CArchiveEb/CArchive113/CState8e/Cd5e20/
   CGuardedDispatch1f870/DDPageSub/ImgOwned/ImgOwnedX/Snd138f20/Killable0/Killable1/
   RezOwner/RezDir/RezDirBase/RezListNode.
2. **Severus/state/bute/logic BASE modeling shells** (WorkerBase39f20/
   WorkerBase8c400/CStateBase8d000/CButeBase1_21/CButeBase2_21/Sev17e240/Sev14fe30/
   Sev161500/Sev138a50/Sev168c10/Sev15b6d0/FaderBase/EmbedBase17e990/Base163a40 and
   the 9 CTile*TriggerLogicBase/CTile*TriggerSwitchLogicBase shells in
   TileTriggerDerivedCtors.cpp): the base subobject folds the SHARED CObject/CState/
   CButeBase base-dtor vtable (0x5e8cb4 / 0x5ea21c / 0x5e94ac / 0x5e949c) already
   bound as `?g_severusBaseDtorVtbl@@3PAXA` &c.; the concrete leaf vtable is the
   DERIVED class's auto-`??_7` (reloc-masked / auto-named). A `VTBL(<base-shell>, ...)`
   would either dup the shared RVA (dup-DATA guard) or mis-attribute one shared/leaf
   vtable to a modeling-base name. KEEP-HAND-ROLLED / shared-base-vtable exception.

Hotness verdict: **the 7 headers are CLEAN**; the ONE codegen-sensitive unit is the
big reconstructed `TriggerMgr.cpp` (fixed via end-of-TU placement). No regression
committed; overall stayed 1870/3394 exact / 63.33% fuzzy throughout.
## Wap32 module (2026-07-01)

Scope: classes defined in `src/Wap32/*` + `include/Wap32/*` (the WAP32 engine base
`CGameApp`/`CGameWnd`/`CGameMgr`/`CGameResource`, the `_zvec`/`zDArray` dynamic-vector
family, the `EngStr`/`zBitVec`/`CContainerErr` string+bit-vector helpers, and the
`.cpp`-local COM/MFC/ring leaf views). `CGameWnd`(0x10) + `WAP32::CGameMgr`(0x2c) +
`CContainerErr`/`zBitVec` already carried SIZE from prior matchers (skipped).

Coverage delta (whole-tree SIZE counter, Wap32 worklist drained):
- SIZE: 518/3316 -> 541/3316 annotated names (**23 Wap32 classes annotated**;
  Wap32 SIZE violators 23 -> 0).
- VTBL: Wap32 vtable violators 1 -> 1 (the lone `CGameResource` is an un-catalogable
  view, below); no new catalog rows.

Annotated: **22 SIZE_UNKNOWN + 1 SIZE(exact) + 0 VTBL**.
- Exact `SIZE`: `GameInfo`(0x1d4) only — the window/launch descriptor is a
  RE'd fixed struct whose own +0x00 `size` field is set to 0x1d4 (== the modeled
  `sizeof`); MSVC5 compiled the assert, confirming the complete retail object size.
- All other 22 are `SIZE_UNKNOWN` (pad-to-last-touched-field / interface / forward
  views: `CGameApp`, `CGameResource`, `CGameWndCreateParams`, `CDdeView`, `CObj653070`,
  `CRect`, `TextRenderer`, `CStamp11d100`, `WndLike`, `ComSingleton2f00`,
  `ComSingleton3210`, `ComStamp`, `EngStrRenderCfg`, `EngStrRenderSub`, `EngStrRenderObj`,
  `RingCtx`, `RingSrc`, `_zvec`, `zDArray`, `zErrHandling`, `zErrRegistry`,
  `zMemberPtrSlot`) — none provably == the full retail object, so no exact upgrade.

### Hot-header handling: `.cpp`-EOF trick, NONE regressed.
Every annotation was appended at the EOF of the owning/including `.cpp`, so the three
hot engine headers (`Wap32.h`, `ZVec.h`, `EngStr.h`) were NEVER edited: `Wap32.h`'s 4
classes -> `GameApp.cpp` EOF; `ZVec.h`'s 3 -> `ZVec.cpp` EOF; `EngStr.h`'s 1 ->
`EngStr.cpp` EOF; the `.cpp`-local classes at their own `.cpp` EOF. A single report.json
snapshot-diff over all 3394 functions after applying all 23 -> **0 REGRESS / 0 IMPROVE**;
build "no regressions vs baseline"; overall stayed 1873/3394 exact / 63.71% fuzzy.
The settled `EngStrRenderText.cpp` (its EngStr_RenderText body is `@early-stop`) took
its two class annotations at EOF with the body untouched (line-shift-neutral).

Hotness verdict: **Wap32 is SAFE via the `.cpp`-EOF rule** — headers untouched, only
one TU's EOF shifts per group, so the CDDraw mid-parse-typedef reschedule risk never
arises. (Not tested whether an in-header typedef would regress; the EOF rule sidesteps it.)

### VTBL skipped (1 — declared-but-undefined view, NOT a casualty):
- `CGameResource` (Wap32.h): the abstract WAP32 resource base whose pointers live in
  `CGameApp::m_4`(the CGameWnd) / `m_8`(the CGameMgr). Its 5 virtuals are all
  declared-but-undefined in every TU we compile, so cl emits no `??_7CGameResource@@6B@`,
  and it has no own concrete vtable RVA (the concrete resources CGameWnd@0x1ea344 /
  CGameMgr@0x1e9b8c already carry VTBL rows in config/vtable_names.csv). No RVA guessed.
  Same case as the Net pilot's `CNetPlayerObj` and the SBI/CDDraw view structs.

### SIZE deferred (out-of-scope owner, NOT a casualty):
- `EngStr` (rep `include/Gruntz/UserLogic.h:44`, also `src/Gruntz/WwdGameObject.cpp:36`) —
  the CUserLogic name string, defined only in UserLogic (do-not-touch) / Gruntz files,
  never in a Wap32 file. Belongs to the UserLogic/Gruntz module sweep (same cross-file
  dedup rule as the SBI section's non-SBI reps). Left un-annotated here.

## Bute module (2026-07-01)

Scope: classes defined in `src/Bute/*` + `include/Bute/*` (the CButeMgr `.att`
attribute-parser, the CSymTab/CSymParser Remus symbol table, the CHash* hash
family, and the parser/reader view structs) plus the Bute stub types still in the
tree (CButeTree in `src/Stub/CButeTree.cpp`; CButeNode/CButeNodeEntry in
`src/Bute/ButeNode.cpp`).

Coverage delta (whole-tree counters, Bute worklist drained):
- SIZE: 908/3316 -> 955/3316 annotated names (**47 Bute classes annotated**; Bute
  SIZE violators 47 -> 0). All `SIZE_UNKNOWN`: every Bute class modeled is a partial
  pad-to-last-touched-field / interface / view (no array-element stride, RE'd fixed
  packet, or byte-matched struct-copy proves a complete retail object), so per
  doctrine none took an exact `SIZE`.
- VTBL: Bute vtable violators 5 -> 5 (all skipped, below); no new catalog rows (tree
  total unchanged at 311 not-catalogued).

Annotated: **47 SIZE_UNKNOWN + 0 SIZE(exact) + 0 VTBL**, all hosted at the owning
`.cpp` EOF (ButeMgr.h is the proven-hot header the task flags):
- ButeMgr.cpp EOF (17): the ButeMgr.h header classes CButeValue / ButeRef24 /
  CButeMgrHelper / CButeStoreBase2 / CButeStore / CButeTail / CButeNode / CButeRef5-8 /
  CButeMgr, plus the `.cpp`-local CButeStream / ButeMgr / CButeValueNode / CButeTextBuf /
  CButeSub.
- ButeMgrParse.cpp EOF (2): ButeIos / ButeFileStream.
- ButeNode.cpp EOF (1): CButeNodeEntry.
- ButeStoreClear.cpp EOF (1): CButeStoreNode.
- ButeTree.cpp EOF (1): CButeTree (the crit-bit trie node WITH fields is the owning
  TU; the ButeMgr.h + `src/Stub/CButeTree.cpp` defs are data-less method-only views of
  the same name — dedup-by-name means one annotation covers all three).
- Hash.cpp EOF (7): the Hash.h classes CHashSlotList / CHashInsertNode / CHashSlot /
  CHashEntry / CHashBase / CHash / CHashB.
- SymParser.cpp EOF (9): the SymParser.h classes CObjNode / CObjList / CSlotNode /
  CParserHash / CSymParser, plus the `.cpp`-local CTextReaderInit / CBinReaderInit /
  SymFindData / CParseSlot.
- SymTab.cpp EOF (9): the SymTab.h classes RezColl / RezNode / CHashTable / CSymRec /
  CSymTab, plus the `.cpp`-local CSymLeafBuilder / CSymSeedOwner / CSymRangeStream /
  CSymSlotPool.

Cross-file dedup notes: three names are defined in more than one Bute header (the
"conflicting hash shape" the SymParser/SymTab comments describe) — CSymTab (SymTab.h +
SymParser.h), CSymParser (SymTab.h struct + SymParser.h class), CHashEntry (Hash.h +
SymTab.h). The tests dedupe by name, so each is annotated exactly ONCE at its fullest
def: CSymTab in SymTab.cpp, CSymParser in SymParser.cpp, CHashEntry in Hash.cpp.
CButeStore / CButeNode likewise have `.cpp`-local partial views; annotated once at the
header def via ButeMgr.cpp.

### Hot-header casualties: NONE.
All 47 SIZE_UNKNOWN were applied together at the `.cpp` EOFs (the four headers were
NEVER edited), then one `gruntz build` + a report.json snapshot-diff over all 3394
functions -> **0 REGRESS / 0 IMPROVE**; build "no regressions vs baseline"; overall
stayed 1873/3394 exact / 63.71% fuzzy (63.70736 -> 63.70736). Confirms the task's
premise that ButeMgr.h is hot only to *in-header* (mid-parse) typedefs; the `.cpp`-EOF
rule sidesteps it.

### VTBL skipped (5 — un-catalogable, NOT casualties; SIZE-only per task):
Every Bute vtable violator is a **declared-but-undefined virtual "view" struct** (same
case as the Net pilot's CNetPlayerObj / the SBI+CDDraw view structs): it declares
`virtual` slots ONLY to lower a `mov eax,[this]; call [eax+slot]` dispatch with no cast,
is never constructed in-TU, so cl emits no `??_7` here and the concrete vtable lives in a
foreign / CRT class. No own RVA to bind.
- `ButeIos` (ButeMgrParse.cpp): the CRT `ios` base view; its virtual dtor's vtable is
  owned by the statically-linked CRT.
- `CButeSub` (ButeMgr.cpp): the CButeMgrHelper +0x4 sub-object; slot-0 is a __thiscall
  scalar-deleting dtor into another TU.
- `CHashInsertNode` (Hash.h): the insert-node prefix; slot-0 is the key-typed bucket
  hash, defined in the concrete record's TU (pure virtual here).
- `CObjNode` (SymParser.h): the CObjList node; its scalar-deleting-dtor / detach
  virtuals point into other unmatched TUs.
- `CSymRangeStream` (SymTab.cpp): the ApplyRange parse-stream; slot 2 (+0x8) Read is a
  reloc-masked virtual on a foreign engine stream class.
The task's Bute z*-family KEEP-HAND-ROLLED vtables (g_buteNodeVtbl / g_storeVtblA /
g_storeVtblB / g_helperVbaseVtbl* etc.) are `DATA()`-bound reloc-masked externs stamped
vptr-LAST, not classes with `virtual`, so they never surface in the vtable worklist; no
VTBL touched (already documented in docs/vtable-conversion-log.md).

Hotness verdict: **Bute is SAFE via the `.cpp`-EOF rule** — headers untouched, one TU's
EOF shifts per group, no mid-parse-typedef reschedule. Applied all at once with a single
all-function snapshot-diff (per prior-pilot practice); 0 casualties.

## Grunt / game-object family (2026-07-01)

Scope: classes in `include/Gruntz/Grunt.h`, `CGrunt*.h`, `GruntzApp/CmdMgr/Command/
Mgr/Player.h`, `CGruntzMapMgr.h`, `WwdGameObject.h`, and classes defined in
`src/Gruntz/Grunt*.cpp` / `CGrunt*.cpp` / `GameObject*.cpp` / `Gruntz*.cpp` /
`Kitchen*.cpp` (37 `.cpp` TUs). Excluded per matcher instructions: GameLevel.*,
ApiCallers.cpp, CDDraw*/Bute*/Discovered/UserLogic (other matchers), the Wap32
engine-support modules, `WwdGrid.h` (not `WwdGameObject`).

Coverage delta:
- SIZE completeness: family violators **389 -> 0**; tree-wide annotated names
  **885/3316 -> 1274/3316** (+389 = the whole family).
- VTBL completeness: family violators **28 -> 28** (all skipped, see below); no
  catalog rows added (every add would collide on a shared/RTTI RVA, target a
  foreign vtable, or guess an unpinned RVA of a declared-only view).

Annotated: **389 SIZE_UNKNOWN + 0 SIZE(exact) + 0 VTBL.** Every family class is a
partial pad-to-last-touched-field modeling view / interface / slot-dispatch shell;
none is provably == its full retail alloc (no array-element stride / RE'd fixed
packet / byte-matched struct-copy in the models), so the scalable `SIZE_UNKNOWN`
default is the only honest annotation — no exact `SIZE` guessed (owning matcher can
upgrade e.g. CGrunt later).

### Placement + hot-header handling: `.cpp`-EOF for EVERYTHING (0 casualties).
Grunt.h is the family's HOT header (the infra proof measured in-header `SIZE(CGrunt)`
costing `StepCompassMove` -0.52%). Per the proven fix, **every** annotation was hosted
at the **owning `.cpp`'s EOF** (never in a header, never interspersed): header classes
at their header's paired `.cpp` EOF (Grunt.h -> Grunt.cpp; GruntzMgr.h -> GruntzMgr.cpp;
CGruntzMapMgr.h -> CGruntzMapMgr.cpp; the per-sprite CGrunt*.h -> their CGrunt*.cpp; the
GruntzCmdMgr/Command/Player headers -> their `.cpp`), and `.cpp`-local classes (incl. the
per-TU shim views in the dense grunt-AI/step TUs and the big GruntzMgr.cpp = 83) at their
own `.cpp` EOF. `CGrunt` itself was hosted at `GruntPathScan.cpp` EOF (its `.cpp`-local
view def). Result: report.json snapshot-diff over all 3394 functions = **0 REGRESS /
0 IMPROVE**, overall unchanged at 1873/3394 exact / 63.71% fuzzy; `gruntz build`
"no regressions vs baseline". EOF placement is neutral even in the codegen-sensitive
grunt-AI `@early-stop` TUs (GruntUpdateStep/ArrivalScan/StateStep/PathScan/etc.) —
their bodies were NOT touched, only appended-at-EOF.

### VTBL skipped (all 28; un-catalogable, NOT casualties — for the final sweep):
Identical buckets to the Net/SBI/CDDraw/Boundary pilots; every family vtable-bearing
violator is one of:
1. **Declared-but-undefined slot-dispatch VIEW structs** (declare `virtual` slots ONLY
   to lower a `mov edx,[o]; call [edx+slot]` thiscall dispatch; NEVER defined/instantiated
   here, so cl emits no `??_7` and the real vtable is a foreign engine/MFC class — no own
   RVA to bind): `AnimWorker`, `CGruntArchive`, `CVtblSlot9`, `GruntObjEntry`, `GzTargetObj`,
   `GzStateProvider`, `GzStream`, `GzSerCmd`, `CHazardStream`, `CWorldDelete`, `CSerializerZ`,
   `CModalDialog`, `CWorldModeIface`, `CmdSinkV`, `CSlimeStream`, `CTsState`, `CmdStream`,
   `GameObjTypeRegistry`, `PlayerArchive`, `PupArchive`, `CTypeKeyColl` (the last: the family
   view is a non-virtual `Resolve` registry; the `virtual` signal is a same-named non-family
   def; its RVA is not pinned here).
2. **Base modeling shells** whose base ctor is out-of-line/external so cl emits no own
   `??_7`; the concrete LEAF vtable auto-derives via RTTI (config/vtable_names.csv):
   `CGruntSpriteBase`, `CPathHazardBase` (leaf `??_7CPathHazard@@6B@`@0x1e7394 IS in
   symbol_names.csv), `CGmmBase` (its `~CGmmBase` is the external `~CMapMgr` @0x135c).
3. **Foreign/library base, declared-only virtual dtor:** `CSaveDlgBase`
   (`~CSaveDlgBase` == MFC `CDialog::~CDialog` @0x1ba51d).
4. **Already-named via the RTTI catalog** (config/vtable_names.csv) — a `VTBL` would be a
   redundant second source / dup on that RVA: `CGrunt` (`??_7CGrunt@@6B@`@0x1e8754; flagged
   only because cl doesn't yet emit CGrunt's `??_7`, so the AUTO path is dormant — the RTTI
   row is the authoritative catalog entry, owning matcher names it when CGrunt goes
   real-polymorphic), `CScrollView` (`??_7CScrollView@@6B@`@0x1ed854, also an MFC library
   class — game-not-library skip).
5. **Already bound via a foreign `g_*Vtbl` DATA extern in an out-of-family TU:**
   `CGruntzSoundInnerZ` (vtable 0x1ef700 == `g_innerSoundVtbl`, `DATA(0x001ef700)` in
   Dsndmgr's CGruntzSoundZ.cpp; VTBL would dup the RVA).

Hotness verdict: **all family headers are safe at `.cpp`-EOF placement**; Grunt.h is
hot only for *in-header* typedefs (the measured -0.52% case), which the EOF rule avoids.
No regression committed.
