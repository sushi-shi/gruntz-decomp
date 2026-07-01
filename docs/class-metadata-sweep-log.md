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
