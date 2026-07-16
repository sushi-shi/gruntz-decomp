// Play.h - the in-game PLAY state, the
// concrete CState subclass whose Render() (vtable slot +0x14) is the
// per-frame heart of the running game: input -> per-entity step -> draw -> the
// HUD/scroll/FX overlays. See GameMode.{h,cpp} for the state hierarchy this
// extends (CState base) and CGruntzMgr::PerFrameTick @0x8b740 (the
// caller of m_curState->Render(), matched in `rezmgr`).
//
// CARCASS doctrine: only the member OFFSETS and the per-frame call/branch
// STRUCTURE are load-bearing. Field names are placeholders (m_<hexoffset>);
// unmatched engine callees are external no-body fns (reloc-masked `call rel32`);
// CPlay's own helper methods + high vtable slots are modeled so the indirect
// call shapes (`mov ecx,esi; call rel32`, `call [eax+0xNN]`) fall out. Globals
// (the frame clock, the game-manager singleton) are file-scope and reloc-mask.
//
// To avoid perturbing the matched-and-shared GameMode.h (entropy), CPlay is
// modeled here as a SELF-CONTAINED class with its own padded virtual interface
// (CState's slots 0..5 reproduced + the high slots Render dispatches to:
// +0x7c BeginFrameClear, +0x9c/+0xa0 the per-frame "slow/fast" virtuals).
#ifndef SRC_GRUNTZ_CPLAY_H
#define SRC_GRUNTZ_CPLAY_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)

// <Mfc.h> brings <windows.h> (RECT, SetRect / CopyRect / wsprintfA) and the central
// WINMM timeGetTime decl (timeGetTime is not in <windows.h> itself).
#include <Mfc.h>

// CGameRegistry - the global game-manager singleton (*g_gameReg), shared via
// <Gruntz/GameRegistry.h> with the CGrunt resolvers in Grunt.h.
#include <Gruntz/GameRegistry.h>

// The zoned sound-bank manager (CWorld::m_48) + its currently-playing inner sound
// (CPlay::m_savedZonedSound). Full defs live in <Dsndmgr/GruntzSoundZ.h> (included by
// the TUs that dispatch on them); forward-declared here so the members can be typed.
class CGruntzSoundZ;
class CGruntzSoundInnerZ;
class CBattlezData;  // CWorld::m_7c score/HUD sink (BattlezData.h; the per-kind counters)
class CChatBoxOwner; // +0x2e0 hit-test/region sink (real type; deref TUs include ChatBoxOwner.h)
// Real classes of the retyped CWorld/CPlay slots (thunk-target proven, 2026-07-13;
// forward-declared so this header stays lean - the deref TUs include the real headers):
class CFontConfig;           // CWorld::m_5c  (TypeChar @0x21e20 - the chat/key text layer)
class CWorldSoundSet;        // CWorld::m_54  (Teardown @0xb660 / Resume @0xbcf0 / Retune @0xbd60)
class CGruntSpawnConfig;     // CWorld::m_60  (ClearSprites @0x11af90 / DtorBody @0x11c7b0)
class CGruntzCmdMgr;         // CWorld::m_6c  (EnqueueSingle @0x23c30 - the marker/waypoint queue)
class CTriggerMgr;           // CWorld::m_68  (== g_gameReg->m_cmdGrid; TriggerMgr.h)
class CStatusBarMgr;         // CPlay::m_guts (+0x2dc; the 0x630-byte alloc in CPlay::Vfunc1)
class CLightFxRender;        // CPlay::m_lightFx (+0x320; the 0x43c alloc in LoadByMode)
class CTileTriggerContainer; // CPlay::m_beginMarker (+0x2e4; Serialize @0x117280)
struct CGameObject;          // CPlay::m_scrollSink (+0x4e4; the CursorSnapSprite game object)

// The per-namespace load-notify sink passed to the GRUNTZ_* installers; its
// OnLoaded() (0x4bc420 thiscall) posts a load-progress tick. Full def in CPlay.cpp.
class CMulti; // notify sink (Multi.h; AckJoinFailure 0xbc420); reloc-masked

// ===========================================================================
// Sub-object layouts CPlay::Render walks through (only the offsets it reads).
// ===========================================================================

// +0x374 start-point marker element: the {x,y} center of one placed start-point
// (FindStartPointAt walks the +-0x20 boxes; HandleMousePress the +-0x10 boxes).
SIZE(CHitMarker, 0x8);
struct CHitMarker {
    i32 m_0; // +0x00  center x
    i32 m_4; // +0x04  center y
};

// The CState +0x0c view/render/resource context (the canonical CSpriteFactoryHolder) and
// its render sub-objects (CRenderer, CDrawSurface, the placed-object warlord list)
// now live in the shared <Gruntz/View.h> so the leaf-state TUs share the one shape.
#include <Gruntz/View.h>
#include <Gruntz/ResMgr.h> // the real CState::m_c sub-object classes (CDrawTarget / CImageRegistry / CSoundRegistry / CAnimRegistry)
// (CWarlordCounters is GONE - the +0x7c counter block IS the canonical
// CBattlezData (BattlezData.h m_30/m_34/m_38/m_40 band; FillRecord/Init are its
// real methods) - the score/HUD sink, one object under two former view names.)

// (CWorldDraw is GONE - a fake view of CWorldSoundSet: its "Blit(a,b) camera blit"
// is CWorldSoundSet::Retune(pan, vol) @0xbd60 (thunk 0x1a7d) and its "Reset" is
// CWorldSoundSet::Teardown @0xb660 (thunk 0x28ab). CWorldSub60 is GONE - a fake
// view of CGruntSpawnConfig (Reset==ClearSprites @0x11af90, Method_11c7b0==DtorBody).
// CWorldLayer is GONE - a fake view of CFontConfig (Forward3508==TypeChar @0x21e20,
// the chat/key text layer). CPlayPlaneGeom is GONE - a fake view of the canonical
// CLevelPlane (<Gruntz/GameLevel.h>: m_flags/m_scaledX/m_scaledY/m_scaleX/m_scaleY).)

// (CInputDispatch is GONE (2026-07-16) - a fake view of the WAP32 CGameWnd at
// mgr+0x04 (CGameMgr::m_gameWnd): its "Bind" @0x53d4e0 (RVA 0x13d4e0) IS
// CGameWnd::PumpMessages (a declared-only duplicate identity), and its nested
// WndHost added a phantom indirection where CGameWnd::m_hwnd (+0x04) is the
// PostMessageA HWND directly - retail (e.g. 0xcbb74) loads [[this+4]+4]+4:
// this->m_4 (the mgr) -> m_gameWnd -> m_hwnd, three loads, not four.)

// (The CWorld facet view of the m_4 owner back-ptr is DISSOLVED (2026-07-16):
// it was the CGruntzMgr singleton itself - as its own comments proved three
// times over - re-declared per-header. Every field is the canonical
// <Gruntz/GruntzMgr.h> member: m_4==m_gameWnd (base CGameMgr; its former
// "ClampApply @0x8f7f0 / ManagerTick @0x8f620 / RestoreVideoMode @0x8df00 /
// ReportError @0x346d" notes are already the real RecomputeViewScale /
// RefreshGameClock / SetVideoMode / ReportError methods there), m_c==m_frameGate,
// m_10==m_soundEnabled, m_14==m_musicEnabled, m_30==m_world, m_48==m_sound,
// m_54==m_inputState, m_5c==m_chatLog, m_60==m_timer, m_68==m_cmdGrid,
// m_6c==m_cmdSubMgr, m_70==m_tileGrid, m_74==m_spriteFactory, m_7c==m_scoreHud,
// m_8c/m_90==m_modeW/m_modeH, m_94/m_98==m_savedModeW/m_savedModeH,
// m_124==m_scrollSpeed, m_128/m_134 same names; the +0x158 "flat config array
// (stride 0x238)" was m_options[g_curPlayer].m_008 - the per-player selected
// sprite descriptor (GruntzPlayer.h). Consumers use CState::m_4 directly.)

// ===========================================================================
// CState (base) - the shared canonical definition (full 41-slot vftable + the
// ctor-pinned scalar layout). CPlay's Render drives the high slots
// (BeginFrameClear/RenderSlow/RenderFast) and reads m_c/m_4 as typed pointers.
// ===========================================================================
#include <Gruntz/State.h>

// A {x,y} edge pair StepInputA overlays on the CState scroll/input block
// (the flat ints at +0x188 first half, +0x198 second half).
struct Edge {
    i32 m_0;
    i32 m_4;
};

// (CPlaySerialChild is GONE - m_beginMarker is typed as the real
// CTileTriggerContainer: Serialize @0x117280, FilterList2 @0x1170b0 - the
// per-frame "begin marker" - RemoveAll @0x116fa0 and the ~ teardown.)

// The per-frame level timer (m_frameMarker's real class): the full canonical
// CTimer lives in <Gruntz/Timer.h> (extracted from SpriteLoaders.cpp). Its
// serialize entry is HandleEvent (0x9c1c0) - the reloc-masked call SyncState
// drives; the 0x8107 timer cheat (HandleCommand) zeroes its accum/expiry/
// running/current block ("Ah, who needed that stupid timer anyway?").
#include <Gruntz/Timer.h>

// The serialize stream: the REAL CFileMemBase (<Gruntz/SerialArchive.h> typedefs
// CSerialArchive onto it). Pointer-only here, so the fwd decl + typedef suffice;
// an elaborated `struct CSerialArchive*` would re-declare a DISTINCT class and
// silently out-rank the typedef (MSVC5).
class CFileMemBase;
typedef CFileMemBase CSerialArchive;
// NOTE: this header deliberately does NOT declare `g_gameReg`. The *0x24556c
// singleton is ONE object (the real CGruntzMgr); a header-level decl forces ONE
// view's type on every includer, which is exactly what kept the CGameRegistry ==
// CGruntzMgr fold frozen: an MFC includer that wants to call a real CGruntzMgr
// method (ReportError @0x08dc60, ...) could not retype the pointer without an
// extern "C" type clash against this line, so it emitted a call to a
// ?...@CGameRegistry@@ symbol that NOTHING defines (an unbound reloc -> link
// failure). Each TU now declares the singleton itself, with the type it actually
// needs; MFC TUs use the real `extern "C" CGruntzMgr* g_gameReg;`.
// The frame image class (each grid/set row is a CImage*; full class in <Image/CImage.h>).
// Pointer-only here, so a forward decl keeps this widely-included header light.
class CImage;

// CPlay::m_grid (@+0x4cc, the level/tile frame grid the GrabTile/AdvanceTile walk drives)
// IS the canonical CImageSet (<Image/ImageSet.h>). The ex-`CFrameGrid` view is DISSOLVED
// (2026-07-15): the SAME image-registry map (m_c->m_10->m_10map) yields both the buf80a
// image SET (typed CImageSet*) and the buf80b GRID (typed CFrameGrid*) - a CMapStringToPtr
// stores one value type, so they are one class - and every field lines up: m_rowTable @+0x14
// == m_frames, m_name24 @+0x24 == m_name, m_firstRow/m_lastRow @+0x64/+0x68 == m_minIndex/
// m_maxIndex, size 0x6c; its SetDelay/SetSprite ARE SetAllTypes/SetAllFormats. Pointer-only
// here (consumers include ImageSet.h), so a forward decl suffices.
class CImageSet;

// ===========================================================================
// CPlay - the in-game PLAY state. Extends CState from +0x1a8. The per-frame
// Render reads a large block of CPlay-specific members (camera/scroll rects,
// message latches, per-region one-shot FX gates). Offsets pinned by Render.
// ===========================================================================
class CPlay : public CState {
public:
    // Construction is inlined into CGruntzMgr::TransitionState (no standalone retail
    // ctor); ~CPlay is the real 0x8c830 /GX dtor. Both defined out-of-class in their
    // owning TUs (GruntzMgrTransition.cpp / GruntzMgr.cpp).
    CPlay();
    virtual ~CPlay() OVERRIDE; // slot 0 (0x8c830)

    RVA(0x0008c910, 0x6)
    virtual GameStateId Update() OVERRIDE {
        return GAMESTATE_PLAY;
    }
    virtual i32 Render() OVERRIDE;               // THE per-frame heart (this TU)
    virtual i32 Vfunc1(i32, i32, i32) OVERRIDE;  // slot 1 (CState override)
    virtual void ReleaseResources() OVERRIDE;    // slot 2 (CState override)
    virtual i32 Vslot06() OVERRIDE;              // slot 6 (CState override)
    virtual i32 InputVirtual() OVERRIDE;         // slot 8 (CState override)
    virtual i32 Vslot09(i32) OVERRIDE;           // slot 9 (CState override)
    virtual i32 FrameSlot28(i32) OVERRIDE;       // slot 10 (CState override)
    virtual i32 Vslot0b(i32, i32) OVERRIDE;      // slot 11 (CState override)
    virtual i32 Vslot0c(i32, i32) OVERRIDE;      // slot 12 (CState override)
    virtual i32 Vslot0d(i32, i32) OVERRIDE;      // slot 13 (CState override)
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14 (CState override)
    virtual i32 Vslot0f(i32, i32, i32) OVERRIDE; // slot 15 (CState override)
    virtual i32 Vslot10(i32, i32, i32) OVERRIDE; // slot 16 (CState override)
    virtual i32 Vslot11(i32, i32, i32) OVERRIDE; // slot 17 (CState override)
    virtual i32 Vslot12(i32, i32, i32) OVERRIDE; // slot 18 (CState override)
    virtual i32 Vslot13(i32, i32, i32) OVERRIDE; // slot 19 (CState override)
    virtual i32 Vslot14(i32, i32, i32) OVERRIDE; // slot 20 (CState override)
    virtual i32 Vslot15() OVERRIDE;              // slot 21 (CState override)
    virtual void Vslot18() OVERRIDE;             // slot 24 (CState override)
    virtual void Vslot19() OVERRIDE;             // slot 25 (CState override)
    // --- CPlay-owned high slots 26..40 (moved from CState; RTTI CState is 26 slots) ---
    virtual i32 Vslot1a();  // slot 26 (0x8c930)  ret 0
    virtual i32 GetFrame(); // slot 27 (+0x6c)  current frame number (debug HUD "Frame = %i")
    virtual i32 Vslot1c(i32 category); // slot 28 (+0x70) count live objects by coll-category
    // slot 29 (+0x74): the GRUNTZ/GAME bank cache load (0x0cffe0, body in Play.cpp).
    // PROVEN virtual: retail ??_7CPlay@0x1ea0bc slot 29 -> ILT 0x1a41 -> 0x0cffe0.
    virtual i32 LoadImageBanks();
    // slot 30 (+0x78): the PLAY per-mode level loader (0x0ca200, /GX megafunction,
    // body in Play.cpp; ex the .cpp-local `CPlayLevelLoad : CPlay` facet - `this` IS
    // this CPlay). PROVEN virtual: retail slot 30 -> ILT 0x3337 -> 0x0ca200.
    virtual i32 LoadByMode(i32 level, i32 unused);
    // Slots 31..34 were each declared TWICE: once as a body-less placeholder virtual
    // holding the slot, and once as the real non-virtual method carrying the body. The
    // RTTI slot map (vtable_hierarchy) names each slot's real function, and the ILT
    // thunk in the vtable jmps straight to it - so the placeholder name was a phantom
    // (nothing defined it) AND a duplicate identity. Merged 2026-07-13:
    //   slot 31 (+0x7c) BeginFrameClear -> HandleDragMove   (ILT 0x3756 -> 0x0d0db0)
    //   slot 32 (+0x80) Vslot20         -> OnExit           (ILT 0x3f30 -> 0x0cb400)
    //   slot 33 (+0x84) Vslot21         -> FreeListTeardown (ILT 0x36ca -> 0x0cb480)
    //   slot 34 (+0x88) Vslot22         -> ModeCleanup      (ILT 0x292d -> 0x0cb740)
    virtual i32 HandleDragMove(i32 a, i32 x, i32 y); // slot 31 (+0x7c) 0x0d0db0
    virtual void OnExit();                           // slot 32 (+0x80) 0x0cb400
    virtual void FreeListTeardown();                 // slot 33 (+0x84) 0x0cb480
    virtual void ModeCleanup();                      // slot 34 (+0x88) 0x0cb740
    // slot 35 (+0x8c): present the GAME_MESSAGEZ overlay image for the state's
    // Update() id (frame 3, or 4 when Update()==7): render it centered on the
    // draw surface then flip. Body in PlayMessageImage.cpp.
    virtual i32 Vslot23();
    // slot 36 (+0x90): a genuine retail NO-OP - 0xd0030 is a lone `ret` (raw bytes
    // verified; a Ghidra recovery gap, so it is reachable only through ILT 0x1d9d
    // from the CPlay/CDemo/CMulti vtables). CPlay::Vfunc1 calls it (no args, result
    // unused) between the slot-29 and slot-30 init hooks. Defining it inline both
    // matches retail and un-phantoms the vtable reloc. (The FID row that claimed
    // 0xd0030 was LIBCMT `__fpclear` was a LOW false positive - pruned; the
    // library-overlap gate flagged the double-claim.)
    RVA(0x000d0030, 0x1)
    virtual void Vslot24() {}
    // slot 37 (+0x94): per-draw text-attr setup (debug HUD hands it the live HDC;
    // void* keeps this widely-included header windows.h-neutral).
    virtual void PostSetup(void* dc);
    virtual void Vslot26();
    // Slots 39/40: the same duplicate-declaration defect (RTTI names them DrawWorldFrame
    // / DrawWorldFrames; the vtable's ILT thunks 0x15eb / 0x311b jmp to 0xc9c20 /
    // 0xc9cc0, which are exactly those two methods' bodies in this TU).
    virtual void DrawWorldFrame();            // slot 39 (+0x9c) 0x0c9c20 (ex "RenderSlow")
    virtual i32 DrawWorldFrames();            // slot 40 (+0xa0) 0x0c9cc0 (ex "RenderFast")
    virtual i32 BuildMusicCategoryTable(i32); // slot 41 (+0xa4) 0x0dba30 (== the MIDIZ installer)
    virtual i32 BuildWorldLevelPath(i32);     // slot 42 (+0xa8) -> CWorldState::BuildWorldLevelPath

    // (the m_4w() CWorld-cast accessor is GONE - CState::m_4 is the typed
    // CGruntzMgr* already; consumers deref it directly.)

    // The start-point marker array (m_startMarkers) is a real CByteArray/CPtrArray whose
    // data(+0x374)/count(+0x378) FindStartPointAt walks directly (byte-identical to
    // the old raw m_markerData/m_markerCount fields).
    CHitMarker** markerData() {
        return *(CHitMarker***)((char*)&m_startMarkers + 4);
    }
    i32 markerCount() {
        return *(i32*)((char*)&m_startMarkers + 8);
    }
    // The same data(+4)/count(+8) reads over the four +0x3a4 placed-object arrays
    // and the +0x488 array (the serialize/free-list walks read them raw).
    void** arrData(i32 k) {
        return *(void***)((char*)&m_3a4[k] + 4);
    }
    i32 arrCount(i32 k) {
        return *(i32*)((char*)&m_3a4[k] + 8);
    }
    void** arr488Data() {
        return *(void***)((char*)&m_488 + 4);
    }
    i32 arr488Count() {
        return *(i32*)((char*)&m_488 + 8);
    }

    // CPlay's own per-frame helper methods (the thunks Render dispatches to
    // with `mov ecx,esi`). External no-body -> reloc-masked.
    // StepInputA (0x0d11e0): the per-frame CURSOR DRAW - BltFast the selected
    // cursor-half surface at the edge-fed {x,y}, error-logged via
    // CDirectDrawMgr::GetErrorString (thunk-target proven; the old "input probe"
    // reading with the fabricated __stdcall Eng_InputProbe/Eng_InputDispatch pair
    // was a fake shape).
    i32 StepInputA(); // (THIS TU)
    void StepWorldB();
    // The PLAY-state keyboard/cheat dispatcher (0xcbcc0, GameKeyHandler.cpp). Routes a
    // virtual-key to its game/cheat action; reads the guts (+0x2dc), chat sink (+0x2e0),
    // area idx and cheat globals. Non-virtual __thiscall; body in its own split TU.
    i32 DispatchKey(i32 vk, i32 lparam); // 0x0cbcc0 (GameKeyHandler.cpp)
    // (ViewPreStep/ViewPostStep are GONE - fabricated; retail's per-frame view
    // pre/post calls are StepGridWalk (0x2e2d) + winapi_0d0b30_CopyRect (0x1519).)
    // PlayCueAt: (cueId,a2,a3,a4,a5,a6,a7,rectSrc)
    void PlayCueAt(
        i32 cueId,
        i32 a2,
        i32 a3,
        i32 a4,
        i32 a5,
        i32 a6,
        i32 a7,
        i32 rectSrc
    ); // (THIS TU)
    // DrawMessageFrame (0x0d1650): look up the GAME_MESSAGEZ image set, fetch frame
    // `index`, and blit it centered in the active viewport (LayerBlitFrame @0x115300).
    // Body in PlayMessageImage.cpp.
    void DrawMessageFrame(i32 index, i32 useFront); // 0x0d1650
    void LoadSBITextEdges(char* name);              // 0x0d1710 (THIS TU)
    i32 BuildGruntNamespaceList(i32 arg);           // 0x0dd050 (THIS TU)
    // The namespace-register op IS CState::BuildAssetNamespacePrefixes
    // (0xdca70, State.h) - reached directly (CPlay inherits it from CState).
    void PostHud(i32 wParam);
    // (MarkerBegin is GONE - it is m_beginMarker->FilterList2 (0x1170b0, a
    // CTileTriggerContainer sub-object thiscall), not a CPlay-this method.)
    void StepC(); // (THIS TU)
    i32 GetAmbientId();
    void StepScroll();    // (THIS TU)
    i32 OnRegion1(i32 z); // (THIS TU)
    i32 OnRegion2(i32 z); // (THIS TU)
    i32 OnRegion3(i32 z); // (THIS TU)
    i32 OnRegion4(i32 z); // (THIS TU)
    void OnRegion5();

    // The viewport-clamp sub-steps (THIS TU): shrink/clamp the active viewport then
    // push it down the draw chain. Both share a common apply-tail.
    i32 ClampViewport(i32 inset);   // 0x0d8dc0 (THIS TU)
    i32 ClampViewport2(i32 stride); // 0x0d8ed0 (THIS TU)
    i32 NotifyVisibleEntities();    // 0x0d9050 (THIS TU)
    // ClampViewport's no-change fallback (resets the viewport then re-applies). (THIS TU)
    i32 ResetViewport(); // 0x0d8c60 (thiscall on this)
    // CPlay state-exit teardown (THIS TU): ready-gate, slot-21 notify, renderer
    // refresh, then clear the registry's per-frame words + run its +0x70 teardown.
    // CPlay state-activation (vtable slot 8; body in PlayStateActivate.cpp): chain
    // the base activate, register the level TILEZ/IMAGEZ namespaces, run the level-
    // specific init chain, kick the state timer. Reached directly by CTriggerMgr.
    i32 OnActivate(); // 0x0cb800

    // --- leaf sub-helpers the THIS-TU functions call (external, reloc-masked) ---
    void StepC_ModeA(i32 z); // (thiscall, 1 arg) StepC m_viewMode==1
    void StepC_ModeB(i32 z); // (thiscall, 1 arg) StepC else
    void RegionEnter();      // (thiscall, no arg) OnRegion on-enter
    void RegionLeave();      // (thiscall, no arg) OnRegion on-leave
    // (GutsStep/FrameTimerBegin/FrameTimerEnd/MarkerEnd/WorldBlit are GONE - they
    // were fabricated CPlay-this wrappers of sub-object thiscalls: the guts step is
    // m_guts->LoadDestructButtonSprite (0x34bd->0xffb20), the frame-marker begin/end
    // are m_frameMarker->Tick/Draw (0x3710/0x27a2, CTimer), and the world blit is
    // m_4->m_54->Retune (0x1a7d->0xbd60, CWorldSoundSet).)
    // Render-carcass leaves still unresolved (CPlay backlog; carcass-only callers):
    void SnapPostMessage(i32 wParam);
    void GutsStepB();
    void GutsStepC();
    void WorldSubstep();
    // (Overlay1/Overlay2 are GONE - they were phantom CPlay wrappers of the two
    // CLightFxRender thunks 0x1fa0/0x14dd, which retail dispatches straight on
    // m_lightFx: Resize(delta,0) + ComputeRect(m_c->m_drawTarget->m_14, &rc).)
    void InputSubStep(void* in);         // (m_4->m_70)
    void RegCue(void* sink, i32 wParam); // (reg->m_60)
    void SnapWalk();

    // --- the trace-discovered CPlay sub-steps reconstructed in this TU ---
    void ApplyGameOptions(); // 0x036be0 (options-dialogs TU: VideoConfig.cpp)
    // The two timeGetTime-instrumented frame variants (the dev profiler builds the
    // "Delta=.." / "Input=.." timing lines via the cached g_pTimeGetTime fn-ptr).
    i32 ProfileDeltaFrame(); // 0x0ca0a0 (THIS TU)
    i32 ProfileInputFrame(); // 0x0c9e40 (THIS TU)
    // 0x0cf770: the Fps/Objs/Pos/Timing/Sent debug-overlay renderer (body defined in
    // DrawDebugStats.cpp; called by Render's tail + CMulti::PumpB). Was misnamed
    // "ProfFlushTail" here and re-declared on a fake CDbgView view - one method.
    void DrawDebugStats();
    i32 DispatchHudClick(i32, i32, i32);                // 0x0ce530 (THIS TU)
    i32 BeginGridWalk(const char*, i32, i32, i32, i32); // 0x0d0920 (THIS TU)
    i32 StepGridWalk(i32 dt);                           // 0x0d0a60 (THIS TU)
    i32 ResetGoals(i32, i32);                           // 0x0d5f00 (THIS TU)
    // The status-bar HUD (SBI_RectOnly) reaches these on the current play-state
    // (g_gameReg->m_curState downcast to CPlay); reloc-masked, bodies out-of-line.
    // 0x0d5b20 was declared here as the fake `SetState(cur, prev)` while its BODY lived
    // under a fake class (CLevelValidator, LevelTileValidation.cpp) - so SBI_RectOnly
    // emitted a call to ?SetState@CPlay@@QAEHHH@Z, a symbol NOTHING defines (an unbound
    // reloc -> link failure), and the real body sat under ?...@CLevelValidator@@.
    // CLevelValidator IS CPlay (proven: CPlay::ResetPlayState @0x0d60b0 calls its sibling
    // PlaceStartGruntz with `mov ecx,esi` - the SAME `this` it writes at +0x4f8 - and both
    // views type +0x2e0 as CChatBoxOwner*). The body is now homed here, under its real name:
    // it repositions the game-timer HUD widget (+0x3f4 CTimer) at a fixed inset from the
    // screen size (m_4->m_modeW/m_modeH) with the mode + X inset chosen by `mode`.
    i32 PositionBridgeToggle(i32 mode, i32 unused); // 0x0d5b20 (body: LevelTileValidation.cpp)
    // The other two ex-CLevelValidator methods, homed here for the same reason (that fake class
    // IS CPlay - see the fold note in LevelTileValidation.cpp, where all three bodies live).
    i32 PlaceStartGruntz();   // 0x0d2b20 (called by ResetPlayState @0x0d60b0 on this same `this`)
    i32 ValidateLevelTiles(); // 0x0d2dd0
    i32 HiRefresh(i32 a);     // 0x0d6560  highlight-cursor refresh
    // BuildHelpReveal (0x0d72c0, THIS TU): the LOADING-BAR wipe tick - retail rets
    // 0x4, so it takes ONE i32 (the old no-arg decl had a DROPPED PARAMETER; every
    // caller pushes 0, the LoadByMode finale pushes 1). Its m_revealFrame/m_revealCap*
    // members are the loading-bar counter + frame sprites (LoadLoadingBarSprite).
    i32 BuildHelpReveal(i32 final); // 0x0d72c0 (THIS TU)
    i32 RegisterInputBindings();    // 0x0d9160 (THIS TU)
    // (LoadByMode moved up to its PROVEN vtable slot 30.)
    // LoadLevelAnims (0x0db750, THIS TU): the LEVEL-namespace anim loader - the
    // missing sibling of LoadLevelSounds/LoadLevelImages (ex the .cpp-local
    // `Cdb750::SyncLevelKey` orphan view: its +0x0c holder is CState::m_c and its
    // +0x28 symtab is CState::m_levelBank).
    i32 LoadLevelAnims(i32 force); // 0x0db750
    // DrawLevelInfoText (0x0d95f0, THIS TU): the full-screen level/grunt info-text
    // panel painter (ex the `GruntInfoTextHost` placeholder view - its +0x0c render
    // surface is CState::m_c, +0x1c is m_levelIndex, +0x20 is m_levelType).
    i32 DrawLevelInfoText(); // 0x0d95f0
    // LoadLoadingBarSprite (0x0d7440; body in SpriteLoaders.cpp): cache the
    // GAME_LOADINGBAR frames 1..3 into m_revealCap* + set m_revealFrame=1 (ex the
    // `CLoadingBar` view - its +0x0c resmgr is CState::m_c and its +0x4bc..+0x4c8
    // block is exactly m_revealFrame/m_revealCapMid/End/Start).
    i32 LoadLoadingBarSprite(); // 0x0d7440
    // Tiny vtable forwarder: tail-call the slot-3 ready gate (Vfunc3).
    i32 ForwardReady(); // 0x0cee70 (out-of-line: tail-call the slot-3 ready gate Vfunc3)
    // Region pause/resume pair (vtable slots 24/25, shared by CDemo/CMulti):
    // PauseGame saves the game clock into m_savedClock + freezes the world; ResumeGame
    // restores the clock + unpauses. Migrated from engine_boundary (CPlay).
    i32 PauseGame();  // 0x0cee90
    i32 ResumeGame(); // 0x0cef00
    // FrameSlot28's own-this reloc-masked callee (external body elsewhere; invoked
    // with ecx=this in the status/pause overlay). QuitToMenu (0x0cef50, no-arg notify
    // when the m_40 latch is set). (0x0fa8f0 is CState::RetireScene - the status-message
    // ticker, inherited from the CState base, called cast-free; no CPlay decl.)
    // QuitToMenu (0x0cef50, ex "Method_cef50"): clear the manager's world-file name and,
    // when the level-quiesce latch m_1c0 is set, close the DDraw worker manager and ask
    // the game manager for state 3 (the menu). Name is evidenced by the body, not invented.
    i32 QuitToMenu(); // 0x0cef50
    // The HandleCommand cheat receivers (reloc-masked; reached via the play-state
    // lookup PickPlayOrPausedState): SetCursorFrame gives the selected grunt item
    // `item` (the 0x80e5..0x8104 ITEMCHEAT family; thunk 0x17a8); Flip returns the
    // AMBIENT%d variant index for the 0x8086 Monolith cheat (thunk 0x1df2).
    i32 SetCursorFrame(i32 item); // 0x0d1b30
    // 0x0d1b60 (ret 0x1c; body in PlayerCommandStep.cpp) - the player-command
    // executor (ex ?Dispatch@CCmdHandler@@ - that view WAS this play state; the
    // CGruntzCommand::ApplyOne/ApplyMask thunk 0x21e4 dispatches it on this play
    // state). Switches on (u8)a4 over the mgr's m_cmdGrid board.
    // PACKED param types (char/i16), settled 2026-07-15 (ex the CGruntzCmdTarget
    // caller-side shim): the Apply* callers push the command's byte/word fields
    // UNEXTENDED (mov dl,[eax+6]; push edx), which only a narrow-param decl
    // reproduces - and MSVC5 reads a narrow stack param as its full dword slot +
    // AND mask ((u8)aN => `mov reg,[esp+N]; and reg,0xff`), which is exactly the
    // retail body's read pattern, so ONE honest signature serves both sides.
    i32 ExecCommand(char a2, char a3, char a4, i16 a5, i16 a6, char a7, char a8);
    i32 Flip(); // 0x0da200
    // Level-lifecycle steps (ex the "CGameModeObj" view, GameModeObjLifecycle.cpp;
    // folded onto CPlay wave3-J - the +0x3a4/+0x2dc/+0x4fc/+0x1cc offsets pin it):
    i32 ReleaseLevelOverlay(i32 unused); // 0x0d6560  drop the overlay + restore the clock
    i32 ClearPlacedObjects();            // 0x0da030  sweep the 4 placed-object arrays
    i32 FlushPendingOps();               // 0x0da2d0  flush the deferred guts ops
    // ArmSnapshot (0x0d9240): latch the snapshot timer (base=clock, dur=arg2) and
    // the active flag (arg1). CanQuickSave (0x0da3b0): all-idle predicate gating
    // the auto/quick path. PostHudRect (0x0da440): post the HUD rect to the world
    // timeline then clear the ready/drag-snap gates. Migrated from engine_boundary.
    i32 ArmSnapshot(i32 active, i32 dur); // 0x0d9240
    i32 CanQuickSave();                   // 0x0da3b0
    i32 PostHudRect();                    // 0x0da440
    // Two more draw/present sub-steps migrated from the engine_boundary backlog:
    i32 DrawWorldPresent(); // 0x0cefc0 (double world-draw + present + manager tick)
    i32 PresentAndFlush();  // 0x0cba10 (restore-mode guard + present-or-notify + flush)
    // Overlay sub-step migrated from the engine_boundary backlog:
    i32 EnterOverlayDrag(i32 arg); // 0x0d6440 (arm overlay-drag + guts busy words)
    // (HudClickInRect/DragHudInRect/ApplyOverlay3e59 are GONE - all three were
    // WRONG-this fakes: retail dispatches them on m_lightFx (+0x320), and they are
    // CLightFxRender::ClearHandle @0xa9500 / ApplyB @0xa95d0 / ApplyA @0xa9480.
    // DragSnapTo is GONE - it is m_guts->SetSpritePos (0x3878 -> 0xfe860).
    // EndDragSel is GONE - the 0xda2d0 target IS CPlay::FlushPendingOps itself.
    // FindStartPointAt309e is GONE - thunk 0x309e IS FindStartPointAt (0xd5f90).)

    // ---- per-level resource loaders (trace-discovered, THIS TU) ----
    // Each casts `this` to a typed loader view (CPlay.cpp): the +0xc resource
    // manager (with its m_10/m_28/m_2c sub-registries) and the +0x28/+0x34 bank
    // sources are reached through offset-specific sub-types that Render models
    // differently, so a single struct-view cast at entry keeps Render's matched
    // member typing untouched.
    // (LoadImageBanks moved up to its PROVEN vtable slot 29.)
    i32 LoadActionTileSprites(i32 force);         // 0x0db600
    i32 LoadLevelSounds(i32 force);               // 0x0db6c0
    i32 LoadLevelImages(i32 force);               // 0x0db7e0
    i32 LoadGameImages(i32 force);                // 0x0db8a0
    i32 LoadGameSounds(i32 force);                // 0x0db930
    i32 LoadGameAnims(i32 force);                 // 0x0db9b0
    i32 LoadGruntSoundNamespaces(CMulti* notify); // 0x0dd830 (GRUNTZ_* sound installer)
    i32 BuildSpriteImageKeyTable(CMulti* notify); // 0x0dd540 (GRUNTZ_* image installer)
    i32 BuildAnizKeyTable(CMulti* notify);        // 0x0ddaa0 (GRUNTZ_* anim installer)

    // ---- the keyboard/UI command dispatcher (THIS TU) ----
    i32 OnKeyCommand(i32 key, i32 flag); // 0x0cbaf0
    // Two large play-state sub-steps the dispatcher tail-calls (external/reloc-masked;
    // deferred to the final sweep): the mode-enter gate (0x0d6fa0) and the per-frame
    // play-state reset (0x0d60b0).
    i32 EnterMode(i32 mode); // 0x0d6fa0
    i32 ResetPlayState();    // 0x0d60b0

    // ---- the trace-discovered CPlay __thiscall cluster (THIS TU) ----
    // ResetForMode (0x0c8a10): capture+hide the cursor, enter a mode, then reset
    // the per-frame drag/world-ready state and three world sub-objects.
    i32 ResetForMode(i32 mode); // 0x0c8a10
    // FindStartPointAt (0x0d5f90): registry-gated hit-test over this->m_374[] +-0x20
    // marker boxes; outputs the matched marker's coords. ret 0x10 (4 args).
    i32 FindStartPointAt(i32 x, i32 y, i32* outX, i32* outY); // 0x0d5f90
    // FreeListTeardown (0x0cb480): release the per-level allocations back onto the
    // global free list (m_374[]/m_3ac[]/m_48c[] arrays + the per-type config rows).
    // CPlayDtorBody (0x0c8700): the ~CPlay teardown body - free the per-frame
    // workers (m_320/m_guts/m_hitTest/m_beginMarker/m_frameMarker), clear the four g_gameReg config
    // rows, flush the m_startMarkers/m_3a4[4]/m_488 free-list arrays, then run the base dtor.
    void CPlayDtorBody(); // 0x0c8700
    // AddLevelGruntz (0x0d5960): walk the registry object list and register each
    // valid grunt object with the session; logs "Could not add Grunt" on failure.
    i32 AddLevelGruntz(); // 0x0d5960
    // SetEffectSpriteDurations (0x0dc060): stamp the +0x18 duration on each named
    // effect-sound descriptor looked up in the sound registry's name map.
    i32 SetEffectSpriteDurations(); // 0x0dc060
    // BuildWarlordNameTable (0x0dd340): probe the 0x39/0x3a warlord ids then bind the
    // NAPOLEAN/VIKING/PATTON CString names. CString temps force the /GX EH frame.
    i32 BuildWarlordNameTable(i32 arg); // 0x0dd340
    // ResetPlayState's own reloc-masked CPlay-thiscall leaves (external):
    void ResetGoalGeom(i32 lo, i32 hi); // 0x2e28 thunk  (this, lo, hi)
    i32 PrepareReset();                 // 0x1d75 thunk  (this) -> proceed gate
    // FreeListTeardown's reloc-masked CPlay-thiscall leaf (external):
    void Teardown1780(); // 0x1780 thunk  (this) early teardown step
    // BuildWarlordNameTable/LoadWarlordSprites leaves: ProbeWarlord IS
    // CPlay::BuildGruntTypeNameTable (0xdc6d0); BindWarlordName IS
    // CState::BuildAssetNamespacePrefixes (0xdca70, inherited from CState).
    // LoadWarlordSprites (0x0d65d0): ensure every sprite set a placed warlord needs is
    // loaded - full campaign preload (registry m_134 != 1) or the in-level walk of the
    // placed-object display list (renderer A's m_10). Re-homed from the ApiCaller
    // backlog; reuses ProbeWarlord (0x12da) + BindWarlordName (0x2bc1). WarlordLoadTick
    // (0x1019) is the per-set progress tick.
    i32 LoadWarlordSprites(i32 ctx, i32* loaded); // 0x0d65d0
    // (WarlordLoadTick is GONE - thunk 0x1019 IS BuildHelpReveal (0xd72c0): the
    // per-set "progress tick" is literally one loading-bar wipe tick.)

    // SyncState (0x0d7520): the mode-dispatched serialize/round-trip of the play
    // state's 64-bit timer blocks + three child sync sub-objects (guts / frame
    // marker / begin marker); mode 8 (re)inits the ambient-sound cue. mode 4 =
    // write (archive vtbl[0x30]), mode 7 = read (archive vtbl[0x2c]).
    i32 SyncState(CSerialArchive* ar, i32 mode, i32 a2, i32 a3); // 0x0d7520
    // SyncState's own reloc-masked CPlay-thiscall leaves (external, no body):
    i32 HeaderSerialize(CSerialArchive* ar, i32 mode, i32 a2, i32 a3); // 0x4016 thunk
    i32 SyncWrite19fb(CSerialArchive* ar);                             // 0x19fb thunk (mode-4)
    i32 SyncRead2f7c(CSerialArchive* ar);                              // 0x2f7c thunk (mode-7)

    // ---- CPlay-specific members (offsets pinned by the Render disasm) ----
    i32 m_inputWarmup1; // +0x1a8  StepInputA first-frame one-shot latch
    i32 m_inputWarmup2; // +0x1ac  StepInputA second-frame one-shot latch
    i32 m_inputHalfSel; // +0x1b0  StepInputA mirrored-half selector (0/1)
    // +0x1b4: the first of five destructible MFC members ~CPlay tears down (reverse
    // decl order); typed here so the dtor's /GX member fold falls out (GruntzMgr.cpp).
    CString m_1b4;                // +0x1b4
    char m_pad1b8[0x1bc - 0x1b8]; // +0x1b8
    i32 m_1bc;                    // +0x1bc  Dispatch re-post gate (WM_COMMAND 0x8023 while set)
    i32 m_1c0; // +0x1c0  Dispatch level-quiesce latch (set 1 on level index 0x20)
    i32 m_1c4; // +0x1c4  deferred-draw gate (LoadByMode sets 1; EnterMode consumes; serialized)
    char m_pad1c8[0x1cc - 0x1c8]; // +0x1c8
    i32 m_savedClock; // +0x1cc  saved game clock (PauseGame stashes / ResumeGame + teardown restore to g_frameTime)
    i32 m_1d0; // +0x1d0  cleared by the ~CPlay teardown body
    char m_pad1d4[0x2d0 - 0x1d4];
    i32 m_packetsRcvd; // +0x2d0  net packets received (debug HUD "Rcvd = %i")
    i32 m_packetsSent; // +0x2d4  net packets sent (debug HUD "Sent = %i")
    i32 m_rngSeed;     // +0x2d8  (CMulti RNG seed)
    // +0x2dc: the "guts"/UI subsystem IS the canonical CStatusBarMgr - proven by the
    // allocation site (CPlay::Vfunc1 @0xc7fea: `push 0x630; call ??2` stored at
    // CPlay+0x2dc == SIZE(CStatusBarMgr, 0x630)) and by every dispatched thunk
    // resolving to a CStatusBarMgr method (HitTest/ClearStat/CommitSlot/SetFallRect/
    // EnterHlRow/HitTestLayer/PlaceCursorTarget/UpdateStatusBarTabHighlight/
    // ClickToggle/ClearTabSprites/Deactivate/SetSpritePos/LoadDestructButtonSprite/
    // LoadMainStatusBarSprite/winapi_0fe520_SetRect/RefreshA/HideRect). The former
    // `GutsSubsystem` view is DISSOLVED; field map: m_state==m_position,
    // m_rect10=={m_10,m_rect14.m_0/m_4/m_8}, m_mode==m_activeTab, m_548==m_hlBusy,
    // m_busyA/m_busyB==m_toggleActive/m_toggleHandle, m_snapPostSel==m_modeArmed,
    // m_614==m_barFrameGate.
    CStatusBarMgr* m_guts; // +0x2dc
    // +0x2e0: a hit-test/region sink; the real CChatBoxOwner (HandleDragMove:
    // m_hitTest->HitTest(x, y); m_10 = the +0x10 active-overlay gate).
    CChatBoxOwner* m_hitTest;
    // +0x2e4: the begin-marker sink - the real CTileTriggerContainer (FilterList2 is
    // the per-frame "begin marker"; Serialize @0x117280 is SyncState's child sync).
    CTileTriggerContainer* m_beginMarker; // +0x2e4
    i32 m_dragSnapActive; // +0x2e8  drag-snap-active latch (HandleDragMove snap path)
    i32 m_dragInProgress; // +0x2ec  box-drag-in-progress latch (HandleDragMove)
    i32 m_2f0;            // +0x2f0
    i32 m_cursorFrame; // +0x2f4  latched cursor sprite frame (SetCursorFrame; FlushPendingOps re-arm)
    i32 m_levelId;     // +0x2f8  level/region id (==0x66 -> booty-region init)
    i32 m_2fc, m_300;    // +0x2fc  serialized 8-byte pair (SyncWrite19fb)
    i32 m_dragClampMaxX; // +0x304  drag-clamp max X
    i32 m_dragClampMaxY; // +0x308  drag-clamp max Y
    i32 m_worldReady;    // +0x30c  world-ready gate (0 until inited)
    RECT m_hudRect;      // +0x310  HUD/selection rect buffer fed to the HUD draw
    // +0x320: the level light-FX overlay - the real CLightFxRender (LoadByMode
    // allocates its 0x43c bytes here + Init/BuildShape; the click/drag paths run
    // ApplyGlobal/ApplyA/ApplyB/ClearHandle on it; ~CPlay frees it). Doubles as the
    // "show-overlay/banner" gate (null-checked). Was the untyped i32 m_overlayActive.
    CLightFxRender* m_lightFx; // +0x320
    char m_pad324[0x328 - 0x324];
    i32 m_bootyTimerLo, m_bootyTimerHi, m_bootyInterval,
        m_bootyIntervalHi; // +0x328  booty-region 64-bit timer
    i32 m_ambientTimerLo, m_ambientTimerHi, m_ambientInterval,
        m_ambientIntervalHi; // +0x338  ambient-init timer
    i32 m_ambientInitDone;   // +0x348  ambient-init DONE latch
    char m_pad34c[0x350 - 0x34c];
    i32 m_syncTimerLo, m_syncTimerHi, m_syncInterval,
        m_syncIntervalHi; // +0x350  play-state 64-bit sync timer (SyncState first block)
    i32 m_tileClickX;     // +0x360  tile-click snapped X (HandleTileClick)
    i32 m_tileClickY;     // +0x364  tile-click snapped Y (HandleTileClick)
    i32 m_dragInhibit1;   // +0x368  drag/select inhibit gate
    i32 m_dragInhibit2;   // +0x36c  drag/select inhibit gate
    // +0x370: a ::CPtrArray of start-point markers (the 2nd destructible member);
    // FindStartPointAt reads its data(+4)/count(+8) via markerData()/markerCount().
    // It is CPtrArray, NOT CByteArray: ~CPlay (0x08c830) does `lea ecx,[esi+0x370] /
    // call 0x1b4f3e`, and 0x1b4f3e lies in [0x1b4f0b, 0x1b527e) - the band whose head
    // ctor 0x1b4f0b DIR32s ??_7CPtrArray@@6B@ (0x1ec2dc).  CByteArray's dtor is 0x1b52b1
    // (band head 0x1b527e, vtable 0x1ed28c) and retail never calls it here.  The four
    // MFC array classes are byte-identical, so every FID row there is AMBIG.
    //     python -m gruntz.analysis.mfc_class 0x1b4f3e
    CPtrArray m_startMarkers; // +0x370  (data@+4 = marker-ptr array, count@+8 = marker count)
    // +0x384: the 4 world fx-spawn anchors (stride 8). SpawnTileFx (0x79ea0) maps the
    // (a3-1) tile-effect id into m_anchors[idx] and spawns at {m_x, m_y}. (Ex the
    // TriggerMgrViews.h `CTmWorld::Anchor` view; m_curState IS this CPlay.)
    struct Anchor {
        i32 m_x;
        i32 m_y;
    };
    Anchor m_anchors[4]; // +0x384  (4 * 8 = 0x20)
    // +0x3a4: the 4 placed-object record arrays. CPtrArray (not CByteArray):
    // ClearPlacedObjects (0xda030) reads the elements as CPlacedObj* and retail
    // calls RemoveAt with ecx = this+0x3a4+idx*0x14 (the ARRAY base, vptr-ful
    // CObject-derived MFC array of pointers). Same 0x14 size / dtor fold.
    CPtrArray m_3a4[4]; // +0x3a4  (4 * 0x14)
    CTimer*
        m_frameMarker; // +0x3f4  frame-marker/timeline CTimer (SyncState serialize; 0x8107 cheat)
    i32 m_cueTimerLo, m_cueTimerHi, m_cueInterval,
        m_cueIntervalHi; // +0x3f8  AMBIENT-cue 64-bit timer
    i32 m_cueToggle;     // +0x408  AMBIENT-cue on/off toggle
    i32 m_lastCueId;     // +0x40c  PlayCueAt last-shown cueId (de-dupe gate)
    CString
        m_cueText; // +0x410  4th destructible member (PlayCueAt reads &m_cueText as its de-dupe state)
    i32 m_drewThisFrame; // +0x414  per-frame "drew" flag (cleared at entry)
    // +0x418..+0x428: the serialized scalar block SyncWrite19fb streams after the
    // cue-text (five i32 + one i16); roles unrecovered (write-only in matched code).
    i32 m_418, m_41c, m_420, m_424; // +0x418
    i16 m_428;                      // +0x428
    char m_pad42a[0x430 - 0x42a];
    i32 m_region0TimerLo, m_region0TimerHi, m_region0Interval, m_region0IntervalHi; // +0x430
    i32 m_region1TimerLo, m_region1TimerHi, m_region1Interval, m_region1IntervalHi; // +0x440
    i32 m_region2TimerLo, m_region2TimerHi, m_region2Interval, m_region2IntervalHi; // +0x450
    i32 m_region3TimerLo, m_region3TimerHi, m_region3Interval, m_region3IntervalHi; // +0x460
    i32 m_region0Gate;   // +0x470  region-0 gate (OnRegion2 / extra HUD layer)
    i32 m_region1Gate;   // +0x474  region-1 gate (OnRegion1 / alt-input draw)
    i32 m_region2Gate;   // +0x478  region-2 gate (OnRegion3)
    i32 m_region3Gate;   // +0x47c  region-3 gate (OnRegion4)
    i32 m_viewMode;      // +0x480  StepC/OnRegion view-mode discriminator (0=idle/1/2)
    i32 m_hudSuppressed; // +0x484  HUD-suppress gate (DispatchHudClick early-out)
    // ::CPtrArray (same proof: ~CPlay does `lea ecx,[esi+0x488] / call 0x1b4f3e`;
    // mfc_class 0x1b4f3e => CPtrArray).
    CPtrArray m_488; // +0x488  5th destructible member (0x14 bytes)
    i32 m_49c;       // +0x49c  serialized scalar (teardown sets -1)
    i32 m_snapBaseLo, m_snapBaseHi, m_snapDur,
        m_snapDurHi;        // +0x4a0  snapshot 64-bit base + duration
    i32 m_snapshotActive;   // +0x4b0  snapshot ACTIVE latch
    i32 m_scrollEdgeActive; // +0x4b4  edge active bits
    i32 m_scrollEdgeLock;   // +0x4b8  edge lock bits
    i32 m_revealFrame;      // +0x4bc  reveal-strip frame counter (BuildHelpReveal)
    // +0x4c0  reveal-strip cap sprite objects (passed by-ptr to the HUD-strip draw).
    void *m_revealCapMid, *m_revealCapEnd, *m_revealCapStart;
    // +0x4cc: the level/tile frame grid GrabTile/AdvanceTile walk (canonical CImageSet)
    CImageSet* m_grid;      // +0x4cc  level tile/frame grid (canonical CImageSet)
    CImage* m_gridCurFrame; // +0x4d0  current tile/frame image (a CImageSet row)
    i32 m_gridHasSprite;    // +0x4d4  has-grid-sprite flag
    i32 m_gridDelayBase;    // +0x4d8  step-delay base
    i32 m_gridDelayCount;   // +0x4dc  step-delay countdown
    i32 m_gridRow;          // +0x4e0  current row index
    // +0x4e4: the CursorSnapSprite GAME OBJECT (LoadByMode stores the
    // CreateSprite(..., "CursorSnapSprite", ...) result here) - the real
    // CGameObject (<Gruntz/UserLogic.h>): its m_stateFlags (+0x40) bit0 is the
    // drag/select active bit and its m_screenX/m_screenY (+0x5c/+0x60) receive
    // StepScroll's snapped scroll offsets. Was the `ScrollSink` view.
    CGameObject* m_scrollSink; // +0x4e4
    i32 m_gridWalkActive;      // +0x4e8  grid-walk active flag
    i32 m_renderDisabled;      // +0x4ec  Render hard early-out gate
    i32 m_4f0;             // +0x4f0  highlight-busy gate (SBI_RectOnly reads it non-zero => bail)
    i32 m_winLoseBanner;   // +0x4f4  win/lose banner gate
    i32 m_inGame;          // +0x4f8  PRIMARY mode: nonzero = main in-game frame
    i32 m_overlayDrag;     // +0x4fc  overlay-drag-active flag
    i32 m_paused;          // +0x500  paused/no-step flag
    i32 m_dragEndNotify;   // +0x504  drag-end notify gate
    i32 m_lastScrollTimeX; // +0x508  last-scroll time (horizontal)
    i32 m_lastScrollTimeY; // +0x50c  last-scroll time (vertical)
    i32 m_stepCountdown;   // +0x510  per-frame entity-step countdown
    i32 m_514;             // +0x514  serialized scalar (SyncWrite19fb; LoadByMode seeds 3)
    CGruntzSoundInnerZ*
        m_savedZonedSound; // +0x518  saved currently-playing zoned sound (region pause/resume)

    // Engine-label backlog stubs.
    void PlayBacklog08c9d0();
    i32 winapi_0cdb10_PostMessageA(i32, i32, i32);
    // HandleTileClick (0xceae0): the menu/pause-state pointer-click handler - the
    // mouse-input twin of OnKeyCommand. Gated resume/report/unpause chain, then an
    // overlay probe + a HUD hit-test + a grid-snapped world marker place/cancel.
    i32 HandleTileClick(i32 a, i32 x, i32 y);
    i32 winapi_0d0b30_CopyRect(i32);
    i32 LoadCursorSprites(i32 frame, i32 flag);
    i32 LoadScrollSpeedOptions();
    i32 BuildGruntTypeNameTable(i32, i32, i32, i32);

    // HandleMousePress (0x0ce660): vtable slot 16 (+0x40) - the in-game
    // pointer/click dispatcher (mouse sibling of OnKeyCommand). Re-homed from
    // GameMouseHandler.cpp; reaches the guts/status-bar sub-objects at m_guts /
    // m_hitTest / m_4 / m_c which that TU casts to its local facet views.
    i32 HandleMousePress(i32 msg, i32 x, i32 y); // 0x0ce660

    // The two per-frame plane-list sub-steps re-homed from CPlayPlaneScan.cpp: walk
    // the renderer's embedded plane list (m_c->renderer+0x10) and dispatch on each
    // plane descriptor's type. Both take a stack MFC temp -> /GX.
    i32 ScanBuildTiles();   // 0x0d53d0
    i32 ScanShuffleQuads(); // 0x0d9290
};

// ===========================================================================
// The frame-clock + singleton globals CPlay::Render reads each frame.
// ===========================================================================
// The dev/render-state singleton DispatchHudClick reads (*g_spawnConfig); its +0x18 is
// a flags word masked with 0x20 to gate the HUD-rect post.
struct StateMgrBZ {
    i32 m_0, m_4, m_8; // +0x00..+0x08
    char m_padc[0x10 - 0xc];
    i32 m_10, m_14; // +0x10, +0x14
    i32 m_18;       // +0x18  flags
    i32 Flush();    // 0x385e0 (?Flush@) (the +0x578 state-mgr flush)
};

extern "C" {
    extern i32 g_lastNow;    // 0x245580 (-> mirror g_killCueClock; also in <Rez/FrameClock.h>)
    extern u32 g_frameDelta; // 0x245584 (frame delta cap)
    extern u32 g_frameTime;  // 0x245588 (the running game clock)
    extern StateMgrBZ* g_spawnConfig; // the dev/render-state singleton (DispatchHudClick)
    extern "C" i32 g_curPlayer;       // a default cue/message wParam
    extern u32 g_killCueClock;        // draw-clock mirror
    extern u32 g_engineFrameDelta;    // draw-delta mirror
}

#endif // SRC_GRUNTZ_CPLAY_H
