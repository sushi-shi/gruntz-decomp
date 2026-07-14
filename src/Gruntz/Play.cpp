// Play.cpp - CPlay::Render: the in-game PLAY state's per-frame
// step+draw, the heart of the running game. CARCASS reconstruction: the control
// flow, the CPlay/CState/CGameRegistry member offsets, and the ordered per-frame
// call sequence are faithful; field/callee names are placeholders and unmatched
// engine callees are external no-body fns (reloc-masked). See CPlay.h for the
// layout + helper-method map.
//
// =====================  THE PER-FRAME CARCASS (the deliverable)  ============
// CPlay::Render runs once per game frame (RezMgr::PerFrameTick -> m_mode->
// vtbl[+0x14]). It is a 3-way dispatch on two state words (m_renderDisabled the hard
// early-out, m_inGame the primary mode) wrapped in a C++ SEH/EH frame (a stack
// CString temp in the alt path -> /GX). The three paths share one WORLD-DRAW
// block and a common return-1 tail.
//
//   this->vtbl[+0x7c](0, m_cursorX, m_cursorY);          // BeginFrameClear (a virtual)
//   m_drewThisFrame = 0;                                    // per-frame "drew" flag
//   if (m_renderDisabled) return 1;                          // hard early-out
//
//   if (m_inGame) {                  // ---- MAIN in-game frame ----
//       StepInputA(); StepWorldB(); ViewPreStep(m_c->m_24);
//       g_killCueClock=g_645580; g_engineFrameDelta=g_frameDelta;     // mirror the draw clock
//       DRAW_WORLD();                             // shared world-draw block
//       <AMBIENT-cue timer +0x3f8, 0x1f4ms, toggles m_cueToggle -> PlayCueAt 0x8128>
//       MarkerBegin(now); GutsStep();             // m_beginMarker marker + m_guts step
//       if (m_c->m_drawTarget->m_14 == 0) return 1;        // no view -> bail
//       FrameTimerEnd; DrawSurfaceFlush; GutsStepX; ViewPostStep; return 1;
//   } else if (m_4->m_c==0 && !(reg->m_134!=2 && m_overlayDrag)) {
//                                 // ---- MENU/PAUSE-OVERLAY frame ----
//       FrameTimerBegin(now); Eng_FrameTimerStep(m_4->m_6c,0);
//       if (m_levelId==0x66) <booty-region one-shot +0x328 0x2710ms -> RegCue 0x33e>;
//       StepInputA(); StepC();
//       if (m_ambientInitDone==0) <AMBIENT level-init: wsprintf "AMBIENT%d" -> PlaySound/
//                      FindSound -> latch m_ambientInitDone=1>;
//       if (m_region0Gate) { Eng_BeginScene(m_c->m_4->m_14->m_2c); GutsStepB(); }
//       if (m_worldReady==0) { if (m_4->m_68->m_armed) WorldSubstep(); StepWorldB(); }
//       StepScroll();
//       if (0x12<dt && dt<0xc8) RenderFast() else RenderSlow();   // UNSIGNED gate
//       DRAW_WORLD();                             // same shared world-draw block
//       InputSubStep(m_4->m_70);
//       if (m_overlayActive && m_guts->state-ok) ShowOverlay();   // on-screen banner
//       WorldBlit(g_frameDelta);
//       if (m_c->m_4->m_14==0) return 1;
//       if (m_snapshotActive) SnapshotStep();                // screenshot countdown +0x4a0
//       <four scroll-region one-shots at +0x430/+0x440/+0x450/+0x460>; return 1;
//   } else {                      // ---- m_4->m_c != 0 short path ----
//       StepInputA();
//       if (m_c->m_4->m_14==0) return 1;
//       <sound tick m_c->m_soundStream: timeGetTime, PurgeVoiceList+TickSubManagers>;
//       if (m_paused) DRAW_ONLY()                    // paused: present + win/lose FX
//       else { if (--m_stepCountdown>0) <entity step + level cue>; if (m_ambientInitDone==0) AmbientInit(); }
//       MarkerBegin(now); PostHud(0); DrawSurfaceFlushTail(); return 1;
//   }
//
// THE SHARED WORLD-DRAW BLOCK (verbatim across both world-draw sites):
//   m_c->m_8->vtbl[+0x24](0);                     // renderer A: begin scene
//   Eng_PushView(m_c->m_24, m_c->m_4->m_14, m_c->m_8);
//   m_c->m_c->vtbl[+0x34](m_c->m_4->m_14, m_c->m_4->m_18);          // present
//   WorldBlit(m_c->m_24->m_5c->m_84, ->m_88) on m_4->m_54;
//   if (m_c->m_soundStream) { t=timeGetTime(); PurgeVoiceList(t); TickSubManagers(t); }  // sound tick
//   MarkerBegin(g_frameDelta); GutsStep();                             // marker + guts
//
// The per-ENTITY layer is one indirection down: g_entityList is walked
// by the world object (m_4->m_54) inside the reloc-masked camera-blit call. The
// next targets are CPlay's own sub-steps + the world-draw helpers
// (push-view, surface-flush, camera blit, HUD).
// ============================================================================

#include <Gruntz/Play.h>
#include <Io/FileMem.h>     // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/AreaMgr.h> // CAreaMgr (g_pAreaMgr; CPlayLevelLoad::LoadByMode, waveP)
#include <Gruntz/AssetNamespaceLoader.h> // CNamespaceLoader (BuildAssetNamespacePrefixes, waveP)
// The GRUNTZ_/GAME image worker registry (owner+0x10): 18 vtable slots then the
// virtual LoadTree at +0x48; plus the non-virtual key probe + direct-load (same shape
// as <DDrawMgr/DDrawAssetRegistryViews.h>, augmented here with the RVA-tagged names
// this TU's other callers use - HasKeyEqual_155550 / RemoveKeysEqual_155360 /
// AnyValueMatches_155630 - since Play already carries a local view of it).
// The image worker registry (holder m_10), the anim registry leaf (holder m_2c)
// and the worker list are the CANONICAL DDrawMgr classes - the former .cpp-local
// CDDrawWorkerRegistry / CDDrawSubMgrAni / CDDrawSubMgrLeaf / CDDrawWorkerList
// duplicate views are dissolved onto them.
#include <DDrawMgr/DDrawWorkerRegistry.h> // CDDrawWorkerRegistry (InstallTree slot 18, +0x48)
#include <DDrawMgr/DDrawSubMgrLeaf.h>     // CDDrawSubMgrLeaf / CDDrawSubMgrAni (0x152xxx leaf API)
#include <DDrawMgr/DDrawWorkerList.h> // CDDrawWorkerList (renderer B: PruneWorkers/ClearWorkers)
#include <DDrawMgr/DDrawChildGroup.h> // CDDrawChildGroup (renderer A: TickKillCues/DestroyChildren_159ef0)
#include <DDrawMgr/DDrawSurfacePair.h> // the CDrawTarget pages (real class of m_10/m_14/m_18)
#include <DDrawMgr/DirectDrawMgr.h>    // CDirectDrawMgr::GetErrorString (cursor-draw fail path)
#include <Bute/SymTab.h>
#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/FontConfig.h>
#include <Io/SaveGame.h>
#include <Gruntz/GameLevel.h>
#include <Image/ImageSet.h> // the REAL CImageSet (m_grid's SetAllTypes/SetAllFormats frame walkers)
#include <Wwd/WwdFile.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GruntzCmdMgr.h> // CGruntzCmdMgr::Spawn (HandleMousePress, waveP)
#include <Gruntz/LeafCue.h>      // LeafCue::PlayIfElapsed_01f940 (HandleMousePress tab cue)
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/ActionOptionsMenuBar.h> // canonical overlay (CTriggerMgr::m_overlay->m_active)
#include <Gruntz/WwdObjMgr.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Timer.h>
#include <Gruntz/LightFxRender.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/WorldSoundSet.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/SpriteRefTable.h>   // CSpriteRefTable (m_74/m_spriteFactory @+0x74; LoadSprite)
#include <Gruntz/GruntzPlayer.h>     // the per-player slot record (its TU folded here, wave3-J)
#include <Wwd/WwdGameObjectFamily.h> // CWwdGameObjectE (the wide-object family base)
#include <Gruntz/Grunt.h>            // CGrunt (Load @0xd8060 folds here per the 0xd5960 dossier)
#include <Gruntz/SerialArchive.h>    // the shared archive stream (GruntzPlayer::Serialize)
#include <rva.h>
#include <Gruntz/ResMgr.h>      // CResMgr + its image/sound/anim registries (m_10/m_28/m_2c)
#include <Gruntz/SoundCue.h>    // CSndHost (m_c->m_28) + SoundStream (m_2c; Vslot15 quiesce stop)
#include <DDrawMgr/DDSurface.h> // the real CDDSurface (render-flip surface: Fill/Restore)
#include <Gruntz/TileTriggerContainer.h> // CTileTriggerContainer (m_beginMarker: Serialize/FilterList2)
#include <Gruntz/LevelSync.h>            // CLevelSync (the +0x2dc guts child-sync @0x1084d0)
#include <Gruntz/UserLogic.h> // CGameObject/AnimWorkerObj (entity views dissolved onto them)

// The zoned sound-bank manager (CWorld::m_48); RegionEnter/RegionLeave pause +
// resume the currently-playing zoned sound via its real (named) methods.
#include <Dsndmgr/GruntzSoundZ.h>
// The real owners of the ex-`Eng` conflation (see the note below): each was reached by
// casting a pointer to a fabricated `Eng` and calling a fabricated method on it.
#include <Gruntz/Multi.h>       // CMulti::AckJoinFailure  (was Eng::Teardown)
#include <Gruntz/CBrickz.h>     // CBrickz::LoadAttributes (was Eng::LoadAttributes)
#include <Gruntz/Brickz.h>      // CBrickzGrid::UpdateDiagonals
#include <Gruntz/ParseSource.h> // CParseSource::BeginParse/EndParse
#include <DDrawMgr/DDrawWorkerHost.h> // CDDrawWorkerHost::GetSize_1633e0 (the plane/grid-owner; ex a CImageSet3 mis-attribution)
#include <DinMgr2/DirectInputMgr2.h> // DirectInputMgr2::ReadAll (was Eng::HideMenu)
#include <Globals.h>

// Placement new: the team-slot "reset" at 0x40a7 is retail RE-CONSTRUCTING a GruntzPlayer
// in place (`mov ecx,<slot>; push 0; call ??0GruntzPlayer@@QAE@H@Z`). No allocation, so it
// is matching-neutral - it just runs the real ctor on the existing storage.
inline void* operator new(u32, void* p) {
    return p;
}

// (CPickedObj is GONE - the picked object IS a CGrunt: thunk 0x1f4b resolves to
// ?OnStruck@CGrunt@@QAEXH@Z @0x588f0, and the +0x1ec/+0x1fc fields are CGrunt's
// m_tileOwnerHi / m_entranceCommitted. CTmCell is already `typedef CGrunt` in
// TriggerMgr.h, so the picks are used cast-free.)
// (CCueSink6 is GONE - thunk 0x39f4 resolves to CGruntSpawnConfig::SpawnVoiceDriver
// @0x11b3b0, the same method LoadCursorSprites already calls on the +0x60 cue sink.)
// (CMarkerPlacer is GONE - thunk 0x2095 resolves to CGruntzCmdMgr::EnqueueSingle
// @0x23c30; CWorld::m_6c is now typed CGruntzCmdMgr*.)

// ---- MFC primitives reused verbatim from the engine (reloc-masked). ----
#include <Gruntz/String.h>
extern i32 MapLookup(void* map, void* key, void*& out); // CMapPtrToPtr::Lookup

// ---- The global CButeMgr text-config tree (the singleton). Modeled as
//      a minimal class so PlayCueAt's `mov ecx,<singleton>; call GetInt`
//      reloc-masks against the already-matched GetInt (butemgr unit). --
#include <Bute/ButeMgr.h>

// The *0x24556c game-manager singleton. Declared HERE (file scope) rather than in
// <Gruntz/Play.h>: a header-level decl pinned one view's type on every includer and
// blocked the CGameRegistry == CGruntzMgr fold (an MFC includer could not retype the
// pointer to the real class without an extern "C" type clash). Type unchanged for this
// TU -- the DATA(0x0024556c) binding below (which re-declares this same extern-"C"
// entity) is unaffected.
extern "C" CGameRegistry* g_gameReg;
DATA(0x002453d8)
extern CButeMgr g_buteMgr;

// The shared engine text renderer (src/Wap32/EngStr.cpp, __cdecl). Forward-declared
// with the exact struct/signature so the call reloc-masks against EngStr.cpp's symbol.
struct EngStrRenderObj;
void EngStr_DrawText(
    EngStrRenderObj* obj,
    i32 a1,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8
);
#define g_buteText (&g_buteMgr)

// ---- Render-carcass leaf callees still unresolved (free fns / reloc-masked). ----
// (Eng_SurfaceFlush/Eng_BeginScene are GONE - CDDSurface::Flip @0x13e850 / Fill
// @0x13e760. Eng_PlaySound/FindSound/StopSound are GONE - CGruntzSoundZ::PlayByName/
// FindBank + CGruntzSoundInnerZ::SetLoop. Eng_InputProbe/Eng_InputDispatch are GONE -
// CDDSurface::BltFast @0x13ef90 + CDirectDrawMgr::GetErrorString @0x141400.
// Eng_CueRenderTop/Def are GONE - ShowHudMessageAlt @0x115520 / EngStr_DrawText
// @0x115440. Eng_HudStrip is GONE - LayerBlitFrame @0x115300.)
extern "C" {
    // (Eng_Profiler1/2 are GONE - the per-frame tick is m_c->m_soundStream, the REAL
    // SoundStream: PurgeVoiceList @0x136e20 + TickSubManagers @0x137ac0, __thiscall.)
    void Eng_HudDraw(void* hud, RECT* r, i32 c);
    void Eng_FrameTimerStep(void* t, i32 now); // carcass-only; identity unrecovered
}
// The per-strip loading-bar blit (0x115300, __cdecl 6 args; GlyphStringDraw.cpp):
// LayerBlitFrame(resMgr, frameImage, x, w, one, zero).
class CImage;
i32 LayerBlitFrame(CResMgr* mgr, CImage* img, i32 x, i32 w, i32 one, i32 zero); // 0x115300
// The manager auto-scroll step (0xebd70, MgrAutoScroll.cpp; the former local
// `ProfReport`/`EmRegWorldStep` duplicate decls of the same fn are gone).
void UpdateMgrScroll(CGruntzMgr* pm, i32* pMode, i32 snapFlag); // 0x0ebd70

// OnRegion3's scroll-region re-arm cue (CmdScrollApply.cpp @0x0ec1c0, cdecl 5-arg).
void Cmd_ApplyScrollParams_0ec1c0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4);
// (CCueState is GONE - PlayCueAt's "+0x410 de-dupe probe" is literally
// m_cueText.LoadString(cueId): retail `lea ecx,[edi+0x410]; call 0x1bedde` where
// 0x1bedde is MFC CString::LoadString - the cue TEXT is loaded from the string
// table and a missing resource skips the cue.)

// Per-frame timer intervals (the game clock g_frameTime is in ms).
typedef enum {
    CUE_INTERVAL_MS = 0x1f4,           // 500 ms  ambient/win-lose cue toggle
    BOOTY_INTERVAL_MS = 0x2710,        // 10000 ms booty-region one-shot
    REGION_INTERVAL_MS = 0x7530,       // 30000 ms scroll-region re-arm
    FIXED_SUBSTEP_MS = 0x12,           // 18 ms   world fixed-substep quantum
    AMBIENT_INTRO_INTERVAL_MS = 0x1f40 // 8000 ms INTRO-cue window (ResetPlayState)
} PlayIntervalMs;

// m_viewMode (StepC / OnRegion2 discriminator).
typedef enum {
    VIEW_MODE_IDLE = 0, // no view yet -> bail
    VIEW_MODE_A = 1,    // mode-A sub-step
    VIEW_MODE_B = 2     // mode-B sub-step
} PlayViewMode;

// The grunt-type ids (the BuildGruntTypeNameTable jump-table index; each name is
// the bute namespace key that case binds - evidence, not invention). 0x39/0x3a
// are the two ids BuildWarlordNameTable/LoadWarlordSprites probe as the "boss"
// pair: HAREKRISHNAGRUNT / REAPERGRUNT.
typedef enum {
    GRUNT_TYPE_NORMAL = 0, // default: NORMALGRUNT
    GRUNT_TYPE_BOMB = 1,
    GRUNT_TYPE_BOOMERANG = 2,
    GRUNT_TYPE_BRICK = 3,
    GRUNT_TYPE_CLUB = 4,
    GRUNT_TYPE_GAUNTLETZ = 5,
    GRUNT_TYPE_GLOVEZ = 6,
    GRUNT_TYPE_GOOBER = 7,
    GRUNT_TYPE_GRAVITYBOOTZ = 8,
    GRUNT_TYPE_GUNHAT = 9,
    GRUNT_TYPE_NERFGUN = 10,
    GRUNT_TYPE_ROCK = 11,
    GRUNT_TYPE_SHIELD = 12,
    GRUNT_TYPE_SHOVEL = 13,
    GRUNT_TYPE_SPRING = 14,
    GRUNT_TYPE_SPY = 15,
    GRUNT_TYPE_SWORD = 16,
    GRUNT_TYPE_TIMEBOMB = 17,
    GRUNT_TYPE_TOOB = 18, // + TOOBWATERGRUNT on success
    GRUNT_TYPE_WAND = 19,
    GRUNT_TYPE_WARPSTONE = 20,
    GRUNT_TYPE_WELDER = 21,
    GRUNT_TYPE_WINGZ = 22,
    GRUNT_TYPE_BABYWALKER = 23,
    GRUNT_TYPE_BEACHBALL = 24,
    GRUNT_TYPE_BIGWHEEL = 25,
    GRUNT_TYPE_GOKART = 26,
    GRUNT_TYPE_JACKINTHEBOX = 27,
    GRUNT_TYPE_JUMPROPE = 28,
    GRUNT_TYPE_POGOSTICK = 29,
    GRUNT_TYPE_SCROLL = 30,
    GRUNT_TYPE_SQUEAKTOY = 31,
    GRUNT_TYPE_YOYO = 32,
    GRUNT_TYPE_HAREKRISHNA = 0x39, // boss pair (BuildWarlordNameTable probes)
    GRUNT_TYPE_REAPER = 0x3a
} GruntTypeId;

// The tool/cursor ids LoadCursorSprites dispatches on (each name is the
// GAME_CURSORZ_* set that case loads). 1..0x26 is the numeric-chip range;
// 0x66 the flailing-grunt cursor; 0xc8..0xe8 the per-tool cursor table.
typedef enum {
    CURSOR_POINTER = 0,
    CURSOR_CHIP_FIRST = 1,
    CURSOR_CHIP_LAST = 0x26,
    CURSOR_FLAILINGGRUNT = 0x66,
    CURSOR_TOOL_HANDZ = 0xc8,
    CURSOR_TOOL_BOMBZ = 0xc9,
    CURSOR_TOOL_BOOMERANGZ = 0xca,
    CURSOR_TOOL_BRICKZ = 0xcb,
    CURSOR_TOOL_CLUBZ = 0xcc,
    CURSOR_TOOL_GAUNTLETZ = 0xcd,
    CURSOR_TOOL_GLOVEZ = 0xce,
    CURSOR_TOOL_GOOBERZ = 0xcf,
    CURSOR_TOOL_GRAVITYBOOTZ = 0xd0,
    CURSOR_TOOL_GUNHATZ = 0xd1,
    CURSOR_TOOL_NERFGUNZ = 0xd2,
    CURSOR_TOOL_ROCKZ = 0xd3,
    CURSOR_TOOL_SHIELDZ = 0xd4,
    CURSOR_TOOL_SHOVELZ = 0xd5,
    CURSOR_TOOL_SPRINGZ = 0xd6,
    CURSOR_TOOL_SPYZ = 0xd7,
    CURSOR_TOOL_SWORDZ = 0xd8,
    CURSOR_TOOL_TIMEBOMBZ = 0xd9,
    CURSOR_TOOL_TOOBZ = 0xda,
    CURSOR_TOOL_WANDZ = 0xdb,
    CURSOR_TOOL_WARPSTONEZ = 0xdc,
    CURSOR_TOOL_WELDERZ = 0xdd,
    CURSOR_TOOL_WINGZ = 0xde,
    CURSOR_TOOL_BABYWALKERZ = 0xdf,
    CURSOR_TOOL_BEACHBALLZ = 0xe0,
    CURSOR_TOOL_BIGWHEELZ = 0xe1,
    CURSOR_TOOL_GOKARTZ = 0xe2,
    CURSOR_TOOL_JACKINTHEBOXZ = 0xe3,
    CURSOR_TOOL_JUMPROPEZ = 0xe4,
    CURSOR_TOOL_POGOSTICKZ = 0xe5,
    CURSOR_TOOL_SCROLLZ = 0xe6,
    CURSOR_TOOL_SQUEAKTOYZ = 0xe7,
    CURSOR_TOOL_YOYOZ = 0xe8
} ToolCursorId;

// CPlay::ApplyGameOptions (0x036be0) lives in its home TU per the interval
// dossier (#10c seam): src/Gruntz/VideoConfig.cpp (the options-dialogs TU).
// Its former CGameMgrSettings local view dissolved with it (the canonical
// CGruntzMgr shape serves there).

// CPlay::Update (0x0008c910) is now an inline member in the header.

// The shared HUD message-sprite helper - the CANONICAL decl (GlyphStringDraw.cpp
// defines it at 0x1154b0 as ShowHudMessage(HudMsgSink*, i32 x8); the former local
// (void*, CString*, RECT*, ...) re-declaration was a PHANTOM: it mangled to a
// symbol nothing defines). The sink is the CState::m_c holder viewed as the HUD
// message sink (HudMsgSink - GlyphStringDraw's facet of the same holder).
struct HudMsgSink;
void ShowHudMessage(
    HudMsgSink* sink,
    i32 text,
    i32 rect,
    i32 dur,
    i32 a,
    i32 b,
    i32 c,
    i32 d,
    i32 e
); // 0x1154b0
// Its +0x14-slot twin (PlayCueAt's a3!=0 cue renderer).
void ShowHudMessageAlt(
    HudMsgSink* sink,
    i32 text,
    i32 rect,
    i32 dur,
    i32 a,
    i32 b,
    i32 c,
    i32 d,
    i32 e
); // 0x115520

// ===========================================================================
// CPlay::FrameSlot28  (vtable slot 10 / +0x28) - the HUD status/pause overlay.
// BYTE-IDENTICAL to CMulti::FrameSlot28 (Multi.cpp).  Freeze m_60, stash the
// game clock, (m_40) run the notify, clear the present surface, then draw the
// LoadString(0x81a9) banner + tick the status message.
// ===========================================================================
// @early-stop
// /GX EH-frame wall (92.75%): full+correct logic, all externs typed/named. Residual
// is (1) the SEH scope-table representation (retail push Unwind@005dde38 / state 8 vs
// cl's push $L.. / state 0 - reloc-masked immediates, docs/seh-eh.md) and (2) cl
// allocates the RECT+CString locals in a 0x14-byte frame vs retail's 0x10, a 4-byte
// /GX frame-packing residue that shifts the [esp+N] stack offsets (not statement-order
// or decl-order steerable - tried both). Permuter candidate for the final sweep.
RVA(0x000c8b80, 0x11b)
i32 CPlay::FrameSlot28(i32 arg) {
    m_4w()->m_60->DtorBody(); // 0x20a4 -> CGruntSpawnConfig::DtorBody @0x11c7b0
    m_savedClock = (i32)g_frameTime;
    if (m_40) {
        QuitToMenu();
    }
    if (arg == 9) {
        return 1;
    }
    RECT r;
    m_c->m_drawTarget->m_18->m_surface->Fill(0);
    CString s;
    s.LoadString(0x81a9);
    r.right = m_4w()->m_8c;
    r.bottom = m_4w()->m_90;
    r.left = 0;
    r.top = 0;
    ShowHudMessage((HudMsgSink*)m_c, (i32)&s, (i32)&r, 0x78, 1, 0xff, 0xff, 0, 1);
    RetireScene(0x50, 0x3e8, 0, 1); // 0xfa8f0 CState::RetireScene (inherited, cast-free)
    if (m_4w() && m_4w()->m_68) {
        m_4w()->m_68->ClearGridRange(5); // 0x41b0 -> CTriggerMgr::ClearGridRange @0x6bd40
    }
    return 1;
}

// ===========================================================================
// CPlay::Render  (vtable slot +0x14)
// ===========================================================================
// @early-stop
// DIVERGING CARCASS (0%): the control flow + member offsets are faithful but the
// codegen is NOT byte-exact yet - retail is 897 instrs, this lowers to ~737. A
// dedicated rewrite (final sweep) must apply these traced structural fixes before
// the codegen residue is even reachable:
//   * zero-register wall: retail pins the 0-constant in EBP (xor ebp,ebp; ebp free
//     because it is NOT a frame pointer here); cl picks EDI, so every `push 0`/
//     `cmp x,0` differs. Pervasive; caps the score. Not source-steerable.
//   * the per-frame sound tick is a __thiscall pair on m_c->m_soundStream (+0x20, the
//     REAL SoundStream): PurgeVoiceList/TickSubManagers (0x136e20/0x137ac0), NOT cdecl.
//   * MarkerBegin(now) -> m_beginMarker->Begin(now) (ecx=[esi+0x2e4], call 0x2cc0);
//     GutsStep() -> m_guts->FrameStep() (ecx=[esi+0x2dc], call 0x21b7) - both are
//     sub-object thiscalls, not CPlay-this methods.
//   * FrameTimerBegin(now) -> m_frameMarker->Begin(now) (ecx=[esi+0x3f4], 0x3710);
//     FrameTimerEnd -> m_frameMarker->End(view, 1) (0x27a2), 2 args.
//   * ALL the interval one-shots (cue +0x3f8, booty +0x328, ambient +0x338,
//     win/lose, snapshot +0x4a0) are 64-BIT elapsed tests: retail does
//     `sub ecx,lo; sbb eax,hi; cmp eax,intervalHi; jl/jg; cmp ecx,intervalLo; jb`
//     ((i64)(u32)g_frameTime - *(i64*)&m_timerLo >= *(i64*)&m_intervalLo), not 32-bit.
//   * the in-game tail (after the view check) has 2 CPlay this-calls the carcass
//     omits (0x2e2d(clock), 0x1519(view)) + a 3-arg cdecl(g_gameReg, m_guts,
//     m_region0Gate) before m_c->m_24->PostStep(); surface flush is a __thiscall on
//     m_2c, not cdecl Eng_SurfaceFlush.
// Also a /GX EH-frame + frame-size residue (0x84 vs 0x6c) once the above land.
// Left as carcass (not touched this pass): a partial rewrite ripples the tight play
// unit's regalloc (StepScroll-style) for no % gain until ALL of the above are done.
RVA(0x000c8cf0, 0xc14)
i32 CPlay::Render() {
    // --- frame entry: clear the per-frame flag, then a `this`-virtual begin. ---
    // (the m_drewThisFrame=0 store is scheduled INTO the BeginFrameClear arg setup.)
    m_drewThisFrame = 0;
    HandleDragMove(0, m_cursorX, m_cursorY); // slot 31: this->vtbl[+0x7c](0, cursorX, cursorY)

    if (m_renderDisabled != 0) {
        return 1; // hard early-out
    }

    if (m_inGame != 0) {
        // =================================================================
        // ---- MAIN in-game frame ----
        // =================================================================
        StepInputA();                    // cursor draw (BltFast)
        StepWorldB();                    // world/camera sub-step B
        StepGridWalk((i32)g_frameDelta); // 0x2e2d  frame-grid advance (was fake "PreStep")

        g_killCueClock = g_645580; // mirror the draw clock
        g_engineFrameDelta = g_frameDelta;

        // --- shared world-draw block #1 ---
        m_c->m_childGroup->TickKillCues_159a70(0); // slot 9 [+0x24]
        m_c->m_24->VisitVisible(m_c->m_drawTarget->m_14, (CGameObjChain*)m_c->m_8); // 0x15dc90
        m_c->m_rendererB->PruneWorkers(
            m_c->m_drawTarget->m_14,
            m_c->m_drawTarget->m_18
        ); // vtbl[+0x34]
        m_4w()->m_54->Retune( // 0x1a7d -> CWorldSoundSet::Retune (positional audio)
            m_c->m_24->m_mainPlane->m_originX,
            m_c->m_24->m_mainPlane->m_originY
        );
        if (m_c->m_soundStream != 0) { // frame profiler
            u32 t = timeGetTime();
            m_c->m_soundStream->PurgeVoiceList(t);  // 0x136e20 (thiscall, SoundDevice base)
            m_c->m_soundStream->TickSubManagers(t); // 0x137ac0 (thiscall)
        }
        m_beginMarker->FilterList2((void*)g_frameDelta);     // 0x2cc0  begin-marker
        m_guts->LoadDestructButtonSprite((i32)g_frameDelta); // 0x34bd  guts step

        // --- periodic AMBIENT-cue timer (+0x3f8, 0x1f4 ms; toggles m_cueToggle) ---
        {
            u32 elapsed = g_frameTime - (u32)m_cueTimerLo;
            if (elapsed >= (u32)m_cueInterval) {
                m_cueToggle = (m_cueToggle == 0);
                m_cueInterval = CUE_INTERVAL_MS;
                m_cueIntervalHi = 0;
                m_cueTimerLo = (i32)g_frameTime;
                m_cueTimerHi = 0;
            }
            if (m_cueToggle != 0) {
                PlayCueAt(0x8128, 0x78, 0, 0xff, 0xff, 0, 1, 0); // cue
            }
        }

        if (m_c->m_drawTarget->m_14 == 0) {
            return 1; // no view -> bail
        }

        m_frameMarker->Tick((i32)g_frameDelta);      // 0x3710  CTimer::Tick
        m_frameMarker->Draw(0, (i32)g_frameDelta);   // 0x27a2  CTimer::Draw
        m_c->m_drawTarget->m_10->m_surface->Flip(0); // 0x13e850  CDDSurface::Flip
        UpdateMgrScroll((CGruntzMgr*)g_gameReg, (i32*)m_guts, m_region0Gate); // 0x2356
        winapi_0d0b30_CopyRect((i32)m_c->m_drawTarget->m_14); // 0x1519 (was fake "PostStep")
        return 1;                                             // -> draw tail
    }

    // m_inGame == 0
    if (m_4w()->m_c != 0) {
        goto alt2;
    }
    {
        CGameRegistry* reg = g_gameReg;
        if (reg->m_134 != 2 && m_overlayDrag != 0) {
            goto alt2;
        }
    }

    // =================================================================
    // ---- MENU / PAUSE-OVERLAY frame ----
    // =================================================================
    {
        CWorld* w = m_4w();
        m_frameMarker->Tick((i32)g_frameDelta); // m_frameMarker begin
        Eng_FrameTimerStep(w->m_6c, 0);         // m_4->m_6c step (carcass; unresolved callee)

        if (m_levelId == CURSOR_FLAILINGGRUNT) { // booty/flailing-grunt one-shot
            u32 elapsed = g_frameTime - (u32)m_bootyTimerLo;
            if (elapsed >= (u32)m_bootyInterval) {
                RegCue(g_gameReg->m_cueSink, 0x33e); // reg->m_cueSink cue
                m_bootyInterval = BOOTY_INTERVAL_MS;
                m_bootyIntervalHi = 0;
                m_bootyTimerLo = (i32)g_frameTime;
                m_bootyTimerHi = 0;
            }
        }

        StepInputA();
        StepC();

        // --- AMBIENT level-init one-shot (+0x348) ---
        if (m_ambientInitDone == 0) {
            u32 elapsed = g_frameTime - (u32)m_ambientTimerLo;
            if (elapsed >= (u32)m_ambientInterval) {
                i32 id = GetAmbientId();
                CString name;
                (void)name; // [esp+0x10] CString temp (/GX)
                char buf[0x80];
                wsprintfA(buf, "AMBIENT%d", id); // s_AMBIENT%d
                if (g_gameReg->m_14 != 0) {
                    w->m_48->PlayByName(buf, 1);
                } else {
                    CGruntzSoundInnerZ* out = 0;
                    CGruntzSoundInnerZ* snd = w->m_48->FindBank(buf);
                    if (snd != 0) {
                        out = snd;
                    }
                    if (out != 0) {
                        out->SetLoop(1);
                    }
                }
                m_ambientInitDone = 1;
            }
        }

        if (m_region0Gate != 0) { // extra HUD/overlay layer
            m_c->m_drawTarget->m_10->m_surface->Fill(0);
            GutsStepB(); // m_guts
        }

        if (m_worldReady == 0) { // world-ready init
            if (w->m_68->m_armed != 0) {
                WorldSubstep();
            }
            StepWorldB();
        }

        StepScroll();

        // per-frame timing gate (UNSIGNED clamp: 0x12 < dt < 0xc8):
        {
            u32 dt = g_frameDelta;
            if (dt > 0x12 && dt < 0xc8) {
                DrawWorldFrames(); // call [eax+0xa0] (slot 40; ex "RenderFast")
            } else {
                DrawWorldFrame(); // call [edx+0x9c] (slot 39; ex "RenderSlow")
            }
        }

        // --- shared world-draw block #2 ---
        m_c->m_24->VisitVisible(m_c->m_drawTarget->m_14, (CGameObjChain*)m_c->m_8);
        m_c->m_rendererB->PruneWorkers(m_c->m_drawTarget->m_14, m_c->m_drawTarget->m_18); // present
        if (m_region1Gate != 0) {
            StepC(); // alt-input draw
        } else {
            m_c->m_24->VisitVisible(m_c->m_drawTarget->m_14, (CGameObjChain*)m_c->m_8);
            m_c->m_rendererB->PruneWorkers(m_c->m_drawTarget->m_14, m_c->m_drawTarget->m_18);
        }
        m_beginMarker->FilterList2((void*)g_frameDelta);
        m_guts->LoadDestructButtonSprite((i32)g_frameDelta);
        InputSubStep(w->m_70); // m_4->m_70

        if (m_lightFx != 0 && m_guts->m_activeTab != 5) { // on-screen overlay/banner
            Overlay1(0, (i32)g_frameDelta);
            Overlay2(m_c, 0);
        }

        m_4w()->m_54->Retune( // world sound retune off the plane scroll origin
            m_c->m_24->m_mainPlane->m_originX,
            m_c->m_24->m_mainPlane->m_originY
        );
        if (m_c->m_drawTarget->m_14 == 0) {
            return 1;
        }

        // --- snapshot/screenshot countdown (+0x4a0/+0x4a8) ---
        if (m_snapshotActive != 0) {
            u32 now = g_frameTime;
            u32 dur = (u32)(m_snapDur + m_snapBaseLo) - now;
            if ((i32)dur >= 0) {
                // duration elapsed: post a message + reset the marker block + walk.
                if (m_guts->m_modeArmed != 0) {
                    SnapPostMessage(5); // reg->m_68 (5)
                } else {
                    SnapPostMessage(g_curPlayer);
                }
                // reset the m_frameMarker marker block (+0x30..0x4c):
                m_frameMarker->Draw(0, 0);
                GutsStepC(); // m_guts
                m_snapshotActive = 0;
                // walk the level tree (CMapPtrToPtr::Lookup):
                if (g_gameReg->m_focusSlots[0].m_0c != 0) {
                    void* out = 0;
                    MapLookup(g_gameReg->m_world->m_8, (void*)g_gameReg->m_focusSlots[0].m_0c, out);
                    if (out != 0) {
                        SnapWalk();
                    }
                }
            } else {
                // not yet: build a CString temp, CopyRect the viewport, HudDraw.
                CString tmp;
                (void)tmp; // [esp+0x10] CString temp
                tmp.Format("%s", "");
                // m_30 is the shared CSpriteFactoryHolder; this WIP path reads it
                // as a resource map whose +0x24 holds the CopyRect-source rect.
                CopyRect(&m_hudRect, (const RECT*)((char*)g_gameReg->m_world + 0x24));
                Eng_HudDraw(g_gameReg->m_world, &m_hudRect, 1);
            }
            // (CString temp dtor runs here under the EH frame)
        }

        // --- the four scroll-region one-shots (+0x430/+0x440/+0x450/+0x460) ---
        m_frameMarker->Draw(0, m_frameMarker != 0); // reset
        OnRegion5();
        Eng_FrameTimerStep(w->m_68, 0); // m_4->m_68 (carcass; unresolved callee)

        if (m_winLoseBanner != 0 && m_guts->m_toggleActive == 0 && m_guts->m_toggleHandle == 0) {
            // win/lose banner timer (+0x3f8 again, 0x1f4 ms):
            u32 elapsed = g_frameTime - (u32)m_cueTimerLo;
            if (elapsed >= (u32)m_cueInterval) {
                m_cueToggle = (m_cueToggle == 0);
                m_cueInterval = CUE_INTERVAL_MS;
                m_cueIntervalHi = 0;
                m_cueTimerLo = (i32)g_frameTime;
                m_cueTimerHi = 0;
            }
            if (m_cueToggle != 0) {
                PlayCueAt(0x8129, 0x78, 0, 0xff, 0xff, 0, 1, 0);
            }
        }

        m_beginMarker->FilterList2((void*)g_frameDelta);
        PostHud(0);
        if (m_worldReady != 0) { // optional HUD overlay draw
            Eng_HudDraw(
                m_c->m_drawTarget->m_10->m_surface,
                &m_hudRect,
                0xff
            ); // (this=m_4->m_10->m_2c)
        }
        m_c->m_drawTarget->m_10->m_surface->Flip(0);

        // --- the four screen-region scroll one-shots: each is
        // a 64-bit "inside region" elapsed test that fires its OnRegion handler. ---
        if (m_region0Gate != 0) { // region-1 (+0x430)
            u32 e = g_frameTime - (u32)m_region0TimerLo;
            if (e >= (u32)m_region0Interval) {
                OnRegion2((i32)g_frameTime);
            }
        }
        if (m_region1Gate != 0) { // region-2 (+0x440)
            u32 e = g_frameTime - (u32)m_region1TimerLo;
            if (e >= (u32)m_region1Interval) {
                OnRegion1((i32)g_frameTime);
            }
        }
        if (m_region2Gate != 0) { // region-3 (+0x450)
            u32 e = g_frameTime - (u32)m_region2TimerLo;
            if (e >= (u32)m_region2Interval) {
                OnRegion3((i32)g_frameTime);
            }
        }
        if (m_region3Gate != 0) { // region-4 (+0x460)
            u32 e = g_frameTime - (u32)m_region3TimerLo;
            if (e >= (u32)m_region3Interval) {
                OnRegion4((i32)g_frameTime);
            }
        }
        return 1; // -> draw tail
    }

alt2:
    // =================================================================
    // ---- the m_4->m_c != 0 short path ----
    // =================================================================
    StepInputA();
    if (m_c->m_drawTarget->m_14 == 0) {
        return 1;
    }
    {
        if (m_c->m_soundStream != 0) { // cursor/frame profiler
            u32 t = timeGetTime();
            m_c->m_soundStream->PurgeVoiceList(t);  // 0x136e20 (thiscall, SoundDevice base)
            m_c->m_soundStream->TickSubManagers(t); // 0x137ac0 (thiscall)
        }
        if (m_paused != 0) {
            // ---- the paused frame: draw-only ----
            m_c->m_24->VisitVisible(m_c->m_drawTarget->m_14, (CGameObjChain*)m_c->m_8);
            m_c->m_rendererB->PruneWorkers(
                m_c->m_drawTarget->m_14,
                m_c->m_drawTarget->m_18
            ); // present
            m_guts->LoadDestructButtonSprite((i32)g_frameDelta);
            if (m_guts->m_toggleActive == 0 && m_guts->m_toggleHandle == 0) {
                PlayCueAt(0x812c, 0x78, 0, 0xff, 0xff, 0, 1, 0); // win/lose
            }
            m_frameMarker->Draw(1, 0);
        } else {
            // ---- the active short frame: entity step + cues ----
            if (m_stepCountdown > 0) {
                m_stepCountdown = m_stepCountdown - 1;
                m_c->m_24->VisitVisible(m_c->m_drawTarget->m_14, (CGameObjChain*)m_c->m_8);
                m_c->m_rendererB->PruneWorkers(
                    m_c->m_drawTarget->m_14,
                    m_c->m_drawTarget->m_18
                ); // present
                m_guts->LoadDestructButtonSprite((i32)g_frameDelta);
                Eng_FrameTimerStep(m_guts, 0x32); // (carcass; unresolved callee)
                PlayCueAt(m_lastCueId, 0x78, 0, 0xff, 0xff, 0, 1, 0); // (cueId=m_lastCueId)
                m_frameMarker->Draw(1, 0);
            }
            if (m_ambientInitDone == 0) {
                // the same AMBIENT level-init one-shot (+0x348):
                u32 elapsed = g_frameTime - (u32)m_ambientTimerLo;
                if (elapsed >= (u32)m_ambientInterval) {
                    i32 id = GetAmbientId();
                    CString name;
                    (void)name;
                    char buf[0x80];
                    wsprintfA(buf, "AMBIENT%d", id);
                    if (g_gameReg->m_14 != 0) {
                        m_4w()->m_48->PlayByName(buf, 1);
                    } else {
                        CGruntzSoundInnerZ* out = 0;
                        CGruntzSoundInnerZ* snd = m_4w()->m_48->FindBank(buf);
                        if (snd != 0) {
                            out = snd;
                        }
                        if (out != 0) {
                            out->SetLoop(1);
                        }
                    }
                    m_ambientInitDone = 1;
                }
            }
        }
        m_beginMarker->FilterList2((void*)g_frameDelta);
        PostHud(0);
        m_c->m_drawTarget->m_10->m_surface->Flip(0);
    }
    return 1; // draw tail
}

// ===========================================================================
// CPlay::OnExit (0x0cb400) - the PLAY-state teardown: run the ready-gate forwarder,
// the slot-21 notify, refresh renderer A, then clear the registry's per-frame word
// (+0x128), drop its mode back to 0 if it was 3, and run the +0x70 tile grid's
// slot-0 Reset (CMapMgr's real virtual @0x9ec30). (The former CRegExit view of the
// singleton is dissolved onto the canonical CGameRegistry.)
// ===========================================================================
// ===========================================================================
// CPlayLevelLoad::LoadByMode (0x0ca200; re-homed from the former loadlevelbymode unit,
// waveP - TU_MIGRATION MOVE row `0x0ca200 LoadByMode@CPlayLevelLoad loadlevelbymode ->
// 0xc8700 play`). The PLAY game-state per-mode level loader (/GX megafunction).
// `this` IS the canonical CPlay; CPlayLevelLoad here is a thin CPlay-derived facet
// (LoadByMode not yet on Play.h - a DEFERRED cross-TU fold onto CPlay, reported). g_curPlayer
// / g_emptyString reuse Play.h's decls.
// ===========================================================================
// ---------------------------------------------------------------------------
// Shared singletons + the per-mode/per-area globals (named so DIR32 reloc-mask).
// ---------------------------------------------------------------------------
// The DRAW-CLOCK MIRROR PAIR - DEFINED HERE (owner = the producer). This TU is the only
// writer: every frame it does `g_killCueClock = g_645580; g_engineFrameDelta = g_frameDelta;`, i.e. it
// snapshots the WAP32 frame clock (now / delta) into the pair the whole game reads to
// gate animations (`clk - m_last >= m_interval`). ~15 TUs consumed them and NONE defined
// them, under FIVE names between them: _g_killCueClock and _g_6bf3c0 for 0x2bf3c0 (plus
// GruntzMgrCmd's C++ ?g_time6bf3c0@@3HA), and _g_6bf3bc plus a C++ ?g_defaultGeo@@3HA
// (Wormhole/Grunt) for 0x2bf3bc - "default geometry" being simply a WRONG name for the
// frame delta. One name + one linkage per cell now.
//
// The surviving name g_killCueClock is narrower than the role the binary shows (it is the
// draw-clock now-mirror generally, not only a kill-cue clock); it is kept because it is
// the majority semantic spelling, and a role rename is separate follow-up.
DATA(0x002bf3bc)
extern "C" {
    u32 g_engineFrameDelta = 0; // 0x2bf3bc  draw-DELTA mirror  (= g_frameDelta / g_lastDelta)
}
DATA(0x002bf3c0)
extern "C" {
    u32 g_killCueClock = 0; // 0x2bf3c0  draw-CLOCK mirror (= g_645580 / g_lastNow)
}

DATA(0x00245270)
extern "C" i32 g_areaPageSize; // 0x645270 (area page size)
// extern "C" to hit the definition's C-linkage name _g_645570 (GruntzMgr.cpp, typed
// DirectInputMgr2*); a plain C++ extern emitted ?g_645570@@3PAXA - unresolved.
extern "C" void* g_645570;          // DAT_00645570
extern "C" i32 g_64558c;            // DAT_0064558c
extern "C" i32 g_64e35c;            // DAT_0064e35c
extern i32 g_resourceInstallActive; // ?g_resourceInstallActive@@3HA @0x6bf37c
// (0x612618 - the last-loaded level number, init -1 - is DEFINED below as
// g_lastLevelNum; the old `void* g_lastLevelCache` view here was the same cell.)
// 0x61139c: the CAreaMgr singleton pointer, statically initialized to &g_areaMgr
// (retail .data holds VA 0x6459b0 = the g_areaMgr object AreaMgr.cpp defines).
// Owner-TU definition here (play is its only referencing unit).
DATA(0x0021139c)
CAreaMgr* g_pAreaMgr = &g_areaMgr;

// ---------------------------------------------------------------------------
// RESIDUAL carcass (9 classes already folded onto their real canonical headers:
// CGruntzMgr/CGruntSpawnConfig/CButeMgr/CAreaMgr/CSymTab/CSymParser/CSaveGame/
// CFontConfig/CWorldSoundSet). The methods below are the DEFERRED remainder,
// grouped by why they aren't folded yet (all reloc-masked, so byte-neutral):
//   (a) cross-module canonical header + retype (SoundStream Dsndmgr[Teardown/grid];
//       CGruntzSoundZ Dsndmgr[Reset0]; CGruntzCmdMgr Net[ClearList]; DirectInputMgr2
//       StateMgrBZ[HideMenu]; CDDSurface ResMgr[ShadeRect]; CStatusBarMgr SBI_Image
//       [ClearTiles/PostMap]);
//   (b) needs a byte-neutral declared-only method added to its Gruntz header first
//       (CParseSource CImage.h[BeginParse/EndParse]; CBrickz/CBrickzGrid[LoadAttributes/
//       UpdateDiagonals]; CLightFxRender[InstallLightFx/BuildLightShape]; CTimer
//       [TimerEqSet/TimerReset]; CBattlezData[BattlezInit=Init]);
//   (c) Teardown-on-savedThis is CMulti::AckJoinFailure (a cross-cast of the CPlay
//       `this`); WorkerReset is the GruntzPlayer(int) ctor (placement-new);
//   (d) @identity-TODO - genuinely unrecovered (no xref/header): ObListInit 0x1b48a6,
//       ButeStore 0x1b5485, GetBool 0x1bedde, CDDrawWorkerHost::GetSize 0x1633e0.
// ---------------------------------------------------------------------------
// (`struct Eng` is GONE - it was a CONFLATION, not a class: 21 fabricated methods
// standing in for 21 functions on ~15 DIFFERENT real classes, every one of which was
// already reconstructed and RVA-bound elsewhere in the tree. Every call went through
// `E(p)` - a cast of some unrelated pointer to the fake type - so all 21 symbols were
// PHANTOM (?Xxx@Eng@@...): referenced by `play`, owned by a class with no retail
// address, and therefore unlinkable. Worse, ONE fake name covered TWO different real
// functions (Eng::Teardown was used both for SoundStream::Stop @0x137a80 and for
// CMulti::AckJoinFailure @0xbc420), so even a hypothetical binding could only ever have
// been right for one of them. Each call site now names its real class + real method, so
// the rel32 binds to the symbol retail actually calls. Resolved by chasing each ILT
// thunk to its body and reading the name off symbol_names (gruntz.analysis.vtable_owner
// / sema disasm). The two still-unresolved ones are marked below.)

// ---------------------------------------------------------------------------
// Genuine __cdecl engine helpers (reloc-masked rel32).
// ---------------------------------------------------------------------------
void Cmd_ResetScroll();            // 0x2bd0  YAXXZ
i32 QueryToken(i32 a);             // 0x39a4  QueryToken(int)
i32 ValidateMainBlock(void* cstr); // 0x2c8e  static WwdFile (CString byval)
void ActiveWait(i32 ms);           // 0x13dfe0 busy-wait
void* RezAlloc(i32 sz);            // 0x1b9b46 operator new
void RezFree(void* p);             // 0x1b9b82 operator delete

#define I32(p, off) (*(i32*)((char*)(p) + (off)))
#define PTR(p, off) (*(void**)((char*)(p) + (off)))

// ---------------------------------------------------------------------------
// The PLAY-state level loader (`this`). Its own init-chain steps are __thiscall
// methods; raw-offset field access via the macros.
// ---------------------------------------------------------------------------
// `this` IS the canonical CPlay (proven by xref - the init-chain leaves are all
// CPlay methods; the +0x10/+0x84/+0xa4/+0xa8 dispatches map to CPlay's real vtable
// slots Update/Vslot21/BuildMusicCategoryTable/BuildWorldLevelPath). LoadByMode is
// modeled on this thin CPlay-derived facet (its two big methods aren't yet in the
// canonical Play.h owned by another unit); the init-chain steps are declared here
// as reloc-masked leaves (real CPlay/CLevelValidator targets in comments).
// (The `CPlayLevelLoad : CPlay` facet class is GONE - `this` IS the canonical
// CPlay, and every one of its ~20 fabricated leaf decls resolved (thunk-decode,
// 2026-07-13) to a CPlay/CState/CGruntzMgr method DEFINED in this tree:
//   BuildHelpReveal(0x1019)==CPlay::BuildHelpReveal (the loading-bar tick),
//   RegisterInputBindings(0x3a71)==CPlay's own, SoundStep(0x1843)==CState::
//   RetireScene, InfoTextStep(0x14b5)==CPlay::DrawLevelInfoText, ZeroBlock
//   (0x32d3)==CPlay::LoadLoadingBarSprite, FadeInTitle(0x1e60)==CState::
//   FadeInTitle, StepB..StepK==the Load*/Build* loader family, FinalizeWorld
//   (0x1db6)==CGruntzMgr::RecomputeViewScale, ScanWarpStone(0x28dd)==
//   ScanShuffleQuads, AfterTitle(0x2e14)==CGruntzMgr::SyncOptionsState, SetTitle
//   (0x175d)==CSaveGame::FillSlot2, LoadMap(0x2b80)==LoadWarlordSprites,
//   LoadStep1/2/3==ScanBuildTiles/ValidateLevelTiles/AddLevelGruntz, StartGame
//   (0x3d55)==CPlay::ResetViewport, AckJoinFailure(0x35e4)==CMulti's.)
// ===========================================================================
RVA(0x000ca200, 0xe34)
i32 CPlay::LoadByMode(i32 level, i32) {
    void* self = this;
    void* gameReg;
    void* set;
    i32 reload = 0; // [esp+0x20] warp-cache reload flag (-> the vtable +0xa4 arg)
    // one contiguous stack buffer: the AREA%i/Level%i name at +0x00, a 148-byte
    // zeroed sub-block at +0x20 (retail's [esp+0x58] rep-stos region / LoadMap arg).
    char nameBuf[0xb4]; // [esp+0x38]

    // ---- 1) reset prior-level scroll/area globals ----
    I32(self, 0x484) = 1;
    g_frameDelta = 0;
    g_645580 = 0;
    g_frameTime = 0;
    g_6455f0 = 0;
    if (level > 0x64) {
        level -= 0x64;
        g_6455f0 = 1;
    }

    // clear the m_3f4 worker's slots
    void* worker = PTR(self, 0x3f4);
    if (worker != 0) {
        I32(worker, 0x40) = 0;
        I32(worker, 0x44) = 0;
        I32(worker, 0x30) = 0;
        I32(worker, 0x34) = 0;
        I32(worker, 0x48) = 0;
        I32(worker, 0x4c) = 0;
    }

    // tear down the old grid (self->m_c->m_28->m_2c) / map / sound sub-objects
    void* grid = PTR(PTR(PTR(self, 0xc), 0x28), 0x2c);
    if (grid != 0) {
        ((SoundStream*)grid)->Stop();
    }
    ((CGruntzSoundZ*)PTR(PTR(self, 0x4), 0x48))->StopAndFlush();
    ((CWorldSoundSet*)PTR(PTR(self, 0x4), 0x54))->Teardown();
    ((CGruntSpawnConfig*)PTR(PTR(self, 0x4), 0x60))->DtorBody();
    ((CGruntSpawnConfig*)PTR(PTR(self, 0x4), 0x60))->ClearSprites();
    ((CGruntzMgr*)PTR(self, 0x4))->RestoreVideoMode(0);

    gameReg = g_gameReg;
    if (I32(gameReg, 0x134) != 2) {
        g_curPlayer = 0;
        if (I32(gameReg, 0xc) != 0) {
            i32 v = I32(gameReg, 0xc) ^ 1;
            I32(gameReg, 0xc) = v;
            ((CGruntzMgr*)g_gameReg)->FinishLevel(v, 1);
        }
    }

    // clear the 4 score/team slots at +0x384 (-4 .. +28, two dwords each)
    {
        i32* p = (i32*)((char*)self + 0x388);
        i32 n = 4;
        do {
            p[-1] = -1;
            p[0] = -1;
            p += 2;
        } while (--n);
    }

    g_killCueClock = g_645580;
    g_engineFrameDelta = g_frameDelta;

    // reset the 4 team blocks at host->m_150 (stride 0x238): single-mode primes
    // team 0 ready, multi-mode zeroes the round counters.
    for (i32 t = 0; t < 4; ++t) {
        void* hostBase = PTR(self, 0x4);
        gameReg = g_gameReg;
        void* team = (char*)hostBase + t * 0x48 * 8 - t * 8 + 0x150; // [edx+ecx*8+0x150]
        if (I32(gameReg, 0x134) == 1) {
            new ((void*)team) GruntzPlayer(0);
            if (t == 0) {
                I32(team, 0x20) = 1;
                I32(team, 0x28) = 1;
            }
        } else {
            I32(team, 0x2c) = 0;
            I32(team, 0x28) = I32(team, 0x20);
            I32(team, 0x24) = 0;
        }
    }

    // ---- 2) mode/level-number resolve ----
    i32 modeFlag = ((i32)Update() == 0x11) ? 1 : 0;
    void* savedThis = modeFlag ? self : 0; // [esp+0x10] = (-modeFlag) & self
    I32(self, 0x1c4) = 1;
    I32(self, 0x1c) = level;
    {
        i32 r = (level - 1) % 0x24;
        I32(self, 0x20) = r / 4 + 1; // ((level-1)%0x24)/4 + 1 (signed div-by-4)
    }

    gameReg = g_gameReg;
    g_frameTime = 0;
    if (I32(gameReg, 0x134) == 3) {
        srand(timeGetTime());
    }
    g_resourceInstallActive = 0;
    Cmd_ResetScroll();
    ((CBattlezData*)PTR(g_gameReg, 0x7c))->Init();
    ((CPtrList*)((char*)PTR(g_gameReg, 0x6c) + 0x1c))->RemoveAll();
    ((CGruntzCmdMgr*)PTR(g_gameReg, 0x6c))->DrainBase();
    g_64558c = 0;
    I32(self, 0x1bc) = 0;
    I32(PTR(self, 0x4), 0x130) = 0;

    // already-loaded guard: when host->m_c8[-2] == 0 skip the name-resolve.
    void* host = PTR(self, 0x4);
    if (I32(PTR(host, 0xc8), -8) != 0) {
        if (I32(host, 0x128) != 0) {
            // BATTLEZ: resolve the level number from the level name's digit run.
            set = ((CSymParser*)PTR(host, 0x34))->ResolvePath("GAME_BATTLEZ");
            if (set == 0) {
                goto fail0;
            }
            i32 ins = ((CSymTab*)set)
                          ->Insert(
                              (const char*)((CGruntzMgr*)PTR(self, 0x4))->GetWorldFileName(),
                              g_emptyString
                          );
            if (ins == 0) {
                return 0;
            }
            void* desc = (void*)((CParseSource*)set)->BeginParse();
            if (desc == 0) {
                goto fail0;
            }
            char* p = (char*)desc + 0x10;
            char c = *p;
            while (c != 0) {
                if (c < '0' || c > '9') {
                    ++p;
                    c = *p;
                    continue;
                }
                break;
            }
            i32 num = atoi(p);
            ((CParseSource*)set)->EndParse();
            level = num;
        } else if (I32(host, 0x12c) != 0) {
            // MULTI: same digit resolve off "GAME_MULTI".
            set = ((CSymParser*)PTR(host, 0x34))->ResolvePath("GAME_MULTI");
            if (set == 0) {
                goto fail0;
            }
            i32 ins = ((CSymTab*)set)
                          ->Insert(
                              (const char*)((CGruntzMgr*)PTR(self, 0x4))->GetWorldFileName(),
                              g_emptyString
                          );
            if (ins == 0) {
                return 0;
            }
            void* desc = (void*)((CParseSource*)set)->BeginParse();
            if (desc == 0) {
                goto fail0;
            }
            char* p = (char*)desc + 0x10;
            char c = *p;
            while (c != 0) {
                if (c < '0' || c > '9') {
                    ++p;
                    c = *p;
                    continue;
                }
                break;
            }
            i32 num = atoi(p);
            ((CParseSource*)set)->EndParse();
            level = num;
        } else {
            // default: bute-driven level number (ValidateMainBlock(CString)).
            level = ValidateMainBlock(
                (void*)(const char*)((CGruntzMgr*)PTR(self, 0x4))->GetWorldFileName()
            );
            I32(self, 0x1bc) = 0;
            I32(PTR(self, 0x4), 0x130) = 0;
        }

        // recompute area page from the resolved level number
        i32 r = (level - 1) % 0x24;
        I32(self, 0x1c) = level;
        I32(self, 0x20) = r / 4 + 1;
    }

    // ---- 3) build the level name + look it up ----
    sprintf(nameBuf, "AREA%i", I32(self, 0x20));
    set = ((CSymParser*)PTR(self, 0x8))->ResolvePath(nameBuf);
    I32(self, 0x28) = (i32)set;
    if (set == 0) {
        goto fail0;
    }

    // ---- 4) area-page jump table over (m_20 - 1) in 0..7 ----
    {
        i32 page = I32(self, 0x20) - 1;
        switch ((u32)page) {
            case 0:
                g_areaPageSize = 4;
                break;
            case 1:
                g_areaPageSize = 0;
                g_64553c = 0;
                break;
            case 2:
                g_areaPageSize = 8;
                g_64553c = 0;
                break;
            case 3:
                g_areaPageSize = 8;
                g_64553c = 0xf;
                break;
            case 4:
                g_areaPageSize = 5;
                g_64553c = 9;
                break;
            case 5:
                g_areaPageSize = 4;
                g_64553c = 0xb;
                break;
            case 6:
                g_areaPageSize = 0xe;
                g_64553c = 0xb;
                break;
            case 7:
                g_areaPageSize = 4;
                g_64553c = 0;
                break;
            default:
                g_areaPageSize = 4;
                g_64553c = 0;
                break;
        }
    }

    // m_2c = m_28 (the resolved area descriptor); refresh the host window
    {
        i32 prevTiles = I32(self, 0x2c);
        I32(self, 0x2c) = I32(self, 0x28);
        UpdateWindow((HWND)PTR(PTR(PTR(self, 0x4), 0x4), 0x4));

        host = PTR(self, 0x4);
        if (I32(PTR(host, 0xc8), -8) != 0) {
            if (I32(host, 0x128) == 0 && I32(host, 0x12c) == 0) {
                sprintf(nameBuf, "CUSTOMLEVEL", 0);
            }
        } else if (level > 0x24) {
            sprintf(nameBuf, "TRAINING", 0);
        }
        (void)prevTiles;
    }

    // ---- 5) the long linear init chain ----
    if (!FadeInTitle(nameBuf, 0, 0, 0, 0, 1)) { // [esp+0x48] out, push 0/0/0/0/1
        goto fail0;
    }
    RetireScene(0x50, 0x3e8, 0, 1); // 0x1843 -> CState::RetireScene
    DrawLevelInfoText();            // 0x14b5 -> 0xd95f0
    I32(self, 0x2c) = 0;
    {
        i32* z = (i32*)((char*)nameBuf + 0x20);
        i32 n = 0x25;
        while (n--) {
            *z++ = 0;
        }
    }
    LoadLoadingBarSprite(); // 0x32d3 -> 0xd7440
    BuildHelpReveal(0);
    FreeListTeardown(); // vtable +0x84 (CPlay slot 33; ex the "Vslot21" placeholder)
    if (savedThis != 0) {
        ((CMulti*)savedThis)->AckJoinFailure(); // AckJoinFailure placeholder (0x35e4 on saved obj)
    }
    RegisterInputBindings();

    if (!BuildAnizKeyTable(0) /* 0x3b61 -> 0xddaa0 */) {
        goto fail0;
    }

    // the warp-stone / last-level comparison (g_lastLevelNum / g_pAreaMgr)
    {
        i32 cached = g_lastLevelNum;
        i32 eq = g_pAreaMgr->SameGroup(cached); // 0x2f2c
        reload = (eq == 0) ? 1 : 0;
        i32 diff = (level != g_lastLevelNum) ? 1 : 0;
        if (g_pAreaMgr == 0) {
            return 0;
        }
        g_lastLevelNum = level;

        BuildHelpReveal(0);
        if (savedThis != 0) {
            ((CMulti*)savedThis)->AckJoinFailure();
        }
        RegisterInputBindings();

        BuildHelpReveal(0);
        if (savedThis != 0) {
            ((CMulti*)savedThis)->AckJoinFailure();
        }
        RegisterInputBindings();

        if (!LoadActionTileSprites(diff)) {
            goto fail0;
        }
    }

    // a tail of paired BeginStep(0)/EndStep brackets around the real init steps.
    BuildHelpReveal(0);
    if (savedThis != 0) {
        ((CMulti*)savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (modeFlag != 0 && I32(g_gameReg, 0x134) == 1) {
        BuildWarlordNameTable((i32)savedThis);
    }
    BuildHelpReveal(0);
    RegisterInputBindings();
    if (!LoadLevelImages(1) /* 0x3021 -> 0xdb7e0 */) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        ((CMulti*)savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadGameImages(1) /* 0x3346 -> 0xdb8a0 */) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        ((CMulti*)savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!BuildSpriteImageKeyTable((CMulti*)savedThis) /* 0x23b5 -> 0xdd540 */) {
        goto fail0;
    }
    BuildHelpReveal(0);
    RegisterInputBindings();
    if (!LoadLevelSounds(1) /* 0x1c2b -> 0xdb6c0 */) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        ((CMulti*)savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadGameSounds(1) /* 0x2964 -> 0xdb930 */) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        ((CMulti*)savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadGruntSoundNamespaces(0) /* 0x2e9b -> 0xdd830 */) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        ((CMulti*)savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    SetEffectSpriteDurations(); // 0x4458 -> 0xdc060
    if (savedThis != 0) {
        ((CMulti*)savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadLevelAnims(1) /* 0x2c07 -> 0xdb750 */) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        ((CMulti*)savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadGameAnims(1) /* 0x247d -> 0xdb9b0 */) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        ((CMulti*)savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!BuildWorldLevelPath(1)) { // vtable +0xa8 (CPlay slot 42)
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        ((CMulti*)savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();

    // finalize the world planes
    ((CGruntzMgr*)PTR(self, 0x4))->RecomputeViewScale();
    if (PTR(PTR(PTR(self, 0xc), 0x24), 0x5c) != 0) {
        ((CDDrawWorkerHost*)PTR(PTR(PTR(self, 0xc), 0x24), 0x5c))->GetSize_1633e0();
    }
    if (PTR(PTR(PTR(self, 0xc), 0x24), 0x5c) != 0) {
        ((CDDrawWorkerHost*)PTR(PTR(PTR(self, 0xc), 0x24), 0x5c))->GetSize_1633e0();
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        ((CMulti*)savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();

    // view setup off host->m_70
    {
        void* g5c = PTR(PTR(PTR(self, 0xc), 0x24), 0x5c);
        void* host70 = PTR(PTR(self, 0x4), 0x70);
        if (!((CBrickz*)host70)->LoadAttributes(I32(g5c, 0x28), I32(g5c, 0x2c))) {
            goto fail0;
        }
    }
    if (!((CBrickzGrid*)PTR(PTR(self, 0x4), 0x70))->UpdateDiagonals((i32)PTR(self, 0x4))) {
        goto fail0;
    }

    // lazily allocate the level context at +0x320
    if (I32(self, 0x320) == 0) {
        void* ctx = RezAlloc(0x43c);
        if (ctx != 0) {
            I32(ctx, 0) = 0;
            I32(ctx, 0x4) = 0;
            I32(ctx, 0x8) = 0;
            I32(ctx, 0xc) = 0;
            I32(ctx, 0x10) = 0;
            I32(ctx, 0x48) = 0;
            I32(ctx, 0x434) = 0;
            I32(ctx, 0x438) = 0;
        } else {
            ctx = 0;
        }
        I32(self, 0x320) = (i32)ctx;
        if (!((CLightFxRender*)ctx)->Init((LfxMgr*)PTR(self, 0x4), 0xfa)) {
            goto fail0;
        }
    }
    if (!((CLightFxRender*)PTR(self, 0x320))->BuildShape(I32(self, 0x20))) {
        goto fail0;
    }

    // ---- the WarpStone bute scan (single-mode only) ----
    gameReg = g_gameReg;
    if (I32(gameReg, 0x134) != 1) {
        CString warp; // [esp+0x14]
        i32 same = 0;
        if (warp.LoadString(0x81ab)) {
            char* a = (char*)(const char*)((CGruntzMgr*)g_gameReg)->GetWorldFileName();
            char* b = (char*)(const char*)warp;
            i32 eq = 1;
            while (*b == *a) {
                if (*b == 0) {
                    break;
                }
                a += 1;
                b += 1;
            }
            if (*b != *a) {
                eq = 0;
            }
            same = eq;
        }
        if (same) {
            ScanShuffleQuads(); // 0x28dd -> 0xd9290
        }
    }

    // ---- area-name -> level-list build ----
    if (I32(PTR(self, 0x4), 0x134) == 3) {
        ((CGruntzMgr*)PTR(self, 0x4))->SyncOptionsState(); // 0x2e14, ecx=m_4
    }
    ((CSaveGame*)PTR(PTR(self, 0x4), 0x58))
        ->FillSlot2((SaveSlot*)((char*)self + 0x1d0), I32(self, 0x1c), 0);
    {
        CString key; // [esp+0x18]
        gameReg = g_gameReg;
        I32(PTR(gameReg, 0x68), 0x2a0) = 0;
        i32 count = I32(self, 0x1c);
        i32 i = count - ((count - 1) % 4); // round-down-to-4 idiom
        for (; i < count; ++i) {
            // key.Format("Level%i", i) -> wsprintf-into-CString (0x1b2cf5)
            key.Format("Level%i", i);
            void* bm = PTR(g_gameReg, 0x68);
            i32 v = g_buteMgr.GetInt((const char*)key, "WarpStone");
            ((CByteArray*)((char*)bm + 0x260))->SetAtGrow((i32)PTR(bm, 0x268), (u8)v);
        }
    }
    ((CStatusBarMgr*)PTR(self, 0x2dc))->LoadMultiplayerBattlezConfig(I32(self, 0x1c));

    // ---- CursorSnapSprite registration (factory at [self+0xc]->m_8) ----
    set = ((CSpriteFactory*)PTR(PTR(self, 0xc), 0x8))
              ->CreateSprite(0, 0, 0, 0x13880, "CursorSnapSprite", 0x40001);
    I32(self, 0x4e4) = (i32)set;
    if (set != 0) {
        void* host8 = PTR(PTR(self, 0xc), 0x8);
        (*(void (**)(void*, i32))((char*)*(void**)host8 + 0x24))(host8, 0); // host8 vtable +0x24
        if (savedThis == 0) {
            // empty cursor-snap set -> reset the resource-install flag
            void* tiles = PTR(self, 0x2dc);
            i32 id = (I32(tiles, 0) == 0) ? 0x1a9 : 0x249;
            if (!((CTimer*)PTR(self, 0x3f4))->LoadTimerSprite(id, 0x1ca)) {
                void* spr = PTR(self, 0x3f4);
                if (spr != 0) {
                    ((CTimer*)spr)->Reset();
                    RezFree(spr);
                    I32(self, 0x3f4) = 0;
                }
            }
        } else {
            // load the level map + the four map sub-steps
            if (LoadWarlordSprites((i32)savedThis, (i32*)((char*)nameBuf + 0x20)) /* 0x2b80 */
                && ScanBuildTiles() /* 0x3553 */ && ValidateLevelTiles()          /* 0x345e */
                && AddLevelGruntz() /* 0x17ee */) {
                void* host8b = PTR(PTR(self, 0xc), 0x8);
                (*(void (**)(void*, i32))((char*)*(void**)host8b + 0x24))(host8b, 0);
                ((CStatusBarMgr*)PTR(self, 0x2dc))->winapi_107d00_SetRect();
                ((DirectInputMgr2*)g_645570)->ReadAll();
                while (ShowCursor(0) >= 0)
                    ;
                ((CGruntzMgr*)PTR(self, 0x4))->CGruntzMgr::PerFrameTick();
                if (PTR(PTR(PTR(self, 0xc), 0x24), 0x5c) != 0) {
                    ((CDDrawWorkerHost*)PTR(PTR(PTR(self, 0xc), 0x24), 0x5c))->GetSize_1633e0();
                }
                if (PTR(PTR(PTR(self, 0xc), 0x24), 0x5c) != 0) {
                    ((CDDrawWorkerHost*)PTR(PTR(PTR(self, 0xc), 0x24), 0x5c))->GetSize_1633e0();
                }
                BuildHelpReveal(0);
                if (savedThis != 0) {
                    ((CMulti*)savedThis)->AckJoinFailure();
                }
                RegisterInputBindings();
                if (BuildMusicCategoryTable(reload)) { // vtable +0xa4 (CPlay slot 41)
                    goto okContinue;
                }
            }
            goto fail1;
        }
    }

okContinue:
    BuildHelpReveal(0);
    if (savedThis != 0) {
        ((CMulti*)savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    BuildHelpReveal(1);
    ActiveWait(0x64);
    if (savedThis != 0) {
        ((CMulti*)savedThis)->AckJoinFailure();
    }

    gameReg = g_gameReg;
    if (I32(gameReg, 0x114) == 0) {
        void* mapHost = PTR(PTR(PTR(PTR(self, 0xc), 0x4), 0x10), 0x2c);
        ((CDDSurface*)mapHost)->ShadeRect(0x32, 0);
        gameReg = g_gameReg;
    }

    // ---- loading-screen blit (mode != 2 && m_114 == 0) ----
    if (I32(gameReg, 0x134) != 2 && I32(gameReg, 0x114) == 0) {
        CString scr; // [esp+0x14]
        I32(self, 0x4f8) = 1;
        I32(self, 0x484) = 0;
        i32 rect[4];
        rect[0] = 0;
        rect[1] = 0;
        rect[2] = 0x280;
        rect[3] = 0x1e0;
        if (scr.LoadString(0x8128)) {
            EngStr_DrawText(
                (EngStrRenderObj*)PTR(self, 0xc),
                (i32)rect,
                (i32)((char*)nameBuf + 0x4),
                0x78,
                1,
                0xff,
                0xff,
                0,
                1
            );
        }
    } else {
        I32(self, 0x484) = 1;
    }

    // ---- final state stamp ----
    I32(self, 0x4b8) = 0;
    I32(self, 0x4fc) = 0;
    I32(self, 0x500) = 0;
    I32(self, 0x4f0) = 0;
    I32(self, 0x4f4) = 0;
    I32(self, 0x400) = 0x1f4;
    I32(self, 0x404) = 0;
    I32(self, 0x3f8) = g_frameTime;
    I32(self, 0x3fc) = 0;
    I32(self, 0x408) = 1;
    ((CString*)((char*)self + 0x410))->operator=(g_emptyString);
    I32(self, 0x40c) = 0;
    I32(self, 0x470) = 0;
    I32(self, 0x474) = 0;
    I32(self, 0x478) = 0;
    I32(self, 0x47c) = 0;
    I32(self, 0x4b0) = 0;
    I32(self, 0x514) = 3;
    I32(self, 0x4ec) = 1;
    g_64e35c = 0;
    ResetViewport(); // 0x3d55 -> 0xd8c60
    if (I32(g_gameReg, 0x134) == 2) {
        g_64e35c = 1;
        I32(self, 0x4ec) = 0;
        ((CGruntzMgr*)PTR(self, 0x4))->CheckSavedMode();
        ((CFontConfig*)PTR(PTR(self, 0x4), 0x5c))->FreeNodes();
    }
    return 1;

fail1:
    return 0;
fail0:
    return 0;
}

#undef I32
#undef PTR

// @early-stop
// ~95% regalloc wall: logic + all 5 relocs pair; retail threads the reloaded registry
// global through eax for all three accesses (m_70 -> eax -> ecx) where MSVC5 here
// colors the first store's load into ecx (and the m_70 deref straight into ecx).
// Not source-steerable (eax/ecx coloring of a thrice-reloaded global).
// docs/patterns/zero-register-pinning.md.
RVA(0x000cb400, 0x58)
void CPlay::OnExit() {
    ForwardReady();
    FreeListTeardown();
    if (m_c) {
        m_c->m_childGroup->DestroyChildren_159ef0();
    }
    g_gameReg->m_128 = 0;
    if (g_gameReg->m_134 == 3) {
        g_gameReg->m_134 = 0;
    }
    g_gameReg->m_tileGrid->Reset(); // slot-0 virtual (CMapMgr::Reset @0x9ec30)
}

// ===========================================================================
// CPlay::ModeCleanup (0x0cb740) - vtable slot 0x22 mode/state-exit teardown.
// For each live sub-object of the view holder (m_c) and the world (m_4) it runs
// that object's teardown method (two are virtual). Self-contained (no DIR32
// relocs); the view ptr is re-read before every block (no cached local).
// (The CExitV58/CExitV44/CExitView/CExitWorld views + their FABRICATED padded
// vtables are GONE: the +0x10 slot-22 teardown is the canonical CDDrawWorkerRegistry
// device (CDDrawWorkerRegistry::MapTeardown_1552b0) and the +0x24 slot-17
// teardown is CGameLevel::ReleaseChildren - a REAL virtual on the real class.)
// ===========================================================================
// @early-stop
// reload-register regalloc tail (99.62%): logic byte-exact and every call reloc
// pairs; the only residual is 4 bytes - the intermediate register cl picks for the
// re-read of m_c before `m_28->ClearMap` and of m_4 before `m_54->Teardown` (cl
// folds into ecx/edx, retail loads via eax/ecx). The view ptr IS re-read each block
// as retail does; reread-member-view-pointer.md regalloc residual, not
// source-steerable (the surrounding standalone blocks already match). Final sweep.
RVA(0x000cb740, 0x8f)
void CPlay::ModeCleanup() {
    if (m_c) {
        if (m_c->m_28->m_2c) {
            m_c->m_28->m_2c->Stop();
        }
        ((CDDrawSubMgrLeafScan*)m_c->m_28)->ClearMap();
    }
    if (m_4) {
        m_4w()->m_48->StopAndFlush();
        // WRONG-CALLEE FIX (assert_relocs): this called Resume() @0x00bcf0. Retail's rel32 here
        // resolves to 0x00b660 == CWorldSoundSet::Teardown (already 100% EXACT in src). Both
        // exist and are methods of the SAME class, so it linked and objdiff reloc-masked it -
        // a silently-wrong callee. Tearing the world sound set down on mode cleanup is also the
        // only reading that makes sense next to the StopAndFlush above.
        m_4w()->m_54->Teardown();
    }
    if (m_c) {
        m_c->m_10->MapTeardown_1552b0(); // slot 22 (+0x58)
    }
    if (m_c) {
        ((CDDrawSubMgrLeaf*)m_c->m_animRegistry)->FreeAll_152720(); // @0x152720
    }
    if (m_c) {
        m_c->m_24->ReleaseChildren(); // slot 17 (+0x44) - real CGameLevel virtual
    }
    if (m_c) {
        m_c->m_childGroup->DestroyChildren_159ef0();
    }
    if (m_c) {
        m_c->m_rendererB->ClearWorkers(); // @0x163c60
    }
}

// ===========================================================================
// CPlay::OnKeyCommand (0x0cbaf0) - the PLAY-state keyboard/UI command dispatcher.
// Early-outs on the HUD-suppress gate, then a priority chain: resume from a paused/
// disabled frame (re-arm the in-game mode), bail to ReportError if the per-frame
// reset is still pending, un-pause, or (no active grunt) forward to the overlay
// layer / dispatch the bracket-key zoom guts steps. __thiscall(key, flag), ret 8.
// ===========================================================================
// @early-stop
// ~82% identical-return-epilogue tail-merge wall: the whole priority chain + every
// cmp/call is byte-faithful and all relocs pair, but MSVC5's epilogue merger picks a
// different set of return-1/return-0 sites to share vs inline than retail (retail
// inlines the m_hudSuppressed return-1 + shares the two return-0 tails; we do the
// reverse), cascading a small offset shift. Not source-steerable.
// docs/patterns/identical-return-epilogue-tailmerge.md.
RVA(0x000cbaf0, 0x16f)
i32 CPlay::OnKeyCommand(i32 key, i32 flag) {
    if (m_hudSuppressed != 0) {
        return 1;
    }
    if (m_renderDisabled != 0) {
        m_renderDisabled = 0;
        m_hudSuppressed = 1;
        EnterMode(3);
        m_inGame = 1;
        return 1;
    }
    if (m_inGame != 0) {
        if (ResetPlayState()) {
            return 1;
        }
        m_4->ReportError(0x800a, 0x456);
        return 1;
    }
    if (m_paused != 0) {
        m_paused = 0;
        PostMessageA(m_4w()->m_4->m_4->m_4, 0x111, 0x816e, 0);
        return 1;
    }
    if (m_4w()->m_c != 0) {
        return 0;
    }
    if (m_hitTest->m_10 != 0) {
        m_4w()->m_5c->TypeChar(key, flag); // 0x3508 -> CFontConfig::TypeChar @0x21e20
        return 1;
    }
    if (key == ']') {
        m_guts->winapi_0fe520_SetRect();
        return 1;
    }
    if (key == '[') {
        m_guts->RefreshA();
        return 1;
    }
    if (key == '-') {
        m_guts->HideRect();
        return 1;
    }
    if (key == '=' || key == '+') {
        m_guts->RefreshState();
        m_hitTest->Configure(m_guts->m_position == 1 ? 2 : 1);
        return 1;
    }
    return 0;
}

// CPlay::Vslot1c (0xd0050, slot 28 / +0x70) - counts the sprite factory's live objects
// whose collision-category word (CGameObject +0xe8) equals `category`. The factory's
// live-object list container sits at m_c->m_8+0x10 (its head pointer at +0x14 IS
// CSpriteFactory::m_liveObjects); retail walks it through the container, so the head +
// node fields are read via documented offsets (this TU does not pull SpriteFactory.h/
// UserLogic.h) to reproduce the `add 0x10; [+4]` node walk. Node: next@+0, sprite@+8.
RVA(0x000d0050, 0x3a)
i32 CPlay::Vslot1c(i32 category) {
    char* container = (char*)m_c->m_8 + 0x10;
    if (container == 0) {
        return 0;
    }
    char* node = *(char**)(container + 4);
    i32 count = 0;
    while (node != 0) {
        char* p = node;
        node = *(char**)node;
        char* sprite = *(char**)(p + 8);
        if (sprite != 0 && *(u32*)(sprite + 0xe8) == (u32)category) {
            count++;
        }
    }
    return count;
}

// ===========================================================================
// CPlay::SyncState  (0x0d7520; re-homed from the former playsync unit, waveP - its
// retail birth position is inside THIS TU's 0xd5960 interval, TU_MIGRATION MOVE row
// `0x0d7520 SyncState@CPlay playsync -> 0xd5960 play`). The play-state serialize/
// round-trip: bail on a null archive, run the header serialize, a mode pre-step
// (4=write, 7=read, 8=re-init ambient cue), then round-trip a sequence of 64-bit
// timer blocks (mode 4 -> Write vtbl[0x30], mode 7 -> Read vtbl[0x2c]) interleaved
// with three child sync sub-objects (m_guts / m_frameMarker / m_beginMarker).
// Archive Read/Write + child syncs are reloc-masked external.
// ---------------------------------------------------------------------------
// Round-trip a 16-byte (two 4-int halves) timer block through the archive; `p` is
// an i32* into the block. The mode-4 (write) body is emitted out-of-line (retail
// checks mode==4 first via je, keeps the mode-7 read body inline).
#define SYNC_PAIR(ar, mode, p)                                                                     \
    if ((mode) != 4) {                                                                             \
        if ((mode) == 7) {                                                                         \
            (ar)->Read((p), 8);                                                                    \
            (ar)->Read((p) + 2, 8);                                                                \
        }                                                                                          \
    } else {                                                                                       \
        (ar)->Write((p), 8);                                                                       \
        (ar)->Write((p) + 2, 8);                                                                   \
    }

// The +0x2dc guts child-sync is CLevelSync::Sync (0x1084d0) - <Gruntz/LevelSync.h>
// canonical, included above. NB the same 0x1084d0 runs with ecx == m_guts (a
// CStatusBarMgr, its 0x630 alloc proven) -> CLevelSync is very likely ANOTHER fake
// name for CStatusBarMgr; the ((CLevelSync*)m_guts) cast below is the honest
// symptom of that pending reconciliation (LevelSync.cpp was just un-merged by a
// parallel lane - not folded here).
// The +0x2e4 begin-marker child-sync is the canonical CTileTriggerContainer
// (<Gruntz/TileTriggerContainer.h>, included above); m_beginMarker is typed so the
// calls below are cast-free.

RVA(0x000d7520, 0x3b9)
i32 CPlay::SyncState(CSerialArchive* ar, i32 mode, i32 a2, i32 a3) {
    if (ar == 0) {
        return 0;
    }
    if (!HeaderSerialize(ar, mode, a2, a3)) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (!SyncWrite19fb(ar)) {
                return 0;
            }
            break;
        case 7:
            if (!SyncRead2f7c(ar)) {
                return 0;
            }
            break;
        case 8: {
            if (m_gridHasSprite) {
                CWorld* w = m_4w();
                i32 id = g_curPlayer;
                void* spr = (void*)w->m_74->GetSel((i32) * (void**)(w->m_158 + (id * 0x47) * 8), 0);
                if (spr == 0) {
                    spr = (void*)g_gameReg->m_spriteFactory->GetSel(1, (i32)spr);
                }
                ((CImageSet*)m_grid)->SetAllTypes(0xa);
                ((CImageSet*)m_grid)->SetAllFormats((i32)spr);
            }
            char buf[0x40];
            wsprintfA(buf, "AMBIENT%d", GetAmbientId());
            if (g_gameReg->m_14) {
                m_4w()->m_48->PlayByName(buf, 1);
            }
            m_ambientInitDone = 1;
            break;
        }
    }

    i32* p;
    p = &m_syncTimerLo;
    SYNC_PAIR(ar, mode, p);
    if (!((CLevelSync*)m_guts)->Sync(ar, mode, a2, a3)) { // guts child-sync @0x1084d0
        return 0;
    }
    if (!m_frameMarker->HandleEvent(ar, mode, a2, a3)) { // CTimer's real serialize entry
        return 0;
    }
    p = &m_cueTimerLo;
    SYNC_PAIR(ar, mode, p);
    if (!m_beginMarker->Serialize(ar, mode, a2, a3)) { // 0x117280
        return 0;
    }
    p = &m_region0TimerLo;
    SYNC_PAIR(ar, mode, p);
    p = &m_region1TimerLo;
    SYNC_PAIR(ar, mode, p);
    p = &m_snapBaseLo;
    SYNC_PAIR(ar, mode, p);
    p = &m_region2TimerLo;
    SYNC_PAIR(ar, mode, p);
    p = &m_region3TimerLo;
    SYNC_PAIR(ar, mode, p);
    p = &m_bootyTimerLo;
    SYNC_PAIR(ar, mode, p);
    return 1;
}
#undef SYNC_PAIR

// ===========================================================================
// CArchiveLoadRec::Load (0x0d79d0; re-homed from the former streamrecordloaders unit,
// waveP - TU_MIGRATION MOVE row `0x0d79d0 Load@CArchiveLoadRec streamrecordloaders ->
// 0xd5960 play`). The CArchive-store / CString-default flavor of the record
// serializer: a larger record streamed through the shared CSerialArchive's +0x30 slot
// (Write). __thiscall, ret 4; returns 0 when the archive or the bound manager (m_c) is
// absent, else 1. NB the CArchiveLoadRec::Load method name is the recovered-symbol
// placeholder; only the +0x30 slot offset is load-bearing. g_serialCounter /
// g_lastLevelNum reuse this TU's decls; the record views are local.
// ===========================================================================
extern i32 g_serialCounter; // ?g_serialCounter@@3HA @0x629ad0 (bumped per string field)

// The last-loaded level number (.data, init -1; compared/assigned by the level loader
// above and streamed after m_40c here). Owner-TU definition; extern in <Globals.h>.
DATA(0x00212618)
i32 g_lastLevelNum = -1;

// (The CArchiveMgr/CArchiveDefInt/CArchiveSubArray/CArchiveLoadRec views are GONE:
// CArchiveLoadRec was a full-object 0x514 placeholder of CPLAY ITSELF (the fn is
// CPlay::SyncWrite19fb, SyncState's mode-4 serializer, called `mov ecx,edi; push ar`),
// its "manager" CArchiveMgr was the CState::m_c holder (the +0x10 registry probe is
// CDDrawWorkerRegistry::AnyValueMatches_155630 - the frame-name reverse lookup over
// m_gridCurFrame), its "+0x24 inline name" object was CPlay::m_grid (CFrameGrid's
// config name), the "+0x188 default-int" object was the CursorSnapSprite game object
// (CGameObject::m_188, the archive-cue id) and the element arrays are the
// m_startMarkers / m_3a4[4] / m_488 MFC arrays read raw. Body folded below.)
// @early-stop
// scratch-slot scheduling tail (~99.8%): every serialize field/size, the two unsigned
// count loops, the nested sub-array loop, the inline strlen/strcpy default copies,
// the g_serialCounter bumps, the conditional reverse-lookup call and the final
// signed element loop are byte-faithful. The sole residual is the MSVC5 scheduler
// parking one extra scratch slot (frame 0x294 vs retail 0x28c) + a few zero-init
// store positions - the entropy tail the CTriggerLoadRec/CEventLoadRec siblings
// share; not source-steerable.
RVA(0x000d79d0, 0x537)
i32 CPlay::SyncWrite19fb(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    CSpriteFactoryHolder* mc = m_c;
    if (mc == 0) {
        return 0;
    }

    s->Write(&m_1bc, 4);
    s->Write(&m_1c0, 4);
    s->Write(&m_savedClock, 4);
    s->Write(&m_rngSeed, 4);
    s->Write(&m_dragInProgress, 4);
    s->Write(&m_2f0, 4);
    s->Write(&m_cursorFrame, 4);
    s->Write(&m_levelId, 4);
    s->Write(&m_2fc, 8);
    s->Write(&m_tileClickX, 8);
    s->Write(&m_dragInhibit1, 4);
    s->Write(&m_dragInhibit2, 4);

    i32 c0 = markerCount();
    s->Write(&c0, 4);
    for (u32 i0 = 0; i0 < (u32)c0; i0++) {
        s->Write(markerData()[i0], 8);
    }

    char* p = m_pad384;
    for (i32 k0 = 4; k0 != 0; k0--) {
        s->Write(p, 8);
        p += 8;
    }

    for (i32 k1 = 0; k1 < 4; k1++) {
        i32 cnt = arrCount(k1);
        s->Write(&cnt, 4);
        for (u32 i1 = 0; i1 < (u32)cnt; i1++) {
            s->Write(arrData(k1)[i1], 8);
        }
    }

    s->Write(&m_cueToggle, 4);

    g_serialCounter++;
    {
        char buf[0x200];
        memset(buf, 0, sizeof(buf));
        strcpy(buf, (const char*)m_cueText);
        s->Write(buf, 0x200);
    }

    s->Write(&m_lastCueId, 4);
    s->Write(&g_lastLevelNum, 4);

    g_serialCounter++;
    {
        char buf[0x80];
        memset(buf, 0, sizeof(buf));
        i32 v = 0;
        if (m_gridCurFrame != 0) {
            ((CDDrawWorkerRegistry*)mc->m_10)
                ->AnyValueMatches_155630(m_gridCurFrame, (i32)buf, (i32)&v);
        }
        s->Write(buf, 0x80);
        s->Write(&v, 4);
    }

    g_serialCounter++;
    {
        char buf[0x80];
        memset(buf, 0, sizeof(buf));
        if (m_grid != 0) {
            strcpy(buf, m_grid->m_name24);
        }
        s->Write(buf, 0x80);
    }

    s->Write(&m_gridDelayBase, 4);
    s->Write(&m_gridDelayCount, 4);
    s->Write(&m_gridRow, 4);

    g_serialCounter++;
    {
        i32 v = 0;
        if (m_scrollSink != 0) {
            v = m_scrollSink->m_188;
        }
        s->Write(&v, 4);
    }

    s->Write(&m_gridWalkActive, 4);
    s->Write(&m_renderDisabled, 4);
    s->Write(&m_winLoseBanner, 4);
    s->Write(&m_1c4, 4);
    s->Write(&m_hudSuppressed, 4);
    s->Write(&m_inGame, 4);
    s->Write(&m_overlayDrag, 4);
    s->Write(&m_paused, 4);
    s->Write(&m_4f0, 4);
    s->Write(&m_dragEndNotify, 4);
    s->Write(&m_drewThisFrame, 4);
    s->Write(&m_418, 4);
    s->Write(&m_41c, 4);
    s->Write(&m_420, 4);
    s->Write(&m_424, 4);
    s->Write(&m_428, 2);
    s->Write(&m_region0Gate, 4);
    s->Write(&m_region1Gate, 4);
    s->Write(&m_region2Gate, 4);
    s->Write(&m_region3Gate, 4);
    s->Write(&m_viewMode, 4);
    s->Write(&m_snapshotActive, 4);
    s->Write(&m_gridHasSprite, 4);
    s->Write(&m_49c, 4);
    s->Write(&m_514, 4);

    i32 c1 = arr488Count();
    s->Write(&c1, 4);
    for (i32 fi = 0; fi < arr488Count(); fi++) {
        void* el = arr488Data()[fi];
        if (el != 0) {
            s->Write(el, 8);
        }
    }

    return 1;
}

// ===========================================================================
// CGrunt::Load (0x0d8060; re-homed from Grunt.cpp wave3-J - the retail body's
// birth position is inside THIS TU's 0xd5960 interval, TU_MIGRATION MOVE row
// `0x0d8060 Load@CGrunt grunt -> 0xd5960 play`; the wave3-I Grunt.h header wall
// is gone). g_gameReg (the CGameRegistry* view, via <Io/SaveGame.h>) is the same
// 0x24556c singleton Grunt.cpp reaches as its WwdGameReg view; m_world is the
// same +0x30 slot, so the GruntResMgr cast is unchanged.
// ---------------------------------------------------------------------------
// CGrunt::Load(ar)  @0xd8060  (__thiscall, ret 4) - the Read inverse of Save.
// Bails (return 0) if the archive or the resource manager (g_gameReg->m_world) is
// null. Reads back the sprite-id + arrival fields, rebuilds the owned node
// collections (this+0x370, the four at this+0x3a4, this+0x488) by recycling each
// existing node back onto the global coord free-list, clearing the array, then
// re-popping fresh nodes for each streamed record. Resolves the two anim-name id
// records (this+0x4d0/0x4cc) + the object record (this+0x4e4, validated by the
// engine kind() vtable slot 0x20 == 5; a found-but-invalid record fails the load),
// streams the ~70 plain fields, then rebuilds the trailing collection.
// @early-stop
// reloc-masked-extern plateau (Save's symmetric pair): the field/record Read
// stream, the freelist recycle + CObArray SetSize/SetAtGrow rebuild loops, the
// resource-mgr name/object lookups and the load-fail bail are reconstructed in
// shape/order. Residue is the ~90 archive-Read + collection + map-lookup call
// operands pairing to differently named retail symbols (the whole referent set
// is external). Final sweep.
// reloc-fidelity: 0xd8060 IS CPlay::SyncRead2f7c - CPlay::SyncState (0xd7520) calls it
// __thiscall (mov ecx,edi=this; push ar) as the mode-7 (read) play-state serializer.
// SYMBOL exports it under the canonical CPlay name so that call binds; the CGrunt::Load
// view is the recovered-symbol placeholder for CPlay's Read serializer (body-fold deferred).
SYMBOL(?SyncRead2f7c@CPlay@@QAEHPAUCSerialArchive@@@Z)
RVA(0x000d8060, 0x6ce)
i32 CGrunt::Load(CGruntArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    GruntResMgr* res = (GruntResMgr*)g_gameReg->m_world;
    if (res == 0) {
        return 0;
    }

    ar->Read(&m_toySprite, 4);
    ar->Read(&m_animSetName, 4);
    ar->Read(&m_toyTimeSprite, 4);
    ar->Read(&m_2d8, 4);
    ar->Read(&m_dwell, 4);
    ar->Read(&m_arrivalCol, 4);
    ar->Read(&m_arrivalRow, 4);
    ar->Read(&m_2f8, 4);
    ar->Read(&m_2fc, 8);
    ar->Read(&m_deathType, 8);
    ar->Read(&m_deathAnimStarted, 4);
    ar->Read(&m_36c, 4);

    {
        GruntLoadColl* coll = (GruntLoadColl*)(&m_370);
        for (i32 i = 0; i < coll->m_count; i++) {
            void* node = coll->m_data[i];
            if (node) {
                void** p = (void**)((char*)node - g_coordPool.m_linkOffset);
                *p = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = p;
            }
        }
        ((CPtrArray*)coll)->SetSize(0, -1);
        i32 n;
        ar->Read(&n, 4);
        for (u32 j = 0; j < (u32)n; j++) {
            void* node = 0;
            void** head = (void**)g_coordPool.m_freeHead;
            void* next = *head;
            if (next) {
                node = (char*)head + 4;
                g_coordPool.m_freeHead = next;
            }
            ar->Read(node, 8);
            ((CPtrArray*)coll)->SetAtGrow(coll->m_count, node);
        }
    }

    ar->Read(&m_coordRetryCount, 8);
    ar->Read(&m_38c, 8);
    ar->Read(&m_poseWalk, 8);
    ar->Read(&m_poseAttack2, 8);

    {
        GruntLoadColl* coll = (GruntLoadColl*)(&m_poseStruck1);
        i32 k = 4;
        do {
            for (i32 i = 0; i < coll->m_count; i++) {
                void* node = coll->m_data[i];
                if (node) {
                    void** p = (void**)((char*)node - g_coordPool.m_linkOffset);
                    *p = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = p;
                }
            }
            ((CPtrArray*)coll)->SetSize(0, -1);
            i32 n;
            ar->Read(&n, 4);
            for (u32 j = 0; j < (u32)n; j++) {
                void* node = 0;
                void** head = (void**)g_coordPool.m_freeHead;
                void* next = *head;
                if (next) {
                    node = (char*)head + 4;
                    g_coordPool.m_freeHead = next;
                }
                ar->Read(node, 8);
                ((CPtrArray*)coll)->SetAtGrow(coll->m_count, node);
            }
            coll = (GruntLoadColl*)((char*)coll + 0x14);
        } while (--k);
    }

    ar->Read(&m_408, 4);
    g_serialCounter++;
    char buf512[0x200];
    ar->Read(buf512, 0x200);
    ((CString*)(&m_410))->operator=(buf512);
    ar->Read((char*)&m_408 + 4, 4);
    ar->Read(&g_load612618, 4);

    g_serialCounter++;
    char buf80a[0x80];
    ar->Read(buf80a, 0x80);
    i32 idx;
    ar->Read(&idx, 4);
    if (strlen(buf80a) == 0) {
        *(i32*)&m_cells[1].m_attack = 0;
    } else {
        GruntIdEntry* entry = 0;
        ((CMapStringToPtr*)(res->m_10 + 0x10))->Lookup((const char*)buf80a, (void*&)entry);
        if (entry == 0 || idx < entry->m_64 || idx > entry->m_68) {
            *(i32*)&m_cells[1].m_attack = 0;
        } else {
            *(i32*)&m_cells[1].m_attack = entry->m_14[idx];
        }
    }

    g_serialCounter++;
    char buf80b[0x80];
    ar->Read(buf80b, 0x80);
    void* entry2 = 0;
    // +0x4cc: the serialize path streams a raw 4-byte slice over m_cells[0].m_stepY's
    // high half (the same (char*)+4 raw-slice style as the m_408/m_410 reads).
    if (strlen(buf80b) == 0) {
        *(i32*)((char*)&m_cells[0].m_stepY + 4) = 0;
    } else {
        ((CMapStringToPtr*)(res->m_10 + 0x10))->Lookup(buf80b, entry2);
        *(i32*)((char*)&m_cells[0].m_stepY + 4) = (i32)entry2;
    }

    ar->Read(&m_cells[1].m_walk, 4);
    ar->Read(&m_cells[1].m_idle, 4);
    ar->Read(&m_cells[1].m_item, 4);
    g_serialCounter++;
    i32 v;
    ar->Read(&v, 4);
    CWwdGameObjectE* oe = 0;
    ((CMapPtrToPtr*)(res->m_8 + 0x48))->Lookup((void*)entry2, (void*&)oe);
    i32 ve;
    if (oe == 0) {
        ve = 0;
    } else {
        // GetClassId (slot 8) == 5: the serialize-map probe kind (id 5's class TBD)
        ve = oe->GetClassId() == 5 ? (i32)oe : 0;
    }
    m_cells[1].m_14 = ve;
    if (ve == 0 && entry2 != 0) {
        return 0;
    }

    ar->Read(&m_cells[1].m_18, 4);
    ar->Read(&m_cells[1].m_1c, 4);
    ar->Read(&m_cells[1].m_24, 4);
    ar->Read(&m_healthSprite, 4);
    ar->Read(&m_cells[0].m_1c, 4);
    ar->Read(&m_cells[1].m_28, 4);
    ar->Read(&m_cells[1].m_2c, 4);
    ar->Read(&m_cells[1].m_30, 4);
    ar->Read(&m_cells[1].m_20, 4);
    ar->Read(&m_cells[1].m_34, 4);
    ar->Read((char*)&m_410 + 4, 4);
    ar->Read(&m_418, 4);
    ar->Read(&m_timePerTile, 4);
    ar->Read(&m_tileClaimed, 4);
    ar->Read(&m_424, 4);
    ar->Read(&m_428, 2);
    ar->Read(&m_cells[0].m_walk, 4);
    ar->Read(&m_cells[0].m_idle, 4);
    ar->Read(&m_cells[0].m_item, 4);
    ar->Read(&m_cells[0].m_14, 4);
    ar->Read(&m_cells[0].m_18, 4);
    ar->Read(&m_cells[0].m_dirX, 4); // raw low half of the +0x48 dir-vector double
    ar->Read(&m_cells[1].m_struck, 4);
    ar->Read(&m_cells[0].m_34, 4);
    m_cells[1].m_40 = 2;
    ar->Read(&m_cells[1].m_44, 4);

    i32 n488;
    ar->Read(&n488, 4);
    {
        GruntLoadColl* coll = (GruntLoadColl*)((char*)this + 0x488);
        for (i32 i = 0; i < coll->m_count; i++) {
            void* node = coll->m_data[i];
            if (node) {
                void** p = (void**)((char*)node - g_coordPool.m_linkOffset);
                *p = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = p;
            }
        }
        ((CPtrArray*)coll)->SetSize(0, -1);
        ((CPtrArray*)coll)->SetSize(n488, -1);
        for (u32 j = 0; j < (u32)n488; j++) {
            void* node = 0;
            void** head = (void**)g_coordPool.m_freeHead;
            void* next = *head;
            if (next) {
                node = (char*)head + 4;
                g_coordPool.m_freeHead = next;
            }
            ar->Read(node, 8);
            coll->m_data[j] = node;
        }
    }
    return 1;
}

// ===========================================================================
// CPlay::ResetViewport (0x0d8c60) - ClampViewport's no-change fallback: build the
// full-screen viewport rect (a guts-state-dependent left/right bias), optionally
// re-center it to a 0xc0 box (region-0 gate), pin the view discriminator to idle,
// install the rect on the draw-surface and run the world apply-tail. __thiscall.
// ===========================================================================
// @early-stop
// ~95% regalloc wall: the SetRect dispatch, the region-0 re-center block and the
// apply-tail are byte-faithful with all relocs pairing; only the 5-instruction
// prologue colors the (m_4, right, bottom) trio eax/ecx-swapped vs retail (retail
// pins m_4 in ecx and right in eax; we mirror). Tried explicit load ordering.
// docs/patterns/zero-register-pinning.md.
RVA(0x000d8c60, 0xea)
i32 CPlay::ResetViewport() {
    CWorld* w = m_4w();
    CStatusBarMgr* guts = m_guts;
    i32 right = w->m_8c;
    i32 state = guts->m_position;
    i32 bottom = w->m_90;
    RECT r;
    if (state == 1) {
        SetRect(&r, 0xa0, 0, right - 1, bottom - 1);
    } else if (state == 0) {
        SetRect(&r, 0, 0, right - 0xa1, bottom - 1);
    } else {
        SetRect(&r, 0, 0, right - 1, bottom - 1);
    }
    if (m_region0Gate) {
        i32 halfW = (r.right - r.left) / 2;
        i32 halfH = (r.bottom - r.top) / 2;
        r.left = r.left + halfW - 0x60;
        r.top = r.top + halfH - 0x60;
        r.right = r.right + (0x60 - halfW);
        r.bottom = r.bottom + (0x60 - halfH);
    }
    m_viewMode = VIEW_MODE_IDLE;
    m_c->m_24->BuildAllPlanes((LevelCoordRect*)&r); // 0x15da80
    m_4->RecomputeViewScale();
    return 1;
}

// ===========================================================================
// CPlay::StepC - the menu/overlay-frame view
// sub-step. A 3-way switch on the view-mode discriminator m_viewMode: 0 = idle (no
// view yet, bail), 1 = mode-A sub-step, 2(+) = mode-B sub-step. MSVC hoists the
// shared `push 4` out of the if/else (both helpers take the same constant).
// ===========================================================================
RVA(0x000d8d90, 0x1e)
void CPlay::StepC() {
    i32 mode = m_viewMode;
    if (mode == VIEW_MODE_IDLE) {
        return;
    }
    if (mode == VIEW_MODE_A) {
        StepC_ModeA(4);
    } else {
        StepC_ModeB(4);
    }
}

// ===========================================================================
// CPlay::ClampViewport (0x0d8dc0) - if the active viewport (m_c->m_24+0x10) is
// wider/taller than 0xc0, inset that axis by `inset` (both edges); if NEITHER axis
// was clamped, just reset the viewport and bail. Otherwise install the clamped rect
// on the draw-surface (SetClipRect), re-prepare the held surface, and run the guts +
// world apply-steps. Returns 1 if clamped, 0 otherwise. __thiscall, ret 4.
// ===========================================================================
// @early-stop
// ~94% regalloc wall - the whole clamp body + the apply-tail call chain are byte-
// faithful and all relocs pair; retail pins `clamped` in edx and the rect-base copy
// in ebx where MSVC5 here colors them edi/ebx-swapped (a cascade off the entry
// `xor edx,edx` vs `xor edi,edi`). Not source-steerable (tried explicit rect-base
// pointer + reordered the clamped init). docs/patterns/zero-register-pinning.md.
RVA(0x000d8dc0, 0xce)
i32 CPlay::ClampViewport(i32 inset) {
    CSpriteFactoryHolder* v = m_c;
    LevelCoordRect* vp = &v->m_24->m_planeCtx;
    RECT r;
    r.left = vp->minX;
    r.top = vp->minY;
    r.right = vp->maxX;
    r.bottom = vp->maxY;

    i32 clamped = 0;
    if (r.right - r.left > 0xc0) {
        r.left += inset;
        r.right -= inset;
        clamped = 1;
    }
    if (r.bottom - r.top > 0xc0) {
        r.top += inset;
        r.bottom -= inset;
        clamped = 1;
    }
    if (clamped == 0) {
        ResetViewport();
        return 0;
    }

    m_c->m_24->BuildAllPlanes((LevelCoordRect*)&r); // 0x15da80 (was the fake "SetClipRect")
    m_c->m_drawTarget->m_14->m_surface->Fill(0);
    m_guts->Deactivate(); // 0x125d -> CStatusBarMgr::Deactivate @0x100cb0 (was "ClampApply")
    m_4->RecomputeViewScale();
    return 1;
}

// ===========================================================================
// CPlay::ClampViewport2 (0x0d8ed0) - the asymmetric viewport clamp. Each axis whose
// extent is BELOW the world limit (horizontal m_4->m_8c, biased down 0xa0 unless the
// guts subsystem is ready; vertical m_4->m_90) is EXPANDED by `stride` on both edges,
// then clamped into [0, limit-1]. If neither axis moved, reset the viewport and bail;
// otherwise install the clamped rect + run the apply-tail. __thiscall, ret 4.
// ===========================================================================
// @early-stop
// ~86% regalloc wall - the asymmetric clamp logic, the guts-gated horizontal bound
// (m_8c / m_8c-0xa0, inlined ternary so it stays in edi not a spill), both axis
// [0, limit-1] clamps, and the 4-call apply-tail are byte-faithful with all relocs
// pairing. The residual: retail carries the `clamped` accumulator through `ecx`
// across the two blocks (allocating an extra spill slot -> `sub esp,0x1c`) where
// MSVC5 here writes it straight to [esp+0x10] (`sub esp,0x18`), shifting the rect
// slots by 4 and renaming the block-2 temps. Not source-steerable (the two-block
// boolean-OR register-merge is the compiler's spill choice).
// docs/patterns/zero-register-pinning.md.
RVA(0x000d8ed0, 0x128)
i32 CPlay::ClampViewport2(i32 stride) {
    i32 clamped = 0;
    CSpriteFactoryHolder* v = m_c;
    CWorld* w = m_4w();
    CStatusBarMgr* guts = m_guts;

    i32* rp = (i32*)&v->m_24->m_planeCtx;
    RECT r;
    r.left = rp[0];
    r.top = rp[1];
    r.right = rp[2];
    r.bottom = rp[3];

    i32 hlimit = w->m_8c;
    i32 vlimit = w->m_90;

    if (r.right - r.left < (guts->m_position == 2 ? hlimit : hlimit - 0xa0)) {
        r.left -= stride;
        r.right += stride;
        if (r.left < 0) {
            r.left = 0;
        }
        if (r.right >= hlimit) {
            r.right = hlimit - 1;
        }
        clamped = 1;
    }
    if (r.bottom - r.top < vlimit) {
        r.top -= stride;
        r.bottom += stride;
        if (r.top < 0) {
            r.top = 0;
        }
        if (r.bottom >= vlimit) {
            r.bottom = vlimit - 1;
        }
        clamped = 1;
    }

    if (clamped == 0) {
        ResetViewport();
        return 0;
    }

    m_c->m_24->BuildAllPlanes((LevelCoordRect*)&r); // 0x15da80
    m_c->m_drawTarget->m_14->m_surface->Fill(0);
    m_guts->Deactivate();
    m_4->RecomputeViewScale();
    return 1;
}

// CPlay::RegionEnter (0x0d88f0) - on entering a special region, save the
// currently-playing zoned sound (m_savedZonedSound) and silence the bank, then (when the dev
// window is up) start the "CURSE" cue. The shared on-enter sub-step the OnRegion
// one-shots call. Migrated from engine_boundary (CPlay).
RVA(0x000d88f0, 0x44)
void CPlay::RegionEnter() {
    if (m_savedZonedSound == 0) {
        CWorld* w = m_4w();
        m_savedZonedSound = w->m_48->m_pCurrent;
        w->m_48->StopAll();
    }
    if (g_gameReg->m_14 != 0) {
        m_4w()->m_48->PlayByName("CURSE", 0);
    }
}

// CPlay::RegionLeave (0x0d8960) - on leaving (only when no region gate is still
// set and a sound was saved), stop the bank, restore the saved zoned sound, and
// (dev-window) restart it. Migrated from engine_boundary (CPlay).
RVA(0x000d8960, 0x75)
void CPlay::RegionLeave() {
    if (m_region0Gate == 0 && m_region1Gate == 0 && m_region2Gate == 0 && m_region3Gate == 0
        && m_savedZonedSound != 0) {
        m_4w()->m_48->IsPlaying();
        m_4w()->m_48->m_pCurrent = m_savedZonedSound;
        if (g_gameReg->m_14 != 0) {
            m_4w()->m_48->Restart(1);
        }
        m_savedZonedSound = 0;
    }
}

// ===========================================================================
// The four screen-region scroll one-shots.
// Each: thiscall(int z), set its region-active gate to bool(z), call the shared
// enter/leave sub-step, (re)arm its 64-bit countdown timer (interval 0x7530 ms,
// lo = g_frameTime, hi = 0), and return 1. They share ONE shape; OnRegion2 also
// pins the view discriminator m_viewMode, OnRegion3/4 fire an extra cue on enter/leave.
// ===========================================================================
RVA(0x000d8a00, 0x73)
i32 CPlay::OnRegion2(i32 z) // (region-0 / gate m_region0Gate, timer +0x430)
{
    if (z != 0) {
        m_region0Gate = 1;
        RegionEnter();
        m_viewMode = VIEW_MODE_A;
    } else {
        m_region0Gate = 0;
        RegionLeave();
        m_viewMode = VIEW_MODE_B;
    }
    m_region0Interval = REGION_INTERVAL_MS;
    m_region0IntervalHi = 0;
    *(u64*)&m_region0TimerLo = g_frameTime; // 64-bit store: lo=g_frameTime, hi=0
    return 1;
}

RVA(0x000d8aa0, 0x5f)
i32 CPlay::OnRegion1(i32 z) // (region-1 / gate m_region1Gate, timer +0x440)
{
    if (z != 0) {
        m_region1Gate = 1;
        RegionEnter();
    } else {
        m_region1Gate = 0;
        RegionLeave();
    }
    m_region1Interval = REGION_INTERVAL_MS;
    m_region1IntervalHi = 0;
    *(u64*)&m_region1TimerLo = g_frameTime; // 64-bit store: lo=g_frameTime, hi=0
    return 1;
}

RVA(0x000d8b20, 0x74)
i32 CPlay::OnRegion3(i32 z) // (region-2 / gate m_region2Gate, timer +0x450)
{
    if (z != 0) {
        m_region2Gate = 1;
        RegionEnter();
        Cmd_ApplyScrollParams_0ec1c0(REGION_INTERVAL_MS, 6, 6, 0, 0x2d);
    } else {
        m_region2Gate = 0;
        RegionLeave();
    }
    m_region2Interval = REGION_INTERVAL_MS;
    m_region2IntervalHi = 0;
    *(u64*)&m_region2TimerLo = g_frameTime; // 64-bit store: lo=g_frameTime, hi=0
    return 1;
}

RVA(0x000d8bc0, 0x71)
i32 CPlay::OnRegion4(i32 z) // (region-3 / gate m_region3Gate, timer +0x460)
{
    if (z != 0) {
        m_region3Gate = 1;
        RegionEnter();
    } else {
        m_region3Gate = 0;
        RegionLeave();
        g_gameReg->m_cmdGrid->CycleMoveIcons(-1, 0); // reg->m_68 (CTriggerMgr) @0x7c2e0
    }
    m_region3Interval = REGION_INTERVAL_MS;
    m_region3IntervalHi = 0;
    *(u64*)&m_region3TimerLo = g_frameTime; // 64-bit store: lo=g_frameTime, hi=0
    return 1;
}

// ===========================================================================
// CPlay::NotifyVisibleEntities (0x0d9050) - notify the draw surface of the (slightly
// inflated) viewport, then walk the renderer's entity list and, for every entity
// whose type-discriminator (entity->m_8->m_7c[4], the slot-4 method pointer) is one
// of 12 known "visible-notify" types, dispatch its slot-0x2c notify with the held
// view surface. Returns 1. __thiscall, ret 0.
//
// The 12 type addresses are reached in retail through ILT jump-thunks (thunk_FUN_*);
// the recompile binds them to the direct functions, so the `cmp eax,imm32; je` BYTES
// match retail 1:1 but the 12 DIR32 operands are differently-named (thunk vs direct)
// reloc-masked symbols.
// ===========================================================================
// @early-stop
// ~75% reloc-masked plateau. The viewport-notify, the entity-list walk, the 12-way
// `cmp eax,<type>; je` type-discriminator chain (same order, same encoding) and the
// slot-0x2c notify dispatch are all byte-faithful. The residual is (1) the 12 DIR32
// reloc-name mismatches - retail compares against ILT thunk addresses, the recompile
// against the direct function addresses (the delinker has no symbol for the thunks),
// and (2) a small head regalloc swap + retail's extra `push 0` frame slot. Logic
// complete; the reloc-name set cannot pair (docs/patterns/reloc-typing-vptr-global.md,
// objdiff-reloc-scoring), so deferred to the final sweep.
// ===========================================================================

// (CVisEntity/CVisEntityType/CVisNode are GONE - the entity is the canonical
// CGameObject (<Gruntz/UserLogic.h>): its REAL slot-11 virtual is Draw (the
// VisitVisible per-object hook, +0x2c) and the "+0x7c type record's slot 4" is
// AnimWorkerObj::Init (+0x10), the post-create init fn-ptr the factory stamps -
// compared by ADDRESS as the object's dynamic-type discriminator. The list node
// is the shared CWarlordListNode (<Gruntz/View.h>).)

// The 12 known "visible-notify" entity-type discriminators: the slot-4 method
// pointers (in retail, the ILT jump-thunks) of 12 entity classes. The entity's
// m_7c[4] is compared by ADDRESS against each, so they are modeled as functions and
// the compares lower to `cmp eax, OFFSET Fn` (DIR32). The thunk-vs-direct reloc
// naming is the scoring artifact; the compare bytes match retail.
extern "C" {
    void VisFn_40fe90();        // 0x40fe90
    void VisFn_4bf150();        // 0x4bf150
    void VisFn_423b40();        // 0x423b40
    void VisFn_Roll();          // 0x4cd70  (Roll)
    void VisFn_41e570();        // 0x41e570
    void VisFn_41e520();        // 0x41e520
    void VisFn_49b410();        // 0x49b410
    void VisFn_IntersectRect(); // 0x432060 (winapi_032060_IntersectRect)
    void VisFn_49b310();        // 0x49b310
    void VisFn_CBattlezDlg();   // 0x414b30 (CBattlezDlg)
    void VisFn_4fce80();        // 0x4fce80
}

RVA(0x000d9050, 0xc7)
i32 CPlay::NotifyVisibleEntities() {
    CSpriteFactoryHolder* v = m_c;
    i32* vp = (i32*)&v->m_24->m_planeCtx;
    CDDrawSurfacePair* held = v->m_drawTarget->m_14;
    CDDrawGroupNode* node = v->m_childGroup->m_head;

    RECT r;
    r.left = vp[0];
    r.top = vp[1];
    r.right = vp[2] + 1;
    r.bottom = vp[3] + 1;
    held->m_surface->Restore(&r, 0);

    while (node != 0) {
        CGameObject* o = node->m_gameObj;
        void* id = (void*)o->m_7c->m_notify;
        if (id == (void*)VisFn_40fe90 || id == (void*)VisFn_4bf150 || id == (void*)VisFn_423b40
            || id == (void*)VisFn_Roll || id == (void*)VisFn_41e570 || id == (void*)VisFn_41e520
            || id == (void*)VisFn_40fe90 || id == (void*)VisFn_49b410
            || id == (void*)VisFn_IntersectRect || id == (void*)VisFn_49b310
            || id == (void*)VisFn_CBattlezDlg || id == (void*)VisFn_4fce80) {
            o->Draw(held); // slot 11 (+0x2c) - CGameObject's real per-object draw hook
        }
        node = node->m_next;
    }
    return 1;
}

// ===========================================================================
// CPlay::StepScroll - per-frame scroll-offset
// recompute. Reads the draw-surface (m_c->m_24) scroll origin (+0x10/+0x14) and
// its geom block (+0x5c -> +0x40.{x,y}), adds the BeginFrameClear extents
// (m_cursorX/m_cursorY), aligns each axis DOWN to a 0x20 boundary (+0x10 bias) and
// stores the result into the scroll-offset sink m_scrollSink (+0x5c X, +0x60 Y).
// ===========================================================================
// @early-stop
// register-coloring wall (~81.6%). Logic + all member offsets are byte-faithful;
// retail pins the draw-surface ptr in edx and geom in esi, our cl swaps them to
// esi/edx, which renames the temp regs and floats two scheduled adds by one slot.
// Not source-steerable (no local-pin/re-read variant flips the edx<->esi choice);
// confirmed coexists with ApplyGameOptions at top/absent/adjacent placement.
// See docs/patterns/zero-register-pinning.md.
RVA(0x000d1ac0, 0x4f)
void CPlay::StepScroll() {
    CGameLevel* v = m_c->m_24;
    CLevelPlane* geom = v->m_mainPlane;

    i32 y = m_cursorY + (geom->m_originY - v->m_planeCtx.minY); // [edx+4]-m_14; +=m_cursorY
    i32 x = geom->m_originX + (m_cursorX - v->m_planeCtx.minX); // [edx]; +=m_cursorX-m_10

    y = (y & ~0x1f) + 0x10; // align down 0x20 (and al,0xe0); + 0x10
    x = (x & ~0x1f) + 0x10; // align down 0x20 (and edi,~0x1f); + 0x10

    m_scrollSink->m_screenX = x;
    m_scrollSink->m_screenY = y;
}

// ===========================================================================
// CPlay::SetCursorFrame (0x0d1b30; ex "CGameModeObj" view, folded wave3-J) -
// cache the cursor sprite set for `item` and latch it as the active cursor
// frame (m_cursorFrame). Returns 1.
RVA(0x000d1b30, 0x20)
i32 CPlay::SetCursorFrame(i32 item) {
    LoadCursorSprites(item, 0);
    m_cursorFrame = item;
    return 1;
}

// ===========================================================================
// CPlay::StepInputA - per-frame input poll.
// Two boot-time one-shot latches (m_inputWarmup1/m_inputWarmup2 fire on the first two frames),
// then a per-frame edge/axis probe over one of two mirrored half-blocks selected
// by m_inputHalfSel. Probes the draw-surface (m_c->m_4->m_14->m_2c); if absent returns 0.
// On a hit it dispatches the probed control. Returns 1.
// ===========================================================================
// StepInputA is the per-frame CURSOR DRAW: BltFast the selected cursor-half
// surface (m_160/m_164, retyped-at-use CDDSurface*) at the {x,y} edge feed, with
// the half's source RECT (&m_168/&m_178) and colour-key flag 0x10; a nonzero
// DDraw HRESULT goes to CDirectDrawMgr::GetErrorString. (The old __stdcall
// "Eng_InputProbe/Eng_InputDispatch" pair was a fabricated shape.)
RVA(0x000d11e0, 0x9b)
i32 CPlay::StepInputA() {
    if (m_inputWarmup1 == 0) {
        m_inputWarmup1 = 1;
        return 1;
    }
    if (m_inputWarmup2 == 0) {
        m_inputWarmup2 = 1;
        return 1;
    }

    i32 axisVal;
    Edge* edge;
    void* halfPtr;
    if (m_inputHalfSel == 0) {
        axisVal = m_160;
        edge = (Edge*)&m_188;
        halfPtr = &m_168;
    } else {
        axisVal = m_164;
        edge = (Edge*)&m_198;
        halfPtr = &m_178;
    }

    // null-check the draw surface m_c->m_4->m_14->m_2c (walks through the this reg).
    CDDSurface* probeTarget = m_c->m_drawTarget->m_14->m_surface;
    if (probeTarget == 0) {
        return 0;
    }

    i32 r = probeTarget->BltFast(edge->m_0, edge->m_4, (CDDSurface*)axisVal, halfPtr, 0x10);
    if (r != 0) {
        CDirectDrawMgr::GetErrorString(0, 0, r); // 0x141400
    }
    return 1;
}

// ===========================================================================
// CPlay::LoadSBITextEdges (0x0d1710) - draw the status-bar text `name` clamped to
// the viewport rect inset by the "Font" Text{Left,Top,Right,Bottom}Edge margins,
// then arm a 2-frame step countdown. /GX EH frame (the CString local).
// ===========================================================================
// @early-stop
// ~97.8%: every instruction matches except retail reserves a 0x24 local frame vs
// our 0x18, which shifts the [esp+N] displacements by 0xc throughout. The ctor/
// assign/GetInt x4/SetRect/EngStr_DrawText sequence + the single `top` spill are
// byte-identical; the residual is MSVC's total-frame/EH-scratch sizing (tried rect-
// before-CString reorder, no change). Logic byte-faithful.
RVA(0x000d1710, 0x122)
void CPlay::LoadSBITextEdges(char* name) {
    CString s;
    s = name;

    RECT rect;
    LevelCoordRect& vp = m_c->m_24->m_planeCtx;
    i32 l = vp.minX, t = vp.minY, r = vp.maxX, b = vp.maxY;
    i32 bottom = b - g_buteText->GetInt("Font", "TextBottomEdge");
    i32 right = r - g_buteText->GetInt("Font", "TextRightEdge");
    i32 top = t + g_buteText->GetInt("Font", "TextTopEdge");
    i32 left = l + g_buteText->GetInt("Font", "TextLeftEdge");
    SetRect(&rect, left, top, right, bottom);

    EngStr_DrawText((EngStrRenderObj*)m_c, (i32)&s, (i32)&rect, 0x78, 1, 0xff, 0xff, 0, 1);
    m_stepCountdown = 2;
}

// ===========================================================================
// CPlay::PlayCueAt - the ambient/
// positional on-screen text-cue. Args: (cueId, a2, a3, a4, a5, a6, a7, rectSrc).
// De-dupes via the per-cue state object this+0x410 (skip if the same cueId is
// already showing AND its Probe says still-live). Builds the cue RECT from the
// font text-margins (CButeMgr GetInt "Font"/"Text{Left,Top,Right,Bottom}Edge")
// applied to either the caller's rect (rectSrc != 0) or the active viewport
// (this->m_c->m_24+0x10). a3 selects the Top vs Default cue renderer.
// ===========================================================================
RVA(0x000d1890, 0x1ba)
void CPlay::PlayCueAt(i32 cueId, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 rectSrc) {
    RECT rect;

    if (cueId != m_lastCueId) {
        // retail: `lea ecx,[edi+0x410]; call 0x1bedde` - MFC CString::LoadString on
        // m_cueText (the cue TEXT comes from the string table; a missing resource
        // skips the cue). The old "((CPlay*)&m_cueText)->BuildGruntTypeNameTable"
        // 4-arg probe was a fabricated shape.
        if (m_cueText.LoadString(cueId) == 0) {
            return; // no such cue string -> skip
        }
        m_lastCueId = cueId;
    }

    if (rectSrc != 0) {
        i32* src = (i32*)rectSrc;
        i32 bottom = src[3] - g_buteText->GetInt("Font", "TextBottomEdge");
        i32 right = src[2] - g_buteText->GetInt("Font", "TextRightEdge");
        i32 top = src[1] + g_buteText->GetInt("Font", "TextTopEdge");
        i32 left = src[0] + g_buteText->GetInt("Font", "TextLeftEdge");
        SetRect(&rect, left, top, right, bottom);
    } else {
        // the viewport rect (m_c->m_24->m_viewport) ptr (edx) does not survive
        // the GetInt calls, so all 4 corners are read up front.
        LevelCoordRect& vp = m_c->m_24->m_planeCtx;
        i32 l = vp.minX, t = vp.minY, r = vp.maxX, b = vp.maxY;
        i32 bottom = b - g_buteText->GetInt("Font", "TextBottomEdge");
        i32 right = r - g_buteText->GetInt("Font", "TextRightEdge");
        i32 top = t + g_buteText->GetInt("Font", "TextTopEdge");
        i32 left = l + g_buteText->GetInt("Font", "TextLeftEdge");
        SetRect(&rect, left, top, right, bottom);
    }

    if (a3 != 0) {
        ShowHudMessageAlt((HudMsgSink*)m_c, (i32)&m_cueText, (i32)&rect, a2, 1, a4, a5, a6, a7);
    } else {
        EngStr_DrawText((EngStrRenderObj*)m_c, (i32)&m_cueText, (i32)&rect, a2, 1, a4, a5, a6, a7);
    }
}

// ===========================================================================
// CPlay::DrawWorldFrame (0x0c9c20) - one in-game world-draw frame: the begin
// virtual (this->vtbl[+0x98]), the m_c->m_24->m_5c sub-step (if present), mirror
// the draw clock, present (m_c->m_8->vtbl[+0x24]), the m_4->m_68 frame-timer step,
// the optional mode-3 reg cue, then the m_guts guts step.
// ===========================================================================
// @early-stop
// regalloc/scheduling wall — structure byte-identical, only temp-register naming
// (eax vs edx in the pointer-chain derefs) + one m_4/global load-order in the
// Step-call setup differ; see docs/patterns/zero-register-pinning.md.
RVA(0x000c9c20, 0x79)
void CPlay::DrawWorldFrame() {
    Vslot26(); // this->vtbl[+0x98]()  (begin-frame virtual, thiscall)
    if (m_c->m_24->m_mainPlane != 0) {
        ((CPlaneRender*)m_c->m_24->m_mainPlane)->CenterScrollA();
    }
    g_killCueClock = g_645580;
    g_engineFrameDelta = g_frameDelta;
    m_c->m_childGroup->TickKillCues_159a70(0); // m_c->m_8->vtbl[+0x24](0)
    m_4w()->m_68->LoadTeleporterGooConfig(
        (i32)g_frameDelta
    ); // 0x3017 -> 0x6eb80 per-frame grid step
    if (g_gameReg->m_134 == 3) {
        // 0x933e0 == CGruntzMgr::AdvanceOptionsCycle (rel32 via ILT 0x2d33; was the
        // `PerFrameCue` phantom - same object, RTTI-true name).
        ((CGruntzMgr*)g_gameReg)->AdvanceOptionsCycle();
    }
    m_guts->LoadDestructButtonSprite((i32)g_frameDelta); // guts step @0xffb20
}

// ===========================================================================
// CPlay::DrawWorldFrames (0x0c9cc0) - the fixed-step world catch-up loop: divide
// the frame delta by 0x12 (the fixed sub-step), then run that many world-draw
// frames (clamping the first/last sub-step to 0x12), finishing with one tail
// frame-timer step over the full delta.
// ===========================================================================
// @early-stop
// regalloc wall — full control flow + the fixed-substep loop + 3-arg StepFull
// calls byte-structure-identical; MSVC colors now/accum into ebx/ebp swapped vs
// retail and the prologue spill schedule differs. See docs/patterns/zero-register-pinning.md.
RVA(0x000c9cc0, 0x12e)
i32 CPlay::DrawWorldFrames() {
    i32 delta = (i32)g_frameDelta;
    i32 steps = (i32)((u32)delta / FIXED_SUBSTEP_MS); // 0x38e38e39 magic-div by 18
    i32 now = (i32)g_645580;
    i32 accum = (i32)g_frameTime;
    i32 rem = delta - steps * FIXED_SUBSTEP_MS;
    i32 saveDelta = delta; // [esp+0x1c]
    i32 saveAccum = accum; // [esp+0x20]
    i32 saveNow = now;     // [esp+0x24]
    if (rem != 0) {
        steps = steps + 1;
    }
    now -= delta;
    accum -= delta;
    if (steps > 0) {
        i32 last = steps - 1;
        i32 i = 0;
        do {
            i32 dt = (i == last && rem != 0) ? rem : FIXED_SUBSTEP_MS;
            accum += dt;
            now += dt;
            m_4->SetGameClock(
                now,
                dt,
                accum
            ); // 0x3404 -> @0x8f7b0 (retail ecx=m_4; the old m_68 this was fake)
            if (i > 0 && i < last) {
                if (m_c->m_24->m_mainPlane != 0) {
                    ((CPlaneRender*)m_c->m_24->m_mainPlane)->CenterScrollB();
                }
            }
            Vslot26(); // this->vtbl[+0x98]()
            if (m_c->m_24->m_mainPlane != 0) {
                ((CPlaneRender*)m_c->m_24->m_mainPlane)->CenterScrollA();
            }
            m_c->m_childGroup->TickKillCues_159a70(0);
            m_4w()->m_68->LoadTeleporterGooConfig((i32)g_frameDelta);
            if (g_gameReg->m_134 == 3) {
                ((CGruntzMgr*)g_gameReg)->AdvanceOptionsCycle(); // 0x933e0 (was PerFrameCue)
            }
            m_guts->LoadDestructButtonSprite((i32)g_frameDelta);
            i++;
        } while (i < steps);
    }
    m_4->SetGameClock(saveNow, saveDelta, saveAccum); // tail step
    return steps;
}

// ===========================================================================
// The dev frame profiler (CPlay::ProfileDeltaFrame / ProfileInputFrame).
// Instrumented variants of the world-draw + present that bracket each phase with
// the cached timeGetTime fn-ptr (::timeGetTime, pinned in a callee-saved reg)
// and emit a per-phase timing line through the variadic logger ProfLog into the
// brick-text overlay line (g_brickText1 @0x645524, the same CString the debug
// overlay renders and GruntzMgrCmd's cheat handler empties; DEFINED in
// GameText.cpp with the rest of the 0x645514..0x645530 CString run).
// ===========================================================================
extern CString g_brickText1; // 0x645524 (def: GameText.cpp)
extern "C" {
    // The variadic profiler logger (cdecl). 0x1b2cf5.
    void ProfLog(void* sink, const char* fmt, ...);
}
// The two timing accumulators ProfileInputFrame folds the back-half phases into.
extern "C" {}

// The draw-surface flush sink (m_c->m_drawTarget->m_10->m_surface) torn through a thiscall flush.

// ===========================================================================
// CPlay::ProfileDeltaFrame (0x0ca0a0) - the simple profiled frame: run the
// frame-rate split (RenderFast/Slow), world-blit, push-view + present, then log
// "Delta/Update/Draw/NumUpdates", flush, and run the final camera draw-B.
// ===========================================================================
RVA(0x000ca0a0, 0x101)
i32 CPlay::ProfileDeltaFrame() {
    DWORD(WINAPI * tg)(void) = ::timeGetTime;
    i32 updates = 0;
    u32 t0 = tg();
    u32 d = g_frameDelta;
    if (d > 0x12 && d < 0xc8) {
        updates = DrawWorldFrames();
    } else {
        DrawWorldFrame();
    }
    i32 renderMs = (i32)(tg() - t0);
    m_4w()->m_54->Retune( // 0x1a7d -> CWorldSoundSet::Retune (positional audio)
        m_c->m_24->m_mainPlane->m_originX,
        m_c->m_24->m_mainPlane->m_originY
    );
    u32 t2 = tg();
    m_c->m_24->VisitVisible(m_c->m_drawTarget->m_14, (CGameObjChain*)m_c->m_8);
    m_c->m_rendererB->PruneWorkers(m_c->m_drawTarget->m_14, m_c->m_drawTarget->m_18);
    i32 presentMs = (i32)(tg() - t2);
    ProfLog(
        &g_brickText1,
        "Delta=%i, Update=%i, Draw=%i, NumUpdates=%i    ",
        (i32)g_frameDelta,
        renderMs,
        presentMs,
        updates
    );
    DrawDebugStats();
    m_c->m_drawTarget->m_10->m_surface->Flip(0);
    if (m_c->m_24->m_mainPlane != 0) {
        ((CPlaneRender*)m_c->m_24->m_mainPlane)->CenterScrollB();
    }
    return 1;
}

// (The "ProfReport" duplicate decl of UpdateMgrScroll @0xebd70 is gone - one fn,
// one canonical decl, above.)

// The cross-frame profiler accumulators (.bss): owner-TU definitions here; the
// canonical externs are in <Globals.h>. g_profAccB caches the flush start/duration,
// g_profAccA the camera draw-B duration - read next frame at ProfLog time.
extern "C" {
    DATA(0x0024c284)
    i32 g_profAccA;
    DATA(0x0024c288)
    i32 g_profAccB;
}

// ===========================================================================
// CPlay::ProfileInputFrame (0x0c9e40) - the fully-instrumented frame: nine
// timeGetTime-bracketed phases (input/activate/deact/update/hit-test/draw/fixed/
// status-bar) logged in one "Input=.." line, then the flush + camera draw-B whose
// times are stashed in the cross-frame accumulators (g_profAccA/g_profAccB read
// at log time = the PREVIOUS frame's flush/draw-B). __thiscall, ret 0.
// ===========================================================================
// @early-stop
// profiler-scheduling wall: the body is the complete, correct reconstruction (the
// nine phase brackets in order, the BeginScene(1)/m_68->Step/m_guts->Step update
// block, the PushView/Present draw block, the m_guts status-bar tick, the 11-arg
// ProfLog with the cross-frame g_profAccA/g_profAccB accumulators, then the timed
// flush + draw-B writing those globals for next frame, and the ProfReport tail).
// MSVC pins the ::timeGetTime fn-ptr in esi across all 14 calls as retail does,
// but the seven live phase-times spill to a different set of stack slots / the
// ebx/ebp coloring of deact/update differs, so the slot-reuse schedule diverges
// despite identical logic. Same idiom as ProfileDeltaFrame (byte-exact); the extra
// phases push it onto the documented register/stack-scheduling plateau. Deferred
// to the final sweep. docs/patterns/zero-register-pinning.md.
RVA(0x000c9e40, 0x1d7)
i32 CPlay::ProfileInputFrame() {
    m_4w()->m_54->Retune( // 0x1a7d -> CWorldSoundSet::Retune (positional audio)
        m_c->m_24->m_mainPlane->m_originX,
        m_c->m_24->m_mainPlane->m_originY
    ); // untimed
    DWORD(WINAPI * tg)(void) = ::timeGetTime;

    u32 t1 = tg();
    Vslot26(); // this->vtbl[+0x98]
    i32 activateMs = (i32)(tg() - t1);

    u32 t3 = tg();
    if (m_c->m_24->m_mainPlane != 0) {
        ((CPlaneRender*)m_c->m_24->m_mainPlane)->CenterScrollA();
    }
    i32 deactMs = (i32)(tg() - t3);

    u32 t5 = tg();
    m_c->m_childGroup->TickKillCues_159a70(1);
    m_4w()->m_68->LoadTeleporterGooConfig((i32)g_frameDelta);
    m_guts->LoadDestructButtonSprite((i32)g_frameDelta);
    i32 updateMs = (i32)(tg() - t5);

    u32 t7 = tg();
    i32 hitTestMs = (i32)(tg() - t7);

    u32 t9 = tg();
    m_c->m_24->VisitVisible(m_c->m_drawTarget->m_14, (CGameObjChain*)m_c->m_8);
    i32 drawMs = (i32)(tg() - t9);

    u32 t11 = tg();
    m_c->m_rendererB->PruneWorkers(m_c->m_drawTarget->m_14, m_c->m_drawTarget->m_18);
    i32 fixedMs = (i32)(tg() - t11);

    u32 t13 = tg();
    m_guts->LoadMainStatusBarSprite(); // 0xfe6b0
    i32 statusBarMs = (i32)(tg() - t13);

    ProfLog(
        &g_brickText1,
        "Input=%i, Activate=%i, Deact=%i, Update=%i, HitTest=%i, Draw=%i, Fixed=%i, "
        "StatusBar=%i, Flip=%i  ",
        activateMs,
        deactMs,
        g_profAccA,
        updateMs,
        hitTestMs,
        drawMs,
        fixedMs,
        statusBarMs,
        g_profAccB
    );

    DrawDebugStats();
    g_profAccB = (i32)tg();
    m_c->m_drawTarget->m_10->m_surface->Flip(0);
    g_profAccB = (i32)(tg() - (u32)g_profAccB);
    g_profAccA = (i32)tg();
    if (m_c->m_24->m_mainPlane != 0) {
        ((CPlaneRender*)m_c->m_24->m_mainPlane)->CenterScrollB();
    }
    g_profAccA = (i32)(tg() - (u32)g_profAccA);
    UpdateMgrScroll((CGruntzMgr*)g_gameReg, (i32*)m_guts, m_region0Gate); // 0xebd70
    return 1;
}

// ===========================================================================
// CPlay::ResetGoals (0x0d5f00) - clear the world goal object's pending bit
// (m_4->m_68->m_23c, OR 0x10000 into +0x8), reset the substep gate (m_68->m_armed),
// then recompute the plane geom (m_4->m_30->m_24->m_5c) from the (x,y) args:
// store them as floats, scaling by the geom's +0x18/+0x1c factors unless bit0 of
// +0x8 is set, then call RecomputePlaneCoords.
// ===========================================================================
RVA(0x000d5f00, 0x69)
i32 CPlay::ResetGoals(i32 x, i32 y) {
    CWorld* w = m_4w();
    CTriggerMgr* g = w->m_68;
    if (g->m_goal != 0) {
        g->m_goal->m_8 |= 0x10000;
        g->m_goal = 0;
    }
    g->m_armed = 0;
    CLevelPlane* pg = m_4w()->m_30->m_24->m_mainPlane;
    if ((pg->m_flags & 1) == 0) {
        pg->m_scaledX = (float)x * pg->m_scaleX;
        pg->m_scaledY = (float)y * pg->m_scaleY;
    } else {
        pg->m_scaledX = (float)x;
        pg->m_scaledY = (float)y;
    }
    pg->RecomputePlaneCoords();
    return 1;
}

// ===========================================================================
// CPlay::RegisterInputBindings (0x0d9160) - register the nine keyboard control
// bindings (the WM_KEYDOWN-style codes 0x100-0x102 / 0x200-0x206, each with flag
// 0x40) on the world input dispatcher (m_4->m_4).
// ===========================================================================
RVA(0x000d9160, 0xac)
i32 CPlay::RegisterInputBindings() {
    m_4w()->Wnd()->PumpMessages(0x102, 0x40);
    m_4w()->Wnd()->PumpMessages(0x100, 0x40);
    m_4w()->Wnd()->PumpMessages(0x200, 0x40);
    m_4w()->Wnd()->PumpMessages(0x201, 0x40);
    m_4w()->Wnd()->PumpMessages(0x202, 0x40);
    m_4w()->Wnd()->PumpMessages(0x203, 0x40);
    m_4w()->Wnd()->PumpMessages(0x204, 0x40);
    m_4w()->Wnd()->PumpMessages(0x205, 0x40);
    m_4w()->Wnd()->PumpMessages(0x206, 0x40);
    return 1;
}

// CPlay::ArmSnapshot (0x0d9240) - thiscall(active, dur). When `active`, latch the
// snapshot duration (dur) and base clock (g_frameTime) 64-bit timers; always store
// `active` into m_snapshotActive. Migrated from engine_boundary (CPlay).
// @early-stop
// scheduling wall (99.2%): logic + regalloc byte-exact except the two independent
// 64-bit-base stores (m_snapBaseLo/m_snapBaseHi) emit in hi,lo order where retail
// emits lo,hi; cl fills the g_frameTime load-use latency gap with the hi=0 store.
// Loading the clock into a local forces lo,hi but diverges the whole regalloc
// (push edi / immediates) to 62% — not source-steerable.
RVA(0x000d9240, 0x3c)
i32 CPlay::ArmSnapshot(i32 active, i32 dur) {
    if (active != 0) {
        m_snapDur = dur;
        m_snapDurHi = 0;
        m_snapBaseLo = g_frameTime;
        m_snapBaseHi = 0;
    }
    m_snapshotActive = active;
    return 1;
}

// The placed map object record (the m_3a4[] element): m_0/m_4 are its grid (x,y)
// cell coordinates. @identity-TODO (likely the placed CGameObject's coord header;
// the +0x124 type-tag probe below reads the LOOKED-UP object, not this record).
// (CPlacedObj is GONE - the m_3a4[] placed-object record is the same 2-int
// coord-node payload as CHitMarker (both come off g_coordPool); one shared type.)
// ===========================================================================
// CPlay::DrawLevelInfoText (0x0d95f0; ex the `GruntInfoTextHost` placeholder view
// - its +0x0c "render surface" is CState::m_c, +0x1c is m_levelIndex and +0x20 is
// m_levelType, all written by LoadByMode on this same `this`). The full-screen
// level/grunt info-text panel painter; four CString locals -> /GX frame.
// ===========================================================================

// @early-stop
// Logic + control-flow byte-exact through the whole switch/mode/QueryLevelName
// chain (all if/else fall-through polarities matched); residual ~11% is a
// register-allocation rotation in the tail SetRect/EngStr_DrawText render block:
// retail threads the RECT/CString/m_c temps through eax/ecx/edx where cl picks
// edx/eax/ecx (a consistent 3-register rotation, identical push order + opcodes,
// only the ModRM reg field differs), plus the basename ptr pushed in-branch vs
// post-merge. Pure allocator coin-flip; no source spelling flips the rotation.
// The 3 `jmpl *table(,%eax,4)` rows are jump-table DIR32 displacement artifacts
// (code bytes identical, reloc-masked). Verified base-vs-target with llvm-objdump -dr.
// The 8 Gruntz worlds (m_20, 1-based); the line-1 name is LoadString'd from the
// retail STRINGTABLE (ids 0x81ae..0x81b5, texts recovered from GRUNTZ.EXE .rsrc).
// Same immediates as the bare labels -> naming is matching-neutral.
enum World {
    WORLD_ROCKY_ROADZ = 1,       // "Rocky Roadz"
    WORLD_GRUNTZICLEZ = 2,       // "Gruntziclez"
    WORLD_TROPICZ = 3,           // "Trouble in the Tropicz"
    WORLD_HIGH_ON_SWEETZ = 4,    // "High on Sweetz"
    WORLD_HIGH_ROLLERZ = 5,      // "High Rollerz"
    WORLD_HONEY_I_SHRUNK = 6,    // "Honey, I Shrunk the Gruntz!"
    WORLD_MINIATURE_MASTERZ = 7, // "The Miniature Masterz"
    WORLD_GRUNTZ_IN_SPACE = 8,   // "Gruntz in Space"
};

RVA(0x000d95f0, 0x756)
i32 CPlay::DrawLevelInfoText() {
    CString s0; // line 1: current World name (LoadString)
    CString s1; // stage/status    (line 2)
    CString s2; // grunt-type name / level basename (line 3)
    CString s3; // footer          (line 4)

    switch (m_levelType) {
        case WORLD_ROCKY_ROADZ:
            s0.LoadString(0x81ae);
            break;
        case WORLD_GRUNTZICLEZ:
            s0.LoadString(0x81af);
            break;
        case WORLD_TROPICZ:
            s0.LoadString(0x81b0);
            break;
        case WORLD_HIGH_ON_SWEETZ:
            s0.LoadString(0x81b1);
            break;
        case WORLD_HIGH_ROLLERZ:
            s0.LoadString(0x81b2);
            break;
        case WORLD_HONEY_I_SHRUNK:
            s0.LoadString(0x81b3);
            break;
        case WORLD_MINIATURE_MASTERZ:
            s0.LoadString(0x81b4);
            break;
        case WORLD_GRUNTZ_IN_SPACE:
            s0.LoadString(0x81b5);
            break;
        default:
            s0 = g_emptyString;
    }

    i32 mode = g_gameReg->m_134;
    if (mode == 1) {
        if (g_gameReg->m_130 != 0) {
            s1.LoadString(0x81a0);
        } else {
            i32 stage = m_levelIndex;
            if (stage > 0x24) {
                switch (stage) {
                    case 0x25:
                        s1.LoadString(0x81a2);
                        break;
                    case 0x26:
                        s1.LoadString(0x81a3);
                        break;
                    case 0x27:
                        s1.LoadString(0x81a4);
                        break;
                    case 0x28:
                        s1.LoadString(0x81a5);
                        break;
                    default:
                        s1 = g_emptyString;
                }
            } else {
                s1.Format("Stage %d", ((stage - 1) % 4) + 1);
            }
            switch (m_levelIndex) {
                case 1:
                    s2.LoadString(0x8177);
                    break;
                case 2:
                    s2.LoadString(0x8178);
                    break;
                case 3:
                    s2.LoadString(0x8179);
                    break;
                case 4:
                    s2.LoadString(0x817a);
                    break;
                case 5:
                    s2.LoadString(0x817b);
                    break;
                case 6:
                    s2.LoadString(0x817c);
                    break;
                case 7:
                    s2.LoadString(0x817d);
                    break;
                case 8:
                    s2.LoadString(0x817e);
                    break;
                case 9:
                    s2.LoadString(0x817f);
                    break;
                case 10:
                    s2.LoadString(0x8180);
                    break;
                case 0xb:
                    s2.LoadString(0x8181);
                    break;
                case 0xc:
                    s2.LoadString(0x8182);
                    break;
                case 0xd:
                    s2.LoadString(0x8183);
                    break;
                case 0xe:
                    s2.LoadString(0x8184);
                    break;
                case 0xf:
                    s2.LoadString(0x8185);
                    break;
                case 0x10:
                    s2.LoadString(0x8186);
                    break;
                case 0x11:
                    s2.LoadString(0x8187);
                    break;
                case 0x12:
                    s2.LoadString(0x8188);
                    break;
                case 0x13:
                    s2.LoadString(0x8189);
                    break;
                case 0x14:
                    s2.LoadString(0x818a);
                    break;
                case 0x15:
                    s2.LoadString(0x818b);
                    break;
                case 0x16:
                    s2.LoadString(0x818c);
                    break;
                case 0x17:
                    s2.LoadString(0x818d);
                    break;
                case 0x18:
                    s2.LoadString(0x818e);
                    break;
                case 0x19:
                    s2.LoadString(0x818f);
                    break;
                case 0x1a:
                    s2.LoadString(0x8190);
                    break;
                case 0x1b:
                    s2.LoadString(0x8191);
                    break;
                case 0x1c:
                    s2.LoadString(0x8192);
                    break;
                case 0x1d:
                    s2.LoadString(0x8193);
                    break;
                case 0x1e:
                    s2.LoadString(0x8194);
                    break;
                case 0x1f:
                    s2.LoadString(0x8195);
                    break;
                case 0x20:
                    s2.LoadString(0x8196);
                    break;
                default:
                    s2.Format(g_emptyString);
                    break;
                case 0x25:
                    s2.LoadString(0x8197);
                    break;
                case 0x26:
                    s2.LoadString(0x8198);
                    break;
                case 0x27:
                    s2.LoadString(0x8199);
                    break;
                case 0x28:
                    s2.LoadString(0x819a);
            }
            if (g_6455f0 != 0) {
                s1.LoadString(0x81ac);
                s2.LoadString(0x81ad);
            }
        }
    } else if (mode == 3) {
        if (g_gameReg->m_130 != 0) {
            s1.LoadString(0x819f);
        } else {
            s1.LoadString(0x819e);
        }
    } else if (mode == 2) {
        if (g_gameReg->m_130 != 0) {
            s1.LoadString(0x819d);
        } else {
            s1.LoadString(0x819c);
        }
    } else {
        s0.Format(g_emptyString);
        s2.Format(g_emptyString);
        s1.Format(g_emptyString);
    }

    if (((CGruntzMgr*)g_gameReg)->GetWorldFileName().GetLength() != 0) {
        char buf[128];
        wsprintfA(buf, ((CGruntzMgr*)g_gameReg)->GetWorldFileName());
        if (strchr(buf, '.')) {
            *strchr(buf, '.') = 0;
        }
        char* base;
        if (strrchr(buf, '\\') != 0) {
            base = strrchr(buf, '\\') + 1;
        } else {
            base = buf;
        }
        s2 = base;
    }

    s3.LoadString(0x819b);

    RECT r1;
    RECT r2;
    RECT r3;
    RECT r4;
    SetRect(&r1, 0, 0, 0x280, 0x38);
    SetRect(&r2, 0, 0x2b, 0x280, 0x59);
    SetRect(&r3, 0, 0x176, 0x280, 0x1a2);
    SetRect(&r4, 0, 0x1b8, 0x280, 0x1e0);
    EngStr_DrawText((EngStrRenderObj*)m_c, (i32)&s0, (i32)&r1, 0x78, 0, 0, 0, 0, 1);
    EngStr_DrawText((EngStrRenderObj*)m_c, (i32)&s1, (i32)&r2, 0x6e, 0, 0, 0, 0, 1);
    EngStr_DrawText((EngStrRenderObj*)m_c, (i32)&s2, (i32)&r3, 0x6e, 0, 0, 0, 0, 1);
    EngStr_DrawText((EngStrRenderObj*)m_c, (i32)&s3, (i32)&r4, 0x6e, 0, 0, 0, 0, 1);
    return 1;
}

// ===========================================================================
// @early-stop
// 361-B map-grid free-list walk. The control flow (the 4 placed-object arrays at
// +0x3a4, the per-array element walk with the restart flag, the grid (x,y)
// occupant lookup with the x*7-dword cell stride, the cell-map lookup, the
// cell-flag clear + CPtrArray::RemoveAt + free-list node return) is a faithful
// reconstruction of retail's logic. The residual is the dual-exit regalloc/
// scheduling wall: MSVC5 pins blockIdx/ebx/edi/esi across the nested loops and
// threads the restart `edi` flag through the block advance in a way the C source
// can't steer (zero-register-pinning.md). Deferred to the final sweep.
//
// CPlay::ClearPlacedObjects (0x0da030; ex "CGameModeObj" view, folded wave3-J) -
// sweep the 4 placed-object arrays (m_3a4); for each still-occupied grid cell
// whose map entry is gone (or not type 0x14), clear the cell, remove the object
// from its array (RemoveAt on the ARRAY base - retail lea ecx,[this+idx*0x14+
// 0x3a4]), and free the node. Returns the array index on an early-out, else -1.
RVA(0x000da030, 0x169)
i32 CPlay::ClearPlacedObjects() {
    for (i32 blockIdx = 0; blockIdx < 4; ++blockIdx) {
        CPtrArray* rec = &m_3a4[blockIdx];
        i32 i = 0;
        i32 restart = 0;
        while (i < rec->GetSize()) {
            CHitMarker* obj = (CHitMarker*)rec->GetAt(i);
            CTileGrid* grid = g_gameReg->m_tileGrid;
            i32* cellObj = 0;
            if ((u32)obj->m_0 < (u32)grid->m_c && (u32)obj->m_4 < (u32)grid->m_10) {
                i32 stride = obj->m_0 * 7;
                i32* row = grid->m_8[obj->m_4];
                cellObj = (i32*)row[stride + 2];
            }
            if (cellObj == 0) {
                restart = 1;
                break;
            }
            void* out = 0;
            // The factory's embedded +0x48 key->object map, reached as the real
            // MFC CMapPtrToPtr (the documented SpriteFactory.h consumer pattern).
            CMapPtrToPtr* map = (CMapPtrToPtr*)&g_gameReg->m_world->m_8->m_objMap;
            i32* result = cellObj;
            if (map->Lookup(cellObj, out)) {
                result = (i32*)out;
            }
            if (result == 0) {
                // cell vacated: clear the cell's occupant + flag bit and unlink.
                CTileGrid* g = g_gameReg->m_tileGrid;
                if ((u32)obj->m_0 < (u32)g->m_c && (u32)obj->m_4 < (u32)g->m_10) {
                    i32 stride = obj->m_0 * 7;
                    i32* row = g->m_8[obj->m_4];
                    row[stride + 2] = 0;
                    i32* row2 = g->m_8[obj->m_4];
                    row2[stride] &= 0xfffbffff;
                }
                rec->RemoveAt(i, 1);
                // return the placed-object node to the MFC free list (the node
                // header sits g_coordPool.m_linkOffset bytes before the payload).
                void* node = (char*)obj - g_coordPool.m_linkOffset;
                *(void**)node = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = node;
                return -1;
            }
            if (*(i32*)((char*)result + 0x124) != 0x14) {
                restart = 1;
            }
            ++i;
            if (restart) {
                break;
            }
        }
        if (i >= rec->GetSize() && !restart) {
            if (i > 0) {
                return blockIdx;
            }
            restart = 1;
        }
        (void)restart;
    }
    return -1;
}

// ===========================================================================
// CPlay::FlushPendingOps (0x0da2d0; ex "CGameModeObj" view, folded wave3-J) - if
// the highlight-busy gate (m_4f0) is clear, flush the two deferred guts ops gated
// by m_dragInhibit1/m_dragInhibit2 (each running a guts step + a cursor-frame
// reset), then clear the trigger grid's pending overlay-fx kind and reset the
// drag boxes. Returns 1 if any of the three was pending, else 0.
// The +0x2dc guts object is reached as CStatusBarMgr for CommitSlot/EnterHlRow
// (the GutsSubsystem view's CommitSlot142e/EnterHlRow213f thunk names mirror
// exactly these CStatusBarMgr methods - the guts==CStatusBarMgr unification is
// a flagged reconciliation TODO).
RVA(0x000da2d0, 0xa5)
i32 CPlay::FlushPendingOps() {
    if (m_4f0 != 0) {
        return 0;
    }
    i32 changed = 0;
    if (m_dragInhibit1 != 0) {
        CStatusBarMgr* worker = (CStatusBarMgr*)m_guts;
        m_dragInhibit1 = 0;
        worker->CommitSlot(0);
        SetCursorFrame(0); // via the 0x17a8 ILT thunk in retail
        changed = 1;
    }
    if (m_dragInhibit2 != 0) {
        i32 spr = m_cursorFrame;
        CStatusBarMgr* worker = (CStatusBarMgr*)m_guts;
        m_dragInhibit2 = 0;
        worker->EnterHlRow(0, spr);
        SetCursorFrame(0);
        changed = 1;
    }
    CTriggerMgr* fx = g_gameReg->m_cmdGrid;
    if (fx->m_pendingFxKind != 0) {
        changed = 1;
    }
    fx->m_pendingFxKind = 0;
    LoadCursorSprites(0, 0); // CPlay @0xd0120 (via the 0x35da ILT thunk in retail)
    return changed;
}

// CPlay::CanQuickSave (0x0da3b0) - all-idle predicate: returns 1 only when the
// render is enabled, not in a main frame, no overlay-drag, no active snapshot, the
// guts subsystem is idle (m_548/m_busyA/m_busyB all 0), the registry has no active
// selection (reg->m_c), and the cue-sink B busy gate is set. Migrated from
// engine_boundary (CPlay).
RVA(0x000da3b0, 0x6e)
i32 CPlay::CanQuickSave() {
    if (m_renderDisabled == 0 && m_inGame == 0 && m_overlayDrag == 0 && m_snapshotActive == 0
        && m_guts->m_hlBusy == 0 && m_guts->m_toggleActive == 0 && m_guts->m_toggleHandle == 0
        && g_gameReg->m_frameGate == 0 && g_gameReg->m_cmdGrid->m_groupFlag != 0) {
        return 1;
    }
    return 0;
}

// CPlay::PostHudRect (0x0da440) - if the world is ready, post the HUD/selection
// rect (by value, with the dev-state 0x20 flag) to the world timeline, then clear
// the ready / drag-snap gates. Migrated from engine_boundary (CPlay).
RVA(0x000da440, 0x60)
i32 CPlay::PostHudRect() {
    if (m_worldReady != 0) {
        m_4w()->m_68->HudRect(m_hudRect, g_spawnConfig->m_18 & 0x20);
    }
    m_worldReady = 0;
    m_dragSnapActive = 0;
    return 1;
}

// ===========================================================================
// GruntzPlayer - the per-player options/state record (CGruntzMgr +0x150 slot
// array; <Gruntz/GruntzPlayer.h>). Its whole TU (ex GruntzPlayer.cpp) + the
// channel-slot free functions (ex src/Net/ChannelSlots.cpp) fold into this obj
// per the 0x0d5960 dossier: the play+channelslots+gruntzplayer+
// gamemodeobjlifecycle interval is ONE original TU (one 20-frag init run; the
// channelslots frag sits INSIDE the play run). RVA-ascending: 0xda870-0xdb31b.
// ===========================================================================

// Per-serialize round counter the CString archive helpers bump (g_serialCounter,
// = ?g_serialCounter@@3HA @0x229ad0). Reloc-masked DATA.
extern i32 g_serialCounter;

// The MFC empty C string (the afxEmptyString data buffer @0x6293f4); the name
// CString members default-init to it. Reloc-masked DATA (also declared by the
// later render-tail section - same extern).
extern "C" char g_emptyString[]; // 0x6293f4
#include <string.h>              // inlined memset / strcpy in Serialize (rep stos / rep movs)

// The dialog-combobox fillers below are __cdecl FREE functions physically in this
// TU; they call the same-TU CString accessors (defined further down, so forward-
// declared here) and reach USER32 through the game's cached fn-ptr globals.
CString GetColorName(i32 colorIdx, i32 upper);
CString GetDifficultyName(i32 diffIdx, i32 upper);

// ===========================================================================
// GruntzPlayer::GruntzPlayer(i32)  @0x0da870  (/GX EH frame)
// Seed the name with "Player" (the temp from GetDefaultName() assigned into the
// member) and the scalar config block from the index. The /GX frame comes from
// the destructible "Player" temp.
// ===========================================================================
// @early-stop
// MFC-version wall (cstring-empty-init-version-divergence.md): the field stores +
// GetDefaultName call + op= are byte-correct, but the packaged MFC's
// CString::CString() is out-of-line, so our base emits an extra ??0CString@@QAE@XZ
// default-ctor call before the m_name = g_emptyString op= where retail elides it
// (its inline CString() folds + dead-store-eliminates). Retail also reads fs:0
// before push -1 in the /GX prologue where ours pushes first. Both are static-MFC
// build artifacts, not source-steerable. Deferred to the final sweep.
RVA(0x000da870, 0xb8)
GruntzPlayer::GruntzPlayer(i32 index) {
    m_name = g_emptyString;
    m_playerIndex = index;
    m_018 = -2;
    m_020 = 0;
    m_028 = 0;
    m_014 = 1;
    m_name = GetDefaultName();
    m_008 = index;
    m_010 = 0;
    m_220 = 0;
    m_224 = 0;
    m_comboSel = 0xf;
    m_02c = 0;
    m_030 = 0;
    m_22c = 0;
    m_230 = 0;
}

// ===========================================================================
// GruntzPlayer::Clear  @0x0da960
// The frameless field-seed helper: re-empty the name CString (op=, the member is
// already constructed) and re-seed the scalar block (-1 / 1 / -2 / 0xf sentinels).
// The default ctor (0x0da790, GruntSpawnLevel.cpp) and the dtor (0x083260) both
// call it after / before the member ctor/dtor run.
//
// THIS IS NOT A CONSTRUCTOR. It was bound as ??0GruntzPlayer@@QAE@XZ, and that
// mis-binding WAS the documented "MFC-version wall": modeled as a ctor, cl has to
// default-construct the CString member first, which drags in the out-of-line
// ??0CString + a /GX EH frame that retail's 0x5b body does not have. As a plain
// method the member is already live, no CString() is emitted, and the frame is gone.
// The real default ctor is 0x0da790 - it constructs m_name AND the m_038
// CBattlezMapConfig (0x24dc0), which a 0x5b frameless body plainly cannot do.
//
// The m_018/m_020/m_014 seeds are written BEFORE the m_name op= call (source order =
// retail): that pins the zero in callee-saved edi (surviving the call) as retail does,
// which lifted this 52->94.65% (writing them AFTER the call let cl use a caller-saved
// zero + a different frame).
// @early-stop
// immediate-float scheduling wall (94.65%, identical to the Reset sibling @0x0da9e0):
// the sole residual is the `m_comboSel = 0xf` (m_228) IMMEDIATE store - MSVC floats it to
// the tail of the store cluster where retail keeps it in source position (between m_224
// and m_02c). Reordering / hoisting to a local does not flip it (the scheduler re-floats
// the imm - proven on the sibling).
// ===========================================================================
RVA(0x000da960, 0x5b)
void GruntzPlayer::Clear() {
    m_playerIndex = -1;
    m_018 = -2;
    m_020 = 0;
    m_014 = 1;
    m_name = g_emptyString;
    m_008 = 0;
    m_010 = 0;
    m_220 = 0;
    m_224 = 0;
    m_comboSel = 0xf;
    m_02c = 0;
    m_030 = 0;
    m_22c = 0;
    m_230 = 0;
}

// ===========================================================================
// GruntzPlayer::Reset  @0x0da9e0
// Frameless re-init of an already-live slot: re-empty the name CString (op= to
// the MFC empty string, NO default ctor / no EH frame since the member is already
// constructed) and re-seed the scalar config block. Returns 1 (success).
// ===========================================================================
// @early-stop
// store-scheduling wall: every instruction (the op= to g_emptyString + all 14 field
// stores) is byte-correct, but MSVC's scheduler floats the m_228 = 0xf IMMEDIATE
// store to the tail of the register-store cluster, where retail keeps it in source
// position (between m_224 and m_02c). Reordering the source / hoisting to a local
// does not flip it (the scheduler re-floats the imm). ~94.9%.
RVA(0x000da9e0, 0x60)
i32 GruntzPlayer::Reset() {
    m_playerIndex = -1;
    m_018 = -2;
    m_020 = 0;
    m_014 = 1;
    m_name = g_emptyString;
    m_008 = 0;
    m_010 = 0;
    m_220 = 0;
    m_224 = 0;
    m_comboSel = 0xf;
    m_02c = 0;
    m_030 = 0;
    m_22c = 0;
    m_230 = 0;
    return 1;
}

// ===========================================================================
// GruntzPlayer::ClearRoundState  @0x0daa60
// Frameless: mark the slot active (m_020 = 1) and clear the per-round scalar
// counters (m_01c / m_02c / m_030 / m_22c / m_230). Returns 1.
// ===========================================================================
RVA(0x000daa60, 0x24)
i32 GruntzPlayer::ClearRoundState_0daa60() {
    m_020 = 1;
    m_01c = 0;
    m_02c = 0;
    m_030 = 0;
    m_22c = 0;
    m_230 = 0;
    return 1;
}

// ===========================================================================
// FillColorCombo  @0x0daaa0  (/GX EH frame, free fn)
// Reset the combobox (dialog item `nID` on `hDlg`) and fill it with all 17 color
// names; select `curSel` when non-negative. Returns 0 if the dialog/item is
// missing, else 1.
// ===========================================================================
RVA(0x000daaa0, 0xd3)
i32 FillColorCombo(HWND hDlg, i32 nID, i32 curSel) {
    if (hDlg == 0) {
        return 0;
    }
    HWND cb = ::GetDlgItem(hDlg, nID);
    if (cb == 0) {
        return 0;
    }
    LRESULT(WINAPI * pSend)(HWND, UINT, WPARAM, LPARAM) = ::SendMessageA;
    pSend(cb, 0x14b, 0, 0);
    for (i32 i = 0; i < 0x11; i++) {
        CString s = GetColorName(i, 0);
        pSend(cb, 0x143, 0, (i32)(const char*)s);
    }
    if (curSel >= 0) {
        pSend(cb, 0x14e, curSel, 0);
    }
    return 1;
}

// ===========================================================================
// FillDifficultyCombo  @0x0dabc0 - twin of FillColorCombo over the 3 difficulty names.
// ===========================================================================
RVA(0x000dabc0, 0xd3)
i32 FillDifficultyCombo(HWND hDlg, i32 nID, i32 curSel) {
    if (hDlg == 0) {
        return 0;
    }
    HWND cb = ::GetDlgItem(hDlg, nID);
    if (cb == 0) {
        return 0;
    }
    LRESULT(WINAPI * pSend)(HWND, UINT, WPARAM, LPARAM) = ::SendMessageA;
    pSend(cb, 0x14b, 0, 0);
    for (i32 i = 0; i < 3; i++) {
        CString s = GetDifficultyName(i, 0);
        pSend(cb, 0x143, 0, (i32)(const char*)s);
    }
    if (curSel >= 0) {
        pSend(cb, 0x14e, curSel, 0);
    }
    return 1;
}

// ===========================================================================
// GruntzPlayer::Serialize  @0x0dace0
// Stream every field through the archive order object. kind 7 = Load (read each
// scalar via [+0x2c], then load the 0x80 name buffer and assign it into m_name),
// kind 4 = Save (write each scalar via [+0x30], then the inlined memset+strcpy of
// the name into a 0x80 buffer and write it). Either way, forward the 4-arg
// command to the +0x38 config bundle and negate (!!) its result.
// ===========================================================================
RVA(0x000dace0, 0x239)
i32 GruntzPlayer::Serialize(void* arArg, i32 kind, i32 a3, i32 a4) {
    CSerialArchive* ar = (CSerialArchive*)arArg;
    char tmp[0x80];
    // Retail lays the kind==4 (Save, [+0x30]) arm out of line and keeps the
    // kind==7 (Load, [+0x2c]) arm inline: `cmp 4; je SAVE / cmp 7; jne TAIL`.
    if (kind != 4) {
        if (kind == 7) {
            // Load.
            ar->Read(&m_playerIndex, 4);
            ar->Read(&m_008, 4);
            ar->Read(&m_00c, 4);
            ar->Read(&m_010, 4);
            ar->Read(&m_014, 4);
            ar->Read(&m_018, 4);
            ar->Read(&m_01c, 4);
            ar->Read(&m_020, 4);
            ar->Read(&m_028, 4);
            ar->Read(&m_024, 4);
            g_serialCounter++;
            ar->Read(tmp, 0x80);
            m_name = tmp;
            ar->Read(&m_220, 4);
            ar->Read(&m_224, 4);
            ar->Read(&m_comboSel, 4);
        }
    } else {
        // Save.
        ar->Write(&m_playerIndex, 4);
        ar->Write(&m_008, 4);
        ar->Write(&m_00c, 4);
        ar->Write(&m_010, 4);
        ar->Write(&m_014, 4);
        ar->Write(&m_018, 4);
        ar->Write(&m_01c, 4);
        ar->Write(&m_020, 4);
        ar->Write(&m_028, 4);
        ar->Write(&m_024, 4);
        g_serialCounter++;
        memset(tmp, 0, sizeof(tmp));
        strcpy(tmp, (const char*)m_name);
        ar->Write(tmp, 0x80);
        ar->Write(&m_220, 4);
        ar->Write(&m_224, 4);
        ar->Write(&m_comboSel, 4);
    }
    return ((CBattlezMapConfig*)&m_038)->Method_02bfc0((i32)ar, (void*)kind, a3, a4) != 0;
}

// ===========================================================================
// GruntzPlayer::GetDefaultName  @0x0dafb0  (/GX EH frame, static)
// Return the literal default player name "Player" by value (NRV); the /GX frame
// comes from the destructible CString("Player") temp.
// ===========================================================================
RVA(0x000dafb0, 0x71)
CString GruntzPlayer::GetDefaultName() {
    // Retail builds a named local temp, then NRV-copies it into the return slot
    // (op= copy-ctor) and destructs the temp -> the /GX frame. A direct
    // `return CString("Player");` would NRV-construct in place (frameless).
    CString name("Player");
    return name;
}

// The color / difficulty name tables (.data pointer arrays -> the "Color N" /
// "Easy"/"Normal"/"Hard" string literals). Owner-TU definitions here; canonical
// externs in <Globals.h>. The pointer slots reloc-mask; the strings match by content.
DATA(0x00212f78)
char* g_colorNames[] =
    {"Color 0", "Color 1", "Color 2", "Color 3", "Color 4", "Color 5", "Color 6", "Color 7"};
DATA(0x00212fc0)
char* g_difficultyNames[] = {"Easy", "Normal", "Hard"};

// ===========================================================================
// GetColorName  @0x0db050  (/GX EH frame, free fn)
// Build a CString from the color-name table (g_colorNames[idx]); uppercase it
// when `upper` is set, then return by value (NRV: the local is copy-ctor'd into
// the caller's return slot and destructed).
// ===========================================================================
RVA(0x000db050, 0x90)
CString GetColorName(i32 colorIdx, i32 upper) {
    CString s;
    s = g_colorNames[colorIdx];
    if (upper) {
        s.MakeUpper();
    }
    return s;
}

// ===========================================================================
// GetDifficultyName  @0x0db110  (/GX EH frame, free fn)
// As GetColorName but over the difficulty table ("Easy"/"Normal"/"Hard").
// ===========================================================================
RVA(0x000db110, 0x90)
CString GetDifficultyName(i32 diffIdx, i32 upper) {
    CString s;
    s = g_difficultyNames[diffIdx];
    if (upper) {
        s.MakeUpper();
    }
    return s;
}

// ===========================================================================
// The global channel-slot in-use table (g_64c3f0[17]) and its free-function
// accessors (ex src/Net/ChannelSlots.cpp; the channelslots init frag sits INSIDE
// this TU's frag run - one obj). A 17-DWORD array of per-channel "free" flags
// (1 = free, 0 = taken); CNetMgr and CGruntzMgr init/scan/claim channels through
// these. Zero-init (.bss) storage, runtime-reset to 1 by ChannelSlots_InitAll.
// ===========================================================================

// Owner-TU definition of the channel-slot table (canonical extern in <Globals.h>).
extern "C" {
    DATA(0x0024c3f0)
    i32 g_64c3f0[17];
}

// Reset every slot to "free" (1).
RVA(0x000db1d0, 0x14)
void ChannelSlots_InitAll() {
    for (i32 i = 0; i < 17; i++) {
        g_64c3f0[i] = 1;
    }
}

// ---------------------------------------------------------------------------
// 0x0db200 - GruntzPlayer::SwapChannel. Move this player onto sound/voice channel
// `channel`: no-op when already there, else - if the target channel is free - release
// the old one, claim the new one, and store it in m_008.
//
// IDENTITY RECOVERED (the old @identity-TODO "owner unrecovered - +0x08-only evidence"
// is CLOSED). The xref settles it: the one and only call site is CMulti's stat-0x3fa
// handler (src/Gruntz/Multi.cpp), which does
//     GruntzPlayer* player = NetGameMgr()->FindPlayer();
//     if (player->SwapChannel(msg->m_c[1]) == 0) { msg->m_c[1] = (char)player->m_008; }
// - it holds a GruntzPlayer, and it reads BACK the very same +0x08 field this method
// writes. So the class is GruntzPlayer (whose 0x0da790..0x0db2f0 RVA band this body sits
// inside) and +0x08 is m_008. The "holder pointer" reading that blocked the earlier
// attribution was an artifact of the fake view's `void*`: the caller passes a zero-
// extended BYTE (`(u8)msg->m_c[1]`), so the parameter is an i32 channel index, exactly
// what the int-seeded ctor writes into m_008.
// Swap's probes are the channel-slot free fns (defined below): the slot read
// (ChannelSlots_Get @0xdb2d0) and the slot set (ChannelSlots_Set @0xdb2b0).
i32 ChannelSlots_Get(i32 i);         // 0xdb2d0
void ChannelSlots_Set(i32 i, i32 v); // 0xdb2b0
RVA(0x000db200, 0x51)
i32 GruntzPlayer::SwapChannel(i32 channel) {
    if (m_008 == channel) {
        return 1;
    }
    if (ChannelSlots_Get(channel)) {
        ChannelSlots_Set(m_008, 1);
        ChannelSlots_Set(channel, 0);
        m_008 = channel;
        return 1;
    }
    return 0;
}

// Return the index of the first free (non-zero) slot, else 0.
RVA(0x000db280, 0x1b)
i32 ChannelSlots_FindFree() {
    for (i32 i = 0; i < 17; i++) {
        if (g_64c3f0[i] != 0) {
            return i;
        }
    }
    return 0;
}

// Set slot[i] = v.
RVA(0x000db2b0, 0x10)
void ChannelSlots_Set(i32 i, i32 v) {
    g_64c3f0[i] = v;
}

// Return slot[i].
RVA(0x000db2d0, 0xc)
i32 ChannelSlots_Get(i32 i) {
    return g_64c3f0[i];
}

// ===========================================================================
// GruntzPlayer::Deactivate (0x0db2f0; ex the "Cdb2f0::Finalize" orphan view,
// folded wave3-J - offsets m_014/m_020/m_038 + the CBattlezMapConfig::Clear_02ade0
// call pin it to this class). If the slot is active (m_020), clear the embedded
// board bundle (unless m_014 is set) and deactivate. Returns 1/0.
RVA(0x000db2f0, 0x2b)
i32 GruntzPlayer::Deactivate() {
    if (m_020 == 0) {
        return 0;
    }
    if (m_014 == 0) {
        ((CBattlezMapConfig*)&m_038)->Clear_02ade0();
    }
    m_020 = 0;
    return 1;
}

// ===========================================================================
// CPlay::StepGridWalk (0x0d0a60) - the per-frame frame-grid advance. If the walk
// is inactive (m_gridWalkActive==0) bail. Decrement the delay countdown (m_gridDelayCount) by dt; while
// it is still positive just return. When it expires reload it from m_gridDelayBase, advance
// the row index (m_gridRow), look up that row's frame in the grid table (clamped to
// [m_64,m_68]); if the lookup is empty wrap back to the first row.
// ===========================================================================
// @early-stop
// regalloc/save-scheduling wall — logic + control flow identical; retail defers the
// `push esi`/`pop esi` past the two early returns (into the m_gridDelayCount>dt block) and colors
// idx/grid into edx/eax swapped vs MSVC here. See docs/patterns/zero-register-pinning.md.
RVA(0x000d0a60, 0x92)
i32 CPlay::StepGridWalk(i32 dt) {
    if (m_gridWalkActive == 0) {
        return 1;
    }
    if ((u32)m_gridDelayCount > (u32)dt) {
        m_gridDelayCount = m_gridDelayCount - dt;
        return 1;
    }
    m_gridDelayCount = m_gridDelayBase;
    m_gridRow = m_gridRow + 1;
    i32 idx = m_gridRow;
    CFrameGrid* g = m_grid;
    i32 frame;
    if (idx >= g->m_firstRow && idx <= g->m_lastRow) {
        frame = g->m_rowTable[idx];
    } else {
        frame = 0;
    }
    m_gridCurFrame = frame;
    if (frame == 0) {
        m_gridCurFrame = g->m_rowTable[g->m_firstRow];
        m_gridRow = g->m_firstRow;
    }
    return 1;
}

// ===========================================================================
// CPlay::DispatchHudClick (0x0ce530) - route a HUD pointer event at (x,y). If the
// HUD is suppressed (m_hudSuppressed) just succeed. If an overlay is up (m_overlayActive) and the guts
// subsystem isn't busy (m_guts->m_position!=2 && m_guts->m_activeTab!=5), forward to the in-rect
// handler. If a pending HUD rect is armed (m_worldReady), post it (gated by the dev-flag
// 0x645578->m_18 & 0x20) and clear m_worldReady/m_dragSnapActive. Finally, unless the guts subsystem
// is busy (==2), point-test against the viewport box (m_c->m_24+0x10): inside ->
// succeed, outside -> forward to the click-at-point handler.
// ===========================================================================
// @early-stop
// zero-register-pinning wall — structure + offsets byte-identical; retail pins ebx=0
// (xor ebx,ebx + cmp ebx,[member] null tests) and colors y into ebp (5 callee-saves),
// MSVC here uses test/4 saves. No source lever forces it. docs/patterns/zero-register-pinning.md.
RVA(0x000ce530, 0xe3)
i32 CPlay::DispatchHudClick(i32 a, i32 x, i32 y) {
    if (m_hudSuppressed != 0) {
        return 1;
    }
    if (m_lightFx != 0 && m_guts->m_position != 2 && m_guts->m_activeTab != 5) {
        m_lightFx
            ->ClearHandle(a, x, y); // 0x13f2 -> CLightFxRender::ClearHandle @0xa9500 (ecx=m_320)
    }
    if (m_worldReady != 0) {
        m_4w()->m_68->HudRect(m_hudRect, g_spawnConfig->m_18 & 0x20);
    }
    m_worldReady = 0;
    m_dragSnapActive = 0;
    if (m_guts->m_position == 2) {
        return 1;
    }
    LevelCoordRect& vp = m_c->m_24->m_planeCtx;
    if (x >= vp.minX && x <= vp.maxX && y >= vp.minY && y <= vp.maxY) {
        return 1;
    }
    m_guts->ClickAt_ff9d0(a, x, y);
    return 1;
}

// ===========================================================================
// CPlay::HandleMousePress (0x0ce660; re-homed from the former gamemousehandler unit,
// waveP - vtable slot 16 / +0x40 of the CPlay/CMulti/CDemo state vtables, mouse
// sibling of the keyboard dispatcher; TU_MIGRATION MOVE row `0x0ce660 HandleMousePress
// @CPlay gamemousehandler -> 0xcdb10 play`). The status-bar sub-objects it reaches
// are modeled minimally as this-facet views; only offsets / code bytes load-bearing,
// every helper reloc-masked external.
// ===========================================================================
// (The ex-`CMapStringToOb` view is DISSOLVED: an empty phantom aliasing the MFC library
// CMapStringToOb::Lookup @0x1b8438 - the member is the real map.)
// (The 13 Sbi* views are GONE - every one was a facet of an already-canonical
// class: SbiSndSet==CSndHost (m_10 map / m_emitGate), SbiCoordSrc==CLevelPlane
// (+0x40 tile origin), SbiHost24==CGameLevel (planeCtx + mainPlane), SbiHostInner
// ==CSpriteFactoryHolder, SbiHost==CWorld, SbiRectSrc/SbiRectHost==CGameLevel via
// the holder, SbiChild==CStatusBarMgr (m_position), SbiEntry==CHitMarker,
// SbiMgr==CGameRegistry (g_sbiMgr was a DUPLICATE binding of the 0x24556c
// g_gameReg singleton!), SbiMgr68==CTriggerMgr (m_rowCount/m_groupFlag),
// SbiCfgEntry==CFocusSlot (m_228). "SbiPointInChild" (a fabricated __stdcall)
// is m_guts->HitTestLayer (thunk 0x1c44 -> 0xfe8a0).)
extern i32 g_sndCueTag; // ?g_sndCueTag@@3HA (HandleMousePress tab cue tag)

// @early-stop
// 4-byte stack-coalesce wall (~84%, body byte-exact). Return-epilogue tail-merge
// (paired guards combined with `||`) + a uniform +4 frame shift (sub esp,0x14 vs
// 0x10): retail packs every local into the 16-byte RECT + reuses dead x/y arg homes
// for the Probe/Find out-params; this build spills one out-param to a fresh 5th slot.
// A documented stack-slot-coalesce coin-flip (docs/patterns/stack-slot-coalesce-
// frame-4b.md), not source-steerable; ZERO logic differences remain.
RVA(0x000ce660, 0x362)
i32 CPlay::HandleMousePress(i32 msg, i32 x, i32 y) {
    if (m_hudSuppressed != 0 || m_guts == 0) {
        return 1;
    }
    if (m_overlayDrag != 0 || g_gameReg->m_cmdGrid->m_groupFlag == 0) {
        return m_guts->ClickHilite(msg, x, y);
    }
    if (m_dragInhibit1 != 0 || m_dragInhibit2 != 0) {
        return this->Vslot0e(msg, x, y); // base handler at vtable slot +0x38
    }

    if (m_guts->m_position == 2 && m_guts->HitTestLayer(x, y)) {
        CSndHost* set = m_4w()->m_30->m_28;
        if (set->m_emitGate == 0) {
            LeafCue* e = 0;
            set->m_10.Lookup("GAME_TABHIGHLIGHT1", (void*&)e); // Ptr map out-param idiom
            if (e != 0) {
                e->PlayIfElapsed_01f940(g_sndCueTag, 0, 0, 0);
            }
        }
        m_guts->RefreshState();
        if (m_guts->m_position == 1) {
            m_hitTest->Configure(2);
        } else {
            m_hitTest->Configure(1);
        }
        return 1;
    }

    i32 idx = m_guts->HitTest(x, y);
    if (idx != -1) {
        m_guts->PlaceCursorTarget(idx, 1);
        return 1;
    }

    RECT* rc = (RECT*)&m_c->m_24->m_planeCtx;
    i32 rl = rc->left;
    i32 rt = rc->top;
    i32 rr = rc->right;
    i32 rb = rc->bottom;
    if (x < rl || x > rr || y < rt || y > rb) {
        return m_guts->ClickHilite(msg, x, y);
    }

    i32 outArea;
    i32 outVal;
    if (m_4w()->m_68->ScreenToCell(x, y, &outArea, &outVal, 5) && g_curPlayer == outArea) {
        m_guts->ToggleStat(outVal);
        return 1;
    }

    if (m_dragInhibit1 != 0) {
        return 1;
    }
    i32 area = g_curPlayer;
    CFocusSlot* cfg = &g_gameReg->m_focusSlots[area];
    if (cfg == 0) {
        return 0;
    }
    if (g_gameReg->m_cmdGrid->m_rowCount[area] >= cfg->m_228) {
        return 0;
    }

    CGameLevel* h = m_4w()->m_30->m_24;
    i32 px = h->m_mainPlane->m_originX - h->m_planeCtx.minX + x;
    i32 py = h->m_mainPlane->m_originY - h->m_planeCtx.minY + y;
    for (i32 i = 0; i < markerCount(); i++) {
        CHitMarker* e = markerData()[i];
        if (e == 0) {
            continue;
        }
        RECT er;
        SetRect(&er, e->m_0 - 0x10, e->m_4 - 0x10, e->m_0 + 0x10, e->m_4 + 0x10);
        if (px < er.right && px >= er.left && py < er.bottom && py >= er.top) {
            if (!m_guts->FindReadySlot()) {
                return 1;
            }
            char ab = (char)g_curPlayer;
            px = (px & 0xffe0) + 0x10;
            py = (py & 0xffe0) + 0x10;
            m_4w()->m_6c->Spawn(1, ab, 0, 0, px, py, 0, 0);
            return 1;
        }
    }
    return 1;
}

// ===========================================================================
// CPlay::BeginGridWalk (0x0d0920) - arm the frame-grid walk for a level cue. Look
// up the grid object for `key` in the view's grid map (m_c->m_10->m_10); bail if
// absent. If `hasGrid`, load the grid's frame sprite from the world sprite factory
// (m_4->m_74, retrying via the registry's factory) using the per-type descriptor
// from the world config array (m_4+0x158, indexed by g_curPlayer), then push it into
// the grid (SetDelay 0xa / SetSprite). Finally seed the current row (m_gridCurFrame) from
// `index` (clamped to [m_64,m_68]) and, if non-empty, latch the e8/delay state.
// ===========================================================================
// @early-stop
// arg-push-scheduling wall — control flow + the config-array index + map Lookup +
// grid-setup byte-identical; the LoadSprite retry pushes (prev,1) in the opposite
// order and the g_gameReg reload sits one slot off. docs/patterns/statement-schedule-faithful.md.
RVA(0x000d0920, 0xfe)
i32 CPlay::BeginGridWalk(const char* key, i32 index, i32 e8, i32 delay, i32 hasGrid) {
    if (m_c == 0) {
        return 1;
    }
    CFrameGrid* grid = 0;
    // frame-grid probe into the image registry's name->object map (frame-grid Lookup overload).
    m_c->m_10->m_10map.Lookup(key, (CObject*&)grid);
    m_grid = grid;
    if (grid == 0) {
        return 1;
    }
    m_gridHasSprite = hasGrid;
    if (hasGrid != 0) {
        CWorld* w = m_4w();
        i32 id = g_curPlayer;
        void* spr = w->m_74->LoadSprite(*(void**)(w->m_158 + (id * 0x47) * 8), 0);
        if (spr == 0) {
            spr = g_gameReg->m_spriteFactory->LoadSprite(spr, 1);
        }
        ((CImageSet*)m_grid)->SetAllTypes(0xa);
        ((CImageSet*)m_grid)->SetAllFormats((i32)spr);
    }
    CFrameGrid* g = m_grid;
    i32 frame;
    if (index >= g->m_firstRow && index <= g->m_lastRow) {
        frame = g->m_rowTable[index];
    } else {
        frame = 0;
    }
    m_gridCurFrame = frame;
    if (frame != 0) {
        m_gridRow = index;
        m_gridWalkActive = e8;
        m_gridDelayBase = delay;
        m_gridDelayCount = delay;
    }
    return 1;
}

// ===========================================================================
// CPlay::HandleDragMove (0x0d0db0) - the per-frame drag/box-select update at
// pointer (x,y) with selector `a`. Bails while the primary mode (m_inGame) or the
// paused flag (m_paused) is set. If an overlay is up (m_overlayActive) and the guts subsystem
// isn't busy, forwards to the in-rect HUD drag. Then a 3-way:
//   m_dragSnapActive  -> a drag is being snapped: snap to (m_snapOriginX+x, m_snapOriginY+y) and re-arm m_scrollSink.
//   m_overlayDrag  -> an overlay drag: dispatch m_guts->DragSelect and return.
//   else   -> the world box-drag: point-test against the viewport box
//             (m_c->m_24+0x10). INSIDE -> finish (clear m_dragInProgress, normalize the sel
//             rect, re-arm m_scrollSink); OUTSIDE -> continue the drag (post to m_hitTest's
//             hit-test, else either WorldPost the world-space delta or DragSelect
//             + clamp the sel rect into the box, then the m_dragEndNotify end-notify).
// ===========================================================================
// @early-stop
// block-placement + min/max-clamp regalloc wall (~35%). Logic, control flow,
// member offsets, the full call set, the box point-test and BOTH drag-rect clamp
// ladders are byte-faithful; the RECT-local copy already forced the matching
// `sub esp,0x10` frame and the goto folds the two snap/inside tails into retail's
// shared Lf1a. The residual is non-source-steerable codegen choice: (1) retail
// hoists the cold OUTSIDE-of-box clamp block to the function end (forward-jumped)
// while MSVC here lays it inline after the box-test; (2) the m_overlayActive/m_dragSnapActive probes
// color into ecx vs eax; (3) the four min/max ternaries pick the eax/ecx operand
// for the clamp the opposite way. The block-float matches the family in
// docs/patterns/nested-if-success-deepest-error-tail.md; the regalloc residual is
// docs/patterns/zero-register-pinning.md.
RVA(0x000d0db0, 0x347)
i32 CPlay::HandleDragMove(i32 a, i32 x, i32 y) {
    // box corners declared (uninitialized) up front so the `goto rearm` tail
    // doesn't cross their initialization (MSVC C2362); they're filled only on
    // the box-drag path below and unused at the rearm label.
    i32 left, top, right, bottom;
    if (m_inGame != 0) {
        return 1;
    }
    if (m_paused != 0) {
        return 1;
    }
    if (m_lightFx != 0 && m_guts->m_position != 2 && m_guts->m_activeTab != 5) {
        m_lightFx->ApplyB(a, x, y); // 0x3e86 -> CLightFxRender::ApplyB @0xa95d0 (ecx=m_320)
    }

    if (m_dragSnapActive != 0) {
        if (m_guts == 0) {
            return 1;
        }
        m_guts->SetSpritePos(m_snapOriginX + x, m_snapOriginY + y); // 0x3878 -> @0xfe860
        goto rearm; // -> shared m_scrollSink re-arm tail
    }

    if (m_overlayDrag != 0) {
        m_guts->ClickToggle(a, x, y);
        return 1;
    }

    // --- the world box-drag: point-test (x,y) against the viewport box ---
    // (box copied to a 0x10 RECT local so the clamp ladders re-read top/right/
    //  bottom from [esp+0x14/0x18/0x1c] across the DragSelect call. The INSIDE
    //  path is the fall-through "success" so the OUTSIDE block floats to the
    //  tail; see docs/patterns/nested-if-success-deepest-error-tail.md.)
    LevelCoordRect box = m_c->m_24->m_planeCtx;
    left = box.minX;
    top = box.minY;
    right = box.maxX;
    bottom = box.maxY;
    if (x >= left && x <= right && y >= top && y <= bottom) {
        // INSIDE the box -> finish the drag.
        if (m_dragInProgress != 0) {
            m_guts->ClearTabSprites(-1);
        }
        m_dragInProgress = 0;
        if (m_worldReady != 0) {
            // normalize {m_cursorX,m_cursorY}..{m_dragClampMaxX,m_dragClampMaxY} into min/max:
            m_hudRect.left = m_cursorX < m_dragClampMaxX ? m_cursorX : m_dragClampMaxX;
            m_hudRect.right = m_cursorX > m_dragClampMaxX ? m_cursorX : m_dragClampMaxX;
            m_hudRect.top = m_cursorY < m_dragClampMaxY ? m_cursorY : m_dragClampMaxY;
            m_hudRect.bottom = m_cursorY > m_dragClampMaxY ? m_cursorY : m_dragClampMaxY;
            goto rearm; // -> shared m_scrollSink re-arm tail
        }

        // m_worldReady == 0: the hit-test / world-post branch.
        if (m_hitTest->HitTest(x, y) != 0 || m_4w()->m_c != 0 || m_inGame != 0
            || m_dragInhibit1 != 0 || m_dragInhibit2 != 0) {
            // (a second, distinct re-arm landing pad in retail.)
            CGameObject* s2 = m_scrollSink;
            if (s2 == 0) {
                return 1;
            }
            s2->m_stateFlags |= 1;
            return 1;
        }
        if (m_levelId != 0) {
            if (m_scrollSink != 0) {
                m_scrollSink->m_stateFlags |= 1;
            }
        } else {
            if (m_scrollSink != 0) {
                m_scrollSink->m_stateFlags &= ~1;
            }
        }
        CGameLevel* v = m_c->m_24;
        i32 wx = v->m_mainPlane->m_originX - v->m_planeCtx.minX + x;
        i32 wy = v->m_mainPlane->m_originY - v->m_planeCtx.minY + y;
        m_4w()->m_68->PlaceObjectFull(wx, wy); // 0x2ca7 -> @0x78a50
        return 1;
    }

    // OUTSIDE the box -> continue dragging (the cold block, floated to the tail).
    if (m_scrollSink != 0) {
        m_scrollSink->m_stateFlags |= 1;
    }
    m_dragInProgress = 1;
    m_guts->ClickToggle(a, x, y);
    if (m_worldReady != 0) {
        // clamp the selection rect into [box, m_dragClampMaxX/m_dragClampMaxY]:
        i32 lo = m_cursorX > left ? m_cursorX : left;
        m_hudRect.left = lo;
        if (lo >= m_dragClampMaxX) {
            m_hudRect.left = m_dragClampMaxX;
        }
        i32 hi = m_cursorX < right ? m_cursorX : right;
        m_hudRect.right = hi;
        if (hi > m_dragClampMaxX) {
            m_hudRect.right = m_dragClampMaxX;
        }
        i32 tlo = m_cursorY <= top ? m_cursorY : top;
        m_hudRect.top = tlo;
        if (tlo < m_dragClampMaxY) {
            m_hudRect.top = m_dragClampMaxY;
        }
        i32 thi = m_cursorY < bottom ? m_cursorY : bottom;
        m_hudRect.bottom = thi;
        if (thi > m_dragClampMaxY) {
            m_hudRect.bottom = m_dragClampMaxY;
        }
    }
    if (m_dragEndNotify != 0 && m_4w()->m_68->m_pendingFxKind == 0) {
        FlushPendingOps(); // "EndDragSel" 0x4da2d0 IS this fn's own 0xda2d0
    }
    return 1;

rearm:
    CGameObject* s = m_scrollSink;
    if (s == 0) {
        return 1;
    }
    s->m_stateFlags |= 1;
    return 1;
}

// ===========================================================================
// CPlay::BuildHelpReveal (0x0d72c0) - the per-frame help-overlay "wipe" animation.
// Bails if there is no view (m_c->m_4->m_14). On the first frame (m_revealFrame==1) draws
// the two static end-cap strips. Each frame computes the wipe column from the
// frame counter (counter * 3.7857, truncated), then either snaps the single cap to
// the right edge (counter>=0x37) or sweeps a column of strips (0xe0 - i*3.7857) for
// i in [counter, 0x37). Always draws the trailing cap and advances the counter.
// ===========================================================================
// @early-stop
// control-flow/float-schedule wall — prologue + the two cap strips + the per-strip
// HudStrip pushes + the __ftol column math are byte-faithful; the counter>=0x37 cap
// branch merges into the trailing-cap tail via a shared landing pad the C if/else
// can't reproduce 1:1, and the x87 fmul/fild ordering diverges. ~68%.
// NB retail rets 0x4 - the old no-arg decl DROPPED the `final` parameter (every
// caller pushes 0; LoadByMode's finale pushes 1). The strip blit is LayerBlitFrame
// (thunk 0x18ca -> 0x115300); retail also runs 2x an 0x11f570 leaf this
// reconstruction still lacks (final-sweep item).
RVA(0x000d72c0, 0x128)
i32 CPlay::BuildHelpReveal(i32 final) {
    (void) final;
    CImage* view = (CImage*)m_c->m_drawTarget->m_14;
    if (view == 0) {
        return 0;
    }
    if (m_revealFrame == 1) {
        LayerBlitFrame((CResMgr*)m_c, (CImage*)m_revealCapStart, 0x140, 0x1a6, 1, 0);
        LayerBlitFrame((CResMgr*)m_c, (CImage*)m_revealCapMid, 0xe0, 0x1a6, 1, 0);
    }

    i32 counter = m_revealFrame;
    i32 col = (i32)((float)counter * 3.7857143878936768f);
    if (counter < 0x37) {
        i32 i = counter;
        do {
            i32 x = 0xe0 - (i32)((float)i * -3.7857143878936768f);
            LayerBlitFrame((CResMgr*)m_c, (CImage*)m_revealCapMid, x, 0x1a6, 1, 0);
            i++;
        } while (i < 0x37);
    } else {
        LayerBlitFrame((CResMgr*)m_c, (CImage*)m_revealCapMid, col + 0xe0, 0x1a6, 1, 0);
    }

    LayerBlitFrame((CResMgr*)m_c, (CImage*)m_revealCapEnd, 0x1b4, 0x1a6, 1, 0);
    m_revealFrame = m_revealFrame + 1;
    return 1;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: med
// @source: rtti-vptr
// @stub
RVA(0x0008c9d0, 0x2bd)
void CPlay::PlayBacklog08c9d0() {}

// CPlay::Vslot1a (0x8c930) - vtable slot 26. CPlay's default: return 0.
RVA(0x0008c930, 0x3)
i32 CPlay::Vslot1a() {
    return 0;
}

// CPlay::GetFrame (0x8c950) - vtable slot 27. CPlay's base returns 0 (frame count;
// overridden by subclasses that track a frame number).
RVA(0x0008c950, 0x3)
i32 CPlay::GetFrame() {
    return 0;
}

// CState::SetBeginClearParams (0x8c970) - seed the begin-clear params.
RVA(0x0008c970, 0x1c)
i32 CState::SetBeginClearParams(i32 unused, i32 arg2, i32 arg3) {
    m_cursorX = arg2;
    m_cursorY = arg3;
    return 1;
}

// The cached PostMessageA fn-ptr (bare 0x6c44c8; the paused-frame unpause posts
// WM_COMMAND 0x816e through it). Same decl used by the Dispatch cluster below.

// CPlay::Vslot0d (0x0cda70) - vtable slot 13, the key-UP scroll-edge filter. On a
// key-up event (flags bit 0x1000000 set) for an arrow key, clear the matching
// scroll-edge-lock direction bit (VK_LEFT->bit0, VK_RIGHT->bit2, VK_UP->bit1,
// VK_DOWN->bit3). Always returns 1 (handled).
RVA(0x000cda70, 0x7a)
i32 CPlay::Vslot0d(i32 key, i32 flags) {
    if (flags & 0x01000000) {
        if (key == VK_LEFT) {
            m_scrollEdgeLock &= ~1;
        } else if (key == VK_RIGHT) {
            m_scrollEdgeLock &= ~4;
        } else if (key == VK_UP) {
            m_scrollEdgeLock &= ~2;
        } else if (key == VK_DOWN) {
            m_scrollEdgeLock &= ~8;
        }
    }
    return 1;
}

// ===========================================================================
// CPlay::OnMouseUp (0x0cdb10, Ghidra-named winapi_0cdb10_PostMessageA after the
// PostMessageA it fires) - the menu/pause-state mouse button-UP / drag-release
// handler, sibling of OnKeyCommand/HandleTileClick: the same hudSuppressed /
// renderDisabled(resume) / inGame(reset-or-report 0x457) / paused(unpause) /
// overlayDrag priority chain, then (no active grunt) the marker/waypoint place,
// drag-box and grunt-pick dispatch. Non-EH; __thiscall(a, x, y), ret 0xc; every
// path returns 1 except the overlay-drag guts dispatch tail (returns its result).
// ===========================================================================
// @early-stop
// full+correct reconstruction (was a bare `return 0` stub; 0.29% -> 34.9%). Six
// steerable codegen fixes landed on the correct structure:
//   1. nest the no-active-grunt body deepest so the guts-dispatch cold block floats
//      to the tail (nested-if-success-deepest-error-tail.md): 27 -> 30
//   2. byte-widen the g_curPlayer marker arg via a `char` Place param (mov cl,[g] +
//      store-byte/read-dword): 30 -> 32.2
//   3. order waypoint_cancel before drag_path per retail (cdf36 < cdf6c)
//   4. route the drag/pick/reset `return 1`s to a shared `ret1:` tail (retail's
//      0xce2e9), matching its 23 vs my former 25 epilogues: 32.2 -> 33.3
//   5. cache arg1 x in a pure-read local `xr` for the pre-snap value reads so cl
//      promotes it to EBX like retail (the arg slot is modified in place by the &x
//      snap/place calls): 33.3 -> 34.6  (drag_box's first rect-test x stays stack)
//   6. size FindGruntAt's span-output arg as an 8-byte buffer (2 ints), not a 4-byte
//      void* - this makes cl reserve retail's 0x20 frame (was 0x1c), so every
//      [esp+arg] aligns: 34.6 -> 34.9. The "frame-coalesce wall" was a sizing bug.
// Residual (regalloc, not source-steerable): the marker/drag blocks assign sx/sy to
// edi/ecx where retail uses ebp/edi, and cl spills the cached CWorld* to [esp+0x10]
// where retail re-reads m_4 (removing the cache regressed 34.9->33.3 - the re-read
// cascades worse); plus m_5c `add 0x40`/`[+0x40]` addressing-mode coin-flips.
// Final-sweep/permuter candidate.
RVA(0x000cdb10, 0x80c)
i32 CPlay::winapi_0cdb10_PostMessageA(i32 a, i32 x, i32 y) {
    // retail keeps the ORIGINAL click x in EBX for the pre-snap value reads while
    // the arg slot [esp+0x38] is modified in place by the snap/place calls (they
    // take &x). Declared before any goto so no init is skipped.
    i32 xr = x;
    if (m_hudSuppressed != 0) {
        return 1;
    }
    if (m_renderDisabled != 0) {
        m_hudSuppressed = 1;
        m_renderDisabled = 0;
        EnterMode(3);
        m_inGame = 1;
        return 1;
    }
    if (m_inGame != 0) {
        if (ResetPlayState()) {
            goto ret1;
        }
        m_4->ReportError(0x800a, 0x457);
        return 1;
    }
    if (m_paused != 0) {
        m_paused = 0;
        ::PostMessageA((HWND)m_4w()->m_4->m_4, 0x111, 0x816e, 0);
        return 1;
    }
    // The overlay-drag / command-grid-idle gates route to the guts dispatch tail
    // (a forward je all the way down); nest the no-active-grunt main body deepest
    // so retail's cold-block-at-tail layout falls out (nested-if-success-deepest).
    if (m_overlayDrag == 0 && g_gameReg->m_cmdGrid->m_groupFlag != 0) {
        if (m_4w()->m_c != 0) {
            goto drag_path;
        }

        // ---- no active grunt: overlay probe, then marker/waypoint place ----
        if (m_lightFx != 0 && m_guts->m_position != 2 && m_guts->m_activeTab != 5) {
            if (m_lightFx->ApplyA(a, xr, y)) { // 0x3e59 -> CLightFxRender::ApplyA @0xa9480
                return 1;
            }
        }
        CWorld* w = m_4w();
        CGameLevel* geom = w->m_30->m_24;
        CLevelPlane* cam = geom->m_mainPlane;
        i32 sx = cam->m_originX - geom->m_planeCtx.minX + xr;
        i32 sy = cam->m_originY - geom->m_planeCtx.minY + y;
        if (m_dragInhibit1 == 0) {
            goto mode_36c;
        }
        if (m_4f0 != 0) {
            goto mode_36c;
        }
        i32 placed = 0;
        RECT* gr = (RECT*)&m_guts->m_10; // +0x10 widget rect (grouping conflict w/ SBI m_rect14)
        if (xr < gr->right && xr >= gr->left && y < gr->bottom && y >= gr->top) {
            // inside the guts HUD rect -> finalize with placed == 0
        } else {
            RECT* wr = (RECT*)&geom->m_planeCtx;
            if (xr < wr->right && xr >= wr->left && y < wr->bottom && y >= wr->top) {
                if (FindStartPointAt(sx, sy, &x, &y)) {
                    char tok = *(char*)&g_curPlayer;
                    w->m_6c
                        ->EnqueueSingle(1, tok, 0, 0, (i16)x, (i16)y, 0, 0); // 0x2095 -> @0x23c30
                    placed = 1;
                }
            }
        }
        if (placed == 0) {
            ((CGruntSpawnConfig*)g_gameReg->m_cueSink)
                ->SpawnVoiceDriver(placed, 0x340, -1, 1, -1, -1);
        }
        m_dragInhibit1 = 0;
        m_guts->CommitSlot(placed);
        SetCursorFrame(0);
        return 1;
    } else {
        goto guts_dispatch;
    }

mode_36c:
    if (m_dragInhibit2 == 0) {
        goto drag_path;
    }
    if (m_4f0 != 0) {
        goto drag_path;
    }
    {
        RECT* gr = (RECT*)&m_guts->m_10; // +0x10 widget rect (grouping conflict w/ SBI m_rect14)
        if (xr < gr->right && xr >= gr->left && y < gr->bottom && y >= gr->top) {
            if (m_guts->SetFallRect(xr, y, *(char*)&m_cursorFrame)) {
                m_dragInhibit2 = 0;
                SetCursorFrame(0);
                return 1;
            }
            goto waypoint_cancel;
        }
        CWorld* w = m_4w();
        CGameLevel* geom = w->m_30->m_24;
        RECT* wr = (RECT*)&geom->m_planeCtx;
        if (!(xr < wr->right && xr >= wr->left && y < wr->bottom && y >= wr->top)) {
            goto waypoint_cancel;
        }
        // inside the world rect: place a waypoint through the trigger grid
        CGameLevel* ds = m_c->m_24;
        CLevelPlane* cam = ds->m_mainPlane;
        i32 wx = cam->m_originX - ds->m_planeCtx.minX + xr;
        i32 wy = cam->m_originY - ds->m_planeCtx.minY + y;
        i32 tok = *(char*)&m_cursorFrame;
        if (g_gameReg->m_cmdGrid->CellHitTest(wx, wy, &x, &y, tok) != 0) {
            w->m_6c->EnqueueSingle(1, (char)a, (char)y, 8, 0, 0, (char)tok, 0);
            m_4f0 = 1;
            return 1;
        }
        // cdea2: nothing placed -> pick a grunt in a 30x30 world box via the grid
        RECT box;
        box.left = wx - 0xf;
        box.top = wy - 0xf;
        box.right = wx + 0xf;
        box.bottom = wy + 0xf;
        i32 out28[2] = {0, 0};
        i32 col = 0;
        CTmCell* p = g_gameReg->m_cmdGrid->FindGruntAt(wx, wy, (RECT*)out28, &col, &y, &box);
        if (p == 0 || g_curPlayer != p->m_tileOwnerHi) {
            goto waypoint_cancel;
        }
        w->m_6c->EnqueueSingle(1, (char)a, (char)y, 8, 0, 0, (char)tok, 0);
        return 1;
    }

waypoint_cancel:
    m_dragInhibit2 = 0;
    m_guts->EnterHlRow(0, *(char*)&m_cursorFrame);
    SetCursorFrame(0);
    return 1;

drag_path: {
    // m_4w()->m_c != 0: an active grunt is selected -> drag / guts dispatch
    (void)y;
    if (m_guts == 0) {
        return 1;
    }
    if (m_guts->m_position == 2) {
        if (m_guts->HitTestLayer(xr, y)) {
            m_dragSnapActive = 1;
            // the guts +0x08 slot holds the dragged widget's display object
            // (screen pos at +0x5c/+0x60 - the CGameObject shape); snap origin =
            // widget pos - click pos.
            CGameObject* g8 = *(CGameObject**)((char*)m_guts + 8);
            i32 dx = 0;
            if (g8 != 0) {
                dx = g8->m_screenX - xr;
            }
            m_snapOriginX = dx;
            CGameObject* g8b = *(CGameObject**)((char*)m_guts + 8);
            if (g8b == 0) {
                m_snapOriginY = 0;
                return 1;
            }
            m_snapOriginY = g8b->m_screenY - y;
            return 1;
        }
        goto drag_box;
    }
    // m_guts->m_position != 2: guts-rect dispatch
    RECT* gr = (RECT*)&m_guts->m_10; // +0x10 widget rect (grouping conflict w/ SBI m_rect14)
    if (xr < gr->right && xr >= gr->left && y < gr->bottom && y >= gr->top) {
        FlushPendingOps();
        return m_guts->UpdateStatusBarTabHighlight(a, xr, y);
    }
    if (m_hitTest->HitTest(xr, y)) { // via ILT 0x43e0 (ex the thunk-alias dup decl "HitTest43e0")
        return 1;
    }
}

drag_box: {
    if (m_4w()->m_c != 0) {
        goto ret1;
    }
    CWorld* w = m_4w();
    RECT* wr = (RECT*)&w->m_30->m_24->m_planeCtx;
    if (!(x < wr->right && x >= wr->left && y < wr->bottom)) {
        goto ret1;
    }
    if (y < wr->top) {
        return 1;
    }
    // inside the world rect
    if (m_dragEndNotify != 0) {
        i32 ex = (y & ~0x1f) + 0x10;
        i32 ey = (y & ~0x1f) + 0x10;
        i32 lv = m_levelId - 0xc8;
        if (lv <= 0x16) {
            g_gameReg->m_cmdGrid->ResetGroup(ex, ey, 0, 0, 0, 2, 1);
        } else if (lv >= 0x17 && lv <= 0x20) {
            g_gameReg->m_cmdGrid->ResetGroup(ex, ey, 0, 0, 0, 3, 1);
        }
        g_gameReg->m_cmdGrid->m_pendingFxKind = 0;
        LoadCursorSprites(0, 0);
        m_dragClampMaxX = xr;
        m_dragClampMaxY = y;
        m_hudRect.left = xr;
        m_hudRect.top = y;
        m_hudRect.right = xr;
        m_hudRect.bottom = y;
        m_worldReady = 1;
        return 1;
    }
    {
        i32 ex = (y & ~0x1f) + 0x10;
        i32 ey = (y & ~0x1f) + 0x10;
        if (g_gameReg->m_cmdGrid->TriggerCell(ex, ey)) {
            return 1;
        }
    }
    // ce191: level-gated curse cue + waypoint queue
    if (m_levelId >= 0xc8) {
        CTriggerMgr* cg = g_gameReg->m_cmdGrid;
        CTmCell* slot = 0;
        if (1 == cg->m_recList.GetCount()) { // exactly one record node
            i32* sel = *(i32**)(*(char**)((char*)&cg->m_recList + 4) + 8);
            slot = cg->m_grid[sel[1] * 15 + sel[0]];
        }
        if (slot != 0 && slot->m_entranceCommitted != 0) {
            ((CGruntSpawnConfig*)g_gameReg->m_cueSink)
                ->SpawnVoiceDriver((i32)slot, 0x324, -1, 0, -1, -1);
        }
    }
    LoadCursorSprites(0, 0);
    i32 hit = m_guts->HitTest(xr, y);
    if (hit != -1) {
        m_guts->PlaceCursorTarget(hit, 0);
        return 1;
    }
    // ce22f: pick a grunt at the point
    i32 slot38 = 0;
    CGrunt* picked = (CGrunt*)m_4w()->m_68->ScreenToCell(xr, y, &slot38, &slot38, 5); // 0x3cb0
    if (picked == 0) {
        m_dragClampMaxX = xr;
        m_dragClampMaxY = y;
        m_hudRect.left = xr;
        m_hudRect.right = xr;
        m_hudRect.top = y;
        m_hudRect.bottom = y;
        m_worldReady = 1;
        goto ret1;
    }
    m_4w()->m_68->ResetCell(slot38, slot38, g_spawnConfig->m_18 & 0x20, 0); // 0x29cd -> @0x6bfd0
    if (a == g_curPlayer) {
        if (0 != (g_spawnConfig->m_18 & 0x20)) {
            goto ret1;
        }
        picked->OnStruck(1); // 0x1f4b -> CGrunt::OnStruck @0x588f0
        return 1;
    }
    picked->OnStruck(0);
    return 1;
}

ret1:
    return 1;

guts_dispatch:
    return m_guts->UpdateStatusBarTabHighlight(a, x, y);
}

// ===========================================================================
// CPlay::HandleTileClick (0x0ceae0) - the menu/pause-state pointer-click handler,
// the mouse-input twin of OnKeyCommand: same hudSuppressed / renderDisabled(resume)
// / inGame(reset-or-report) / paused(unpause+PostMessage) priority chain, then the
// no-active-grunt overlay path: probe the overlay object, early-out on the HUD hit
// rect, run the guts HUD hit-test, else (inside the world rect) snap the click to
// the tile grid and place/cancel the world marker. Returns 1 in every path.
// ===========================================================================
// (COverlayClick is GONE - the +0x320 overlay is the typed CLightFxRender
// (m_lightFx) and its click probe is ApplyGlobal @0xa9550, thunk 0x27a7 - the old
// "Probe IS BuildGruntTypeNameTable via 0x12da" claim was WRONG for this site.)
// CPlay::Vslot13 (0x0ceab0) - vtable slot 19. Re-forward all three args to the
// class's own slot-17 virtual (Vslot11).
RVA(0x000ceab0, 0x17)
i32 CPlay::Vslot13(i32 a, i32 b, i32 c) {
    return Vslot11(a, b, c);
}

// @early-stop
// regalloc coin-flip wall (docs/patterns/zero-register-pinning.md): the whole
// priority chain, the overlay probe, both HUD/world rect hit-tests, the grid-snap
// math + PlaceMarker/CancelMarker tail are all byte-faithful (the grid math +
// 7-arg PlaceMarker push match exactly). Residual: MSVC assigns the x-coord to
// edi and y to ebx where retail pins x->ebx / y->edi (mirror pair) and defers the
// y-load to the guts-rect site; because the compare ORDER is byte-matched I cannot
// reorder to flip the pair without breaking the matched guts rect. The swap also
// spares retail's rect-field spill, so my frame drops sub esp,0x10 (the 3 arg-load
// displacements + 7 epilogue add esp shift). Pure allocator choice, no source
// lever. ~81%.
RVA(0x000ceae0, 0x268)
i32 CPlay::HandleTileClick(i32 a, i32 x, i32 y) {
    if (m_hudSuppressed != 0) {
        return 1;
    }
    if (m_renderDisabled != 0) {
        m_hudSuppressed = 1;
        m_renderDisabled = 0;
        EnterMode(3);
        m_inGame = 1;
        return 1;
    }
    if (m_inGame != 0) {
        if (ResetPlayState()) {
            return 1;
        }
        m_4->ReportError(0x800a, 0x458);
        return 1;
    }
    if (m_paused != 0) {
        m_paused = 0;
        PostMessageA(m_4w()->m_4->m_4->m_4, 0x111, 0x816e, 0);
        return 1;
    }
    if (m_overlayDrag != 0) {
        return 1;
    }
    if (g_gameReg->m_cmdGrid->m_groupFlag == 0) { // reg->m_68 cue-sink busy gate
        return 1;
    }
    if (m_4w()->m_c != 0) {
        return 1;
    }
    if (m_lightFx != 0 && m_guts->m_position != 2 && m_guts->m_activeTab != 5) {
        if (m_lightFx->ApplyGlobal(a, x, y)) { // 0x27a7 -> CLightFxRender::ApplyGlobal @0xa9550
            return 1;
        }
    }
    // the guts widget rect: {m_10, m_rect14.m_0/.m_4/.m_8} = the +0x10..+0x1c
    // left/top/right/bottom quad (the SBI side reads the same bytes as base-x +
    // SbiRect - a field-grouping conflict flagged for reconciliation).
    if (x < m_guts->m_rect14.m_4 && x >= m_guts->m_10 && y < m_guts->m_rect14.m_8
        && y >= m_guts->m_rect14.m_0) {
        return 1;
    }
    i32 idx = m_guts->HitTest(x, y);
    if (idx != -1) {
        m_guts->ClearStat(idx);
        CTriggerMgr* w = m_4w()->m_68;
        if (w->m_goal != 0) {
            w->m_goal->m_8 |= 0x10000;
            w->m_goal = 0;
        }
        w->m_armed = 0;
        return 1;
    }
    if (m_4w()->m_68->m_recList.GetCount() == 0) { // +0x24c == m_recList.m_nCount
        return 1;
    }
    CGameLevel* ph = m_4w()->m_30->m_24;
    if (x < ph->m_planeCtx.maxX && x >= ph->m_planeCtx.minX && y < ph->m_planeCtx.maxY
        && y >= ph->m_planeCtx.minY) {
        CGameLevel* ds = m_c->m_24;
        CLevelPlane* geom = ds->m_mainPlane;
        i32 rawX = geom->m_originX - ds->m_planeCtx.minX + x;
        i32 rawY = geom->m_originY - ds->m_planeCtx.minY + y;
        i32 snapX = (rawX & ~0x1f) + 0x10;
        i32 snapY = (rawY & ~0x1f) + 0x10;
        m_tileClickX = snapX;
        m_tileClickY = snapY;
        CTriggerMgr* w = m_4w()->m_68;
        if (w->m_overlay != 0 && w->m_overlay->m_active != 0) {
            w->OverlayTick(); // 0x1514 -> @0x78a30 (was "CancelMarker")
            return 1;
        }
        w->ResetGroup(snapX, snapY, rawX, rawY, 1, 0, 1); // 0x3044 -> @0x79520 (was "PlaceMarker")
    }
    return 1;
}

// @confidence: low
// @source: winapi:CopyRect
// @stub
RVA(0x000d0b30, 0x200)
i32 CPlay::winapi_0d0b30_CopyRect(i32) {
    return 0;
}

// ---------------------------------------------------------------------------
// @rehomed CGameLevel::MainPlaneQueryA/B <- gamelevel (rule-(c) interleaver)
// CGameLevel::MainPlaneQueryA/B (0x000cedf0 / 0x000cee10) - homed here from
// GameLevel.cpp (REHOME D10). Retail emits both CGameLevel forwarder COMDATs
// out-of-line INSIDE CPlay's block (0x000ceae0 HandleTileClick .. 0x000cee70
// ForwardReady), a rule-(c) interleaver run of 2 surrounded by play on both sides.
// Each forwards to the main plane's CPlaneRender::CenterScroll{A,B}; no-op/0 when
// the plane ptr is null. CGameLevel + CPlaneRender both declared in this TU.
RVA(0x000cedf0, 0xf)
i32 CGameLevel::MainPlaneQueryA() {
    if (m_mainPlane != 0) {
        return ((CPlaneRender*)m_mainPlane)->CenterScrollA(); // 0x163300
    }
    return 0;
}

RVA(0x000cee10, 0xf)
i32 CGameLevel::MainPlaneQueryB() {
    if (m_mainPlane != 0) {
        return ((CPlaneRender*)m_mainPlane)->CenterScrollB(); // 0x163370
    }
    return 0;
}

// ===========================================================================
// DrawWorldPresent (0x0cefc0) - a present-only world frame: run the two camera
// sub-steps (DrawB then DrawA), guarded on the camera-geom ptr, twice - each
// pair preceded by a renderer begin-scene(1) - then push the view, present, and
// tick the manager. Migrated from engine_boundary (CPlay: the m_c draw chain +
// m_4 manager). All draw callees are out-of-line / reloc-masked.
// @early-stop
// ~99%: code structure byte-exact; residual is (a) a 1-2 byte regalloc nit in
// the first m_c->m_24->m_5c chain load (cl spreads to ecx where retail reuses
// eax) and (b) the reloc-masked callee symbols (CameraGeom DrawA/DrawB, PushView,
// ManagerTick) which pair once those engine fns are named.
RVA(0x000cefc0, 0xa2)
i32 CPlay::DrawWorldPresent() {
    if (m_c->m_24->m_mainPlane != 0) {
        ((CPlaneRender*)m_c->m_24->m_mainPlane)->CenterScrollB();
    }
    if (m_c->m_24->m_mainPlane != 0) {
        ((CPlaneRender*)m_c->m_24->m_mainPlane)->CenterScrollA();
    }
    m_c->m_childGroup->TickKillCues_159a70(1);
    if (m_c->m_24->m_mainPlane != 0) {
        ((CPlaneRender*)m_c->m_24->m_mainPlane)->CenterScrollB();
    }
    if (m_c->m_24->m_mainPlane != 0) {
        ((CPlaneRender*)m_c->m_24->m_mainPlane)->CenterScrollA();
    }
    m_c->m_childGroup->TickKillCues_159a70(1);
    m_c->m_24->VisitVisible(m_c->m_drawTarget->m_14, (CGameObjChain*)m_c->m_8);
    m_c->m_rendererB->PruneWorkers(m_c->m_drawTarget->m_14, m_c->m_drawTarget->m_18);
    m_4->PerFrameTick();
    return 1;
}

// ===========================================================================
// PresentAndFlush (0x0cba10) - the overlay-frame present path: bail unless the
// state is active (Vfunc3), restore the saved display mode if it drifted, then
// either notify-visible (region-1 gate) or push+present, and flush the draw
// surface. Migrated from engine_boundary (CPlay).
// @early-stop
// reloc-masked plateau (~97%): code bytes exact; residual is the call-rel32
// operands to the unmatched engine callees (m_guts ClampApply 0x500cb0, the
// PushView 0x15dc90, surface flush 0x13e850, m_4 RestoreVideoMode 0x8df00).
RVA(0x000cba10, 0xb0)
i32 CPlay::PresentAndFlush() {
    if (Vfunc3() == 0) {
        return 0;
    }
    CWorld* w = m_4w();
    i32 savedW = w->m_94;
    i32 liveW = w->m_8c;
    i32 savedH = w->m_98;
    i32 liveH = w->m_90;
    if (savedW != liveW || savedH != liveH) {
        if (((CGruntzMgr*)w)->SetVideoMode(savedW, savedH, 1) == 0) {
            return 0;
        }
    }
    if (m_guts != 0) {
        m_guts->Deactivate();
        if (m_region1Gate != 0) {
            NotifyVisibleEntities();
        } else {
            m_c->m_24->VisitVisible(m_c->m_drawTarget->m_14, (CGameObjChain*)m_c->m_8);
            m_c->m_rendererB->PruneWorkers(m_c->m_drawTarget->m_14, m_c->m_drawTarget->m_18);
        }
        m_c->m_drawTarget->m_10->m_surface->Flip(0); // 0x13e850
    }
    return 1;
}

// ===========================================================================
// CPlay::EnterOverlayDrag (0x0d6440) - arm the win/lose-overlay drag mode. If
// already armed (m_overlayDrag) bail. Otherwise latch m_overlayDrag, clear the
// world-ready / drag-snap gates, run the prep sub-step, and (only when `arg`==0)
// reset the guts subsystem (state/mode resync + two configure calls). Then arm
// the guts busy words (m_busyA=1, m_busyB=arg, m_548=1) and republish the level
// clock into m_savedClock. Migrated from engine_boundary (CPlay). Returns 1.
// ===========================================================================
RVA(0x000d6440, 0xd3)
i32 CPlay::EnterOverlayDrag(i32 arg) {
    if (m_overlayDrag != 0) {
        return 1;
    }
    m_overlayDrag = 1;
    m_worldReady = 0;
    m_dragSnapActive = 0;
    FlushPendingOps();
    if (arg == 0) {
        CStatusBarMgr* g = m_guts;
        if (g->m_position == 2) {
            g->RefreshState();
        }
        if (g->m_activeTab != 5) {
            g->SetTabState(5, 3);
        }
        g->SetTab(0x1fb, 1);
        g->Deactivate();
    }
    m_guts->BuildGameTabResumeButton(1);
    CStatusBarMgr* g = m_guts;
    g->m_toggleActive = 1;
    g->m_toggleHandle = arg;
    g->ResetWidgets(0);
    g->TryActivate();
    g->m_hlBusy = 1;
    g->Deactivate();
    m_savedClock = g_frameTime;
    return 1;
}

// ===========================================================================
// CPlay::ReleaseLevelOverlay (0x0d6560; ex "CGameModeObj" view, folded wave3-J) -
// if the overlay is live (m_overlayDrag), exit the guts overlay mode, clear the
// flag, and (unless multiplayer, g_gameReg->m_134 == 2) publish the saved level
// clock (m_savedClock) back to the running game clock. Returns 1. The single
// stack arg is unused. (The EnterOverlayDrag counterpart above.)
RVA(0x000d6560, 0x45)
i32 CPlay::ReleaseLevelOverlay(i32) {
    if (m_overlayDrag != 0) {
        CStatusBarMgr* worker = (CStatusBarMgr*)m_guts;
        m_overlayDrag = 0;
        worker->ExitMode();
        if (g_gameReg->m_134 != 2) {
            g_frameTime = m_savedClock;
        }
    }
    return 1;
}

// ForwardReady (0x0cee70): tiny vtable forwarder that tail-calls the slot-3 ready
// gate (Vfunc3). Out-of-line (retail emits it standalone; the inline member folded
// into its callers and never emitted).
RVA(0x000cee70, 0x5)
i32 CPlay::ForwardReady() {
    return Vfunc3();
}

// CPlay::PauseGame (0x0cee90) - vtable slot 24 (shared by CDemo/CMulti). Flush
// the pending mode ops, freeze the guts subsystem (passing whether we were
// running), clear the world-ready / drag-snap gates, and save the running game
// clock into m_savedClock. Migrated from engine_boundary (CPlay).
RVA(0x000cee90, 0x49)
i32 CPlay::PauseGame() {
    FlushPendingOps();
    if (m_paused) {
        m_guts->BuildGameTabResumeButton(0);
    } else {
        m_guts->BuildGameTabResumeButton(1);
    }
    m_worldReady = 0;
    m_dragSnapActive = 0;
    m_savedClock = g_frameTime;
    return 1;
}

// CPlay::ResumeGame (0x0cef00) - vtable slot 25. Step the guts subsystem, restore
// the saved game clock from m_savedClock, clear the paused flag, and (if the guts object
// is live) run its resume sub-step. Migrated from engine_boundary (CPlay).
RVA(0x000cef00, 0x39)
i32 CPlay::ResumeGame() {
    m_guts->BuildGameTabPauseButton();
    g_frameTime = m_savedClock;
    m_paused = 0;
    if (m_guts != 0) {
        m_guts->Deactivate();
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x0cef50 (spatially re-homed from src/Stub/BoundaryLowerMethods.cpp). Teardown:
// empty the +0x04 owner's +0xc8 CString; when +0x1c0 is live, run the +0x0c
// worker-mgr close (Method_158d20 -> Method_158e40) and dispatch the manager's
// ChangeState(3). Returns 1. Uses this TU's real CDDrawSubMgrPages/CGruntzMgr.
// (The Ccef50/CMid_cef50 orphan views are GONE - the receiver IS CPlay:
// +0x04 is the CState owner back-ptr (CGruntzMgr), +0x0c the CState holder
// (whose +0x04 draw target doubles as the CDDrawSubMgrPages worker manager,
// the same facet EnterMode drives) and +0x1c0 is CPlay::m_1c0, the Dispatch
// level-quiesce latch. FrameSlot28 calls it as QuitToMenu (thunk 0x21cb).)
RVA(0x000cef50, 0x46)
i32 CPlay::QuitToMenu() {
    // The +0xc8 call both prior passes guessed as a list dtor (~CPtrList / ~CObList)
    // is NEITHER: retail calls 0x1b9c69 == ?Empty@CString@@QAEXXZ (NAFXCW, anchored).
    // It clears the manager's world-file name - CGruntzMgr::m_strWorldFile @+0xc8.
    m_4->m_strWorldFile.Empty();
    if (m_1c0 != 0) {
        if (((CDDrawSubMgrPages*)m_c->m_drawTarget)->Method_158d20() != 0) {
            ((CDDrawSubMgrPages*)m_c->m_drawTarget)->Method_158e40();
        }
        m_4->ChangeState_8fab0(3);
    }
    return 1;
}

// CPlay::Vslot26 (0x0cfbb0) - vtable slot 38. Tail-forward to the game manager's
// per-frame state-manager tick (m_4->TickStateMgrs).
RVA(0x000cfbb0, 0x8)
void CPlay::Vslot26() {
    m_4->TickStateMgrs();
}

// CPlay::Vslot15 (0x0cfbd0) - vtable slot 21 (override of CState), the level-quiesce
// dispatch. On level index 0x20: latch the quiesce flags (m_1c0/m_40), stop the current
// zoned sound stream (m_c->m_28->m_2c, SoundStream::Stop) + flush the sound bank
// (m_4->m_48, CGruntzSoundZ::StopAndFlush), reset the two world teardown sub-objects
// (m_4->m_54/m_60), then PostMessageA WM_COMMAND 0x8023. Otherwise re-post 0x8023 while
// the m_1bc gate is set, else advance via the manager (m_4->Post, level index + 1). The
// PostMessageA calls go through the cached ::PostMessageA fn-ptr (bare 0x6c44c8, no
// import symbol). Re-homed from the ApiCaller stubs (was Dispatcher_0cfbd0::Dispatch).
// @early-stop
// regalloc-rotation wall (98.4%): logic, instruction selection, hop counts and
// order are byte-identical (llvm-objdump -dr / sema disasm --diff). The only
// residual is the scratch register that holds the reloaded m_4 (CWorld) pointer
// across the three quiesce sub-calls - retail colors it ecx/edx/eax, cl picks
// edx/eax/ecx (a symmetric reload coin-flip); the same rotation shifts the
// PostMessageA HWND chain's registers. Not source-steerable.
RVA(0x000cfbd0, 0x8f)
i32 CPlay::Vslot15() {
    if (m_levelIndex == 0x20) {
        m_1c0 = 1;
        m_40 = 1;
        SoundStream* stream = m_c->m_28->m_2c;
        if (stream) {
            stream->Stop();
        }
        m_4w()->m_48->StopAndFlush();
        m_4w()->m_54->Teardown();     // 0x28ab -> CWorldSoundSet::Teardown @0xb660
        m_4w()->m_60->ClearSprites(); // 0x244b -> CGruntSpawnConfig::ClearSprites @0x11af90
        ::PostMessageA((HWND)m_4w()->m_4->m_4, 0x111, 0x8023, 0);
        return 1;
    }
    if (m_1bc) {
        ::PostMessageA((HWND)m_4w()->m_4->m_4, 0x111, 0x8023, 0);
        return 1;
    }
    m_4->Post(m_levelIndex + 1);
    return 1;
}

// LoadCursorSprites (0xd0120): select + load the on-screen cursor sprite set for a tool
// `frame`. Early-outs when the requested (frame,flag) already matches the loaded pair.
// Frame 1..0x26 = the numeric chip cursor; 0 = the plain pointer; 0x66 = the flailing-grunt
// cursor (which also fires a booty cue + arms the +0x328 one-shot timer); 0xc8..0xe8 = the
// per-tool cursor table (a dense switch, one GAME_CURSORZ_* per tool). Each path loads via
// the reloc-masked LoadCursor helper (0x39ea, == CPlay::BeginGridWalk) and, on success,
// stamps the cursor state on CPlay's real members (m_dragClampMaxX/Y, m_dragEndNotify,
// m_levelId; the flailing-grunt one-shot reuses the booty-timer block at +0x328).
// @confidence: med
// @source: string-xref
// @early-stop
// ~93%: complete + correct (the early-out guard, all four dispatch arms - the 1..0x26
// chip range, the pointer, the flailing-grunt cue arm, and the full 33-case tool-cursor
// switch - all match, every GAME_CURSORZ_*/helper named). Residual walls: (1) the tool
// switch's range check - retail emits the signed two-bound form (cmp eax,0xc8;jl + sub;cmp
// 0x20;ja) where cl folds it to the single unsigned check; (2) the jump-table dispatch is
// the delinker's `jmp [eax*4+$L]` reloc-typing vs cl's separate DIR32 base (same bytes);
// (3) the three prefix blocks reload `frame` into a different scratch reg (edx vs eax) for
// the trailing m_2f8 store. All logic + externs/strings named.
RVA(0x000d0120, 0x5d8)
i32 CPlay::LoadCursorSprites(i32 frame, i32 flag) {
    if (this->m_levelId == frame && flag == this->m_dragEndNotify) {
        return 1;
    }
    if (frame >= CURSOR_CHIP_FIRST && frame <= CURSOR_CHIP_LAST) {
        if (this->BeginGridWalk("GAME_INGAMEICONZ_NORMCHIPZ", frame, 0, 0x64, 0) == 0) {
            return 0;
        }
        if (this->m_scrollSink != 0) {
            this->m_scrollSink->m_flags |= 1;
        }
        this->m_dragClampMaxX = 0;
        this->m_dragClampMaxY = 0;
        this->m_dragInhibit2 = 1;
        this->m_dragEndNotify = 0;
        this->m_levelId = frame;
        return 1;
    }
    if (frame == CURSOR_POINTER) {
        if (this->BeginGridWalk("GAME_CURSORZ_POINTER", 1, 1, 0x64, 0) == 0) {
            return 0;
        }
        if (this->m_scrollSink != 0) {
            this->m_scrollSink->m_flags &= ~1;
        }
        this->m_dragClampMaxX = 0x10;
        this->m_dragClampMaxY = 0x10;
        this->m_dragEndNotify = 0;
        this->m_levelId = frame;
        return 1;
    }
    if (frame == CURSOR_FLAILINGGRUNT) {
        if (this->BeginGridWalk("GAME_CURSORZ_FLAILINGGRUNT", 1, 1, 0x64, 1) == 0) {
            return 0;
        }
        if (this->m_scrollSink != 0) {
            this->m_scrollSink->m_flags |= 1;
        }
        this->m_dragClampMaxX = 0;
        this->m_dragClampMaxY = 0;
        this->m_dragInhibit1 = 1;
        this->m_dragEndNotify = 0;
        ((CGruntSpawnConfig*)g_gameReg->m_cueSink)->SpawnVoiceDriver(0, 0x33e, -1, 1, -1, -1);
        this->m_bootyInterval = BOOTY_INTERVAL_MS;
        this->m_bootyIntervalHi = 0;
        this->m_bootyTimerLo = g_frameTime;
        this->m_bootyTimerHi = 0;
        this->m_levelId = frame;
        return 1;
    }
    switch (frame) {
        case CURSOR_TOOL_HANDZ:
            if (this->BeginGridWalk("GAME_CURSORZ_HANDZ", 1, flag, 0x64, 1) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_BOMBZ:
            if (this->BeginGridWalk("GAME_CURSORZ_BOMBZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_BOOMERANGZ:
            if (this->BeginGridWalk("GAME_CURSORZ_BOOMERANGZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_BRICKZ:
            if (this->BeginGridWalk("GAME_CURSORZ_BRICKZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_CLUBZ:
            if (this->BeginGridWalk("GAME_CURSORZ_CLUBZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_GAUNTLETZ:
            if (this->BeginGridWalk("GAME_CURSORZ_GAUNTLETZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_GLOVEZ:
            if (this->BeginGridWalk("GAME_CURSORZ_GLOVEZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_GOOBERZ:
            if (this->BeginGridWalk("GAME_CURSORZ_GOOBERZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_GRAVITYBOOTZ:
            if (this->BeginGridWalk("GAME_CURSORZ_GRAVITYBOOTZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_GUNHATZ:
            if (this->BeginGridWalk("GAME_CURSORZ_GUNHATZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_NERFGUNZ:
            if (this->BeginGridWalk("GAME_CURSORZ_NERFGUNZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_ROCKZ:
            if (this->BeginGridWalk("GAME_CURSORZ_ROCKZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_SHIELDZ:
            if (this->BeginGridWalk("GAME_CURSORZ_SHIELDZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_SHOVELZ:
            if (this->BeginGridWalk("GAME_CURSORZ_SHOVELZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_SPRINGZ:
            if (this->BeginGridWalk("GAME_CURSORZ_SPRINGZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_SPYZ:
            if (this->BeginGridWalk("GAME_CURSORZ_SPYZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_SWORDZ:
            if (this->BeginGridWalk("GAME_CURSORZ_SWORDZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_TIMEBOMBZ:
            if (this->BeginGridWalk("GAME_CURSORZ_TIMEBOMBZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_TOOBZ:
            if (this->BeginGridWalk("GAME_CURSORZ_TOOBZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_WANDZ:
            if (this->BeginGridWalk("GAME_CURSORZ_WANDZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_WARPSTONEZ:
            if (this->BeginGridWalk("GAME_CURSORZ_WARPSTONEZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_WELDERZ:
            if (this->BeginGridWalk("GAME_CURSORZ_WELDERZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_WINGZ:
            if (this->BeginGridWalk("GAME_CURSORZ_WINGZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_BABYWALKERZ:
            if (this->BeginGridWalk("GAME_CURSORZ_BABYWALKERZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_BEACHBALLZ:
            if (this->BeginGridWalk("GAME_CURSORZ_BEACHBALLZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_BIGWHEELZ:
            if (this->BeginGridWalk("GAME_CURSORZ_BIGWHEELZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_GOKARTZ:
            if (this->BeginGridWalk("GAME_CURSORZ_GOKARTZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_JACKINTHEBOXZ:
            if (this->BeginGridWalk("GAME_CURSORZ_JACKINTHEBOXZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_JUMPROPEZ:
            if (this->BeginGridWalk("GAME_CURSORZ_JUMPROPEZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_POGOSTICKZ:
            if (this->BeginGridWalk("GAME_CURSORZ_POGOSTICKZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_SCROLLZ:
            if (this->BeginGridWalk("GAME_CURSORZ_SCROLLZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_SQUEAKTOYZ:
            if (this->BeginGridWalk("GAME_CURSORZ_SQUEAKTOYZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_YOYOZ:
            if (this->BeginGridWalk("GAME_CURSORZ_YOYOZ", 1, flag, 0x64, 0) == 0) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    if (this->m_scrollSink != 0) {
        this->m_scrollSink->m_flags |= 1;
    }
    this->m_dragClampMaxX = 0;
    this->m_dragClampMaxY = 0;
    this->m_dragEndNotify = flag;
    this->m_levelId = frame;
    return 1;
}

// CPlay::LoadScrollSpeedOptions (0xd12b0): lazy-load the bute-configured scroll
// speed range on first use, then run the per-frame edge auto-scroll. Four edge
// zones (left/right/top/bottom); each times a mouse-at-edge dwell against
// timeGetTime and nudges the plane-geom scroll offset by (elapsed*speed/100)
// clamped to 100 px, committing the new offset when anything moved. speed =
// (int)((double)m_4->m_124 * 0.01 * range + min). The bute getter, timeGetTime
// ptr and the ApplyScroll tail are reloc-masked; only offsets + code bytes bind.
// @early-stop
// scheduling wall (85.9%, from 0% stub): logic + all four edge blocks byte-faithful.
// Residual is MSVC's interleave of the geom pointer-chase (sx/sy loads) into the
// float speed-computation FPU latency gaps (fild/fmul/fimul/fiadd/ftol) + the
// trailing nop padding - not source-steerable (zero-register-pinning family).
extern "C" u8 g_scrollLoadFlags;      // 0x64c01c  lazy-load bitset (bit0 min, bit1 max)
extern "C" i32 g_scrollMinSpeed;      // 0x64c274  cached MinScrollSpeed
extern "C" i32 g_scrollSpeedRange;    // 0x64c270  cached (Max - Min)
extern CButeMgr g_buteMgr;            // 0x6453d8
extern "C" double g_scrollSpeedScale; // 0x5eaa10  (== 0.01)

// (ScrollGeom/ScrollWorld are GONE - the geom is the canonical CLevelPlane
// (m_originX/m_originY, the integer scroll origin) reached through the CWorld
// holder chain, and the +0x124 speed base is the registry's m_scrollSpeed
// (CWorld::m_scrollSpeed - one singleton).)
RVA(0x000d12b0, 0x2d5)
i32 CPlay::LoadScrollSpeedOptions() {
    if (!(g_scrollLoadFlags & 1)) {
        g_scrollLoadFlags |= 1;
        g_scrollMinSpeed = g_buteMgr.GetInt("Optionz", "MinScrollSpeed");
    }
    if (!(g_scrollLoadFlags & 2)) {
        g_scrollLoadFlags |= 2;
        g_scrollSpeedRange = g_buteMgr.GetInt("Optionz", "MaxScrollSpeed")
                             - g_buteMgr.GetInt("Optionz", "MinScrollSpeed");
    }

    CPlay* self = this;
    CWorld* w = m_4w();
    i32 changed = 0;
    i32 speed = (i32)((double)w->m_scrollSpeed * g_scrollSpeedScale * g_scrollSpeedRange
                      + g_scrollMinSpeed);
    CLevelPlane* g = w->m_30->m_24->m_mainPlane;
    i32 sx = g->m_originX;
    i32 sy = g->m_originY;
    i32 extentX = w->m_8c;
    i32 extentY = w->m_90;

    // LEFT edge
    if (self->m_cursorX < 0xc || (self->m_scrollEdgeLock & 1)) {
        if (self->m_scrollEdgeActive & 1) {
            i32 d = (::timeGetTime() - self->m_lastScrollTimeX) * speed / 100;
            if (d) {
                if (d > 0x64) {
                    d = 0x64;
                }
                sx -= d;
                self->m_lastScrollTimeX = ::timeGetTime();
                changed = 1;
            }
        } else {
            self->m_scrollEdgeActive |= 1;
            self->m_lastScrollTimeX = ::timeGetTime();
        }
    } else {
        self->m_scrollEdgeActive &= ~1;
    }

    // RIGHT edge
    if (self->m_cursorX > extentX - 0xc || (self->m_scrollEdgeLock & 4)) {
        if (self->m_scrollEdgeActive & 4) {
            i32 d = (::timeGetTime() - self->m_lastScrollTimeX) * speed / 100;
            if (d) {
                if (d > 0x64) {
                    d = 0x64;
                }
                sx += d;
                self->m_lastScrollTimeX = ::timeGetTime();
                changed = 1;
            }
        } else {
            self->m_scrollEdgeActive |= 4;
            self->m_lastScrollTimeX = ::timeGetTime();
        }
    } else {
        self->m_scrollEdgeActive &= ~4;
    }

    // TOP edge
    if (self->m_cursorY < 0xf || (self->m_scrollEdgeLock & 2)) {
        if (self->m_scrollEdgeActive & 2) {
            i32 d = (::timeGetTime() - self->m_lastScrollTimeY) * speed / 100;
            if (d) {
                if (d > 0x64) {
                    d = 0x64;
                }
                sy -= d;
                self->m_lastScrollTimeY = ::timeGetTime();
                changed = 1;
            }
        } else {
            self->m_scrollEdgeActive |= 2;
            self->m_lastScrollTimeY = ::timeGetTime();
        }
    } else {
        self->m_scrollEdgeActive &= ~2;
    }

    // BOTTOM edge
    if (self->m_cursorY > extentY - 0xf || (self->m_scrollEdgeLock & 8)) {
        if (self->m_scrollEdgeActive & 8) {
            i32 d = (::timeGetTime() - self->m_lastScrollTimeY) * speed / 100;
            if (d) {
                if (d > 0x64) {
                    d = 0x64;
                }
                sy += d;
                self->m_lastScrollTimeY = ::timeGetTime();
                changed = 1;
            }
        } else {
            self->m_scrollEdgeActive |= 8;
            self->m_lastScrollTimeY = ::timeGetTime();
        }
    } else {
        self->m_scrollEdgeActive &= ~8;
    }

    if (changed) {
        self->ResetGoals(sx, sy);
    }
    return 1;
}

// BuildGruntTypeNameTable (0xdc6d0): map a grunt-type id to its bute namespace key
// via a 58-case jump table (NORMALGRUNT default), then register it through the shared
// namespace-loader tail (BindWarlordName == the 0x2bc1 CNamespaceLoader thunk). The
// TOOB case is special: it registers TOOBGRUNT first and, only if that succeeds, ALSO
// registers TOOBWATERGRUNT (a separate `return`, not a `break`, so a2/a3/a4 stay in
// edi/ebx/ebp local to the TOOB block instead of being hoisted for the whole fn). The
// CString name temp forces the /GX EH frame.
// @early-stop
// jump-table-data-overlap wall (33.3%, from 0% stub): the full body is byte-exact vs
// retail (verified llvm-objdump -dr base vs target — prologue, dispatch, all 58 case
// pushes, the TOOB/TOOBWATER special path, and the shared destruct-tail all match). The
// residual is the 194-byte switch data (58-entry index byte-table + 34-slot jump table)
// which cl emits as separate $L symbols vs the delinker inlining it into the fn symbol
// at fn+0x218/+0x2a4; the table DATA + the 2 dispatch reloc operands never pair. Not
// source-steerable (docs/patterns/jumptable-data-overlap.md, cf. LoadPowerupIconSprites).
RVA(0x000dc6d0, 0x215)
i32 CPlay::BuildGruntTypeNameTable(i32 typeIdx, i32 a2, i32 a3, i32 a4) {
    CString name("NORMALGRUNT");
    switch (typeIdx) {
        case GRUNT_TYPE_BOMB:
            name = "BOMBGRUNT";
            break;
        case GRUNT_TYPE_BOOMERANG:
            name = "BOOMERANGGRUNT";
            break;
        case GRUNT_TYPE_BRICK:
            name = "BRICKGRUNT";
            break;
        case GRUNT_TYPE_CLUB:
            name = "CLUBGRUNT";
            break;
        case GRUNT_TYPE_GAUNTLETZ:
            name = "GAUNTLETZGRUNT";
            break;
        case GRUNT_TYPE_GLOVEZ:
            name = "GLOVEZGRUNT";
            break;
        case GRUNT_TYPE_GOOBER:
            name = "GOOBERGRUNT";
            break;
        case GRUNT_TYPE_GRAVITYBOOTZ:
            name = "GRAVITYBOOTZGRUNT";
            break;
        case GRUNT_TYPE_GUNHAT:
            name = "GUNHATGRUNT";
            break;
        case GRUNT_TYPE_NERFGUN:
            name = "NERFGUNGRUNT";
            break;
        case GRUNT_TYPE_ROCK:
            name = "ROCKGRUNT";
            break;
        case GRUNT_TYPE_SHIELD:
            name = "SHIELDGRUNT";
            break;
        case GRUNT_TYPE_SHOVEL:
            name = "SHOVELGRUNT";
            break;
        case GRUNT_TYPE_SPRING:
            name = "SPRINGGRUNT";
            break;
        case GRUNT_TYPE_SPY:
            name = "SPYGRUNT";
            break;
        case GRUNT_TYPE_SWORD:
            name = "SWORDGRUNT";
            break;
        case GRUNT_TYPE_TIMEBOMB:
            name = "TIMEBOMBGRUNT";
            break;
        case GRUNT_TYPE_TOOB:
            name = "TOOBGRUNT";
            if (((CNamespaceLoader*)this)->BuildAssetNamespacePrefixes(name, a2, a3, a4) == 0) {
                return 0;
            }
            name = "TOOBWATERGRUNT";
            return ((CNamespaceLoader*)this)->BuildAssetNamespacePrefixes(name, a2, a3, a4);
        case GRUNT_TYPE_WAND:
            name = "WANDGRUNT";
            break;
        case GRUNT_TYPE_WARPSTONE:
            name = "WARPSTONEGRUNT";
            break;
        case GRUNT_TYPE_WELDER:
            name = "WELDERGRUNT";
            break;
        case GRUNT_TYPE_WINGZ:
            name = "WINGZGRUNT";
            break;
        case GRUNT_TYPE_BABYWALKER:
            name = "BABYWALKERGRUNT";
            break;
        case GRUNT_TYPE_BEACHBALL:
            name = "BEACHBALLGRUNT";
            break;
        case GRUNT_TYPE_BIGWHEEL:
            name = "BIGWHEELGRUNT";
            break;
        case GRUNT_TYPE_GOKART:
            name = "GOKARTGRUNT";
            break;
        case GRUNT_TYPE_JACKINTHEBOX:
            name = "JACKINTHEBOXGRUNT";
            break;
        case GRUNT_TYPE_JUMPROPE:
            name = "JUMPROPEGRUNT";
            break;
        case GRUNT_TYPE_POGOSTICK:
            name = "POGOSTICKGRUNT";
            break;
        case GRUNT_TYPE_SCROLL:
            name = "SCROLLGRUNT";
            break;
        case GRUNT_TYPE_SQUEAKTOY:
            name = "SQUEAKTOYGRUNT";
            break;
        case GRUNT_TYPE_YOYO:
            name = "YOYOGRUNT";
            break;
        case GRUNT_TYPE_HAREKRISHNA:
            name = "HAREKRISHNAGRUNT";
            break;
        case GRUNT_TYPE_REAPER:
            name = "REAPERGRUNT";
            break;
    }
    return ((CNamespaceLoader*)this)->BuildAssetNamespacePrefixes(name, a2, a3, a4);
}

// ===========================================================================
// Per-level resource loaders (trace-discovered CPlay __thiscall cluster).
// ===========================================================================
// A small family of asset-namespace loaders. Each reaches the game resource
// registry through this->m_c (->m_10 image/tile registry [virtual install slot
// 18 = +0x48], ->m_28 sound registry, ->m_2c animation registry) and looks the
// level's named set up off a bank source (this->m_levelBank or this->m_gameBank, both expose
// LookupSet @0x13bae0). LoadImageBanks (the parent) caches the GRUNTZ/GAME banks
// into this->m_gruntzBank/m_gameBank first; the per-type loaders then read this->m_gameBank as their
// source. Helpers are modeled NO-body so their `call`s reloc-mask; only OFFSETS +
// code bytes are load-bearing. The loaders touch sub-object faces (m_c->m_10,
// m_8->Lookup) that Render's matched member typing models differently, so each
// casts `this` to the self-contained CPlayRes view below (a struct-view-of-this
// overlay - matching-neutral, keeps Render untouched).

extern i32 g_resourceInstallActive; // ?g_resourceInstallActive@@3HA @0x6bf37c (mangled-name match)
extern "C" char g_emptyString[];    // _g_emptyString @0x6293f4

// CResSource (the named-set source: this->m_levelBank / m_gameBank, the banks cached
// in m_gruntzBank/m_gameBank; LookupSet 0x13bae0) and CBankMgr (the bank manager at
// this->m_8; Lookup 0x13c030) are the shared CState bank/source facet - one definition
// in <Gruntz/BankMgr.h> (also used by CSplashState/BacklogStateLoaders). Included here
// in-place (declaration order preserved) rather than at the top so the fold stays
// codegen-neutral for this TU.
#include <Gruntz/BankMgr.h>
// (0xfa8f0 is CState::RetireScene - inherited by CPlay; the former local CSoundFxEmitter
//  view is dissolved, the loader calls it cast-free on the CPlay `self`.)
// (0xfa150 cleanup is CGameModeBase::BaseCleanup - reached via the CState<->CGameModeBase
// reinterpret at offset 0, the same pattern CState.h uses; no local view needed.)
// The loader family reaches its resource state directly through `this` (a CPlay):
// the bank manager (CState::m_8), the level/GRUNTZ/GAME banks (CState::m_levelBank/
// m_gruntzBank/m_gameBank) and the shared CSpriteFactoryHolder resource registries (CState::m_c->m_10/m_28/m_2c).

// LoadImageBanks (0x0cffe0) - cache the GRUNTZ + GAME asset banks off m_8 into
// m_30/m_34; the int (BOOL) return reuses the just-loaded value at each guard.
RVA(0x000cffe0, 0x3c)
i32 CPlay::LoadImageBanks() {
    CPlay* self = this;
    if (!self->m_8) {
        return 0;
    }
    self->m_gruntzBank = (CSymTab*)((CSymParser*)self->m_8)->ResolvePath("GRUNTZ");
    if (!self->m_gruntzBank) {
        return 0;
    }
    self->m_gameBank = (CSymTab*)((CSymParser*)self->m_8)->ResolvePath("GAME");
    return self->m_gameBank != 0;
}

// LoadActionTileSprites (0x0db600) - register the ACTION/BACK namespaces then
// install the level's TILEZ set through the +0x48 virtual slot. int (BOOL) return:
// each guard `return 0` reuses the just-zeroed eax, the success path is `mov
// eax,1`; a void return tail-merges the bare epilogues and never emits eax=1.
RVA(0x000db600, 0x8f)
i32 CPlay::LoadActionTileSprites(i32 force) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    if (!force && ((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("ACTION")) {
        return 1;
    }

    ((CDDrawWorkerRegistry*)self->m_c->m_10)->RemoveKeysEqual_155360("ACTION", g_emptyString);
    ((CDDrawWorkerRegistry*)self->m_c->m_10)->RemoveKeysEqual_155360("BACK", g_emptyString);
    g_resourceInstallActive = 0;

    void* tiles = (self->m_levelBank)->ResolvePath("TILEZ");
    if (!tiles) {
        return 0;
    }
    self->m_c->m_10->InstallTree(tiles, g_emptyString, "_");
    return 1;
}

// LoadLevelSounds (0x0db6c0) - register the LEVEL namespace then install the
// level's SOUNDZ set through m_c->m_28 (non-virtual). Single Register, no install-gate
// reset. Same int-return idiom as its siblings.
RVA(0x000db6c0, 0x70)
i32 CPlay::LoadLevelSounds(i32 force) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    if (!force && ((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("LEVEL")) {
        return 1;
    }

    ((CDDrawSubMgrLeafScan*)self->m_c->m_28)->RemoveKeysEqual_157c70("LEVEL", "_");

    void* sounds = (self->m_levelBank)->ResolvePath("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    ((CDDrawSubMgrLeafScan*)self->m_c->m_28)->ScanTree_157ee0((CSymTab*)sounds, "LEVEL", "_");
    return 1;
}

// The "ANIZ" symtab-path tag SyncLevelKey resolves (owner-TU .data definition; the
// canonical extern is in <Globals.h>). RVA-ascending: 0x213054 follows the name arrays.
DATA(0x00213054)
char g_dat613054[8] = "ANIZ";

// ---------------------------------------------------------------------------
// 0x0db750 (spatially re-homed from src/Stub/BoundaryLowerMethods.cpp). "LEVEL"
// config sync through the +0x0c owner's +0x2c config leaf (the 0x152xxx registry
// API - distinct from this TU's 0x157xxx CDDrawSubMgrLeafScan). @orphan (owner
// unrecovered; the 0x152xxx leaf + symtab kept as unique local views).
// m_2c is a CDDrawSubMgrAni : CDDrawSubMgrLeaf (0x152xxx config leaf); m_28 is a
// CSymTab (ResolvePath @0x13bae0). Reached via cast at the call sites (real classes).
// CPlay::LoadLevelAnims (0x0db750; ex the `Cdb750::SyncLevelKey` orphan view - its
// +0x0c holder is CState::m_c and its +0x28 symtab is CState::m_levelBank, so the
// receiver IS this CPlay): the LEVEL-namespace anim loader, the missing sibling of
// LoadLevelSounds/LoadLevelImages. LoadByMode runs it as init step I (thunk 0x2c07).
RVA(0x000db750, 0x70)
i32 CPlay::LoadLevelAnims(i32 force) {
    if (m_c == 0) {
        return 0;
    }
    if (force == 0) {
        if (((CDDrawSubMgrLeaf*)m_c->m_animRegistry)->HasKeyPrefix_152c50("LEVEL") != 0) {
            return 1;
        }
    }
    ((CDDrawSubMgrLeaf*)m_c->m_animRegistry)
        ->RemoveKeysEqual_1527d0("LEVEL", (const char*)&g_dat60b588);
    void* e = m_levelBank->ResolvePath((const char*)&g_dat613054);
    if (e == 0) {
        return 0;
    }
    ((CDDrawSubMgrAni*)m_c->m_animRegistry)
        ->ScanTree_152ad0((CSymTab*)e, "LEVEL", (const char*)&g_dat60b588);
    return 1;
}

// LoadLevelImages (0x0db7e0) - sibling of LoadActionTileSprites; a single Register
// (LEVEL), then install the level's IMAGEZ set through the +0x48 virtual slot.
// Brackets the install with two install-active resets.
RVA(0x000db7e0, 0x84)
i32 CPlay::LoadLevelImages(i32 force) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    if (!force && ((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("LEVEL")) {
        return 1;
    }

    ((CDDrawWorkerRegistry*)self->m_c->m_10)->RemoveKeysEqual_155360("LEVEL", "_");
    g_resourceInstallActive = 0;

    void* images = (self->m_levelBank)->ResolvePath("IMAGEZ");
    if (!images) {
        return 0;
    }
    self->m_c->m_10->InstallTree(images, "LEVEL", "_");
    g_resourceInstallActive = 0;
    return 1;
}

// LoadGameImages (0x0db8a0) - the GAME-namespace image loader. No force gate, no
// Register; reads m_gameBank (the cached GAME bank) for the IMAGEZ set, installs through
// the +0x48 virtual slot. Brackets the install with the install-active flag = 1 then 0.
RVA(0x000db8a0, 0x67)
i32 CPlay::LoadGameImages(i32 force) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    if (((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("GAME")) {
        return 1;
    }

    g_resourceInstallActive = 1;
    void* images = (self->m_gameBank)->ResolvePath("IMAGEZ");
    if (!images) {
        return 0;
    }
    self->m_c->m_10->InstallTree(images, "GAME", "_");
    g_resourceInstallActive = 0;
    return 1;
}

// LoadGameSounds (0x0db930) - the GAME-namespace sound loader. Reads m_gameBank for the
// SOUNDZ set and installs through m_c->m_28 (non-virtual). No Register, no install-gate.
RVA(0x000db930, 0x53)
i32 CPlay::LoadGameSounds(i32 force) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    if (((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("GAME")) {
        return 1;
    }

    void* sounds = (self->m_gameBank)->ResolvePath("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    ((CDDrawSubMgrLeafScan*)self->m_c->m_28)->ScanTree_157ee0((CSymTab*)sounds, "GAME", "_");
    return 1;
}

// LoadGameAnims (0x0db9b0) - the GAME-namespace animation loader. Reads m_gameBank for
// the ANIZ set and installs through m_c->m_2c (non-virtual). No Register/install-gate.
RVA(0x000db9b0, 0x53)
i32 CPlay::LoadGameAnims(i32 force) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    if (((CDDrawSubMgrLeaf*)self->m_c->m_animRegistry)->HasKeyPrefix_152c50("GAME")) {
        return 1;
    }

    void* anims = (self->m_gameBank)->ResolvePath("ANIZ");
    if (!anims) {
        return 0;
    }
    ((CDDrawSubMgrAni*)self->m_c->m_animRegistry)->ScanTree_152ad0((CSymTab*)anims, "GAME", "_");
    return 1;
}

// ===========================================================================
// CPlay::BuildMusicCategoryTable (0x0dba30) - install the level's + game's XMI music
// categories into the sound manager's category table (m_4->m_48). For each of the
// two MIDIZ sources (level m_levelBank, game m_gameBank) it resolves a fixed list of category
// names ("AMBIENT0/1", "INTRO0/1" for the level; "POWERUP", "CURSE", "MONOLITH" for
// the game) as 'XMI' resources and, if the resolved entry loads, installs it under
// its name. The arg (force flag) is unused. __thiscall, ret 4.
// ===========================================================================
// The resolved music-category entry (CSymTab::Insert result) IS the canonical
// CParseSource (<Gruntz/ParseSource.h>) - the entry IS the parse source, and its
// BeginParse @0x139960 loads it. The local re-declaration is dissolved; its "+0xc install
// key" was a MIS-NAMING of the canonical m_length: the call it feeds is
// CreateBank(buffer, LENGTH, name), so +0x0c is the byte length, exactly as the stream
// methods (SetPos/Read) already read it.
// The destination category table is the CGruntzSoundZ manager at m_4->m_48
// (StopAndFlush @0x138530, CreateBank @0x138670; GruntzSoundZ.h).
// (CMusicOwner is GONE - the destination is m_4w()->m_48, the typed
// CGruntzSoundZ on the canonical CWorld view.)

#define MUSIC_TAG_XMI 0x584d49 // 'XMI'

RVA(0x000dba30, 0x1ca)
i32 CPlay::BuildMusicCategoryTable(i32) {
    m_4w()->m_48->StopAndFlush();

    CSymTab* levelSet = (CSymTab*)m_levelBank->ResolvePath("MIDIZ");
    if (levelSet) {
        CParseSource* e = (CParseSource*)levelSet->Insert("AMBIENT0", (void*)MUSIC_TAG_XMI);
        if (e) {
            void* res = (void*)e->BeginParse();
            if (res) {
                m_4w()->m_48->CreateBank(res, e->m_length, "AMBIENT0");
            }
        }
        e = (CParseSource*)levelSet->Insert("AMBIENT1", (void*)MUSIC_TAG_XMI);
        if (e) {
            void* res = (void*)e->BeginParse();
            if (res) {
                m_4w()->m_48->CreateBank(res, e->m_length, "AMBIENT1");
            }
        }
        e = (CParseSource*)levelSet->Insert("INTRO0", (void*)MUSIC_TAG_XMI);
        if (e) {
            void* res = (void*)e->BeginParse();
            if (res) {
                m_4w()->m_48->CreateBank(res, e->m_length, "INTRO0");
            }
        }
        e = (CParseSource*)levelSet->Insert("INTRO1", (void*)MUSIC_TAG_XMI);
        if (e) {
            void* res = (void*)e->BeginParse();
            if (res) {
                m_4w()->m_48->CreateBank(res, e->m_length, "INTRO1");
            }
        }
    }

    CSymTab* gameSet = (CSymTab*)m_gameBank->ResolvePath("MIDIZ");
    if (gameSet) {
        CParseSource* e = (CParseSource*)gameSet->Insert("POWERUP", (void*)MUSIC_TAG_XMI);
        if (e) {
            void* res = (void*)e->BeginParse();
            if (res) {
                m_4w()->m_48->CreateBank(res, e->m_length, "POWERUP");
            }
        }
        e = (CParseSource*)gameSet->Insert("CURSE", (void*)MUSIC_TAG_XMI);
        if (e) {
            void* res = (void*)e->BeginParse();
            if (res) {
                m_4w()->m_48->CreateBank(res, e->m_length, "CURSE");
            }
        }
        e = (CParseSource*)gameSet->Insert("MONOLITH", (void*)MUSIC_TAG_XMI);
        if (e) {
            void* res = (void*)e->BeginParse();
            if (res) {
                m_4w()->m_48->CreateBank(res, e->m_length, "MONOLITH");
            }
        }
    }
    return 1;
}

// ===========================================================================
// CPlay::LoadGruntSoundNamespaces (0x0dd830) - register the per-grunt sound
// namespaces (GRUNTZ_<X>) into the level sound registry (m_c->m_28), sourcing each
// SOUNDZ_<X> set off the GRUNTZ bank (m_gruntzBank). The first three namespaces load silently;
// the last four also tick the load-notify object (arg) when present. __thiscall.
// ===========================================================================
// The per-namespace load-notify sink (arg): OnLoaded() probes its ready flags and
// posts a progress message. External/reloc-masked.
// The load-notify sink IS a CMulti (OnLoaded @0xbc420 = CMulti::AckJoinFailure); the
// canonical class comes from <Gruntz/Multi.h>, now included at the top - the minimal
// local re-declaration is dissolved.

RVA(0x000dd830, 0x1e3)
i32 CPlay::LoadGruntSoundNamespaces(CMulti* notify) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }

    if (!((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("GRUNTZ_NORMALGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_NORMALGRUNT");
        if (s) {
            ((CDDrawSubMgrLeafScan*)self->m_c->m_28)
                ->ScanTree_157ee0((CSymTab*)s, "GRUNTZ_NORMALGRUNT", "_");
        }
    }
    if (!((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("GRUNTZ_DEATHZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_DEATHZ");
        if (s) {
            ((CDDrawSubMgrLeafScan*)self->m_c->m_28)
                ->ScanTree_157ee0((CSymTab*)s, "GRUNTZ_DEATHZ", "_");
        }
    }
    if (!((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("GRUNTZ_ENTRANCEZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_ENTRANCEZ");
        if (s) {
            ((CDDrawSubMgrLeafScan*)self->m_c->m_28)
                ->ScanTree_157ee0((CSymTab*)s, "GRUNTZ_ENTRANCEZ", "_");
        }
    }
    if (!((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("GRUNTZ_EXITZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_EXITZ");
        if (s) {
            ((CDDrawSubMgrLeafScan*)self->m_c->m_28)
                ->ScanTree_157ee0((CSymTab*)s, "GRUNTZ_EXITZ", "_");
        }
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("GRUNTZ_GRUNTPUDDLE")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_GRUNTPUDDLE");
        if (s) {
            ((CDDrawSubMgrLeafScan*)self->m_c->m_28)
                ->ScanTree_157ee0((CSymTab*)s, "GRUNTZ_GRUNTPUDDLE", "_");
        }
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("GRUNTZ_PICKUPS")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_PICKUPS");
        if (s) {
            ((CDDrawSubMgrLeafScan*)self->m_c->m_28)
                ->ScanTree_157ee0((CSymTab*)s, "GRUNTZ_PICKUPS", "_");
        }
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawSubMgrLeafScan*)self->m_c->m_28)->HasKeyEqual_1583c0("GRUNTZ_BOMBGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_BOMBGRUNT");
        if (s) {
            ((CDDrawSubMgrLeafScan*)self->m_c->m_28)
                ->ScanTree_157ee0((CSymTab*)s, "GRUNTZ_BOMBGRUNT", "_");
        }
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    return 1;
}
// ===========================================================================
// CNamespaceLoader::BuildAssetNamespacePrefixes (0x0dca70; re-homed from the former
// assetnamespaceprefixes unit, waveP - TU_MIGRATION MOVE row `0x0dca70 -> 0xd5960 play`).
// The GRUNTZ_ per-object asset-namespace loader (/GX). The three worker-registry views
// (m_c->m_10/m_28/m_2c) reuse THIS TU's CDDrawWorkerRegistry / canonical CDDrawSubMgrLeafScan
// / CDDrawSubMgrAni (augmented above with LoadTree/HasKeyEqual/... ). g_gameReg is the
// 0x64556c singleton, g_resourceInstallActive is reused. Callees reloc-masked.
// ===========================================================================
extern i32 g_resourceInstallActive; // 0x6bf37c

// (AssetRoot is GONE - CNamespaceLoader::m_c is the typed CSpriteFactoryHolder.
// GRAssetMgr is GONE - the preview rect source is m_world->m_24 (CGameLevel) and
// "DrawPreview" (thunk 0x1c5d) IS EngStr_DrawText @0x115440. "FinishAssetLoad"
// (thunk 0x35e4) IS CMulti::AckJoinFailure @0xbc420 with ecx = the finishGate
// notify object - the same dropped-`this` pattern as the loaders.)
extern "C" CGameRegistry* g_gameReg; // *0x64556c (def: GruntzMgr.cpp)

// @source: decomp-xref
// @early-stop
// /GX shared-return block-layout wall (~90.5%): the logic, both mode branches, the
// three GRUNTZ_ namespace registrations, the lighting/preview draw, g_resourceInstallActive
// gating and the vtable-LoadTree/direct-load split are all byte-identical to retail.
// The residual is where cl places the single return-cleanup epilogue: the recompile
// makes the ResolvePath-fail path the fall-through (epilogue mid-body at ~0x173) and
// reaches LoadTree by branch, while retail makes the success path the fall-through
// (epilogue at the tail); that block-ordering drift cascades stack-offset/branch bytes
// through the back half. Both `return 0`/single-`goto done` spellings compile identical
// (block-layout heuristic, not source-steerable). The rest is reloc/EH scoring artifact
// (differently-named externs, the __except handler-index push, __imp__CopyRect vs the
// 0x6c44bc pointer). Verified llvm-objdump -dr, base main body vs delinked target.
RVA(0x000dca70, 0x4a4)
i32 CNamespaceLoader::BuildAssetNamespacePrefixes(
    const CString& name,
    i32 mode,
    i32 lightGate,
    i32 finishGate
) {
    i32 result;
    if (mode != 0) {
        if (((CDDrawWorkerRegistry*)m_c->m_10)->HasKeyEqual_155550("GRUNTZ_" + name) == 0) {
            ((CGruntSpawnConfig*)g_gameReg->m_cueSink)->DtorBody();
            ((CTriggerMgr*)g_gameReg->m_cmdGrid)->DestroyAllAnims();
            if (lightGate != 0) {
                CString cs;
                cs.LoadString(0x819b);
                RECT r = *(RECT*)&g_gameReg->m_world->m_24->m_planeCtx;
                RECT r2;
                CopyRect(&r2, &r);
                EngStr_DrawText(
                    (EngStrRenderObj*)g_gameReg->m_world,
                    (i32)&cs,
                    (i32)&r2,
                    0x82,
                    1,
                    0xff,
                    0xff,
                    0,
                    1
                );
            }
            g_resourceInstallActive = 1;
            void* tree = m_30->ResolvePath("IMAGEZ_" + name);
            if (tree == 0) {
                result = 0;
                goto done;
            }
            m_c->m_10->InstallTree(tree, "GRUNTZ_" + name, "_"); // slot 18 (+0x48)
            g_resourceInstallActive = 0;
            if (finishGate != 0) {
                ((CMulti*)finishGate)->AckJoinFailure(); // 0x35e4, ecx=notify
            }
        }
        if (((CDDrawSubMgrLeafScan*)m_c->m_28)->HasKeyEqual_1583c0("GRUNTZ_" + name) == 0) {
            void* tree = m_30->ResolvePath("SOUNDZ_" + name);
            if (tree != 0) {
                // the m_28 cast stays until the CSndHost/CDDrawSubMgrLeafScan conflation is settled (Fable);
                // `tree` is the real CSymTab - DirNode was a view of it.
                ((CDDrawSubMgrLeafScan*)m_c->m_28)
                    ->ScanTree_157ee0((CSymTab*)tree, "GRUNTZ_" + name, "_");
            }
        }
        if (((CDDrawSubMgrLeaf*)m_c->m_animRegistry)->HasKeyPrefix_152c50("GRUNTZ_" + name) == 0) {
            void* tree = m_30->ResolvePath("ANIZ_" + name);
            if (tree == 0) {
                result = 0;
                goto done;
            }
            ((CDDrawSubMgrAni*)m_c->m_animRegistry)
                ->ScanTree_152ad0((CSymTab*)tree, "GRUNTZ_" + name, "_");
        }
        result = 1;
        goto done;
    }

    if (((CDDrawWorkerRegistry*)m_c->m_10)->HasKeyEqual_155550("GRUNTZ_" + name) != 0) {
        ((CDDrawWorkerRegistry*)m_c->m_10)->RemoveKeysEqual_155360("GRUNTZ_" + name, "_");
        if (finishGate != 0) {
            ((CMulti*)finishGate)->AckJoinFailure(); // 0x35e4, ecx=notify
        }
    }
    if (((CDDrawSubMgrLeafScan*)m_c->m_28)->HasKeyEqual_1583c0("GRUNTZ_" + name) != 0) {
        ((CDDrawSubMgrLeafScan*)m_c->m_28)->RemoveKeysEqual_157c70("GRUNTZ_" + name, "_");
    }
    if (((CDDrawSubMgrLeaf*)m_c->m_animRegistry)->HasKeyPrefix_152c50("GRUNTZ_" + name) != 0) {
        ((CDDrawSubMgrLeaf*)m_c->m_animRegistry)->RemoveKeysEqual_1527d0("GRUNTZ_" + name, "_");
    }
    result = 1;
done:
    return result;
}

// ===========================================================================
// CPlay::BuildSpriteImageKeyTable (0x0dd540) - install the per-grunt IMAGE namespaces
// (GRUNTZ_<X>) into the image registry (m_c->m_10, virtual install at +0x48), each
// sourced from the GRUNTZ bank's IMAGEZ_<X> set; brackets the run with the install-active
// counter (1 .. 0) and ticks the notify object after each install. A missing source
// set aborts (return 0, install-active flag left set). __thiscall.
// ===========================================================================
RVA(0x000dd540, 0x241)
i32 CPlay::BuildSpriteImageKeyTable(CMulti* notify) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    g_resourceInstallActive = 1;
    if (!((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("GRUNTZ_NORMALGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_NORMALGRUNT");
        if (!s) {
            return 0;
        }
        self->m_c->m_10->InstallTree(s, "GRUNTZ_NORMALGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("GRUNTZ_DEATHZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_DEATHZ");
        if (!s) {
            return 0;
        }
        self->m_c->m_10->InstallTree(s, "GRUNTZ_DEATHZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("GRUNTZ_ENTRANCEZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_ENTRANCEZ");
        if (!s) {
            return 0;
        }
        self->m_c->m_10->InstallTree(s, "GRUNTZ_ENTRANCEZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("GRUNTZ_EXITZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_EXITZ");
        if (!s) {
            return 0;
        }
        self->m_c->m_10->InstallTree(s, "GRUNTZ_EXITZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("GRUNTZ_GRUNTPUDDLE")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_GRUNTPUDDLE");
        if (!s) {
            return 0;
        }
        self->m_c->m_10->InstallTree(s, "GRUNTZ_GRUNTPUDDLE", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("GRUNTZ_PICKUPS")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_PICKUPS");
        if (!s) {
            return 0;
        }
        self->m_c->m_10->InstallTree(s, "GRUNTZ_PICKUPS", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawWorkerRegistry*)self->m_c->m_10)->HasKeyEqual_155550("GRUNTZ_BOMBGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_BOMBGRUNT");
        if (!s) {
            return 0;
        }
        self->m_c->m_10->InstallTree(s, "GRUNTZ_BOMBGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    g_resourceInstallActive = 0;
    return 1;
}

// ===========================================================================
// CPlay::BuildAnizKeyTable (0x0ddaa0) - install the per-grunt ANIM namespaces
// (GRUNTZ_<X>) into the animation registry (m_c->m_2c), each sourced from the GRUNTZ
// bank's ANIZ_<X> set; ticks the notify object after each install. A missing source
// set aborts (return 0). __thiscall.
// ===========================================================================
RVA(0x000ddaa0, 0x228)
i32 CPlay::BuildAnizKeyTable(CMulti* notify) {
    CPlay* self = this;
    if (!self->m_c) {
        return 0;
    }
    if (!((CDDrawSubMgrLeaf*)self->m_c->m_animRegistry)
             ->HasKeyPrefix_152c50("GRUNTZ_NORMALGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_NORMALGRUNT");
        if (!s) {
            return 0;
        }
        ((CDDrawSubMgrAni*)self->m_c->m_animRegistry)
            ->ScanTree_152ad0((CSymTab*)s, "GRUNTZ_NORMALGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawSubMgrLeaf*)self->m_c->m_animRegistry)->HasKeyPrefix_152c50("GRUNTZ_DEATHZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_DEATHZ");
        if (!s) {
            return 0;
        }
        ((CDDrawSubMgrAni*)self->m_c->m_animRegistry)
            ->ScanTree_152ad0((CSymTab*)s, "GRUNTZ_DEATHZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawSubMgrLeaf*)self->m_c->m_animRegistry)->HasKeyPrefix_152c50("GRUNTZ_ENTRANCEZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_ENTRANCEZ");
        if (!s) {
            return 0;
        }
        ((CDDrawSubMgrAni*)self->m_c->m_animRegistry)
            ->ScanTree_152ad0((CSymTab*)s, "GRUNTZ_ENTRANCEZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawSubMgrLeaf*)self->m_c->m_animRegistry)->HasKeyPrefix_152c50("GRUNTZ_EXITZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_EXITZ");
        if (!s) {
            return 0;
        }
        ((CDDrawSubMgrAni*)self->m_c->m_animRegistry)
            ->ScanTree_152ad0((CSymTab*)s, "GRUNTZ_EXITZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawSubMgrLeaf*)self->m_c->m_animRegistry)
             ->HasKeyPrefix_152c50("GRUNTZ_GRUNTPUDDLE")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_GRUNTPUDDLE");
        if (!s) {
            return 0;
        }
        ((CDDrawSubMgrAni*)self->m_c->m_animRegistry)
            ->ScanTree_152ad0((CSymTab*)s, "GRUNTZ_GRUNTPUDDLE", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawSubMgrLeaf*)self->m_c->m_animRegistry)->HasKeyPrefix_152c50("GRUNTZ_PICKUPS")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_PICKUPS");
        if (!s) {
            return 0;
        }
        ((CDDrawSubMgrAni*)self->m_c->m_animRegistry)
            ->ScanTree_152ad0((CSymTab*)s, "GRUNTZ_PICKUPS", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!((CDDrawSubMgrLeaf*)self->m_c->m_animRegistry)->HasKeyPrefix_152c50("GRUNTZ_BOMBGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_BOMBGRUNT");
        if (!s) {
            return 0;
        }
        ((CDDrawSubMgrAni*)self->m_c->m_animRegistry)
            ->ScanTree_152ad0((CSymTab*)s, "GRUNTZ_BOMBGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    return 1;
}

// ===========================================================================
// The trace-discovered CPlay __thiscall cluster (analyzed; this TU).
// ===========================================================================

// (The old `ShowCursorFn g_ShowCursor` fn-ptr global @0x2c44c4 is GONE: that
// address is USER32's ShowCursor IAT slot, and retail's "cached fn-ptr" shape is
// just the dllimport-call CSE - `mov esi,[__imp__ShowCursor]` reused across the
// loop. Plain ::ShowCursor calls below are the honest source.)

// ResetForMode (0x0c8a10) - capture the live cursor position into the
// BeginFrameClear args (m_cursorX/m_cursorY), force the cursor hidden, enter the
// requested mode (publishing the level-start clock for mode 9), then reset the
// per-frame drag/world-ready latches and the three world sub-objects.
RVA(0x000c8a10, 0x119)
i32 CPlay::ResetForMode(i32 mode) {
    POINT pt;
    GetCursorPos(&pt);
    m_cursorX = pt.x;
    m_cursorY = pt.y;
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    if (mode == 9) {
        g_frameTime = m_savedClock;
        if (!EnterMode(9)) {
            return 0;
        }
        m_stepCountdown = 2;
    } else if (m_renderDisabled == 0 || m_4w()->m_134 == 2) {
        if (!EnterMode(mode)) {
            return 0;
        }
    }
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    m_dragSnapActive = 0;
    m_dragInProgress = 0;
    m_dragInhibit1 = 0;
    m_dragInhibit2 = 0;
    m_dragEndNotify = 0;
    m_worldReady = 0;
    if (m_renderDisabled == 0) {
        if (mode != 9) {
            ((CWorldSoundSet*)m_4w()->m_54)->Resume();
        }
        ((CTriggerMgr*)m_4w()->m_68)->DestroyAllAnims(); // reg m_68 CTriggerMgr @0x7d330
        ((CGruntSpawnConfig*)m_4w()->m_60)->DtorBody();
    }
    return 1;
}

// FindStartPointAt (0x0d5f90) - a registry-gated hit-test over this->m_markerData[]
// markers: bail unless the active config slot's per-slot counter is below its
// cap, then return the first marker whose +-0x20 box contains (x, y), reporting
// its coords. __thiscall(x, y, outX, outY), ret 0x10.
// (CHitMarker's definition moved to Play.h; the CRegSlot/CRegHitGate/CRegHitView
// singleton views are GONE - the +0x150 slot array is m_focusSlots (CFocusSlot's
// m_228 cap) and the +0x68 per-slot table is m_cmdGrid->m_rowCount, both on the
// canonical CGameRegistry.)
// @early-stop
// ~83% regalloc-coloring wall: logic + all relocs pair, but MSVC5 colors the
// registry base into edx and the id*0x238 slot-index into ecx (we get the
// opposite swap), cascading through the whole gate; plus a return-0 epilogue
// tail-merge difference. Not source-steerable. docs/patterns/zero-register-pinning.md.
RVA(0x000d5f90, 0xd7)
i32 CPlay::FindStartPointAt(i32 x, i32 y, i32* outX, i32* outY) {
    i32 id = g_curPlayer;
    CGameRegistry* reg = g_gameReg;
    CFocusSlot* slot = &reg->m_focusSlots[id];
    if (slot == 0) {
        return 0;
    }
    if (reg->m_cmdGrid->m_rowCount[id] >= slot->m_228) {
        return 0;
    }
    for (i32 i = 0; i < markerCount(); i++) {
        CHitMarker* m = markerData()[i];
        if (m != 0) {
            RECT rc;
            SetRect(&rc, m->m_0 - 0x20, m->m_4 - 0x20, m->m_0 + 0x20, m->m_4 + 0x20);
            if (x < rc.right && x >= rc.left && y < rc.bottom && y >= rc.top) {
                *outX = m->m_0;
                *outY = m->m_4;
                return 1;
            }
        }
    }
    return 0;
}

// ResetPlayState (0x0d60b0) - the per-frame play-state reset OnKeyCommand runs to
// (re)prime the level: pick the INTRO vs AMBIENT ambient-sound cue (formatting
// "INTRO%d"/"AMBIENT%d" off GetAmbientId), re-seed the ambient timer, reset the
// goal geometry / per-slot config rows, clear the win-lose/in-game latches and the
// frame-marker timeline. __thiscall, no args, ret 1 (0 if PrepareReset bails).
// Self-contained views keep Render's CPlay/CWorld typing untouched. The sound
// manager (m_4->m_48) is the real CGruntzSoundZ (<Dsndmgr/GruntzSoundZ.h>,
// included above): FindBank (0x138730), PlayByName (0x138840, discarded here) and
// m_pCurrent (+0x1c); the found bank is a CGruntzSoundInnerZ and Play(1) is its
// SetLoop (0x139030).
// (The 10 CRp* views are GONE - every field resolved onto the canonicals:
// CRpThis==CPlay (m_1c==m_levelIndex, the ambient timer block, m_frameMarker,
// m_4e4==m_scrollSink, m_winLoseBanner/m_inGame), CRpWorld==CWorld (m_14/m_30/
// m_48/m_68/m_7c/m_134), CRpM30==CSpriteFactoryHolder, CRpGeom==CGameLevel (the
// +0x3b0/+0x3b4 goal coords - past the 0x158-byte plane, i.e. ON the level
// object reached via m_30->m_24), CRpTimeline==CTriggerMgr (m_288/m_2a4/
// m_pendingFxKind/the overlayDesc word blocks/m_3ec/m_3f8/m_3fc/m_groupFlag),
// CRpFrame==CTimer (m_baseTime/m_accum/m_38..m_44/m_running), CRpScroll==
// CGameObject (m_stateFlags), CRpReg/CRpReg44/CRpSlot==CGameRegistry (+0x44
// sub-object read via documented offset, m_focusSlots' m_220/m_224).)
RVA(0x000d60b0, 0x2cd)
i32 CPlay::ResetPlayState() {
    char buf[0x40];
    if (m_4w()->m_14 != 0 && g_gameReg->m_134 == 1) {
        m_ambientInterval = AMBIENT_INTRO_INTERVAL_MS;
        m_ambientIntervalHi = 0;
        m_ambientTimerLo = g_frameTime;
        m_ambientTimerHi = 0;
        wsprintfA(buf, "INTRO%d", GetAmbientId());
        if (g_gameReg->m_14 != 0) {
            m_4w()->m_48->PlayByName(buf, 0);
        }
        m_ambientInitDone = 0;
    } else {
        wsprintfA(buf, "AMBIENT%d", GetAmbientId());
        CGruntzSoundZ* snd = m_4w()->m_48;
        CGruntzSoundInnerZ* h = snd->FindBank(buf);
        if (h != 0) {
            snd->m_pCurrent = h;
        }
        if (m_4w()->m_48->m_pCurrent != 0) {
            m_4w()->m_48->m_pCurrent->SetLoop(1);
        }
        CGameRegistry* reg = g_gameReg;
        if (reg->m_14 != 0 && reg->m_134 == 3) {
            m_4w()->m_48->PlayByName(buf, 1);
        }
        m_ambientTimerLo = 0;
        m_ambientInterval = 0;
        m_ambientTimerHi = 0;
        m_ambientIntervalHi = 0;
        m_ambientInitDone = 1;
    }
    if (m_4w()->m_134 == 1) {
        CGameRegistry* reg = g_gameReg;
        // +0xc8 holds a buffer whose [-8] header word gates the single-player save
        // (same node-header idiom LoadByMode probes; identity unrecovered).
        if (*(i32*)(*(char**)((char*)reg + 0xc8) - 8) == 0) {
            m_4w()->m_7c->FillRecord(m_levelIndex, 1);
            reg = g_gameReg;
            // +0x44 sub-object's +0x124 replay gate (identity unrecovered; raw offsets).
            if (*(i32*)(*(char**)((char*)reg + 0x44) + 0x124) == 0) {
                i32 id = m_levelIndex;
                if (id > 0x24 || id == 1) {
                    ((CSaveGame*)reg->m_saveSink)->SetMaxLevel(id);
                    reg = g_gameReg;
                }
            }
            ((CSaveGame*)reg->m_saveSink)->Save(0, 0x81a6);
        }
        CGameLevel* g = m_4w()->m_30->m_24;
        ResetGoalGeom(g->m_header.startX, g->m_header.startY);
    } else {
        CFocusSlot* slot = &g_gameReg->m_focusSlots[g_curPlayer];
        if (slot != 0) {
            ResetGoalGeom(slot->m_220, slot->m_224);
        } else {
            CGameLevel* g = m_4w()->m_30->m_24;
            ResetGoalGeom(g->m_header.startX, g->m_header.startY);
        }
    }
    if (m_scrollSink != 0) {
        m_scrollSink->m_stateFlags &= ~1;
    }
    m_inGame = 0;
    if (!PrepareReset()) {
        return 0;
    }
    for (i32 off = 0; off < 0x8e0; off += 0x238) {
        ((CBattlezMapConfig*)((char*)g_gameReg + 0x188 + off))->Method_025c20();
    }
    m_winLoseBanner = 0;
    CTimer* fm = m_frameMarker;
    if (fm != 0) {
        fm->m_40 = -1;
        fm->m_44 = 0;
        if (fm->m_currentMs != 0) {
            fm->m_38 = g_frameTime;
            fm->m_3c = 0;
            fm->m_accumLo = fm->m_currentMs;
            fm->m_accumHi = 0;
            fm->m_baseTimeLo = g_frameTime;
            fm->m_baseTimeHi = 0;
            fm->m_running = 1;
        } else {
            fm->m_38 = g_frameTime;
            fm->m_3c = 0;
        }
    }
    CTriggerMgr* tl = m_4w()->m_68;
    tl->m_countdownActive = 1;
    tl->m_phase = 0;
    tl->m_pendingFxKind = 0;
    // zero the goo/resource i64 timer pairs half-by-half, preserving retail's
    // lo(base), lo(window), hi(base), hi(window) store order per pair
    ((i32*)&tl->m_gooTimerBase)[0] = 0;
    ((i32*)&tl->m_gooInterval)[0] = 0;
    ((i32*)&tl->m_gooTimerBase)[1] = 0;
    ((i32*)&tl->m_gooInterval)[1] = 0;
    ((i32*)&tl->m_resourceTimerBase)[0] = 0;
    ((i32*)&tl->m_resourceInterval)[0] = 0;
    ((i32*)&tl->m_resourceTimerBase)[1] = 0;
    ((i32*)&tl->m_resourceInterval)[1] = 0;
    tl->m_3ec = 0;
    tl->m_rollingballWanted = 0;
    tl->m_teleportWanted = 0;
    tl->m_groupFlag = 1;
    return 1;
}

// FreeListTeardown (0x0cb480) - the per-level teardown OnExit runs: flush the
// world timeline, reset the resource sub-registries / world sub-objects, release
// every per-level allocation (the m_startMarkers/m_3a4[4]/m_488 pointer arrays) back onto
// the global node free list, then reset the per-grunt-type config rows.
// __thiscall, no args, no return. Self-contained view.
#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540
// The pool's INTERIOR FIELDS - m_freeHead (+0x04) and m_linkOffset (+0x0c) - used to be
// declared here as the standalone globals g_coordPool.m_freeHead / g_coordPool.m_linkOffset. They are not
// globals: they are fields of g_coordPool (DEFINED in src/Gruntz/GameText.cpp), which is
// why the free-list push/pop code reads exactly [pool+4] and [pool+0xc].
extern FreeNodePool g_coordPool;

// (The 9 CRt* views are GONE - CRtThis==CPlay, CRtWorld==CWorld, CRtResMgr==
// CSpriteFactoryHolder (its m_8 "CWwdObjMgr" facet is the sprite-factory/renderer
// conflation, reached by cast), CRtImageReg==CGameLevel (its slot-17 "Teardown"
// with the FABRICATED 17-filler vtable is the REAL CGameLevel::ReleaseChildren
// virtual), CRtSoundReg==CSndHost, CRtReg==CGameRegistry, CRtTimeline==CTriggerMgr
// (m_260==m_byteArr - kept behind the retail-proven CPtrArray::SetSize cast, the
// CByteArray-vs-CPtrArray library ambiguity is a container-identity TODO - m_284,
// m_2a0==m_pendingFx, Reset1b48a6/OverlayTick), CRtArr==the raw data/count reads
// over the real MFC arrays (markerData()/arrData()/arr488Data() accessors).)
// @early-stop
// ~99% register-coloring plateau: logic + all relocs pair; the only residual is
// MSVC5 coloring the reloaded m_4/m_c base pointers into eax/edx/ecx differently
// than retail across the six teardown-call setups. Not source-steerable.
// docs/patterns/zero-register-pinning.md.
RVA(0x000cb480, 0x22c)
void CPlay::FreeListTeardown() {
    i32 i;
    i32 k;
    if (m_c == 0) {
        return;
    }
    if (m_4w() == 0) {
        return;
    }
    if (m_4w()->m_68 != 0) {
        m_4w()->m_68->ClearGridRange(5);
    }
    Teardown1780();
    if (m_c->m_28->m_2c != 0) {
        m_c->m_28->m_2c->Stop();
    }
    m_4w()->m_48->StopAndFlush();
    m_4w()->m_54->Teardown();
    m_4w()->m_60->ClearSprites();
    g_gameReg->m_cmdGrid->DestroyAllAnims();
    m_c->m_24->ReleaseChildren(); // slot 17 (+0x44) - the real CGameLevel virtual
    ((CWwdObjMgr*)m_c->m_8)->PruneList_15aa90();
    if (m_guts != 0) {
        m_guts->ResetWidgets(0);
    }
    if (m_beginMarker != 0) {
        m_beginMarker->RemoveAll();
    }
    if (m_frameMarker != 0) {
        m_frameMarker->Reset();
    }
    m_scrollSink = 0;
    m_4w()->m_68->OverlayTick();
    CTriggerMgr* tl68 = m_4w()->m_68;
    ((CPtrArray*)&tl68->m_byteArr)->SetSize(0, -1); // retail-proven CPtrArray::SetSize @0x1b52e8
    tl68->m_284 = 0;
    m_4w()
        ->m_68->m_baseList
        .RemoveAll(); // ?RemoveAll@CPtrList@@ @0x1b48a6 (+0 member; ex Reset1b48a6)
    m_4w()->m_68->m_pendingFx = 0;
    ((CDDrawWorkerList*)m_c->m_rendererB)->ClearWorkers();
    for (i = 0; i < markerCount(); i++) {
        void* node = markerData()[i];
        if (node != 0) {
            void** p = (void**)((char*)node - g_coordPool.m_linkOffset);
            *p = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = p;
        }
    }
    m_startMarkers.SetSize(0, -1); // CPtrArray::SetSize @0x1b52e8 (retail band)
    for (k = 0; k < 4; k++) {
        for (i = 0; i < arrCount(k); i++) {
            void* node = arrData(k)[i];
            if (node != 0) {
                void** p = (void**)((char*)node - g_coordPool.m_linkOffset);
                *p = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = p;
            }
        }
        m_3a4[k].SetSize(0, -1); // CPtrArray::SetSize @0x1b52e8
    }
    for (i = 0; i < arr488Count(); i++) {
        void* node = arr488Data()[i];
        if (node != 0) {
            void** p = (void**)((char*)node - g_coordPool.m_linkOffset);
            *p = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = p;
        }
    }
    m_488.SetSize(0, -1); // CPtrArray::SetSize @0x1b52e8
    for (i32 off = 0; off < 0x8e0; off += 0x238) {
        ((CBattlezMapConfig*)((char*)m_4 + 0x188 + off))->FreeArrays();
        ((CBattlezMapConfig*)((char*)m_4 + 0x188 + off))->Clear_02ade0();
    }
    m_49c = -1;
}

// ---------------------------------------------------------------------------
// CPlayDtorBody (0x0c8700): the ~CPlay teardown body. Frees the per-frame
// workers, clears the four g_gameReg config rows, flushes the free-list
// arrays, then chains the base (CState) dtor. Same free-list idiom as
// FreeListTeardown (the m_markerData/m_3a4[4]/m_48c flush).
// ---------------------------------------------------------------------------
// (The DtorObList/DtorWorld/CDtorThis views are GONE - CDtorThis was CPlay
// itself (its FABRICATED 32-filler vtable's "Vfunc80" slot 32/+0x80 is CPlay's
// REAL Vslot20 virtual), DtorWorld was CWorld (m_5c CFontConfig / m_128), and the
// +0xc8 "list dtor" was NEITHER ~CPtrList nor ~CObList: retail calls 0x1b9c69 ==
// ?Empty@CString@@QAEXXZ (NAFXCW, anchored) - CGruntzMgr::m_strWorldFile @+0xc8.)
// @early-stop
// hard-regalloc wall: ebp pinned to the zero-const + the cached free-list head
// in edx across the m_markerData/m_3a4/m_48c flush loops are not source-steerable
// (same coloring plateau as FreeListTeardown 0xcb480, ~99%).
RVA(0x000c8700, 0x1f4)
void CPlay::CPlayDtorBody() {
    i32 i;
    if (m_lightFx) {
        m_lightFx->Ctor();
        ::operator delete(m_lightFx);
        m_lightFx = 0;
    }
    OnExit(); // slot 32 (+0x80) - the real CPlay virtual (ex the "Vslot20" placeholder)
    if (m_4) {
        m_4w()->m_128 = 0;
        m_4->m_strWorldFile.Empty(); // 0x1b9c69 CString::Empty (world-file name clear)
    }
    m_1d0 = 0;
    i32 off = 0;
    do {
        off += 0x238;
        *(i32*)((char*)g_gameReg + off - 0xc8) = 0;
    } while (off < 0x8e0);
    if (m_4 && m_4w()->m_5c) {
        m_4w()->m_5c->FreeNodes();
    }
    if (m_guts) {
        m_guts->DtorMembers();
        ::operator delete(m_guts);
        m_guts = 0;
    }
    if (m_hitTest) {
        m_hitTest->Deactivate();
        ::operator delete(m_hitTest);
        m_hitTest = 0;
    }
    if (m_beginMarker) {
        m_beginMarker->~CTileTriggerContainer();
        ::operator delete(m_beginMarker);
        m_beginMarker = 0;
    }
    if (m_frameMarker) {
        m_frameMarker->Reset();
        ::operator delete(m_frameMarker);
        m_frameMarker = 0;
    }
    for (i = 0; i < markerCount(); i++) {
        void* node = markerData()[i];
        if (node != 0) {
            void** p = (void**)((char*)node - g_coordPool.m_linkOffset);
            *p = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = p;
        }
    }
    m_startMarkers.SetSize(0, -1); // CPtrArray::SetSize @0x1b52e8 (retail band)
    for (i32 k = 0; k < 4; k++) {
        for (i = 0; i < arrCount(k); i++) {
            void* node = arrData(k)[i];
            if (node != 0) {
                void** p = (void**)((char*)node - g_coordPool.m_linkOffset);
                *p = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = p;
            }
        }
        m_3a4[k].SetSize(0, -1); // CPtrArray::SetSize @0x1b52e8
    }
    for (i = 0; i < arr488Count(); i++) {
        void* node = arr488Data()[i];
        if (node != 0) {
            void** p = (void**)((char*)node - g_coordPool.m_linkOffset);
            *p = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = p;
        }
    }
    m_49c = -1;
    m_488.SetSize(0, -1); // CPtrArray::SetSize @0x1b52e8
    ((CGameModeBase*)this)->BaseCleanup();
}

// ---------------------------------------------------------------------------
// EnterMode (0x0d6fa0): the mode-enter gate. Pause the guts + world, optionally
// run the deferred draw, (re)install the renderer view, then re-arm the guts and
// (mode 9) latch the saved game clock. A dense chain of CPlay/registry thunks.
// ---------------------------------------------------------------------------
// (The 8 Em* views are GONE - EmThis==CPlay (m_1c4/m_savedClock/m_inputWarmup*/
// m_guts, m_470/m_474==the region gates, m_484==m_hudSuppressed), EmResMgr==
// CSpriteFactoryHolder, EmCWorld==CDrawTarget (its m_14/m_18 surface pages - the
// "CDDrawWorkerMgr" facet is the CDDrawSubMgrPages cast on m_drawTarget), EmHdr14
// ==CDrawTarget::SurfaceB, EmRendC (a FABRICATED 13-filler vtable)==CRenderer
// (Present IS its real slot-13 virtual), EmWorld==CWorld (m_10/m_54), EmGuts==
// CStatusBarMgr, "g_645588_clk"==g_frameTime (a duplicate extern of the game clock),
// "EmRegWorldStep"==UpdateMgrScroll @0xebd70.)
// @early-stop
// large state-machine wall: the mode dispatch + renderer reinstall + guts re-arm
// are faithful, but the whole ILT-thunk referent set (~15 unnamed CPlay/registry
// leaves) keeps it reloc-fuzzy; codegen plateau, not source-steerable.
RVA(0x000d6fa0, 0x1fa)
i32 CPlay::EnterMode(i32 mode) {
    ((CGruntzMgr*)g_gameReg)->CheckSavedMode();
    m_guts->Deactivate();
    m_guts->LoadDestructButtonSprite(0);
    m_4->PerFrameTick();

    if (m_1c4 != 0) {
        m_1c4 = 0;
        m_c->m_drawTarget->m_14->m_surface->Fill(0);
        UpdateMgrScroll((CGruntzMgr*)g_gameReg, (i32*)m_guts, m_region0Gate); // 0x2356
        if (m_region1Gate != 0) {
            NotifyVisibleEntities();
        } else {
            m_c->m_24->VisitVisible(m_c->m_drawTarget->m_14, (CGameObjChain*)m_c->m_8);
            m_c->m_rendererB->PruneWorkers(m_c->m_drawTarget->m_14, m_c->m_drawTarget->m_18);
        }
        m_guts->Deactivate();
        m_guts->LoadMainStatusBarSprite();
    } else {
        if (m_region1Gate != 0) {
            NotifyVisibleEntities();
        } else {
            m_c->m_24->VisitVisible(m_c->m_drawTarget->m_14, (CGameObjChain*)m_c->m_8);
            m_c->m_rendererB->PruneWorkers(m_c->m_drawTarget->m_14, m_c->m_drawTarget->m_18);
        }
        m_guts->Deactivate();
        m_guts->LoadMainStatusBarSprite();
        if (mode == 9) {
            if (((CDDrawSubMgrPages*)m_c->m_drawTarget)->Method_158d20() != 0) {
                goto finish;
            }
            if (((CDDrawSubMgrPages*)m_c->m_drawTarget)->Method_158cb0(0, 0x30000) != 0) {
                goto finish;
            }
            return 0;
        }
        m_c->m_drawTarget->m_14->m_surface->Fill(0);
    }

finish:
    ((CDDrawSubMgrPages*)m_c->m_drawTarget)->Method_158e90();
    RetireScene(
        0x50,
        0x3e8,
        0,
        1
    ); // 0xfa8f0 CState::RetireScene (inherited by CPlay `this`, cast-free)
    if (m_c->m_24->m_mainPlane != 0) {
        ((CPlaneRender*)m_c->m_24->m_mainPlane)->CenterScrollB();
    }
    m_4->PerFrameTick();
    m_inputWarmup1 = 0;
    m_inputWarmup2 = 0;
    m_inputHalfSel = 0;
    if (m_4w()->m_10 != 0 && mode != 9) {
        m_4w()->m_54->Resume();
    }
    if (mode == 9) {
        g_frameTime = m_savedClock;
    }
    m_guts->Deactivate();
    RegisterInputBindings();
    m_hudSuppressed = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// AddLevelGruntz (0x0d5960): walk the registry's object list; for each valid
// grunt object (vtable-id 0x4024a5, not the placeholder m_124) register it with
// the session via the world's +0x68 sink. On a -1 failure, format + log the
// "Could not add Grunt: Player=%d" message (a CString temp -> /GX frame).
// ---------------------------------------------------------------------------
// The four placed-object dynamic-type markers LoadWarlordSprites tests obj->m_7c[4]
// against: each object's type-record +0x10 field holds its factory create-fn, so
// the marker is that create-fn's ILT thunk. These are the SAME thunks
// RegisterGameObjectTypes registers (GameObjectFactory.cpp) - so reference the real
// `_CreateXxx` symbols, bound to their thunk RVAs there (reloc fidelity). The
// `cmp eax,offset _CreateXxx` DIR32 reloc is what the match needs; declared here as
// address tokens (char[]) sharing the factory's decorated `_CreateXxx` symbol.
extern "C" char
    CreateGruntStartingPoint[];     // 0x24a5 ("multi-sprite warlord" m_11c/m_120 + m_118 switch)
extern "C" char CreateInGameIcon[]; // 0x288d (counted object keyed on m_124)
extern "C" char CreateCoveredPowerup[]; // 0x3d0f (counted object keyed on m_11c)
extern "C" char
    CreateGiantRock[]; // 0x137a (counted object keyed on m_11c; sibling of CoveredPowerup)

// (The 6 Ag* views are GONE - AgGrunt==CGameObject (m_08==m_flags, m_5c/m_60==
// m_screenX/Y, m_7c[4]==AnimWorkerObj::Init, the m_114..m_134 record band),
// AgNode/AgListHdr==CWarlordListNode/CWarlordListHead (View.h), AgWorld==CWorld,
// AgResMgr==CSpriteFactoryHolder, AgThis==CPlay. The bare `0x4024a5` type-marker
// immediate (a reloc-less VA!) is now the CreateGruntStartingPoint symbol.)
extern "C" i32 g_curPlayer; // 0x644c54 placeholder token

// @early-stop
// /GX list-walk wall: the registration loop + CString error log are faithful, but
// the 13-arg AddGrunt + CString-temp EH frame keep it reloc-fuzzy.
RVA(0x000d5960, 0x160)
i32 CPlay::AddLevelGruntz() {
    CDDrawGroupNode* node = m_c->m_childGroup->m_head;
    while (node != 0) {
        CGameObject* g = node->m_gameObj;
        node = node->m_next;
        if (g == 0) {
            continue;
        }
        if ((void*)g->m_7c->m_notify != (void*)CreateGruntStartingPoint) {
            continue;
        }
        if (g->m_124 == g_curPlayer) {
            continue;
        }
        i32 x = ((g->m_screenX & ~0x1f) + 0x10);
        i32 y = ((g->m_screenY & ~0x1f) + 0x10);
        // ILT 0x40bb == ?PlaceObject@CTriggerMgr@@ @0x6b6d0 (the ex-`AddGrunt` decl was a
        // phantom alias of it). The 13th arg really is an address at THIS site (the a30
        // slot is kind-dependent).
        i32 r = m_4w()->m_68->PlaceObject(
            g->m_124,
            y,
            x,
            0x186a0,
            0,
            g->m_114,
            g->m_11c,
            g->m_120,
            g->m_118,
            g->m_12c,
            g->m_7c->m_2c,
            g->m_7c->m_30,
            (i32)&g->m_extentL
        );
        if (r == -1) {
            CString msg;
            msg.Format("Could not add Grunt: Player=%d", g->m_124, y, x);
            // `mov ecx,[0x64556c]; push msg; call 0x417e` (read off the retail site):
            // a __thiscall on the singleton - CGruntzMgr::EnterModalUI @0x8ef10 (ILT
            // 0x417e), NOT the 2-arg __cdecl `AgLog` the old decl faked.
            ((CGruntzMgr*)g_gameReg)->EnterModalUI(msg); // CString -> LPCTSTR (implicit)
            return 0;
        }
        g->m_flags |= 0x10000;
    }
    return 1;
}

// BuildGruntNamespaceList (0x0dd050) - register the seven core grunt asset
// namespaces (NORMALGRUNT/DEATHZ/ENTRANCEZ/EXITZ/GRUNTPUDDLE/PICKUPS/BOMBGRUNT)
// in order; bail (return 0) on the first that fails to bind. The CString name temp
// forces the /GX EH frame. __thiscall(arg), ret 4.
RVA(0x000dd050, 0x24b)
i32 CPlay::BuildGruntNamespaceList(i32 arg) {
    CString s;
    s = "NORMALGRUNT";
    if (!((CNamespaceLoader*)this)->BuildAssetNamespacePrefixes(s, 1, 0, arg)) {
        return 0;
    }
    s = "DEATHZ";
    if (!((CNamespaceLoader*)this)->BuildAssetNamespacePrefixes(s, 1, 0, arg)) {
        return 0;
    }
    s = "ENTRANCEZ";
    if (!((CNamespaceLoader*)this)->BuildAssetNamespacePrefixes(s, 1, 0, arg)) {
        return 0;
    }
    s = "EXITZ";
    if (!((CNamespaceLoader*)this)->BuildAssetNamespacePrefixes(s, 1, 0, arg)) {
        return 0;
    }
    s = "GRUNTPUDDLE";
    if (!((CNamespaceLoader*)this)->BuildAssetNamespacePrefixes(s, 1, 0, arg)) {
        return 0;
    }
    s = "PICKUPS";
    if (!((CNamespaceLoader*)this)->BuildAssetNamespacePrefixes(s, 1, 0, arg)) {
        return 0;
    }
    s = "BOMBGRUNT";
    if (!((CNamespaceLoader*)this)->BuildAssetNamespacePrefixes(s, 1, 0, arg)) {
        return 0;
    }
    return 1;
}

// BuildWarlordNameTable (0x0dd340) - verify warlord ids 2..0x20 are present, then
// the two boss ids 0x39/0x3a, then bind the three named warlord sprite sets
// (NAPOLEAN/VIKING/PATTON). The CString name temp forces the /GX EH frame.
// __thiscall(arg), ret 4.
RVA(0x000dd340, 0x189)
i32 CPlay::BuildWarlordNameTable(i32 arg) {
    for (i32 id = GRUNT_TYPE_BOOMERANG; id <= GRUNT_TYPE_YOYO; id++) {
        if (!BuildGruntTypeNameTable(id, 0, 0, 0)) {
            return 0;
        }
    }
    if (!BuildGruntTypeNameTable(GRUNT_TYPE_HAREKRISHNA, 0, 0, arg)) {
        return 0;
    }
    if (!BuildGruntTypeNameTable(GRUNT_TYPE_REAPER, 0, 0, arg)) {
        return 0;
    }
    CString s("WARLORDZ_NAPOLEAN");
    if (!((CNamespaceLoader*)this)->BuildAssetNamespacePrefixes(s, 0, 0, arg)) {
        return 0;
    }
    s = "WARLORDZ_VIKING";
    if (!((CNamespaceLoader*)this)->BuildAssetNamespacePrefixes(s, 0, 0, arg)) {
        return 0;
    }
    s = "WARLORDZ_PATTON";
    if (!((CNamespaceLoader*)this)->BuildAssetNamespacePrefixes(s, 0, 0, arg)) {
        return 0;
    }
    return 1;
}

// (CWarlordObj is GONE - the placed object is the canonical CGameObject; the
// "+0x7c raw vtable's slot 4" is AnimWorkerObj::Init. CWarlordListNode now lives
// in <Gruntz/View.h> next to its list head.)

// LoadWarlordSprites (0x0d65d0) - ensure every sprite set a placed warlord needs is
// loaded. Two modes: the full campaign preload (registry m_134 != 1) loads sets
// 2..0x20 + 0x39/0x3a + the three named warlord banks; the in-level mode walks the
// placed-object display list (renderer A's m_10) and, per object type (its vtable
// marker), loads the sets that object uses + bumps the world's per-kind counters
// (m_4->m_7c). `loaded[]` guards each set so its progress tick fires once. Reuses
// ProbeWarlord (0x12da) + BindWarlordName (0x2bc1), like BuildWarlordNameTable above.
// @early-stop  (0.x% -> 91.06%)
// EH/frame wall: retail reuses the dead incoming-arg slots for the CString `s` (no
// `sub esp` for locals) while the recompile allocates a fresh frame, so the
// cleanup/dtor tail shifts + a few esi/edx/ecx spill recolors. Code shape + every
// data/marker reloc match.
RVA(0x000d65d0, 0x7a4)
i32 CPlay::LoadWarlordSprites(i32 ctx, i32* loaded) {
    if (g_gameReg->m_134 != 1) {
        for (i32 id = GRUNT_TYPE_BOOMERANG; id <= GRUNT_TYPE_YOYO; id++) {
            if (loaded[id] == 0) {
                BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                loaded[id] = 1;
            }
            if (!BuildGruntTypeNameTable(id, 1, 0, ctx)) {
                return 0;
            }
        }
        if (!BuildGruntTypeNameTable(GRUNT_TYPE_HAREKRISHNA, 1, 0, ctx)) {
            return 0;
        }
        if (loaded[0x21] == 0) {
            BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
            loaded[0x21] = 1;
        }
        if (!BuildGruntTypeNameTable(GRUNT_TYPE_REAPER, 1, 0, ctx)) {
            return 0;
        }
        if (loaded[0x22] == 0) {
            BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
            loaded[0x22] = 1;
        }
        CString s("WARLORDZ_NAPOLEAN");
        if (!((CNamespaceLoader*)this)->BuildAssetNamespacePrefixes(s, 1, 0, ctx)) {
            return 0;
        }
        if (loaded[0x23] == 0) {
            BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
            loaded[0x23] = 1;
        }
        s = "WARLORDZ_VIKING";
        if (!((CNamespaceLoader*)this)->BuildAssetNamespacePrefixes(s, 1, 0, ctx)) {
            return 0;
        }
        if (loaded[0x24] == 0) {
            BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
            loaded[0x24] = 1;
        }
        s = "WARLORDZ_PATTON";
        if (!((CNamespaceLoader*)this)->BuildAssetNamespacePrefixes(s, 1, 0, ctx)) {
            return 0;
        }
        if (loaded[0x25] == 0) {
            BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
            loaded[0x25] = 1;
        }
        return 1;
    }
    // (retail tests the list-head SUB-OBJECT address, group+0x10 - kept as-is)
    char* head = (char*)&this->m_c->m_childGroup->m_pad10;
    if (!head) {
        return 0;
    }
    CDDrawGroupNode* node = this->m_c->m_childGroup->m_head;
    while (node) {
        CGameObject* obj = node->m_gameObj;
        CDDrawGroupNode* nxt = node->m_next;
        if (obj) {
            void* marker = (void*)obj->m_7c->m_notify;
            if (marker == (void*)CreateGruntStartingPoint) {
                i32 v = obj->m_11c;
                if (v) {
                    if (!BuildGruntTypeNameTable(v, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[v] == 0) {
                        BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                        loaded[v] = 1;
                    }
                }
                v = obj->m_120;
                if (v) {
                    if (!BuildGruntTypeNameTable(v, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[v] == 0) {
                        BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                        loaded[v] = 1;
                    }
                }
                switch (obj->m_118) {
                    case 0x7:
                        if (!BuildGruntTypeNameTable(1, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[1] == 0) {
                            BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                            loaded[1] = 1;
                        }
                        break;
                    case 0x8:
                        if (!BuildGruntTypeNameTable(3, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[3] == 0) {
                            BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                            loaded[3] = 1;
                        }
                        break;
                    case 0x9:
                        if (!BuildGruntTypeNameTable(5, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[5] == 0) {
                            BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                            loaded[5] = 1;
                        }
                        break;
                    case 0xa:
                        if (!BuildGruntTypeNameTable(7, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[7] == 0) {
                            BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                            loaded[7] = 1;
                        }
                        break;
                    case 0xb:
                        if (!BuildGruntTypeNameTable(0xd, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[0xd] == 0) {
                            BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                            loaded[0xd] = 1;
                        }
                        break;
                    case 0xc:
                        if (!BuildGruntTypeNameTable(0x11, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[0x11] == 0) {
                            BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                            loaded[0x11] = 1;
                        }
                        break;
                    case 0xf:
                        if (!BuildGruntTypeNameTable(0x13, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[0x13] == 0) {
                            BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                            loaded[0x13] = 1;
                        }
                        break;
                    case 0x10:
                        if (!BuildGruntTypeNameTable(0x1e, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[0x1e] == 0) {
                            BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                            loaded[0x1e] = 1;
                        }
                        break;
                }
            } else if (marker == (void*)CreateInGameIcon) {
                i32 cv = obj->m_124 == 0x32 ? obj->m_118 : obj->m_124;
                if (cv >= 1 && cv <= 0x16 && cv != 0x14) {
                    m_4w()->m_7c->m_34++;
                } else if (cv >= 0x17 && cv <= 0x20) {
                    m_4w()->m_7c->m_30++;
                } else if (cv >= 0x36 && cv <= 0x3c) {
                    m_4w()->m_7c->m_38++;
                } else if (cv == 0x50) {
                    m_4w()->m_7c->m_40++;
                }
                i32 d = obj->m_124;
                if (d <= 0x20) {
                    if (!BuildGruntTypeNameTable(d, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[obj->m_124] == 0) {
                        BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                        loaded[obj->m_124] = 1;
                    }
                } else if (d == GRUNT_TYPE_HAREKRISHNA) {
                    if (!BuildGruntTypeNameTable(GRUNT_TYPE_HAREKRISHNA, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[0x21] == 0) {
                        BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                        loaded[0x21] = 1;
                    }
                } else if (d == GRUNT_TYPE_REAPER) {
                    if (!BuildGruntTypeNameTable(GRUNT_TYPE_REAPER, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[0x22] == 0) {
                        BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                        loaded[0x22] = 1;
                    }
                } else if (d == 0x55 || d == 0x32) {
                    if (!BuildGruntTypeNameTable(obj->m_118, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[obj->m_118] == 0) {
                        BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                        loaded[obj->m_118] = 1;
                    }
                }
            } else if (marker == (void*)CreateCoveredPowerup || marker == (void*)CreateGiantRock) {
                i32 cv = obj->m_11c == 0x32 ? obj->m_118 : obj->m_11c;
                if (cv >= 1 && cv <= 0x16 && cv != 0x14) {
                    m_4w()->m_7c->m_34++;
                } else if (cv >= 0x17 && cv <= 0x20) {
                    m_4w()->m_7c->m_30++;
                } else if (cv >= 0x36 && cv <= 0x3c) {
                    m_4w()->m_7c->m_38++;
                } else if (cv == 0x50) {
                    m_4w()->m_7c->m_40++;
                }
                i32 e = obj->m_11c;
                if (e <= 0x20) {
                    if (!BuildGruntTypeNameTable(e, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[obj->m_11c] == 0) {
                        BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                        loaded[obj->m_11c] = 1;
                    }
                } else if (obj->m_124 == GRUNT_TYPE_HAREKRISHNA) {
                    if (!BuildGruntTypeNameTable(GRUNT_TYPE_HAREKRISHNA, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[0x21] == 0) {
                        BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                        loaded[0x21] = 1;
                    }
                } else if (obj->m_124 == GRUNT_TYPE_REAPER) {
                    if (!BuildGruntTypeNameTable(GRUNT_TYPE_REAPER, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[0x22] == 0) {
                        BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                        loaded[0x22] = 1;
                    }
                } else if (e == 0x55 || e == 0x32) {
                    if (!BuildGruntTypeNameTable(obj->m_118, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[obj->m_118] == 0) {
                        BuildHelpReveal(0); // (was "WarlordLoadTick" - same 0x1019 thunk)
                        loaded[obj->m_118] = 1;
                    }
                }
            }
        }
        node = nxt;
    }
    return 1;
}

// SetEffectSpriteDurations (0x0dc060) - stamp the per-effect display duration
// (+0x18) onto each named effect-sound descriptor looked up by name in the sound
// registry's embedded CMapStringToOb. __thiscall, no args, ret 1. Self-contained
// view (an unrolled run of Lookup-then-store).
// (The CEffDesc/CEffResMgr/CEffMgr/CPlayEff views are GONE - the map is the
// canonical CSndHost::m_10 (the holder's +0x28 sound-cue host) and the looked-up
// descriptor is the CSbiCueRecord shape (StatusBarMgr.h): +0x18 is its interval/
// display duration.)
// @early-stop
// ~67% Lookup out-param zero-init scheduling wall (large unrolled fn): logic is
// complete and every name string + duration is byte-exact (all relocs pair), but
// MSVC5 permutes each block's `lea &out` / `out = 0` init / m_c->m_28 load vs
// retail (retail hoists the first `lea &out` into the prologue, shifting the
// out-slot operand 0xc<->0x10), repeating across all 32 blocks. Documented wall;
// deferred to the final sweep. docs/patterns/outparam-zeroinit-scheduling.md.
RVA(0x000dc060, 0x51b)
i32 CPlay::SetEffectSpriteDurations() {
    CSbiCueRecord* d;
    d = 0;
    m_c->m_28->m_10.Lookup("GAME_PYRAMIDMOVE", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("GAME_TELEPORTEROPEN", (void*&)d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("GAME_TELEPORTERCLOSE", (void*&)d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("GAME_TELEPORTERALL", (void*&)d);
    if (d != 0) {
        d->m_18 = 4000;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("GAME_BRICKBREAK", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("LEVEL_DEATHBRIDGEMOVE", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("LEVEL_WATERBRIDGEMOVE", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("LEVEL_ROCKBREAK", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("LEVEL_LAVAGEYSER", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("LEVEL_TRAPDOORCLOSE", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("LEVEL_TRAPDOOROPEN", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("LEVEL_CANDLEIGNITE", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("LEVEL_CANDLEUP", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("LEVEL_CANDLEDOWN", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("LEVEL_GOLFBALLAIR2", (void*&)d);
    if (d != 0) {
        d->m_18 = 250;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("LEVEL_GOLFBALLHOLE", (void*&)d);
    if (d != 0) {
        d->m_18 = 250;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("LEVEL_GOLFBALLSINK", (void*&)d);
    if (d != 0) {
        d->m_18 = 250;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("GAME_EXPLOSION1", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("LEVEL_OUTLETHAZARD", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("GRUNTZ_DEATHZ_DEATHZFREEZE1A", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("GRUNTZ_DEATHZ_DEATHZFREEZE2A", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("GRUNTZ_DEATHZ_DEATHZUNFREEZE1A", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("GRUNTZ_DEATHZ_DEATHZUNFREEZE1A", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("GRUNTZ_DEATHZ_RESSURECT", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("GRUNTZ_DEATHZ_DEATHZSQUASH1A", (void*&)d);
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("LEVEL_CLOUDHAZARDMOVE", (void*&)d);
    if (d != 0) {
        d->m_18 = 10000;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("LEVEL_CLOUDHAZARDKILL", (void*&)d);
    if (d != 0) {
        d->m_18 = 3000;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("GRUNTZ_DEATHZ_DEATHZELECTROCUTE1A", (void*&)d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("GRUNTZ_NERFGUNGRUNT_NERFGUNZGRUNTP1AS1", (void*&)d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("GRUNTZ_GUNHATGRUNT_GUNHATGRUNTP1AS1", (void*&)d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("GRUNTZ_WELDERGRUNT_WELDERZGRUNTP1AS1", (void*&)d);
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    m_c->m_28->m_10.Lookup("LEVEL_PLANEHAZARDFLY", (void*&)d);
    if (d != 0) {
        d->m_18 = 5000;
    }
    return 1;
}

// The two low-RVA 5-byte ILT `jmp rel32` islands for CPlay::RegionEnter/RegionLeave
// (0x1b9a / 0x19f1) are linker-emitted incremental-link thunks, not game code; they
// are carved to config/library_labels.csv (asm-carveout) rather than reconstructed
// as naked jmp bodies.

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CAnimRegistry);
SIZE_UNKNOWN(CImageRegistry);
SIZE_UNKNOWN(CInputDispatch);
SIZE_UNKNOWN(CPlay);
SIZE_UNKNOWN(CSoundRegistry);
SIZE_UNKNOWN(CState);
SIZE_UNKNOWN(CWorld);
SIZE_UNKNOWN(Edge);
SIZE_UNKNOWN(StateMgrBZ);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

// @early-stop
// 0x0cf0a0 (1.4 KB) - homed from src/Stub/GapFunctions.cpp (matcher-5); a large Play
// worker in this TU's .text block, no vtable-ref. Homed pending leaf-first reconstruction.
RVA(0x000cf0a0, 0x567)
i32 Gap_0cf0a0(void) {
    return 0;
}

// @early-stop
// 0x0cfc90 (465 B) - homed from src/Stub/GapFunctions.cpp (matcher-5); a Play leaf,
// no vtable-ref. Homed pending leaf-first reconstruction.
RVA(0x000cfc90, 0x1d1)
i32 Gap_0cfc90(void) {
    return 0;
}
