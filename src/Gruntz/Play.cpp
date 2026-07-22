#include <Gruntz/GameObjectFactory.h> // the real Create* registrants (ex char[] thunk views)
#include <Gruntz/Play.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr; ex the CGameRegistry view)
#include <Rez/FrameClock.h>       // g_lastNow / g_frameTicks (frame-clock band)
#include <Io/FileMem.h>           // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/AreaMgr.h>       // CAreaMgr (g_pAreaMgr; CPlayLevelLoad::LoadByMode)
#include <DDrawMgr/DDrawWorkerRegistry.h> // CDDrawWorkerRegistry (InstallTree slot 18, +0x48)
#include <DDrawMgr/DDrawSubMgrLeaf.h>     // CDDrawSubMgrLeaf (0x152xxx leaf API incl. the ANI set)
#include <DDrawMgr/DDrawWorkerList.h> // CDDrawWorkerList (renderer B: PruneWorkers/ClearWorkers)
#include <DDrawMgr/DDrawChildGroup.h> // CDDrawChildGroup (renderer A: TickKillCues/DestroyChildren_159ef0)
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (real class of m_10/m_14/m_18)
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
#include <Gruntz/CheatMgr.h> // CCheatMgr (the m_124 cheat-used latch)
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GruntzCmdMgr.h> // CGruntzCmdMgr::Spawn (HandleMousePress)
#include <Gruntz/LeafCue.h>      // LeafCue::PlayIfElapsed (HandleMousePress tab cue)
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/ActionOptionsMenuBar.h> // canonical overlay (CTriggerMgr::m_overlay->m_active)
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Timer.h>
#include <Gruntz/LightFxRender.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/WorldSoundSet.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/SpriteRefTable.h>   // CSpriteRefTable (m_74/m_spriteFactory @+0x74; LoadSprite)
#include <Gruntz/GruntzPlayer.h>     // the per-player slot record
#include <Wwd/WwdGameObjectFamily.h> // CGameObject (the wide-object family base)
#include <Gruntz/Grunt.h>            // CGrunt (Load @0xd8060 folds here per the 0xd5960 dossier)
#include <Gruntz/SerialArchive.h>    // the shared archive stream (GruntzPlayer::Serialize)
#include <rva.h>
#include <DDrawMgr/DDrawSurfaceMgr.h> // CDDrawSurfaceMgr + its image/sound/anim registries (m_10/m_28/m_2c)
#include <Gruntz/SoundCue.h> // CSndHost (m_c->m_soundRegistry) + SoundStream (m_2c; Vslot15 quiesce stop)
#include <DDrawMgr/DDSurface.h>          // the real CDDSurface (render-flip surface: Fill/Restore)
#include <Gruntz/TileTriggerContainer.h> // CTileTriggerContainer (m_beginMarker: Serialize/FilterList2)
#include <Gruntz/LevelSync.h>            // CLevelSync (the +0x2dc guts child-sync @0x1084d0)
#include <Gruntz/UserLogic.h>            // CGameObject/AnimWorkerObj

#include <Dsndmgr/GruntzSoundZ.h>
#include <Gruntz/Multi.h>             // CMulti::AckJoinFailure
#include <Gruntz/CBrickz.h>           // CBrickz::LoadAttributes
#include <Gruntz/Brickz.h>            // CBrickzGrid::UpdateDiagonals
#include <Gruntz/ParseSource.h>       // CParseSource::BeginParse/EndParse
#include <DDrawMgr/DDrawWorkerHost.h> // CDDrawWorkerHost::GetSize (the plane/grid-owner)
#include <DinMgr2/DirectInputMgr2.h>  // DirectInputMgr2::ReadAll
#include <DinMgr2/InputMgrPtr.h>      // g_inputMgr (DirectInputMgr2* view; the one decl)
#include <Globals.h>

#include <Gruntz/GameText.h> // g_brickText1 (ex .cpp extern)
inline void* operator new(u32, void* p) {
    return p;
}

#include <Gruntz/String.h>

#include <Bute/ButeMgr.h>

#include <Wap32/EngStr.h>

class CImage;
i32 LayerBlitFrame(CDDrawSurfaceMgr* mgr, CImage* img, i32 x, i32 w, i32 one, i32 zero); // 0x115300
void UpdateMgrScroll(CGruntzMgr* pm, CStatusBarMgr* bar, i32 snapFlag);                  // 0x0ebd70

void Cmd_ApplyScrollParams(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4);

typedef enum {
    CUE_INTERVAL_MS = 0x1f4,           // 500 ms  ambient/win-lose cue toggle
    BOOTY_INTERVAL_MS = 0x2710,        // 10000 ms booty-region one-shot
    REGION_INTERVAL_MS = 0x7530,       // 30000 ms scroll-region re-arm
    FIXED_SUBSTEP_MS = 0x12,           // 18 ms   world fixed-substep quantum
    AMBIENT_INTRO_INTERVAL_MS = 0x1f40 // 8000 ms INTRO-cue window (ResetPlayState)
} PlayIntervalMs;

typedef enum {
    VIEW_MODE_IDLE = 0, // no view yet -> bail
    VIEW_MODE_A = 1,    // mode-A sub-step
    VIEW_MODE_B = 2     // mode-B sub-step
} PlayViewMode;

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

void ShowHudMessage(
    CDDrawSurfaceMgr* sink,
    i32 text,
    i32 rect,
    i32 dur,
    i32 a,
    i32 b,
    i32 c,
    i32 d,
    i32 e
); // 0x1154b0
void ShowHudMessageAlt(
    CDDrawSurfaceMgr* sink,
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
    m_mgr->m_cueSink->DtorBody(); // 0x20a4 -> CGruntSpawnConfig::DtorBody @0x11c7b0
    m_savedClock = static_cast<i32>(g_frameTime);
    if (m_notifyLatch) {
        QuitToMenu();
    }
    if (arg == 9) {
        return 1;
    }
    RECT r;
    m_world->m_drawTarget->m_overlayPair->m_surface->Fill(0);
    CString s;
    s.LoadString(0x81a9);
    r.right = m_mgr->m_modeW;
    r.bottom = m_mgr->m_modeH;
    r.left = 0;
    r.top = 0;
    ShowHudMessage(
        m_world,
        reinterpret_cast<i32>(&s),
        reinterpret_cast<i32>(&r),
        0x78,
        1,
        0xff,
        0xff,
        0,
        1
    );
    RetireScene(0x50, 0x3e8, 0, 1); // 0xfa8f0 CState::RetireScene (inherited, cast-free)
    if (m_mgr && m_mgr->m_cmdGrid) {
        m_mgr->m_cmdGrid->ClearGridRange(5); // 0x41b0 -> CTriggerMgr::ClearGridRange @0x6bd40
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
//     ((i64)(u32)g_frameTime - *reinterpret_cast<i64*>(&m_timerLo) >= *reinterpret_cast<i64*>(&m_intervalLo)), not 32-bit.
//   * the in-game tail (after the view check) has 2 CPlay this-calls the carcass
//     omits (0x2e2d(clock), 0x1519(view)) + a 3-arg cdecl(g_gameReg, m_guts,
//     m_region0Gate) before m_c->m_level->PostStep(); surface flush is a __thiscall on
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
        StepInputA();                                 // cursor draw (BltFast)
        StepWorldB();                                 // world/camera sub-step B
        StepGridWalk(static_cast<i32>(g_frameDelta)); // 0x2e2d  frame-grid advance

        g_killCueClock = g_lastNow; // mirror the draw clock
        g_engineFrameDelta = g_frameDelta;

        // --- shared world-draw block #1 ---
        m_world->m_childGroup->TickKillCues(0); // slot 9 [+0x24]
        m_world->m_level->VisitVisible(
            m_world->m_drawTarget->m_backPair,
            m_world->m_childGroup
        ); // 0x15dc90
        m_world->m_workerList->PruneWorkers(
            m_world->m_drawTarget->m_backPair,
            m_world->m_drawTarget->m_overlayPair
        ); // vtbl[+0x34]
        m_mgr->m_inputState->Retune( // 0x1a7d -> CWorldSoundSet::Retune (positional audio)
            m_world->m_level->m_mainPlane->m_originX,
            m_world->m_level->m_mainPlane->m_originY
        );
        if (m_world->m_soundStream != 0) { // frame profiler
            u32 t = timeGetTime();
            m_world->m_soundStream->PurgeVoiceList(t);  // 0x136e20 (thiscall, SoundDevice base)
            m_world->m_soundStream->TickSubManagers(t); // 0x137ac0 (thiscall)
        }
        m_beginMarker->FilterList2(reinterpret_cast<void*>(g_frameDelta)); // 0x2cc0  begin-marker
        m_guts->LoadDestructButtonSprite(static_cast<i32>(g_frameDelta));  // 0x34bd  guts step

        // --- periodic AMBIENT-cue timer (+0x3f8, 0x1f4 ms; toggles m_cueToggle) ---
        {
            u32 elapsed = g_frameTime - static_cast<u32>(m_cueTimerLo);
            if (elapsed >= static_cast<u32>(m_cueInterval)) {
                m_cueToggle = (m_cueToggle == 0);
                m_cueInterval = CUE_INTERVAL_MS;
                m_cueIntervalHi = 0;
                m_cueTimerLo = static_cast<i32>(g_frameTime);
                m_cueTimerHi = 0;
            }
            if (m_cueToggle != 0) {
                PlayCueAt(0x8128, 0x78, 0, 0xff, 0xff, 0, 1, 0); // cue
            }
        }

        if (m_world->m_drawTarget->m_backPair == 0) {
            return 1; // no view -> bail
        }

        m_frameMarker->Tick(static_cast<i32>(g_frameDelta));    // 0x3710  CTimer::Tick
        m_frameMarker->Draw(0, static_cast<i32>(g_frameDelta)); // 0x27a2  CTimer::Draw
        m_world->m_drawTarget->m_frontPair->m_surface->Flip(0); // 0x13e850  CDDSurface::Flip
        UpdateMgrScroll(g_gameReg, m_guts, m_region0Gate);      // 0x2356
        winapi_0d0b30_CopyRect(reinterpret_cast<i32>(m_world->m_drawTarget->m_backPair)); // 0x1519
        return 1; // -> draw tail
    }

    // m_inGame == 0
    if (m_mgr->m_frameGate != 0) {
        goto alt2;
    }
    {
        CGruntzMgr* reg = g_gameReg;
        if (reg->m_134 != 2 && m_overlayDrag != 0) {
            goto alt2;
        }
    }

    // =================================================================
    // ---- MENU / PAUSE-OVERLAY frame ----
    // =================================================================
    {
        CGruntzMgr* w = m_mgr;
        m_frameMarker->Tick(static_cast<i32>(g_frameDelta)); // m_frameMarker begin
        Eng_FrameTimerStep(w->m_cmdSubMgr, 0); // m_4->m_6c step (carcass; unresolved callee)

        if (m_levelId == CURSOR_FLAILINGGRUNT) { // booty/flailing-grunt one-shot
            u32 elapsed = g_frameTime - static_cast<u32>(m_bootyTimerLo);
            if (elapsed >= static_cast<u32>(m_bootyInterval)) {
                RegCue(g_gameReg->m_cueSink, 0x33e); // the +0x60 cue-sink cue
                m_bootyInterval = BOOTY_INTERVAL_MS;
                m_bootyIntervalHi = 0;
                m_bootyTimerLo = static_cast<i32>(g_frameTime);
                m_bootyTimerHi = 0;
            }
        }

        StepInputA();
        StepC();

        // --- AMBIENT level-init one-shot (+0x348) ---
        if (m_ambientInitDone == 0) {
            u32 elapsed = g_frameTime - static_cast<u32>(m_ambientTimerLo);
            if (elapsed >= static_cast<u32>(m_ambientInterval)) {
                i32 id = GetAmbientId();
                CString name;
                static_cast<void>(name); // [esp+0x10] CString temp (/GX)
                char buf[0x80];
                wsprintfA(buf, "AMBIENT%d", id); // s_AMBIENT%d
                if (g_gameReg->m_musicEnabled
                    != 0) { // +0x14 (the WAP32 base field; the registry view called it m_14)
                    w->m_sound->PlayByName(buf, 1);
                } else {
                    CGruntzSoundInnerZ* out = 0;
                    CGruntzSoundInnerZ* snd = w->m_sound->FindBank(buf);
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
            m_world->m_drawTarget->m_frontPair->m_surface->Fill(0);
            GutsStepB(); // m_guts
        }

        if (m_worldReady == 0) { // world-ready init
            if (w->m_cmdGrid->m_armed != 0) {
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
        m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
        m_world->m_workerList->PruneWorkers(
            m_world->m_drawTarget->m_backPair,
            m_world->m_drawTarget->m_overlayPair
        ); // present
        if (m_region1Gate != 0) {
            StepC(); // alt-input draw
        } else {
            m_world->m_level->VisitVisible(
                m_world->m_drawTarget->m_backPair,
                m_world->m_childGroup
            );
            m_world->m_workerList->PruneWorkers(
                m_world->m_drawTarget->m_backPair,
                m_world->m_drawTarget->m_overlayPair
            );
        }
        m_beginMarker->FilterList2(reinterpret_cast<void*>(g_frameDelta));
        m_guts->LoadDestructButtonSprite(static_cast<i32>(g_frameDelta));
        InputSubStep(w->m_tileGrid); // m_4->m_70

        // On-screen overlay/banner: the retail block (0xc91b7..0xc9259) is the same
        // shape as CMulti::PumpB's - place the 120x120 overlay rect by HUD position,
        // decay-tick then blit+border via the draw page. Both calls run on
        // m_lightFx: thunks 0x1fa0=Resize / 0x14dd=ComputeRect.
        if (m_lightFx != 0 && m_guts->m_position != 2 && m_guts->m_activeTab != 5) {
            RECT rc;
            if (m_guts->m_position == 1) {
                SetRect(&rc, 20, 5, 140, 125);
            } else {
                i32 cx = g_gameReg->m_modeH;
                i32 cy = g_gameReg->m_modeW;
                rc.top = cx; // the kept redundant store (rc escapes to SetRect)
                SetRect(&rc, cy - 140, 5, cy - 20, 125);
            }
            m_lightFx->Resize(static_cast<i32>(g_frameDelta), 0);
            m_lightFx->ComputeRect(m_world->m_drawTarget->m_backPair, &rc);
        }

        m_mgr->m_inputState->Retune( // world sound retune off the plane scroll origin
            m_world->m_level->m_mainPlane->m_originX,
            m_world->m_level->m_mainPlane->m_originY
        );
        if (m_world->m_drawTarget->m_backPair == 0) {
            return 1;
        }

        // --- snapshot/screenshot countdown (+0x4a0/+0x4a8) ---
        if (m_snapshotActive != 0) {
            u32 now = g_frameTime;
            u32 dur = static_cast<u32>((m_snapDur + m_snapBaseLo)) - now;
            if (static_cast<i32>(dur) >= 0) {
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
                if (g_gameReg->m_options[0].m_00c != 0) {
                    void* out = 0;
                    MapLookup(
                        g_gameReg->m_world->m_childGroup,
                        reinterpret_cast<void*>(g_gameReg->m_options[0].m_00c),
                        out
                    );
                    if (out != 0) {
                        SnapWalk();
                    }
                }
            } else {
                // not yet: build a CString temp, CopyRect the viewport, HudDraw.
                CString tmp;
                static_cast<void>(tmp); // [esp+0x10] CString temp
                tmp.Format("%s", "");
                // m_30 is the shared CDDrawSurfaceMgr; this WIP path reads it
                // as a resource map whose +0x24 holds the CopyRect-source rect.
                CopyRect(
                    &m_hudRect,
                    // the WIP path reads the 4 dwords AT &m_level as a rect (retail quirk)
                    reinterpret_cast<const RECT*>(&g_gameReg->m_world->m_level)
                );
                Eng_HudDraw(g_gameReg->m_world, &m_hudRect, 1);
            }
            // (CString temp dtor runs here under the EH frame)
        }

        // --- the four scroll-region one-shots (+0x430/+0x440/+0x450/+0x460) ---
        m_frameMarker->Draw(0, m_frameMarker != 0); // reset
        OnRegion5();
        Eng_FrameTimerStep(w->m_cmdGrid, 0); // m_4->m_68 (carcass; unresolved callee)

        if (m_winLoseBanner != 0 && m_guts->m_toggleActive == 0 && m_guts->m_toggleHandle == 0) {
            // win/lose banner timer (+0x3f8 again, 0x1f4 ms):
            u32 elapsed = g_frameTime - static_cast<u32>(m_cueTimerLo);
            if (elapsed >= static_cast<u32>(m_cueInterval)) {
                m_cueToggle = (m_cueToggle == 0);
                m_cueInterval = CUE_INTERVAL_MS;
                m_cueIntervalHi = 0;
                m_cueTimerLo = static_cast<i32>(g_frameTime);
                m_cueTimerHi = 0;
            }
            if (m_cueToggle != 0) {
                PlayCueAt(0x8129, 0x78, 0, 0xff, 0xff, 0, 1, 0);
            }
        }

        m_beginMarker->FilterList2(reinterpret_cast<void*>(g_frameDelta));
        PostHud(0);
        if (m_worldReady != 0) { // optional HUD overlay draw
            Eng_HudDraw(
                m_world->m_drawTarget->m_frontPair->m_surface,
                &m_hudRect,
                0xff
            ); // (this=m_4->m_10->m_2c)
        }
        m_world->m_drawTarget->m_frontPair->m_surface->Flip(0);

        // --- the four screen-region scroll one-shots: each is
        // a 64-bit "inside region" elapsed test that fires its OnRegion handler. ---
        if (m_region0Gate != 0) { // region-1 (+0x430)
            u32 e = g_frameTime - static_cast<u32>(m_region0TimerLo);
            if (e >= static_cast<u32>(m_region0Interval)) {
                OnRegion2(static_cast<i32>(g_frameTime));
            }
        }
        if (m_region1Gate != 0) { // region-2 (+0x440)
            u32 e = g_frameTime - static_cast<u32>(m_region1TimerLo);
            if (e >= static_cast<u32>(m_region1Interval)) {
                OnRegion1(static_cast<i32>(g_frameTime));
            }
        }
        if (m_region2Gate != 0) { // region-3 (+0x450)
            u32 e = g_frameTime - static_cast<u32>(m_region2TimerLo);
            if (e >= static_cast<u32>(m_region2Interval)) {
                OnRegion3(static_cast<i32>(g_frameTime));
            }
        }
        if (m_region3Gate != 0) { // region-4 (+0x460)
            u32 e = g_frameTime - static_cast<u32>(m_region3TimerLo);
            if (e >= static_cast<u32>(m_region3Interval)) {
                OnRegion4(static_cast<i32>(g_frameTime));
            }
        }
        return 1; // -> draw tail
    }

alt2:
    // =================================================================
    // ---- the m_4->m_c != 0 short path ----
    // =================================================================
    StepInputA();
    if (m_world->m_drawTarget->m_backPair == 0) {
        return 1;
    }
    {
        if (m_world->m_soundStream != 0) { // cursor/frame profiler
            u32 t = timeGetTime();
            m_world->m_soundStream->PurgeVoiceList(t);  // 0x136e20 (thiscall, SoundDevice base)
            m_world->m_soundStream->TickSubManagers(t); // 0x137ac0 (thiscall)
        }
        if (m_paused != 0) {
            // ---- the paused frame: draw-only ----
            m_world->m_level->VisitVisible(
                m_world->m_drawTarget->m_backPair,
                m_world->m_childGroup
            );
            m_world->m_workerList->PruneWorkers(
                m_world->m_drawTarget->m_backPair,
                m_world->m_drawTarget->m_overlayPair
            ); // present
            m_guts->LoadDestructButtonSprite(static_cast<i32>(g_frameDelta));
            if (m_guts->m_toggleActive == 0 && m_guts->m_toggleHandle == 0) {
                PlayCueAt(0x812c, 0x78, 0, 0xff, 0xff, 0, 1, 0); // win/lose
            }
            m_frameMarker->Draw(1, 0);
        } else {
            // ---- the active short frame: entity step + cues ----
            if (m_stepCountdown > 0) {
                m_stepCountdown = m_stepCountdown - 1;
                m_world->m_level->VisitVisible(
                    m_world->m_drawTarget->m_backPair,
                    m_world->m_childGroup
                );
                m_world->m_workerList->PruneWorkers(
                    m_world->m_drawTarget->m_backPair,
                    m_world->m_drawTarget->m_overlayPair
                ); // present
                m_guts->LoadDestructButtonSprite(static_cast<i32>(g_frameDelta));
                Eng_FrameTimerStep(m_guts, 0x32); // (carcass; unresolved callee)
                PlayCueAt(m_lastCueId, 0x78, 0, 0xff, 0xff, 0, 1, 0); // (cueId=m_lastCueId)
                m_frameMarker->Draw(1, 0);
            }
            if (m_ambientInitDone == 0) {
                // the same AMBIENT level-init one-shot (+0x348):
                u32 elapsed = g_frameTime - static_cast<u32>(m_ambientTimerLo);
                if (elapsed >= static_cast<u32>(m_ambientInterval)) {
                    i32 id = GetAmbientId();
                    CString name;
                    static_cast<void>(name);
                    char buf[0x80];
                    wsprintfA(buf, "AMBIENT%d", id);
                    if (g_gameReg->m_musicEnabled
                        != 0) { // +0x14 (the WAP32 base field; the registry view called it m_14)
                        m_mgr->m_sound->PlayByName(buf, 1);
                    } else {
                        CGruntzSoundInnerZ* out = 0;
                        CGruntzSoundInnerZ* snd = m_mgr->m_sound->FindBank(buf);
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
        m_beginMarker->FilterList2(reinterpret_cast<void*>(g_frameDelta));
        PostHud(0);
        m_world->m_drawTarget->m_frontPair->m_surface->Flip(0);
    }
    return 1; // draw tail
}

DATA(0x002bf3bc)
u32 g_engineFrameDelta = 0; // 0x2bf3bc  draw-DELTA mirror  (= g_frameDelta / g_lastDelta)
DATA(0x002bf3c0)
u32 g_killCueClock = 0; // 0x2bf3c0  draw-CLOCK mirror (= g_lastNow)

DATA(0x0021139c)
CAreaMgr* g_pAreaMgr = &g_areaMgr;

// ---------------------------------------------------------------------------
// The methods below are the DEFERRED remainder, grouped by why they aren't
// folded onto their canonical headers yet (all reloc-masked, so byte-neutral):
//   (a) cross-module canonical header + retype (SoundStream Dsndmgr[Teardown/grid];
//       CGruntzSoundZ Dsndmgr[Reset0]; CGruntzCmdMgr Net[ClearList]; DirectInputMgr2
//       StateMgrBZ[HideMenu]; CDDSurface ResMgr[ShadeRect]; CStatusBarMgr SBI_Image
//       [ClearTiles/PostMap]);
//   (b) needs a byte-neutral declared-only method added to its Gruntz header first
//       (CParseSource CImage.h[BeginParse/EndParse]; CBrickz/CBrickzGrid[LoadAttributes/
//       UpdateDiagonals]; CLightFxRender[InstallLightFx/BuildLightShape]; CTimer
//       [TimerEqSet/TimerReset]; CBattlezData[BattlezInit=Init]);
//   (c) Teardown-on-savedThis is CMulti::AckJoinFailure (a cross-cast of the CPlay
//       `this`); WorkerReset is GruntzPlayer::SeedForSlot;
//   (d) @identity-TODO - genuinely unrecovered (no xref/header): ObListInit 0x1b48a6,
//       ButeStore 0x1b5485, GetBool 0x1bedde, CDDrawWorkerHost::GetSize 0x1633e0.
// ---------------------------------------------------------------------------
// Genuine __cdecl engine helpers (reloc-masked rel32).
// ---------------------------------------------------------------------------
void Cmd_ResetScroll();            // 0x2bd0  YAXXZ
i32 QueryToken(i32 a);             // 0x39a4  QueryToken(int)
i32 ValidateMainBlock(void* cstr); // 0x2c8e  static WwdFile (CString byval)
void ActiveWait(i32 ms);           // 0x13dfe0 busy-wait
void* RezAlloc(i32 sz);            // 0x1b9b46 operator new
void RezFree(void* p);             // 0x1b9b82 operator delete

RVA(0x000ca200, 0xe34)
i32 CPlay::LoadByMode(i32 level, i32) {
    CPlay* self = this;
    CGruntzMgr* gameReg;
    void* set;
    i32 reload = 0; // [esp+0x20] warp-cache reload flag (-> the vtable +0xa4 arg)
    // one contiguous stack buffer: the AREA%i/Level%i name at +0x00, a 148-byte
    // zeroed sub-block at +0x20 (retail's [esp+0x58] rep-stos region / LoadMap arg).
    char nameBuf[0xb4]; // [esp+0x38]

    // ---- 1) reset prior-level scroll/area globals ----
    self->m_hudSuppressed = 1;
    g_frameDelta = 0;
    g_lastNow = 0;
    g_frameTime = 0;
    g_levelBias100 = 0;
    if (level > 0x64) {
        level -= 0x64;
        g_levelBias100 = 1;
    }

    // clear the frame-marker timer's slots
    CTimer* worker = self->m_frameMarker;
    if (worker != 0) {
        worker->m_40 = 0;
        worker->m_44 = 0;
        worker->m_accumLo = 0;
        worker->m_accumHi = 0;
        worker->m_running = 0;
        worker->m_currentMs = 0;
    }

    // tear down the old grid (self->m_c->m_soundRegistry->m_2c) / map / sound sub-objects
    SoundStream* grid = self->m_world->m_soundRegistry->m_2c; // the CSndHost-held DSound stream
    if (grid != 0) {
        grid->Stop();
    }
    self->m_mgr->m_sound->StopAndFlush();
    self->m_mgr->m_inputState->Teardown();
    self->m_mgr->m_cueSink->DtorBody();
    self->m_mgr->m_cueSink->ClearSprites();
    self->m_mgr->RestoreVideoMode(0);

    gameReg = g_gameReg;
    if (gameReg->m_134 != 2) {
        g_curPlayer = 0;
        if (gameReg->m_frameGate != 0) {
            i32 v = gameReg->m_frameGate ^ 1;
            gameReg->m_frameGate = v;
            gameReg->FinishLevel(v, 1);
        }
    }

    // reset the 4 fx-spawn anchors (m_x then m_y per slot - retail's p[-1]/p[0] pair)
    {
        CPlay::Anchor* p = self->m_anchors;
        i32 n = 4;
        do {
            p->m_x = -1;
            p->m_y = -1;
            p++;
        } while (--n);
    }

    g_killCueClock = g_lastNow;
    g_engineFrameDelta = g_frameDelta;

    // reset the 4 team blocks at host->m_150 (stride 0x238): single-mode primes
    // team 0 ready, multi-mode zeroes the round counters.
    for (i32 t = 0; t < 4; ++t) {
        CGruntzMgr* hostBase = self->m_mgr;
        gameReg = g_gameReg;
        GruntzPlayer* team = reinterpret_cast<GruntzPlayer*>(
            (reinterpret_cast<char*>(hostBase) + t * 0x48 * 8 - t * 8 + 0x150)
        ); // [edx+ecx*8+0x150]
        if (gameReg->m_134 == 1) {
            team->SeedForSlot(0);
            if (t == 0) {
                team->m_liveGate = 1;
                team->m_joined = 1;
            }
        } else {
            team->m_doneFlag = 0;
            team->m_joined = team->m_liveGate;
            team->m_clearedRound = 0;
        }
    }

    // ---- 2) mode/level-number resolve ----
    i32 modeFlag = (static_cast<i32>(Update()) == 0x11) ? 1 : 0;
    void* savedThis = modeFlag ? self : 0; // [esp+0x10] = (-modeFlag) & self
    self->m_1c4 = 1;
    self->m_levelIndex = level;
    {
        i32 r = (level - 1) % 0x24;
        self->m_levelType = r / 4 + 1; // ((level-1)%0x24)/4 + 1 (signed div-by-4)
    }

    gameReg = g_gameReg;
    g_frameTime = 0;
    if (gameReg->m_134 == 3) {
        srand(timeGetTime());
    }
    g_resourceInstallActive = 0;
    Cmd_ResetScroll();
    gameReg->m_scoreHud->Init();
    gameReg->m_cmdSubMgr->m_1c.RemoveAll();
    gameReg->m_cmdSubMgr->DrainBase();
    g_frameTicks = 0;
    self->m_1bc = 0;
    self->m_mgr->m_130 = 0;

    // already-loaded guard: when host->m_strWorldFile length != 0 skip the name-resolve.
    CGruntzMgr* host = self->m_mgr;
    if (host->m_strWorldFile.GetLength() != 0) {
        if (host->m_128 != 0) {
            // BATTLEZ: resolve the level number from the level name's digit run.
            set = host->m_symParser->ResolvePath("GAME_BATTLEZ");
            if (set == 0) {
                goto fail0;
            }
            CParseSource* ins = (static_cast<CSymTab*>(set))
                                    ->Insert(
                                        static_cast<const char*>(self->m_mgr->GetWorldFileName()),
                                        g_emptyString
                                    );
            if (ins == 0) {
                return 0;
            }
            void* desc = reinterpret_cast<void*>((static_cast<CParseSource*>(set))->BeginParse());
            if (desc == 0) {
                goto fail0;
            }
            char* p = static_cast<char*>(desc) + 0x10; // the record's +0x10 name text
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
            (static_cast<CParseSource*>(set))->EndParse();
            level = num;
        } else if (host->m_12c != 0) {
            // MULTI: same digit resolve off "GAME_MULTI".
            set = host->m_symParser->ResolvePath("GAME_MULTI");
            if (set == 0) {
                goto fail0;
            }
            CParseSource* ins = (static_cast<CSymTab*>(set))
                                    ->Insert(
                                        static_cast<const char*>(self->m_mgr->GetWorldFileName()),
                                        g_emptyString
                                    );
            if (ins == 0) {
                return 0;
            }
            void* desc = reinterpret_cast<void*>((static_cast<CParseSource*>(set))->BeginParse());
            if (desc == 0) {
                goto fail0;
            }
            char* p = static_cast<char*>(desc) + 0x10; // the record's +0x10 name text
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
            (static_cast<CParseSource*>(set))->EndParse();
            level = num;
        } else {
            // default: bute-driven level number (ValidateMainBlock(CString)).
            level = ValidateMainBlock(
                const_cast<char*>(static_cast<const char*>(self->m_mgr->GetWorldFileName()))
            );
            self->m_1bc = 0;
            self->m_mgr->m_130 = 0;
        }

        // recompute area page from the resolved level number
        i32 r = (level - 1) % 0x24;
        self->m_levelIndex = level;
        self->m_levelType = r / 4 + 1;
    }

    // ---- 3) build the level name + look it up ----
    sprintf(nameBuf, "AREA%i", self->m_levelType);
    set = self->m_symParser->ResolvePath(nameBuf);
    self->m_levelBank = static_cast<CSymTab*>(set);
    if (set == 0) {
        goto fail0;
    }

    // ---- 4) area-page jump table over (m_20 - 1) in 0..7 ----
    {
        i32 page = self->m_levelType - 1;
        switch (static_cast<u32>(page)) {
            case 0:
                g_areaPageSize = 4;
                break;
            case 1:
                g_areaPageSize = 0;
                g_areaHazardParam = 0;
                break;
            case 2:
                g_areaPageSize = 8;
                g_areaHazardParam = 0;
                break;
            case 3:
                g_areaPageSize = 8;
                g_areaHazardParam = 0xf;
                break;
            case 4:
                g_areaPageSize = 5;
                g_areaHazardParam = 9;
                break;
            case 5:
                g_areaPageSize = 4;
                g_areaHazardParam = 0xb;
                break;
            case 6:
                g_areaPageSize = 0xe;
                g_areaHazardParam = 0xb;
                break;
            case 7:
                g_areaPageSize = 4;
                g_areaHazardParam = 0;
                break;
            default:
                g_areaPageSize = 4;
                g_areaHazardParam = 0;
                break;
        }
    }

    // m_2c = m_28 (the resolved area descriptor); refresh the host window
    {
        CResSource* prevTiles = self->m_2c;
        self->m_2c = (self->m_levelBank);
        UpdateWindow(self->m_mgr->m_gameWnd->m_hwnd);

        host = self->m_mgr;
        if (host->m_strWorldFile.GetLength() != 0) {
            if (host->m_128 == 0 && host->m_12c == 0) {
                sprintf(nameBuf, "CUSTOMLEVEL", 0);
            }
        } else if (level > 0x24) {
            sprintf(nameBuf, "TRAINING", 0);
        }
        static_cast<void>(prevTiles);
    }

    // ---- 5) the long linear init chain ----
    if (!FadeInTitle(nameBuf, 0, 0, 0, 0, 1)) { // [esp+0x48] out, push 0/0/0/0/1
        goto fail0;
    }
    RetireScene(0x50, 0x3e8, 0, 1); // 0x1843 -> CState::RetireScene
    DrawLevelInfoText();            // 0x14b5 -> 0xd95f0
    self->m_2c = 0;
    {
        i32* z = reinterpret_cast<i32*>(nameBuf + 0x20); // scratch tail of the local buffer
        i32 n = 0x25;
        while (n--) {
            *z++ = 0;
        }
    }
    LoadLoadingBarSprite(); // 0x32d3 -> 0xd7440
    BuildHelpReveal(0);
    FreeListTeardown(); // vtable +0x84 (CPlay slot 33)
    if (savedThis != 0) {
        (static_cast<CMulti*>(savedThis))
            ->AckJoinFailure(); // AckJoinFailure placeholder (0x35e4 on saved obj)
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
            (static_cast<CMulti*>(savedThis))->AckJoinFailure();
        }
        RegisterInputBindings();

        BuildHelpReveal(0);
        if (savedThis != 0) {
            (static_cast<CMulti*>(savedThis))->AckJoinFailure();
        }
        RegisterInputBindings();

        if (!LoadActionTileSprites(diff)) {
            goto fail0;
        }
    }

    // a tail of paired BeginStep(0)/EndStep brackets around the real init steps.
    BuildHelpReveal(0);
    if (savedThis != 0) {
        (static_cast<CMulti*>(savedThis))->AckJoinFailure();
    }
    RegisterInputBindings();
    if (modeFlag != 0 && (g_gameReg)->m_134 == 1) {
        BuildWarlordNameTable(reinterpret_cast<i32>(savedThis));
    }
    BuildHelpReveal(0);
    RegisterInputBindings();
    if (!LoadLevelImages(1) /* 0x3021 -> 0xdb7e0 */) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        (static_cast<CMulti*>(savedThis))->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadGameImages(1) /* 0x3346 -> 0xdb8a0 */) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        (static_cast<CMulti*>(savedThis))->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!BuildSpriteImageKeyTable(static_cast<CMulti*>(savedThis)) /* 0x23b5 -> 0xdd540 */) {
        goto fail0;
    }
    BuildHelpReveal(0);
    RegisterInputBindings();
    if (!LoadLevelSounds(1) /* 0x1c2b -> 0xdb6c0 */) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        (static_cast<CMulti*>(savedThis))->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadGameSounds(1) /* 0x2964 -> 0xdb930 */) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        (static_cast<CMulti*>(savedThis))->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadGruntSoundNamespaces(0) /* 0x2e9b -> 0xdd830 */) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        (static_cast<CMulti*>(savedThis))->AckJoinFailure();
    }
    RegisterInputBindings();
    SetEffectSpriteDurations(); // 0x4458 -> 0xdc060
    if (savedThis != 0) {
        (static_cast<CMulti*>(savedThis))->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadLevelAnims(1) /* 0x2c07 -> 0xdb750 */) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        (static_cast<CMulti*>(savedThis))->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadGameAnims(1) /* 0x247d -> 0xdb9b0 */) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        (static_cast<CMulti*>(savedThis))->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!BuildWorldLevelPath(1)) { // vtable +0xa8 (CPlay slot 42)
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        (static_cast<CMulti*>(savedThis))->AckJoinFailure();
    }
    RegisterInputBindings();

    // finalize the world planes
    self->m_mgr->RecomputeViewScale();
    if (self->m_world->m_level->m_mainPlane != 0) {
        (static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane))->GetSize();
    }
    if (self->m_world->m_level->m_mainPlane != 0) {
        (static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane))->GetSize();
    }
    BuildHelpReveal(0);
    if (savedThis != 0) {
        (static_cast<CMulti*>(savedThis))->AckJoinFailure();
    }
    RegisterInputBindings();

    // view setup off host->m_70
    {
        CDDrawWorkerHost* g5c = static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane);
        CGruntzMapMgr* host70 = self->m_mgr->m_tileGrid;
        if (!host70->LoadAttributes(g5c->m_gridW, g5c->m_gridH)) {
            goto fail0;
        }
    }
    if (!(static_cast<CBrickzGrid*>(self->m_mgr->m_tileGrid))
             ->UpdateDiagonals(reinterpret_cast<i32>(self->m_mgr))) {
        goto fail0;
    }

    // lazily allocate the level context at +0x320
    if (self->m_lightFx == 0) {
        CLightFxRender* ctx = static_cast<CLightFxRender*>(RezAlloc(0x43c));
        if (ctx != 0) {
            ctx->m_mgr = 0;
            ctx->m_cmdGrid = 0;
            ctx->m_tileGrid = 0;
            ctx->m_world = 0;
            ctx->m_surface = 0;
            ctx->m_handle = 0;
            ctx->m_refreshInterval = 0;
            ctx->m_refreshRemaining = 0;
        } else {
            ctx = 0;
        }
        self->m_lightFx = ctx;
        if (!ctx->Init(self->m_mgr, 0xfa)) { // m_4 IS the CGruntzMgr
            goto fail0;
        }
    }
    if (!self->m_lightFx->BuildShape(self->m_levelType)) {
        goto fail0;
    }

    // ---- the WarpStone bute scan (single-mode only) ----
    gameReg = g_gameReg;
    if (gameReg->m_134 != 1) {
        CString warp; // [esp+0x14]
        i32 same = 0;
        if (warp.LoadString(0x81ab)) {
            char* a = const_cast<char*>(static_cast<const char*>(gameReg->GetWorldFileName()));
            char* b = const_cast<char*>(static_cast<const char*>(warp));
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
    if (self->m_mgr->m_134 == 3) {
        self->m_mgr->SyncOptionsState(); // 0x2e14, ecx=m_4
    }
    self->m_mgr->m_saveSink
        ->FillSlot2(reinterpret_cast<SaveSlot*>(&self->m_1d0), self->m_levelIndex, 0);
    {
        CString key; // [esp+0x18]
        gameReg = g_gameReg;
        gameReg->m_cmdGrid->m_pendingFx = 0;
        i32 count = self->m_levelIndex;
        i32 i = count - ((count - 1) % 4); // round-down-to-4 idiom
        for (; i < count; ++i) {
            // key.Format("Level%i", i) -> wsprintf-into-CString (0x1b2cf5)
            key.Format("Level%i", i);
            CTriggerMgr* bm = gameReg->m_cmdGrid;
            i32 v = g_buteMgr.GetInt(static_cast<const char*>(key), "WarpStone");
            bm->m_byteArr.SetAtGrow(
                bm->m_byteArr.GetSize(),
                static_cast<u8>(v)
            ); // inline GetSize == the +0x268 m_nSize load
        }
    }
    self->m_guts->LoadMultiplayerBattlezConfig(self->m_levelIndex);

    // ---- CursorSnapSprite registration (factory at [self+0xc]->m_8) ----
    set = self->m_world->m_childGroup->CreateSprite(0, 0, 0, 0x13880, "CursorSnapSprite", 0x40001);
    self->m_scrollSink = static_cast<CWwdGameObjectA*>(set);
    if (set != 0) {
        self->m_world->m_childGroup->TickKillCues(0); // vtable +0x24, the real slot 9
        if (savedThis == 0) {
            // empty cursor-snap set -> reset the resource-install flag
            CStatusBarMgr* tiles = self->m_guts;
            i32 id = (tiles->m_position == 0) ? 0x1a9 : 0x249;
            if (!self->m_frameMarker->LoadTimerSprite(id, 0x1ca)) {
                CTimer* spr = self->m_frameMarker;
                if (spr != 0) {
                    spr->Reset();
                    RezFree(spr);
                    self->m_frameMarker = 0;
                }
            }
        } else {
            // load the level map + the four map sub-steps
            if (LoadWarlordSprites(
                    reinterpret_cast<i32>(savedThis),
                    reinterpret_cast<i32*>(nameBuf + 0x20)
                )                                                        /* 0x2b80 */
                && ScanBuildTiles() /* 0x3553 */ && ValidateLevelTiles() /* 0x345e */
                && AddLevelGruntz() /* 0x17ee */) {
                self->m_world->m_childGroup->TickKillCues(0);
                self->m_guts->winapi_107d00_SetRect();
                (static_cast<DirectInputMgr2*>(g_inputMgr))->ReadAll();
                while (ShowCursor(0) >= 0)
                    ;
                self->m_mgr->RefreshGameClock(); // 0x8f620 direct (thunk 0x3d23)
                if (self->m_world->m_level->m_mainPlane != 0) {
                    (static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane))
                        ->GetSize();
                }
                if (self->m_world->m_level->m_mainPlane != 0) {
                    (static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane))
                        ->GetSize();
                }
                BuildHelpReveal(0);
                if (savedThis != 0) {
                    (static_cast<CMulti*>(savedThis))->AckJoinFailure();
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
        (static_cast<CMulti*>(savedThis))->AckJoinFailure();
    }
    RegisterInputBindings();
    BuildHelpReveal(1);
    ActiveWait(0x64);
    if (savedThis != 0) {
        (static_cast<CMulti*>(savedThis))->AckJoinFailure();
    }

    gameReg = g_gameReg;
    if (gameReg->m_114 == 0) {
        CDDSurface* mapHost = self->m_world->m_drawTarget->m_frontPair->m_surface;
        mapHost->ShadeRect(0x32, 0);
        gameReg = g_gameReg;
    }

    // ---- loading-screen blit (mode != 2 && m_114 == 0) ----
    if (gameReg->m_134 != 2 && gameReg->m_114 == 0) {
        CString scr; // [esp+0x14]
        self->m_inGame = 1;
        self->m_hudSuppressed = 0;
        i32 rect[4];
        rect[0] = 0;
        rect[1] = 0;
        rect[2] = 0x280;
        rect[3] = 0x1e0;
        if (scr.LoadString(0x8128)) {
            EngStr_DrawText(
                self->m_world,
                reinterpret_cast<i32>(rect),
                reinterpret_cast<i32>(nameBuf + 0x4),
                0x78,
                1,
                0xff,
                0xff,
                0,
                1
            );
        }
    } else {
        self->m_hudSuppressed = 1;
    }

    // ---- final state stamp ----
    self->m_scrollEdgeLock = 0;
    self->m_overlayDrag = 0;
    self->m_paused = 0;
    self->m_4f0 = 0;
    self->m_winLoseBanner = 0;
    self->m_cueInterval = 0x1f4;
    self->m_cueIntervalHi = 0;
    self->m_cueTimerLo = g_frameTime;
    self->m_cueTimerHi = 0;
    self->m_cueToggle = 1;
    self->m_cueText = g_emptyString;
    self->m_lastCueId = 0;
    self->m_region0Gate = 0;
    self->m_region1Gate = 0;
    self->m_region2Gate = 0;
    self->m_region3Gate = 0;
    self->m_snapshotActive = 0;
    self->m_514 = 3;
    self->m_renderDisabled = 1;
    g_playActive = 0;
    ResetViewport(); // 0x3d55 -> 0xd8c60
    if ((g_gameReg)->m_134 == 2) {
        g_playActive = 1;
        self->m_renderDisabled = 0;
        self->m_mgr->CheckSavedMode();
        self->m_mgr->m_chatLog->FreeNodes();
    }
    return 1;

fail1:
    return 0;
fail0:
    return 0;
}

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
    if (m_world) {
        m_world->m_childGroup->DestroyChildren_159ef0();
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
// device (CDDrawWorkerRegistry::MapTeardown) and the +0x24 slot-17
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
    if (m_world) {
        if (m_world->m_soundRegistry->m_2c) {
            m_world->m_soundRegistry->m_2c->Stop();
        }
        m_world->m_soundRegistry->ClearMap();
    }
    if (m_mgr) {
        m_mgr->m_sound->StopAndFlush();
        // WRONG-CALLEE FIX (assert_relocs): this called Resume() @0x00bcf0. Retail's rel32 here
        // resolves to 0x00b660 == CWorldSoundSet::Teardown (already 100% EXACT in src). Both
        // exist and are methods of the SAME class, so it linked and objdiff reloc-masked it -
        // a silently-wrong callee. Tearing the world sound set down on mode cleanup is also the
        // only reading that makes sense next to the StopAndFlush above.
        m_mgr->m_inputState->Teardown();
    }
    if (m_world) {
        m_world->m_imageRegistry->MapTeardown(); // slot 22 (+0x58)
    }
    if (m_world) {
        m_world->m_animRegistry->FreeAll(); // @0x152720
    }
    if (m_world) {
        m_world->m_level->ReleaseChildren(); // slot 17 (+0x44) - real CGameLevel virtual
    }
    if (m_world) {
        m_world->m_childGroup->DestroyChildren_159ef0();
    }
    if (m_world) {
        m_world->m_workerList->ClearWorkers(); // @0x163c60
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
i32 CPlay::Vslot0b(i32 key, i32 flag) {
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
        m_mgr->ReportError(0x800a, 0x456);
        return 1;
    }
    if (m_paused != 0) {
        m_paused = 0;
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, 0x111, 0x816e, 0);
        return 1;
    }
    if (m_mgr->m_frameGate != 0) {
        return 0;
    }
    if (m_hitTest->m_10 != 0) {
        m_mgr->m_chatLog->TypeChar(key, flag); // 0x3508 -> CFontConfig::TypeChar @0x21e20
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

RVA(0x000d0050, 0x3a)
i32 CPlay::CountObjectsByCategory(i32 category) {
    CObList* container = &m_world->m_childGroup->m_list;
    if (container == 0) {
        return 0;
    }
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(container->GetHeadPosition());
    i32 count = 0;
    while (node != 0) {
        CDDrawGroupNode* p = node;
        node = node->m_next;
        CGameObject* sprite = p->m_obj;
        if (sprite != 0 && sprite->m_collCategory == static_cast<u32>(category)) {
            count++;
        }
    }
    return count;
}

RVA(0x000d00a0, 0x5a)
void CPlay::PostSetup(void* dc) {
    RECT src = *(&m_world->m_level->m_planeCtx);
    RECT dst;
    CopyRect(&dst, &src);
    m_mgr->m_chatLog->DrawTextLines(8, static_cast<HDC>(dc), &dst, 0x10);
}

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
                CGruntzMgr* w = m_mgr;
                i32 id = g_curPlayer;
                void* spr =
                    reinterpret_cast<void*>(w->m_spriteFactory->GetSel(w->m_options[id].m_008, 0));
                if (spr == 0) {
                    spr = reinterpret_cast<void*>(
                        g_gameReg->m_spriteFactory->GetSel(1, reinterpret_cast<i32>(spr))
                    );
                }
                m_grid->SetAllTypes(0xa);
                m_grid->SetAllFormats(reinterpret_cast<i32>(spr));
            }
            char buf[0x40];
            wsprintfA(buf, "AMBIENT%d", GetAmbientId());
            if (g_gameReg->m_musicEnabled) {
                m_mgr->m_sound->PlayByName(buf, 1);
            }
            m_ambientInitDone = 1;
            break;
        }
    }

    i32* p;
    p = &m_syncTimerLo;
    SYNC_PAIR(ar, mode, p);
    if (!(reinterpret_cast<CLevelSync*>(m_guts))
             ->Sync(ar, mode, a2, a3)) { // guts child-sync @0x1084d0
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

DATA(0x00212618)
i32 g_lastLevelNum = -1;

// (The CArchiveMgr/CArchiveDefInt/CArchiveSubArray/CArchiveLoadRec views are GONE:
// CArchiveLoadRec was a full-object 0x514 placeholder of CPLAY ITSELF (the fn is
// CPlay::SyncWrite19fb, SyncState's mode-4 serializer, called `mov ecx,edi; push ar`),
// its "manager" CArchiveMgr was the CState::m_c holder (the +0x10 registry probe is
// CDDrawWorkerRegistry::AnyValueMatches - the frame-name reverse lookup over
// m_gridCurFrame), its "+0x24 inline name" object was CPlay::m_grid (the CImageSet's
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
    CDDrawSurfaceMgr* mc = m_world;
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
    for (u32 i0 = 0; i0 < static_cast<u32>(c0); i0++) {
        s->Write(markerData()[i0], 8);
    }

    Anchor* p = m_anchors; // the four 8-byte fx-anchor pairs at +0x384
    for (i32 k0 = 4; k0 != 0; k0--) {
        s->Write(p, 8);
        p++;
    }

    for (i32 k1 = 0; k1 < 4; k1++) {
        i32 cnt = arrCount(k1);
        s->Write(&cnt, 4);
        for (u32 i1 = 0; i1 < static_cast<u32>(cnt); i1++) {
            s->Write(arrData(k1)[i1], 8);
        }
    }

    s->Write(&m_cueToggle, 4);

    g_serialCounter++;
    {
        char buf[0x200];
        memset(buf, 0, sizeof(buf));
        strcpy(buf, static_cast<const char*>(m_cueText));
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
            mc->m_imageRegistry->AnyValueMatches(m_gridCurFrame, buf, &v);
        }
        s->Write(buf, 0x80);
        s->Write(&v, 4);
    }

    g_serialCounter++;
    {
        char buf[0x80];
        memset(buf, 0, sizeof(buf));
        if (m_grid != 0) {
            strcpy(buf, m_grid->m_name);
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
// CPlay::SyncRead2f7c (0x0d8060) - the mode-7 (read) play-state serializer, the
// symmetric inverse of SyncWrite19fb above. Retail's
// CPlay::SyncState (0xd7520, 100% EXACT) calls it `mov ecx,edi(=this); push ar`
// through thunk 0x2f7c, and BroadcastCmd hands SyncState the play state from
// PickPlayOrPausedState (thunk 0x355d -> 0x92990) - the receiver is the PLAY
// STATE, not a grunt. The old `CGrunt::Load` reconstruction was the WRONG overlay:
// its "CGrunt fields" (m_toySprite@+0x1bc etc.) coincided numerically, but every
// offset it reads is a CPlay field the Write twin already names (m_1bc, m_1c0,
// m_savedClock@+0x1cc, m_rngSeed@+0x2d8, ... m_514) - one field map, two
// serializers. The Grunt.h Load-support views (GruntLoadColl/GruntResMgr/
// GruntLoadStr/g_load612618) die with it: the collections are the real
// m_startMarkers/m_3a4[4]/m_488 CPtrArrays, the "GruntIdEntry" IS ::CImageSet
// (m_14==m_frames, m_64/m_68==m_minIndex/m_maxIndex), the resource manager is
// the canonical CDDrawSurfaceMgr, "g_load612618" is g_lastLevelNum, and the
// +0x410 CString is m_cueText.
// ---------------------------------------------------------------------------
// Bails (return 0) if the archive or the world holder (g_gameReg->m_world) is
// null. Reads back the scalar block, rebuilds the marker/placed-object/488
// collections by recycling each node onto the global coord free-list and
// re-popping fresh ones per streamed record, resolves the grid frame by
// (name,index) through the image registry, the frame grid by name, and the
// scroll-sink game object through the factory's serialize map (validated by
// GetClassId == CLASSID_SERIALREF), then streams the remaining scalars.
// @early-stop
// reloc-masked-extern plateau (SyncWrite19fb's symmetric pair): the field/record
// Read stream, the freelist recycle + CPtrArray SetSize/SetAtGrow rebuild loops,
// the registry/map lookups and the load-fail bail are reconstructed in shape/
// order. Residue is the ~90 archive-Read + collection + map-lookup call operands
// pairing to differently named retail symbols (the whole referent set is
// external). Final sweep.
RVA(0x000d8060, 0x6ce)
i32 CPlay::SyncRead2f7c(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* res = g_gameReg->m_world;
    if (res == 0) {
        return 0;
    }

    ar->Read(&m_1bc, 4);
    ar->Read(&m_1c0, 4);
    ar->Read(&m_savedClock, 4);
    ar->Read(&m_rngSeed, 4);
    ar->Read(&m_dragInProgress, 4);
    ar->Read(&m_2f0, 4);
    ar->Read(&m_cursorFrame, 4);
    ar->Read(&m_levelId, 4);
    ar->Read(&m_2fc, 8);
    ar->Read(&m_tileClickX, 8);
    ar->Read(&m_dragInhibit1, 4);
    ar->Read(&m_dragInhibit2, 4);

    {
        // rebuild the start-marker array: recycle the old nodes onto the coord
        // pool, then pop + fill one fresh node per streamed record.
        for (i32 i = 0; i < markerCount(); i++) {
            void* node = markerData()[i];
            if (node) {
                CoordPoolNode* q = g_coordPool.NodeOf(node);
                q->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = q;
            }
        }
        m_startMarkers.SetSize(0, -1);
        i32 n;
        ar->Read(&n, 4);
        for (u32 j = 0; j < static_cast<u32>(n); j++) {
            void* node = 0;
            CoordPoolNode* head = g_coordPool.m_freeHead;
            CoordPoolNode* next = head->m_next;
            if (next) {
                node = reinterpret_cast<char*>(&head->m_coord);
                g_coordPool.m_freeHead = next;
            }
            ar->Read(node, 8);
            m_startMarkers.SetAtGrow(markerCount(), node);
        }
    }

    {
        // the four 8-byte fx-anchor pairs at +0x384 (raw block, as the Write twin).
        Anchor* q = m_anchors;
        for (i32 k = 4; k != 0; k--) {
            ar->Read(q, 8);
            q++;
        }
    }

    {
        // rebuild the four placed-object arrays (+0x3a4, stride 0x14).
        for (i32 k = 0; k < 4; k++) {
            CPtrArray* coll = &m_3a4[k];
            for (i32 i = 0; i < arrCount(k); i++) {
                void* node = arrData(k)[i];
                if (node) {
                    CoordPoolNode* q = g_coordPool.NodeOf(node);
                    q->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = q;
                }
            }
            coll->SetSize(0, -1);
            i32 n;
            ar->Read(&n, 4);
            for (u32 j = 0; j < static_cast<u32>(n); j++) {
                void* node = 0;
                CoordPoolNode* head = g_coordPool.m_freeHead;
                CoordPoolNode* next = head->m_next;
                if (next) {
                    node = &head->m_coord;
                    g_coordPool.m_freeHead = next;
                }
                ar->Read(node, 8);
                coll->SetAtGrow(arrCount(k), node);
            }
        }
    }

    ar->Read(&m_cueToggle, 4);
    g_serialCounter++;
    {
        char buf512[0x200];
        ar->Read(buf512, 0x200);
        m_cueText = buf512;
    }
    ar->Read(&m_lastCueId, 4);
    ar->Read(&g_lastLevelNum, 4);

    // resolve the current grid frame by (set name, frame index) through the image
    // registry's name map: the record IS a ::CImageSet (frames @+0x14, populated
    // range m_minIndex/m_maxIndex) - the inverse of the Write twin's
    // AnyValueMatches reverse lookup.
    g_serialCounter++;
    char buf80a[0x80];
    ar->Read(buf80a, 0x80);
    i32 idx;
    ar->Read(&idx, 4);
    if (strlen(buf80a) == 0) {
        m_gridCurFrame = 0;
    } else {
        CImageSet* set = 0;
        (reinterpret_cast<CMapStringToPtr*>(&res->m_imageRegistry->m_10map))
            ->Lookup(static_cast<const char*>(buf80a), reinterpret_cast<void*&>(set));
        if (set == 0 || idx < set->m_minIndex || idx > set->m_maxIndex) {
            m_gridCurFrame = 0;
        } else {
            m_gridCurFrame = static_cast<CImage*>(set->m_items.GetAt(idx));
        }
    }

    // resolve the frame grid by name through the same registry map.
    g_serialCounter++;
    char buf80b[0x80];
    ar->Read(buf80b, 0x80);
    void* gridObj = 0;
    if (strlen(buf80b) == 0) {
        m_grid = 0;
    } else {
        (reinterpret_cast<CMapStringToPtr*>(&res->m_imageRegistry->m_10map))
            ->Lookup(buf80b, gridObj);
        m_grid = static_cast<CImageSet*>(gridObj);
    }

    ar->Read(&m_gridDelayBase, 4);
    ar->Read(&m_gridDelayCount, 4);
    ar->Read(&m_gridRow, 4);
    g_serialCounter++;
    i32 v;
    ar->Read(&v, 4);
    // resolve the scroll-sink object through the factory's serialize map, gated on
    // the serialize-referent class tag (retail keys the lookup off the spilled
    // grid-resolve slot; data flow preserved from the byte-validated shape).
    CGameObject* oe = 0;
    res->m_childGroup->m_map48.Lookup(gridObj, reinterpret_cast<void*&>(oe));
    CWwdGameObjectA* sink;
    if (oe == 0) {
        sink = 0;
    } else {
        // GetClassId (slot 8) == CLASSID_SERIALREF (5): the serialize-map referent kind
        sink = (reinterpret_cast<CGameObject*>(oe))->GetClassId() == CLASSID_SERIALREF
                   ? reinterpret_cast<CWwdGameObjectA*>(oe)
                   : 0;
    }
    m_scrollSink = sink;
    if (sink == 0 && gridObj != 0) {
        return 0;
    }

    ar->Read(&m_gridWalkActive, 4);
    ar->Read(&m_renderDisabled, 4);
    ar->Read(&m_winLoseBanner, 4);
    ar->Read(&m_1c4, 4);
    ar->Read(&m_hudSuppressed, 4);
    ar->Read(&m_inGame, 4);
    ar->Read(&m_overlayDrag, 4);
    ar->Read(&m_paused, 4);
    ar->Read(&m_4f0, 4);
    ar->Read(&m_dragEndNotify, 4);
    ar->Read(&m_drewThisFrame, 4);
    ar->Read(&m_418, 4);
    ar->Read(&m_41c, 4);
    ar->Read(&m_420, 4);
    ar->Read(&m_424, 4);
    ar->Read(&m_428, 2);
    ar->Read(&m_region0Gate, 4);
    ar->Read(&m_region1Gate, 4);
    ar->Read(&m_region2Gate, 4);
    ar->Read(&m_region3Gate, 4);
    ar->Read(&m_viewMode, 4);
    ar->Read(&m_snapshotActive, 4);
    ar->Read(&m_gridHasSprite, 4);
    ar->Read(&m_49c, 4);
    m_stepCountdown = 2; // retail forces the post-load step countdown ([ebx+0x510] = 2)
    ar->Read(&m_514, 4);

    i32 n488;
    ar->Read(&n488, 4);
    {
        for (i32 i = 0; i < arr488Count(); i++) {
            void* node = arr488Data()[i];
            if (node) {
                CoordPoolNode* q = g_coordPool.NodeOf(node);
                q->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = q;
            }
        }
        m_488.SetSize(0, -1);
        m_488.SetSize(n488, -1);
        for (u32 j = 0; j < static_cast<u32>(n488); j++) {
            void* node = 0;
            CoordPoolNode* head = g_coordPool.m_freeHead;
            CoordPoolNode* next = head->m_next;
            if (next) {
                node = reinterpret_cast<char*>(&head->m_coord);
                g_coordPool.m_freeHead = next;
            }
            ar->Read(node, 8);
            arr488Data()[j] = node;
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
    CGruntzMgr* w = m_mgr;
    CStatusBarMgr* guts = m_guts;
    i32 right = w->m_modeW;
    i32 state = guts->m_position;
    i32 bottom = w->m_modeH;
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
    m_world->m_level->BuildAllPlanes((&r)); // 0x15da80
    m_mgr->RecomputeViewScale();
    return 1;
}

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
// CPlay::ClampViewport (0x0d8dc0) - if the active viewport (m_c->m_level+0x10) is
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
    CDDrawSurfaceMgr* v = m_world;
    LevelCoordRect* vp = &v->m_level->m_planeCtx;
    RECT r;
    r.left = vp->left;
    r.top = vp->top;
    r.right = vp->right;
    r.bottom = vp->bottom;

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

    m_world->m_level->BuildAllPlanes((&r)); // 0x15da80
    m_world->m_drawTarget->m_backPair->m_surface->Fill(0);
    m_guts->Deactivate(); // 0x125d -> CStatusBarMgr::Deactivate @0x100cb0
    m_mgr->RecomputeViewScale();
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
    CDDrawSurfaceMgr* v = m_world;
    CGruntzMgr* w = m_mgr;
    CStatusBarMgr* guts = m_guts;

    i32* rp = reinterpret_cast<i32*>(&v->m_level->m_planeCtx);
    RECT r;
    r.left = rp[0];
    r.top = rp[1];
    r.right = rp[2];
    r.bottom = rp[3];

    i32 hlimit = w->m_modeW;
    i32 vlimit = w->m_modeH;

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

    m_world->m_level->BuildAllPlanes((&r)); // 0x15da80
    m_world->m_drawTarget->m_backPair->m_surface->Fill(0);
    m_guts->Deactivate();
    m_mgr->RecomputeViewScale();
    return 1;
}

RVA(0x000d88f0, 0x44)
void CPlay::RegionEnter() {
    if (m_savedZonedSound == 0) {
        CGruntzMgr* w = m_mgr;
        m_savedZonedSound = w->m_sound->m_pCurrent;
        w->m_sound->StopAll();
    }
    if (g_gameReg->m_musicEnabled
        != 0) { // +0x14 (the WAP32 base field; the registry view called it m_14)
        m_mgr->m_sound->PlayByName("CURSE", 0);
    }
}

RVA(0x000d8960, 0x75)
void CPlay::RegionLeave() {
    if (m_region0Gate == 0 && m_region1Gate == 0 && m_region2Gate == 0 && m_region3Gate == 0
        && m_savedZonedSound != 0) {
        m_mgr->m_sound->IsPlaying();
        m_mgr->m_sound->m_pCurrent = m_savedZonedSound;
        if (g_gameReg->m_musicEnabled
            != 0) { // +0x14 (the WAP32 base field; the registry view called it m_14)
            m_mgr->m_sound->Restart(1);
        }
        m_savedZonedSound = 0;
    }
}

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
    *reinterpret_cast<u64*>(&m_region0TimerLo) = g_frameTime; // 64-bit store: lo=g_frameTime, hi=0
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
    *reinterpret_cast<u64*>(&m_region1TimerLo) = g_frameTime; // 64-bit store: lo=g_frameTime, hi=0
    return 1;
}

RVA(0x000d8b20, 0x74)
i32 CPlay::OnRegion3(i32 z) // (region-2 / gate m_region2Gate, timer +0x450)
{
    if (z != 0) {
        m_region2Gate = 1;
        RegionEnter();
        Cmd_ApplyScrollParams(REGION_INTERVAL_MS, 6, 6, 0, 0x2d);
    } else {
        m_region2Gate = 0;
        RegionLeave();
    }
    m_region2Interval = REGION_INTERVAL_MS;
    m_region2IntervalHi = 0;
    *reinterpret_cast<u64*>(&m_region2TimerLo) = g_frameTime; // 64-bit store: lo=g_frameTime, hi=0
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
    *reinterpret_cast<u64*>(&m_region3TimerLo) = g_frameTime; // 64-bit store: lo=g_frameTime, hi=0
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
// Two real bugs fixed (76.4%->85.5%): (1) the type-discriminator chain had only 11
// distinct compares - VisFn_40fe90 appeared TWICE (positions 1 and 7), so cl CSE'd
// them to 11 cmps where retail has 12 distinct (the 7th is the fn at 0x47e160 via
// thunk 0x402d24); (2) the entity-list walk processed-then-advanced, where retail's
// inlined GetNext advances the node FIRST (cur=node; node=node->m_next; use cur).
// Residual: the head/RECT pointer-setup regalloc (retail derives vp/held via add
// reg,0x10; cl colours the three base pointers differently) + the 12 DIR32 reloc-name
// masks (retail compares ILT thunk addresses, delinker has no thunk symbols).
// ===========================================================================


RVA(0x000d9050, 0xc7)
i32 CPlay::NotifyVisibleEntities() {
    CDDrawSurfaceMgr* v = m_world;
    i32* vp = reinterpret_cast<i32*>(&v->m_level->m_planeCtx);
    CDDrawSurfacePair* held = v->m_drawTarget->m_backPair;
    CDDrawGroupNode* node =
        reinterpret_cast<CDDrawGroupNode*>(v->m_childGroup->m_list.GetHeadPosition());

    RECT r;
    r.left = vp[0];
    r.top = vp[1];
    r.right = vp[2] + 1;
    r.bottom = vp[3] + 1;
    held->m_surface->Restore(&r, 0);

    while (node != 0) {
        CDDrawGroupNode* cur = node;
        node = node->m_next;
        CGameObject* o = cur->m_obj;
        void* id = static_cast<void*>(o->m_7c->m_notify);
        if (id == static_cast<void*>(VisFn_40fe90) || id == static_cast<void*>(VisFn_4bf150)
            || id == static_cast<void*>(VisFn_423b40) || id == static_cast<void*>(VisFn_Roll)
            || id == static_cast<void*>(VisFn_41e570) || id == static_cast<void*>(VisFn_41e520)
            || id == static_cast<void*>(VisFn_47e160) || id == static_cast<void*>(VisFn_49b410)
            || id == static_cast<void*>(VisFn_IntersectRect)
            || id == static_cast<void*>(VisFn_49b310) || id == static_cast<void*>(VisFn_CBattlezDlg)
            || id == static_cast<void*>(VisFn_4fce80)) {
            o->Render(held); // slot 11 (+0x2c) - CGameObject's real per-object render hook
        }
    }
    return 1;
}

// ===========================================================================
// CPlay::StepScroll - per-frame scroll-offset
// recompute. Reads the draw-surface (m_c->m_level) scroll origin (+0x10/+0x14) and
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
    CGameLevel* v = m_world->m_level;
    CLevelPlane* geom = v->m_mainPlane;

    i32 y = m_cursorY + (geom->m_originY - v->m_planeCtx.top);  // [edx+4]-m_14; +=m_cursorY
    i32 x = geom->m_originX + (m_cursorX - v->m_planeCtx.left); // [edx]; +=m_cursorX-m_10

    y = (y & ~0x1f) + 0x10; // align down 0x20 (and al,0xe0); + 0x10
    x = (x & ~0x1f) + 0x10; // align down 0x20 (and edi,~0x1f); + 0x10

    m_scrollSink->m_screenX = x;
    m_scrollSink->m_screenY = y;
}

RVA(0x000d1b30, 0x20)
i32 CPlay::SetCursorFrame(i32 item) {
    LoadCursorSprites(item, 0);
    m_cursorFrame = item;
    return 1;
}

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

    CDDSurface* half;
    Edge* edge;
    void* halfPtr;
    if (m_inputHalfSel == 0) {
        half = m_scratchSurface0;
        edge = reinterpret_cast<Edge*>(&m_188);
        halfPtr = &m_168;
    } else {
        half = m_scratchSurface1;
        edge = reinterpret_cast<Edge*>(&m_198);
        halfPtr = &m_178;
    }

    // null-check the draw surface m_c->m_4->m_14->m_2c (walks through the this reg).
    CDDSurface* probeTarget = m_world->m_drawTarget->m_backPair->m_surface;
    if (probeTarget == 0) {
        return 0;
    }

    i32 r = probeTarget->BltFast(edge->m_0, edge->m_4, half, halfPtr, 0x10);
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
    LevelCoordRect& vp = m_world->m_level->m_planeCtx;
    i32 l = vp.left, t = vp.top, r = vp.right, b = vp.bottom;
    i32 bottom = b - g_buteMgr.GetInt("Font", "TextBottomEdge");
    i32 right = r - g_buteMgr.GetInt("Font", "TextRightEdge");
    i32 top = t + g_buteMgr.GetInt("Font", "TextTopEdge");
    i32 left = l + g_buteMgr.GetInt("Font", "TextLeftEdge");
    SetRect(&rect, left, top, right, bottom);

    EngStr_DrawText(
        m_world,
        reinterpret_cast<i32>(&s),
        reinterpret_cast<i32>(&rect),
        0x78,
        1,
        0xff,
        0xff,
        0,
        1
    );
    m_stepCountdown = 2;
}

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
        i32* src = reinterpret_cast<i32*>(rectSrc);
        i32 bottom = src[3] - g_buteMgr.GetInt("Font", "TextBottomEdge");
        i32 right = src[2] - g_buteMgr.GetInt("Font", "TextRightEdge");
        i32 top = src[1] + g_buteMgr.GetInt("Font", "TextTopEdge");
        i32 left = src[0] + g_buteMgr.GetInt("Font", "TextLeftEdge");
        SetRect(&rect, left, top, right, bottom);
    } else {
        // the viewport rect (m_c->m_level->m_viewport) ptr (edx) does not survive
        // the GetInt calls, so all 4 corners are read up front.
        LevelCoordRect& vp = m_world->m_level->m_planeCtx;
        i32 l = vp.left, t = vp.top, r = vp.right, b = vp.bottom;
        i32 bottom = b - g_buteMgr.GetInt("Font", "TextBottomEdge");
        i32 right = r - g_buteMgr.GetInt("Font", "TextRightEdge");
        i32 top = t + g_buteMgr.GetInt("Font", "TextTopEdge");
        i32 left = l + g_buteMgr.GetInt("Font", "TextLeftEdge");
        SetRect(&rect, left, top, right, bottom);
    }

    if (a3 != 0) {
        ShowHudMessageAlt(
            m_world,
            reinterpret_cast<i32>(&m_cueText),
            reinterpret_cast<i32>(&rect),
            a2,
            1,
            a4,
            a5,
            a6,
            a7
        );
    } else {
        EngStr_DrawText(
            m_world,
            reinterpret_cast<i32>(&m_cueText),
            reinterpret_cast<i32>(&rect),
            a2,
            1,
            a4,
            a5,
            a6,
            a7
        );
    }
}

// ===========================================================================
// CPlay::DrawWorldFrame (0x0c9c20) - one in-game world-draw frame: the begin
// virtual (this->vtbl[+0x98]), the m_c->m_level->m_5c sub-step (if present), mirror
// the draw clock, present (m_c->m_childGroup->vtbl[+0x24]), the m_4->m_68 frame-timer step,
// the optional mode-3 reg cue, then the m_guts guts step.
// ===========================================================================
// @early-stop
// regalloc/scheduling wall — structure byte-identical, only temp-register naming
// (eax vs edx in the pointer-chain derefs) + one m_4/global load-order in the
// Step-call setup differ; see docs/patterns/zero-register-pinning.md.
RVA(0x000c9c20, 0x79)
void CPlay::DrawWorldFrame() {
    TickStateMgrs(); // slot 38, this->vtbl[+0x98]() (begin-frame virtual, thiscall; ex "Vslot26")
    if (m_world->m_level->m_mainPlane != 0) {
        m_world->m_level->m_mainPlane->CenterScrollA();
    }
    g_killCueClock = g_lastNow;
    g_engineFrameDelta = g_frameDelta;
    m_world->m_childGroup->TickKillCues(0); // m_c->m_childGroup->vtbl[+0x24](0)
    m_mgr->m_cmdGrid->LoadTeleporterGooConfig(
        static_cast<i32>(g_frameDelta)
    ); // 0x3017 -> 0x6eb80 per-frame grid step
    if (g_gameReg->m_134 == 3) {
        // 0x933e0 == CGruntzMgr::AdvanceOptionsCycle (rel32 via ILT 0x2d33).
        (g_gameReg)->AdvanceOptionsCycle();
    }
    m_guts->LoadDestructButtonSprite(static_cast<i32>(g_frameDelta)); // guts step @0xffb20
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
    i32 delta = static_cast<i32>(g_frameDelta);
    i32 steps = static_cast<i32>(
        (static_cast<u32>(delta) / FIXED_SUBSTEP_MS)
    ); // 0x38e38e39 magic-div by 18
    i32 now = static_cast<i32>(g_lastNow);
    i32 accum = static_cast<i32>(g_frameTime);
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
            m_mgr->SetGameClock(now, dt,
                                accum); // 0x3404 -> @0x8f7b0 (retail ecx=m_4)
            if (i > 0 && i < last) {
                if (m_world->m_level->m_mainPlane != 0) {
                    m_world->m_level->m_mainPlane->CenterScrollB();
                }
            }
            TickStateMgrs(); // slot 38, this->vtbl[+0x98]()
            if (m_world->m_level->m_mainPlane != 0) {
                m_world->m_level->m_mainPlane->CenterScrollA();
            }
            m_world->m_childGroup->TickKillCues(0);
            m_mgr->m_cmdGrid->LoadTeleporterGooConfig(static_cast<i32>(g_frameDelta));
            if (g_gameReg->m_134 == 3) {
                (g_gameReg)->AdvanceOptionsCycle(); // 0x933e0
            }
            m_guts->LoadDestructButtonSprite(static_cast<i32>(g_frameDelta));
            i++;
        } while (i < steps);
    }
    m_mgr->SetGameClock(saveNow, saveDelta, saveAccum); // tail step
    return steps;
}


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
    i32 renderMs = static_cast<i32>((tg() - t0));
    m_mgr->m_inputState->Retune( // 0x1a7d -> CWorldSoundSet::Retune (positional audio)
        m_world->m_level->m_mainPlane->m_originX,
        m_world->m_level->m_mainPlane->m_originY
    );
    u32 t2 = tg();
    m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
    m_world->m_workerList->PruneWorkers(
        m_world->m_drawTarget->m_backPair,
        m_world->m_drawTarget->m_overlayPair
    );
    i32 presentMs = static_cast<i32>((tg() - t2));
    ProfLog(
        &g_brickText1,
        "Delta=%i, Update=%i, Draw=%i, NumUpdates=%i    ",
        static_cast<i32>(g_frameDelta),
        renderMs,
        presentMs,
        updates
    );
    DrawDebugStats();
    m_world->m_drawTarget->m_frontPair->m_surface->Flip(0);
    if (m_world->m_level->m_mainPlane != 0) {
        m_world->m_level->m_mainPlane->CenterScrollB();
    }
    return 1;
}

DATA_SYMBOL(0x0024c284, 0x4, _g_profAccA)
DATA_SYMBOL(0x0024c288, 0x4, _g_profAccB)

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
    m_mgr->m_inputState->Retune( // 0x1a7d -> CWorldSoundSet::Retune (positional audio)
        m_world->m_level->m_mainPlane->m_originX,
        m_world->m_level->m_mainPlane->m_originY
    ); // untimed
    DWORD(WINAPI * tg)(void) = ::timeGetTime;

    u32 t1 = tg();
    TickStateMgrs(); // slot 38, this->vtbl[+0x98]
    i32 activateMs = static_cast<i32>((tg() - t1));

    u32 t3 = tg();
    if (m_world->m_level->m_mainPlane != 0) {
        m_world->m_level->m_mainPlane->CenterScrollA();
    }
    i32 deactMs = static_cast<i32>((tg() - t3));

    u32 t5 = tg();
    m_world->m_childGroup->TickKillCues(1);
    m_mgr->m_cmdGrid->LoadTeleporterGooConfig(static_cast<i32>(g_frameDelta));
    m_guts->LoadDestructButtonSprite(static_cast<i32>(g_frameDelta));
    i32 updateMs = static_cast<i32>((tg() - t5));

    u32 t7 = tg();
    i32 hitTestMs = static_cast<i32>((tg() - t7));

    u32 t9 = tg();
    m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
    i32 drawMs = static_cast<i32>((tg() - t9));

    u32 t11 = tg();
    m_world->m_workerList->PruneWorkers(
        m_world->m_drawTarget->m_backPair,
        m_world->m_drawTarget->m_overlayPair
    );
    i32 fixedMs = static_cast<i32>((tg() - t11));

    u32 t13 = tg();
    m_guts->LoadMainStatusBarSprite(); // 0xfe6b0
    i32 statusBarMs = static_cast<i32>((tg() - t13));

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
    g_profAccB = static_cast<i32>(tg());
    m_world->m_drawTarget->m_frontPair->m_surface->Flip(0);
    g_profAccB = static_cast<i32>((tg() - static_cast<u32>(g_profAccB)));
    g_profAccA = static_cast<i32>(tg());
    if (m_world->m_level->m_mainPlane != 0) {
        m_world->m_level->m_mainPlane->CenterScrollB();
    }
    g_profAccA = static_cast<i32>((tg() - static_cast<u32>(g_profAccA)));
    UpdateMgrScroll(g_gameReg, m_guts, m_region0Gate); // 0xebd70
    return 1;
}

RVA(0x000d5f00, 0x69)
i32 CPlay::ResetGoals(i32 x, i32 y) {
    CGruntzMgr* w = m_mgr;
    CTriggerMgr* g = w->m_cmdGrid;
    if (g->m_goal != 0) {
        g->m_goal->m_flags |= 0x10000;
        g->m_goal = 0;
    }
    g->m_armed = 0;
    CLevelPlane* pg = m_mgr->m_world->m_level->m_mainPlane;
    if ((pg->m_flags & 1) == 0) {
        pg->m_scaledX = static_cast<float>(x) * pg->m_scaleX;
        pg->m_scaledY = static_cast<float>(y) * pg->m_scaleY;
    } else {
        pg->m_scaledX = static_cast<float>(x);
        pg->m_scaledY = static_cast<float>(y);
    }
    pg->RecomputePlaneCoords();
    return 1;
}

RVA(0x000d9160, 0xac)
i32 CPlay::RegisterInputBindings() {
    m_mgr->m_gameWnd->PumpMessages(0x102, 0x40);
    m_mgr->m_gameWnd->PumpMessages(0x100, 0x40);
    m_mgr->m_gameWnd->PumpMessages(0x200, 0x40);
    m_mgr->m_gameWnd->PumpMessages(0x201, 0x40);
    m_mgr->m_gameWnd->PumpMessages(0x202, 0x40);
    m_mgr->m_gameWnd->PumpMessages(0x203, 0x40);
    m_mgr->m_gameWnd->PumpMessages(0x204, 0x40);
    m_mgr->m_gameWnd->PumpMessages(0x205, 0x40);
    m_mgr->m_gameWnd->PumpMessages(0x206, 0x40);
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
            if (g_levelBias100 != 0) {
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

    if ((g_gameReg)->GetWorldFileName().GetLength() != 0) {
        char buf[128];
        wsprintfA(buf, (g_gameReg)->GetWorldFileName());
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
    EngStr_DrawText(
        m_world,
        reinterpret_cast<i32>(&s0),
        reinterpret_cast<i32>(&r1),
        0x78,
        0,
        0,
        0,
        0,
        1
    );
    EngStr_DrawText(
        m_world,
        reinterpret_cast<i32>(&s1),
        reinterpret_cast<i32>(&r2),
        0x6e,
        0,
        0,
        0,
        0,
        1
    );
    EngStr_DrawText(
        m_world,
        reinterpret_cast<i32>(&s2),
        reinterpret_cast<i32>(&r3),
        0x6e,
        0,
        0,
        0,
        0,
        1
    );
    EngStr_DrawText(
        m_world,
        reinterpret_cast<i32>(&s3),
        reinterpret_cast<i32>(&r4),
        0x6e,
        0,
        0,
        0,
        0,
        1
    );
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
// CPlay::ClearPlacedObjects (0x0da030) -
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
            CHitMarker* obj = static_cast<CHitMarker*>(rec->GetAt(i));
            CTileGrid* grid = g_gameReg->m_tileGrid;
            CGameObject* cellObj = 0;
            if (static_cast<u32>(obj->m_0) < static_cast<u32>(grid->m_width)
                && static_cast<u32>(obj->m_4) < static_cast<u32>(grid->m_height)) {
                i32 stride = obj->m_0 * 7;
                i32* row = grid->m_rowInts[obj->m_4];
                cellObj = reinterpret_cast<CGameObject*>(row[stride + 2]);
            }
            if (cellObj == 0) {
                restart = 1;
                break;
            }
            void* out = 0;
            // The factory's embedded +0x48 key->object map, reached as the real
            // MFC CMapPtrToPtr (the documented SpriteFactory.h consumer pattern).
            CMapPtrToPtr* map = &g_gameReg->m_world->m_childGroup->m_map48;
            CGameObject* result = cellObj;
            if (map->Lookup(cellObj, out)) {
                result = static_cast<CGameObject*>(out);
            }
            if (result == 0) {
                // cell vacated: clear the cell's occupant + flag bit and unlink.
                CTileGrid* g = g_gameReg->m_tileGrid;
                if (static_cast<u32>(obj->m_0) < static_cast<u32>(g->m_width)
                    && static_cast<u32>(obj->m_4) < static_cast<u32>(g->m_height)) {
                    i32 stride = obj->m_0 * 7;
                    i32* row = g->m_rowInts[obj->m_4];
                    row[stride + 2] = 0;
                    i32* row2 = g->m_rowInts[obj->m_4];
                    row2[stride] &= 0xfffbffff;
                }
                rec->RemoveAt(i, 1);
                // return the placed-object node to the MFC free list (the node
                // header sits g_coordPool.m_linkOffset bytes before the payload).
                CoordPoolNode* node = g_coordPool.NodeOf(obj);
                node->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = node;
                return -1;
            }
            if (result->m_124 != 0x14) {
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
        static_cast<void>(restart);
    }
    return -1;
}

RVA(0x000da2d0, 0xa5)
i32 CPlay::FlushPendingOps() {
    if (m_4f0 != 0) {
        return 0;
    }
    i32 changed = 0;
    if (m_dragInhibit1 != 0) {
        CStatusBarMgr* worker = m_guts;
        m_dragInhibit1 = 0;
        worker->CommitSlot(0);
        SetCursorFrame(0); // via the 0x17a8 ILT thunk in retail
        changed = 1;
    }
    if (m_dragInhibit2 != 0) {
        i32 spr = m_cursorFrame;
        CStatusBarMgr* worker = m_guts;
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

RVA(0x000da3b0, 0x6e)
i32 CPlay::CanQuickSave() {
    if (m_renderDisabled == 0 && m_inGame == 0 && m_overlayDrag == 0 && m_snapshotActive == 0
        && m_guts->m_hlBusy == 0 && m_guts->m_toggleActive == 0 && m_guts->m_toggleHandle == 0
        && g_gameReg->m_frameGate == 0 && g_gameReg->m_cmdGrid->m_groupFlag != 0) {
        return 1;
    }
    return 0;
}

RVA(0x000da440, 0x60)
i32 CPlay::PostHudRect() {
    if (m_worldReady != 0) {
        m_mgr->m_cmdGrid->HudRect(m_hudRect, g_spawnConfig->m_18 & 0x20);
    }
    m_worldReady = 0;
    m_dragSnapActive = 0;
    return 1;
}

#include <string.h> // inlined memset / strcpy in Serialize (rep stos / rep movs)

CString GetColorName(i32 colorIdx, i32 upper);
CString GetDifficultyName(i32 diffIdx, i32 upper);

// ===========================================================================
// GruntzPlayer::GruntzPlayer()  (0x0da790) - THE default constructor
// ===========================================================================
// Construct the CString m_name + the CBattlezMapConfig m_038, then run the shared
// frameless field seed (the Clear body at 0x0da960, /O2-inlined here exactly as retail
// inlines it: the -1 / -2 / 1 / 0xf sentinels + the empty-string assign into m_name).
// The two destructible members drive the /GX frame.
// @early-stop
// /GX EH-state wall (docs/seh-eh.md): the member ctor calls (CString, CBattlezMapConfig),
// the empty-string assign and the field seeds are all recovered; the residual is a
// 2-destructible-member ctor's EH state numbering + cl's interleave of the field stores
// (same family as the CWarlord dtor wall).
RVA(0x000da790, 0xb0)
GruntzPlayer::GruntzPlayer() {
    m_latency = 0;
    m_230 = 0;
    m_playerIndex = -1;
    m_slotKey = -2;
    m_liveGate = 0;
    m_joined = 0;
    m_014 = 1;
    m_name = g_emptyString;
    m_008 = 0;
    m_configId = 0;
    m_focusX = 0;
    m_focusY = 0;
    m_comboSel = 0xf;
    m_doneFlag = 0;
    m_030 = 0;
    m_latency = 0;
    m_230 = 0;
}

// ===========================================================================
// GruntzPlayer::~GruntzPlayer  (0x083260)
// ===========================================================================
// Reverse the ctor: run the shared field seed (Clear, 0x0da960 - retail's FIRST call in
// this body; it is referenceable now that 0x0da960 is bound as a METHOD instead of a
// phantom second constructor, which is what previously blocked this dtor), then cl
// destructs the +0x38 CBattlezMapConfig and the CString m_name. The /GX frame numbers
// the teardowns 2 / 0 / -1.
// @early-stop
// /GX teardown-state wall: the Clear() call + both member destructions are present and
// bind to their real bodies; the residual is cl's EH state numbering for the 2-member
// teardown (same family as the ctor above).
RVA(0x00083260, 0x57)
GruntzPlayer::~GruntzPlayer() {
    Clear();
}

// @early-stop
// ~53%: the ctor-vs-method misbinding is FIXED (retail returns 1, so this is a
// plain method - no member CString ctor is emitted now), but the /GX frame shape
// + the g_emptyString/GetDefaultName CString traffic still diverge (static-MFC
// packaged-inline artifacts; see cstring-empty-init-version-divergence.md).
// Deferred to the final sweep.
RVA(0x000da870, 0xb8)
i32 GruntzPlayer::SeedForSlot(i32 index) {
    m_name = g_emptyString;
    m_playerIndex = index;
    m_slotKey = -2;
    m_liveGate = 0;
    m_joined = 0;
    m_014 = 1;
    m_name = GetDefaultName();
    m_008 = index;
    m_configId = 0;
    m_focusX = 0;
    m_focusY = 0;
    m_comboSel = 0xf;
    m_doneFlag = 0;
    m_030 = 0;
    m_latency = 0;
    m_230 = 0;
    return 1;
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
    m_slotKey = -2;
    m_liveGate = 0;
    m_014 = 1;
    m_name = g_emptyString;
    m_008 = 0;
    m_configId = 0;
    m_focusX = 0;
    m_focusY = 0;
    m_comboSel = 0xf;
    m_doneFlag = 0;
    m_030 = 0;
    m_latency = 0;
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
    m_slotKey = -2;
    m_liveGate = 0;
    m_014 = 1;
    m_name = g_emptyString;
    m_008 = 0;
    m_configId = 0;
    m_focusX = 0;
    m_focusY = 0;
    m_comboSel = 0xf;
    m_doneFlag = 0;
    m_030 = 0;
    m_latency = 0;
    m_230 = 0;
    return 1;
}

RVA(0x000daa60, 0x24)
i32 GruntzPlayer::ClearRoundState() {
    m_liveGate = 1;
    m_readyFlag = 0;
    m_doneFlag = 0;
    m_030 = 0;
    m_latency = 0;
    m_230 = 0;
    return 1;
}

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
        pSend(cb, 0x143, 0, reinterpret_cast<i32>(static_cast<const char*>(s)));
    }
    if (curSel >= 0) {
        pSend(cb, 0x14e, curSel, 0);
    }
    return 1;
}

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
        pSend(cb, 0x143, 0, reinterpret_cast<i32>(static_cast<const char*>(s)));
    }
    if (curSel >= 0) {
        pSend(cb, 0x14e, curSel, 0);
    }
    return 1;
}

RVA(0x000dace0, 0x239)
i32 GruntzPlayer::Serialize(void* arArg, i32 kind, i32 a3, i32 a4) {
    CSerialArchive* ar = static_cast<CSerialArchive*>(arArg);
    char tmp[0x80];
    // Retail lays the kind==4 (Save, [+0x30]) arm out of line and keeps the
    // kind==7 (Load, [+0x2c]) arm inline: `cmp 4; je SAVE / cmp 7; jne TAIL`.
    if (kind != 4) {
        if (kind == 7) {
            // Load.
            ar->Read(&m_playerIndex, 4);
            ar->Read(&m_008, 4);
            ar->Read(&m_00c, 4);
            ar->Read(&m_configId, 4);
            ar->Read(&m_014, 4);
            ar->Read(&m_slotKey, 4);
            ar->Read(&m_readyFlag, 4);
            ar->Read(&m_liveGate, 4);
            ar->Read(&m_joined, 4);
            ar->Read(&m_clearedRound, 4);
            g_serialCounter++;
            ar->Read(tmp, 0x80);
            m_name = tmp;
            ar->Read(&m_focusX, 4);
            ar->Read(&m_focusY, 4);
            ar->Read(&m_comboSel, 4);
        }
    } else {
        // Save.
        ar->Write(&m_playerIndex, 4);
        ar->Write(&m_008, 4);
        ar->Write(&m_00c, 4);
        ar->Write(&m_configId, 4);
        ar->Write(&m_014, 4);
        ar->Write(&m_slotKey, 4);
        ar->Write(&m_readyFlag, 4);
        ar->Write(&m_liveGate, 4);
        ar->Write(&m_joined, 4);
        ar->Write(&m_clearedRound, 4);
        g_serialCounter++;
        memset(tmp, 0, sizeof(tmp));
        strcpy(tmp, static_cast<const char*>(m_name));
        ar->Write(tmp, 0x80);
        ar->Write(&m_focusX, 4);
        ar->Write(&m_focusY, 4);
        ar->Write(&m_comboSel, 4);
    }
    return (static_cast<CBattlezMapConfig*>(&m_038))
               ->SerializeState(reinterpret_cast<i32>(ar), reinterpret_cast<void*>(kind), a3, a4)
           != 0;
}

RVA(0x000dafb0, 0x71)
CString GruntzPlayer::GetDefaultName() {
    // Retail builds a named local temp, then NRV-copies it into the return slot
    // (op= copy-ctor) and destructs the temp -> the /GX frame. A direct
    // `return CString("Player");` would NRV-construct in place (frameless).
    CString name("Player");
    return name;
}

DATA(0x00212f78)
char* g_colorNames[] =
    {"Color 0", "Color 1", "Color 2", "Color 3", "Color 4", "Color 5", "Color 6", "Color 7"};
DATA(0x00212fc0)
char* g_difficultyNames[] = {"Easy", "Normal", "Hard"};

RVA(0x000db050, 0x90)
CString GetColorName(i32 colorIdx, i32 upper) {
    CString s;
    s = g_colorNames[colorIdx];
    if (upper) {
        s.MakeUpper();
    }
    return s;
}

RVA(0x000db110, 0x90)
CString GetDifficultyName(i32 diffIdx, i32 upper) {
    CString s;
    s = g_difficultyNames[diffIdx];
    if (upper) {
        s.MakeUpper();
    }
    return s;
}

DATA_SYMBOL(0x0024c3f0, 0x44, _g_soundChannelInUse)

RVA(0x000db1d0, 0x14)
void ChannelSlots_InitAll() {
    for (i32 i = 0; i < 17; i++) {
        g_soundChannelInUse[i] = 1;
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

RVA(0x000db280, 0x1b)
i32 ChannelSlots_FindFree() {
    for (i32 i = 0; i < 17; i++) {
        if (g_soundChannelInUse[i] != 0) {
            return i;
        }
    }
    return 0;
}

RVA(0x000db2b0, 0x10)
void ChannelSlots_Set(i32 i, i32 v) {
    g_soundChannelInUse[i] = v;
}

RVA(0x000db2d0, 0xc)
i32 ChannelSlots_Get(i32 i) {
    return g_soundChannelInUse[i];
}

RVA(0x000db2f0, 0x2b)
i32 GruntzPlayer::Deactivate() {
    if (m_liveGate == 0) {
        return 0;
    }
    if (m_014 == 0) {
        (static_cast<CBattlezMapConfig*>(&m_038))->Clear();
    }
    m_liveGate = 0;
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
    if (static_cast<u32>(m_gridDelayCount) > static_cast<u32>(dt)) {
        m_gridDelayCount = m_gridDelayCount - dt;
        return 1;
    }
    m_gridDelayCount = m_gridDelayBase;
    m_gridRow = m_gridRow + 1;
    i32 idx = m_gridRow;
    CImageSet* g = m_grid;
    CImage* frame;
    if (idx >= g->m_minIndex && idx <= g->m_maxIndex) {
        frame = static_cast<CImage*>(g->m_items.GetAt(idx));
    } else {
        frame = 0;
    }
    m_gridCurFrame = frame;
    if (frame == 0) {
        m_gridCurFrame = static_cast<CImage*>(g->m_items.GetAt(g->m_minIndex));
        m_gridRow = g->m_minIndex;
    }
    return 1;
}

// ===========================================================================
// CPlay::DispatchHudClick (0x0ce530) - route a HUD pointer event at (x,y). If the
// HUD is suppressed (m_hudSuppressed) just succeed. If an overlay is up (m_overlayActive) and the guts
// subsystem isn't busy (m_guts->m_position!=2 && m_guts->m_activeTab!=5), forward to the in-rect
// handler. If a pending HUD rect is armed (m_worldReady), post it (gated by the dev-flag
// 0x645578->m_18 & 0x20) and clear m_worldReady/m_dragSnapActive. Finally, unless the guts subsystem
// is busy (==2), point-test against the viewport box (m_c->m_level+0x10): inside ->
// succeed, outside -> forward to the click-at-point handler.
// ===========================================================================
// @early-stop
// zero-register-pinning wall — structure + offsets byte-identical; retail pins ebx=0
// (xor ebx,ebx + cmp ebx,[member] null tests) and colors y into ebp (5 callee-saves),
// MSVC here uses test/4 saves. No source lever forces it. docs/patterns/zero-register-pinning.md.
RVA(0x000ce530, 0xe3)
i32 CPlay::Vslot0f(i32 a, i32 x, i32 y) {
    if (m_hudSuppressed != 0) {
        return 1;
    }
    if (m_lightFx != 0 && m_guts->m_position != 2 && m_guts->m_activeTab != 5) {
        m_lightFx
            ->ClearHandle(a, x, y); // 0x13f2 -> CLightFxRender::ClearHandle @0xa9500 (ecx=m_320)
    }
    if (m_worldReady != 0) {
        m_mgr->m_cmdGrid->HudRect(m_hudRect, g_spawnConfig->m_18 & 0x20);
    }
    m_worldReady = 0;
    m_dragSnapActive = 0;
    if (m_guts->m_position == 2) {
        return 1;
    }
    LevelCoordRect& vp = m_world->m_level->m_planeCtx;
    if (x >= vp.left && x <= vp.right && y >= vp.top && y <= vp.bottom) {
        return 1;
    }
    m_guts->ClickAt_ff9d0(a, x, y);
    return 1;
}

// @early-stop
// 4-byte stack-coalesce wall (~84%, body byte-exact). Return-epilogue tail-merge
// (paired guards combined with `||`) + a uniform +4 frame shift (sub esp,0x14 vs
// 0x10): retail packs every local into the 16-byte RECT + reuses dead x/y arg homes
// for the Probe/Find out-params; this build spills one out-param to a fresh 5th slot.
// A documented stack-slot-coalesce coin-flip (docs/patterns/stack-slot-coalesce-
// frame-4b.md), not source-steerable; ZERO logic differences remain.
RVA(0x000ce660, 0x362)
i32 CPlay::Vslot10(i32 msg, i32 x, i32 y) {
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
        CSndHost* set = m_mgr->m_world->m_soundRegistry;
        if (set->m_emitGate == 0) {
            LeafCue* e = 0;
            set->m_10.Lookup(
                "GAME_TABHIGHLIGHT1",
                reinterpret_cast<void*&>(e)
            ); // Ptr map out-param idiom
            if (e != 0) {
                e->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
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

    RECT* rc = (&m_world->m_level->m_planeCtx);
    i32 rl = rc->left;
    i32 rt = rc->top;
    i32 rr = rc->right;
    i32 rb = rc->bottom;
    if (x < rl || x > rr || y < rt || y > rb) {
        return m_guts->ClickHilite(msg, x, y);
    }

    i32 outArea;
    i32 outVal;
    if (m_mgr->m_cmdGrid->ScreenToCell(x, y, &outArea, &outVal, 5) && g_curPlayer == outArea) {
        m_guts->ToggleStat(outVal);
        return 1;
    }

    if (m_dragInhibit1 != 0) {
        return 1;
    }
    i32 area = g_curPlayer;
    GruntzPlayer* cfg = &g_gameReg->m_options[area];
    if (cfg == 0) {
        return 0;
    }
    if (g_gameReg->m_cmdGrid->m_rowCount[area] >= cfg->m_comboSel) {
        return 0;
    }

    CGameLevel* h = m_mgr->m_world->m_level;
    i32 px = h->m_mainPlane->m_originX - h->m_planeCtx.left + x;
    i32 py = h->m_mainPlane->m_originY - h->m_planeCtx.top + y;
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
            char ab = static_cast<char>(g_curPlayer);
            px = (px & 0xffe0) + 0x10;
            py = (py & 0xffe0) + 0x10;
            m_mgr->m_cmdSubMgr->Spawn(1, ab, 0, 0, px, py, 0, 0);
            return 1;
        }
    }
    return 1;
}

// ===========================================================================
// CPlay::BeginGridWalk (0x0d0920) - arm the frame-grid walk for a level cue. Look
// up the grid object for `key` in the view's grid map (m_c->m_imageRegistry->m_10); bail if
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
    if (m_world == 0) {
        return 1;
    }
    CImageSet* grid = 0;
    CObject* gridOb = 0;
    // frame-grid probe into the image registry's name->object map (frame-grid Lookup overload).
    m_world->m_imageRegistry->m_10map.Lookup(key, gridOb);
    grid = static_cast<CImageSet*>(gridOb);
    m_grid = grid;
    if (grid == 0) {
        return 1;
    }
    m_gridHasSprite = hasGrid;
    if (hasGrid != 0) {
        CGruntzMgr* w = m_mgr;
        i32 id = g_curPlayer;
        void* spr =
            w->m_spriteFactory->LoadSprite(reinterpret_cast<void*>(w->m_options[id].m_008), 0);
        if (spr == 0) {
            spr = g_gameReg->m_spriteFactory->LoadSprite(spr, 1);
        }
        m_grid->SetAllTypes(0xa);
        m_grid->SetAllFormats(reinterpret_cast<i32>(spr));
    }
    CImageSet* g = m_grid;
    CImage* frame;
    if (index >= g->m_minIndex && index <= g->m_maxIndex) {
        frame = static_cast<CImage*>(g->m_items.GetAt(index));
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
//             (m_c->m_level+0x10). INSIDE -> finish (clear m_dragInProgress, normalize the sel
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
    LevelCoordRect box = m_world->m_level->m_planeCtx;
    left = box.left;
    top = box.top;
    right = box.right;
    bottom = box.bottom;
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
        if (m_hitTest->HitTest(x, y) != 0 || m_mgr->m_frameGate != 0 || m_inGame != 0
            || m_dragInhibit1 != 0 || m_dragInhibit2 != 0) {
            // (a second, distinct re-arm landing pad in retail.)
            CWwdGameObjectA* s2 = m_scrollSink;
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
        CGameLevel* v = m_world->m_level;
        i32 wx = v->m_mainPlane->m_originX - v->m_planeCtx.left + x;
        i32 wy = v->m_mainPlane->m_originY - v->m_planeCtx.top + y;
        m_mgr->m_cmdGrid->PlaceObjectFull(wx, wy); // 0x2ca7 -> @0x78a50
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
    if (m_dragEndNotify != 0 && m_mgr->m_cmdGrid->m_pendingFxKind == 0) {
        FlushPendingOps(); // "EndDragSel" 0x4da2d0 IS this fn's own 0xda2d0
    }
    return 1;

rearm:
    CWwdGameObjectA* s = m_scrollSink;
    if (s == 0) {
        return 1;
    }
    s->m_stateFlags |= 1;
    return 1;
}

RVA(0x000d7220, 0x7b)
i32 CPlay::PostActionCue(i32 cueId) {
    if (m_paused) {
        return 0;
    }
    if (!m_cueText.LoadStringA(cueId)) {
        return 0;
    }
    m_lastCueId = cueId;
    m_stepCountdown = 2;
    m_paused = 1;
    // retail reads the g_gameReg GLOBAL here (not this->m_4 like the other 0x816e
    // posts in this TU - `a1 6c 55 64` in the bytes): same manager, global path.
    PostMessageA(g_gameReg->m_gameWnd->m_hwnd, 0x111, 0x816e, 0);
    if (m_scrollSink) {
        m_scrollSink->m_stateFlags |= 1;
    }
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
// NB retail rets 0x4 (every
// caller pushes 0; LoadByMode's finale pushes 1). The strip blit is LayerBlitFrame
// (thunk 0x18ca -> 0x115300); retail also runs 2x an 0x11f570 leaf this
// reconstruction still lacks (final-sweep item).
RVA(0x000d72c0, 0x128)
i32 CPlay::BuildHelpReveal(i32 final) {
    static_cast<void>(final);
    CImage* view = reinterpret_cast<CImage*>(m_world->m_drawTarget->m_backPair);
    if (view == 0) {
        return 0;
    }
    if (m_revealFrame == 1) {
        LayerBlitFrame(m_world, static_cast<CImage*>(m_revealCapStart), 0x140, 0x1a6, 1, 0);
        LayerBlitFrame(m_world, static_cast<CImage*>(m_revealCapMid), 0xe0, 0x1a6, 1, 0);
    }

    i32 counter = m_revealFrame;
    i32 col = static_cast<i32>((static_cast<float>(counter) * 3.7857143878936768f));
    if (counter < 0x37) {
        i32 i = counter;
        do {
            i32 x = 0xe0 - static_cast<i32>((static_cast<float>(i) * -3.7857143878936768f));
            LayerBlitFrame(m_world, static_cast<CImage*>(m_revealCapMid), x, 0x1a6, 1, 0);
            i++;
        } while (i < 0x37);
    } else {
        LayerBlitFrame(m_world, static_cast<CImage*>(m_revealCapMid), col + 0xe0, 0x1a6, 1, 0);
    }

    LayerBlitFrame(m_world, static_cast<CImage*>(m_revealCapEnd), 0x1b4, 0x1a6, 1, 0);
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
void CPlay::PlayBacklog() {}

RVA(0x0008c930, 0x3)
i32 CPlay::Vslot1a() {
    return 0;
}

RVA(0x0008c950, 0x3)
i32 CPlay::GetFrame() {
    return 0;
}

RVA(0x0008c970, 0x1c)
i32 CPlay::SetBeginClearParams(i32 unused, i32 arg2, i32 arg3) {
    m_cursorX = arg2;
    m_cursorY = arg3;
    return 1;
}

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
i32 CPlay::Vslot0e(i32 a, i32 x, i32 y) {
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
        m_mgr->ReportError(0x800a, 0x457);
        return 1;
    }
    if (m_paused != 0) {
        m_paused = 0;
        ::PostMessageA(m_mgr->m_gameWnd->m_hwnd, 0x111, 0x816e, 0);
        return 1;
    }
    // The overlay-drag / command-grid-idle gates route to the guts dispatch tail
    // (a forward je all the way down); nest the no-active-grunt main body deepest
    // so retail's cold-block-at-tail layout falls out (nested-if-success-deepest).
    if (m_overlayDrag == 0 && g_gameReg->m_cmdGrid->m_groupFlag != 0) {
        if (m_mgr->m_frameGate != 0) {
            goto drag_path;
        }

        // ---- no active grunt: overlay probe, then marker/waypoint place ----
        if (m_lightFx != 0 && m_guts->m_position != 2 && m_guts->m_activeTab != 5) {
            if (m_lightFx->ApplyA(a, xr, y)) { // 0x3e59 -> CLightFxRender::ApplyA @0xa9480
                return 1;
            }
        }
        CGruntzMgr* w = m_mgr;
        CGameLevel* geom = w->m_world->m_level;
        CLevelPlane* cam = geom->m_mainPlane;
        i32 sx = cam->m_originX - geom->m_planeCtx.left + xr;
        i32 sy = cam->m_originY - geom->m_planeCtx.top + y;
        if (m_dragInhibit1 == 0) {
            goto mode_36c;
        }
        if (m_4f0 != 0) {
            goto mode_36c;
        }
        i32 placed = 0;
        RECT* gr = reinterpret_cast<RECT*>(
            &m_guts->m_10
        ); // +0x10 widget rect (grouping conflict w/ SBI m_rect14)
        if (xr < gr->right && xr >= gr->left && y < gr->bottom && y >= gr->top) {
            // inside the guts HUD rect -> finalize with placed == 0
        } else {
            RECT* wr = (&geom->m_planeCtx);
            if (xr < wr->right && xr >= wr->left && y < wr->bottom && y >= wr->top) {
                if (FindStartPointAt(sx, sy, &x, &y)) {
                    char tok = *reinterpret_cast<char*>(&g_curPlayer);
                    w->m_cmdSubMgr->EnqueueSingle(
                        1,
                        tok,
                        0,
                        0,
                        static_cast<i16>(x),
                        static_cast<i16>(y),
                        0,
                        0
                    ); // 0x2095 -> @0x23c30
                    placed = 1;
                }
            }
        }
        if (placed == 0) {
            g_gameReg->m_cueSink->SpawnVoiceDriver(placed, 0x340, -1, 1, -1, -1);
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
        RECT* gr = reinterpret_cast<RECT*>(
            &m_guts->m_10
        ); // +0x10 widget rect (grouping conflict w/ SBI m_rect14)
        if (xr < gr->right && xr >= gr->left && y < gr->bottom && y >= gr->top) {
            if (m_guts->SetFallRect(xr, y, *reinterpret_cast<char*>(&m_cursorFrame))) {
                m_dragInhibit2 = 0;
                SetCursorFrame(0);
                return 1;
            }
            goto waypoint_cancel;
        }
        CGruntzMgr* w = m_mgr;
        CGameLevel* geom = w->m_world->m_level;
        RECT* wr = (&geom->m_planeCtx);
        if (!(xr < wr->right && xr >= wr->left && y < wr->bottom && y >= wr->top)) {
            goto waypoint_cancel;
        }
        // inside the world rect: place a waypoint through the trigger grid
        CGameLevel* ds = m_world->m_level;
        CLevelPlane* cam = ds->m_mainPlane;
        i32 wx = cam->m_originX - ds->m_planeCtx.left + xr;
        i32 wy = cam->m_originY - ds->m_planeCtx.top + y;
        i32 tok = *reinterpret_cast<char*>(&m_cursorFrame);
        if (g_gameReg->m_cmdGrid->CellHitTest(wx, wy, &x, &y, tok) != 0) {
            w->m_cmdSubMgr->EnqueueSingle(
                1,
                static_cast<char>(a),
                static_cast<char>(y),
                8,
                0,
                0,
                static_cast<char>(tok),
                0
            );
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
        CTmCell* p = g_gameReg->m_cmdGrid
                         ->FindGruntAt(wx, wy, reinterpret_cast<RECT*>(out28), &col, &y, &box);
        if (p == 0 || g_curPlayer != p->m_tileOwnerHi) {
            goto waypoint_cancel;
        }
        w->m_cmdSubMgr->EnqueueSingle(
            1,
            static_cast<char>(a),
            static_cast<char>(y),
            8,
            0,
            0,
            static_cast<char>(tok),
            0
        );
        return 1;
    }

waypoint_cancel:
    m_dragInhibit2 = 0;
    m_guts->EnterHlRow(0, *reinterpret_cast<char*>(&m_cursorFrame));
    SetCursorFrame(0);
    return 1;

drag_path: {
    // m_4->m_frameGate != 0: an active grunt is selected -> drag / guts dispatch
    static_cast<void>(y);
    if (m_guts == 0) {
        return 1;
    }
    if (m_guts->m_position == 2) {
        if (m_guts->HitTestLayer(xr, y)) {
            m_dragSnapActive = 1;
            // the guts +0x08 slot holds the dragged widget's display object
            // (screen pos at +0x5c/+0x60 - the CGameObject shape); snap origin =
            // widget pos - click pos.
            CGameObject* g8 = m_guts->m_barSprite;
            i32 dx = 0;
            if (g8 != 0) {
                dx = g8->m_screenX - xr;
            }
            m_snapOriginX = dx;
            CGameObject* g8b = m_guts->m_barSprite;
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
    RECT* gr = reinterpret_cast<RECT*>(
        &m_guts->m_10
    ); // +0x10 widget rect (grouping conflict w/ SBI m_rect14)
    if (xr < gr->right && xr >= gr->left && y < gr->bottom && y >= gr->top) {
        FlushPendingOps();
        return m_guts->UpdateStatusBarTabHighlight(a, xr, y);
    }
    if (m_hitTest->HitTest(xr, y)) { // via ILT 0x43e0
        return 1;
    }
}

drag_box: {
    if (m_mgr->m_frameGate != 0) {
        goto ret1;
    }
    CGruntzMgr* w = m_mgr;
    RECT* wr = (&w->m_world->m_level->m_planeCtx);
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
            i32* sel = *reinterpret_cast<i32**>(
                (*reinterpret_cast<char**>(reinterpret_cast<char*>(&cg->m_recList) + 4) + 8)
            );
            slot = cg->m_grid[sel[1] * 15 + sel[0]];
        }
        if (slot != 0 && slot->m_entranceCommitted != 0) {
            g_gameReg->m_cueSink
                ->SpawnVoiceDriver(reinterpret_cast<i32>(slot), 0x324, -1, 0, -1, -1);
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
    CGrunt* picked =
        static_cast<CGrunt*>(m_mgr->m_cmdGrid->ScreenToCell(xr, y, &slot38, &slot38, 5)); // 0x3cb0
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
    m_mgr->m_cmdGrid
        ->ResetCell(slot38, slot38, g_spawnConfig->m_18 & 0x20, 0); // 0x29cd -> @0x6bfd0
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
i32 CPlay::Vslot11(i32 a, i32 x, i32 y) {
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
        m_mgr->ReportError(0x800a, 0x458);
        return 1;
    }
    if (m_paused != 0) {
        m_paused = 0;
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, 0x111, 0x816e, 0);
        return 1;
    }
    if (m_overlayDrag != 0) {
        return 1;
    }
    if (g_gameReg->m_cmdGrid->m_groupFlag == 0) { // reg->m_68 cue-sink busy gate
        return 1;
    }
    if (m_mgr->m_frameGate != 0) {
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
        CTriggerMgr* w = m_mgr->m_cmdGrid;
        if (w->m_goal != 0) {
            w->m_goal->m_flags |= 0x10000;
            w->m_goal = 0;
        }
        w->m_armed = 0;
        return 1;
    }
    if (m_mgr->m_cmdGrid->m_recList.GetCount() == 0) { // +0x24c == m_recList.m_nCount
        return 1;
    }
    CGameLevel* ph = m_mgr->m_world->m_level;
    if (x < ph->m_planeCtx.right && x >= ph->m_planeCtx.left && y < ph->m_planeCtx.bottom
        && y >= ph->m_planeCtx.top) {
        CGameLevel* ds = m_world->m_level;
        CLevelPlane* geom = ds->m_mainPlane;
        i32 rawX = geom->m_originX - ds->m_planeCtx.left + x;
        i32 rawY = geom->m_originY - ds->m_planeCtx.top + y;
        i32 snapX = (rawX & ~0x1f) + 0x10;
        i32 snapY = (rawY & ~0x1f) + 0x10;
        m_tileClickX = snapX;
        m_tileClickY = snapY;
        CTriggerMgr* w = m_mgr->m_cmdGrid;
        if (w->m_overlay != 0 && w->m_overlay->m_active != 0) {
            w->OverlayTick(); // 0x1514 -> @0x78a30
            return 1;
        }
        w->ResetGroup(snapX, snapY, rawX, rawY, 1, 0, 1); // 0x3044 -> @0x79520
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

RVA(0x000cedf0, 0xf)
i32 CGameLevel::MainPlaneQueryA() {
    if (m_mainPlane != 0) {
        return m_mainPlane->CenterScrollA(); // 0x163300
    }
    return 0;
}

RVA(0x000cee10, 0xf)
i32 CGameLevel::MainPlaneQueryB() {
    if (m_mainPlane != 0) {
        return m_mainPlane->CenterScrollB(); // 0x163370
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
// the first m_c->m_level->m_5c chain load (cl spreads to ecx where retail reuses
// eax) and (b) the reloc-masked callee symbols (CameraGeom DrawA/DrawB, PushView,
// ManagerTick) which pair once those engine fns are named.
RVA(0x000cefc0, 0xa2)
i32 CPlay::DrawWorldPresent() {
    if (m_world->m_level->m_mainPlane != 0) {
        m_world->m_level->m_mainPlane->CenterScrollB();
    }
    if (m_world->m_level->m_mainPlane != 0) {
        m_world->m_level->m_mainPlane->CenterScrollA();
    }
    m_world->m_childGroup->TickKillCues(1);
    if (m_world->m_level->m_mainPlane != 0) {
        m_world->m_level->m_mainPlane->CenterScrollB();
    }
    if (m_world->m_level->m_mainPlane != 0) {
        m_world->m_level->m_mainPlane->CenterScrollA();
    }
    m_world->m_childGroup->TickKillCues(1);
    m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
    m_world->m_workerList->PruneWorkers(
        m_world->m_drawTarget->m_backPair,
        m_world->m_drawTarget->m_overlayPair
    );
    m_mgr->RefreshGameClock(); // 0x8f620 direct (thunk 0x3d23)
    return 1;
}

// ===========================================================================
// PresentAndFlush (0x0cba10) - the overlay-frame present path: bail unless the
// state is active (IsActive), restore the saved display mode if it drifted, then
// either notify-visible (region-1 gate) or push+present, and flush the draw
// surface. Migrated from engine_boundary (CPlay).
// @early-stop
// reloc-masked plateau (~97%): code bytes exact; residual is the call-rel32
// operands to the unmatched engine callees (m_guts ClampApply 0x500cb0, the
// PushView 0x15dc90, surface flush 0x13e850, m_4 RestoreVideoMode 0x8df00).
RVA(0x000cba10, 0xb0)
i32 CPlay::Vslot06() {
    if (IsActive() == 0) {
        return 0;
    }
    CGruntzMgr* w = m_mgr;
    i32 savedW = w->m_savedModeW;
    i32 liveW = w->m_modeW;
    i32 savedH = w->m_savedModeH;
    i32 liveH = w->m_modeH;
    if (savedW != liveW || savedH != liveH) {
        if ((static_cast<CGruntzMgr*>(w))->SetVideoMode(savedW, savedH, 1) == 0) {
            return 0;
        }
    }
    if (m_guts != 0) {
        m_guts->Deactivate();
        if (m_region1Gate != 0) {
            NotifyVisibleEntities();
        } else {
            m_world->m_level->VisitVisible(
                m_world->m_drawTarget->m_backPair,
                m_world->m_childGroup
            );
            m_world->m_workerList->PruneWorkers(
                m_world->m_drawTarget->m_backPair,
                m_world->m_drawTarget->m_overlayPair
            );
        }
        m_world->m_drawTarget->m_frontPair->m_surface->Flip(0); // 0x13e850
    }
    return 1;
}

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

RVA(0x000d6560, 0x45)
i32 CPlay::ReleaseLevelOverlay(i32) {
    if (m_overlayDrag != 0) {
        CStatusBarMgr* worker = m_guts;
        m_overlayDrag = 0;
        worker->ExitMode();
        if (g_gameReg->m_134 != 2) {
            g_frameTime = m_savedClock;
        }
    }
    return 1;
}

RVA(0x000cee70, 0x5)
i32 CPlay::ForwardReady() {
    return IsActive();
}

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

RVA(0x000cef50, 0x46)
i32 CPlay::QuitToMenu() {
    // The +0xc8 call both prior passes guessed as a list dtor (~CPtrList / ~CObList)
    // is NEITHER: retail calls 0x1b9c69 == ?Empty@CString@@QAEXXZ (NAFXCW, anchored).
    // It clears the manager's world-file name - CGruntzMgr::m_strWorldFile @+0xc8.
    m_mgr->m_strWorldFile.Empty();
    if (m_1c0 != 0) {
        if (m_world->m_drawTarget->HasOverlay() != 0) {
            m_world->m_drawTarget->TransEnter();
        }
        m_mgr->ChangeState(3);
    }
    return 1;
}

RVA(0x000cfbb0, 0x8)
void CPlay::TickStateMgrs() {
    m_mgr->TickStateMgrs();
}

// CPlay::Vslot15 (0x0cfbd0) - vtable slot 21 (override of CState), the level-quiesce
// dispatch. On level index 0x20: latch the quiesce flags (m_1c0/m_40), stop the current
// zoned sound stream (m_c->m_soundRegistry->m_2c, SoundStream::Stop) + flush the sound bank
// (m_4->m_48, CGruntzSoundZ::StopAndFlush), reset the two world teardown sub-objects
// (m_4->m_54/m_60), then PostMessageA WM_COMMAND 0x8023. Otherwise re-post 0x8023 while
// the m_1bc gate is set, else advance via the manager (m_4->Post, level index + 1). The
// PostMessageA calls go through the cached ::PostMessageA fn-ptr (bare 0x6c44c8, no
// import symbol).
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
        m_notifyLatch = 1;
        SoundStream* stream = m_world->m_soundRegistry->m_2c;
        if (stream) {
            stream->Stop();
        }
        m_mgr->m_sound->StopAndFlush();
        m_mgr->m_inputState->Teardown();  // 0x28ab -> CWorldSoundSet::Teardown @0xb660
        m_mgr->m_cueSink->ClearSprites(); // 0x244b -> CGruntSpawnConfig::ClearSprites @0x11af90
        ::PostMessageA(m_mgr->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
        return 1;
    }
    if (m_1bc) {
        ::PostMessageA(m_mgr->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
        return 1;
    }
    m_mgr->Post(m_levelIndex + 1);
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
        g_gameReg->m_cueSink->SpawnVoiceDriver(0, 0x33e, -1, 1, -1, -1);
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
    CGruntzMgr* w = m_mgr;
    i32 changed = 0;
    i32 speed = static_cast<i32>(
        (static_cast<double>(w->m_scrollSpeed) * g_scrollSpeedScale * g_scrollSpeedRange
         + g_scrollMinSpeed)
    );
    CLevelPlane* g = w->m_world->m_level->m_mainPlane;
    i32 sx = g->m_originX;
    i32 sy = g->m_originY;
    i32 extentX = w->m_modeW;
    i32 extentY = w->m_modeH;

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
// namespace-loader tail (BindWarlordName == the 0x2bc1 BuildAssetNamespacePrefixes thunk). The
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
            if (BuildAssetNamespacePrefixes(name, a2, a3, a4) == 0) {
                return 0;
            }
            name = "TOOBWATERGRUNT";
            return BuildAssetNamespacePrefixes(name, a2, a3, a4);
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
    return BuildAssetNamespacePrefixes(name, a2, a3, a4);
}

#include <Gruntz/BankMgr.h>

RVA(0x000cffe0, 0x3c)
i32 CPlay::LoadImageBanks() {
    CPlay* self = this;
    if (!self->m_symParser) {
        return 0;
    }
    self->m_gruntzBank = static_cast<CSymTab*>(self->m_symParser->ResolvePath("GRUNTZ"));
    if (!self->m_gruntzBank) {
        return 0;
    }
    self->m_gameBank = static_cast<CSymTab*>(self->m_symParser->ResolvePath("GAME"));
    return self->m_gameBank != 0;
}

RVA(0x000db600, 0x8f)
i32 CPlay::LoadActionTileSprites(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if (!force
        && (static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
               ->HasKeyEqual("ACTION")) {
        return 1;
    }

    (static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
        ->RemoveKeysEqual("ACTION", g_emptyString);
    (static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
        ->RemoveKeysEqual("BACK", g_emptyString);
    g_resourceInstallActive = 0;

    void* tiles = (self->m_levelBank)->ResolvePath("TILEZ");
    if (!tiles) {
        return 0;
    }
    self->m_world->m_imageRegistry->InstallTree(tiles, g_emptyString, "_");
    return 1;
}

RVA(0x000db6c0, 0x70)
i32 CPlay::LoadLevelSounds(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if (!force
        && (static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
               ->HasKeyEqual("LEVEL")) {
        return 1;
    }

    (static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
        ->RemoveKeysEqual("LEVEL", "_");

    void* sounds = (self->m_levelBank)->ResolvePath("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    (static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
        ->ScanTree(static_cast<CSymTab*>(sounds), "LEVEL", "_");
    return 1;
}

RVA(0x000db750, 0x70)
i32 CPlay::LoadLevelAnims(i32 force) {
    if (m_world == 0) {
        return 0;
    }
    if (force == 0) {
        if (m_world->m_animRegistry->HasKeyPrefix("LEVEL") != 0) {
            return 1;
        }
    }
    m_world->m_animRegistry->RemoveKeysEqual("LEVEL", "_");
    void* e = m_levelBank->ResolvePath("ANIZ");
    if (e == 0) {
        return 0;
    }
    m_world->m_animRegistry->ScanTree(static_cast<CSymTab*>(e), "LEVEL", "_");
    return 1;
}

RVA(0x000db7e0, 0x84)
i32 CPlay::LoadLevelImages(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if (!force
        && (static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
               ->HasKeyEqual("LEVEL")) {
        return 1;
    }

    (static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
        ->RemoveKeysEqual("LEVEL", "_");
    g_resourceInstallActive = 0;

    void* images = (self->m_levelBank)->ResolvePath("IMAGEZ");
    if (!images) {
        return 0;
    }
    self->m_world->m_imageRegistry->InstallTree(images, "LEVEL", "_");
    g_resourceInstallActive = 0;
    return 1;
}

RVA(0x000db8a0, 0x67)
i32 CPlay::LoadGameImages(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if ((static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
            ->HasKeyEqual("GAME")) {
        return 1;
    }

    g_resourceInstallActive = 1;
    void* images = (self->m_gameBank)->ResolvePath("IMAGEZ");
    if (!images) {
        return 0;
    }
    self->m_world->m_imageRegistry->InstallTree(images, "GAME", "_");
    g_resourceInstallActive = 0;
    return 1;
}

RVA(0x000db930, 0x53)
i32 CPlay::LoadGameSounds(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if ((static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
            ->HasKeyEqual("GAME")) {
        return 1;
    }

    void* sounds = (self->m_gameBank)->ResolvePath("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    (static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
        ->ScanTree(static_cast<CSymTab*>(sounds), "GAME", "_");
    return 1;
}

RVA(0x000db9b0, 0x53)
i32 CPlay::LoadGameAnims(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if (self->m_world->m_animRegistry->HasKeyPrefix("GAME")) {
        return 1;
    }

    void* anims = (self->m_gameBank)->ResolvePath("ANIZ");
    if (!anims) {
        return 0;
    }
    self->m_world->m_animRegistry->ScanTree(static_cast<CSymTab*>(anims), "GAME", "_");
    return 1;
}

typedef enum MusicFormatTag {
    MUSIC_TAG_XMI = 0x584d49, // 'XMI'
} MusicFormatTag;

RVA(0x000dba30, 0x1ca)
i32 CPlay::BuildMusicCategoryTable(i32) {
    m_mgr->m_sound->StopAndFlush();

    CSymTab* levelSet = static_cast<CSymTab*>(m_levelBank->ResolvePath("MIDIZ"));
    if (levelSet) {
        CParseSource* e = levelSet->Insert("AMBIENT0", reinterpret_cast<void*>(MUSIC_TAG_XMI));
        if (e) {
            void* res = reinterpret_cast<void*>(e->BeginParse());
            if (res) {
                m_mgr->m_sound->CreateBank(res, e->m_length, "AMBIENT0");
            }
        }
        e = levelSet->Insert("AMBIENT1", reinterpret_cast<void*>(MUSIC_TAG_XMI));
        if (e) {
            void* res = reinterpret_cast<void*>(e->BeginParse());
            if (res) {
                m_mgr->m_sound->CreateBank(res, e->m_length, "AMBIENT1");
            }
        }
        e = levelSet->Insert("INTRO0", reinterpret_cast<void*>(MUSIC_TAG_XMI));
        if (e) {
            void* res = reinterpret_cast<void*>(e->BeginParse());
            if (res) {
                m_mgr->m_sound->CreateBank(res, e->m_length, "INTRO0");
            }
        }
        e = levelSet->Insert("INTRO1", reinterpret_cast<void*>(MUSIC_TAG_XMI));
        if (e) {
            void* res = reinterpret_cast<void*>(e->BeginParse());
            if (res) {
                m_mgr->m_sound->CreateBank(res, e->m_length, "INTRO1");
            }
        }
    }

    CSymTab* gameSet = static_cast<CSymTab*>(m_gameBank->ResolvePath("MIDIZ"));
    if (gameSet) {
        CParseSource* e = gameSet->Insert("POWERUP", reinterpret_cast<void*>(MUSIC_TAG_XMI));
        if (e) {
            void* res = reinterpret_cast<void*>(e->BeginParse());
            if (res) {
                m_mgr->m_sound->CreateBank(res, e->m_length, "POWERUP");
            }
        }
        e = gameSet->Insert("CURSE", reinterpret_cast<void*>(MUSIC_TAG_XMI));
        if (e) {
            void* res = reinterpret_cast<void*>(e->BeginParse());
            if (res) {
                m_mgr->m_sound->CreateBank(res, e->m_length, "CURSE");
            }
        }
        e = gameSet->Insert("MONOLITH", reinterpret_cast<void*>(MUSIC_TAG_XMI));
        if (e) {
            void* res = reinterpret_cast<void*>(e->BeginParse());
            if (res) {
                m_mgr->m_sound->CreateBank(res, e->m_length, "MONOLITH");
            }
        }
    }
    return 1;
}

RVA(0x000dd830, 0x1e3)
i32 CPlay::LoadGruntSoundNamespaces(CMulti* notify) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }

    if (!(static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
             ->HasKeyEqual("GRUNTZ_NORMALGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_NORMALGRUNT");
        if (s) {
            (static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
                ->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_NORMALGRUNT", "_");
        }
    }
    if (!(static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
             ->HasKeyEqual("GRUNTZ_DEATHZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_DEATHZ");
        if (s) {
            (static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
                ->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_DEATHZ", "_");
        }
    }
    if (!(static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
             ->HasKeyEqual("GRUNTZ_ENTRANCEZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_ENTRANCEZ");
        if (s) {
            (static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
                ->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_ENTRANCEZ", "_");
        }
    }
    if (!(static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
             ->HasKeyEqual("GRUNTZ_EXITZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_EXITZ");
        if (s) {
            (static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
                ->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_EXITZ", "_");
        }
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
             ->HasKeyEqual("GRUNTZ_GRUNTPUDDLE")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_GRUNTPUDDLE");
        if (s) {
            (static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
                ->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_GRUNTPUDDLE", "_");
        }
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
             ->HasKeyEqual("GRUNTZ_PICKUPS")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_PICKUPS");
        if (s) {
            (static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
                ->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_PICKUPS", "_");
        }
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
             ->HasKeyEqual("GRUNTZ_BOMBGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_BOMBGRUNT");
        if (s) {
            (static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))
                ->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_BOMBGRUNT", "_");
        }
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    return 1;
}

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
i32 CState::BuildAssetNamespacePrefixes(
    const CString& name,
    i32 mode,
    i32 lightGate,
    i32 finishGate
) {
    i32 result;
    if (mode != 0) {
        if (m_world->m_imageRegistry->HasKeyEqual("GRUNTZ_" + name) == 0) {
            g_gameReg->m_cueSink->DtorBody();
            (static_cast<CTriggerMgr*>(g_gameReg->m_cmdGrid))->DestroyAllAnims();
            if (lightGate != 0) {
                CString cs;
                cs.LoadString(0x819b);
                RECT r = *(&g_gameReg->m_world->m_level->m_planeCtx);
                RECT r2;
                CopyRect(&r2, &r);
                EngStr_DrawText(
                    g_gameReg->m_world,
                    reinterpret_cast<i32>(&cs),
                    reinterpret_cast<i32>(&r2),
                    0x82,
                    1,
                    0xff,
                    0xff,
                    0,
                    1
                );
            }
            g_resourceInstallActive = 1;
            void* tree = m_gruntzBank->ResolvePath("IMAGEZ_" + name);
            if (tree == 0) {
                result = 0;
                goto done;
            }
            m_world->m_imageRegistry->InstallTree(tree, "GRUNTZ_" + name, "_"); // slot 18 (+0x48)
            g_resourceInstallActive = 0;
            if (finishGate != 0) {
                (reinterpret_cast<CMulti*>(finishGate))->AckJoinFailure(); // 0x35e4, ecx=notify
            }
        }
        if (m_world->m_soundRegistry->HasKeyEqual("GRUNTZ_" + name) == 0) {
            void* tree = m_gruntzBank->ResolvePath("SOUNDZ_" + name);
            if (tree != 0) {
                // the m_28 cast stays until the CSndHost/CDDrawSubMgrLeafScan conflation is settled (Fable);
                // `tree` is the real CSymTab - DirNode was a view of it.
                m_world->m_soundRegistry
                    ->ScanTree(static_cast<CSymTab*>(tree), "GRUNTZ_" + name, "_");
            }
        }
        if (m_world->m_animRegistry->HasKeyPrefix("GRUNTZ_" + name) == 0) {
            void* tree = m_gruntzBank->ResolvePath("ANIZ_" + name);
            if (tree == 0) {
                result = 0;
                goto done;
            }
            m_world->m_animRegistry
                ->ScanTree(static_cast<CSymTab*>(tree), "GRUNTZ_" + name, "_");
        }
        result = 1;
        goto done;
    }

    if (m_world->m_imageRegistry->HasKeyEqual("GRUNTZ_" + name) != 0) {
        m_world->m_imageRegistry->RemoveKeysEqual("GRUNTZ_" + name, "_");
        if (finishGate != 0) {
            (reinterpret_cast<CMulti*>(finishGate))->AckJoinFailure(); // 0x35e4, ecx=notify
        }
    }
    if (m_world->m_soundRegistry->HasKeyEqual("GRUNTZ_" + name) != 0) {
        m_world->m_soundRegistry->RemoveKeysEqual("GRUNTZ_" + name, "_");
    }
    if (m_world->m_animRegistry->HasKeyPrefix("GRUNTZ_" + name) != 0) {
        m_world->m_animRegistry->RemoveKeysEqual("GRUNTZ_" + name, "_");
    }
    result = 1;
done:
    return result;
}

RVA(0x000dd540, 0x241)
i32 CPlay::BuildSpriteImageKeyTable(CMulti* notify) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    g_resourceInstallActive = 1;
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
             ->HasKeyEqual("GRUNTZ_NORMALGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_NORMALGRUNT");
        if (!s) {
            return 0;
        }
        self->m_world->m_imageRegistry->InstallTree(s, "GRUNTZ_NORMALGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
             ->HasKeyEqual("GRUNTZ_DEATHZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_DEATHZ");
        if (!s) {
            return 0;
        }
        self->m_world->m_imageRegistry->InstallTree(s, "GRUNTZ_DEATHZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
             ->HasKeyEqual("GRUNTZ_ENTRANCEZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_ENTRANCEZ");
        if (!s) {
            return 0;
        }
        self->m_world->m_imageRegistry->InstallTree(s, "GRUNTZ_ENTRANCEZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
             ->HasKeyEqual("GRUNTZ_EXITZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_EXITZ");
        if (!s) {
            return 0;
        }
        self->m_world->m_imageRegistry->InstallTree(s, "GRUNTZ_EXITZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
             ->HasKeyEqual("GRUNTZ_GRUNTPUDDLE")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_GRUNTPUDDLE");
        if (!s) {
            return 0;
        }
        self->m_world->m_imageRegistry->InstallTree(s, "GRUNTZ_GRUNTPUDDLE", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
             ->HasKeyEqual("GRUNTZ_PICKUPS")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_PICKUPS");
        if (!s) {
            return 0;
        }
        self->m_world->m_imageRegistry->InstallTree(s, "GRUNTZ_PICKUPS", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
             ->HasKeyEqual("GRUNTZ_BOMBGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_BOMBGRUNT");
        if (!s) {
            return 0;
        }
        self->m_world->m_imageRegistry->InstallTree(s, "GRUNTZ_BOMBGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    g_resourceInstallActive = 0;
    return 1;
}

RVA(0x000ddaa0, 0x228)
i32 CPlay::BuildAnizKeyTable(CMulti* notify) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if (!self->m_world->m_animRegistry->HasKeyPrefix("GRUNTZ_NORMALGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_NORMALGRUNT");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry
            ->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_NORMALGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!self->m_world->m_animRegistry->HasKeyPrefix("GRUNTZ_DEATHZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_DEATHZ");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry
            ->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_DEATHZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!self->m_world->m_animRegistry->HasKeyPrefix("GRUNTZ_ENTRANCEZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_ENTRANCEZ");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry
            ->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_ENTRANCEZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!self->m_world->m_animRegistry->HasKeyPrefix("GRUNTZ_EXITZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_EXITZ");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry
            ->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_EXITZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!self->m_world->m_animRegistry->HasKeyPrefix("GRUNTZ_GRUNTPUDDLE")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_GRUNTPUDDLE");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry
            ->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_GRUNTPUDDLE", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!self->m_world->m_animRegistry->HasKeyPrefix("GRUNTZ_PICKUPS")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_PICKUPS");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry
            ->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_PICKUPS", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!self->m_world->m_animRegistry->HasKeyPrefix("GRUNTZ_BOMBGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_BOMBGRUNT");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry
            ->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_BOMBGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    return 1;
}

RVA(0x000c8a10, 0x119)
i32 CPlay::Vslot09(i32 mode) {
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
    } else if (m_renderDisabled == 0 || m_mgr->m_134 == 2) {
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
            (static_cast<CWorldSoundSet*>(m_mgr->m_inputState))->Resume();
        }
        (static_cast<CTriggerMgr*>(m_mgr->m_cmdGrid))
            ->DestroyAllAnims(); // reg m_68 CTriggerMgr @0x7d330
        (static_cast<CGruntSpawnConfig*>(m_mgr->m_cueSink))->DtorBody();
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
    CGruntzMgr* reg = g_gameReg;
    GruntzPlayer* slot = &reg->m_options[id];
    if (slot == 0) {
        return 0;
    }
    if (reg->m_cmdGrid->m_rowCount[id] >= slot->m_comboSel) {
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

RVA(0x000d60b0, 0x2cd)
i32 CPlay::ResetPlayState() {
    char buf[0x40];
    if (m_mgr->m_musicEnabled != 0 && g_gameReg->m_134 == 1) {
        m_ambientInterval = AMBIENT_INTRO_INTERVAL_MS;
        m_ambientIntervalHi = 0;
        m_ambientTimerLo = g_frameTime;
        m_ambientTimerHi = 0;
        wsprintfA(buf, "INTRO%d", GetAmbientId());
        if (g_gameReg->m_musicEnabled
            != 0) { // +0x14 (the WAP32 base field; the registry view called it m_14)
            m_mgr->m_sound->PlayByName(buf, 0);
        }
        m_ambientInitDone = 0;
    } else {
        wsprintfA(buf, "AMBIENT%d", GetAmbientId());
        CGruntzSoundZ* snd = m_mgr->m_sound;
        CGruntzSoundInnerZ* h = snd->FindBank(buf);
        if (h != 0) {
            snd->m_pCurrent = h;
        }
        if (m_mgr->m_sound->m_pCurrent != 0) {
            m_mgr->m_sound->m_pCurrent->SetLoop(1);
        }
        CGruntzMgr* reg = g_gameReg;
        if (reg->m_musicEnabled != 0 && reg->m_134 == 3) {
            m_mgr->m_sound->PlayByName(buf, 1);
        }
        m_ambientTimerLo = 0;
        m_ambientInterval = 0;
        m_ambientTimerHi = 0;
        m_ambientIntervalHi = 0;
        m_ambientInitDone = 1;
    }
    if (m_mgr->m_134 == 1) {
        CGruntzMgr* reg = g_gameReg;
        // the world-file CString's length (retail inlines GetLength = the [-8]
        // CStringData nDataLength read; +0xc8 IS m_strWorldFile).
        if (reg->m_strWorldFile.GetLength() == 0) {
            m_mgr->m_scoreHud->FillRecord(m_levelIndex, 1);
            reg = g_gameReg;
            // the cheat gate: m_cheatMgr's documented m_124 "a cheat was used" latch.
            if (reg->m_cheatMgr->m_124 == 0) {
                i32 id = m_levelIndex;
                if (id > 0x24 || id == 1) {
                    (static_cast<CSaveGame*>(reg->m_saveSink))->SetMaxLevel(id);
                    reg = g_gameReg;
                }
            }
            (static_cast<CSaveGame*>(reg->m_saveSink))->Save(0, 0x81a6);
        }
        CGameLevel* g = m_mgr->m_world->m_level;
        ResetGoalGeom(g->m_header.startX, g->m_header.startY);
    } else {
        GruntzPlayer* slot = &g_gameReg->m_options[g_curPlayer]; // roster-view keep (disp wall)
        if (slot != 0) {
            ResetGoalGeom(slot->m_focusX, slot->m_focusY);
        } else {
            CGameLevel* g = m_mgr->m_world->m_level;
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
    for (i32 i = 0; i < 4; i++) {
        g_gameReg->m_options[i].m_038.StepAllRowSpawns();
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
    CTriggerMgr* tl = m_mgr->m_cmdGrid;
    tl->m_countdownActive = 1;
    tl->m_phase = 0;
    tl->m_pendingFxKind = 0;
    // zero the goo/resource i64 timer pairs half-by-half, preserving retail's
    // lo(base), lo(window), hi(base), hi(window) store order per pair
    (reinterpret_cast<i32*>(&tl->m_gooTimerBase))[0] = 0;
    (reinterpret_cast<i32*>(&tl->m_gooInterval))[0] = 0;
    (reinterpret_cast<i32*>(&tl->m_gooTimerBase))[1] = 0;
    (reinterpret_cast<i32*>(&tl->m_gooInterval))[1] = 0;
    (reinterpret_cast<i32*>(&tl->m_resourceTimerBase))[0] = 0;
    (reinterpret_cast<i32*>(&tl->m_resourceInterval))[0] = 0;
    (reinterpret_cast<i32*>(&tl->m_resourceTimerBase))[1] = 0;
    (reinterpret_cast<i32*>(&tl->m_resourceInterval))[1] = 0;
    tl->m_3ec = 0;
    tl->m_rollingballWanted = 0;
    tl->m_teleportWanted = 0;
    tl->m_groupFlag = 1;
    return 1;
}

#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540

DATA(0x00245270)
i32 g_areaPageSize; // owner def (zero-init .bss)

// (The 9 CRt* views are GONE - CRtThis==CPlay, CRtWorld==CWorld, CRtResMgr==
// CDDrawSurfaceMgr (its m_8 "CDDrawChildGroup" facet is the sprite-factory/renderer
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
    if (m_world == 0) {
        return;
    }
    if (m_mgr == 0) {
        return;
    }
    if (m_mgr->m_cmdGrid != 0) {
        m_mgr->m_cmdGrid->ClearGridRange(5);
    }
    Teardown1780();
    if (m_world->m_soundRegistry->m_2c != 0) {
        m_world->m_soundRegistry->m_2c->Stop();
    }
    m_mgr->m_sound->StopAndFlush();
    m_mgr->m_inputState->Teardown();
    m_mgr->m_cueSink->ClearSprites();
    g_gameReg->m_cmdGrid->DestroyAllAnims();
    m_world->m_level->ReleaseChildren(); // slot 17 (+0x44) - the real CGameLevel virtual
    (m_world->m_childGroup)->PruneList();
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
    m_mgr->m_cmdGrid->OverlayTick();
    CTriggerMgr* tl68 = m_mgr->m_cmdGrid;
    (reinterpret_cast<CPtrArray*>(&tl68->m_byteArr))
        ->SetSize(0, -1); // retail-proven CPtrArray::SetSize @0x1b52e8
    tl68->m_284 = 0;
    m_mgr->m_cmdGrid->m_baseList
        .RemoveAll(); // ?RemoveAll@CPtrList@@ @0x1b48a6 (+0 member; ex Reset1b48a6)
    m_mgr->m_cmdGrid->m_pendingFx = 0;
    (static_cast<CDDrawWorkerList*>(m_world->m_workerList))->ClearWorkers();
    for (i = 0; i < markerCount(); i++) {
        void* node = markerData()[i];
        if (node != 0) {
            CoordPoolNode* p = g_coordPool.NodeOf(node);
            p->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = p;
        }
    }
    m_startMarkers.SetSize(0, -1); // CPtrArray::SetSize @0x1b52e8 (retail band)
    for (k = 0; k < 4; k++) {
        for (i = 0; i < arrCount(k); i++) {
            void* node = arrData(k)[i];
            if (node != 0) {
                CoordPoolNode* p = g_coordPool.NodeOf(node);
                p->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = p;
            }
        }
        m_3a4[k].SetSize(0, -1); // CPtrArray::SetSize @0x1b52e8
    }
    for (i = 0; i < arr488Count(); i++) {
        void* node = arr488Data()[i];
        if (node != 0) {
            CoordPoolNode* p = g_coordPool.NodeOf(node);
            p->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = p;
        }
    }
    m_488.SetSize(0, -1); // CPtrArray::SetSize @0x1b52e8
    for (i = 0; i < 4; i++) {
        m_mgr->m_options[i].m_038.FreeArrays();
        m_mgr->m_options[i].m_038.Clear();
    }
    m_49c = -1;
}

// ---------------------------------------------------------------------------
// CPlay::ReleaseResources (0x0c8700; the slot-2 override, ex "CPlayDtorBody" -
// retail ??_7CPlay slot 2 = ILT 0x1dc5 -> 0xc8700): the ~CPlay teardown body.
// Frees the per-frame workers, clears the four g_gameReg config rows, flushes
// the free-list arrays, then chains CState::ReleaseResources. Same free-list
// idiom as FreeListTeardown (the m_markerData/m_3a4[4]/m_48c flush).
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
void CPlay::ReleaseResources() {
    i32 i;
    if (m_lightFx) {
        m_lightFx->Ctor();
        ::operator delete(m_lightFx);
        m_lightFx = 0;
    }
    OnExit(); // slot 32 (+0x80) - the real CPlay virtual
    if (m_mgr) {
        m_mgr->m_128 = 0;
        m_mgr->m_strWorldFile.Empty(); // 0x1b9c69 CString::Empty (world-file name clear)
    }
    m_1d0 = 0;
    i32 off = 0;
    do {
        off += 0x238;
        *reinterpret_cast<i32*>((reinterpret_cast<char*>(g_gameReg) + off - 0xc8)) = 0;
    } while (off < 0x8e0);
    if (m_mgr && m_mgr->m_chatLog) {
        m_mgr->m_chatLog->FreeNodes();
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
        delete m_beginMarker; // ~CTileTriggerContainer non-virtual (0xc8640) + ??3
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
            CoordPoolNode* p = g_coordPool.NodeOf(node);
            p->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = p;
        }
    }
    m_startMarkers.SetSize(0, -1); // CPtrArray::SetSize @0x1b52e8 (retail band)
    for (i32 k = 0; k < 4; k++) {
        for (i = 0; i < arrCount(k); i++) {
            void* node = arrData(k)[i];
            if (node != 0) {
                CoordPoolNode* p = g_coordPool.NodeOf(node);
                p->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = p;
            }
        }
        m_3a4[k].SetSize(0, -1); // CPtrArray::SetSize @0x1b52e8
    }
    for (i = 0; i < arr488Count(); i++) {
        void* node = arr488Data()[i];
        if (node != 0) {
            CoordPoolNode* p = g_coordPool.NodeOf(node);
            p->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = p;
        }
    }
    m_49c = -1;
    m_488.SetSize(0, -1);       // CPtrArray::SetSize @0x1b52e8
    CState::ReleaseResources(); // 0xfa150 (chain the base slot-2 teardown; direct)
}

// ---------------------------------------------------------------------------
// EnterMode (0x0d6fa0): the mode-enter gate. Pause the guts + world, optionally
// run the deferred draw, (re)install the renderer view, then re-arm the guts and
// (mode 9) latch the saved game clock. A dense chain of CPlay/registry thunks.
// ---------------------------------------------------------------------------
// (The 8 Em* views are GONE - EmThis==CPlay (m_1c4/m_savedClock/m_inputWarmup*/
// m_guts, m_470/m_474==the region gates, m_484==m_hudSuppressed), EmResMgr==
// CDDrawSurfaceMgr, EmCWorld==CDDrawSubMgrPages (its m_14/m_18 surface pages - the
// "CDDrawWorkerMgr" facet is the CDDrawSubMgrPages cast on m_drawTarget), EmHdr14
// ==CDDrawSubMgrPages::SurfaceB, EmRendC (a FABRICATED 13-filler vtable)==CRenderer
// (Present IS its real slot-13 virtual), EmWorld==CWorld (m_10/m_54), EmGuts==
// CStatusBarMgr, "g_645588_clk"==g_frameTime (a duplicate extern of the game clock),
// "EmRegWorldStep"==UpdateMgrScroll @0xebd70.)
// @early-stop
// large state-machine wall: the mode dispatch + renderer reinstall + guts re-arm
// are faithful, but the whole ILT-thunk referent set (~15 unnamed CPlay/registry
// leaves) keeps it reloc-fuzzy; codegen plateau, not source-steerable.
RVA(0x000d6fa0, 0x1fa)
i32 CPlay::EnterMode(i32 mode) {
    (g_gameReg)->CheckSavedMode();
    m_guts->Deactivate();
    m_guts->LoadDestructButtonSprite(0);
    m_mgr->RefreshGameClock(); // 0x8f620 direct (thunk 0x3d23)

    if (m_1c4 != 0) {
        m_1c4 = 0;
        m_world->m_drawTarget->m_backPair->m_surface->Fill(0);
        UpdateMgrScroll(g_gameReg, m_guts, m_region0Gate); // 0x2356
        if (m_region1Gate != 0) {
            NotifyVisibleEntities();
        } else {
            m_world->m_level->VisitVisible(
                m_world->m_drawTarget->m_backPair,
                m_world->m_childGroup
            );
            m_world->m_workerList->PruneWorkers(
                m_world->m_drawTarget->m_backPair,
                m_world->m_drawTarget->m_overlayPair
            );
        }
        m_guts->Deactivate();
        m_guts->LoadMainStatusBarSprite();
    } else {
        if (m_region1Gate != 0) {
            NotifyVisibleEntities();
        } else {
            m_world->m_level->VisitVisible(
                m_world->m_drawTarget->m_backPair,
                m_world->m_childGroup
            );
            m_world->m_workerList->PruneWorkers(
                m_world->m_drawTarget->m_backPair,
                m_world->m_drawTarget->m_overlayPair
            );
        }
        m_guts->Deactivate();
        m_guts->LoadMainStatusBarSprite();
        if (mode == 9) {
            if (m_world->m_drawTarget->HasOverlay() != 0) {
                goto finish;
            }
            if (m_world->m_drawTarget->CreateOverlay(0, 0x30000) != 0) {
                goto finish;
            }
            return 0;
        }
        m_world->m_drawTarget->m_backPair->m_surface->Fill(0);
    }

finish:
    m_world->m_drawTarget->TransTitle();
    RetireScene(
        0x50,
        0x3e8,
        0,
        1
    ); // 0xfa8f0 CState::RetireScene (inherited by CPlay `this`, cast-free)
    if (m_world->m_level->m_mainPlane != 0) {
        m_world->m_level->m_mainPlane->CenterScrollB();
    }
    m_mgr->RefreshGameClock(); // 0x8f620 direct (thunk 0x3d23)
    m_inputWarmup1 = 0;
    m_inputWarmup2 = 0;
    m_inputHalfSel = 0;
    if (m_mgr->m_soundEnabled != 0 && mode != 9) {
        m_mgr->m_inputState->Resume();
    }
    if (mode == 9) {
        g_frameTime = m_savedClock;
    }
    m_guts->Deactivate();
    RegisterInputBindings();
    m_hudSuppressed = 0;
    return 1;
}


// @early-stop
// /GX list-walk wall: the registration loop + CString error log are faithful, but
// the 13-arg AddGrunt + CString-temp EH frame keep it reloc-fuzzy.
RVA(0x000d5960, 0x160)
i32 CPlay::AddLevelGruntz() {
    CDDrawGroupNode* node =
        reinterpret_cast<CDDrawGroupNode*>(m_world->m_childGroup->m_list.GetHeadPosition());
    while (node != 0) {
        CGameObject* g = node->m_obj;
        node = node->m_next;
        if (g == 0) {
            continue;
        }
        if (static_cast<void*>(g->m_7c->m_notify) != static_cast<void*>(CreateGruntStartingPoint)) {
            continue;
        }
        if (g->m_124 == g_curPlayer) {
            continue;
        }
        i32 x = ((g->m_screenX & ~0x1f) + 0x10);
        i32 y = ((g->m_screenY & ~0x1f) + 0x10);
        // ILT 0x40bb == ?PlaceObject@CTriggerMgr@@ @0x6b6d0. The 13th arg really is an
        // address at THIS site (the a30
        // slot is kind-dependent).
        i32 r = m_mgr->m_cmdGrid->PlaceObject(
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
            reinterpret_cast<i32>(&g->m_extent.left)
        );
        if (r == -1) {
            CString msg;
            msg.Format("Could not add Grunt: Player=%d", g->m_124, y, x);
            // `mov ecx,[0x64556c]; push msg; call 0x417e` (read off the retail site):
            // a __thiscall on the singleton - CGruntzMgr::EnterModalUI @0x8ef10 (ILT
            // 0x417e).
            (g_gameReg)->EnterModalUI(msg); // CString -> LPCTSTR (implicit)
            return 0;
        }
        g->m_flags |= 0x10000;
    }
    return 1;
}

RVA(0x000dd050, 0x24b)
i32 CPlay::BuildGruntNamespaceList(i32 arg) {
    CString s;
    s = "NORMALGRUNT";
    if (!BuildAssetNamespacePrefixes(s, 1, 0, arg)) {
        return 0;
    }
    s = "DEATHZ";
    if (!BuildAssetNamespacePrefixes(s, 1, 0, arg)) {
        return 0;
    }
    s = "ENTRANCEZ";
    if (!BuildAssetNamespacePrefixes(s, 1, 0, arg)) {
        return 0;
    }
    s = "EXITZ";
    if (!BuildAssetNamespacePrefixes(s, 1, 0, arg)) {
        return 0;
    }
    s = "GRUNTPUDDLE";
    if (!BuildAssetNamespacePrefixes(s, 1, 0, arg)) {
        return 0;
    }
    s = "PICKUPS";
    if (!BuildAssetNamespacePrefixes(s, 1, 0, arg)) {
        return 0;
    }
    s = "BOMBGRUNT";
    if (!BuildAssetNamespacePrefixes(s, 1, 0, arg)) {
        return 0;
    }
    return 1;
}

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
    if (!BuildAssetNamespacePrefixes(s, 0, 0, arg)) {
        return 0;
    }
    s = "WARLORDZ_VIKING";
    if (!BuildAssetNamespacePrefixes(s, 0, 0, arg)) {
        return 0;
    }
    s = "WARLORDZ_PATTON";
    if (!BuildAssetNamespacePrefixes(s, 0, 0, arg)) {
        return 0;
    }
    return 1;
}

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
                BuildHelpReveal(0);
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
            BuildHelpReveal(0);
            loaded[0x21] = 1;
        }
        if (!BuildGruntTypeNameTable(GRUNT_TYPE_REAPER, 1, 0, ctx)) {
            return 0;
        }
        if (loaded[0x22] == 0) {
            BuildHelpReveal(0);
            loaded[0x22] = 1;
        }
        CString s("WARLORDZ_NAPOLEAN");
        if (!BuildAssetNamespacePrefixes(s, 1, 0, ctx)) {
            return 0;
        }
        if (loaded[0x23] == 0) {
            BuildHelpReveal(0);
            loaded[0x23] = 1;
        }
        s = "WARLORDZ_VIKING";
        if (!BuildAssetNamespacePrefixes(s, 1, 0, ctx)) {
            return 0;
        }
        if (loaded[0x24] == 0) {
            BuildHelpReveal(0);
            loaded[0x24] = 1;
        }
        s = "WARLORDZ_PATTON";
        if (!BuildAssetNamespacePrefixes(s, 1, 0, ctx)) {
            return 0;
        }
        if (loaded[0x25] == 0) {
            BuildHelpReveal(0);
            loaded[0x25] = 1;
        }
        return 1;
    }
    // (retail tests the list-head SUB-OBJECT address, group+0x10 - kept as-is)
    char* head = reinterpret_cast<char*>(&this->m_world->m_childGroup->m_list);
    if (!head) {
        return 0;
    }
    CDDrawGroupNode* node =
        reinterpret_cast<CDDrawGroupNode*>(this->m_world->m_childGroup->m_list.GetHeadPosition());
    while (node) {
        CGameObject* obj = node->m_obj;
        CDDrawGroupNode* nxt = node->m_next;
        if (obj) {
            void* marker = static_cast<void*>(obj->m_7c->m_notify);
            if (marker == static_cast<void*>(CreateGruntStartingPoint)) {
                i32 v = obj->m_11c;
                if (v) {
                    if (!BuildGruntTypeNameTable(v, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[v] == 0) {
                        BuildHelpReveal(0);
                        loaded[v] = 1;
                    }
                }
                v = obj->m_120;
                if (v) {
                    if (!BuildGruntTypeNameTable(v, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[v] == 0) {
                        BuildHelpReveal(0);
                        loaded[v] = 1;
                    }
                }
                switch (obj->m_118) {
                    case 0x7:
                        if (!BuildGruntTypeNameTable(1, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[1] == 0) {
                            BuildHelpReveal(0);
                            loaded[1] = 1;
                        }
                        break;
                    case 0x8:
                        if (!BuildGruntTypeNameTable(3, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[3] == 0) {
                            BuildHelpReveal(0);
                            loaded[3] = 1;
                        }
                        break;
                    case 0x9:
                        if (!BuildGruntTypeNameTable(5, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[5] == 0) {
                            BuildHelpReveal(0);
                            loaded[5] = 1;
                        }
                        break;
                    case 0xa:
                        if (!BuildGruntTypeNameTable(7, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[7] == 0) {
                            BuildHelpReveal(0);
                            loaded[7] = 1;
                        }
                        break;
                    case 0xb:
                        if (!BuildGruntTypeNameTable(0xd, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[0xd] == 0) {
                            BuildHelpReveal(0);
                            loaded[0xd] = 1;
                        }
                        break;
                    case 0xc:
                        if (!BuildGruntTypeNameTable(0x11, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[0x11] == 0) {
                            BuildHelpReveal(0);
                            loaded[0x11] = 1;
                        }
                        break;
                    case 0xf:
                        if (!BuildGruntTypeNameTable(0x13, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[0x13] == 0) {
                            BuildHelpReveal(0);
                            loaded[0x13] = 1;
                        }
                        break;
                    case 0x10:
                        if (!BuildGruntTypeNameTable(0x1e, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[0x1e] == 0) {
                            BuildHelpReveal(0);
                            loaded[0x1e] = 1;
                        }
                        break;
                }
            } else if (marker == static_cast<void*>(CreateInGameIcon)) {
                i32 cv = obj->m_124 == 0x32 ? obj->m_118 : obj->m_124;
                if (cv >= 1 && cv <= 0x16 && cv != 0x14) {
                    m_mgr->m_scoreHud->m_34++;
                } else if (cv >= 0x17 && cv <= 0x20) {
                    m_mgr->m_scoreHud->m_30++;
                } else if (cv >= 0x36 && cv <= 0x3c) {
                    m_mgr->m_scoreHud->m_38++;
                } else if (cv == 0x50) {
                    m_mgr->m_scoreHud->m_40++;
                }
                i32 d = obj->m_124;
                if (d <= 0x20) {
                    if (!BuildGruntTypeNameTable(d, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[obj->m_124] == 0) {
                        BuildHelpReveal(0);
                        loaded[obj->m_124] = 1;
                    }
                } else if (d == GRUNT_TYPE_HAREKRISHNA) {
                    if (!BuildGruntTypeNameTable(GRUNT_TYPE_HAREKRISHNA, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[0x21] == 0) {
                        BuildHelpReveal(0);
                        loaded[0x21] = 1;
                    }
                } else if (d == GRUNT_TYPE_REAPER) {
                    if (!BuildGruntTypeNameTable(GRUNT_TYPE_REAPER, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[0x22] == 0) {
                        BuildHelpReveal(0);
                        loaded[0x22] = 1;
                    }
                } else if (d == 0x55 || d == 0x32) {
                    if (!BuildGruntTypeNameTable(obj->m_118, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[obj->m_118] == 0) {
                        BuildHelpReveal(0);
                        loaded[obj->m_118] = 1;
                    }
                }
            } else if (marker == static_cast<void*>(CreateCoveredPowerup)
                       || marker == static_cast<void*>(CreateGiantRock)) {
                i32 cv = obj->m_11c == 0x32 ? obj->m_118 : obj->m_11c;
                if (cv >= 1 && cv <= 0x16 && cv != 0x14) {
                    m_mgr->m_scoreHud->m_34++;
                } else if (cv >= 0x17 && cv <= 0x20) {
                    m_mgr->m_scoreHud->m_30++;
                } else if (cv >= 0x36 && cv <= 0x3c) {
                    m_mgr->m_scoreHud->m_38++;
                } else if (cv == 0x50) {
                    m_mgr->m_scoreHud->m_40++;
                }
                i32 e = obj->m_11c;
                if (e <= 0x20) {
                    if (!BuildGruntTypeNameTable(e, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[obj->m_11c] == 0) {
                        BuildHelpReveal(0);
                        loaded[obj->m_11c] = 1;
                    }
                } else if (obj->m_124 == GRUNT_TYPE_HAREKRISHNA) {
                    if (!BuildGruntTypeNameTable(GRUNT_TYPE_HAREKRISHNA, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[0x21] == 0) {
                        BuildHelpReveal(0);
                        loaded[0x21] = 1;
                    }
                } else if (obj->m_124 == GRUNT_TYPE_REAPER) {
                    if (!BuildGruntTypeNameTable(GRUNT_TYPE_REAPER, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[0x22] == 0) {
                        BuildHelpReveal(0);
                        loaded[0x22] = 1;
                    }
                } else if (e == 0x55 || e == 0x32) {
                    if (!BuildGruntTypeNameTable(obj->m_118, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[obj->m_118] == 0) {
                        BuildHelpReveal(0);
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
// descriptor is a LeafCue (LeafCue.h): +0x18 is its interval/display duration.)
// @early-stop
// ~67% Lookup out-param zero-init scheduling wall (large unrolled fn): logic is
// complete and every name string + duration is byte-exact (all relocs pair), but
// MSVC5 permutes each block's `lea &out` / `out = 0` init / m_c->m_soundRegistry load vs
// retail (retail hoists the first `lea &out` into the prologue, shifting the
// out-slot operand 0xc<->0x10), repeating across all 32 blocks. Documented wall;
// deferred to the final sweep. docs/patterns/outparam-zeroinit-scheduling.md.
RVA(0x000dc060, 0x51b)
i32 CPlay::SetEffectSpriteDurations() {
    LeafCue* d;
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("GAME_PYRAMIDMOVE", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("GAME_TELEPORTEROPEN", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("GAME_TELEPORTERCLOSE", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("GAME_TELEPORTERALL", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 4000;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("GAME_BRICKBREAK", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("LEVEL_DEATHBRIDGEMOVE", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("LEVEL_WATERBRIDGEMOVE", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("LEVEL_ROCKBREAK", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("LEVEL_LAVAGEYSER", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("LEVEL_TRAPDOORCLOSE", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("LEVEL_TRAPDOOROPEN", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("LEVEL_CANDLEIGNITE", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("LEVEL_CANDLEUP", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("LEVEL_CANDLEDOWN", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("LEVEL_GOLFBALLAIR2", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 250;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("LEVEL_GOLFBALLHOLE", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 250;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("LEVEL_GOLFBALLSINK", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 250;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("GAME_EXPLOSION1", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("LEVEL_OUTLETHAZARD", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup(
        "GRUNTZ_DEATHZ_DEATHZFREEZE1A",
        reinterpret_cast<void*&>(d)
    );
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup(
        "GRUNTZ_DEATHZ_DEATHZFREEZE2A",
        reinterpret_cast<void*&>(d)
    );
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup(
        "GRUNTZ_DEATHZ_DEATHZUNFREEZE1A",
        reinterpret_cast<void*&>(d)
    );
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup(
        "GRUNTZ_DEATHZ_DEATHZUNFREEZE1A",
        reinterpret_cast<void*&>(d)
    );
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("GRUNTZ_DEATHZ_RESSURECT", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup(
        "GRUNTZ_DEATHZ_DEATHZSQUASH1A",
        reinterpret_cast<void*&>(d)
    );
    if (d != 0) {
        d->m_18 = 100;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("LEVEL_CLOUDHAZARDMOVE", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 10000;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("LEVEL_CLOUDHAZARDKILL", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 3000;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup(
        "GRUNTZ_DEATHZ_DEATHZELECTROCUTE1A",
        reinterpret_cast<void*&>(d)
    );
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup(
        "GRUNTZ_NERFGUNGRUNT_NERFGUNZGRUNTP1AS1",
        reinterpret_cast<void*&>(d)
    );
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup(
        "GRUNTZ_GUNHATGRUNT_GUNHATGRUNTP1AS1",
        reinterpret_cast<void*&>(d)
    );
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup(
        "GRUNTZ_WELDERGRUNT_WELDERZGRUNTP1AS1",
        reinterpret_cast<void*&>(d)
    );
    if (d != 0) {
        d->m_18 = 1000;
    }
    d = 0;
    m_world->m_soundRegistry->m_10.Lookup("LEVEL_PLANEHAZARDFLY", reinterpret_cast<void*&>(d));
    if (d != 0) {
        d->m_18 = 5000;
    }
    return 1;
}


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
