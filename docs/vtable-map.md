# Vtable map — full inventory + exact sizes

`gruntz verify vtable-scan` recovers **every vtable** in
`GRUNTZ.EXE` and its **exact entry count**, from three binary signals:

1. **reloc runs** — every vtable slot is an absolute DIR32 fn address ⇒ a PE
   base-reloc site. A stride-4 run of *data*-section reloc sites whose values all
   point into `.text` is a band of one-or-more adjacent vtables.
2. **RTTI COL** — at `vtable-4`, MSVC `/GR` code stores a Complete Object Locator
   pointer (.rdata) ⇒ confirms the start, names the class, gives the base-subobject
   offset.
3. **code refs** — a ctor/dtor stamps the vptr (`mov [this], offset vtable`), so a
   `.text` DIR32 whose target is a run member marks a real vtable start — incl.
   **non-RTTI** vtables with no COL.

Cutting each run at `{COL starts} ∪ {code-referenced starts}` **de-merges**
adjacent vtables (a raw run glues them — nothing separates them in memory) and
gives the per-vtable size. Output: stdout, or `--csv <path>` for the machine-readable form.

## Counts (retail v1.0, MD5 `81c7f648…`)

| bucket | n | notes |
|---|---|---|
| RTTI-confirmed | 222 | 210 primary + 12 secondary (MI base subobjects: iostream/strstream/fstream) |
| non-RTTI, code-ref, `.rdata`, sz≥2 | 75 | real vtables, slot-0 = scalar/vector-deleting-dtor / `__purecall` / GetRuntimeClass thunk |
| non-RTTI, weak (sz1 or `.data`) | 39 | ambiguous — mostly fn-ptr globals, not vtables |
| **total real** | **336** | |
| excluded (unref run-heads) | 2960 | C++ EH tables (funcinfo/unwind/handler) + switch jump tables — odd/funclet pointers, no COL, no code ref |

- 231 RTTI typedescriptors, 246 COLs. 10 type names have no vtable: COM
  interfaces (`IUnknown`/`IStream`/`ISequentialStream`), CRT `_zvec/_zdvec`
  (COL present, vtable COMDAT-folded), and `CWapObj`/`CWapX`/`CSimpleException`
  (abstract / folded).
- The recurring slot-0 `sub_1bef01` is a real but **unaligned** thunk
  (`mov eax,0x5eb848; ret`, an MFC GetRuntimeClass accessor) MSVC packed without
  16-byte alignment — Ghidra never carved it, hence the "off fn-boundary" noise.

## Non-RTTI vtables stamped manually in ctors (already modeled in `src/`)

These classes have **no compiler-emitted vtable in our TU** (contents live in
unmatched TUs or are foreign/library), so the ctor does a manual `*(void**)this =
&g_xVtbl` / `m_vptr = &g_xVtbl` with `g_xVtbl` an `extern` annotated `DATA(rva)`
(reloc-masked). 36 of the 75 strong non-RTTI starts are already annotated:
DinMgr2 `g_deviceConfig{A,B,C}`, the whole Dsndmgr family
(`g_{DirectSoundMgr,DirectSoundClone,SoundDevice,StreamVoice,SoundStream,StreamFeeder,Pure}Vtbl`),
WAP `z`-container/worker vtables (`g_{severusWorkerBase,ddrawWorkerA,albusWorker,
siriusWorker,remusNode,wwdObj,catalog,leafScan}Vtbl`), `g_fileImageVtbl`,
`g_fileMemBaseVtbl`, `g_zDArrayDtorVtbl`, `g_menuItemVtbl`, `g_faderSineVtbl`, etc.
The remaining **39 strong non-RTTI starts are not yet annotated** — see the CSV
(`in_src_data=0`, `confidence=code-ref`); several sit *between* vtables the source
already knows (e.g. `0x1ef640`/`0x1ef658` between the DinMgr2 device-config set).

## Cataloguing the non-RTTI vtables

`include/UnknownVTables.h` — a tracking header of `ClassWithUnknownVTableN` structs —
is **retired and deleted**. It was scaffolding from when vtables had no recoverable
class name; the `Unknown ids` metric is now 0, so there is nothing left for it to
track.

The live catalogs are `config/retail/data_vtables.tsv` and
`config/retail/data_static_libs.tsv`. They are manually maintained and cross-checked by
`gruntz verify vtables`. A class whose `??_7` retail never emitted simply has NO
`data_vtables.tsv` row — the `VTBL_ABSENT` source marker is retired, because
absence is class-model state and needs no executable label.
Regenerate the address map with `gruntz verify vtable-scan`;
`docs/rtti-vtable-catalog.tsv` is the RTTI-side census.

## Why the manual stamps can't just flip to real virtuals

A class only flips to a real compiler-emitted vtable once its **virtuals + base
subobject are reconstructed** — that's matching-campaign work, not a flag toggle.
Two structural blockers (both proven from the binary; see git history of this
branch): the vtable slots reference **unnamed** virtual bodies (only 2/51 manual
vtables are fully-named), and the two named ones (`SoundDevice`/`SoundStream`) are
walled by a **`/GX` EH frame** needing the non-trivial base subobject (source:
`docs/patterns/eh-dtor-needs-base-subobject.md`). Flags **`/GR`** (RTTI; MSVC5
default *off*, so the build is already `/GR-`) and **`/GX`** (EH; build doesn't set
it) are real per-TU levers to *finish* such a class, but are necessary-not-sufficient.

RTTI-backed classes are *also* sometimes stamped manually (incomplete recon: real
class in RTTI but virtuals not all reconstructed) — the `CSBI_*` status-bar item
family, the trigger-logic classes, CUserBase, CMulti/CPlay/CState.
That's a transitional artifact; the end state is a real C++ class with declared
virtuals. See [correctness-not-artifacts].

## Removing the struct vtables — follow-up worklist

PR #56 landed the infra + the one *flat* conversion (`CTileTriggerSwitchLogic`).
Recipe (proven): declare the class's real virtuals (declared-only is fine — the
bodies live in engine TUs), delete the `extern …g_xVtbl` + `DATA()` + the
`*(void**)this = &g_xVtbl` stamp; the `??_7` name **auto-derives** for RTTI
classes; add one primary or secondary row to `config/retail/data_vtables.tsv` for
non-RTTI. Then `gruntz build` and confirm byte-exact + no regressions.

Everything left is a base-hierarchy *family* — convert per family (model the
shared base once, then the derived classes), not one-off:

- **Sprite family** (`GameObjectCtors.cpp`; RTTI, measurable): CUFO, CRainCloud,
  CGruntStaminaSprite, CGruntToyTimeSprite, CGruntWingzTimeSprite — shared
  `CGruntSpriteBase`, base-ctor calls. Most self-contained → do first.
- **Status-bar chain** (`SBI_*Eh.cpp`; RTTI, measurable): CStatusBarItem ←
  CSBI_RectOnly ← CSBI_Image ← CSBI_MenuItem (+ ImageSet/WarlordHead/WellGoo/
  SideTab/StatzTabGruntBar/GruntMachine). Multi-vtable **EH-dtor** restamps
  (`docs/patterns/eh-dtor-*`) — model the chain + virtual dtors.
- **Trigger-logic family** (`src/Gruntz/TileTriggerSwitchLogic.cpp` + siblings, CGiantRockLogic,
  CCoveredPowerupLogic, CCheckpointTriggerSwitchLogic; RTTI): size-1 vtables over
  the (graduated) `CTileTriggerLogic` base. **Blocked on measurability** — they
  aggregate into `engine_label_stubs` (0/519 oracle); graduate each to its own
  unit first, then convert.
- **EH / special singletons**: Dsndmgr (`SoundDevice`/`SoundStream`/
  `DirectSoundMgr`/`StreamVoice`/`StreamFeeder` — `/GX` EH frames, non-RTTI),
  CBoomerang (sunk-store wall), CImage/CFileImage, CRemusNode (PMF-vtable +
  CObject base), CDDrawSurface{Mgr,Pair}/CDDrawSubMgrAni (CObject restamps),
  CMulti/CPlay/CState (43/43/26-slot vtables), ZVec/EngStr.
