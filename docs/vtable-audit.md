# vtable audit — the definitive "what remains and why"

Tree-wide classification of **every** surviving `g_*Vtbl` / `m_vtbl` / `m_vptr` /
`struct *Vtbl` form in `src/` + `include/`, produced by the final vtable mop-up
pass (2026-07-02). This is the companion to `docs/vtable-conversion-log.md` (the
per-class decision log) and `docs/vtable-map.md`; it answers the campaign's
open question **"when are vtables DONE?"**

Method: grep the tree for the three form families, then classify each hit against
the retail `.rdata` (VA = RVA + 0x400000), `config/vtable_names.csv` (the realized
`??_7` catalog), `include/UnknownVTables.h` (the non-RTTI vtable catalog), and the
Ghidra/delinker symbol names. Categories:

1. **realized `??_7` DATA name** — a manual `extern g_*Vtbl` that merely *names*
   an already-emitted (or templated/namespaced, un-`VTBL`-able) `??_7` vtable.
   Faithful reference form; keep.
2. **faithful external / COM dispatch table** — a typed `struct *Vtbl` (COM
   `__stdcall` fn-ptr table, or engine `__thiscall` PMF table) modeling a **foreign
   object's** vtable so the exact `mov edx,[ecx]; call [edx+N]` dispatch falls out.
   The object's real class lives in another TU / is a COM interface; a real C++
   virtual would diverge. Keep + comment.
3. **MFC / engine-base layout vptr** — a `void* m_vtbl`/`m_vptr` at +0x00 that is
   the base subobject's own vptr field (MFC `CObject`, engine `CObArray`/
   `CTypedPtrArray`). Keep.
4. **documented wall** (`@early-stop`) — an OWN-class or base vtable that cannot be
   `cl`-auto-emitted at its reference site: vptr-last (member stored before vptr),
   foreign-factory (a sub-object the class placement-`new`s, class elsewhere),
   dtor-phase base-vtable RE-stamp, vptr-MIDDLE ctor stamp, out-of-line stamp
   thunk, or unmatched-virtuals/non-trivial-foreign-base. The logic is a complete
   correct reconstruction; only the vptr *shape* is parked. Keep the manual form.
5. **GENUINE convertible placeholder still-to-do** — a real own-class vtable that
   *would* `cl`-auto-emit cleanly (real ctor/dtor, primary vptr at +0x00, base
   modelable) but is still modeled with a manual `void*` stamp. **This is the only
   category that is unfinished work.**

## Verdict — vtables are DONE

**Category 5 (genuine clean convertibles remaining) = 0.**

Every surviving form is terminal (categories 1–4): a realized-`??_7` reference, a
faithful foreign/COM dispatch view, an MFC/engine-base vptr field, or a documented
`@early-stop` wall. The grunt-AI / `CUserLogic` / `CState` / `CBooty*` /
`CDoNothing*` / `CSpotLight` cluster is already fully real-polymorphic; this pass
converted the last clean own-class placeholder in scope (`CAmbientSound`, below).
The three borderline walls (`CLogicRecord`, `CDDSurface`, `CContainerErr`) are
**not** clean cat-5: each needs a foreign/MFC base (`CObject` / `CPoolItemA`)
modeled real-polymorphic first and regresses the vptr shape if forced — they stay
cat-4 until the base class + full vtable are modeled (a leaf-first final-sweep
task, not a mop-up conversion).

The raw greps still report large counts, but they are all terminal:

| Form family (tree-wide grep)                 | count | disposition |
| :------------------------------------------- | ----: | :---------- |
| distinct `g_*Vtbl` externs                    | ~124  | cat 1/3/4 (78 active in code; rest are `UnknownVTables.h` tracking catalog) |
| `struct/class *Vtbl` declarations            | ~61   | cat 2 (foreign COM/PMF dispatch views) |
| `void* m_vtbl` / `void* m_vptr` +0x00 fields | ~15   | cat 2/3/4 (dispatch-view holders, engine-base vptr, foreign-base walls) |
| **genuine convertible (cat 5)**              | **0** | — |

## Conversions done this pass

| Class | RVA | Was | Now | Effect |
| :---- | :-- | :-- | :-- | :----- |
| `CAmbientSound` | dtor 0x00b790 | non-poly `class CAmbientSound {void* m_vptr; ...}` + manual `m_vptr = g_CUserBaseVtbl` base restamp | `class CAmbientSound : public CUserBase` (real virtual dtor + `virtual Update`), `VTBL(CAmbientSound,0x1e710c)`, `void* m_vptr` + `g_CUserBaseVtbl` extern removed | dtor **100%** (cl DCEs the derived-vptr store — no virtual dispatch in the body — and auto-emits the `??_7CUserBase` base restamp, reproducing retail's 15-byte shape byte-for-byte); Restart/SetLevel 100%, Update 76.8% (unchanged tail-merge `@early-stop`). Build green, 0 regressions. |

Recipe that worked (reusable): a leaf whose retail dtor stores **only** the base
vptr (the derived-vptr store DCE'd because the dtor body has no virtual dispatch)
converts cleanly by deriving from the already-modeled base (`CUserBase`) with a
real `virtual ~Leaf()` — `cl` reproduces the DCE + base restamp. Compare the sibling
`CRandomAmbientSound`, already converted via the standalone declared-only-virtual-
anchor variant (`class X { virtual void Vf0(); ... }`) where deriving is awkward.

## Category 1 — realized `??_7`, referenced by a manual `g_*Vtbl` extern (keep)

The extern only *names* an already-emitted (or templated, un-`VTBL`-able) vtable;
the manual store reloc-masks against the real `??_7`.

| `g_*Vtbl` extern | RVA (VA) | realized `??_7` | note |
| :--------------- | :------- | :-------------- | :--- |
| `g_zDArrayVtbl` | 0x1e70fc (0x5e70fc) | `??_7?$zDArray@P8CUserLogic@@AEHXZ@@6B@` | templated — can't `VTBL()`, stays `vtable_names.csv` |
| `g_buteTreeDtorVtbl` | 0x1e94ac | `??_7zPTree@@6B@` | templated tree |
| `g_cmdBaseVtbl` | 0x1e9674 (0x5e9674) | `??_7CGruntzCommand@@6B@` | own class emits it; stamp lives in out-of-line thunks (also cat 4) |
| `g_movieScratchVtbl` | 0x1e971c | `??_7?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@6B@` | templated CArray |

The far larger set of realized classes carry **no** manual form at all — they are
the *success* of the campaign (real `virtual` + `VTBL()`/`OVERRIDE`, `cl` emits
`??_7`): `CAmbientSound` (this pass), `CRandomAmbientSound`, `CSpotLight`, `CState`,
`CDoNothing`/`CDoNothingNormal`, `CUserBase`/`CUserLogic`, `GruntObjEntry`,
`CVtblSlot9`, the DinMgr2 device-config chain, `StreamFeeder`/`DSoundVoice`/
`CGruntzSoundInnerZ`, `CSymParser`, `CRezItmBase`, the CDDraw worker family, etc.

## Category 2 — faithful external / COM `__stdcall` + engine `__thiscall`-PMF dispatch views (keep)

~61 `struct *Vtbl` declarations, each a typed slot-view onto a **foreign** object
(class in another TU or a COM interface). A real virtual would emit a divergent
`??_7` in the wrong TU. Representative (all keep):

- COM `__stdcall` tables: nested `struct Vtbl` in `CDDSurface.h`, `NetMgr.h`,
  `GruntzMgr.h`, `DirectInputMgr2.h`, `CImage.h`, `CDirectDrawMgr.h`,
  `CDDrawShadeBlit.h`, `GameObjectCtors.cpp`; `IDDVtbl`, `ISndBufVtbl`,
  `CNetEndpointVtbl`, `PalSurfVtbl`, `PalDeviceVtbl`, `SmkBufVtbl`.
- engine `__thiscall`-PMF slot views (`mov edx,[ecx]; call [edx+N]`):
  `BootyRegistrarVtbl`, `BcImageRegistryVtbl`, `GLSRegistrarVtbl`/`GLSPolyCVtbl`,
  `MenuItemVtbl`, `TileClassVtbl`, `GameObjAux7cVtbl`, `CProjShadowVtbl`,
  `CProjSetupVtbl`, `RockCellVtbl`, `CGMInputVtbl`, `CMultiVtbl`, `CDdEnumVtbl`,
  `CDdPoolVtbl`, `CImageSource/Payload/ProviderVtbl`, `CSpriteVtbl`/`CSprite2Vtbl`,
  `AttractBusyVtbl`, `DbgVtbl`/`DbgDcVtbl`, `CChatBoxDcVtbl`, `CreditzDcVtbl`,
  `IFadeSinkVtbl`, `SbiVtbl`, `ModeObjVtbl`, `CObjVtbl`, `CVisEntityVtbl`,
  `EmRendVtbl`, `CLightVtbl`/`CPathHazardVtbl`, `CCachedSurfaceVtbl`,
  `CStatzSpriteInitVtbl`, `CImageLoad/Reload/Vtbl`, `CImageSurfaceSrcVtbl`,
  `CPoolItemAVtbl`, plus the serialize-archive PMF tables (`CMsSerialArchiveVtbl`,
  `CMgrArchiveVtbl`, `CMlSerialArchiveVtbl`, `CSerialArchiveVtbl`).

These overlap the separate *view-class* campaign (sub-object member-pointer views);
for the vtable audit they are terminal — a genuinely `this`-last-`__stdcall` COM
interface *may* later become a real abstract `__stdcall` class (as Dsndmgr's
`IDirectSound*` did), but that is a COM-modeling task, not a manual-vtable removal.

## Category 3 — MFC / engine-base layout vptr (keep)

| form | RVA | base | sites |
| :--- | :-- | :--- | :---- |
| `g_wapObjectDtorVtbl` | 0x1e8cb4 (0x5e8cb4) | `??_7CObject@@6B@` (MFC) — the SHARED grand-base dtor vtable restamped at teardown | ~20 (WwdFile, CFaderMgr, GameLevel, CDDrawSubMgr*/Worker*, CImage, Net*, ReconBatch2, Boundary*, InterfaceObject, …). One shared datum — a per-class `VTBL` would dup-DATA. |
| `g_LogicRecordBaseVtbl` | 0x1e8cb4 | same `??_7CObject@@6B@` | LogicRecord dtor exit restamp |
| `void* m_vtbl` @+0x00 | — | engine `CObArray` `{vtbl,data,count,max,grow}` base | `GruntLoadColl`, `CFaderArray`, embedded array subobjects |

## Category 4 — documented walls (`@early-stop`, keep the manual form)

Complete correct reconstructions whose vptr *shape* can't be `cl`-auto-emitted at
the reference site. Two sub-families:

**Foreign-factory / non-RTTI object vtables** (a sub-object the class placement-
`new`s + stamps; object class in another TU; not in `vtable_names.csv`):
`g_wwdObjVtbl`/`g_wwdSubVtbl`/`g_wwdGameObjectVtbl`/`g_wwd159250FinalVtbl`/
`g_wwd159440FinalVtbl`/`g_wwd1598d0FinalVtbl`/`g_wwdObjFinalVtbl`/`g_wwdGridVtbl`/
`g_wwdGridIterVtbl` (0x1f00xx), `g_planeRenderVtbl` (0x1f02a8), `g_poolItemVtbl`
(0x1ef7f0)/`g_poolItemVtbl7f0`, `g_fileImageVtbl`, `g_imageFrameVtbl`,
`g_imageSet1/2/3Vtbl`, `g_shadeArrayVtbl`, `g_streamVtbl` (0x1f0510),
`g_purecallVtbl`/`g_PureVtbl` (0x1ef760), `g_faderArrayVtbl` (0x1f0790),
`g_aniRecordVtbl`, `g_ddrawSurfacePairVtbl`/`g_ddrawSurfaceChildAVtbl`,
`g_ddrawWorker*Vtbl`/`g_drawSubWorkerVtbl`/`g_ddrawWorkerHostVtbl`,
`g_leafElemVtbl`/`g_leafScanVtbl`/`g_catalogVtbl`/`g_loadableVtbl`,
`g_tileGridCmdVtbl`/`g_tileTriggerSwitchVtbl`, `g_projActVtbl`, `g_animWorkerVtbl`,
`g_gameLevelVtbl`, `g_rezBufferObjectVtbl`, `g_typeKeyCollVtbl`/`g_typeCollRunVtbl`/
`g_zArray2DVtbl`, `g_buteTreeVtbl`/`g_buteTreeSubVtbl`/`g_buteNodeSubVtbl`/
`g_buteTreeDtorSubVtbl`, `g_deviceConfigVtblB/C` (vptr-MIDDLE), the Net node
vtables (`g_netMgrVtbl`/`g_net*NodeVtbl`/`g_net*NodeDtorVtbl`), `g_resolveNodeVtbl`,
`g_voiceTriggerVtbl`, `g_zDArrayDtorVtbl`, `g_subVtbl_5f0310`.

**Own-class placeholder walls** (the class's own vtable, but converting regresses):

| class | vtable | wall |
| :---- | :----- | :--- |
| `CLogicRecord` | own 0x1efb80 (non-RTTI) + `CObject` base 0x1e8cb4 | eh-dtor-needs-base-subobject: /GX frame needs a non-trivial `CObject` base + full (unmatched) vtable modeled. Borderline cat-5 — a leaf-first sweep item. |
| `CDDSurface` | foreign `CPoolItemA` base 0x1ef7f0 | eh-dtor-vptr-stamp-vs-trylevel-order: base vtable owned by another TU; a `cl`-emitted `??_7CPoolItemA` would diverge. |
| `CContainerErr` | own vtable | vptr-last (retail stores `m_msg` before the vptr); a real `virtual` forces vptr-first. Empirically tested → reverted (`vtable-conversion-log.md`). |
| `CGruntzCommand` | own realized 0x1e9674 | stamp lives in two standalone out-of-line thunks (real retail fns, not a ctor) `cl` can't auto-emit. |

**Not-a-vtable:** `g_keyFinderVtbl` @ 0x16e220 is a `.text` callback fn mislabeled
as a vtable (< data-section start 0x1e0000) — skip; not a vtable form.

## Follow-up (for the campaign, not this pass)

The only path to a literal grep-zero is to model the three foreign/MFC bases real-
polymorphic (`CObject` for `CLogicRecord`, `CPoolItemA` for `CDDSurface`) so their
derived vtables auto-emit, and to re-test the `CContainerErr` vptr-last member-init
shape. All three regress if forced today and are already complete + `@early-stop`;
they are leaf-first final-sweep items, **not** blockers to declaring the vtable
campaign done. No `src/`-wide manual own-class *placeholder* vtable remains.
