#include <rva.h>

#include <Gruntz/Play.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <DinMgr2/InputMgrPtr.h>
#include <Dsndmgr/MidiManager.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/ActionOptionsMenuBar.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/AreaMgr.h>
#include <Gruntz/BankMgr.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CBrickz.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/CheatMgr.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/DrawDebugStats.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GameText.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/ImageSets.h>
#include <Gruntz/InputState.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/LightFxRender.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MgrAutoScroll.h>
#include <Gruntz/Multi.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/PlayerCommandKind.h>
#include <Gruntz/PlayHudLayoutPx.h>
#include <Gruntz/PlayIntervalMs.h>
#include <Gruntz/PlayStringId.h>
#include <Gruntz/QuestLevel.h>
#include <Gruntz/SBI_Image.h>
#include <Gruntz/SbiMenuItemState.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarTab.h>
#include <Gruntz/String.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/Timer.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/View.h>
#include <Gruntz/VoiceManager.h>
#include <Gruntz/Warlord.h>
#include <Gruntz/WorldSoundSet.h>
#include <Gruntz/WwdGameReg.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <Io/SaveGame.h>
#include <Pix16.h>
#include <Rez/FrameClock.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/EngStr.h>
#include <Wap32/Object.h>
#include <Wap32/ScreenGeometry.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/WwdFile.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <ddraw.h>
#include <new>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

class CImage;

GZ_ENUM_BEGIN(ToolCursorId)
    CURSOR_POINTER = 0,
    CURSOR_CHIP_FIRST = 1,
    CURSOR_CHIP_LAST = 0x26,
    CURSOR_FLAILINGGRUNT = 0x66,
    CURSOR_TOOL_HANDZ = 0xc8,
    // The tool band's low edge. LoadCursorSprites guards its switch with
    // `< CURSOR_TOOL_FIRST` (retail 0xd0159: `cmp eax,0xc8; jl`).
    CURSOR_TOOL_FIRST = CURSOR_TOOL_HANDZ,
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
GZ_ENUM_END(ToolCursorId)

#define CLEAR_TAB_HINT(sndHost)                                                                    \
    do {                                                                                           \
        SoundCueRegistry* _s = (sndHost);                                                      \
        if (_s->m_silentMode == 0) {                                                                 \
            SoundCue* found = NULL;                                                                 \
            MapLookup(_s->m_cues, "GAME_TABHIGHLIGHT1", found);                                    \
            if (found != NULL)                                                                     \
                found->PlayIfElapsed(g_soundVolumePercent, 0, 0, 0);                                       \
        }                                                                                          \
    } while (0)

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* ob = NULL;
    map.Lookup(name, ob);
    return static_cast<CDDrawWorker*>(ob);
}

static inline CDDrawWorker* LookupWorker(CDDrawSurfaceMgr* host, LPCTSTR name) {
    CObject* ob = NULL;
    host->m_imageRegistry->m_workersByName.Lookup(name, ob);
    return static_cast<CDDrawWorker*>(ob);
}

DATA(0x002bf3bc)
u32 g_engineFrameDelta = 0;
DATA(0x002bf3c0)
u32 g_soundCueTimeMs = 0;

DATA(0x00212618)
i32 g_lastLevelNum = -1;

DATA(0x0024c284)
i32 g_profAccA;
DATA(0x0024c288)
i32 g_profAccB;

DATA(0x00212f78)
char* g_colorNames[] =
    {"Color 0", "Color 1", "Color 2", "Color 3", "Color 4", "Color 5", "Color 6", "Color 7"};
DATA(0x00212fc0)
char* g_difficultyNames[] = {"Easy", "Normal", "Hard"};

// NOT sound channels, despite the name this was reconstructed under: it is one
// slot per ColorTint saying whether that colour is still AVAILABLE to a player.
// The multiplayer roster proves both halves - it calls FindFree(), assigns the
// result to s->m_colorIndex, then Set(free, false) to take the colour, and
// Set(s->m_colorIndex, true) to hand it back when the slot empties. The size is
// TINT_COUNT, not a coincidence.
DATA(0x0024c3f0)
i32 g_soundChannelInUse[TINT_COUNT];

DATA(0x002455f0)
i32 g_levelBias100 = 0;

DATA(0x0024c020)
char g_customLevelText[0x200];

// @early-stop
// INLINE/CALL-SET with two independent halves; NOT a wall, and the second half is
// a demonstrated live lever.
//
// (a) ~CTileTriggerContainer: retail CALLS it here (rel32 to 0xc8640) and EXPANDS
// it in CMulti::LoadGameAssetNamespaces (0xb5460) from the same one inline
// definition, so its linkage is not the variable - out-of-lining it satisfies this
// caller only by breaking that one, measured.  It is NOT budget either: cb(callee)
// titrated 22 -> ~134 and cb(caller) +336 both leave the site expanded in both
// callers, so the /Ob1 arithmetic does not reach this decision.  Unexplained.
//
// (b) the CSbiHlRow arrays inside the expanded inline CStatusBarMgr ctor: retail
// hands the element ctor's ADDRESS to ??_H twice (m_groupSlots[3], m_hlGrid[12])
// and expands the two scalars; we reach one ??_H and a ctor loop.  This one IS
// budget-bound - cb(CSbiHlRow::CSbiHlRow) = 13 + 4 stores sits right at the <= 40
// exemption edge, and padding it makes ??_H appear (K=1 and K=8 emit one).  The
// response is non-monotone, as sequential consumption predicts, so the open
// question is which side carries the wrong front-end mass, not whether it moves.
// See docs/relevations/cl5-inline-budget-is-arithmetic-you-can-compute.md.
RVA(0x000c7ec0, 0x5f5)
i32 CPlay::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {
    {
        if (mgr == NULL) {
            return 0;
        }
        GruntzPlayer* sub = mgr->m_options;
        if (sub == NULL) {
            return 0;
        }
        sub->m_liveGate = 1;
        sub->m_humanControlled = 1;
        m_region0Gate = 0;
        m_region1Gate = 0;
        m_region2Gate = 0;
        m_region3Gate = 0;
        m_viewMode = VIEW_MODE_IDLE;
        m_hudSuppressed = 1;
        m_cameraBookmarkIndex = -1;
        m_defeatCountdownActive = 0;
        m_scrollEdgeActive = 0;
        m_scrollEdgeLock = 0;
        m_frameMarker = NULL;

        if (!CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId)) {
            return 0;
        }

        CChatBoxOwner* ctl = new CChatBoxOwner;
        m_hitTest = ctl;
        if (m_hitTest->Attach(m_world, m_mgr->m_chatLog) == 0) {
            // retail loads m_hitTest ONCE (0xc7f9a `mov esi,[ebx+0x2e0]`) and feeds
            // the same esi to Deactivate and operator delete - a cached local, not
            // two member reads.
            CChatBoxOwner* dead = m_hitTest;
            if (dead == NULL) {
                return 0;
            }
            dead->Deactivate();
            ::operator delete(dead);
            m_hitTest = NULL;
            return 0;
        }
        m_hitTest->m_inputActive = 0;
        m_hitTest->Configure(CHATBOX_WITH_RIGHT_STATUSBAR);

        m_guts = new CStatusBarMgr;
        if (m_guts->LoadBattlezItemConfig(m_world) == 0) {
            if (m_guts == NULL) {
                return 0;
            }
            delete m_guts;
            m_guts = NULL;
            return 0;
        }

        CTileTriggerContainer* r78 = new CTileTriggerContainer;
        m_beginMarker = r78;
        if (m_beginMarker->Initialize() == 0) {
            if (m_beginMarker == NULL) {
                return 0;
            }
            delete m_beginMarker;
            m_beginMarker = NULL;
            return 0;
        }

        CTimer* r50 = new CTimer;
        m_frameMarker = r50;
        if (r50 == NULL) {
            return 0;
        }

        while (ShowCursor(0) >= 0) {
        }
        m_initialFramePending = 1;
        m_notifyLatch = 0;
        m_completedFinalLevel = 0;
        memset(&m_saveSlot, 0, sizeof(m_saveSlot));
        mgr->ResetClockGlobals();
        m_savedClock = 0;
        m_rngSeed = timeGetTime();
        m_lightFx = NULL;
        if (m_mgr->m_loadingSaveGame == 0) {
            m_mgr->m_saveInfoRec = NULL;
        }
        if (!LoadImageBanks()) {
            return 0;
        }
        PostLoadImageBanks();
        if (!LoadByMode(areaArg, 1)) {
            return 0;
        }
        if (!LoadCursorSprites(0, 0)) {
            return 0;
        }
        CWwdGameObjectA* peer = m_scrollSink;
        if (peer) {
            peer->m_stateFlags |= SPRITE_STATE_HIDDEN;
        }
        return 1;
    }
}

RVA(0x000c8700, 0x1f4)
void CPlay::ReleaseResources() {
    i32 i;

    CLightFxRender* fx = m_lightFx;
    if (fx) {
        fx->Reset();
        ::operator delete(fx);
        m_lightFx = NULL;
    }
    OnExit();
    if (m_mgr) {
        m_mgr->m_isBattlezLevel = 0;
        m_mgr->m_strWorldFile.Empty();
    }
    m_saveSlot.m_type = 0;
    i32 t = 0;
    do {
        g_gameReg->m_options[t].m_liveGate = 0;
        t++;
    } while (t < 4);
    if (m_mgr && m_mgr->m_chatLog) {
        m_mgr->m_chatLog->FreeNodes();
    }
    if (m_guts) {
        delete m_guts;
        m_guts = NULL;
    }
    CChatBoxOwner* hit = m_hitTest;
    if (hit) {
        hit->Deactivate();
        ::operator delete(hit);
        m_hitTest = NULL;
    }
    if (m_beginMarker) {
        delete m_beginMarker;
        m_beginMarker = NULL;
    }
    CTimer* fm = m_frameMarker;
    if (fm) {
        fm->Reset();
        ::operator delete(fm);
        m_frameMarker = NULL;
    }
    for (i = 0; i < StartMarkerCount(); i++) {
        Coord* node = StartMarkerAt(i);
        if (node != NULL) {
            CoordPoolNode* p = g_coordPool.NodeOf(node);
            p->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = p;
        }
    }
    m_startMarkers.SetSize(0, -1);
    for (i32 k = 0; k < 4; k++) {
        for (i = 0; i < PlacedObjectCellCount(k); i++) {
            Coord* node = PlacedObjectCellAt(k, i);
            if (node != NULL) {
                CoordPoolNode* p = g_coordPool.NodeOf(node);
                p->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = p;
            }
        }
        m_placedObjectCells[k].SetSize(0, -1);
    }
    for (i = 0; i < CameraBookmarkCount(); i++) {
        Coord* node = CameraBookmarkAt(i);
        if (node != NULL) {
            CoordPoolNode* p = g_coordPool.NodeOf(node);
            p->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = p;
        }
    }
    m_cameraBookmarkIndex = -1;
    m_cameraBookmarks.SetSize(0, -1);
    CState::ReleaseResources();
}

RVA(0x000c8a10, 0x119)
i32 CPlay::EnterState(GameStateId previousState) {
    POINT pt;
    GetCursorPos(&pt);
    m_cursorX = pt.x;
    m_cursorY = pt.y;
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    if (previousState == GAMESTATE_HELP) {
        g_frameTime = m_savedClock;
        if (!EnterMode(GAMESTATE_HELP)) {
            return 0;
        }
        m_stepCountdown = 2;
    } else if (m_renderDisabled == 0 || m_mgr->m_gameMode == GAMEMODE_MULTIPLAYER) {
        if (!EnterMode(previousState)) {
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
    m_cursorTargetValid = 0;
    m_worldReady = 0;
    if (m_renderDisabled == 0) {
        if (previousState != GAMESTATE_HELP) {
            m_mgr->m_worldSounds->Resume();
        }
        (static_cast<CTriggerMgr*>(m_mgr->m_cmdGrid))->DestroyAllAnims();
        (static_cast<CVoiceManager*>(m_mgr->m_voiceManager))->PauseAllVoices();
    }
    return 1;
}

RVA(0x000c8b80, 0x11b)
i32 CPlay::LeaveState(GameStateId nextState) {
    m_mgr->m_voiceManager->PauseAllVoices();
    m_savedClock = static_cast<i32>(g_frameTime);
    if (m_notifyLatch) {
        QuitToMenu();
    }
    if (nextState != GAMESTATE_HELP) {
        RECT r;
        m_world->m_drawTarget->m_overlayPair->m_surface->Fill(0);
        CString s;
        s.LoadString(IDS_PLEASE_WAIT);
        tagSIZE mode = m_mgr->GetModeSize();
        r.right = mode.cx;
        r.bottom = mode.cy;
        r.left = 0;
        r.top = 0;
        ShowHudMessage(m_world, &s, &r, 0x78, 1, 0xff, 0xff, 0, 1);
        RetireScene(0x50, 0x3e8, 0, 1);
        if (m_mgr && m_mgr->m_cmdGrid) {
            m_mgr->m_cmdGrid->RemovePlayerUnitsImmediately(TM_ALL_PLAYERS);
        }
    }
    return 1;
}

// @early-stop
RVA(0x000c8cf0, 0xc14)
i32 CPlay::Render() {

    m_drewThisFrame = 0;
    HandleDragMove(0, m_cursorX, m_cursorY);

    if (m_renderDisabled != 0) {
        return 1;
    }

    if (m_inGame != 0) {

        StepInputA();
        LoadScrollSpeedOptions();
        m_world->m_level->ActivateVisibleObjectsOnMainPlane();

        g_soundCueTimeMs = g_lastNow;
        g_engineFrameDelta = g_frameDelta;

        m_world->m_childGroup->TickKillCues(0);
        m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
        m_world->m_workerList->RenderAndPruneWorkers(
            m_world->m_drawTarget->m_backPair,
            m_world->m_drawTarget->m_overlayPair
        );
        m_mgr->m_worldSounds->Retune(
            m_world->m_level->m_mainPlane->m_snappedX,
            m_world->m_level->m_mainPlane->m_snappedY
        );
        SoundStream* stream = m_world->m_soundStream;
        if (stream != NULL) {
            u32 t = timeGetTime();
            stream->TickVolumeRamps(t);
            stream->TickStreams(t);
        }
        m_beginMarker->UpdateTimedLogics(g_frameDelta);
        m_guts->LoadMainStatusBarSprite();

        {
            if (static_cast<i64>(g_frameTime) - m_cueTiming.m_start.m_v
                >= m_cueTiming.m_interval.m_v) {
                m_cueToggle = (m_cueToggle == 0);
                m_cueTiming.m_interval.m_lo = CUE_INTERVAL_MS;
                m_cueTiming.m_interval.m_hi = 0;
                m_cueTiming.m_start.m_lo = static_cast<i32>(g_frameTime);
                m_cueTiming.m_start.m_hi = 0;
            }
            if (m_cueToggle != 0) {
                PlayCueAt(0x8128, 0x78, 0, 0xff, 0xff, 0, 1, NULL);
            }
        }

        CDDrawSurfacePair* back =
            static_cast<CDDrawSurfacePair*>(m_world->m_drawTarget->m_backPair);
        if (back == NULL) {
            return 0;
        }

        m_frameMarker->Tick(static_cast<i32>(g_frameDelta));
        m_frameMarker->Draw(back, 1);
        AdvanceCursorAnimation(static_cast<i32>(g_frameDelta));
        DrawCursorSaveUnder(back);
        m_world->m_drawTarget->m_frontPair->m_surface->Flip(NULL);
        UpdateMgrScroll(g_gameReg, m_guts, m_region0Gate);
        m_world->m_level->DeactivateDistantObjectsOnMainPlane();
        return 1;
    }

    if (m_mgr->m_frameGate == 0
        && (g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER || m_levelOverlayOpen == 0)) {
        m_frameMarker->Tick(static_cast<i32>(g_frameDelta));
        m_mgr->m_cmdSubMgr->ExecuteScheduledCommands(0);

        if (m_cursorId == IDX(CURSOR_FLAILINGGRUNT)) {
            if (static_cast<i64>(g_frameTime) - m_bootyTiming.m_start.m_v
                >= m_bootyTiming.m_interval.m_v) {
                g_gameReg->m_voiceManager->PlayVoice(NULL, 0x33e, -1, 1, -1, -1);
                m_bootyTiming.m_interval.m_lo = BOOTY_INTERVAL_MS;
                m_bootyTiming.m_interval.m_hi = 0;
                m_bootyTiming.m_start.m_lo = static_cast<i32>(g_frameTime);
                m_bootyTiming.m_start.m_hi = 0;
            }
        }

        StepInputA();
        StepViewportResize();

        if (m_ambientInitDone == 0) {
            if (static_cast<i64>(g_frameTime) - m_ambientTiming.m_start.m_v
                >= m_ambientTiming.m_interval.m_v) {
                i32 ambientId = GetAmbientId();
                char sequenceName[0x40];
                wsprintfA(sequenceName, "AMBIENT%d", ambientId);
                if (g_gameReg->m_musicEnabled != 0) {
                    m_mgr->m_midi->PlaySequence(sequenceName, 1);
                } else {
                    MidiManager* midi = m_mgr->m_midi;
                    MidiSequence* sequence = midi->FindSequence(sequenceName);
                    if (sequence != NULL) {
                        midi->m_currentSequence = sequence;
                    }
                    if (m_mgr->m_midi->m_currentSequence != NULL) {
                        m_mgr->m_midi->m_currentSequence->SetLooping(1);
                    }
                }
                m_ambientInitDone = 1;
            }
        }

        if (m_region0Gate != 0) {
            m_world->m_drawTarget->m_backPair->m_surface->Fill(0);
            m_guts->Deactivate();
        }

        if (m_worldReady == 0) {
            if (m_mgr->m_cmdGrid->m_armed != 0) {
                m_mgr->m_cmdGrid->ScrollToActiveRecord();
            }
            LoadScrollSpeedOptions();
        }

        StepScroll();

        {
            u32 dt = g_frameDelta;
            if (dt > 0x12 && dt < 0xc8) {
                DrawWorldFrames();
            } else {
                DrawWorldFrame();
            }
        }

        m_mgr->m_worldSounds->Retune(
            m_world->m_level->m_mainPlane->m_snappedX,
            m_world->m_level->m_mainPlane->m_snappedY
        );
        {
            SoundStream* stream = m_world->m_soundStream;
            if (stream != NULL) {
                u32 t = timeGetTime();
                stream->TickVolumeRamps(t);
                stream->TickStreams(t);
            }
        }
        if (m_region1Gate != 0) {
            NotifyVisibleEntities();
        } else {
            m_world->m_level->VisitVisible(
                m_world->m_drawTarget->m_backPair,
                m_world->m_childGroup
            );
            m_world->m_workerList->RenderAndPruneWorkers(
                m_world->m_drawTarget->m_backPair,
                m_world->m_drawTarget->m_overlayPair
            );
        }
        m_beginMarker->UpdateTimedLogics(g_frameDelta);
        m_guts->LoadMainStatusBarSprite();
        m_mgr->m_tileGrid->UpdateDiagonals(m_mgr);

        if (m_lightFx != NULL && m_guts->m_position != STATUSBAR_HIDDEN
            && m_guts->m_activeTab != TAB_GAME) {
            RECT rc;
            if (m_guts->m_position == STATUSBAR_DOCK_LEFT) {
                SetRect(&rc, 20, 5, 140, 125);
            } else {
                SetRect(
                    &rc,
                    g_gameReg->GetModeSize().cx - 140,
                    5,
                    g_gameReg->GetModeSize().cx - 20,
                    125
                );
            }
            m_lightFx->Resize(static_cast<i32>(g_frameDelta), 0);
            m_lightFx->ComputeRect(m_world->m_drawTarget->m_backPair, &rc);
        }

        m_mgr->m_chatLog->Scroll(static_cast<i32>(g_frameDelta));
        CDDrawSurfacePair* view =
            static_cast<CDDrawSurfacePair*>(m_world->m_drawTarget->m_backPair);
        if (view == NULL) {
            return 0;
        }

        if (m_defeatCountdownActive != 0) {
            // Note: retail adds the pair FIRST and subtracts the clock
            // after (0xc92b8 add/adc, then 0xc92c2 sub/sbb).  cl reassociates it into
            // `(base - now) + dur` and no spelling stops it - a separate `deadline`
            // temp, a compound `-=`, and a separate `now` temp all fold to the same
            // tree, and swapping the two addends only moves which one is the
            // accumulator.
            i64 deadline =
                m_defeatCountdownTiming.m_interval.m_v + m_defeatCountdownTiming.m_start.m_v;
            i64 left = deadline - static_cast<i64>(g_frameTime);
            u32 leftMs = static_cast<u32>(left);
            if (left < 0) {
                leftMs = 0;
            }
            i32 secsLeft = static_cast<i32>(leftMs / MS_PER_SECOND) + 1;
            if (static_cast<i64>(g_frameTime) - m_defeatCountdownTiming.m_start.m_v
                >= m_defeatCountdownTiming.m_interval.m_v) {

                if (m_guts->m_destructButtonLocked != 0) {
                    g_gameReg->m_cmdGrid->StartPlayerDefeatSequence(5);
                } else {
                    i32 row = g_curPlayer;
                    g_gameReg->m_cmdGrid->StartPlayerDefeatSequence(row);
                }

                CTimer* marker = m_frameMarker;
                marker->m_unusedStamp.m_lo = 0;
                marker->m_unusedStamp.m_hi = 0;
                marker->m_accum.m_lo = 0;
                marker->m_accum.m_hi = 0;
                marker->m_running = 0;
                marker->m_currentMs = 0;
                m_guts->LockDestructButton(0);
                m_defeatCountdownActive = 0;

                if (g_gameReg->m_options[0].m_warlordObjectId != 0) {
                    // The lookup result is materialised BEFORE the null test, not
                    // nested inside it: retail's 0xc9396 `je` skips only the
                    // `mov eax,[esp+0x10]` and both paths share the one `cmp eax,edi`
                    // (a failed lookup leaves eax 0 from its own `test`).
                    CGameObject* out = NULL;
                    CGameObject* object = NULL;
                    if (MapLookupById(
                            g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
                            g_gameReg->m_options[0].m_warlordObjectId,
                            out
                        )) {
                        object = out;
                    }
                    if (object != NULL && object->m_logicRecord->m_userLogic != NULL) {
                        static_cast<CWarlord*>(object->m_logicRecord->m_userLogic)
                            ->ResolveDeathAnimation();
                    }
                }
            } else {

                CString tmp;
                tmp.Format("%d", secsLeft);
                RECT lvl = g_gameReg->m_world->m_level->m_planeCtx;
                RECT box;
                CopyRect(&box, &lvl);
                ShowHudMessageAlt(g_gameReg->m_world, &tmp, &box, 0x82, 1, 0xff, 0xff, 0, 1);
            }
        }

        m_frameMarker->Draw(view, 0);
        m_hitTest->LoadChatBoxSprite(view);
        DrawDebugStats();
        m_mgr->m_cmdGrid->OverlayRelease();

        if (m_winLoseBanner != 0 && m_guts->m_levelOverlayActive == 0
            && m_guts->m_quitConfirmationActive == 0) {

            if (static_cast<i64>(g_frameTime) - m_cueTiming.m_start.m_v
                >= m_cueTiming.m_interval.m_v) {
                m_cueToggle = (m_cueToggle == 0);
                m_cueTiming.m_interval.m_lo = CUE_INTERVAL_MS;
                m_cueTiming.m_interval.m_hi = 0;
                m_cueTiming.m_start.m_lo = static_cast<i32>(g_frameTime);
                m_cueTiming.m_start.m_hi = 0;
            }
            if (m_cueToggle != 0) {
                PlayCueAt(0x8129, 0x78, 0, 0xff, 0xff, 0, 1, NULL);
            }
        }

        AdvanceCursorAnimation(static_cast<i32>(g_frameDelta));
        DrawCursorSaveUnder(view);
        if (m_worldReady != 0) {
            view->DrawBox(&m_hudRect, 0xff);
        }
        m_world->m_drawTarget->m_frontPair->m_surface->Flip(NULL);
        UpdateMgrScroll(g_gameReg, m_guts, m_region0Gate);
        {
            CGameLevel* lvl = m_world->m_level;
            if (lvl->m_mainPlane != NULL) {
                lvl->m_mainPlane->DeactivateDistantObjects();
            }
        }

        if (m_region0Gate != 0) {
            if (static_cast<i64>(g_frameTime) - m_region0Timing.m_start.m_v
                >= m_region0Timing.m_interval.m_v) {
                SetTinyViewportCurse(0);
            }
        }
        if (m_region1Gate != 0) {
            if (static_cast<i64>(g_frameTime) - m_region1Timing.m_start.m_v
                >= m_region1Timing.m_interval.m_v) {
                SetDarknessCurse(0);
            }
        }
        if (m_region2Gate != 0) {
            if (static_cast<i64>(g_frameTime) - m_region2Timing.m_start.m_v
                >= m_region2Timing.m_interval.m_v) {
                SetMonitorCurse(0);
            }
        }
        if (m_region3Gate != 0) {
            if (static_cast<i64>(g_frameTime) - m_region3Timing.m_start.m_v
                >= m_region3Timing.m_interval.m_v) {
                SetRandomMoveIconsCurse(0);
            }
        }
        return 1;
    }

    StepInputA();
    CDDrawSurfacePair* back = m_world->m_drawTarget->m_backPair;
    if (back == NULL) {
        return 0;
    }
    {
        SoundStream* stream = m_world->m_soundStream;
        if (stream != NULL) {
            u32 t = timeGetTime();
            stream->TickVolumeRamps(t);
            stream->TickStreams(t);
        }
        if (m_paused != 0) {

            if (m_stepCountdown > 0) {
                m_stepCountdown = m_stepCountdown - 1;
                m_world->m_level->VisitVisible(
                    m_world->m_drawTarget->m_backPair,
                    m_world->m_childGroup
                );
                m_world->m_workerList->RenderAndPruneWorkers(
                    m_world->m_drawTarget->m_backPair,
                    m_world->m_drawTarget->m_overlayPair
                );
                m_guts->LoadMainStatusBarSprite();
                back->m_surface->ShadeRect(0x32, NULL);
                PlayCueAt(m_lastCueId, 0x78, 0, 0xff, 0xff, 0, 1, NULL);
                m_frameMarker->Draw(back, 1);
            }
            if (m_ambientInitDone == 0) {
                if (static_cast<i64>(g_frameTime) - m_ambientTiming.m_start.m_v
                    >= m_ambientTiming.m_interval.m_v) {
                    i32 ambientId = GetAmbientId();
                    char sequenceName[0x40];
                    wsprintfA(sequenceName, "AMBIENT%d", ambientId);
                    if (g_gameReg->m_musicEnabled != 0) {
                        m_mgr->m_midi->PlaySequence(sequenceName, 1);
                    } else {
                        MidiManager* midi = m_mgr->m_midi;
                        MidiSequence* sequence = midi->FindSequence(sequenceName);
                        if (sequence != NULL) {
                            midi->m_currentSequence = sequence;
                        }
                        if (m_mgr->m_midi->m_currentSequence != NULL) {
                            m_mgr->m_midi->m_currentSequence->SetLooping(1);
                        }
                    }
                    m_ambientInitDone = 1;
                }
            }
        } else {

            m_world->m_level->VisitVisible(
                m_world->m_drawTarget->m_backPair,
                m_world->m_childGroup
            );
            m_world->m_workerList->RenderAndPruneWorkers(
                m_world->m_drawTarget->m_backPair,
                m_world->m_drawTarget->m_overlayPair
            );
            m_guts->LoadMainStatusBarSprite();
            if (m_guts->m_levelOverlayActive == 0 && m_guts->m_quitConfirmationActive == 0) {
                PlayCueAt(0x812c, 0x78, 0, 0xff, 0xff, 0, 1, NULL);
            }
            m_frameMarker->Draw(back, 1);
        }
        AdvanceCursorAnimation(static_cast<i32>(g_frameDelta));
        DrawCursorSaveUnder(back);
        m_world->m_drawTarget->m_frontPair->m_surface->Flip(NULL);
    }
    return 1;
}

RVA(0x000c9c20, 0x79)
void CPlay::DrawWorldFrame() {
    TickStateMgrs();
    {

        CGameLevel* lvl = m_world->m_level;
        if (lvl->m_mainPlane != NULL) {
            lvl->m_mainPlane->ActivateVisibleObjects();
        }
    }
    g_soundCueTimeMs = g_lastNow;
    g_engineFrameDelta = g_frameDelta;
    m_world->m_childGroup->TickKillCues(0);
    m_mgr->m_cmdGrid->UpdateFrame(static_cast<i32>(g_frameDelta));
    if (g_gameReg->m_gameMode == GAMEMODE_REPLAY) {

        (g_gameReg)->AdvanceOptionsCycle();
    }
    m_guts->UpdateStatusBar(static_cast<i32>(g_frameDelta));
}

// @early-stop
// Residue is one register-naming swap (retail colours the two saved clocks ebx/ebp
// where cl picks ebp/ebx) and where the second `sub` lands.  Both clocks are live
// across SetGameClock, so this is the rotating-cursor class - see
// docs/relevations/cl5-c2-register-picker-is-a-rotating-cursor.md.  The substep
// counter is declared ABOVE the `if (steps > 0)` because that is where retail zeroes
// it (0xc9d0a `xor edi,edi`, before the `test edx,edx`).
RVA(0x000c9cc0, 0x12e)
i32 CPlay::DrawWorldFrames() {
    i32 delta = static_cast<i32>(g_frameDelta);
    i32 steps = static_cast<i32>((static_cast<u32>(delta) / FIXED_SUBSTEP_MS));

    i32 now;
    i32 accum;
    now = static_cast<i32>(g_lastNow);
    accum = static_cast<i32>(g_frameTime);

    u32 rem = static_cast<u32>(delta - steps * FIXED_SUBSTEP_MS);
    i32 saveNow = now;
    i32 saveDelta = delta;
    i32 saveAccum = accum;
    if (rem > 0) {
        steps = steps + 1;
    }
    now -= delta;
    accum -= delta;
    i32 i = 0;
    if (steps > 0) {
        i32 last = steps - 1;
        do {
            i32 dt = (i == last && rem > 0) ? static_cast<i32>(rem) : FIXED_SUBSTEP_MS;
            accum += dt;
            now += dt;
            m_mgr->SetGameClock(now, dt, accum);
            if (i > 0 && i < last) {

                CGameLevel* lvl = m_world->m_level;
                if (lvl->m_mainPlane != NULL) {
                    lvl->m_mainPlane->DeactivateDistantObjects();
                }
            }
            TickStateMgrs();
            {
                CGameLevel* lvl = m_world->m_level;
                if (lvl->m_mainPlane != NULL) {
                    lvl->m_mainPlane->ActivateVisibleObjects();
                }
            }
            m_world->m_childGroup->TickKillCues(0);
            m_mgr->m_cmdGrid->UpdateFrame(static_cast<i32>(g_frameDelta));
            if (g_gameReg->m_gameMode == GAMEMODE_REPLAY) {
                (g_gameReg)->AdvanceOptionsCycle();
            }
            m_guts->UpdateStatusBar(static_cast<i32>(g_frameDelta));
            i++;
        } while (i < steps);
    }
    m_mgr->SetGameClock(saveNow, saveDelta, saveAccum);
    return steps;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c9e40, 0x1d7)
i32 CPlay::ProfileInputFrame() {
    m_mgr->m_worldSounds->Retune(
        m_world->m_level->m_mainPlane->m_snappedX,
        m_world->m_level->m_mainPlane->m_snappedY
    );
    DWORD(WINAPI * tg)(void) = timeGetTime;

    i32 activateMs = static_cast<i32>(tg());
    TickStateMgrs();
    activateMs = static_cast<i32>(tg() - static_cast<u32>(activateMs));

    i32 deactMs = static_cast<i32>(tg());
    {

        CGameLevel* lvl = m_world->m_level;
        if (lvl->m_mainPlane != NULL) {
            lvl->m_mainPlane->ActivateVisibleObjects();
        }
    }
    deactMs = static_cast<i32>(tg() - static_cast<u32>(deactMs));

    i32 updateMs = static_cast<i32>(tg());
    m_world->m_childGroup->TickKillCues(1);
    m_mgr->m_cmdGrid->UpdateFrame(static_cast<i32>(g_frameDelta));
    m_guts->UpdateStatusBar(static_cast<i32>(g_frameDelta));
    updateMs = static_cast<i32>(tg() - static_cast<u32>(updateMs));

    i32 hitTestMs = static_cast<i32>(tg());
    hitTestMs = static_cast<i32>(tg() - static_cast<u32>(hitTestMs));

    i32 drawMs = static_cast<i32>(tg());
    m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
    drawMs = static_cast<i32>(tg() - static_cast<u32>(drawMs));

    i32 fixedMs = static_cast<i32>(tg());
    m_world->m_workerList->RenderAndPruneWorkers(
        m_world->m_drawTarget->m_backPair,
        m_world->m_drawTarget->m_overlayPair
    );
    fixedMs = static_cast<i32>(tg() - static_cast<u32>(fixedMs));

    i32 statusBarMs = static_cast<i32>(tg());
    m_guts->LoadMainStatusBarSprite();
    statusBarMs = static_cast<i32>(tg() - static_cast<u32>(statusBarMs));

    g_brickText1.Format(
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
    m_world->m_drawTarget->m_frontPair->m_surface->Flip(NULL);
    g_profAccB = static_cast<i32>((tg() - static_cast<u32>(g_profAccB)));
    g_profAccA = static_cast<i32>(tg());
    {
        CGameLevel* lvl = m_world->m_level;
        if (lvl->m_mainPlane != NULL) {
            lvl->m_mainPlane->DeactivateDistantObjects();
        }
    }
    g_profAccA = static_cast<i32>((tg() - static_cast<u32>(g_profAccA)));
    UpdateMgrScroll(g_gameReg, m_guts, m_region0Gate);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000ca0a0, 0x101)
i32 CPlay::ProfileDeltaFrame() {
    DWORD(WINAPI * tg)(void) = timeGetTime;
    i32 updates = 0;
    u32 t0 = tg();
    u32 d = g_frameDelta;
    if (d > 0x12 && d < 0xc8) {
        updates = DrawWorldFrames();
    } else {
        DrawWorldFrame();
    }
    i32 renderMs = static_cast<i32>((tg() - t0));
    m_mgr->m_worldSounds->Retune(
        m_world->m_level->m_mainPlane->m_snappedX,
        m_world->m_level->m_mainPlane->m_snappedY
    );
    u32 t2 = tg();
    m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
    m_world->m_workerList->RenderAndPruneWorkers(
        m_world->m_drawTarget->m_backPair,
        m_world->m_drawTarget->m_overlayPair
    );
    i32 presentMs = static_cast<i32>((tg() - t2));
    g_brickText1.Format(
        "Delta=%i, Update=%i, Draw=%i, NumUpdates=%i    ",
        static_cast<i32>(g_frameDelta),
        renderMs,
        presentMs,
        updates
    );
    DrawDebugStats();
    m_world->m_drawTarget->m_frontPair->m_surface->Flip(NULL);

    CGameLevel* lvl = m_world->m_level;
    if (lvl->m_mainPlane != NULL) {
        lvl->m_mainPlane->DeactivateDistantObjects();
    }
    return 1;
}

// @early-stop
// Two residues, both stack-packing/colouring.  (1) The frame is 4 bytes short of
// retail's 0xdc: retail keeps `key` in its own slot (S+0x18) and lets the late `scr`
// reuse warp's S+0x14, cl merges `key` into warp's slot instead - same slot count for
// the three CStrings but a different pairing, so every local above modeFlag sits 4
// low.  (2) Retail parks 1 in ebp/edi and re-materialises each zero fresh, cl parks
// the zero, which flips stores between the register and immediate forms and blocks
// the cross-jump of the two `atoi(p) / EndParse()` arms (cl's tails carry an extra
// `xor edi,edi` to restore the parked zero, so they no longer tail-merge).
RVA(0x000ca200, 0xe54)
i32 CPlay::LoadByMode(i32 level, i32) {
    CPlay* self = this;
    CGruntzMgr* gameReg;
    CSymTab* bank;
    CSymTab* prevTiles;
    i32 reload = 0;
    i32 diff = 0;

    char nameBuf[0x20];
    i32 initScratch[0x25];

    self->m_hudSuppressed = 1;
    g_frameDelta = 0;
    g_lastNow = 0;
    g_frameTime = 0;
    g_levelBias100 = 0;
    if (level > 0x64) {
        level -= 0x64;
        g_levelBias100 = 1;
    }

    // Per-dword, as in Render: retail fills 0x40/0x44 then 0x30/0x34 (0xca25e),
    // while the i64 spelling makes cl interleave the two pairs 0x40/0x30/0x44/0x34.
    CTimer* worker = self->m_frameMarker;
    if (worker != NULL) {
        worker->m_unusedStamp.m_lo = 0;
        worker->m_unusedStamp.m_hi = 0;
        worker->m_accum.m_lo = 0;
        worker->m_accum.m_hi = 0;
        worker->m_running = 0;
        worker->m_currentMs = 0;
    }

    SoundStream* grid = self->m_world->m_soundRegistry->m_soundStream;
    if (grid != NULL) {
        grid->StopAllStreams();
    }
    self->m_mgr->m_midi->ClearSequences();
    self->m_mgr->m_worldSounds->Teardown();
    self->m_mgr->m_voiceManager->PauseAllVoices();
    self->m_mgr->m_voiceManager->ClearVoiceIndicatorSlots();
    self->m_mgr->RestoreVideoMode(0);

    if (g_gameReg->m_gameMode != GAMEMODE_MULTIPLAYER) {
        g_curPlayer = 0;
        if (g_gameReg->m_frameGate != 0) {
            g_gameReg->m_frameGate ^= 1;
            g_gameReg->FinishLevel(g_gameReg->m_frameGate, 1);
        }
    }

    // Indexed, not a walked pointer: retail's cursor is biased to &m_anchors[0].m_y
    // (`lea eax,[esi+0x388]`, the array is at 0x384) and writes [eax-4]/[eax], which
    // is what cl's strength reduction of an INDEXED loop produces; `p++` pins the
    // cursor at +0 and both stores land one slot high.
    for (i32 a = 0; a < 4; ++a) {
        self->m_anchors[a].m_x = -1;
        self->m_anchors[a].m_y = -1;
    }

    g_soundCueTimeMs = g_lastNow;
    g_engineFrameDelta = g_frameDelta;

    for (i32 t = 0; t < 4; ++t) {
        CGruntzMgr* hostBase = self->m_mgr;
        gameReg = g_gameReg;
        GruntzPlayer* team = &hostBase->m_options[t];
        if (gameReg->m_gameMode == GAMEMODE_SINGLE) {
            team->SeedForSlot(t);
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

    i32 modeFlag = (Update() == GAMESTATE_MULTI) ? 1 : 0;
    CMulti* savedThis = modeFlag ? static_cast<CMulti*>(self) : NULL;
    self->m_initialFramePending = 1;
    self->m_levelIndex = level;
    {
        i32 r = (level - 1) % 0x24;
        self->m_levelType = static_cast<LevelArea>(r / 4 + 1);
    }

    g_frameTime = 0;
    if (g_gameReg->m_gameMode == GAMEMODE_REPLAY) {
        srand(timeGetTime());
    }
    g_resourceInstallActive = 0;
    Cmd_ResetScroll();
    g_gameReg->m_scoreHud->Init();
    g_gameReg->m_cmdSubMgr->m_pendingLocalCommands.RemoveAll();
    g_gameReg->m_cmdSubMgr->RecycleQueuedCommands();
    g_frameTicks = 0;
    self->m_returnToMenuOnComplete = 0;
    self->m_mgr->m_isCustomLevel = 0;

    CGruntzMgr* host = self->m_mgr;
    if (host->m_strWorldFile.GetLength() != 0) {
        CParseSource* ins;
        char* desc;
        char* p;
        char c;
        if (host->m_isBattlezLevel != 0) {

            bank = host->m_symParser->ResolvePath("GAME_BATTLEZ");
            if (bank == NULL) {
                goto fail0;
            }
            ins = bank->Insert(
                static_cast<const char*>(self->m_mgr->GetWorldFileName()),
                REZ_TAG_WWD
            );
            if (ins == NULL) {
                return 0;
            }
            desc = ins->BeginParse();
            if (desc == NULL) {
                goto fail0;
            }
            p = desc + 0x10;
            c = *p;
            while (c != 0) {
                if (c < '0' || c > '9') {
                    ++p;
                    c = *p;
                    continue;
                }
                break;
            }
            level = atoi(p);
            ins->EndParse();
        } else if (host->m_isMultiLevel != 0) {

            bank = host->m_symParser->ResolvePath("GAME_MULTI");
            if (bank == NULL) {
                goto fail0;
            }
            ins = bank->Insert(
                static_cast<const char*>(self->m_mgr->GetWorldFileName()),
                REZ_TAG_WWD
            );
            if (ins == NULL) {
                return 0;
            }
            desc = ins->BeginParse();
            if (desc == NULL) {
                goto fail0;
            }
            p = desc + 0x10;
            c = *p;
            while (c != 0) {
                if (c < '0' || c > '9') {
                    ++p;
                    c = *p;
                    continue;
                }
                break;
            }
            level = atoi(p);
            ins->EndParse();
        } else {

            // 1, not 0: this arm IS the custom-level path, and retail stores edi -
            // which `mov edi,0x1` at 0xca392 (the m_initialFramePending store) has
            // held ever since - into both fields at 0xca5a6 / 0xca5af.
            level = WwdFile::ValidateMainBlock(self->m_mgr->GetWorldFileName());
            self->m_returnToMenuOnComplete = 1;
            self->m_mgr->m_isCustomLevel = 1;
        }

        i32 r = (level - 1) % 0x24;
        self->m_levelIndex = level;
        self->m_levelType = static_cast<LevelArea>(r / 4 + 1);
    }

    sprintf(nameBuf, "AREA%i", IDX(self->m_levelType));
    bank = self->m_symParser->ResolvePath(nameBuf);
    self->m_levelBank = bank;
    if (bank == NULL) {
        goto fail0;
    }

    {
        LevelArea page = self->m_levelType;
        switch (page) {
            case AREA_ROCKY_ROADZ:
                g_areaPitDeath = DEATH_SINK;
                break;
            // 7, not DEATH_DROP(0).  cl hoists the switch bound into ecx
            // (`mov ecx,0x7` at 0xca5fb) precisely because the same constant is
            // stored in these two arms: retail writes `[g_areaPitDeath],ecx` /
            // `[g_areaHazardDeath],ecx` at 0xca627 and `[g_areaHazardDeath],ecx`
            // at 0xca63f, while the arm that really is DEATH_DROP writes the
            // immediate 0 (0xca6a9).  And PIT is assigned first in both.
            case AREA_GRUNTZICLEZ:
                g_areaPitDeath = DEATH_BURN;
                g_areaHazardDeath = DEATH_BURN;
                break;
            case AREA_TROUBLE_IN_THE_TROPICZ:
                g_areaPitDeath = DEATH_FALL;
                g_areaHazardDeath = DEATH_BURN;
                break;
            case AREA_HIGH_ON_SWEETZ:
                g_areaPitDeath = DEATH_FALL;
                g_areaHazardDeath = DEATH_QUICKFALL;
                break;
            case AREA_HIGH_ROLLERZ:
                g_areaPitDeath = DEATH_MELT;
                g_areaHazardDeath = DEATH_ELECTROCUTE;
                break;
            case AREA_HONEY_I_SHRUNK_THE_GRUNTZ:
                g_areaPitDeath = DEATH_SINK;
                g_areaHazardDeath = DEATH_EXPLODE;
                break;
            case AREA_MINIATURE_MASTERZ:
                g_areaPitDeath = DEATH_FALL2;
                g_areaHazardDeath = DEATH_EXPLODE;
                break;
            case AREA_GRUNTZ_IN_SPACE:
                g_areaPitDeath = DEATH_SINK;
                g_areaHazardDeath = DEATH_DROP;
                break;
            default:
                g_areaPitDeath = DEATH_SINK;
                g_areaHazardDeath = DEATH_DROP;
                break;
        }
    }

    {
        prevTiles = self->m_stateBank;
        self->m_stateBank = (self->m_levelBank);
        UpdateWindow(self->m_mgr->m_gameWnd->m_hwnd);

        host = self->m_mgr;
        if (host->m_strWorldFile.GetLength() != 0) {
            if (host->m_isBattlezLevel == 0 && host->m_isMultiLevel == 0) {
                sprintf(nameBuf, "CUSTOMLEVEL");
            }
        } else if (level > 0x24) {
            sprintf(nameBuf, "TRAINING");
        }
    }

    if (!LoadTitlePage(nameBuf, 0, 0, 0, 0, 1)) {
        goto fail0;
    }
    RetireScene(0x50, 0x3e8, 0, 1);
    DrawLevelInfoText();
    // RESTORE, not clear: retail reads m_stateBank into edi before overwriting it
    // (0xca6b9 `mov edi,[esi+0x2c]`) and writes edi back here (0xca722), so the
    // bank is saved across the title fade rather than nulled.
    self->m_stateBank = prevTiles;
    {
        i32* z = initScratch;
        i32 n = 0x25;
        while (n--) {
            *z++ = 0;
        }
    }
    LoadLoadingBarSprite();
    BuildHelpReveal(0);
    FreeListTeardown();
    if (modeFlag) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();

    if (!InitializeLevelArea(level)) {
        goto fail0;
    }

    {
        i32 cached = g_lastLevelNum;
        i32 eq = g_pAreaMgr->IsSameWorld(cached);
        // Note: retail lowers this with cl's negate idiom (0xca9dc
        // `neg ebp` / `sbb ebp,ebp` / `inc ebp`); both `!eq` and the ternary give
        // `test eax,eax` / `sete cl` here.  The sibling `diff` below already
        // matches (`cmp` / `setne`).
        reload = !eq;
        diff = (level != g_lastLevelNum) ? 1 : 0;
        if (g_pAreaMgr == NULL) {
            return 0;
        }
        g_lastLevelNum = level;

        BuildHelpReveal(0);
        if (modeFlag) {
            (savedThis)->AckJoinFailure();
        }
        RegisterInputBindings();

        BuildHelpReveal(0);
        if (modeFlag) {
            (savedThis)->AckJoinFailure();
        }
        RegisterInputBindings();

        if (!LoadActionTileSprites(diff)) {
            goto fail0;
        }
    }

    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (diff != 0 && (g_gameReg)->m_gameMode == GAMEMODE_SINGLE) {
        BuildWarlordNameTable(savedThis);
    }
    BuildHelpReveal(0);
    RegisterInputBindings();
    if (!LoadLevelImages(reload)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadGameImages(reload)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!BuildSpriteImageKeyTable(savedThis)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    RegisterInputBindings();
    if (!LoadLevelSounds(reload)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadGameSounds(reload)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadGruntSoundNamespaces(NULL)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    SetEffectSpriteDurations();
    if (modeFlag) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadLevelAnims(reload)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadGameAnims(reload)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!BuildAnizKeyTable(NULL)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!BuildWorldLevelPath(reload)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();

    self->m_mgr->RecomputeViewScale();
    if (self->m_world->m_level->m_mainPlane != NULL) {
        (static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane))->GetSize();
    }
    if (self->m_world->m_level->m_mainPlane != NULL) {
        (static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane))
            ->ActivateVisibleObjects();
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    self->m_mgr->m_tileGrid->Reset();

    {
        CDDrawWorkerHost* g5c = static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane);
        CGruntzMapMgr* host70 = self->m_mgr->m_tileGrid;
        if (!host70->LoadAttributes(g5c->m_gridW, g5c->m_gridH)) {
            goto fail0;
        }
    }
    if (!(static_cast<CMapMgr*>(self->m_mgr->m_tileGrid))->UpdateDiagonals(self->m_mgr)) {
        goto fail0;
    }

    if (self->m_lightFx == NULL) {
        CLightFxRender* ctx = new CLightFxRender;
        self->m_lightFx = ctx;
        if (!ctx->Init(self->m_mgr, 0xfa)) {
            goto fail0;
        }
    }
    if (!self->m_lightFx->BuildShape(self->m_levelType)) {
        goto fail0;
    }

    // The sense is NOT "equal": retail seeds the flag with the 1 it parked in ebp for
    // the mode compare (0xcab47) and CLEARS it on a match (0xcabd5 `xor ebp,ebp`), so
    // the quad scan runs when the world file is NOT the training world - and also when
    // LoadString fails, which jumps straight to the still-1 test at 0xcabd7.
    if (g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
        CString warp;
        i32 notTraining = 1;
        if (warp.LoadString(IDS_TRAINING_WORLD_NAME)) {
            if (strcmp(
                    static_cast<const char*>(warp),
                    static_cast<const char*>(g_gameReg->GetWorldFileName())
                )
                == 0) {
                notTraining = 0;
            }
        }
        if (notTraining) {
            ScanShuffleQuads();
        }
    }

    if (self->m_mgr->m_gameMode == GAMEMODE_REPLAY) {
        self->m_mgr->SyncOptionsState();
    }
    // Third arg is the manager, not 0 - CSaveGame::InitializeLevelSlot returns 0
    // immediately on a null mgr, so passing 0 made the whole call a no-op.  Retail
    // pushes the same `[esi+4]` it uses as the m_saveSink base (0xcac0d/0xcac13).
    self->m_mgr->m_saveSink
        ->InitializeLevelSlot(&self->m_saveSlot, self->m_levelIndex, self->m_mgr);
    {
        CString key;
        g_gameReg->m_cmdGrid->m_pendingFx = NULL;
        i32 count = self->m_levelIndex;
        i32 i = count - ((count - 1) % 4);
        for (; i < self->m_levelIndex; ++i) {

            key.Format("Level%i", i);
            CTriggerMgr* bm = g_gameReg->m_cmdGrid;
            i32 v = g_buteMgr.GetInt("WarpStone", static_cast<const char*>(key));
            bm->m_byteArr.SetAtGrow(bm->m_byteArr.GetSize(), static_cast<u8>(v));
        }
        self->m_guts->LoadMultiplayerBattlezConfig(self->m_levelIndex);

        CWwdGameObjectA* scrollSink =
            self->m_world->m_childGroup
                ->CreateSprite(0, 0, 0, 0x13880, "CursorSnapSprite", 0x40001);
        self->m_scrollSink = scrollSink;
        if (scrollSink != NULL) {
            self->m_world->m_childGroup->TickKillCues(0);
            if (savedThis == NULL) {

                CStatusBarMgr* tiles = self->m_guts;
                // Initialise to the docked-right origin and OVERWRITE, which is
                // retail's shape (0xcacfd `mov eax,0x1a9` / `cmp [ecx],ebx` / `je` /
                // `mov eax,0x249`).  A ternary compiles to cl's branchless select
                // `neg / sbb / and 0xa0 / add 0x1a9` - and the 0xa0 in it IS the
                // status-bar width the two origins differ by, which is what named
                // them.
                i32 originX = TIMER_ORIGIN_X_STATUSBAR_RIGHT_PX;
                if (tiles->m_position != STATUSBAR_DOCK_RIGHT) {
                    originX = TIMER_ORIGIN_X_PX;
                }
                if (!self->m_frameMarker->LoadTimerSprite(originX, TIMER_ORIGIN_Y_PX)) {
                    CTimer* spr = self->m_frameMarker;
                    if (spr != NULL) {
                        spr->Reset();
                        ::operator delete(spr);
                        self->m_frameMarker = NULL;
                    }
                }
            }
            {
                // The load chain is NOT the multiplayer alternative to the timer
                // sprite - it runs for every level. As an `else` it never executed
                // in single player (savedThis is only set when modeFlag is), so the
                // level got no warlord sprites, no tile scan, no ValidateLevelTiles
                // (hence an empty switch registry and error 1100) and no gruntz.
                if (LoadWarlordSprites(savedThis, initScratch) && ScanBuildTiles()
                    && ValidateLevelTiles() && AddLevelGruntz()) {
                    self->m_world->m_childGroup->TickKillCues(0);
                    self->m_guts->StartChipMachineCycle();
                    (static_cast<DirectInputMgr2*>(g_inputMgr))->ReadAll();
                    while (ShowCursor(0) >= 0)
                        ;
                    self->m_mgr->RefreshGameClock();
                    if (self->m_world->m_level->m_mainPlane != NULL) {
                        (static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane))
                            ->GetSize();
                    }
                    if (self->m_world->m_level->m_mainPlane != NULL) {
                        (static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane))
                            ->ActivateVisibleObjects();
                    }
                    BuildHelpReveal(0);
                    if (modeFlag) {
                        (savedThis)->AckJoinFailure();
                    }
                    RegisterInputBindings();
                    if (BuildMusicCategoryTable(reload)) {
                        goto okContinue;
                    }
                }
                return 0;
            }
        }

    okContinue:
        BuildHelpReveal(0);
        if (modeFlag) {
            (savedThis)->AckJoinFailure();
        }
        RegisterInputBindings();
        BuildHelpReveal(1);
        ActiveWait(0x64);
        if (modeFlag) {
            (savedThis)->AckJoinFailure();
        }

        gameReg = g_gameReg;
        if (gameReg->m_loadingSaveGame == 0) {
            CDDSurface* mapHost = self->m_world->m_drawTarget->m_frontPair->m_surface;
            mapHost->ShadeRect(0x32, NULL);
            gameReg = g_gameReg;
        }

        if (gameReg->m_gameMode != GAMEMODE_MULTIPLAYER && gameReg->m_loadingSaveGame == 0) {
            CString scr;
            self->m_inGame = 1;
            self->m_hudSuppressed = 0;
            RECT rect;
            rect.left = 0;
            rect.top = 0;
            rect.right = SCREEN_W_PX;
            rect.bottom = SCREEN_H_PX;
            if (scr.LoadString(IDS_CONTINUE_PROMPT)) {
                EngStr_DrawText(self->m_world, &scr, &rect, 0x78, 1, 0xff, 0xff, 0, 1);
            }
        } else {
            self->m_hudSuppressed = 1;
        }

        self->m_scrollEdgeLock = 0;
        self->m_levelOverlayOpen = 0;
        self->m_paused = 0;
        self->m_playerCommandPending = 0;
        self->m_winLoseBanner = 0;
        self->m_cueTiming.m_interval.m_lo = 0x1f4;
        self->m_cueTiming.m_interval.m_hi = 0;
        self->m_cueTiming.m_start.m_lo = g_frameTime;
        self->m_cueTiming.m_start.m_hi = 0;
        self->m_cueToggle = 1;
        self->m_cueText = "";
        self->m_lastCueId = 0;
        self->m_region0Gate = 0;
        self->m_region1Gate = 0;
        self->m_region2Gate = 0;
        self->m_region3Gate = 0;
        self->m_defeatCountdownActive = 0;
        self->m_focusPlayerIndex = 3;
        self->m_renderDisabled = 1;
        g_playActive = 0;
        ResetViewport();
        if ((g_gameReg)->m_gameMode == GAMEMODE_MULTIPLAYER) {
            g_playActive = 1;
            self->m_renderDisabled = 0;
            self->m_mgr->CheckSavedMode();
            self->m_mgr->m_chatLog->FreeNodes();
        }
        return 1;
    }

fail0:
    return 0;
}

#undef PTR

// @early-stop
// Register choice only: retail routes the world pointer through eax and copies
// it into ecx for the virtual dispatch, cl loads straight into ecx.
RVA(0x000cb400, 0x58)
void CPlay::OnExit() {
    ForwardReady();
    FreeListTeardown();
    if (m_world) {
        m_world->m_childGroup->ClearChildren();
    }
    g_gameReg->m_isBattlezLevel = 0;
    if (g_gameReg->m_gameMode == GAMEMODE_REPLAY) {
        g_gameReg->m_gameMode = GAMEMODE_NONE;
    }
    g_gameReg->m_tileGrid->Reset();
}

RVA(0x000cb480, 0x22c)
void CPlay::FreeListTeardown() {
    i32 i;
    i32 k;
    if (m_world == NULL) {
        return;
    }
    if (m_mgr == NULL) {
        return;
    }
    if (m_mgr->m_cmdGrid != NULL) {
        m_mgr->m_cmdGrid->RemovePlayerUnitsImmediately(TM_ALL_PLAYERS);
    }
    ForwardReady();
    {

        SoundCueRegistry* reg = m_world->m_soundRegistry;
        if (reg->m_soundStream != NULL) {
            reg->m_soundStream->StopAllStreams();
        }
    }
    m_mgr->m_midi->ClearSequences();
    m_mgr->m_worldSounds->Teardown();
    m_mgr->m_voiceManager->ClearVoiceIndicatorSlots();
    g_gameReg->m_cmdGrid->DestroyAllAnims();
    m_world->m_level->ReleaseChildren();
    (m_world->m_childGroup)->PruneList();
    if (m_guts != NULL) {
        m_guts->ResetWidgets(0);
    }
    if (m_beginMarker != NULL) {
        m_beginMarker->RemoveAll();
    }
    if (m_frameMarker != NULL) {
        m_frameMarker->Reset();
    }
    m_scrollSink = NULL;
    m_mgr->m_cmdGrid->OverlayTick();
    CTriggerMgr* tl68 = m_mgr->m_cmdGrid;

    tl68->m_byteArr.SetSize(0, -1);
    tl68->m_groupInitialized = 0;
    m_mgr->m_cmdGrid->m_baseList.RemoveAll();
    m_mgr->m_cmdGrid->m_pendingFx = NULL;
    (static_cast<CDDrawWorkerList*>(m_world->m_workerList))->ClearWorkers();
    for (i = 0; i < StartMarkerCount(); i++) {
        Coord* node = StartMarkerAt(i);
        if (node != NULL) {
            CoordPoolNode* p = g_coordPool.NodeOf(node);
            p->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = p;
        }
    }
    m_startMarkers.SetSize(0, -1);
    for (k = 0; k < 4; k++) {
        for (i = 0; i < PlacedObjectCellCount(k); i++) {
            Coord* node = PlacedObjectCellAt(k, i);
            if (node != NULL) {
                CoordPoolNode* p = g_coordPool.NodeOf(node);
                p->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = p;
            }
        }
        m_placedObjectCells[k].SetSize(0, -1);
    }
    for (i = 0; i < CameraBookmarkCount(); i++) {
        Coord* node = CameraBookmarkAt(i);
        if (node != NULL) {
            CoordPoolNode* p = g_coordPool.NodeOf(node);
            p->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = p;
        }
    }
    m_cameraBookmarks.SetSize(0, -1);
    for (i = 0; i < 4; i++) {
        m_mgr->m_options[i].m_battlezConfig.FreeArrays();
        m_mgr->m_options[i].m_battlezConfig.Clear();
    }
    m_cameraBookmarkIndex = -1;
}

RVA(0x000cb740, 0x8f)
void CPlay::ModeCleanup() {
    if (m_world) {
        {

            SoundCueRegistry* reg = m_world->m_soundRegistry;
            if (reg->m_soundStream) {
                reg->m_soundStream->StopAllStreams();
            }
        }
        m_world->m_soundRegistry->ClearCues();
    }
    if (m_mgr) {
        m_mgr->m_midi->ClearSequences();

        m_mgr->m_worldSounds->Teardown();
    }
    if (m_world) {
        m_world->m_imageRegistry->MapTeardown();
    }
    if (m_world) {
        m_world->m_animRegistry->ClearAnimations();
    }
    if (m_world) {
        m_world->m_level->ReleaseChildren();
    }
    if (m_world) {
        m_world->m_childGroup->ClearChildren();
    }
    if (m_world) {
        m_world->m_workerList->ClearWorkers();
    }
}

RVA(0x000cb800, 0x191)
i32 CPlay::InputVirtual() {
    if (!CState::InputVirtual()) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0)
        ;

    CSymTab* h = m_levelBank->ResolvePath("TILEZ");
    if (!h) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(h, "", "_") == -1) {
        return 0;
    }

    h = m_levelBank->ResolvePath("IMAGEZ");
    if (!h) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(h, "LEVEL", "_") == -1) {
        return 0;
    }

    h = m_gruntzBank->ResolvePath("IMAGEZ");
    if (!h) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(h, "GRUNTZ", "_") == -1) {
        return 0;
    }

    g_inputMgr->ReadAll();
    while (ShowCursor(FALSE) >= 0)
        ;

    m_world->m_drawTarget->m_backPair->m_surface->Fill(0);
    UpdateMgrScroll(g_gameReg, m_guts, m_region0Gate);

    if (m_region1Gate != 0) {
        NotifyVisibleEntities();
    } else {
        m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
        m_world->m_workerList->RenderAndPruneWorkers(
            m_world->m_drawTarget->m_backPair,
            m_world->m_drawTarget->m_overlayPair
        );
    }

    m_guts->Deactivate();
    m_guts->LoadMainStatusBarSprite();
    m_stepCountdown = 2;
    m_world->m_drawTarget->TransTitle();
    RetireScene(0x50, 0x3e8, 0, 1);
    return 1;
}

// @early-stop
// The four mode-size scalars hold each other's registers: retail hands them out in
// pure rotation order after `this`/`m_mgr` take esi/ecx (eax, edx, edi, ebx), ours
// enters the same rotation one slot later (eax, ebx, edx, edi), so the cursor phase
// differs rather than the request order.  Both declaration orders were measured and
// emit byte-identical code, which also bounds P1 of
// docs/relevations/cl5-c2-register-picker-is-a-rotating-cursor.md: feeding a call's
// argument pushes freezes the pick even though the values die at the call.
RVA(0x000cba10, 0xb0)
i32 CPlay::RestoreDisplay() {
    if (IsActive() == 0) {
        return 0;
    }
    i32 savedW = m_mgr->m_savedModeSize.cx;
    i32 liveW = m_mgr->m_modeSize.cx;
    i32 savedH = m_mgr->m_savedModeSize.cy;
    i32 liveH = m_mgr->m_modeSize.cy;
    if (savedW != liveW || savedH != liveH) {
        if (m_mgr->SetVideoMode(savedW, savedH, 1) == 0) {
            return 0;
        }
    }
    if (m_guts != NULL) {
        m_guts->Deactivate();
        if (m_region1Gate != 0) {
            NotifyVisibleEntities();
        } else {
            m_world->m_level->VisitVisible(
                m_world->m_drawTarget->m_backPair,
                m_world->m_childGroup
            );
            m_world->m_workerList->RenderAndPruneWorkers(
                m_world->m_drawTarget->m_backPair,
                m_world->m_drawTarget->m_overlayPair
            );
        }
        m_world->m_drawTarget->m_frontPair->m_surface->Flip(NULL);
    }
    return 1;
}

RVA(0x000cbaf0, 0x16f)
i32 CPlay::OnChar(i32 charCode, i32 keyData) {
    if (m_hudSuppressed != 0) {
        return 1;
    }
    if (m_renderDisabled != 0) {
        m_renderDisabled = 0;
        m_hudSuppressed = 1;
        EnterMode(GAMESTATE_PLAY);
        m_inGame = 1;
        return 1;
    }
    if (m_inGame != 0) {

        if (ResetPlayState() == 0) {
            m_mgr->ReportError(IDX(IDS_INITIALIZE_GAME), 0x456);
        }
        return 1;
    }
    if (m_paused != 0) {
        m_paused = 0;
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_FINISH_LEVEL), 0);
        return 1;
    }

    if (m_mgr->m_frameGate == 0) {
        if (m_hitTest->m_inputActive != 0) {
            m_mgr->m_chatLog->HandleInputChar(charCode, keyData);
            return 1;
        }
        if (charCode == ']') {
            m_guts->DockStatusBarRight();
            return 1;
        }
        if (charCode == '[') {
            m_guts->DockStatusBarLeft();
            return 1;
        }
        if (charCode == '-') {
            m_guts->HideRect();
            return 1;
        }
        if (charCode == '=' || charCode == '+') {
            m_guts->RestoreStatusBar();

            if (m_guts->m_position == STATUSBAR_DOCK_LEFT) {
                m_hitTest->Configure(CHATBOX_WITH_LEFT_STATUSBAR);
            } else {
                m_hitTest->Configure(CHATBOX_WITH_RIGHT_STATUSBAR);
            }
            return 1;
        }
    }
    return 0;
}

// @early-stop
// The residue is ordering. Two things dominate: cl materialises both loop constants in the prologue (the
// zero and the 1), so the five leading guards read `cmp [x],reg` where retail,
// which materialises each at its first use, reads `mov eax,[x]` / `test eax,eax`;
// and cl cross-jumps the 'Y' and 'Q' arms' identical CLEAR_TAB_HINT + phase-check
// prefix into one copy where retail emits both (retail 374 blocks, we have 371).
//
// MEASURED AND REJECTED, do not repeat: retail's frame is 0x10 = one 16-byte RECT
// and it spills only .top/.bottom, 8 apart at [esp+0x14] and [esp+0x1c], which
// reads exactly like a whole-struct `LevelCoordRect r = ...->m_planeCtx;` local at
// both bounds tests - and it is not.  Writing it that way (with `m_mgr`/`m_guts`
// inlined so cl stops spilling them, which does give retail's `sub esp,0x10`)
// drops the instruction count further below retail's and the in-order agreement with it.
// The four scalars are closer everywhere else, so they stay and the frame does not
// match.  See docs/patterns/whole-struct-copy-vs-scalars.md.  Hoisting the four
// bounds to function scope does NOT buy the frame either: cl sizes it by SPILL
// count, not declaration count, and still spills only two dwords.
RVA(0x000cbcc0, 0x17c0)
i32 CPlay::OnKeyDown(i32 vk, i32 lparam) {
    if (this->m_hudSuppressed != 0) {
        return 1;
    }
    if (this->m_renderDisabled != 0) {
        return 1;
    }
    if (this->m_inGame != 0) {
        return 1;
    }
    if (this->m_paused != 0) {
        return 1;
    }
    if (this->m_mgr->m_frameGate != 0) {
        return 1;
    }

    CGruntzMgr* host = this->m_mgr;
    CStatusBarMgr* level = this->m_guts;

    if (level->m_levelOverlayActive != 0 || level->m_quitConfirmationActive != 0) {
        if (level->m_quitConfirmationActive != 0) {

            if (vk == 'Y' || vk == VK_RETURN) {
                if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    if (g_gameReg->m_cmdGrid->m_phase == FINISH_STATE_VICTORY) {
                        g_gameReg->UpdateScoreHud();
                    }
                    PostMessageA(host->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
                    return 1;
                }
                CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                host->AccrueScoreTime();
                return 1;
            }
            if (vk == 'N' || vk == VK_ESCAPE) {
                CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                this->CloseLevelOverlay(0);
                return 1;
            }

        } else {

            if (vk == 'Q') {
                if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    if (g_gameReg->m_cmdGrid->m_phase == FINISH_STATE_VICTORY) {
                        g_gameReg->UpdateScoreHud();
                    }
                    PostMessageA(host->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
                }
                return 1;
            }

            if (vk == 'S' && g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                host->AccrueScoreTime();
            }
            if (vk == 'R') {
                if (host->m_gameMode == GAMEMODE_SINGLE
                    && g_gameReg->m_cmdGrid->m_phase != FINISH_STATE_VICTORY) {
                    // g_gameReg, not m_mgr: retail reads m_world off the global here
                    // (0xcbf65 `mov eax,[ecx+0x30]` with ecx = ds:0x64556c) while the
                    // gameMode test above it still comes from m_mgr.  Same for the 'N'
                    // and 'O' arms below; the five arms before them do use m_mgr.
                    CLEAR_TAB_HINT(g_gameReg->m_world->m_soundRegistry);
                    CGameWnd* r = g_gameReg->m_gameWnd;
                    PostMessageA(r->m_hwnd, WM_COMMAND, IDX(CMD_RELOAD_LEVEL), 0);
                }
                return 1;
            }
            if (vk == 'N') {
                if (host->m_gameMode == GAMEMODE_SINGLE
                    && g_gameReg->m_cmdGrid->m_phase == FINISH_STATE_VICTORY) {
                    CLEAR_TAB_HINT(g_gameReg->m_world->m_soundRegistry);
                    host->AccrueScoreTime();
                }
                return 1;
            }
            if (vk == 'O') {
                if (host->m_gameMode != GAMEMODE_SINGLE
                    && this->m_guts->m_observerTabAvailable != 0) {
                    CLEAR_TAB_HINT(g_gameReg->m_world->m_soundRegistry);
                    this->CloseLevelOverlay(0);
                }
                return 1;
            }
        }
    }

    if (vk == VK_RETURN) {
        CChatBoxOwner* rec = this->m_hitTest;
        if (rec->m_inputActive != 0) {
            rec->HandleTextInputKey('\r', lparam);
        } else {
            rec->m_fontConfig->EndInput();
            rec->m_inputActive = 1;
            this->m_hitTest->HandleTextInputKey('\r', lparam);
        }
        return 1;
    }

    if (vk == VK_ESCAPE) {
        CTriggerMgr* h68 = host->m_cmdGrid;
        CWwdGameObjectA* n = h68->m_goal;
        if (n != NULL) {
            n->m_flags |= 0x10000;
            h68->m_goal = NULL;
        }
        h68->m_armed = 0;
        CChatBoxOwner* rec = this->m_hitTest;
        if (rec->m_inputActive != 0) {
            this->FlushPendingOps();
            this->m_hitTest->m_fontConfig->EndInput();
            this->m_hitTest->m_inputActive = 0;
            return 1;
        }
        if (this->FlushPendingOps() != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(g_gameReg->m_world->m_soundRegistry);
        if (g_gameReg->m_frameGate != 0) {
            g_gameReg->m_frameGate ^= 1;
            g_gameReg->FinishLevel(g_gameReg->m_frameGate, 1);
        }
        this->OpenLevelOverlay(1);
        return 1;
    }

    if (this->m_hitTest->m_inputActive != 0) {
        return 1;
    }
    if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
        return 1;
    }

    if (vk == VK_TAB) {
        i32 idx = this->m_focusPlayerIndex;
        i32 pick;
        GruntzPlayer* area;
        if (g_gameplayInput->m_heldButtons & 1) {
            pick = idx - 1;
            if (pick < 0) {
                pick = 3;
            }
            area = &g_gameReg->m_options[pick];
            while (pick != idx) {
                if (area->m_joined == 0 || (area->m_doneFlag == 0 && area->m_clearedRound == 0)) {
                    break;
                }
                pick--;
                if (pick < 0) {
                    pick = 3;
                }
                area = &g_gameReg->m_options[pick];
            }
        } else {
            pick = idx + 1;
            if (pick >= 4) {
                pick = 0;
            }
            area = &g_gameReg->m_options[pick];
            while (pick != idx) {
                if (area->m_joined == 0 || (area->m_doneFlag == 0 && area->m_clearedRound == 0)) {
                    break;
                }
                pick++;
                if (pick >= 4) {
                    pick = 0;
                }
                area = &g_gameReg->m_options[pick];
            }
        }
        if (area->m_joined != 0 && area->m_doneFlag == 0 && area->m_clearedRound == 0) {
            this->m_focusPlayerIndex = pick;
            this->ResetGoals(area->m_focusX, area->m_focusY);
        }
    }

    if (vk == 'H') {
        GruntzPlayer* a = &g_gameReg->m_options[g_curPlayer];
        if (a == NULL) {
            return 1;
        }
        this->ResetGoals(a->m_focusX, a->m_focusY);
        return 1;
    }

    if (vk == 'Q') {
        if ((g_gameplayInput->m_heldButtons & 0x20) == 0) {
            return 1;
        }
        CGruntzMgr* h = this->m_mgr;
        if (h->m_frameGate != 0) {
            h->m_frameGate ^= 1;
            this->m_mgr->FinishLevel(h->m_frameGate, 1);
        }
        CLEAR_TAB_HINT(this->m_mgr->m_world->m_soundRegistry);
        this->OpenLevelOverlay(1);
        return 1;
    }

    if (vk == 'Z') {
        g_gameReg->m_cmdGrid->EnqueueGroupCells();
        return 1;
    }

    if (vk == 'C') {
        g_gameReg->m_cmdGrid->CenterOnGroup(g_gameplayInput->m_heldButtons & 0x20);
        return 1;
    }

    if (vk == 'T') {
        this->FlushPendingOps();
        g_gameReg->m_cmdGrid->ToggleRegionA();
        return 1;
    }

    if (vk == 'Y') {
        this->FlushPendingOps();
        g_gameReg->m_cmdGrid->ToggleRegionB();
        return 1;
    }

    if (vk == VK_SPACE) {
        if (g_gameplayInput->m_heldButtons & 0x20) {
            CDDrawWorkerHost* obj = this->m_world->m_level->m_mainPlane;
            i32 v0 = obj->m_snappedX;
            i32 v1 = obj->m_snappedY;
            Coord* slot;
            if (this->CameraBookmarkCount() < 4) {
                CoordPoolNode* head = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
                CoordPoolNode* nx = head->m_next;
                if (nx != NULL) {
                    slot = &head->m_coord;
                    g_coordPool.m_freeHead = nx;
                } else {
                    slot = NULL;
                }
            } else {

                slot = this->CameraBookmarkAt(0);
                this->m_cameraBookmarks.RemoveAt(0, 1);
                i32 c = this->m_cameraBookmarkIndex - 1;
                this->m_cameraBookmarkIndex = c;
                if (c < 0) {
                    this->m_cameraBookmarkIndex = this->CameraBookmarkCount() - 1;
                }
            }
            slot->m_x = v0;
            slot->m_y = v1;
            if (this->m_cameraBookmarkIndex != this->CameraBookmarkCount() - 1) {
                this->m_cameraBookmarks.InsertAt(this->m_cameraBookmarkIndex + 1, slot, 1);
                this->m_cameraBookmarkIndex = this->m_cameraBookmarkIndex + 1;
                return 1;
            }
            this->m_cameraBookmarks.SetAtGrow(this->CameraBookmarkCount(), slot);
            this->m_cameraBookmarkIndex = this->m_cameraBookmarkIndex + 1;
            return 1;
        }
        if (this->CameraBookmarkCount() == 0) {
            return 1;
        }
        if (g_gameplayInput->m_heldButtons & 1) {
            i32 c = this->m_cameraBookmarkIndex - 1;
            this->m_cameraBookmarkIndex = c;
            if (c < 0) {
                this->m_cameraBookmarkIndex = this->CameraBookmarkCount() - 1;
            }
        } else {
            i32 c = this->m_cameraBookmarkIndex + 1;
            this->m_cameraBookmarkIndex = c;
            if (c >= this->CameraBookmarkCount()) {
                this->m_cameraBookmarkIndex = 0;
            }
        }
        Coord* e = this->CameraBookmarkAt(this->m_cameraBookmarkIndex);
        this->ResetGoals(e->m_x, e->m_y);
        return 1;
    }

    if (vk == VK_BACK) {
        if (this->CameraBookmarkCount() <= 0) {
            return 1;
        }
        i32 cur = this->m_cameraBookmarkIndex;
        if (cur < 0) {
            return 1;
        }
        CoordPoolNode* node = g_coordPool.NodeOf(this->CameraBookmarkAt(cur));
        this->m_cameraBookmarks.RemoveAt(cur, 1);
        node->m_next = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
        g_coordPool.m_freeHead = node;
        i32 c = this->m_cameraBookmarkIndex - 1;
        this->m_cameraBookmarkIndex = c;
        if (c != -1) {
            return 1;
        }
        if (this->CameraBookmarkCount() == 0) {
            return 1;
        }
        this->m_cameraBookmarkIndex = this->CameraBookmarkCount() - 1;
        return 1;
    }

    if (vk == 'M' && (g_gameplayInput->m_heldButtons & 0x20)) {
        g_gameReg->SetMusicEnabled(g_gameReg->m_musicEnabled == 0);
        return 1;
    }

    if (vk == 'V' && (g_gameplayInput->m_heldButtons & 0x20)) {
        g_gameReg->m_isVoiceEnabled = (g_gameReg->m_isVoiceEnabled == 0);
        return 1;
    }

    if (vk == 'A') {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        CStatusBarMgr* lv = this->m_guts;
        if (lv->m_hlBusy != 0) {
            return 1;
        }
        if (lv->m_position == STATUSBAR_HIDDEN) {
            lv->RestoreStatusBar();
        }
        if (lv->m_activeTab != TAB_GRUNTZ) {
            lv->SetTabState(SBICMD_TAB_GRUNTZ, MENUITEM_SELECTED);
            lv->Deactivate();
        } else {
            lv->Deactivate();
        }
        return 1;
    }

    if (vk == 'S') {
        if (g_gameplayInput->m_heldButtons & 0x20) {
            g_gameReg->SetSoundEnabled(g_gameReg->m_soundEnabled == 0);
            return 1;
        }
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        CStatusBarMgr* lv = this->m_guts;
        if (lv->m_hlBusy != 0) {
            return 1;
        }
        if (lv->m_position == STATUSBAR_HIDDEN) {
            lv->RestoreStatusBar();
        }
        if (lv->m_activeTab != TAB_RESOURCE) {
            lv->SetTabState(SBICMD_TAB_RESOURCE, MENUITEM_SELECTED);
            lv->Deactivate();
        } else {
            lv->Deactivate();
        }
        return 1;
    }

    if (vk == 'D') {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        CStatusBarMgr* lv = this->m_guts;
        if (lv->m_hlBusy != 0) {
            return 1;
        }
        if (lv->m_position == STATUSBAR_HIDDEN) {
            lv->RestoreStatusBar();
        }
        if (lv->m_activeTab != TAB_STATZ) {
            lv->SetTabState(SBICMD_TAB_STATZ, MENUITEM_SELECTED);
            lv->Deactivate();
        } else {
            lv->Deactivate();
        }
        return 1;
    }

    if (vk == 'F') {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        this->m_guts->AdvanceTab(g_gameplayInput->m_heldButtons & 1);
        return 1;
    }

    if (vk == 'G') {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        CStatusBarMgr* lv = this->m_guts;
        if (lv->m_hlBusy != 0) {
            return 1;
        }
        if (lv->m_position == STATUSBAR_HIDDEN) {
            lv->RestoreStatusBar();
        }
        if (lv->m_activeTab != TAB_GAME) {
            lv->SetTabState(SBICMD_TAB_GAME, MENUITEM_SELECTED);
        }
        lv->SetTab(GAME_TAB_MENU, 1);
        lv->Deactivate();
        return 1;
    }

    if (lparam & 0x1000000) {
        if (vk == VK_LEFT) {
            this->m_scrollEdgeLock |= 1;
            return 1;
        }
        if (vk == VK_RIGHT) {
            this->m_scrollEdgeLock |= 4;
            return 1;
        }
        if (vk == VK_UP) {
            this->m_scrollEdgeLock |= 2;
            return 1;
        }
        if (vk == VK_DOWN) {
            this->m_scrollEdgeLock |= 8;
            return 1;
        }
        if (vk == VK_INSERT || vk == VK_DELETE || vk == VK_HOME || vk == VK_END || vk == VK_PRIOR
            || vk == VK_NEXT) {
            return 1;
        }
    }

    if (vk == VK_NUMPAD1 || vk == VK_NUMPAD2 || vk == VK_NUMPAD3 || vk == VK_NUMPAD4
        || vk == VK_NUMPAD5 || vk == VK_NUMPAD6 || vk == VK_NUMPAD7 || vk == VK_NUMPAD8
        || vk == VK_NUMPAD9 || vk == VK_NUMLOCK || vk == VK_DIVIDE || vk == VK_MULTIPLY
        || vk == VK_HOME || vk == VK_END || vk == VK_PRIOR || vk == VK_NEXT || vk == VK_CLEAR
        || vk == VK_UP || vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT || vk == VK_INSERT
        || vk == VK_DELETE || vk == VK_DECIMAL) {
        goto recorder_place;
    }

    if (vk == 'I') {
        if (g_gruntCreation == 0) {
            return 1;
        }
        GruntzPlayer* a = &g_gameReg->m_options[g_curPlayer];
        if (a == NULL) {
            return 1;
        }
        if (g_gameReg->m_cmdGrid->m_unitCountByPlayer[g_curPlayer] >= a->m_comboSel) {
            return 1;
        }
        CGruntzMgr* h = this->m_mgr;
        i32 my = this->m_cursorY;
        LevelCoordRect* r = &h->m_world->m_level->m_planeCtx;
        i32 x0 = r->left;
        i32 y0 = r->top;
        i32 x1 = r->right;
        i32 y1 = r->bottom;
        i32 mx = this->m_cursorX;
        if (mx >= x1 || mx < x0 || my >= y1 || my < y0) {
            return 1;
        }
        // EnqueuePlaceGruntAtScreenPoint takes i32; retail emits no sign-extension anywhere in
        // OnKeyDown.  The i16 casts made cl re-read the members as signed WORDs
        // (`movsx eax,word ptr [esi+0x154]`), clipping the cursor position.
        h->m_cmdSubMgr->EnqueuePlaceGruntAtScreenPoint(1, g_curPlayer, mx, my, 0);
        return 1;
    }

    if (vk == 'P') {
        if (g_gooPuddlez == 0) {
            return 1;
        }
        if (g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER) {
            return 1;
        }
        CGruntzMgr* h = this->m_mgr;
        i32 mx = this->m_cursorX;
        CGameLevel* q = h->m_world->m_level;
        LevelCoordRect* r = &q->m_planeCtx;
        i32 x0 = r->left;
        i32 y0 = r->top;
        i32 x1 = r->right;
        i32 y1 = r->bottom;
        i32 my = this->m_cursorY;
        if (!(mx >= x1 || mx < x0 || my >= y1 || my < y0)) {
            CDDrawWorkerHost* g = q->m_mainPlane;
            RECT* view = &g->m_viewRect;
            i32 by = view->top - q->m_planeCtx.top + my;
            i32 bx = view->left - q->m_planeCtx.left + mx;
            host->m_cmdGrid->SpawnPuddle(bx, by, 0, 0, 1, 0x19);
        }
    }

    if (vk == VK_F9) {
        if (g_explosionz == 0) {
            return 1;
        }
        CGruntzMgr* h = this->m_mgr;
        i32 my = this->m_cursorY;
        CGameLevel* q = h->m_world->m_level;
        CDDrawWorkerHost* g = q->m_mainPlane;
        RECT* view = &g->m_viewRect;
        i32 by = ((view->top - q->m_planeCtx.top + my) & ~TILE_MASK_PX) + TILE_HALF_PX;
        i32 bx =
            ((this->m_cursorX - q->m_planeCtx.left + view->left) & ~TILE_MASK_PX) + TILE_HALF_PX;
        g_gameReg->m_cmdGrid->LoadExplosionSprites(bx, by, -1, 1);
        return 1;
    }

    if (vk == 'K') {
        if (g_gruntDestruction == 0) {
            return 1;
        }
        i32 playerIndex;
        i32 unitIndex;
        CGrunt* r = host->m_cmdGrid->ScreenToCell(
            this->m_cursorX,
            this->m_cursorY,
            &playerIndex,
            &unitIndex,
            TM_ALL_PLAYERS
        );
        if (r == NULL) {
            return 1;
        }
        host->m_cmdGrid->StartUnitDeath(playerIndex, unitIndex, DEATH_DROP, -1);
        return 1;
    }

    if (vk == '1') {
        if (g_gameplayInput->m_heldButtons & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(1);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(1);
        }
        return 1;
    }
    if (vk == '2') {
        if (g_gameplayInput->m_heldButtons & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(2);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(2);
        }
        return 1;
    }
    if (vk == '3') {
        if (g_gameplayInput->m_heldButtons & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(3);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(3);
        }
        return 1;
    }
    if (vk == '4') {
        if (g_gameplayInput->m_heldButtons & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(4);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(4);
        }
        return 1;
    }
    if (vk == '5') {
        if (g_gameplayInput->m_heldButtons & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(5);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(5);
        }
        return 1;
    }
    if (vk == '6') {
        if (g_gameplayInput->m_heldButtons & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(6);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(6);
        }
        return 1;
    }
    if (vk == '7') {
        if (g_gameplayInput->m_heldButtons & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(7);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(7);
        }
        return 1;
    }
    if (vk == '8') {
        if (g_gameplayInput->m_heldButtons & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(8);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(8);
        }
        return 1;
    }
    if (vk == '9') {
        if (g_gameplayInput->m_heldButtons & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(9);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(9);
        }
        return 1;
    }
    return 1;

recorder_place:

{
    if (this->m_playerCommandPending != 0) {
        return 1;
    }
    if (this->m_dragInhibit1 != 0) {
        this->m_dragInhibit1 = 0;
        this->m_guts->CommitSlot(0);
        this->SetCursorFrame(0);
        if (vk != VK_INSERT) {
            goto tail_default;
        }
        return 1;
    }
    if (this->m_dragInhibit2 == 0) {
        goto tail_default2;
    }
    i32 st = this->m_cursorFrame;
    StatusBarHighlightRow ph = this->m_guts->m_pendingHlRow;
    i32 lvl;
    if (st >= 0x22) {
        lvl = 2;
    } else {
        lvl = (st >= 0x17);
    }
    this->m_dragInhibit2 = 0;
    if (vk == VK_DELETE || vk == VK_DECIMAL) {
        level->ReportTab(st);
        this->SetCursorFrame(0);
        return 1;
    }
    level->EnterHlRow(0, st);
    this->SetCursorFrame(0);
    if (lvl == 0) {
        if (ph == STATUS_HL_ROW_CATEGORY) {
            if (vk != VK_NUMLOCK) {
                goto tail_default;
            }
            return 1;
        }
        if (ph == STATUS_HL_ROW_UPPER) {
            if (vk == VK_NUMPAD7) {
                return 1;
            }
            if (vk != VK_HOME) {
                goto tail_default;
            }
            return 1;
        }
        if (ph == STATUS_HL_ROW_MIDDLE) {
            if (vk == VK_NUMPAD4) {
                return 1;
            }
            if (vk != VK_LEFT) {
                goto tail_default;
            }
            return 1;
        }
        if (vk == VK_NUMPAD1) {
            return 1;
        }
        if (vk != VK_END) {
            goto tail_default;
        }
        return 1;
    }
    if (lvl == 1) {
        if (ph == STATUS_HL_ROW_CATEGORY) {
            if (vk != VK_DIVIDE) {
                goto tail_default;
            }
            return 1;
        }
        if (ph == STATUS_HL_ROW_UPPER) {
            if (vk == VK_NUMPAD8) {
                return 1;
            }
            if (vk != VK_UP) {
                goto tail_default;
            }
            return 1;
        }
        if (ph == STATUS_HL_ROW_MIDDLE) {
            if (vk != VK_CLEAR) {
                goto tail_default;
            }
            return 1;
        }
        if (vk == VK_NUMPAD2) {
            return 1;
        }
        if (vk != VK_DOWN) {
            goto tail_default;
        }
        return 1;
    }
    if (ph == STATUS_HL_ROW_CATEGORY) {
        if (vk != VK_MULTIPLY) {
            goto tail_default;
        }
        return 1;
    }
    if (ph == STATUS_HL_ROW_UPPER) {
        if (vk == VK_NUMPAD9) {
            return 1;
        }
        if (vk != VK_PRIOR) {
            goto tail_default;
        }
        return 1;
    }
    if (ph == STATUS_HL_ROW_MIDDLE) {
        if (vk == VK_NUMPAD6) {
            return 1;
        }
        if (vk != VK_RIGHT) {
            goto tail_default;
        }
        return 1;
    }
    if (vk == VK_NUMPAD3) {
        return 1;
    }
    if (vk != VK_NEXT) {
        goto tail_default;
    }
    return 1;
}

tail_default:

{
    g_gameReg->m_cmdGrid->m_pendingFxKind = 0;
    this->LoadCursorSprites(0, 0);
}
tail_default2:

    if (this->m_guts->m_hitTestDisabled != 0) {
        return 1;
    }
    {

        // Retail's arms run in NUMPAD LAYOUT order (1..9, NumLock, /, *, Ins) and
        // each numpad vk shares its arm with the nav vk on the same pad cell -
        // 21 case labels, 13 emitted bodies.
        CStatusBarMgr* lv = this->m_guts;
        switch (vk) {
            case VK_END:
            case VK_NUMPAD1:
                lv->HlClickGroup0(STATUS_HL_ROW_LOWER);
                return 1;
            case VK_DOWN:
            case VK_NUMPAD2:
                lv->HlClickGroup1(STATUS_HL_ROW_LOWER);
                return 1;
            case VK_NEXT:
            case VK_NUMPAD3:
                lv->HlClickGroup2(STATUS_HL_ROW_LOWER);
                return 1;
            case VK_LEFT:
            case VK_NUMPAD4:
                lv->HlClickGroup0(STATUS_HL_ROW_MIDDLE);
                return 1;
            case VK_CLEAR:
            case VK_NUMPAD5:
                lv->HlClickGroup1(STATUS_HL_ROW_MIDDLE);
                return 1;
            case VK_RIGHT:
            case VK_NUMPAD6:
                lv->HlClickGroup2(STATUS_HL_ROW_MIDDLE);
                return 1;
            case VK_HOME:
            case VK_NUMPAD7:
                lv->HlClickGroup0(STATUS_HL_ROW_UPPER);
                return 1;
            case VK_UP:
            case VK_NUMPAD8:
                lv->HlClickGroup1(STATUS_HL_ROW_UPPER);
                return 1;
            case VK_PRIOR:
            case VK_NUMPAD9:
                lv->HlClickGroup2(STATUS_HL_ROW_UPPER);
                return 1;
            case VK_NUMLOCK:
                lv->HlClickGroup0(STATUS_HL_ROW_CATEGORY);
                return 1;
            case VK_DIVIDE:
                lv->HlClickGroup1(STATUS_HL_ROW_CATEGORY);
                return 1;
            case VK_MULTIPLY:
                lv->HlClickGroup2(STATUS_HL_ROW_CATEGORY);
                return 1;
            case VK_INSERT:
                lv->ActivateSlot(-1);
                break;
        }
    }
    return 1;
}

#undef CLEAR_TAB_HINT

RVA(0x000cda70, 0x7a)
i32 CPlay::OnKeyUp(i32 key, i32 flags) {
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

// @early-stop
// Block topology is exact (101/101 blocks, every jcc target aligned, 66 branches,
// 22 rets) and the frame matches retail's 0x20.  Residue is the x/y parameter pair:
// both are live across BeginMinimapPan/PointInRect/EnqueueSingle and hold each
// other's callee-saved register, the rotating-cursor class - see
// docs/relevations/cl5-c2-register-picker-is-a-rotating-cursor.md.  Retail also
// re-reads y from [esp+0x3c] at every use where cl enregisters it.
// The `RECT* view = &cam->m_viewRect` local reproduces retail's 0xcdc0f
// `add eax,0x40` scroll conversion. Two pre-merge lanes measured the same
// spelling as a net loss (body recolor); the 2026-08-22 tree measures it a
// gain with the +0x40/+0x44 operand exclusives cleared - the earlier
// rejection was TU-state, not structure.
// MEASURED AND REJECTED: naming the `(char)g_curPlayer` argument as a `char` local -
// retail stages that byte through the dead first-parameter home (`mov byte
// [esp+0x40],cl` / `mov ecx,[esp+0x40]`), but a named local gets its own dword and
// puts the frame back to 0x24.
RVA(0x000cdb10, 0x80c)
i32 CPlay::OnLButtonDown(i32 eventArg, i32 x, i32 y) {
    i32 xr;
    i32 sx;
    i32 sy;

    if (m_hudSuppressed != 0) {
        return 1;
    }
    if (m_renderDisabled != 0) {
        m_hudSuppressed = 1;
        m_renderDisabled = 0;
        EnterMode(GAMESTATE_PLAY);
        m_inGame = 1;
        return 1;
    }
    if (m_inGame != 0) {
        if (ResetPlayState()) {
            goto ret1;
        }
        m_mgr->ReportError(IDX(IDS_INITIALIZE_GAME), 0x457);
        return 1;
    }
    if (m_paused != 0) {
        m_paused = 0;
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_FINISH_LEVEL), 0);
        return 1;
    }

    if (m_levelOverlayOpen != 0 || g_gameReg->m_cmdGrid->m_groupFlag == 0) {
        return m_guts->UpdateStatusBarTabHighlight(eventArg, x, y);
    }

    xr = x;
    if (m_mgr->m_frameGate == 0) {
        if (m_lightFx != NULL && m_guts->m_position != STATUSBAR_HIDDEN
            && m_guts->m_activeTab != TAB_GAME) {
            if (m_lightFx->BeginMinimapPan(eventArg, xr, y)) {
                return 1;
            }
        }
        CGameLevel* geom = m_mgr->m_world->m_level;
        CDDrawWorkerHost* cam = geom->m_mainPlane;
        RECT* view = &cam->m_viewRect;
        sx = view->left - geom->m_planeCtx.left + xr;
        sy = view->top - geom->m_planeCtx.top + y;

        if (m_dragInhibit1 != 0 && m_playerCommandPending == 0) {
            // The "placed" flag IS the parameter reused: retail writes 0/1 into the
            // incoming `eventArg` slot at [esp+0x34] (0xcdc90 / 0xcdd1d) and reads it back
            // for both the voice-cue guard and CommitSlot.  A separate local costs
            // the extra dword that made the frame 0x24 instead of retail's 0x20.
            eventArg = 0;
            RECT* gr = &m_guts->m_barRect;
            if (CGameLevel::PointInRect(gr, xr, y)) {

            } else {
                // No copy: retail forms `&geom->m_planeCtx` (`add edx,0x10`) and reads
                // the four members straight off it, spilling only `top`; a whole-rect
                // local spills `left` as well and adds the dword that made the frame
                // 0x24 instead of retail's 0x20.
                if (CGameLevel::PointInRect(&geom->m_planeCtx, xr, y)) {
                    if (FindStartPointAt(sx, sy, &x, &y)) {
                        m_mgr->m_cmdSubMgr->EnqueueSingle(
                            1,
                            static_cast<char>(g_curPlayer),
                            0,
                            static_cast<char>(IDX(PLAYERCMD_PLACE_GRUNT)),
                            static_cast<i16>(x),
                            static_cast<i16>(y),
                            0,
                            0
                        );
                        eventArg = 1;
                    }
                }
            }
            if (eventArg == 0) {
                g_gameReg->m_voiceManager->PlayVoice(NULL, 0x340, -1, 1, -1, -1);
            }
            m_dragInhibit1 = 0;
            m_guts->CommitSlot(eventArg);
            SetCursorFrame(0);
            return 1;
        }

        if (m_dragInhibit2 != 0 && m_playerCommandPending == 0) {
            {
                RECT* gr = &m_guts->m_barRect;
                if (CGameLevel::PointInRect(gr, xr, y)) {
                    // No narrowing: retail loads the WHOLE dword (`mov eax,[esi+0x2f4]`),
                    // and DropFallingItemAt takes an i32. Casting to char clipped the held
                    // cursor item to its low signed byte before the status bar saw it.
                    if (m_guts->DropFallingItemAt(xr, y, m_cursorFrame)) {
                        m_dragInhibit2 = 0;
                        SetCursorFrame(0);
                        return 1;
                    }
                    goto waypoint_cancel;
                }
                CGameLevel* geom2 = m_mgr->m_world->m_level;
                RECT* wr = (&geom2->m_planeCtx);
                if (!CGameLevel::PointInRect(wr, xr, y)) {
                    goto waypoint_cancel;
                }

                CGameLevel* ds = m_world->m_level;
                LevelCoordRect* vr2 = &ds->m_mainPlane->m_viewRect;
                i32 wx = vr2->left - ds->m_planeCtx.left + xr;
                i32 wy = vr2->top - ds->m_planeCtx.top + y;
                if (g_gameReg->m_cmdGrid->CellHitTest(wx, wy, &eventArg, &y, g_curPlayer) != NULL) {
                    m_mgr->m_cmdSubMgr->EnqueueSingle(
                        1,
                        static_cast<char>(eventArg),
                        static_cast<char>(y),
                        static_cast<char>(IDX(PLAYERCMD_GIVE_TOOL)),
                        0,
                        0,
                        static_cast<char>(m_cursorFrame),
                        0
                    );
                    m_playerCommandPending = 1;
                    return 1;
                }

                RECT box;
                box.left = wx - 0xf;
                box.top = wy - 0xf;
                box.right = wx + 0xf;
                box.bottom = wy + 0xf;

                RECT span = {0, 0, 0, 0};
                CGrunt* p = g_gameReg->m_cmdGrid->FindGruntAt(wx, wy, &span, &eventArg, &y, &box);
                if (p == NULL || g_curPlayer != p->m_playerIndex) {
                    goto waypoint_cancel;
                }
                m_mgr->m_cmdSubMgr->EnqueueSingle(
                    1,
                    static_cast<char>(eventArg),
                    static_cast<char>(y),
                    static_cast<char>(IDX(PLAYERCMD_GIVE_TOOL)),
                    0,
                    0,
                    static_cast<char>(m_cursorFrame),
                    0
                );
                return 1;
            }

        waypoint_cancel:
            m_dragInhibit2 = 0;
            m_guts->EnterHlRow(0, m_cursorFrame);
            SetCursorFrame(0);
            return 1;
        }
    } else {
        sx = y;
        sy = y;
    }

    {

        if (m_guts == NULL) {
            return 1;
        }
        if (m_guts->m_position == STATUSBAR_HIDDEN) {
            if (m_guts->HitTestLayer(xr, y)) {
                m_dragSnapActive = 1;

                CGameObject* g8 = m_guts->m_barSprite;
                i32 dx = 0;
                if (g8 != NULL) {
                    dx = g8->m_screenX - xr;
                }
                m_snapOriginX = dx;
                CGameObject* g8b = m_guts->m_barSprite;
                if (g8b == NULL) {
                    m_snapOriginY = 0;
                    return 1;
                }
                m_snapOriginY = g8b->m_screenY - y;
                return 1;
            }
            goto drag_box;
        }

        RECT* gr = &m_guts->m_barRect;
        if (CGameLevel::PointInRect(gr, xr, y)) {
            FlushPendingOps();
            return m_guts->UpdateStatusBarTabHighlight(eventArg, xr, y);
        }
        if (m_hitTest->HitTest(xr, y)) {
            return 1;
        }
    }

drag_box: {
    if (m_mgr->m_frameGate != 0) {
        goto ret1;
    }
    LevelCoordRect wr = m_mgr->m_world->m_level->m_planeCtx;
    if (!(x < wr.right && x >= wr.left && y < wr.bottom)) {
        goto ret1;
    }
    if (y < wr.top) {
        return 1;
    }

    if (m_cursorTargetValid != 0) {
        i32 ex = (sx & ~TILE_MASK_PX) + TILE_HALF_PX;
        i32 ey = (sy & ~TILE_MASK_PX) + TILE_HALF_PX;
        i32 lv = m_cursorId - IDX(CURSOR_TOOL_HANDZ);
        PickupType item = static_cast<PickupType>(lv);
        if (item <= PICKUP_EQUIPPABLE_LAST) {
            g_gameReg->m_cmdGrid->ResetGroup(ex, ey, 0, 0, 0, TARGET_SELECTION_GRUNT, 1);
        } else if (item >= PICKUP_TOYZ_FIRST && item <= PICKUP_TOYZ_LAST) {
            g_gameReg->m_cmdGrid->ResetGroup(ex, ey, 0, 0, 0, TARGET_SELECTION_TOY, 1);
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
    if (g_gameReg->m_cmdGrid->TriggerCell(sx, sy)) {
        return 1;
    }

    if (m_cursorId >= IDX(CURSOR_TOOL_HANDZ)) {
        CTriggerMgr* cg = g_gameReg->m_cmdGrid;
        CGrunt* slot;
        if (1 != cg->m_recList.GetCount()) {
            slot = NULL;
        } else {
            i32* sel = static_cast<i32*>(cg->m_recList.GetHead());
            slot = cg->m_units[sel[0] * 15 + sel[1]];
        }
        if (slot != NULL && slot->m_entranceCommitted != 0) {
            g_gameReg->m_voiceManager->PlayVoice(slot, 0x324, -1, 0, -1, -1);
        }
    }
    LoadCursorSprites(0, 0);
    i32 hit = m_guts->HitTest(xr, y);
    if (hit != -1) {
        m_guts->PlaceCursorTarget(hit, 0);
        return 1;
    }

    // ScreenToCell reuses the dead input homes: eventArg becomes playerIndex and x becomes
    // unitIndex.
    CGrunt* picked =
        static_cast<CGrunt*>(m_mgr->m_cmdGrid->ScreenToCell(xr, y, &eventArg, &x, TM_ALL_PLAYERS));
    if (picked != NULL) {
        m_mgr->m_cmdGrid->ResetCell(eventArg, x, g_gameplayInput->m_heldButtons & 0x20, 0);
        if (eventArg == g_curPlayer) {
            if (g_gameplayInput->m_heldButtons & 0x20) {
                goto ret1;
            }
            picked->OnStruck(1);
            return 1;
        }
        picked->OnStruck(0);
        return 1;
    }
    m_dragClampMaxX = xr;
    m_dragClampMaxY = y;
    m_hudRect.left = xr;
    m_hudRect.right = xr;
    m_hudRect.top = y;
    m_hudRect.bottom = y;
    m_worldReady = 1;
    goto ret1;
}

ret1:
    return 1;
}

RVA(0x000ce530, 0xe3)
i32 CPlay::OnLButtonUp(i32 keyFlags, i32 x, i32 y) {
    if (m_hudSuppressed == 0) {
        if (m_lightFx != NULL && m_guts->m_position != STATUSBAR_HIDDEN
            && m_guts->m_activeTab != TAB_GAME) {
            m_lightFx->EndMinimapPan(keyFlags, x, y);
        }
        if (m_worldReady != 0) {
            m_mgr->m_cmdGrid->HudRect(m_hudRect, g_gameplayInput->m_heldButtons & 0x20);
        }
        m_worldReady = 0;
        m_dragSnapActive = 0;
        if (m_guts->m_position != STATUSBAR_HIDDEN) {
            LevelCoordRect vp = m_world->m_level->m_planeCtx;
            if (x < vp.left || x > vp.right || y < vp.top || y > vp.bottom) {
                return m_guts->OnPointerRelease(keyFlags, x, y);
            }
        }
    }
    return 1;
}

// @early-stop
RVA(0x000ce660, 0x362)
i32 CPlay::OnLButtonDblClk(i32 keyFlags, i32 x, i32 y) {
    if (m_hudSuppressed != 0 || m_guts == NULL) {
        return 1;
    }
    if (m_levelOverlayOpen != 0 || g_gameReg->m_cmdGrid->m_groupFlag == 0) {
        return m_guts->HandleDoubleClick(keyFlags, x, y);
    }
    if (m_dragInhibit1 != 0 || m_dragInhibit2 != 0) {
        return this->OnLButtonDown(keyFlags, x, y);
    }

    if (m_guts->m_position == STATUSBAR_HIDDEN && m_guts->HitTestLayer(x, y)) {
        SoundCueRegistry* set = m_mgr->m_world->m_soundRegistry;
        if (set->m_silentMode == 0) {
            SoundCue* e = NULL;
            MapLookup(set->m_cues, "GAME_TABHIGHLIGHT1", e);
            if (e != NULL) {
                e->PlayIfElapsed(g_soundVolumePercent, 0, 0, 0);
            }
        }
        m_guts->RestoreStatusBar();
        if (m_guts->m_position == STATUSBAR_DOCK_LEFT) {
            m_hitTest->Configure(CHATBOX_WITH_LEFT_STATUSBAR);
        } else {
            m_hitTest->Configure(CHATBOX_WITH_RIGHT_STATUSBAR);
        }
        return 1;
    }

    i32 idx = m_guts->HitTest(x, y);
    if (idx != -1) {
        m_guts->PlaceCursorTarget(idx, 1);
        return 1;
    }

    // Whole-struct copy, as in OnKeyDown: retail loads all four fields off one
    // materialised base (0xce6e6 `add eax,0x10` / `mov esi,eax`, then [esi],
    // [esi+4], [esi+8], [esi+0xc]) and compares against the REGISTER.
    RECT rc = m_world->m_level->m_planeCtx;
    if (x < rc.left || x > rc.right || y < rc.top || y > rc.bottom) {
        return m_guts->HandleDoubleClick(keyFlags, x, y);
    }

    {
        i32 playerIndex;
        i32 unitIndex;
        if (m_mgr->m_cmdGrid->ScreenToCell(x, y, &playerIndex, &unitIndex, TM_ALL_PLAYERS)
            && g_curPlayer == playerIndex) {
            m_guts->ToggleStat(unitIndex);
            return 1;
        }
    }

    if (m_dragInhibit1 != 0) {
        return 1;
    }
    CGameLevel* h;
    RECT* vr;
    i32 px;
    i32 py;
    i32 i;
    i32 area = g_curPlayer;
    GruntzPlayer* cfg = &g_gameReg->m_options[area];
    if (cfg == NULL || g_gameReg->m_cmdGrid->m_unitCountByPlayer[area] >= cfg->m_comboSel) {
        return 0;
    }

    h = m_mgr->m_world->m_level;
    vr = &h->m_mainPlane->m_viewRect;
    px = vr->left - h->m_planeCtx.left + x;
    py = vr->top - h->m_planeCtx.top + y;
    for (i = 0; i < StartMarkerCount(); i++) {
        Coord* e = StartMarkerAt(i);
        if (e == NULL) {
            continue;
        }
        RECT er;
        SetRect(&er, e->m_x - 0x10, e->m_y - 0x10, e->m_x + 0x10, e->m_y + 0x10);
        if (CGameLevel::PointInRect(&er, px, py)) {
            if (!m_guts->FindReadySlot()) {
                return 1;
            }
            char ab = static_cast<char>(g_curPlayer);
            px = (px & 0xffe0) + 0x10;
            py = (py & 0xffe0) + 0x10;
            m_mgr->m_cmdSubMgr->EnqueueSingle(
                1,
                ab,
                0,
                static_cast<char>(IDX(PLAYERCMD_PLACE_GRUNT)),
                px,
                py,
                0,
                0
            );
            return 1;
        }
    }
    return 1;
}

RVA(0x000ceab0, 0x17)
i32 CPlay::OnRButtonDblClk(i32 keyFlags, i32 x, i32 y) {
    return OnRButtonDown(keyFlags, x, y);
}

// @early-stop
// Two residues, both screened as unreachable from source:
//  - retail keeps its own `mov eax,1` + epilogue after the minimap-command arm
//    (six such copies survive) where cl folds that one into the shared return-1
//    tail. Cross-jumping merges common suffixes, so this is the partial-merge
//    THRESHOLD, the open residue of
//    docs/relevations/cl5-crossjump-merges-suffixes-not-blocks.md, not a blocker.
//  - the x/y parameter pair sits in ebx/edi with the roles swapped, and both are
//    live across IssueMinimapCommand/HitTest: the rotating-cursor class, see
//    docs/relevations/cl5-c2-register-picker-is-a-rotating-cursor.md.
// Retail forms `&m_viewRect` here (0xcec8d `add ecx,0x40`), but a `RECT*` local is
// NOT its source: re-measured against the build, it moves this function away.
RVA(0x000ceae0, 0x268)
i32 CPlay::OnRButtonDown(i32 keyFlags, i32 x, i32 y) {
    if (m_hudSuppressed != 0) {
        return 1;
    }
    if (m_renderDisabled != 0) {
        m_hudSuppressed = 1;
        m_renderDisabled = 0;
        EnterMode(GAMESTATE_PLAY);
        m_inGame = 1;
        return 1;
    }
    if (m_inGame != 0) {
        if (ResetPlayState()) {
            return 1;
        }
        m_mgr->ReportError(IDX(IDS_INITIALIZE_GAME), 0x458);
        return 1;
    }
    if (m_paused != 0) {
        m_paused = 0;
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_FINISH_LEVEL), 0);
        return 1;
    }
    if (m_levelOverlayOpen != 0) {
        return 1;
    }
    if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
        return 1;
    }
    if (m_mgr->m_frameGate != 0) {
        return 1;
    }
    if (m_lightFx != NULL && m_guts->m_position != STATUSBAR_HIDDEN
        && m_guts->m_activeTab != TAB_GAME && m_lightFx->IssueMinimapCommand(keyFlags, x, y)) {
        return 1;
    }

    if (CGameLevel::PointInRect(&m_guts->m_barRect, x, y)) {
        return 1;
    }
    i32 idx = m_guts->HitTest(x, y);
    if (idx != -1) {
        m_guts->ClearStat(idx);
        CTriggerMgr* w = m_mgr->m_cmdGrid;
        if (w->m_goal != NULL) {
            w->m_goal->m_flags |= 0x10000;
            w->m_goal = NULL;
        }
        w->m_armed = 0;
        return 1;
    }
    if (m_mgr->m_cmdGrid->m_recList.GetCount() == 0) {
        return 1;
    }
    CGameLevel* ph = m_mgr->m_world->m_level;
    LevelCoordRect pr = ph->m_planeCtx;
    if (CGameLevel::PointInRect(&pr, x, y)) {
        CGameLevel* ds = m_world->m_level;
        CDDrawWorkerHost* geom = ds->m_mainPlane;
        i32 rawX = geom->m_viewRect.left - ds->m_planeCtx.left + x;
        i32 rawY = geom->m_viewRect.top - ds->m_planeCtx.top + y;
        i32 snapX = (rawX & ~TILE_MASK_PX) + TILE_HALF_PX;
        i32 snapY = (rawY & ~TILE_MASK_PX) + TILE_HALF_PX;
        m_tileClick.m_x = snapX;
        m_tileClick.m_y = snapY;
        CTriggerMgr* w = m_mgr->m_cmdGrid;
        if (w->m_overlay != NULL && w->m_overlay->m_active != 0) {
            w->OverlayTick();
            return 1;
        }
        w->ResetGroup(snapX, snapY, rawX, rawY, 1, TARGET_SELECTION_AUTO, 1);
    }
    return 1;
}

RVA(0x000cedf0, 0xf)
i32 CGameLevel::ActivateVisibleObjectsOnMainPlane() {
    if (m_mainPlane != NULL) {
        return m_mainPlane->ActivateVisibleObjects();
    }
    return 0;
}

RVA(0x000cee10, 0xf)
i32 CGameLevel::DeactivateDistantObjectsOnMainPlane() {
    if (m_mainPlane != NULL) {
        return m_mainPlane->DeactivateDistantObjects();
    }
    return 0;
}

RVA(0x000cee30, 0x8)
i32 CPlay::OnRButtonUp(i32, i32, i32) {
    return 1;
}

// @identity-TODO: owner and no-op behavior are proven; the method identity is not.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000cee50, 0x1)
void CPlay::ResetRightClickState() {}

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
    if (m_guts != NULL) {
        m_guts->Deactivate();
    }
    return 1;
}

RVA(0x000cef50, 0x46)
i32 CPlay::QuitToMenu() {

    m_mgr->m_strWorldFile.Empty();
    if (m_completedFinalLevel != 0) {
        if (m_world->m_drawTarget->HasOverlay() != 0) {
            m_world->m_drawTarget->TransEnter();
        }
        m_mgr->PlayMovieEntry(3);
    }
    return 1;
}

RVA(0x000cefc0, 0xa2)
i32 CPlay::DrawWorldPresent() {

    {
        CGameLevel* lvl = m_world->m_level;
        if (lvl->m_mainPlane != NULL) {
            lvl->m_mainPlane->DeactivateDistantObjects();
        }
    }
    {
        CGameLevel* lvl = m_world->m_level;
        if (lvl->m_mainPlane != NULL) {
            lvl->m_mainPlane->ActivateVisibleObjects();
        }
    }
    m_world->m_childGroup->TickKillCues(1);
    {
        CGameLevel* lvl = m_world->m_level;
        if (lvl->m_mainPlane != NULL) {
            lvl->m_mainPlane->DeactivateDistantObjects();
        }
    }
    {
        CGameLevel* lvl = m_world->m_level;
        if (lvl->m_mainPlane != NULL) {
            lvl->m_mainPlane->ActivateVisibleObjects();
        }
    }
    m_world->m_childGroup->TickKillCues(1);
    m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
    m_world->m_workerList->RenderAndPruneWorkers(
        m_world->m_drawTarget->m_backPair,
        m_world->m_drawTarget->m_overlayPair
    );
    m_mgr->RefreshGameClock();
    return 1;
}

// @early-stop
// The only body residue is the dead source-RECT top word: retail spills it to
// [esp+0x30], while this build uses the soon-overwritten dr.top at [esp+0x20].
// Copy-initialized RECT forms change the live copy and are not equivalent; the
// complete parser-state forest leaves this dead-slot choice unchanged.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000cf0a0, 0x567)
void CPlay::DrawDebugStatsFull() {
    if (g_debugDisplayFlags & 0x20) {
        return;
    }

    char buf[0x200];
    char fpsScratch[0x40];
    char scratch[0x40];
    buf[0] = 0;

    sprintf(fpsScratch, "Fps = %i ", m_mgr->m_fps);
    strcat(buf, fpsScratch);

    CDDrawChildGroup* group = m_world->m_childGroup;
    if (group->m_flags & 0x10000) {
        strcat(buf, " rcHit ");
    }
    if (group->m_flags & 0x20000) {
        strcat(buf, " rcAttack ");
    }
    if (group->m_flags & 0x40000) {
        strcat(buf, " rcMove ");
    }
    if (group->m_flags & 0x100000) {
        strcat(buf, " ptOrg ");
    }
    if (group->m_flags & 0x200000) {
        strcat(buf, " Z = On");
    }

    if (g_debugDisplayFlags & 0x1) {
        sprintf(scratch, " Sprites = %i ", m_world->m_childGroup->m_list.GetCount());
        strcat(buf, scratch);
    }
    if (g_debugDisplayFlags & 0x4) {
        CDDrawWorkerHost* p = m_world->m_level->m_mainPlane;
        sprintf(scratch, " Pos = %i,%i", p->m_snappedX, p->m_snappedY);
        strcat(buf, scratch);
    }
    if (g_debugDisplayFlags & 0x80) {
        CString t = FormatElapsedTime(g_frameTime);
        t += DATA_COMPGEN(0x00212754, " ");
        strcat(buf, t);
        t += " ";
    }
    if (g_debugDisplayFlags & 0x2) {
        sprintf(
            scratch,
            " Sent = %i, Rcvd = %i, Frame = %i Counter = %lu",
            m_packetsSent,
            m_packetsRcvd,
            GetFrame(),
            g_frameTime
        );
        strcat(buf, scratch);
    }
    if (g_debugDisplayFlags & 0x200) {
        sprintf(scratch, " FpsLimit = %i ", m_mgr->m_pacingGate);
        strcat(buf, scratch);
    }

    CDDSurface* host = m_world->m_drawTarget->m_backPair->m_surface;
    HDC hdc = NULL;
    host->m_ddSurface->GetDC(&hdc);
    if (hdc == NULL) {
        return;
    }
    SetBkMode(hdc, 1);
    SetTextColor(hdc, 0xffffff);
    SetBkColor(hdc, 0);
    PostSetup(hdc);

    // Whole-struct copy: retail still writes `lr.top` to its own slot (0xcf4bd
    // `mov [esp+0x30],edx`) even though nothing reads it, which field-by-field
    // assignment does not survive - cl dead-stores the unread field.
    {
        RECT* src = &m_world->m_level->m_planeCtx;
        RECT lr = *src;
        RECT dr;
        dr.left = lr.left;
        dr.top = lr.bottom - 0x1c;
        dr.right = lr.right;
        dr.bottom = lr.bottom;
        DrawTextA(hdc, buf, -1, &dr, 0x20);
    }

    if (g_debugDisplayFlags & 0x8) {
        SetBkMode(hdc, 2);
        if (g_brickText1.GetLength() != 0) {
            TextOutA(hdc, 0, 0x00, g_brickText1, g_brickText1.GetLength());
        }
        if (g_brickText2.GetLength() != 0) {
            TextOutA(hdc, 0, 0x10, g_brickText2, g_brickText2.GetLength());
        }
        if (g_brickText3.GetLength() != 0) {
            TextOutA(hdc, 0, 0x20, g_brickText3, g_brickText3.GetLength());
        }
        if (g_brickText4.GetLength() != 0) {
            TextOutA(hdc, 0, 0x30, g_brickText4, g_brickText4.GetLength());
        }
        if (g_brickText5.GetLength() != 0) {
            TextOutA(hdc, 0, 0x40, g_brickText5, g_brickText5.GetLength());
        }
        if (g_brickText6.GetLength() != 0) {
            TextOutA(hdc, 0, 0x50, g_brickText6, g_brickText6.GetLength());
        }
        if (g_brickText7.GetLength() != 0) {
            TextOutA(hdc, 0, 0x60, g_brickText7, g_brickText7.GetLength());
        }
        if (g_brickText8.GetLength() != 0) {
            TextOutA(hdc, 0, 0x70, g_brickText8, g_brickText8.GetLength());
        }
    }
    host->m_ddSurface->ReleaseDC(hdc);
}

RVA(0x000cf770, 0x35e)
void CPlay::DrawDebugStats() {
    if (g_debugDisplayFlags & 0x20) {
        return;
    }

    char buf[0x200];
    char scratch[0x40];
    buf[0] = 0;

    if (g_debugDisplayFlags & 0x10) {
        sprintf(scratch, "Fps = %i ", m_mgr->m_fps);
        strcat(buf, scratch);
    }
    if (g_debugDisplayFlags & 0x1) {
        sprintf(scratch, " Objs = %i ", m_world->m_childGroup->m_list.GetCount());
        strcat(buf, scratch);
    }
    if (g_debugDisplayFlags & 0x4) {
        CDDrawWorkerHost* p = m_world->m_level->m_mainPlane;

        sprintf(scratch, " Pos = %i,%i", p->m_snappedX, p->m_snappedY);
        strcat(buf, scratch);
    }
    if (g_debugDisplayFlags & 0x40) {
        strcat(buf, " Timing = On ");
    }
    if (g_debugDisplayFlags & 0x80) {
        CString t = FormatElapsedTime(g_frameTime);
        t += " ";
        strcat(buf, t);
        t += " ";
    }
    if (g_debugDisplayFlags & 0x2) {
        sprintf(
            scratch,
            " Sent = %i, Rcvd = %i, Frame = %i Counter = %lu",
            m_packetsSent,
            m_packetsRcvd,
            GetFrame(),
            g_frameTime
        );
        strcat(buf, scratch);
    }

    CDDSurface* host = m_world->m_drawTarget->m_backPair->m_surface;
    HDC hdc = NULL;
    host->m_ddSurface->GetDC(&hdc);
    if (hdc == NULL) {
        return;
    }
    SetBkMode(hdc, 1);
    SetTextColor(hdc, 0xffffff);
    SetBkColor(hdc, 0);
    PostSetup(hdc);

    if (buf[0] != 0) {
        RECT lr;

        RecordBytes<RECT> reuse;
        reuse.m_chars = scratch;
        CopyRect(&lr, g_gameReg->GetRect(static_cast<RECT*>(reuse.m_rec)));
        RECT dr;
        dr.left = lr.left;
        dr.top = lr.bottom - 0x1c;
        dr.right = lr.right;
        dr.bottom = lr.bottom;
        if (lr.left > 0) {
            DrawTextA(hdc, buf, -1, &dr, 0x20);
        } else {
            TextOutA(hdc, 0, dr.top, buf, strlen(buf));
        }
    }
    host->m_ddSurface->ReleaseDC(hdc);
}

RVA(0x000cfbb0, 0x8)
void CPlay::TickStateMgrs() {
    m_mgr->TickStateMgrs();
}

RVA(0x000cfbd0, 0x8f)
i32 CPlay::CompleteLevel() {
    QuestLevel level = CurrentQuestLevel();
    if (level == QUESTLEVEL_CAMPAIGN_LAST) {
        m_completedFinalLevel = 1;
        m_notifyLatch = 1;

        SoundCueRegistry* reg = m_world->m_soundRegistry;
        if (reg->m_soundStream) {
            reg->m_soundStream->StopAllStreams();
        }
        m_mgr->m_midi->ClearSequences();
        m_mgr->m_worldSounds->Teardown();
        m_mgr->m_voiceManager->ClearVoiceIndicatorSlots();
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
        return 1;
    }
    if (m_returnToMenuOnComplete) {
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
        return 1;
    }
    m_mgr->Post(m_levelIndex + 1);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000cfc90, 0x1d1)
void CPlay::DrawCustomLevelBanner() {
    if (m_mgr->m_strWorldFile.IsEmpty()) {
        return;
    }
    {
        CString world = m_mgr->GetWorldFileName();
        if (world.IsEmpty()) {
            return;
        }
        CString base;
        if (m_mgr->m_isBattlezLevel == 0 && m_mgr->m_isMultiLevel == 0) {
            base = WwdFile::GetMapBaseName(world);
        } else {
            base = world;
        }
        if (base.IsEmpty()) {
            return;
        }
        sprintf(g_customLevelText, "Custom Level: %s", static_cast<const char*>(base));
    }
    CDDSurface* host = m_world->m_drawTarget->m_frontPair->m_surface;
    if (host == NULL) {
        return;
    }
    HDC hdc = NULL;
    host->m_ddSurface->GetDC(&hdc);
    if (hdc == NULL) {
        return;
    }
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, 0);
    RECT rc;
    rc.left = 0;
    rc.top = 0x1b8;
    rc.right = 0x27f;
    rc.bottom = 0x1d6;
    DrawTextA(hdc, g_customLevelText, -1, &rc, DT_CENTER | DT_SINGLELINE);
    host->m_ddSurface->ReleaseDC(hdc);
}

RVA(0x000cfef0, 0xbc)
i32 CPlay::DrawStateMessage() {
    Present(0x3c);

    CDDrawWorker* set = LookupWorker(m_world->m_imageRegistry->m_workersByName, "GAME_MESSAGEZ");
    if (set == NULL) {
        return 0;
    }

    i32 index = 3;
    if (Update() == GAMESTATE_DEMO) {
        index = 4;
    }
    CImage* frame = set->GetAt(index);
    if (frame == NULL) {
        return 0;
    }

    CDDrawSurfacePair* surf = m_world->m_drawTarget->m_backPair;
    if (surf == NULL) {
        return 0;
    }
    (static_cast<CImage*>(frame))->RenderFrame(surf, surf->m_width / 2, surf->m_height / 2, 0);
    m_world->m_drawTarget->m_frontPair->m_surface->Flip(static_cast<CDDSurface*>(0));
    return 1;
}

RVA(0x000cffe0, 0x3c)
i32 CPlay::LoadImageBanks() {
    CPlay* self = this;
    if (!self->m_symParser) {
        return 0;
    }
    self->m_gruntzBank = self->m_symParser->ResolvePath("GRUNTZ");
    if (!self->m_gruntzBank) {
        return 0;
    }
    self->m_gameBank = self->m_symParser->ResolvePath("GAME");
    return self->m_gameBank != NULL;
}

RVA(0x000d0050, 0x3a)
i32 CPlay::CountObjectsByCategory(i32 category) {
    CObList* container = &m_world->m_childGroup->m_list;
    if (container == NULL) {
        return 0;
    }
    POSITION pos = container->GetHeadPosition();
    i32 count = 0;
    while (pos != NULL) {
        CGameObject* sprite = static_cast<CGameObject*>(container->GetNext(pos));
        if (sprite != NULL && sprite->m_objectType == static_cast<u32>(category)) {
            count++;
        }
    }
    return count;
}

RVA(0x000d00a0, 0x5a)
void CPlay::PostSetup(HDC dc) {
    RECT src = *(&m_world->m_level->m_planeCtx);
    RECT dst;
    CopyRect(&dst, &src);
    m_mgr->m_chatLog->DrawTextLines(8, dc, &dst, 0x10);
}

#define SYNC_PAIR(ar, mode, p)                                                                     \
    if ((mode) != SERIAL_SAVE) {                                                                   \
        if ((mode) == SERIAL_LOAD) {                                                               \
            (ar)->Read((p), 8);                                                                    \
            (ar)->Read((p) + 2, 8);                                                                \
        }                                                                                          \
    } else {                                                                                       \
        (ar)->Write((p), 8);                                                                       \
        (ar)->Write((p) + 2, 8);                                                                   \
    }

// @early-stop
// Residue: cl constant-folds `cursorId` into the m_cursorId store inside the
// CURSOR_POINTER and CURSOR_FLAILINGGRUNT arms (it propagates the compared
// constant along the equality edge); retail re-reads the parameter home slot in
// every arm. Neither dropping the enum local nor comparing the raw parameter
// stops the fold.
RVA(0x000d0120, 0x65c)
i32 CPlay::LoadCursorSprites(i32 cursorId, i32 targetValid) {
    // Callers build the id by arithmetic (`<grunt kind> + CURSOR_TOOL_HANDZ`, the
    // chip index, ...), so the cursor domain is entered here.
    ToolCursorId cursor = static_cast<ToolCursorId>(cursorId);
    if (this->m_cursorId == cursorId && targetValid == this->m_cursorTargetValid) {
        return 1;
    }
    if (cursor >= CURSOR_CHIP_FIRST && cursor <= CURSOR_CHIP_LAST) {
        if (this->LoadCursorAnimation("GAME_INGAMEICONZ_NORMCHIPZ", cursorId, 0, 0x64, 0) == 0) {
            return 0;
        }
        if (this->m_scrollSink != NULL) {
            this->m_scrollSink->m_stateFlags |= SPRITE_STATE_HIDDEN;
        }
        this->m_cursorOffset.m_x = 0;
        this->m_cursorOffset.m_y = 0;
        this->m_dragInhibit2 = 1;
        this->m_cursorTargetValid = 0;
        this->m_cursorId = cursorId;
        return 1;
    }
    if (cursor == CURSOR_POINTER) {
        if (this->LoadCursorAnimation("GAME_CURSORZ_POINTER", 1, 1, 0x64, 0) == 0) {
            return 0;
        }
        if (this->m_scrollSink != NULL) {
            this->m_scrollSink->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
        }
        this->m_cursorOffset.m_x = 0x10;
        this->m_cursorOffset.m_y = 0x10;
        this->m_cursorTargetValid = 0;
        this->m_cursorId = cursorId;
        return 1;
    }
    if (cursor == CURSOR_FLAILINGGRUNT) {
        if (this->LoadCursorAnimation("GAME_CURSORZ_FLAILINGGRUNT", 1, 1, 0x64, 1) == 0) {
            return 0;
        }
        if (this->m_scrollSink != NULL) {
            this->m_scrollSink->m_stateFlags |= SPRITE_STATE_HIDDEN;
        }
        this->m_cursorOffset.m_x = 0;
        this->m_cursorOffset.m_y = 0;
        this->m_dragInhibit1 = 1;
        this->m_cursorTargetValid = 0;
        g_gameReg->m_voiceManager->PlayVoice(NULL, 0x33e, -1, 1, -1, -1);
        this->m_bootyTiming.m_interval.m_lo = BOOTY_INTERVAL_MS;
        this->m_bootyTiming.m_interval.m_hi = 0;
        this->m_bootyTiming.m_start.m_lo = g_frameTime;
        this->m_bootyTiming.m_start.m_hi = 0;
        this->m_cursorId = cursorId;
        return 1;
    }
    if (cursor < CURSOR_TOOL_FIRST) {
        return 0;
    }
    switch (cursor) {
        case CURSOR_TOOL_HANDZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_HANDZ", 1, targetValid, 0x64, 1) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_BOMBZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_BOMBZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_BOOMERANGZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_BOOMERANGZ", 1, targetValid, 0x64, 0)
                == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_BRICKZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_BRICKZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_CLUBZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_CLUBZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_GAUNTLETZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_GAUNTLETZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_GLOVEZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_GLOVEZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_GOOBERZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_GOOBERZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_GRAVITYBOOTZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_GRAVITYBOOTZ", 1, targetValid, 0x64, 0)
                == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_GUNHATZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_GUNHATZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_NERFGUNZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_NERFGUNZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_ROCKZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_ROCKZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_SHIELDZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_SHIELDZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_SHOVELZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_SHOVELZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_SPRINGZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_SPRINGZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_SPYZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_SPYZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_SWORDZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_SWORDZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_TIMEBOMBZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_TIMEBOMBZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_TOOBZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_TOOBZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_WANDZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_WANDZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_WARPSTONEZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_WARPSTONEZ", 1, targetValid, 0x64, 0)
                == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_WELDERZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_WELDERZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_WINGZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_WINGZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_BABYWALKERZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_BABYWALKERZ", 1, targetValid, 0x64, 0)
                == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_BEACHBALLZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_BEACHBALLZ", 1, targetValid, 0x64, 0)
                == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_BIGWHEELZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_BIGWHEELZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_GOKARTZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_GOKARTZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_JACKINTHEBOXZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_JACKINTHEBOXZ", 1, targetValid, 0x64, 0)
                == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_JUMPROPEZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_JUMPROPEZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_POGOSTICKZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_POGOSTICKZ", 1, targetValid, 0x64, 0)
                == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_SCROLLZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_SCROLLZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_SQUEAKTOYZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_SQUEAKTOYZ", 1, targetValid, 0x64, 0)
                == 0) {
                return 0;
            }
            break;
        case CURSOR_TOOL_YOYOZ:
            if (this->LoadCursorAnimation("GAME_CURSORZ_YOYOZ", 1, targetValid, 0x64, 0) == 0) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    if (this->m_scrollSink != NULL) {
        this->m_scrollSink->m_stateFlags |= SPRITE_STATE_HIDDEN;
    }
    this->m_cursorOffset.m_x = 0;
    this->m_cursorOffset.m_y = 0;
    this->m_cursorTargetValid = targetValid;
    this->m_cursorId = cursorId;
    return 1;
}

RVA(0x000d0920, 0xfe)
i32 CPlay::LoadCursorAnimation(
    const char* spriteKey,
    i32 initialFrame,
    i32 animate,
    i32 frameDelayMs,
    i32 tintForPlayer
) {
    if (m_world == NULL) {
        return 0;
    }
    CDDrawWorker* grid = LookupWorker(m_world, spriteKey);
    m_cursorSprite = grid;
    if (grid == NULL) {
        return 0;
    }
    m_cursorUsesPlayerTint = tintForPlayer;
    if (tintForPlayer != 0) {
        CGruntzMgr* w = m_mgr;
        i32 id = g_curPlayer;
        CShadeTable* spr = w->m_spriteFactory->GetSel(IDX(w->m_options[id].m_colorIndex), 0);
        if (spr == NULL) {
            spr = g_gameReg->m_spriteFactory->GetSel(1, 0);
        }
        m_cursorSprite->SetAllTypes(SHADE_PAL_16);
        m_cursorSprite->SetAllFormats(spr);
    }
    CDDrawWorker* g = m_cursorSprite;
    CImage* frame;
    if (DDRAW_WORKER_FRAME_IN_RANGE(g, initialFrame)) {
        frame = DDRAW_WORKER_FRAME_AT_UNCHECKED(g, initialFrame);
    } else {
        frame = NULL;
    }
    m_cursorImage = frame;
    if (frame == NULL) {
        return 0;
    }
    m_cursorFrameIndex = initialFrame;
    m_cursorAnimationActive = animate;
    m_cursorFrameDelayMs = frameDelayMs;
    m_cursorFrameCountdownMs = frameDelayMs;
    return 1;
}

RVA(0x000d0a60, 0x92)
i32 CPlay::AdvanceCursorAnimation(i32 elapsedMs) {
    if (m_cursorAnimationActive == 0) {
        return 1;
    }
    if (static_cast<u32>(m_cursorFrameCountdownMs) > static_cast<u32>(elapsedMs)) {
        m_cursorFrameCountdownMs = m_cursorFrameCountdownMs - elapsedMs;
    } else {
        m_cursorFrameCountdownMs = m_cursorFrameDelayMs;
        m_cursorFrameIndex = m_cursorFrameIndex + 1;
        i32 idx = m_cursorFrameIndex;
        CDDrawWorker* g = m_cursorSprite;
        CImage* frame;
        if (DDRAW_WORKER_FRAME_IN_RANGE(g, idx)) {
            frame = DDRAW_WORKER_FRAME_AT_UNCHECKED(g, idx);
        } else {
            frame = NULL;
        }
        m_cursorImage = frame;
        if (frame == NULL) {
            m_cursorImage = static_cast<CImage*>(g->m_items.GetAt(g->m_minIndex));
            m_cursorFrameIndex = g->m_minIndex;
        }
    }
    return 1;
}

// @early-stop
// sole residue: retail homes the 4-byte DDSCAPS in the dead `pair` parameter's slot
// ([esp+0x38]) where cl overlays it onto `half`'s local ([esp+0x10]); hoisting the
// declaration and the complete parser-state forest do not move it.
RVA(0x000d0b30, 0x200)
i32 CPlay::DrawCursorSaveUnder(CDDrawSurfacePair* pair) {
    i32 x = m_cursorX + m_cursorOffset.m_x;
    i32 y = m_cursorY + m_cursorOffset.m_y;

    CDDSurface* half;
    RECT* dst;
    RECT* src;
    if (m_inputHalfSel == 0) {
        dst = &m_cursorSaveDst0;
        half = m_scratchSurface0;
        src = &m_cursorSaveSrc0;
    } else {
        dst = &m_cursorSaveDst1;
        half = m_scratchSurface1;
        src = &m_cursorSaveSrc1;
    }

    dst->left = x - m_cursorImage->m_anchorX;
    dst->right = m_cursorImage->m_width + dst->left;
    dst->top = y - m_cursorImage->m_anchorY;
    dst->bottom = m_cursorImage->m_height + dst->top;
    tagSIZE mode = m_mgr->GetModeSize();
    if (dst->left < 0) {
        dst->left = 0;
    }
    if (dst->right > mode.cx) {
        dst->right = mode.cx;
    }
    if (dst->top < 0) {
        dst->top = 0;
    }
    if (dst->bottom > mode.cy) {
        dst->bottom = mode.cy;
    }
    src->right = dst->right - dst->left;
    src->bottom = dst->bottom - dst->top;

    CDDSurface* target = pair->m_surface;
    if (target == NULL) {
        return 0;
    }

    i32 r = half->BltFast(0, 0, target, dst, 0x10);
    if (r != 0) {
        CDDrawDeviceManager::ReportError(NULL, 0, r);
    }

    if (m_drewThisFrame != 0) {
        RECT vp = m_world->m_level->m_planeCtx;
        RECT clip;
        CopyRect((&clip), (&vp));
        target->DecodeThunk(
            m_pathPreviewSource.x,
            m_pathPreviewSource.y,
            m_pathPreviewDestination.x,
            m_pathPreviewDestination.y,
            3,
            m_pathPreviewColor,
            clip
        );
    }

    m_cursorImage->RenderFrame(pair, x, y, 0);

    DDSCAPS caps;
    i32 inSysMem;
    if (target->m_ddSurface->GetCaps(&caps) == 0) {
        inSysMem = caps.dwCaps & DDSCAPS_SYSTEMMEMORY;
    } else {
        inSysMem = 0;
    }
    if (inSysMem == 0) {
        m_inputHalfSel = m_inputHalfSel == 0;
    }
    return 1;
}

// @early-stop
// Residue is one block: retail's `rearm` exit is a REGISTER read-modify-write
// (mov eax,[esi+0x40] / or al,1 / mov [esi+0x40],eax at 0xd0f28) while cl emits the
// memory form `or dword ptr [esi+0x40],1`, which frees eax early so cl also hoists
// `mov eax,1` out of the shared tail. No source spelling found that steers the RMW
// form; the other three |= sites in this function pick the memory form in retail too.
// (The drag-rect clamp above WAS a source shape - see the comment there.)
RVA(0x000d0db0, 0x347)
i32 CPlay::HandleDragMove(i32 keyFlags, i32 x, i32 y) {

    LevelCoordRect box;
    if (m_inGame != 0) {
        return 1;
    }
    if (m_paused != 0) {
        return 1;
    }
    if (m_lightFx != NULL && m_guts->m_position != STATUSBAR_HIDDEN
        && m_guts->m_activeTab != TAB_GAME) {
        m_lightFx->ContinueMinimapPan(keyFlags, x, y);
    }

    if (m_dragSnapActive != 0) {
        if (m_guts == NULL) {
            return 1;
        }
        m_guts->SetSpritePos(m_snapOriginX + x, m_snapOriginY + y);
        goto rearm;
    }

    if (m_levelOverlayOpen != 0) {
        return m_guts->HandlePointerDrag(keyFlags, x, y);
    }

    box = m_world->m_level->m_planeCtx;
    if (x >= box.left && x <= box.right && y >= box.top && y <= box.bottom) {

        if (m_dragInProgress != 0) {
            m_guts->ClearTabSprites(TAB_ALL);
        }
        m_dragInProgress = 0;
        if (m_worldReady != 0) {

            // Clamp anchor first: retail loads m_dragClampMaxX into the accumulator
            // BEFORE m_cursorX (0xd0ee8/0xd0eee), which is what makes the max arm
            // read `jle` (keep the anchor) rather than cl's mirrored `jg`.
            {
                i32 anchorX = m_dragClampMaxX;
                i32 curX = m_cursorX;
                m_hudRect.left = curX < anchorX ? curX : anchorX;
                m_hudRect.right = curX <= anchorX ? anchorX : curX;
                i32 anchorY = m_dragClampMaxY;
                i32 curY = m_cursorY;
                m_hudRect.top = curY < anchorY ? curY : anchorY;
                m_hudRect.bottom = curY <= anchorY ? anchorY : curY;
            }
        rearm:
            CWwdGameObjectA* s = m_scrollSink;
            if (s == NULL) {
                return 1;
            }
            s->m_stateFlags |= SPRITE_STATE_HIDDEN;
            return 1;
        }

        if (m_hitTest->HitTest(x, y) == 0 && m_mgr->m_frameGate == 0 && m_inGame == 0
            && m_dragInhibit1 == 0 && m_dragInhibit2 == 0) {

            if (m_cursorId != 0) {
                if (m_scrollSink != NULL) {
                    m_scrollSink->m_stateFlags |= SPRITE_STATE_HIDDEN;
                }
            } else {
                if (m_scrollSink != NULL) {
                    m_scrollSink->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
                }
            }
            CGameLevel* v = m_world->m_level;
            LevelCoordRect* vr = &v->m_mainPlane->m_viewRect;
            i32 wx = vr->left - v->m_planeCtx.left + x;
            i32 wy = vr->top - v->m_planeCtx.top + y;
            m_mgr->m_cmdGrid->PlaceObjectFull(wx, wy);
            return 1;
        }
        if (m_scrollSink != NULL) {
            m_scrollSink->m_stateFlags |= SPRITE_STATE_HIDDEN;
        }
        return 1;
    }

    if (m_scrollSink != NULL) {
        m_scrollSink->m_stateFlags |= SPRITE_STATE_HIDDEN;
    }
    m_dragInProgress = 1;
    m_guts->HandlePointerDrag(keyFlags, x, y);
    if (m_worldReady != 0) {

        m_hudRect.left = m_cursorX > box.left ? m_cursorX : box.left;
        m_hudRect.left = m_hudRect.left < m_dragClampMaxX ? m_hudRect.left : m_dragClampMaxX;
        m_hudRect.right = m_cursorX < box.right ? m_cursorX : box.right;
        m_hudRect.right = m_hudRect.right > m_dragClampMaxX ? m_hudRect.right : m_dragClampMaxX;
        m_hudRect.top = m_cursorY <= box.top ? box.top : m_cursorY;
        m_hudRect.top = m_hudRect.top < m_dragClampMaxY ? m_hudRect.top : m_dragClampMaxY;
        m_hudRect.bottom = m_cursorY < box.bottom ? m_cursorY : box.bottom;
        m_hudRect.bottom = m_hudRect.bottom > m_dragClampMaxY ? m_hudRect.bottom : m_dragClampMaxY;
    }
    if (m_cursorTargetValid != 0 && m_mgr->m_cmdGrid->m_pendingFxKind == 0) {
        FlushPendingOps();
    }
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
    RECT* dst;
    RECT* src;
    if (m_inputHalfSel == 0) {
        half = m_scratchSurface0;
        dst = &m_cursorSaveDst0;
        src = &m_cursorSaveSrc0;
    } else {
        half = m_scratchSurface1;
        dst = &m_cursorSaveDst1;
        src = &m_cursorSaveSrc1;
    }

    CDDSurface* probeTarget = m_world->m_drawTarget->m_backPair->m_surface;
    if (probeTarget == NULL) {
        return 0;
    }

    i32 r = probeTarget->BltFast(dst->left, dst->top, half, src, 0x10);
    if (r != 0) {
        CDDrawDeviceManager::ReportError(NULL, 0, r);
    }
    return 1;
}

// @early-stop
RVA(0x000d12b0, 0x2d5)
i32 CPlay::LoadScrollSpeedOptions() {
    // Two function-local statics sharing one dynamic-init guard byte, bit 1 and
    // bit 2 (retail 0x24c01c); the data are at 0x24c274 and 0x24c270.
    DATA(0x0024c274)
    static i32 s_minScrollSpeed = g_buteMgr.GetInt("Optionz", "MinScrollSpeed");
    DATA(0x0024c270)
    static i32 s_scrollSpeedRange = g_buteMgr.GetInt("Optionz", "MaxScrollSpeed")
                                    - g_buteMgr.GetInt("Optionz", "MinScrollSpeed");

    CPlay* self = this;
    CGruntzMgr* w = m_mgr;
    i32 changed = 0;
    CDDrawWorkerHost* g = w->m_world->m_level->m_mainPlane;

    i32 sx = g->m_snappedX;
    i32 sy = g->m_snappedY;
    // retail keeps the percent scale as its own temp: the int range multiply
    // lands after the double multiply, not reassociated ahead of it.
    double frac = static_cast<double>(w->m_scrollSpeed) * 0.01;
    i32 speed = static_cast<i32>(frac * s_scrollSpeedRange + s_minScrollSpeed);

    SIZE
    extent;
    extent.cx = w->m_modeSize.cx;
    extent.cy = w->m_modeSize.cy;

    if (self->m_cursorX < 0xc || (self->m_scrollEdgeLock & 1)) {
        if (self->m_scrollEdgeActive & 1) {
            i32 d = (timeGetTime() - self->m_lastScrollTimeX) * speed / 1000;
            if (d) {
                if (d > 0x64) {
                    d = 0x64;
                }
                sx -= d;
                self->m_lastScrollTimeX = timeGetTime();
                changed = 1;
            }
        } else {
            self->m_scrollEdgeActive |= 1;
            self->m_lastScrollTimeX = timeGetTime();
        }
    } else {
        self->m_scrollEdgeActive &= ~1;
    }

    if (self->m_cursorX > extent.cx - 0xc || (self->m_scrollEdgeLock & 4)) {
        if (self->m_scrollEdgeActive & 4) {
            i32 d = (timeGetTime() - self->m_lastScrollTimeX) * speed / 1000;
            if (d) {
                if (d > 0x64) {
                    d = 0x64;
                }
                sx += d;
                self->m_lastScrollTimeX = timeGetTime();
                changed = 1;
            }
        } else {
            self->m_scrollEdgeActive |= 4;
            self->m_lastScrollTimeX = timeGetTime();
        }
    } else {
        self->m_scrollEdgeActive &= ~4;
    }

    if (self->m_cursorY < 0xf || (self->m_scrollEdgeLock & 2)) {
        if (self->m_scrollEdgeActive & 2) {
            i32 d = (timeGetTime() - self->m_lastScrollTimeY) * speed / 1000;
            if (d) {
                if (d > 0x64) {
                    d = 0x64;
                }
                sy -= d;
                self->m_lastScrollTimeY = timeGetTime();
                changed = 1;
            }
        } else {
            self->m_scrollEdgeActive |= 2;
            self->m_lastScrollTimeY = timeGetTime();

            changed = 1;
        }
    } else {
        self->m_scrollEdgeActive &= ~2;
    }

    if (self->m_cursorY > extent.cy - 0xf || (self->m_scrollEdgeLock & 8)) {
        if (self->m_scrollEdgeActive & 8) {
            i32 d = (timeGetTime() - self->m_lastScrollTimeY) * speed / 1000;
            if (d) {
                if (d > 0x64) {
                    d = 0x64;
                }
                sy += d;
                self->m_lastScrollTimeY = timeGetTime();
                changed = 1;
            }
        } else {
            self->m_scrollEdgeActive |= 8;
            self->m_lastScrollTimeY = timeGetTime();
        }
    } else {
        self->m_scrollEdgeActive &= ~8;
    }

    if (changed) {
        self->ResetGoals(sx, sy);
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000d1650, 0x90)
void CPlay::DrawMessageFrame(i32 index, i32 useFront) {
    CDDrawWorker* set = LookupWorker(m_world->m_imageRegistry->m_workersByName, "GAME_MESSAGEZ");
    if (set != NULL) {
        CImage* frame = set->GetAt(index);
        if (frame != NULL) {
            LevelCoordRect vp = m_world->m_level->m_planeCtx;
            i32 cx = vp.left + (vp.right - vp.left) / 2;
            i32 cy = vp.top + (vp.bottom - vp.top) / 2;
            LayerBlitFrame(m_world, frame, cx, cy, useFront, 1);
        }
    }
}

RVA(0x000d1710, 0x122)
void CPlay::LoadSBITextEdges(i32 msgId) {
    CString s;
    s.LoadString(msgId);

    RECT rect;

    RECT vp = m_world->m_level->m_planeCtx;
    i32 bottom = vp.bottom - g_buteMgr.GetInt("Font", "TextBottomEdge");
    i32 right = vp.right - g_buteMgr.GetInt("Font", "TextRightEdge");
    i32 top = vp.top + g_buteMgr.GetInt("Font", "TextTopEdge");
    i32 left = vp.left + g_buteMgr.GetInt("Font", "TextLeftEdge");
    SetRect(&rect, left, top, right, bottom);

    EngStr_DrawText(m_world, &s, &rect, 0x78, 1, 0xff, 0xff, 0, 1);
    m_stepCountdown = 2;
}

RVA(0x000d1890, 0x1ba)
void CPlay::PlayCueAt(
    i32 cueId,
    i32 fontSel,
    i32 toFrontPage,
    i32 r,
    i32 g,
    i32 b,
    i32 flag,
    RECT* rectSrc
) {
    RECT rect;

    if (cueId != m_lastCueId) {

        if (m_cueText.LoadString(cueId) == 0) {
            return;
        }
        m_lastCueId = cueId;
    }

    if (rectSrc != NULL) {
        i32 bottom = rectSrc->bottom - g_buteMgr.GetInt("Font", "TextBottomEdge");
        i32 right = rectSrc->right - g_buteMgr.GetInt("Font", "TextRightEdge");
        i32 top = rectSrc->top + g_buteMgr.GetInt("Font", "TextTopEdge");
        i32 left = rectSrc->left + g_buteMgr.GetInt("Font", "TextLeftEdge");
        SetRect(&rect, left, top, right, bottom);
    } else {

        RECT vp = m_world->m_level->m_planeCtx;
        i32 bottom = vp.bottom - g_buteMgr.GetInt("Font", "TextBottomEdge");
        i32 right = vp.right - g_buteMgr.GetInt("Font", "TextRightEdge");
        i32 top = vp.top + g_buteMgr.GetInt("Font", "TextTopEdge");
        i32 left = vp.left + g_buteMgr.GetInt("Font", "TextLeftEdge");
        SetRect(&rect, left, top, right, bottom);
    }

    // Retail 0xd19c6 `je 0xd1a09`: the NON-zero arm is the FRONT/primary renderer
    // (thunk 0x1c5d -> EngStr_DrawText 0x115440) and the zero arm is the BACK page
    // (thunk 0x31d9 -> ShowHudMessageAlt 0x115520). Every in-game caller passes 0,
    // so the cue text belongs on the back page - drawing it on the primary lets the
    // frame's own Flip swap it away, which is the level-start text blink.
    if (toFrontPage != 0) {
        EngStr_DrawText(m_world, &m_cueText, &rect, fontSel, 1, r, g, b, flag);
    } else {
        ShowHudMessageAlt(m_world, &m_cueText, &rect, fontSel, 1, r, g, b, flag);
    }
}

RVA(0x000d1ac0, 0x4f)
void CPlay::StepScroll() {
    CGameLevel* v = m_world->m_level;

    RECT* vr = &v->m_mainPlane->m_viewRect;

    i32 y = m_cursorY + (vr->top - v->m_planeCtx.top);
    i32 x = vr->left + (m_cursorX - v->m_planeCtx.left);

    y = (y & ~TILE_MASK_PX) + TILE_HALF_PX;
    x = (x & ~TILE_MASK_PX) + TILE_HALF_PX;

    m_scrollSink->m_screenX = x;
    m_scrollSink->m_screenY = y;
}

RVA(0x000d1b30, 0x20)
i32 CPlay::SetCursorFrame(i32 item) {
    LoadCursorSprites(item, 0);
    m_cursorFrame = item;
    return 1;
}

RVA(0x000d1b60, 0xc90)
i32 CPlay::ExecuteCommand(
    u8 playerIndex,
    char unitIndex,
    GZ_ENUM_STORAGE(PlayerCommandKind, char) commandKind,
    i16 targetXOrPlayerIndex,
    i16 targetYOrUnitIndex,
    char pickupType,
    u8 unusedScheduleSlot
) {
    CGruntzMgr* mgr = m_mgr;
    if (mgr->m_frameGate != 0) {
        return 0;
    }
    i32 res;
    i32 hitPlayerIndex;
    i32 hitUnitIndex;

    switch (static_cast<u8>(commandKind)) {
        case PLAYERCMD_PLACE_GRUNT: {
            u32 currentPlayer = static_cast<u32>(g_curPlayer);

            i32 r = mgr->m_cmdGrid->PlaceObject(
                static_cast<u8>(playerIndex),
                static_cast<u16>(targetXOrPlayerIndex),
                static_cast<u16>(targetYOrUnitIndex),
                100000,
                GRUNT_ENTRANCE_DROP,
                g_groupSentinel,
                0,
                0,
                0,
                0,
                0,
                0,
                NULL
            );
            if (r == -1) {
                if (m_world->m_soundRegistry->m_silentMode == 0) {
                    SoundCue* cue =
                        static_cast<SoundCue*>(m_world->m_soundRegistry->Lookup("GAME_BADSELECT"));
                    if (cue != NULL) {
                        cue->PlayIfElapsed(g_soundVolumePercent, 0, 0, 0);
                    }
                }
                return 0;
            }
            if (static_cast<u8>(playerIndex) == currentPlayer) {
                g_gameReg->m_cmdGrid->ResetAll();
            }
            return 1;
        }

        case PLAYERCMD_MOVE: {
            u32 player = static_cast<u8>(playerIndex);
            u32 gi = static_cast<u8>(unitIndex);
            CGrunt* g = mgr->m_cmdGrid->m_units[gi + player * 0xf];
            if (g != NULL && g->m_entranceCommitted != 0) {
                g->m_arrivalActive = 0;
            }
            if (!m_mgr->m_cmdGrid->ClearCell(
                    player,
                    gi,
                    static_cast<u16>(targetXOrPlayerIndex),
                    static_cast<u16>(targetYOrUnitIndex),
                    0
                )) {
                if (player != static_cast<u32>(g_curPlayer) || g == NULL
                    || g->m_entranceCommitted == 0) {
                    return 0;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (player != static_cast<u32>(g_curPlayer) || g == NULL
                || g->m_entranceCommitted == 0) {
                return 1;
            }
            g_gameReg->m_voiceManager->PlayVoice(g, 0x323, -1, 0, -1, -1);
            return 1;
        }

        case PLAYERCMD_GUARD_BEGIN: {
            CGrunt* g =
                mgr->m_cmdGrid
                    ->m_units[static_cast<u8>(playerIndex) * 0xf + static_cast<u8>(unitIndex)];
            if (g != NULL) {
                if (g->m_tileClaimed != 1) {
                    g->m_arrivalRerollLo = 0;
                    g->m_arrivalRerollWindowLo = 0;
                    g->m_arrivalRerollHi = 0;
                    g->m_arrivalRerollWindowHi = 0;
                    g->m_defenderPx.m_x = g->m_lastTilePx.m_x;
                    g->m_tileClaimed = 1;
                    g->m_defenderPx.m_y = g->m_lastTilePx.m_y;

                    switch (g->m_entranceReason) {
                        case PICKUP_BOOMERANG:
                            g->m_defenderRadius = 1;
                            break;
                        case PICKUP_GUNHAT:
                        case PICKUP_NERFGUN:
                        case PICKUP_ROCK:
                            g->m_defenderRadius = 1;
                            break;
                        case PICKUP_WELDER:
                        case PICKUP_WINGZ:
                            g->m_defenderRadius = 1;
                            break;
                        default:
                            g->m_defenderRadius =
                                g_buteMgr.GetIntDef("Grunt", "PlayerDefenderRadius", 3) + 1;
                    }
                    g->m_arrivalFlags |= 0x18040402;
                    g->m_arrivalCell.m_x = -1;
                    g->m_arrivalState = AI_DEFENDER;
                    g->m_defenderState = AISTATE_SEEK;
                    g->m_arrivalCell.m_y = -1;
                    g->m_arrivalActive = 0;
                    g->m_object->m_extent.left = 0;
                    g->m_object->m_extent.right = 0;
                    g->m_object->m_extent.top = 0;
                    g->m_object->m_extent.bottom = 0;
                    g->SetEntrancePos(1, 1);
                }
                g->m_arrivalNotified = 0;
            }
            return 1;
        }

        case PLAYERCMD_GUARD_END: {

            CGrunt* g =
                mgr->m_cmdGrid
                    ->m_units[static_cast<u8>(playerIndex) * 0xf + static_cast<u8>(unitIndex)];
            // Gate on m_tileClaimed (+0x420), NOT m_entranceCommitted (+0x1fc): retail
            // reads the SAME slot it is about to clear five instructions later, i.e.
            // "if this grunt is not guarding, do nothing".  Gating on the always-set
            // committed flag ran the whole guard teardown - m_arrivalState = AI_NONE,
            // the 0xe7fbfbfd flag mask and SetEntrancePos(1,1) - on any grunt, so a
            // stray GUARD_END cancelled whatever that grunt was actually doing.
            if (g == NULL || g->m_tileClaimed == 0) {
                return 1;
            }
            g->m_arrivalRerollLo = 0;
            g->m_arrivalRerollWindowLo = 0;
            g->m_arrivalRerollHi = 0;
            g->m_arrivalRerollWindowHi = 0;
            g->m_tileClaimed = 0;
            g->m_arrivalState = AI_NONE;
            g->m_arrivalFlags &= 0xe7fbfbfd;
            g->SetEntrancePos(1, 1);
            return 1;
        }

        case PLAYERCMD_USE_TOOL_AT_POINT: {
            u32 player = static_cast<u8>(playerIndex);
            u32 gi = static_cast<u8>(unitIndex);
            CGrunt* g = mgr->m_cmdGrid->m_units[gi + player * 0xf];
            if (g == NULL || g->m_entranceCommitted == 0) {
                return 0;
            }
            if (g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = AI_NONE;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            i32 px = static_cast<u16>(targetXOrPlayerIndex);
            i32 py = static_cast<u16>(targetYOrUnitIndex);

            CGrunt* node =
                m_mgr->m_cmdGrid
                    ->CellHitTest(px, py, &hitPlayerIndex, &hitUnitIndex, TM_ALL_PLAYERS);
            if (node != NULL && g->m_entranceActive == 0) {
                g->SetArrivalTarget(
                    hitPlayerIndex,
                    hitUnitIndex,
                    node->m_object->m_screenX,
                    node->m_object->m_screenY
                );
            } else {
                g->m_arrivalActive = 0;
            }
            res = m_mgr->m_cmdGrid->ApplyTriggerA(player, gi, px, py);
            if (res == 0) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 0;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (res == -1) {
                if (!m_mgr->m_cmdGrid->ClearCell(player, gi, px, py, 2)) {
                    if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                        return 0;
                    }
                    g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                    return 0;
                }
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 1;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x323, -1, 0, -1, -1);
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                return 1;
            }
            g_gameReg->m_voiceManager->PlayVoice(g, 0x323, -1, 0, -1, -1);
            return 1;
        }

        case PLAYERCMD_USE_TOOL_ON_GRUNT: {
            u32 player = static_cast<u8>(playerIndex);
            u32 gi = static_cast<u8>(unitIndex);
            CGrunt* g = mgr->m_cmdGrid->m_units[gi + player * 0xf];
            if (g == NULL || g->m_entranceCommitted == 0) {
                return 0;
            }
            if (g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = AI_NONE;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            i32 targetPlayerIndex = static_cast<u16>(targetXOrPlayerIndex);
            i32 targetUnitIndex = static_cast<u16>(targetYOrUnitIndex);
            CGrunt* g2 = m_mgr->m_cmdGrid
                             ->m_units[targetUnitIndex + targetPlayerIndex * TM_UNITS_PER_PLAYER];
            if (g2 == NULL || g->m_entranceActive != 0) {
                g->m_arrivalActive = 0;
                return 0;
            }
            i32 sx = g2->m_object->m_screenX;
            i32 sy = g2->m_object->m_screenY;
            g->SetArrivalTarget(targetPlayerIndex, targetUnitIndex, sx, sy);
            res = m_mgr->m_cmdGrid->ApplyTriggerA(player, gi, sx, sy);
            if (res == 0) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 0;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (res == -1) {
                if (!m_mgr->m_cmdGrid->ClearCell(player, gi, sx, sy, 2)) {
                    if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                        return 0;
                    }
                    g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                    return 0;
                }
                if (player != static_cast<u32>(g_curPlayer)
                    || static_cast<u32>(g_curPlayer) == static_cast<u32>(targetPlayerIndex)
                    || g->m_entranceCommitted == 0) {
                    return 1;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x325, -1, 0, -1, -1);
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer)
                || static_cast<u32>(g_curPlayer) == static_cast<u32>(targetPlayerIndex)
                || g->m_entranceCommitted == 0) {
                return 1;
            }
            g_gameReg->m_voiceManager->PlayVoice(g, 0x325, -1, 0, -1, -1);
            return 1;
        }

        case PLAYERCMD_USE_TOY_AT_POINT: {
            u32 player = static_cast<u8>(playerIndex);
            u32 gi = static_cast<u8>(unitIndex);
            CGrunt* g = mgr->m_cmdGrid->m_units[gi + player * 0xf];
            if (g == NULL || g->m_entranceCommitted == 0 || g->m_entranceActive != 0) {
                return 0;
            }
            if (g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = AI_NONE;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            i32 px = static_cast<u16>(targetXOrPlayerIndex);
            i32 py = static_cast<u16>(targetYOrUnitIndex);
            CGrunt* node =
                m_mgr->m_cmdGrid
                    ->CellHitTest(px, py, &hitPlayerIndex, &hitUnitIndex, TM_ALL_PLAYERS);
            if (node != NULL && g->m_entranceActive == 0) {
                g->SetArrivalTarget(
                    hitPlayerIndex,
                    hitUnitIndex,
                    node->m_object->m_screenX,
                    node->m_object->m_screenY
                );
            } else {
                g->m_arrivalActive = 0;
            }
            res = m_mgr->m_cmdGrid->ApplyTriggerB(player, gi, px, py);
            if (res == 0) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 0;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (res == -1) {
                if (!m_mgr->m_cmdGrid->ClearCell(player, gi, px, py, 3)) {
                    if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                        return 0;
                    }
                    g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                    return 0;
                }
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 1;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x323, -1, 0, -1, -1);
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                return 1;
            }
            g_gameReg->m_voiceManager->PlayVoice(g, 0x323, -1, 0, -1, -1);
            return 1;
        }

        case PLAYERCMD_USE_TOY_ON_GRUNT: {
            u32 player = static_cast<u8>(playerIndex);
            u32 gi = static_cast<u8>(unitIndex);
            CGrunt* g = mgr->m_cmdGrid->m_units[gi + player * 0xf];
            if (g == NULL || g->m_entranceCommitted == 0 || g->m_entranceActive != 0) {
                return 0;
            }
            if (g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = AI_NONE;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            i32 targetPlayerIndex = static_cast<u16>(targetXOrPlayerIndex);
            i32 targetUnitIndex = static_cast<u16>(targetYOrUnitIndex);
            CGrunt* g2 = m_mgr->m_cmdGrid
                             ->m_units[targetUnitIndex + targetPlayerIndex * TM_UNITS_PER_PLAYER];
            if (g2 == NULL || g->m_entranceActive != 0) {
                g->m_arrivalActive = 0;
                return 0;
            }
            i32 sx = g2->m_object->m_screenX;
            i32 sy = g2->m_object->m_screenY;
            g->SetArrivalTarget(targetPlayerIndex, targetUnitIndex, sx, sy);
            res = m_mgr->m_cmdGrid->ApplyTriggerB(player, gi, sx, sy);
            if (res == 0) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                    return 0;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (res == -1) {
                if (!m_mgr->m_cmdGrid->ClearCell(player, gi, sx, sy, 3)) {
                    if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == 0) {
                        return 0;
                    }
                    g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                    return 0;
                }
                if (player != static_cast<u32>(g_curPlayer)
                    || static_cast<u32>(g_curPlayer) == static_cast<u32>(targetPlayerIndex)
                    || g->m_entranceCommitted == 0) {
                    return 1;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x325, -1, 0, -1, -1);
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer)
                || static_cast<u32>(g_curPlayer) == static_cast<u32>(targetPlayerIndex)
                || g->m_entranceCommitted == 0) {
                return 1;
            }
            g_gameReg->m_voiceManager->PlayVoice(g, 0x325, -1, 0, -1, -1);
            return 1;
        }

        case PLAYERCMD_GIVE_TOOL: {
            u32 player = static_cast<u8>(playerIndex);
            if (player == static_cast<u32>(g_curPlayer)) {
                m_playerCommandPending = 0;
            }
            u32 gi = static_cast<u8>(unitIndex);
            i32 idx = gi + player * 0xf;
            CGrunt* g = mgr->m_cmdGrid->m_units[idx];
            if (g != NULL && g->m_entranceCommitted != 0 && g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = AI_NONE;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            i32 sel = 0;
            i32 live = (g_gameReg->m_gameMode != GAMEMODE_SINGLE);
            CGrunt* g2 = m_mgr->m_cmdGrid->m_units[idx];
            i32 r;
            if (g2 == NULL || g2->m_entranceCommitted == 0) {
                r = 0;
            } else {
                r = g2->LoadPickupSprites(
                    static_cast<PickupType>(pickupType & 0xff),
                    0,
                    0,
                    0,
                    live
                );
            }
            if (r != 0) {
                if (player == static_cast<u32>(g_curPlayer)) {
                    m_mgr->m_cmdGrid->ResetCell(player, gi, 0, 0);
                }
                sel = 1;
            }
            if (player == static_cast<u32>(g_curPlayer)) {
                m_dragInhibit2 = 0;
                m_guts->EnterHlRow(sel, m_cursorFrame);
                SetCursorFrame(0);
            }
            return r;
        }

        case PLAYERCMD_STOP: {
            CGrunt* g =
                mgr->m_cmdGrid
                    ->m_units[static_cast<u8>(playerIndex) * 0xf + static_cast<u8>(unitIndex)];
            if (g == NULL || g->m_entranceCommitted == 0 || g->m_entranceActive != 0) {
                return 0;
            }
            g->SetEntrancePos(1, 1);
            if (g->m_tileClaimed != 0) {
                g->m_arrivalRerollLo = 0;
                g->m_arrivalRerollWindowLo = 0;
                g->m_arrivalRerollHi = 0;
                g->m_arrivalRerollWindowHi = 0;
                g->m_tileClaimed = 0;
                g->m_arrivalState = AI_NONE;
                g->m_arrivalFlags &= 0xe7fbfbfd;
                g->SetEntrancePos(1, 1);
            }
            return 1;
        }
    }

    return 1;
}

static inline CGameLevel* LevelOf(CDDrawSurfaceMgr* holder) {
    return holder->m_level;
}

static inline TileCollisionKind LookupTileType(CGameLevel* level, i32 x, i32 y) {
    CDDrawWorkerHost* g = level->m_mainPlane;
    if (x < 0) {
        x = 0;
    } else if (x >= g->m_wrapW) {
        x = g->m_wrapW - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= g->m_wrapH) {
        y = g->m_wrapH - 1;
    }
    i32 tx = x >> g->m_shiftX;
    i32 ty = y >> g->m_shiftY;
    i32 subX = x - (tx << g->m_shiftX);
    i32 subY = y - (ty << g->m_shiftY);
    i32 cell = g->GetTileHandle(tx, ty);
    if (cell == UNINIT_FILL || cell == -1) {
        return TILEKIND_PASSABLE;
    }

    CImageSet1* tc = static_cast<CImageSet1*>(level->m_imageSets.GetAt(cell & 0xffff));
    return tc->GetCollisionAt(subX, subY);
}

// The three placement arms below are hand-copies that diverged in retail: arm 1
// (DispatchTileTriggerSwitchLogic) reaches the grid through CDDrawWorkerHost::GetTileHandle
// (0xd2ee7 call), arms 2 and 3 subscript it (0xd3a6d, 0xd3e5b), and arm 2's powerup
// test lists three tile kinds where arm 1 lists four.
static inline TileCollisionKind LookupTileTypeDirect(CGameLevel* level, i32 x, i32 y) {
    CDDrawWorkerHost* g = level->m_mainPlane;
    if (x < 0) {
        x = 0;
    } else if (x >= g->m_wrapW) {
        x = g->m_wrapW - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= g->m_wrapH) {
        y = g->m_wrapH - 1;
    }
    i32 tx = x >> g->m_shiftX;
    i32 ty = y >> g->m_shiftY;
    i32 subX = x - (tx << g->m_shiftX);
    i32 subY = y - (ty << g->m_shiftY);
    i32 cell = g->m_tileGrid[g->m_rowOffsets[ty] + tx];
    if (cell == UNINIT_FILL || cell == -1) {
        return TILEKIND_PASSABLE;
    }

    CImageSet1* tc = static_cast<CImageSet1*>(level->m_imageSets.GetAt(cell & 0xffff));
    return tc->GetCollisionAt(subX, subY);
}

RVA(0x000d2b20, 0x21f)
b32 CPlay::PlaceStartGruntz() {

    CObList* list = &m_world->m_childGroup->m_list;
    if (list == NULL) {
        return false;
    }
    i32 counter = 0;
    GruntEntranceMode entranceMode = GRUNT_ENTRANCE_NONE;
    POSITION pos = list->GetHeadPosition();
    if (m_mgr->m_gameMode == GAMEMODE_SINGLE) {
        entranceMode = GRUNT_ENTRANCE_WORMHOLE;
    }
    while (pos != NULL) {
        CGameObject* obj = static_cast<CGameObject*>(list->GetNext(pos));
        if (obj != NULL) {
            CLogicRecord* record = obj->m_logicRecord;

            LogicRecordDispatchFn dispatch = record->m_dispatch;
            if (dispatch == DispatchGruntStartingPointLogic) {
                i32 x = (obj->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
                i32 y = (obj->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
                i32 idx = m_mgr->m_cmdGrid->PlaceObject(
                    obj->m_smarts,
                    x,
                    y,
                    100000,
                    entranceMode,
                    obj->m_score,
                    obj->m_powerup,
                    obj->m_damage,
                    obj->m_points,
                    obj->m_direction,
                    record->m_minX,
                    record->m_maxX,
                    &obj->m_extent
                );
                if (idx == -1) {
                    CString s;
                    s.Format("Could not add Grunt: Player=%d, x=%d, y=%d", obj->m_smarts, x, y);
                    g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                    return false;
                }
                obj->m_flags |= 0x10000;
            } else if (g_gameReg->m_gameMode != GAMEMODE_SINGLE
                       && dispatch == DispatchGruntCreationPointLogic
                       && obj->m_smarts == g_curPlayer) {

                GruntzPlayer* e = &g_gameReg->m_options[g_curPlayer];
                if (e != NULL && counter < e->m_comboSel) {
                    i32 x = (obj->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
                    i32 y = (obj->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
                    m_mgr->m_cmdSubMgr->EnqueueSingle(
                        true,
                        static_cast<char>(obj->m_smarts),
                        0,
                        static_cast<char>(IDX(PLAYERCMD_PLACE_GRUNT)),
                        x,
                        y,
                        0,
                        0
                    );
                    counter++;
                }
            }
        }
    }
    return true;
}

// @early-stop
RVA(0x000d2dd0, 0x1e40)
i32 CPlay::ValidateLevelTiles() {
    i32 validCount = 0;
    i32 counts[4];
    for (i32 c = 0; c < 4; c++) {
        counts[c] = 0;
    }

    CObList* list = &m_world->m_childGroup->m_list;
    if (list == NULL) {
        return 0;
    }
    POSITION pos = list->GetHeadPosition();
    if (pos == NULL) {
        return 1;
    }

    i32 ok = 1;
    do {
        CGameObject* obj = static_cast<CGameObject*>(list->GetNext(pos));
        if (obj == NULL) {
            continue;
        }

        LogicRecordDispatchFn dispatch = obj->m_logicRecord->m_dispatch;

        if (dispatch == DispatchTileTriggerSwitchLogic) {
            TileCollisionKind type =
                LookupTileType(LevelOf(m_world), obj->m_screenX, obj->m_screenY);
            if (type == TILEKIND_GIANT_ROCK) {

                CTileTriggerLogic* hit;
                i32 col = obj->m_speedX - 1;
                i32 row = obj->m_speedY - 1;
                b32 found = 0;
                i32 colOff = col << 8;
                while (found == 0 && col < obj->m_speedX + 2) {
                    row = obj->m_speedY - 1;
                    while (found == 0 && row < obj->m_speedY + 2) {
                        hit = m_beginMarker->FindLogic(row + colOff, TRIGID_GIANT_ROCK_22);
                        if (hit != NULL) {
                            found = 1;
                        }
                        if (found == 0) {
                            row++;
                        }
                    }
                    if (found == 0) {
                        col++;
                        colOff += 0x100;
                    }
                }
                if (found == 0) {
                    CString s;
                    s.Format("Bad switch at: x=%d, y=%d", obj->m_screenX, obj->m_screenY);
                    g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                    return 0;
                }
                i32 rel = (obj->m_speedY - row) * 3 - col + obj->m_speedX;

                i32 tcidx = (static_cast<CGiantRockLogic*>(hit))->m_matrix[rel + 4];
                if (tcidx == 0) {
                    CString s;
                    s.Format("Bad switch at: x=%d, y=%d", obj->m_screenX, obj->m_screenY);
                    g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                    return 0;
                }
                type = (static_cast<CImageSet1*>(LevelOf(m_world)->m_imageSets.GetAt(tcidx)))
                           ->GetCollisionAt(0, 0);
            }
            if (type == TILEKIND_GAUNTLET_ROCK_A || type == TILEKIND_GAUNTLET_ROCK_B
                || type == TILEKIND_COVERED_POWERUP || type == TILEKIND_REVEALED_POWERUP) {

                CTileTriggerLogic* r =
                    m_beginMarker->FindLogic(obj->m_id, TRIGID_COVERED_POWERUP_26);
                if (r == NULL) {
                    CString s;
                    s.Format("Bad switch at: x=%d, y=%d", obj->m_screenX, obj->m_screenY);
                    g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                    return 0;
                }
                i32 tcidx = r->m_tileToken;
                if (tcidx == 0) {
                    CString s;
                    s.Format("Bad switch at: x=%d, y=%d", obj->m_screenX, obj->m_screenY);
                    g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                    return 0;
                }
                type = (static_cast<CImageSet1*>(LevelOf(m_world)->m_imageSets.GetAt(tcidx)))
                           ->GetCollisionAt(0, 0);
            }
            switch (type) {
                case TILEKIND_MULTI_SWITCH:
                case TILEKIND_MULTI_SWITCH_UP:
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_MULTI_SWITCH_3,
                            obj->m_speedX,
                            obj->m_speedY,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_logicRecord->m_userRect1,
                            obj->m_logicRecord->m_userRect2,
                            type == TILEKIND_MULTI_SWITCH_UP,
                            obj->m_damage,
                            0
                        )) {
                        CString s;
                        s.Format("Bad multi switch at: x=%d, y=%d", obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case TILEKIND_EXCLUSIVE_SWITCH:
                case TILEKIND_EXCLUSIVE_SWITCH_UP:
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_EXCLUSIVE_SWITCH_4,
                            obj->m_speedX,
                            obj->m_speedY,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_logicRecord->m_userRect1,
                            obj->m_logicRecord->m_userRect2,
                            type == TILEKIND_EXCLUSIVE_SWITCH_UP,
                            obj->m_damage,
                            0
                        )) {
                        CString s;
                        s.Format(
                            "Bad up-down switch at: x=%d, y=%d",
                            obj->m_screenX,
                            obj->m_screenY
                        );
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case TILEKIND_SECRET_SWITCH:
                    // Only the DOWN variant counts: an UP secret switch has already
                    // been triggered. Retail gives case 0x3d its own 12-byte entry
                    // point at 0xd32b7 that falls into the shared body at 0xd32c3,
                    // which is what makes the dispatch a DIRECT 16-entry table rather
                    // than the byte-index LUT cl builds when the pairs all coincide.
                    g_gameReg->m_scoreHud->m_secretsAvailable++;
                    // fall through
                case TILEKIND_SECRET_SWITCH_UP:
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_SECRET_SWITCH_6,
                            obj->m_speedX,
                            obj->m_speedY,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_logicRecord->m_userRect1,
                            obj->m_logicRecord->m_userRect2,
                            type == TILEKIND_SECRET_SWITCH_UP,
                            obj->m_damage,
                            0
                        )) {
                        CString s;
                        s.Format(
                            "Bad secret switch at: x=%d, y=%d",
                            obj->m_screenX,
                            obj->m_screenY
                        );
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case TILEKIND_TIME_SWITCH:
                case TILEKIND_TIME_SWITCH_UP:
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_TIME_SWITCH_7,
                            obj->m_speedX,
                            obj->m_speedY,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_logicRecord->m_userRect1,
                            obj->m_logicRecord->m_userRect2,
                            type == TILEKIND_TIME_SWITCH_UP,
                            obj->m_damage,
                            0
                        )) {
                        CString s;
                        s.Format("Bad time switch at: x=%d, y=%d", obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case TILEKIND_CHECKPOINT:
                case TILEKIND_CHECKPOINT_UP:
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_CHECKPOINT_SWITCH_8,
                            obj->m_speedX,
                            obj->m_speedY,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_logicRecord->m_userRect1,
                            obj->m_logicRecord->m_userRect2,
                            type == TILEKIND_CHECKPOINT_UP,
                            obj->m_damage,
                            obj->m_smarts
                        )) {
                        CString s;
                        s.Format(
                            "Bad pressure plate at: x=%d, y=%d",
                            obj->m_screenX,
                            obj->m_screenY
                        );
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case TILEKIND_SWITCH_A:
                case TILEKIND_SWITCH_A_UP:
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_SWITCH_1,
                            obj->m_speedX,
                            obj->m_speedY,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_logicRecord->m_userRect1,
                            obj->m_logicRecord->m_userRect2,
                            type == TILEKIND_SWITCH_A_UP || type == TILEKIND_SWITCH_B_UP
                                || type == TILEKIND_SWITCH_C_UP
                                || type == TILEKIND_SECRET_SWITCH_UP,
                            obj->m_damage,
                            0
                        )) {
                        CString s;
                        s.Format(
                            "Bad toggle switch at: x=%d, y=%d",
                            obj->m_screenX,
                            obj->m_screenY
                        );
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case TILEKIND_SWITCH_B:
                case TILEKIND_SWITCH_B_UP:
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_SWITCH_2,
                            obj->m_speedX,
                            obj->m_speedY,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_logicRecord->m_userRect1,
                            obj->m_logicRecord->m_userRect2,
                            type == TILEKIND_SWITCH_A_UP || type == TILEKIND_SWITCH_B_UP
                                || type == TILEKIND_SWITCH_C_UP
                                || type == TILEKIND_SECRET_SWITCH_UP,
                            obj->m_damage,
                            0
                        )) {
                        CString s;
                        s.Format("Bad hold switch at: x=%d, y=%d", obj->m_screenX, obj->m_screenY);
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                case TILEKIND_SWITCH_C:
                case TILEKIND_SWITCH_C_UP:
                    if (!m_beginMarker->AddSwitchLogic(
                            TRIGID_SWITCH_5,
                            obj->m_speedX,
                            obj->m_speedY,
                            obj->m_id,
                            obj->m_extent,
                            obj->m_area,
                            obj->m_switchRect,
                            obj->m_clip,
                            obj->m_logicRecord->m_userRect1,
                            obj->m_logicRecord->m_userRect2,
                            type == TILEKIND_SWITCH_A_UP || type == TILEKIND_SWITCH_B_UP
                                || type == TILEKIND_SWITCH_C_UP
                                || type == TILEKIND_SECRET_SWITCH_UP,
                            obj->m_damage,
                            0
                        )) {
                        CString s;
                        s.Format(
                            "Bad once-only switch at: x=%d, y=%d",
                            obj->m_screenX,
                            obj->m_screenY
                        );
                        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                        return 0;
                    }
                    validCount++;
                    obj->m_flags |= 0x10000;
                    break;
                default: {
                    CString s;
                    s.Format(
                        "Switch on an unknown tile at: x=%d, y=%d",
                        obj->m_screenX,
                        obj->m_screenY
                    );
                    g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                    return 0;
                }
            }
        } else if (dispatch == DispatchTileTriggerLogic) {
            TileCollisionKind type =
                LookupTileTypeDirect(LevelOf(m_world), obj->m_screenX, obj->m_screenY);
            if (type == TILEKIND_GIANT_ROCK) {

                CTileTriggerLogic* hit;
                i32 col = obj->m_speedX - 1;
                i32 row = obj->m_speedY - 1;
                b32 found = 0;
                i32 colOff = col << 8;
                while (found == 0 && col < obj->m_speedX + 2) {
                    row = obj->m_speedY - 1;
                    while (found == 0 && row < obj->m_speedY + 2) {
                        hit = m_beginMarker->FindLogic(row + colOff, TRIGID_GIANT_ROCK_22);
                        if (hit != NULL) {
                            found = 1;
                        }
                        if (found == 0) {
                            row++;
                        }
                    }
                    if (found == 0) {
                        col++;
                        colOff += 0x100;
                    }
                }
                if (found == 0) {
                    CString s;
                    s.Format("Bad trigger at: x=%d, y=%d", obj->m_screenX, obj->m_screenY);
                    g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                    return 0;
                }
                i32 rel = (obj->m_speedX - col) * 3 - row + obj->m_speedY;

                i32 tcidx = (static_cast<CGiantRockLogic*>(hit))->m_matrix[rel + 4];
                if (tcidx == 0) {
                    CString s;
                    s.Format("Bad trigger at: x=%d, y=%d", obj->m_screenX, obj->m_screenY);
                    g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                    return 0;
                }
                type = (static_cast<CImageSet1*>(LevelOf(m_world)->m_imageSets.GetAt(tcidx)))
                           ->GetCollisionAt(0, 0);
            } else if (type == TILEKIND_GAUNTLET_ROCK_A || type == TILEKIND_GAUNTLET_ROCK_B
                       || type == TILEKIND_COVERED_POWERUP) {

                CTileTriggerLogic* r =
                    m_beginMarker->FindLogic(obj->m_id, TRIGID_COVERED_POWERUP_26);
                if (r == NULL) {
                    CString s;
                    s.Format("Bad trigger at: x=%d, y=%d", obj->m_screenX, obj->m_screenY);
                    g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                    return 0;
                }
                i32 tcidx = r->m_tileToken;
                if (tcidx == 0) {
                    CString s;
                    s.Format("Bad trigger at: x=%d, y=%d", obj->m_screenX, obj->m_screenY);
                    g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                    return 0;
                }
                type = (static_cast<CImageSet1*>(LevelOf(m_world)->m_imageSets.GetAt(tcidx)))
                           ->GetCollisionAt(0, 0);
            }
            if (type >= TILEKIND_TOGGLE_BRIDGE_FIRST && type <= TILEKIND_TOGGLE_BRIDGE_LAST) {
                if (!m_beginMarker->AddLogic(
                        static_cast<TileCollisionKind>(type),
                        TRIGID_TIME_TRIGGER_23,
                        obj->m_speedX,
                        obj->m_speedY,
                        obj->m_id,
                        obj->m_extent,
                        obj->m_area,
                        obj->m_switchRect,
                        obj->m_clip,
                        obj->m_logicRecord->m_userRect1,
                        obj->m_logicRecord->m_userRect2,
                        0,
                        obj->m_damage,
                        obj->m_points,
                        obj->m_health
                    )) {
                    CString s;
                    s.Format(
                        "Bad toggle-bridge trigger at: x=%d, y=%d",
                        obj->m_screenX,
                        obj->m_screenY
                    );
                    g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                    return 0;
                }
                validCount++;
                obj->m_flags |= 0x10000;
            } else {
                if (!m_beginMarker->AddLogic(
                        static_cast<TileCollisionKind>(type),
                        TRIGID_TILE_TRIGGER_21,
                        obj->m_speedX,
                        obj->m_speedY,
                        obj->m_id,
                        obj->m_extent,
                        obj->m_area,
                        obj->m_switchRect,
                        obj->m_clip,
                        obj->m_logicRecord->m_userRect1,
                        obj->m_logicRecord->m_userRect2,
                        obj->m_smarts,
                        obj->m_damage,
                        obj->m_points,
                        0
                    )) {
                    CString s;
                    s.Format("Bad trigger at: x=%d, y=%d", obj->m_screenX, obj->m_screenY);
                    g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                    return 0;
                }
                validCount++;
                obj->m_flags |= 0x10000;
            }
        } else if (dispatch == DispatchTileSecretTriggerLogic) {
            TileCollisionKind type =
                LookupTileTypeDirect(LevelOf(m_world), obj->m_screenX, obj->m_screenY);
            if (!m_beginMarker->AddLogic(
                    type,
                    TRIGID_SECRET_TRIGGER_25,
                    obj->m_speedX,
                    obj->m_speedY,
                    obj->m_id,
                    obj->m_extent,
                    obj->m_area,
                    obj->m_switchRect,
                    obj->m_clip,
                    obj->m_logicRecord->m_userRect1,
                    obj->m_logicRecord->m_userRect2,
                    obj->m_smarts,
                    obj->m_damage,
                    obj->m_points,
                    0
                )) {
                CString s;
                s.Format("Bad secret trigger at: x=%d, y=%d", obj->m_screenX, obj->m_screenY);
                g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                return 0;
            }
            validCount++;
            obj->m_flags |= 0x10000;
        } else if (dispatch == DispatchLevelTimeLogic) {

            if (m_frameMarker != NULL && m_mgr->m_gameMode != GAMEMODE_MULTIPLAYER
                && g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                i32 seconds = obj->m_points;
                i32 minutes = obj->m_score;
                seconds += seconds;
                minutes += minutes;
                if (seconds > 0x3b) {
                    minutes++;
                    seconds -= 0x3c;
                }
                m_frameMarker->SetTime(minutes, seconds);
            }
            obj->m_flags |= 0x10000;
        } else if (dispatch == DispatchInGameIconLogic) {
            if (obj->m_smarts == IDX(PICKUP_MEGAPHONE)) {

                m_guts->QueuePickupReward(obj->m_points, obj->m_score);
            }
        } else if (dispatch == DispatchGruntCreationPointLogic) {
            if (obj->m_smarts == g_curPlayer) {
                CoordPoolNode* cell = g_coordPool.m_freeHead;
                Coord* slot = NULL;
                if (cell->m_next != NULL) {
                    slot = &cell->m_coord;
                    g_coordPool.m_freeHead = cell->m_next;
                }
                slot->m_x = (obj->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
                slot->m_y = (obj->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
                m_startMarkers.SetAtGrow(StartMarkerCount(), slot);
            }
        } else if (dispatch == DispatchBrickzLogic) {

            CDDrawWorkerHost* pl = m_world->m_level->m_mainPlane;
            i32 tile = pl->m_tileGrid[pl->m_rowOffsets[obj->m_speedY] + obj->m_speedX];
            if (tile >= 0x12f && tile <= 0x149) {
                if (m_beginMarker->AddActionEvent(
                        static_cast<BrickTileId>(tile),
                        obj->m_speedX,
                        obj->m_speedY,
                        obj->m_id,
                        obj->m_extent
                    )
                    == NULL) {
                    CString s;
                    s.Format("Bad brickz at: x=%d, y=%d", obj->m_screenX, obj->m_screenY);
                    g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                    return 0;
                }
                validCount++;
                obj->m_flags |= 0x10000;
            } else {
                CString s;
                s.Format("Bad brickz at: x=%d, y=%d", obj->m_screenX, obj->m_screenY);
                g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                return 0;
            }
        } else if (dispatch == DispatchGruntPuddleLogic) {

            m_mgr->m_cmdGrid->PlacePuddle(obj, 0);
        } else if (dispatch == DispatchGuardPointLogic) {

            i32 col = obj->m_screenX >> TILE_SHIFT_PX;
            i32 rowBase = obj->m_screenY >> TILE_SHIFT_PX;
            i32 stride = (col << 3) - col;

            i32 ebp = stride - 7;
            for (i32 dy = -1; dy < 2; dy++, ebp += 7) {
                i32 row = rowBase;
                i32 ofs = rowBase - 1;
                for (i32 k = 3; k != 0; k--, ofs++, row++) {
                    i32 gx = dy + col;
                    i32 gyy = row - 1;
                    CGruntzMapMgr* gg = g_gameReg->m_tileGrid;
                    if (static_cast<u32>(gx) >= gg->m_width
                        || static_cast<u32>(gyy) >= gg->m_height) {
                        continue;
                    }
                    i32 kind = obj->m_smarts;
                    // Four dense cases and NO default: that is what makes cl lower this
                    // to retail's `cmp eax,3 / ja / jmp [eax*4+T]` (0xd41cf) instead of a
                    // comparison ladder. Retail's out-of-range path leaves `bit` at
                    // whatever it held and still does `counts[kind]++`; spelling that as
                    // an uninitialised read costs 0.4% and is not worth writing.
                    i32 bit = 0;
                    switch (static_cast<PlayerSlot>(kind)) {
                        case PLAYER_SLOT_0:
                            bit = 0x100000;
                            break;
                        case PLAYER_SLOT_1:
                            bit = 0x200000;
                            break;
                        case PLAYER_SLOT_2:
                            bit = 0x400000;
                            break;
                        case PLAYER_SLOT_3:
                            bit = 0x800000;
                            break;
                    }
                    counts[kind]++;
                    gg = g_gameReg->m_tileGrid;
                    if (static_cast<u32>(gx) >= gg->m_width
                        || static_cast<u32>(gyy) >= gg->m_height) {
                        continue;
                    }
                    i32* cellRow = gg->m_rowInts[ofs];
                    cellRow[ebp] |= bit;
                }
            }
        } else if (dispatch == DispatchToobSpikezLogic) {
            CGruntzMapMgr* gg = g_gameReg->m_tileGrid;
            i32 tileX = obj->m_screenX >> TILE_SHIFT_PX;
            i32 tileY = obj->m_screenY >> TILE_SHIFT_PX;
            if (static_cast<u32>(tileX) < gg->m_width && static_cast<u32>(tileY) < gg->m_height) {
                gg->m_rowInts[tileY][tileX * 7] |= 0x2000000;
            }
        } else if (dispatch == DispatchWarpStonePadLogic) {
            if (g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
                CoordPoolNode* cell = g_coordPool.m_freeHead;
                Coord* slot = NULL;
                if (cell->m_next != NULL) {
                    slot = &cell->m_coord;
                    g_coordPool.m_freeHead = cell->m_next;
                }
                slot->m_x = obj->m_screenX >> TILE_SHIFT_PX;
                slot->m_y = obj->m_screenY >> TILE_SHIFT_PX;
                CPtrArray* cells = &m_placedObjectCells[obj->m_score];
                cells->SetAtGrow(cells->GetSize(), slot);
            }
        }
    } while (pos != NULL);

    TRACE("%s\n", static_cast<LPCTSTR>(CString("ValidateLevelTiles")));
    return ok;
}

RVA(0x000d53a0, 0x19)
i32 CDDrawWorkerHost::GetTileHandle(i32 tileX, i32 tileY) {
    return m_tileGrid[m_rowOffsets[tileY] + tileX];
}

// @early-stop
// The 332 instructions, 13 calls, CFG and 19 relocations match. The only body
// residue is nine stack stores: retail alternates each field load/store, while
// cl batches three loads before draining stores. Scalar, cursor, LONG, memcpy,
// flat-array, 3x3 aggregate and pointer/reference helper forms all batch alike;
// by-value helpers instead swap the function-wide EBX/EDI allocation.
RVA(0x000d53d0, 0x466)
i32 CPlay::ScanBuildTiles() {
    CObList* pl = &m_world->m_childGroup->m_list;
    if (pl == NULL) {
        return 0;
    }
    POSITION pos = pl->GetHeadPosition();
    while (pos != NULL) {
        CGameObject* p = static_cast<CGameObject*>(pl->GetNext(pos));
        if (p == NULL) {
            continue;
        }
        if (p->m_extent.left == COORD_UNSET) {
            p->m_extent.left = 0;
        }
        if (p->m_area.left == COORD_UNSET) {
            p->m_area.left = 0;
        }
        if (p->m_switchRect.left == COORD_UNSET) {
            p->m_switchRect.left = 0;
        }
        if (p->m_clip.left == COORD_UNSET) {
            p->m_clip.left = 0;
        }
        LogicRecordDispatchFn dispatch = p->m_logicRecord->m_dispatch;
        if (dispatch == DispatchGiantRockLogic) {
            i32 buf[9];
            buf[0] = p->m_extent.left;
            buf[1] = p->m_extent.top;
            buf[2] = p->m_extent.right;
            buf[3] = p->m_area.left;
            buf[4] = p->m_area.top;
            buf[5] = p->m_area.right;
            buf[6] = p->m_switchRect.left;
            buf[7] = p->m_switchRect.top;
            buf[8] = p->m_switchRect.right;
            if (m_beginMarker->AddGiantRockLogic(
                    p->m_speedX,
                    p->m_speedY,
                    p->m_id,
                    buf,
                    p->m_powerup,
                    p->m_points,
                    p->m_faceDirection
                )
                == NULL) {
                CString s;
                s.Format("Bad rock at: x=%d, y=%d", p->m_screenX, p->m_screenY);
                g_gameReg->EnterModalUI(s);
                return 0;
            }
            if (p->m_powerup == IDX(PICKUP_MEGAPHONE)) {
                m_guts->QueuePickupReward(p->m_points, p->m_score);
            }
            p->m_flags |= 0x10000;
        } else if (dispatch == DispatchCoveredPowerupLogic) {
            CGameLevel* ds = m_world->m_level;
            i32 x = p->m_screenX;
            i32 y = p->m_screenY;
            if (x < 0) {
                x = 0;
            } else {
                i32 lim = ds->m_mainPlane->m_wrapW;
                if (x >= lim) {
                    x = lim - 1;
                }
            }
            if (y < 0) {
                y = 0;
            } else {
                i32 lim = ds->m_mainPlane->m_wrapH;
                if (y >= lim) {
                    y = lim - 1;
                }
            }
            CDDrawWorkerHost* g = ds->m_mainPlane;
            i32 shX = g->m_shiftX;
            i32 tileX = x >> shX;
            i32 shY = g->m_shiftY;
            i32 tileY = y >> shY;
            i32 subX = x - (tileX << shX);
            i32 subY = y - (tileY << shY);
            i32 cell = g->m_tileGrid[g->m_rowOffsets[tileY] + tileX];
            TileCollisionKind tile;
            if (cell == UNINIT_FILL || cell == static_cast<i32>(0xffffffff)) {
                tile = TILEKIND_PASSABLE;
            } else {

                // Ingest: the raw WWD attribute byte for this cell.
                tile = (static_cast<CImageSet1*>(ds->m_imageSets[cell & 0xffff]))
                           ->GetCollisionAt(subX, subY);
            }
            if (m_beginMarker->AddLogic(
                    tile,
                    TRIGID_COVERED_POWERUP_26,
                    p->m_speedX,
                    p->m_speedY,
                    p->m_id,
                    p->m_extent,
                    p->m_area,
                    p->m_switchRect,
                    p->m_clip,
                    p->m_logicRecord->m_userRect1,
                    p->m_logicRecord->m_userRect2,
                    p->m_smarts,
                    p->m_powerup,
                    p->m_points,
                    p->m_faceDirection
                )
                == NULL) {
                CString s;
                s.Format("Bad covered powerup at: x=%d, y=%d", p->m_screenX, p->m_screenY);
                g_gameReg->EnterModalUI(s);
                return 0;
            }
            if (p->m_powerup == IDX(PICKUP_MEGAPHONE)) {
                m_guts->QueuePickupReward(p->m_points, p->m_score);
            }
            p->m_flags |= 0x10000;
        }
    }
    return 1;
}

// @early-stop
// Every instruction agrees; the two call-crossing values (`this` and the CObList
// POSITION cursor) hold each other's home.  Retail spills `this` to the single
// 4-byte local slot and gives ebp to the cursor, cl does the reverse, and the
// extra `jmp` into the loop plus the doubled [esp+0x10] reload are consequences of
// a memory-resident cursor.  Rotating-cursor class, see
// docs/relevations/cl5-c2-register-picker-is-a-rotating-cursor.md; declaration
// order, the for/while form and a hoisted m_mgr were measured and none rotate it.
RVA(0x000d5960, 0x160)
i32 CPlay::AddLevelGruntz() {
    CObList* chain = &m_world->m_childGroup->m_list;
    if (chain == NULL) {
        return 0;
    }
    POSITION pos = chain->GetHeadPosition();
    while (pos != NULL) {
        CGameObject* g = static_cast<CGameObject*>(chain->GetNext(pos));
        if (g == NULL) {
            continue;
        }
        if (g->m_logicRecord->m_dispatch != DispatchGruntStartingPointLogic) {
            continue;
        }
        if (g->m_smarts == g_curPlayer) {
            continue;
        }
        i32 x = ((g->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX);
        i32 y = ((g->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX);

        i32 r = m_mgr->m_cmdGrid->PlaceObject(
            g->m_smarts,
            x,
            y,
            0x186a0,
            GRUNT_ENTRANCE_NONE,
            g->m_score,
            g->m_powerup,
            g->m_damage,
            g->m_points,
            g->m_direction,
            g->m_logicRecord->m_minX,
            g->m_logicRecord->m_maxX,
            &g->m_extent
        );
        if (r == -1) {
            CString msg;
            msg.Format("Could not add Grunt: Player=%d, x=%d, y=%d", g->m_smarts, x, y);

            (g_gameReg)->EnterModalUI(msg);
            return 0;
        }
        g->m_flags |= 0x10000;
    }
    return 1;
}

// @early-stop
// Both m_mgr reloads agree in shape; only the scratch register rotates (retail
// ecx then edx, ours eax then ecx - the same sequence entered one slot earlier).
// Rotating-cursor class, see
// docs/relevations/cl5-c2-register-picker-is-a-rotating-cursor.md.
RVA(0x000d5b20, 0xbb)
i32 CPlay::PositionBridgeToggle(StatusBarDock mode, StatusBarDock) {
    CGruntzMgr* w = m_mgr;
    i32 ex = w->m_modeSize.cx;
    i32 ey = w->m_modeSize.cy;
    CTimer* pt;
    if (mode == STATUSBAR_DOCK_LEFT) {
        m_hitTest->Configure(CHATBOX_WITH_LEFT_STATUSBAR);
        pt = m_frameMarker;
        if (pt == NULL) {
            goto done;
        }
        ex -= 0x37;
        ey -= 0x16;
        pt->m_baseX = ex;
        pt->m_baseY = ey;
    } else if (mode == STATUSBAR_DOCK_RIGHT) {
        m_hitTest->Configure(CHATBOX_WITH_RIGHT_STATUSBAR);
        pt = m_frameMarker;
        if (pt == NULL) {
            goto done;
        }
        ex -= 0xd7;
        ey -= 0x16;
        pt->m_baseX = ex;
        pt->m_baseY = ey;
    } else {
        m_hitTest->Configure(CHATBOX_WITH_HIDDEN_STATUSBAR);
        pt = m_frameMarker;
        if (pt == NULL) {
            goto done;
        }
        ex -= 0x37;
        ey -= 0x16;
        pt->m_baseX = ex;
        pt->m_baseY = ey;
    }
done:

    // The outer gate spells the whole member chain and the body caches it: cl5's
    // redundant-test peephole is syntactic, so both `test eax,eax` survive as retail
    // has them (docs/patterns/guard-reads-the-array-element-not-the-cached-local.md).
    if (m_mgr->m_cmdGrid->m_goal != NULL) {
        CTriggerMgr* g = m_mgr->m_cmdGrid;
        if (g->m_goal != NULL) {
            g->m_goal->m_flags |= 0x10000;
            g->m_goal = NULL;
        }
        m_mgr->m_cmdGrid->LoadCameraSprite();
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000d5c10, 0x10d)
i32 CState::DrawScreenTextImage(const char* name) {
    char buf[0x40];
    sprintf(buf, "\\SCREENZ\\%sTEXT", name);
    CParseSource* src = SymTab2c()->ResolveQualified(buf, IMGTAG_DIP);
    if (src == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* world = m_world;
    CDDrawSurfacePair* page = world->m_drawTarget->m_backPair;
    if (page == NULL) {
        return 0;
    }
    CImage img(0, world);
    if (img.Resolve(src, 1) == 0) {
        return 0;
    }
    img.RenderFrame(page, 0x140, 0x158, 0);
    return 1;
}

RVA_COMPGEN(0x000d5d70, 0x16, ??1CWapObj@@UAE@XZ)

RVA_COMPGEN(0x000d5e50, 0x1e, ??_GCImage@@UAEPAXI@Z)

RVA_COMPGEN(0x000d5e80, 0x5b, ??1CImage@@UAE@XZ)

RVA(0x000d5f00, 0x69)
i32 CPlay::ResetGoals(i32 x, i32 y) {
    CGruntzMgr* w = m_mgr;
    CTriggerMgr* g = w->m_cmdGrid;
    if (g->m_goal != NULL) {
        g->m_goal->m_flags |= 0x10000;
        g->m_goal = NULL;
    }
    g->m_armed = 0;
    CDDrawWorkerHost* pg = m_mgr->m_world->m_level->m_mainPlane;
    SET_SCROLL_POSITION_SCALED_FIRST(pg, x, y);
    return 1;
}

RVA(0x000d5f90, 0xd7)
i32 CPlay::FindStartPointAt(i32 x, i32 y, i32* outX, i32* outY) {

    i32 id = g_curPlayer;
    GruntzPlayer* slot = &g_gameReg->m_options[id];

    if (slot != NULL && g_gameReg->m_cmdGrid->m_unitCountByPlayer[id] < slot->m_comboSel) {
        i32 i = 0;
        if (i < StartMarkerCount()) {
            do {
                Coord* m = StartMarkerAt(i);
                if (m != NULL) {
                    RECT rc;
                    SetRect(&rc, m->m_x - 0x20, m->m_y - 0x20, m->m_x + 0x20, m->m_y + 0x20);
                    if (CGameLevel::PointInRect(&rc, x, y)) {
                        *outX = m->m_x;
                        *outY = m->m_y;
                        return 1;
                    }
                }
                i++;
            } while (i < StartMarkerCount());
        }
    }
    return 0;
}

// @early-stop
// 36/36 blocks. Two residues: retail RELOADS fm->m_currentMs for m_accum (the
// preceding store through the same pointer defeats its CSE) and puts m_running
// last, where cl keeps the tested value in a register. The arm-statement order
// below is a HOIST BLOCKER - writing m_startStamp first, which is retail's
// EMITTED order, makes cl hoist those three instructions into the predecessor.
RVA(0x000d60b0, 0x2cd)
i32 CPlay::ResetPlayState() {
    char sequenceName[0x40];
    if (m_mgr->m_musicEnabled != 0 && g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
        m_ambientTiming.m_interval.m_lo = AMBIENT_INTRO_INTERVAL_MS;
        m_ambientTiming.m_interval.m_hi = 0;
        m_ambientTiming.m_start.m_lo = g_frameTime;
        m_ambientTiming.m_start.m_hi = 0;
        wsprintfA(sequenceName, "INTRO%d", GetAmbientId());
        if (g_gameReg->m_musicEnabled != 0) {
            m_mgr->m_midi->PlaySequence(sequenceName, 0);
        }
        m_ambientInitDone = 0;
    } else {
        wsprintfA(sequenceName, "AMBIENT%d", GetAmbientId());
        MidiManager* midi = m_mgr->m_midi;
        MidiSequence* sequence = midi->FindSequence(sequenceName);
        if (sequence != NULL) {
            midi->m_currentSequence = sequence;
        }
        if (m_mgr->m_midi->m_currentSequence != NULL) {
            m_mgr->m_midi->m_currentSequence->SetLooping(1);
        }
        CGruntzMgr* gameManager = g_gameReg;
        if (gameManager->m_musicEnabled != 0 && gameManager->m_gameMode == GAMEMODE_REPLAY) {
            m_mgr->m_midi->PlaySequence(sequenceName, 1);
        }
        m_ambientTiming.m_start.m_lo = 0;
        m_ambientTiming.m_interval.m_lo = 0;
        m_ambientTiming.m_start.m_hi = 0;
        m_ambientTiming.m_interval.m_hi = 0;
        m_ambientInitDone = 1;
    }
    if (m_mgr->m_gameMode == GAMEMODE_SINGLE) {
        CGruntzMgr* reg = g_gameReg;

        if (reg->m_strWorldFile.GetLength() == 0) {
            m_mgr->m_scoreHud->FillRecord(m_levelIndex, 1);
            reg = g_gameReg;

            if (reg->m_cheatMgr->m_cheatsUsed == 0) {
                i32 id = m_levelIndex;
                if (id > 0x24 || id == 1) {
                    (static_cast<CSaveGame*>(reg->m_saveSink))
                        ->SetMaxLevel(static_cast<QuestLevel>(id));
                    reg = g_gameReg;
                }
            }
            (static_cast<CSaveGame*>(reg->m_saveSink))->Save(NULL, 0x81a6);
        }
        CGameLevel* g = m_mgr->m_world->m_level;
        ResetGoals(g->m_header.startX, g->m_header.startY);
    } else {
        GruntzPlayer* slot = &g_gameReg->m_options[g_curPlayer];
        if (slot != NULL) {
            ResetGoals(slot->m_focusX, slot->m_focusY);
        } else {
            CGameLevel* g = m_mgr->m_world->m_level;
            ResetGoals(g->m_header.startX, g->m_header.startY);
        }
    }
    if (m_scrollSink != NULL) {
        m_scrollSink->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    }
    m_inGame = 0;
    if (!PlaceStartGruntz()) {
        return 0;
    }
    for (i32 i = 0; i < 4; i++) {
        g_gameReg->m_options[i].m_battlezConfig.StepAllRowSpawns();
    }
    m_winLoseBanner = 0;
    CTimer* fm = m_frameMarker;
    if (fm != NULL) {
        fm->m_unusedStamp.m_v = 0xffffffff;
        if (fm->m_currentMs != 0) {
            fm->m_running = 1;
            fm->m_startStamp.m_v = static_cast<u32>(g_frameTime);
            fm->m_accum.m_v = static_cast<u32>(fm->m_currentMs);
            fm->m_baseTime.m_v = static_cast<u32>(g_frameTime);
        } else {
            fm->m_startStamp.m_v = static_cast<u32>(g_frameTime);
        }
    }
    CTriggerMgr* tl = m_mgr->m_cmdGrid;
    tl->m_countdownActive = 1;
    tl->m_phase = FINISH_STATE_ACTIVE;
    tl->m_pendingFxKind = 0;
    tl->m_gooTimerBaseLo = 0;
    tl->m_gooIntervalLo = 0;
    tl->m_gooTimerBaseHi = 0;
    tl->m_gooIntervalHi = 0;
    tl->m_resourceTimerBaseLo = 0;
    tl->m_resourceIntervalLo = 0;
    tl->m_resourceTimerBaseHi = 0;
    tl->m_resourceIntervalHi = 0;
    tl->m_finishReasonFrame = FINISH_REASON_NONE;
    tl->m_rollingballWanted = 0;
    tl->m_teleportWanted = 0;
    tl->m_groupFlag = 1;
    return 1;
}

RVA(0x000d6440, 0xd3)
i32 CPlay::OpenLevelOverlay(i32 showQuitConfirmation) {
    if (m_levelOverlayOpen != 0) {
        return 1;
    }
    m_levelOverlayOpen = 1;
    m_worldReady = 0;
    m_dragSnapActive = 0;
    FlushPendingOps();
    if (showQuitConfirmation == 0) {
        CStatusBarMgr* g = m_guts;
        if (g->m_position == STATUSBAR_HIDDEN) {
            g->RestoreStatusBar();
        }
        if (g->m_activeTab != TAB_GAME) {
            g->SetTabState(SBICMD_TAB_GAME, MENUITEM_SELECTED);
        }
        g->SetTab(GAME_TAB_MISSION_STATUS, 1);
        g->Deactivate();
    }
    m_guts->BuildGameTabResumeButton(1);
    CStatusBarMgr* g = m_guts;
    g->m_levelOverlayActive = 1;
    g->m_quitConfirmationActive = showQuitConfirmation;
    g->ResetWidgets(0);
    g->TryActivate();
    g->m_hlBusy = 1;
    g->Deactivate();
    m_savedClock = g_frameTime;
    return 1;
}

RVA(0x000d6560, 0x45)
i32 CPlay::CloseLevelOverlay(i32) {
    if (m_levelOverlayOpen != 0) {
        CStatusBarMgr* worker = m_guts;
        m_levelOverlayOpen = 0;
        worker->ExitMode();
        if (g_gameReg->m_gameMode != GAMEMODE_MULTIPLAYER) {
            g_frameTime = m_savedClock;
        }
    }
    return 1;
}

// @early-stop

RVA(0x000d65d0, 0x7cc)
i32 CPlay::LoadWarlordSprites(CMulti* ctx, i32* loaded) {
    if (g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
        for (i32 id = IDX(GRUNT_BOOMERANG); id <= IDX(GRUNT_YOYO); id++) {
            if (loaded[id] == 0) {
                BuildHelpReveal(0);
                loaded[id] = 1;
            }
            if (!BuildGruntTypeNameTable(static_cast<PickupType>(id), 1, 0, ctx)) {
                return 0;
            }
        }
        if (!BuildGruntTypeNameTable(GRUNT_HAREKRISHNA, 1, 0, ctx)) {
            return 0;
        }
        if (loaded[0x21] == 0) {
            BuildHelpReveal(0);
            loaded[0x21] = 1;
        }
        if (!BuildGruntTypeNameTable(GRUNT_REAPER, 1, 0, ctx)) {
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

    CObList* head = &this->m_world->m_childGroup->m_list;
    POSITION pos = head == NULL ? NULL : head->GetHeadPosition();
    while (pos != NULL) {
        CGameObject* obj = static_cast<CGameObject*>(head->GetNext(pos));
        if (obj) {
            LogicRecordDispatchFn dispatch = obj->m_logicRecord->m_dispatch;
            if (dispatch == DispatchGruntStartingPointLogic) {
                i32 v = obj->m_powerup;
                if (v) {
                    if (!BuildGruntTypeNameTable(static_cast<PickupType>(v), 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[v] == 0) {
                        BuildHelpReveal(0);
                        loaded[v] = 1;
                    }
                }
                v = obj->m_damage;
                if (v) {
                    if (!BuildGruntTypeNameTable(static_cast<PickupType>(v), 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[v] == 0) {
                        BuildHelpReveal(0);
                        loaded[v] = 1;
                    }
                }
                EnemyAiType aiType = static_cast<EnemyAiType>(obj->m_points);
                switch (aiType) {
                    case AI_BOMBER:
                        if (!BuildGruntTypeNameTable(PICKUP_BOMB, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[1] == 0) {
                            BuildHelpReveal(0);
                            loaded[1] = 1;
                        }
                        break;
                    case AI_BRICKLAYER:
                        if (!BuildGruntTypeNameTable(PICKUP_BRICK, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[3] == 0) {
                            BuildHelpReveal(0);
                            loaded[3] = 1;
                        }
                        break;
                    case AI_GAUNTLETZGRUNT:
                        if (!BuildGruntTypeNameTable(PICKUP_GAUNTLETZ, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[5] == 0) {
                            BuildHelpReveal(0);
                            loaded[5] = 1;
                        }
                        break;
                    case AI_GOOSUCKER:
                        if (!BuildGruntTypeNameTable(PICKUP_GOOBER, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[7] == 0) {
                            BuildHelpReveal(0);
                            loaded[7] = 1;
                        }
                        break;
                    case AI_DIGGER:
                        if (!BuildGruntTypeNameTable(PICKUP_SHOVEL, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[0xd] == 0) {
                            BuildHelpReveal(0);
                            loaded[0xd] = 1;
                        }
                        break;
                    case AI_TIMEBOMBER:
                        if (!BuildGruntTypeNameTable(PICKUP_TIMEBOMB, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[0x11] == 0) {
                            BuildHelpReveal(0);
                            loaded[0x11] = 1;
                        }
                        break;
                    case AI_MAGICWANDGRUNT:
                        if (!BuildGruntTypeNameTable(PICKUP_WAND, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[0x13] == 0) {
                            BuildHelpReveal(0);
                            loaded[0x13] = 1;
                        }
                        break;
                    case AI_SCROLLGRUNT:
                        if (!BuildGruntTypeNameTable(PICKUP_SCROLL, 1, 0, ctx)) {
                            return 0;
                        }
                        if (loaded[0x1e] == 0) {
                            BuildHelpReveal(0);
                            loaded[0x1e] = 1;
                        }
                        break;
                }
            } else if (dispatch == DispatchInGameIconLogic) {
                PickupType smarts = static_cast<PickupType>(obj->m_smarts);
                PickupType cv =
                    smarts == PICKUP_MEGAPHONE ? static_cast<PickupType>(obj->m_points) : smarts;
                if (cv >= PICKUP_EQUIPPABLE_FIRST && cv <= PICKUP_EQUIPPABLE_LAST
                    && cv != PICKUP_WARPSTONE) {
                    m_mgr->m_scoreHud->m_toolzAvailable++;
                } else if (cv >= PICKUP_TOYZ_FIRST && cv <= PICKUP_TOYZ_LAST) {
                    m_mgr->m_scoreHud->m_toyzAvailable++;
                } else if (cv >= PICKUP_TIMEDPOWERUP_FIRST && cv <= PICKUP_TIMEDPOWERUP_LAST) {
                    m_mgr->m_scoreHud->m_powerupzAvailable++;
                } else if (cv == PICKUP_COIN) {
                    m_mgr->m_scoreHud->m_coinsAvailable++;
                }
                i32 d = obj->m_smarts;
                PickupType item = static_cast<PickupType>(d);
                if (item <= PICKUP_TOYZ_LAST) {
                    if (!BuildGruntTypeNameTable(item, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[obj->m_smarts] == 0) {
                        BuildHelpReveal(0);
                        loaded[obj->m_smarts] = 1;
                    }
                } else if (d == IDX(GRUNT_HAREKRISHNA)) {
                    if (!BuildGruntTypeNameTable(GRUNT_HAREKRISHNA, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[0x21] == 0) {
                        BuildHelpReveal(0);
                        loaded[0x21] = 1;
                    }
                } else if (d == IDX(GRUNT_REAPER)) {
                    if (!BuildGruntTypeNameTable(GRUNT_REAPER, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[0x22] == 0) {
                        BuildHelpReveal(0);
                        loaded[0x22] = 1;
                    }
                } else if (item == PICKUP_TOYBOX) {
                    if (!BuildGruntTypeNameTable(
                            static_cast<PickupType>(obj->m_points),
                            1,
                            0,
                            ctx
                        )) {
                        return 0;
                    }
                    if (loaded[obj->m_points] == 0) {
                        BuildHelpReveal(0);
                        loaded[obj->m_points] = 1;
                    }
                } else if (item == PICKUP_MEGAPHONE) {
                    if (!BuildGruntTypeNameTable(
                            static_cast<PickupType>(obj->m_points),
                            1,
                            0,
                            ctx
                        )) {
                        return 0;
                    }
                    if (loaded[obj->m_points] == 0) {
                        BuildHelpReveal(0);
                        loaded[obj->m_points] = 1;
                    }
                }
            } else if (dispatch == DispatchCoveredPowerupLogic
                       || dispatch == DispatchGiantRockLogic) {
                PickupType powerup = static_cast<PickupType>(obj->m_powerup);
                PickupType cv =
                    powerup == PICKUP_MEGAPHONE ? static_cast<PickupType>(obj->m_points) : powerup;
                if (cv >= PICKUP_EQUIPPABLE_FIRST && cv <= PICKUP_EQUIPPABLE_LAST
                    && cv != PICKUP_WARPSTONE) {
                    m_mgr->m_scoreHud->m_toolzAvailable++;
                } else if (cv >= PICKUP_TOYZ_FIRST && cv <= PICKUP_TOYZ_LAST) {
                    m_mgr->m_scoreHud->m_toyzAvailable++;
                } else if (cv >= PICKUP_TIMEDPOWERUP_FIRST && cv <= PICKUP_TIMEDPOWERUP_LAST) {
                    m_mgr->m_scoreHud->m_powerupzAvailable++;
                } else if (cv == PICKUP_COIN) {
                    m_mgr->m_scoreHud->m_coinsAvailable++;
                }
                i32 e = obj->m_powerup;
                PickupType item = static_cast<PickupType>(e);
                if (item <= PICKUP_TOYZ_LAST) {
                    if (!BuildGruntTypeNameTable(item, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[obj->m_powerup] == 0) {
                        BuildHelpReveal(0);
                        loaded[obj->m_powerup] = 1;
                    }
                } else if (obj->m_smarts == IDX(GRUNT_HAREKRISHNA)) {
                    if (!BuildGruntTypeNameTable(GRUNT_HAREKRISHNA, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[0x21] == 0) {
                        BuildHelpReveal(0);
                        loaded[0x21] = 1;
                    }
                } else if (obj->m_smarts == IDX(GRUNT_REAPER)) {
                    if (!BuildGruntTypeNameTable(GRUNT_REAPER, 1, 0, ctx)) {
                        return 0;
                    }
                    if (loaded[0x22] == 0) {
                        BuildHelpReveal(0);
                        loaded[0x22] = 1;
                    }
                } else if (item == PICKUP_TOYBOX) {
                    if (!BuildGruntTypeNameTable(
                            static_cast<PickupType>(obj->m_points),
                            1,
                            0,
                            ctx
                        )) {
                        return 0;
                    }
                    if (loaded[obj->m_points] == 0) {
                        BuildHelpReveal(0);
                        loaded[obj->m_points] = 1;
                    }
                } else if (item == PICKUP_MEGAPHONE) {
                    if (!BuildGruntTypeNameTable(
                            static_cast<PickupType>(obj->m_points),
                            1,
                            0,
                            ctx
                        )) {
                        return 0;
                    }
                    if (loaded[obj->m_points] == 0) {
                        BuildHelpReveal(0);
                        loaded[obj->m_points] = 1;
                    }
                }
            }
        }
    }
    return 1;
}

RVA(0x000d6fa0, 0x1fa)
i32 CPlay::EnterMode(GameStateId mode) {
    (g_gameReg)->CheckSavedMode();
    m_guts->Deactivate();
    m_guts->UpdateStatusBar(0);
    m_mgr->RefreshGameClock();

    if (m_initialFramePending != 0) {
        m_initialFramePending = 0;
        m_world->m_drawTarget->m_backPair->m_surface->Fill(0);
        UpdateMgrScroll(g_gameReg, m_guts, m_region0Gate);
        if (m_region1Gate != 0) {
            NotifyVisibleEntities();
        } else {
            m_world->m_level->VisitVisible(
                m_world->m_drawTarget->m_backPair,
                m_world->m_childGroup
            );
            m_world->m_workerList->RenderAndPruneWorkers(
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
            m_world->m_workerList->RenderAndPruneWorkers(
                m_world->m_drawTarget->m_backPair,
                m_world->m_drawTarget->m_overlayPair
            );
        }
        m_guts->Deactivate();
        m_guts->LoadMainStatusBarSprite();
        if (mode == GAMESTATE_HELP) {
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
    RetireScene(0x50, 0x3e8, 0, 1);

    CGameLevel* lvl = m_world->m_level;
    if (lvl->m_mainPlane != NULL) {
        lvl->m_mainPlane->DeactivateDistantObjects();
    }
    m_mgr->RefreshGameClock();
    m_inputWarmup1 = 0;
    m_inputWarmup2 = 0;
    m_inputHalfSel = 0;
    if (m_mgr->m_soundEnabled != 0 && mode != GAMESTATE_HELP) {
        m_mgr->m_worldSounds->Resume();
    }
    if (mode == GAMESTATE_HELP) {
        g_frameTime = m_savedClock;
    }
    m_guts->Deactivate();
    RegisterInputBindings();
    m_hudSuppressed = 0;
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

    PostMessageA(g_gameReg->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_FINISH_LEVEL), 0);
    if (m_scrollSink) {
        m_scrollSink->m_stateFlags |= SPRITE_STATE_HIDDEN;
    }
    return 1;
}

RVA(0x000d72c0, 0x128)
i32 CPlay::BuildHelpReveal(i32 final) {
    CDDrawSurfacePair* view = m_world->m_drawTarget->m_backPair;
    if (view == NULL) {
        return 0;
    }
    if (m_revealFrame == 1) {
        LayerBlitFrame(
            m_world,
            static_cast<CImage*>(m_revealCapStart),
            SCREEN_HALF_W_PX,
            0x1a6,
            1,
            0
        );
        LayerBlitFrame(m_world, static_cast<CImage*>(m_revealCapMid), 0xe0, 0x1a6, 1, 0);
    }

    i32 counter = m_revealFrame;
    i32 col = static_cast<i32>((static_cast<float>(counter) * 3.7857143878936768f));
    // `final` picks the shape: a mid-reveal frame slides ONE strip to col+0xe0
    // and stops there; the final frame paints every remaining strip and then
    // caps the run. cl cross-jumps the two LayerBlitFrame calls into one.
    if (counter < 0x37 && final != 1) {
        LayerBlitFrame(m_world, static_cast<CImage*>(m_revealCapMid), col + 0xe0, 0x1a6, 1, 0);
    } else {
        // cl5's redundant-compare peephole is SYNTACTIC on the operand order, so
        // reversing the loop bound keeps retail's second `cmp edi,0x37 / jge`
        // (docs/patterns/redundant-test-elimination-is-syntactic.md).
        if (counter < 0x37) {
            for (i32 i = counter; 0x37 > i; i++) {
                i32 x = 0xe0 - static_cast<i32>((static_cast<float>(i) * -3.7857143878936768f));
                LayerBlitFrame(m_world, static_cast<CImage*>(m_revealCapMid), x, 0x1a6, 1, 0);
            }
        }
        LayerBlitFrame(m_world, static_cast<CImage*>(m_revealCapEnd), 0x1b4, 0x1a6, 1, 0);
    }
    m_revealFrame = m_revealFrame + 1;
    return 1;
}

RVA(0x000d7440, 0xad)
i32 CPlay::LoadLoadingBarSprite() {
    CDDrawWorker* spr = LookupWorker(m_world->m_imageRegistry->m_workersByName, "GAME_LOADINGBAR");
    if (!spr) {
        return 0;
    }

    m_revealCapStart =
        DDRAW_WORKER_CONTAINS_FRAME(spr, 1) ? DDRAW_WORKER_FRAME_AT_UNCHECKED(spr, 1) : NULL;
    m_revealCapMid =
        DDRAW_WORKER_CONTAINS_FRAME(spr, 2) ? DDRAW_WORKER_FRAME_AT_UNCHECKED(spr, 2) : NULL;
    m_revealCapEnd =
        DDRAW_WORKER_CONTAINS_FRAME(spr, 3) ? DDRAW_WORKER_FRAME_AT_UNCHECKED(spr, 3) : NULL;
    m_revealFrame = 1;
    return 1;
}

RVA(0x000d7520, 0x3b9)
i32 CPlay::SyncState(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload) {
    if (ar == NULL) {
        return 0;
    }
    if (!HeaderSerialize(ar, mode, typeId, payload)) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            if (!SavePlayState(ar)) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (!LoadPlayState(ar)) {
                return 0;
            }
            break;
        case SERIAL_POSTLOAD: {
            if (m_cursorUsesPlayerTint) {
                CGruntzMgr* gameManager = m_mgr;
                i32 playerIndex = g_curPlayer;
                CShadeTable* shadeTable = gameManager->m_spriteFactory->GetSel(
                    IDX(gameManager->m_options[playerIndex].m_colorIndex),
                    0
                );
                if (shadeTable == NULL) {
                    shadeTable = g_gameReg->m_spriteFactory->GetSel(1, 0);
                }
                m_cursorSprite->SetAllTypes(SHADE_PAL_16);
                m_cursorSprite->SetAllFormats(shadeTable);
            }
            char sequenceName[0x40];
            wsprintfA(sequenceName, "AMBIENT%d", GetAmbientId());
            if (g_gameReg->m_musicEnabled) {
                m_mgr->m_midi->PlaySequence(sequenceName, 1);
            }
            m_ambientInitDone = 1;
            break;
        }
    }

    i32* p;
    p = &m_syncTiming.m_start.m_lo;
    SYNC_PAIR(ar, mode, p);
    if (!m_guts->Sync(ar, mode, typeId, payload)) {
        return 0;
    }
    if (!m_frameMarker->HandleEvent(ar, mode, typeId, payload)) {
        return 0;
    }
    p = &m_cueTiming.m_start.m_lo;
    SYNC_PAIR(ar, mode, p);
    if (!m_beginMarker->Serialize(ar, mode, typeId, payload)) {
        return 0;
    }
    p = &m_region0Timing.m_start.m_lo;
    SYNC_PAIR(ar, mode, p);
    p = &m_region1Timing.m_start.m_lo;
    SYNC_PAIR(ar, mode, p);
    p = &m_defeatCountdownTiming.m_start.m_lo;
    SYNC_PAIR(ar, mode, p);
    p = &m_region2Timing.m_start.m_lo;
    SYNC_PAIR(ar, mode, p);
    p = &m_region3Timing.m_start.m_lo;
    SYNC_PAIR(ar, mode, p);
    p = &m_bootyTiming.m_start.m_lo;
    SYNC_PAIR(ar, mode, p);
    return 1;
}
#undef SYNC_PAIR

RVA(0x000d79d0, 0x537)
i32 CPlay::SavePlayState(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* mc = m_world;
    if (mc == NULL) {
        return 0;
    }

    i32 count;

    s->Write(&m_returnToMenuOnComplete, sizeof(m_returnToMenuOnComplete));
    s->Write(&m_completedFinalLevel, sizeof(m_completedFinalLevel));
    s->Write(&m_savedClock, sizeof(m_savedClock));
    s->Write(&m_rngSeed, sizeof(m_rngSeed));
    s->Write(&m_dragInProgress, sizeof(m_dragInProgress));
    s->Write(&m_reserved2f0, sizeof(m_reserved2f0));
    s->Write(&m_cursorFrame, sizeof(m_cursorFrame));
    s->Write(&m_cursorId, sizeof(m_cursorId));
    s->Write(&m_cursorOffset, sizeof(m_cursorOffset));
    s->Write(&m_tileClick, sizeof(m_tileClick));
    s->Write(&m_dragInhibit1, sizeof(m_dragInhibit1));
    s->Write(&m_dragInhibit2, sizeof(m_dragInhibit2));

    count = StartMarkerCount();
    s->Write(&count, sizeof(count));
    for (u32 i0 = 0; i0 < static_cast<u32>(count); i0++) {
        s->Write(StartMarkerAt(i0), sizeof(*StartMarkerAt(i0)));
    }

    Anchor* p = m_anchors;
    for (i32 k0 = 4; k0 != 0; k0--) {
        s->Write(p, sizeof(*p));
        p++;
    }

    for (i32 k1 = 0; k1 < 4; k1++) {
        count = PlacedObjectCellCount(k1);
        s->Write(&count, sizeof(count));
        for (u32 i1 = 0; i1 < static_cast<u32>(count); i1++) {
            s->Write(PlacedObjectCellAt(k1, i1), sizeof(*PlacedObjectCellAt(k1, i1)));
        }
    }

    s->Write(&m_cueToggle, sizeof(m_cueToggle));

    g_serialCounter++;
    {
        char buf[0x200];
        memset(buf, 0, sizeof(buf));
        strcpy(buf, static_cast<const char*>(m_cueText));
        s->Write(buf, 0x200);
    }

    s->Write(&m_lastCueId, sizeof(m_lastCueId));
    s->Write(&g_lastLevelNum, sizeof(g_lastLevelNum));

    g_serialCounter++;
    {
        char buf[SERIAL_NAME_LEN];
        memset(buf, 0, sizeof(buf));

        CImage* frame = m_cursorImage;
        i32 v = 0;
        if (frame != NULL) {
            mc->m_imageRegistry->AnyValueMatches(frame, buf, &v);
        }
        s->Write(buf, SERIAL_NAME_LEN);
        s->Write(&v, sizeof(v));
    }

    g_serialCounter++;
    {
        char buf[SERIAL_NAME_LEN];
        memset(buf, 0, sizeof(buf));
        if (m_cursorSprite != NULL) {
            strcpy(buf, m_cursorSprite->m_name);
        }
        s->Write(buf, SERIAL_NAME_LEN);
    }

    s->Write(&m_cursorFrameDelayMs, sizeof(m_cursorFrameDelayMs));
    s->Write(&m_cursorFrameCountdownMs, sizeof(m_cursorFrameCountdownMs));
    s->Write(&m_cursorFrameIndex, sizeof(m_cursorFrameIndex));

    g_serialCounter++;
    {
        i32 v = 0;
        if (m_scrollSink != NULL) {
            v = m_scrollSink->m_objectId;
        }
        s->Write(&v, sizeof(v));
    }

    s->Write(&m_cursorAnimationActive, sizeof(m_cursorAnimationActive));
    s->Write(&m_renderDisabled, sizeof(m_renderDisabled));
    s->Write(&m_winLoseBanner, sizeof(m_winLoseBanner));
    s->Write(&m_initialFramePending, sizeof(m_initialFramePending));
    s->Write(&m_hudSuppressed, sizeof(m_hudSuppressed));
    s->Write(&m_inGame, sizeof(m_inGame));
    s->Write(&m_levelOverlayOpen, sizeof(m_levelOverlayOpen));
    s->Write(&m_paused, sizeof(m_paused));
    s->Write(&m_playerCommandPending, sizeof(m_playerCommandPending));
    s->Write(&m_cursorTargetValid, sizeof(m_cursorTargetValid));
    s->Write(&m_drewThisFrame, sizeof(m_drewThisFrame));
    s->Write(&m_pathPreviewSource.x, sizeof(m_pathPreviewSource.x));
    s->Write(&m_pathPreviewSource.y, sizeof(m_pathPreviewSource.y));
    s->Write(&m_pathPreviewDestination.x, sizeof(m_pathPreviewDestination.x));
    s->Write(&m_pathPreviewDestination.y, sizeof(m_pathPreviewDestination.y));
    s->Write(&m_pathPreviewColor, sizeof(m_pathPreviewColor));
    s->Write(&m_region0Gate, sizeof(m_region0Gate));
    s->Write(&m_region1Gate, sizeof(m_region1Gate));
    s->Write(&m_region2Gate, sizeof(m_region2Gate));
    s->Write(&m_region3Gate, sizeof(m_region3Gate));
    s->Write(&m_viewMode, sizeof(m_viewMode));
    s->Write(&m_defeatCountdownActive, sizeof(m_defeatCountdownActive));
    s->Write(&m_cursorUsesPlayerTint, sizeof(m_cursorUsesPlayerTint));
    s->Write(&m_cameraBookmarkIndex, sizeof(m_cameraBookmarkIndex));
    s->Write(&m_focusPlayerIndex, sizeof(m_focusPlayerIndex));

    count = CameraBookmarkCount();
    s->Write(&count, sizeof(count));
    for (i32 fi = 0; fi < CameraBookmarkCount(); fi++) {
        Coord* el = CameraBookmarkAt(fi);
        if (el != NULL) {
            s->Write(el, 8);
        }
    }

    return 1;
}

// @early-stop
// Frame is 0x294 against retail's 0x290 - one scalar cl still refuses to overlay -
// and retail parks &m_startMarkers / &m_placedObjectCells[k] in ebp across each
// record loop while cl rematerialises the `lea` and spills the loop counter instead.
// At the map48 lookup retail keeps the false return in eax at the merge and emits
// the null out-param as a separate fall-through; cl cross-jumps both zero arms.
// Nested/combined/ternary spellings are byte-identical, and reusing the out-param
// as the result is worse (docs/patterns/over-merge-is-decided-before-layout.md).
RVA(0x000d8060, 0x6ce)
i32 CPlay::LoadPlayState(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* res = g_gameReg->m_world;
    if (res == NULL) {
        return 0;
    }

    ar->Read(&m_returnToMenuOnComplete, sizeof(m_returnToMenuOnComplete));
    ar->Read(&m_completedFinalLevel, sizeof(m_completedFinalLevel));
    ar->Read(&m_savedClock, sizeof(m_savedClock));
    ar->Read(&m_rngSeed, sizeof(m_rngSeed));
    ar->Read(&m_dragInProgress, sizeof(m_dragInProgress));
    ar->Read(&m_reserved2f0, sizeof(m_reserved2f0));
    ar->Read(&m_cursorFrame, sizeof(m_cursorFrame));
    ar->Read(&m_cursorId, sizeof(m_cursorId));
    ar->Read(&m_cursorOffset, sizeof(m_cursorOffset));
    ar->Read(&m_tileClick, sizeof(m_tileClick));
    ar->Read(&m_dragInhibit1, sizeof(m_dragInhibit1));
    ar->Read(&m_dragInhibit2, sizeof(m_dragInhibit2));

    {

        for (i32 i = 0; i < StartMarkerCount(); i++) {
            Coord* node = StartMarkerAt(i);
            if (node) {
                CoordPoolNode* q = g_coordPool.NodeOf(node);
                q->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = q;
            }
        }
        // retail parks &m_startMarkers in ebp across the read loop (0xd8149
        // `lea ebp,[ebx+0x370]`, then `mov eax,[ebp+0x8]` for the count and
        // `mov ecx,ebp` for both calls) instead of rematerialising the lea.
        CPtrArray* markers = &m_startMarkers;
        markers->SetSize(0, -1);
        i32 n;
        ar->Read(&n, sizeof(n));
        for (u32 j = 0; j < static_cast<u32>(n); j++) {
            Coord* node = NULL;
            CoordPoolNode* head = g_coordPool.m_freeHead;
            CoordPoolNode* next = head->m_next;
            if (next) {
                node = &head->m_coord;
                g_coordPool.m_freeHead = next;
            }
            ar->Read(node, sizeof(*node));
            markers->SetAtGrow(markers->GetSize(), node);
        }
    }

    {

        Anchor* q = m_anchors;
        for (i32 k = 4; k != 0; k--) {
            ar->Read(q, sizeof(*q));
            q++;
        }
    }

    {

        // ONE cursor, as retail: 0xd81c0 `lea ebp,[ebx+0x3ac]` (biased to
        // &elem.m_nSize) and `lea ecx,[ebp-0x8]` recomputed at each call.  Mixing a
        // cached `CPtrArray*` with the subscript form gave cl two induction
        // variables and it spilled both.
        for (i32 k = 0; k < 4; k++) {
            for (i32 i = 0; i < PlacedObjectCellCount(k); i++) {
                Coord* node = PlacedObjectCellAt(k, i);
                if (node) {
                    CoordPoolNode* q = g_coordPool.NodeOf(node);
                    q->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = q;
                }
            }
            m_placedObjectCells[k].SetSize(0, -1);
            i32 n;
            ar->Read(&n, sizeof(n));
            for (u32 j = 0; j < static_cast<u32>(n); j++) {
                Coord* node = NULL;
                CoordPoolNode* head = g_coordPool.m_freeHead;
                CoordPoolNode* next = head->m_next;
                if (next) {
                    node = &head->m_coord;
                    g_coordPool.m_freeHead = next;
                }
                ar->Read(node, sizeof(*node));
                m_placedObjectCells[k].SetAtGrow(PlacedObjectCellCount(k), node);
            }
        }
    }

    ar->Read(&m_cueToggle, sizeof(m_cueToggle));
    g_serialCounter++;
    {
        char buf512[0x200];
        ar->Read(buf512, 0x200);
        m_cueText = buf512;
    }
    ar->Read(&m_lastCueId, sizeof(m_lastCueId));
    ar->Read(&g_lastLevelNum, sizeof(g_lastLevelNum));

    // ONE 0x80 name buffer, reused for both records: retail's frame is 0x290 =
    // 0x200 (m_cueText scratch) + 0x80 (this) + 0x10, so the second record does
    // not get a slot of its own.
    g_serialCounter++;
    char nameBuf[SERIAL_NAME_LEN];
    ar->Read(nameBuf, SERIAL_NAME_LEN);
    {
        i32 idx;
        ar->Read(&idx, sizeof(idx));
        // Positive gate: retail's `je` at 0xd82be reaches PAST the lookup to a sunk
        // `m_cursorImage = NULL`, i.e. the non-empty name is the FALL-THROUGH.
        if (strlen(nameBuf) != 0) {
            CObject* found = NULL;
            res->m_imageRegistry->m_workersByName.Lookup(static_cast<const char*>(nameBuf), found);
            CDDrawWorker* set = static_cast<CDDrawWorker*>(found);
            if (set == NULL || DDRAW_WORKER_FRAME_OUT_OF_RANGE(set, idx)) {
                m_cursorImage = NULL;
            } else {
                m_cursorImage = DDRAW_WORKER_FRAME_AT_UNCHECKED(set, idx);
            }
        } else {
            m_cursorImage = NULL;
        }
    }

    g_serialCounter++;
    ar->Read(nameBuf, SERIAL_NAME_LEN);
    {
        CObject* found = NULL;
        if (strlen(nameBuf) != 0) {
            res->m_imageRegistry->m_workersByName.Lookup(nameBuf, found);
            m_cursorSprite = static_cast<CDDrawWorker*>(found);
        } else {
            m_cursorSprite = NULL;
        }

        ar->Read(&m_cursorFrameDelayMs, sizeof(m_cursorFrameDelayMs));
        ar->Read(&m_cursorFrameCountdownMs, sizeof(m_cursorFrameCountdownMs));
        ar->Read(&m_cursorFrameIndex, sizeof(m_cursorFrameIndex));
        g_serialCounter++;
        ar->Read(&found, sizeof(found));

        CGameObject* oe = NULL;
        CWwdGameObjectA* sink;
        if (MapLookup(
                res->m_childGroup->m_registeredGameObjectsById,
                static_cast<void*>(found),
                oe
            )) {
            if (oe == NULL) {
                sink = NULL;
            } else {
                sink = oe->GetClassId() == CLASSID_SERIALREF ? static_cast<CWwdGameObjectA*>(oe)
                                                             : NULL;
            }
        } else {
            sink = NULL;
        }
        m_scrollSink = sink;
        if (sink == NULL && found != NULL) {
            return 0;
        }
    }

    ar->Read(&m_cursorAnimationActive, sizeof(m_cursorAnimationActive));
    ar->Read(&m_renderDisabled, sizeof(m_renderDisabled));
    ar->Read(&m_winLoseBanner, sizeof(m_winLoseBanner));
    ar->Read(&m_initialFramePending, sizeof(m_initialFramePending));
    ar->Read(&m_hudSuppressed, sizeof(m_hudSuppressed));
    ar->Read(&m_inGame, sizeof(m_inGame));
    ar->Read(&m_levelOverlayOpen, sizeof(m_levelOverlayOpen));
    ar->Read(&m_paused, sizeof(m_paused));
    ar->Read(&m_playerCommandPending, sizeof(m_playerCommandPending));
    ar->Read(&m_cursorTargetValid, sizeof(m_cursorTargetValid));
    ar->Read(&m_drewThisFrame, sizeof(m_drewThisFrame));
    ar->Read(&m_pathPreviewSource.x, sizeof(m_pathPreviewSource.x));
    ar->Read(&m_pathPreviewSource.y, sizeof(m_pathPreviewSource.y));
    ar->Read(&m_pathPreviewDestination.x, sizeof(m_pathPreviewDestination.x));
    ar->Read(&m_pathPreviewDestination.y, sizeof(m_pathPreviewDestination.y));
    ar->Read(&m_pathPreviewColor, sizeof(m_pathPreviewColor));
    ar->Read(&m_region0Gate, sizeof(m_region0Gate));
    ar->Read(&m_region1Gate, sizeof(m_region1Gate));
    ar->Read(&m_region2Gate, sizeof(m_region2Gate));
    ar->Read(&m_region3Gate, sizeof(m_region3Gate));
    ar->Read(&m_viewMode, sizeof(m_viewMode));
    ar->Read(&m_defeatCountdownActive, sizeof(m_defeatCountdownActive));
    ar->Read(&m_cursorUsesPlayerTint, sizeof(m_cursorUsesPlayerTint));
    ar->Read(&m_cameraBookmarkIndex, sizeof(m_cameraBookmarkIndex));
    m_stepCountdown = 2;
    ar->Read(&m_focusPlayerIndex, sizeof(m_focusPlayerIndex));

    {
        i32 n488;
        ar->Read(&n488, sizeof(n488));
        for (i32 i = 0; i < CameraBookmarkCount(); i++) {
            Coord* node = CameraBookmarkAt(i);
            if (node) {
                CoordPoolNode* q = g_coordPool.NodeOf(node);
                q->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = q;
            }
        }
        m_cameraBookmarks.SetSize(0, -1);
        m_cameraBookmarks.SetSize(n488, -1);
        for (u32 j = 0; j < static_cast<u32>(n488); j++) {
            Coord* node = NULL;
            CoordPoolNode* head = g_coordPool.m_freeHead;
            CoordPoolNode* next = head->m_next;
            if (next) {
                node = &head->m_coord;
                g_coordPool.m_freeHead = next;
            }
            ar->Read(node, 8);
            SetCameraBookmarkAt(j, node);
        }
    }
    return 1;
}

RVA(0x000d88f0, 0x44)
void CPlay::RegionEnter() {
    if (m_savedMusicSequence == NULL) {
        CGruntzMgr* gameManager = m_mgr;
        m_savedMusicSequence = gameManager->m_midi->m_currentSequence;
        gameManager->m_midi->PauseCurrent();
    }
    if (g_gameReg->m_musicEnabled != 0) {
        m_mgr->m_midi->PlaySequence("CURSE", 0);
    }
}

RVA(0x000d8960, 0x75)
void CPlay::RegionLeave() {
    if (m_region0Gate == 0 && m_region1Gate == 0 && m_region2Gate == 0 && m_region3Gate == 0
        && m_savedMusicSequence != NULL) {
        m_mgr->m_midi->EndCurrent();
        m_mgr->m_midi->m_currentSequence = m_savedMusicSequence;
        if (g_gameReg->m_musicEnabled != 0) {
            m_mgr->m_midi->RestartCurrent(1);
        }
        m_savedMusicSequence = NULL;
    }
}

RVA(0x000d8a00, 0x73)
i32 CPlay::SetTinyViewportCurse(i32 active) {
    if (active != 0) {
        m_region0Gate = 1;
        RegionEnter();
        m_viewMode = VIEW_MODE_A;
    } else {
        m_region0Gate = 0;
        RegionLeave();
        m_viewMode = VIEW_MODE_B;
    }
    m_region0Timing.m_interval.m_lo = REGION_INTERVAL_MS;
    m_region0Timing.m_interval.m_hi = 0;
    m_region0Timing.m_start.m_v = g_frameTime;
    return 1;
}

RVA(0x000d8aa0, 0x5f)
i32 CPlay::SetDarknessCurse(i32 active) {
    if (active != 0) {
        m_region1Gate = 1;
        RegionEnter();
    } else {
        m_region1Gate = 0;
        RegionLeave();
    }
    m_region1Timing.m_interval.m_lo = REGION_INTERVAL_MS;
    m_region1Timing.m_interval.m_hi = 0;
    m_region1Timing.m_start.m_v = g_frameTime;
    return 1;
}

RVA(0x000d8b20, 0x74)
i32 CPlay::SetMonitorCurse(i32 active) {
    if (active != 0) {
        m_region2Gate = 1;
        RegionEnter();
        Cmd_ApplyScrollParams(REGION_INTERVAL_MS, 6, 6, 0, 0x2d);
    } else {
        m_region2Gate = 0;
        RegionLeave();
    }
    m_region2Timing.m_interval.m_lo = REGION_INTERVAL_MS;
    m_region2Timing.m_interval.m_hi = 0;
    m_region2Timing.m_start.m_v = g_frameTime;
    return 1;
}

RVA(0x000d8bc0, 0x71)
i32 CPlay::SetRandomMoveIconsCurse(i32 active) {
    if (active != 0) {
        m_region3Gate = 1;
        RegionEnter();
    } else {
        m_region3Gate = 0;
        RegionLeave();
        g_gameReg->m_cmdGrid->CycleMoveIcons(-1, 0);
    }
    m_region3Timing.m_interval.m_lo = REGION_INTERVAL_MS;
    m_region3Timing.m_interval.m_hi = 0;
    m_region3Timing.m_start.m_v = g_frameTime;
    return 1;
}

RVA(0x000d8c60, 0xea)
i32 CPlay::ResetViewport() {
    CGruntzMgr* w = m_mgr;
    tagSIZE mode = w->GetModeSize();
    i32 right = mode.cx;
    StatusBarDock state = m_guts->m_position;
    i32 bottom = mode.cy;
    RECT r;
    if (state == STATUSBAR_DOCK_LEFT) {
        SetRect(&r, STATUSBAR_WIDTH_PX, 0, right - 1, bottom - 1);
    } else if (state == STATUSBAR_DOCK_RIGHT) {
        SetRect(&r, 0, 0, right - (STATUSBAR_WIDTH_PX + 1), bottom - 1);
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
    m_world->m_level->BuildAllPlanes((&r));
    m_mgr->RecomputeViewScale();
    return 1;
}

RVA(0x000d8d90, 0x1e)
i32 CPlay::StepViewportResize() {
    PlayViewMode mode = m_viewMode;
    if (mode == VIEW_MODE_IDLE) {
        return 0;
    }
    if (mode == VIEW_MODE_A) {
        return ClampViewport(4);
    }
    return ClampViewport2(4);
}

RVA(0x000d8dc0, 0xce)
i32 CPlay::ClampViewport(i32 inset) {
    CDDrawSurfaceMgr* v = m_world;
    i32 clamped = 0;
    LevelCoordRect* vp = &v->m_level->m_planeCtx;
    RECT r = *vp;

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

    m_world->m_level->BuildAllPlanes((&r));
    m_world->m_drawTarget->m_backPair->m_surface->Fill(0);
    m_guts->Deactivate();
    m_mgr->RecomputeViewScale();
    return 1;
}

// @early-stop
// One scheduling position, then a full register renaming cascade: retail
// materialises all three head pointers in declaration order and compares
// m_position straight from memory (`cmp [ebp+0],2`), while cl sinks the m_guts
// load to its use and loads the field VALUE first.
// MEASURED AND REJECTED: an `i32 lim` if/else in place of the ternary does buy the
// memory compare, but not the hoist, and costs more than it buys.  Dropping the
// `guts` local entirely is byte-identical (the local is codegen-neutral).  Using
// v/w/guts in the tail instead of re-reading the members is far worse - retail
// re-reads them, so the current spelling is confirmed, not merely untested.
RVA(0x000d8ed0, 0x128)
i32 CPlay::ClampViewport2(i32 stride) {
    i32 clamped = 0;
    CDDrawSurfaceMgr* v = m_world;
    CGruntzMgr* w = m_mgr;
    CStatusBarMgr* guts = m_guts;

    LevelCoordRect* rp = &v->m_level->m_planeCtx;
    RECT r = *rp;

    SIZE
    limit;
    limit.cx = w->m_modeSize.cx;
    limit.cy = w->m_modeSize.cy;

    if (r.right - r.left
        < (guts->m_position == STATUSBAR_HIDDEN ? limit.cx : limit.cx - STATUSBAR_WIDTH_PX)) {
        r.left -= stride;
        r.right += stride;
        if (r.left < 0) {
            r.left = 0;
        }
        if (r.right >= limit.cx) {
            r.right = limit.cx - 1;
        }
        clamped = 1;
    }
    if (r.bottom - r.top < limit.cy) {
        r.top -= stride;
        r.bottom += stride;
        if (r.top < 0) {
            r.top = 0;
        }
        if (r.bottom >= limit.cy) {
            r.bottom = limit.cy - 1;
        }
        clamped = 1;
    }

    if (clamped == 0) {
        ResetViewport();
        return 0;
    }

    m_world->m_level->BuildAllPlanes((&r));
    m_world->m_drawTarget->m_backPair->m_surface->Fill(0);
    m_guts->Deactivate();
    m_mgr->RecomputeViewScale();
    return 1;
}

RVA(0x000d9050, 0xc7)
i32 CPlay::NotifyVisibleEntities() {
    CDDrawSurfaceMgr* v = m_world;
    const LevelCoordRect& vp = v->m_level->m_planeCtx;
    CDDrawSurfacePair* held = v->m_drawTarget->m_backPair;
    CObList& chain = v->m_childGroup->m_list;

    RECT r = vp;
    r.right = r.right + 1;
    r.bottom = r.bottom + 1;
    held->m_surface->Restore(&r, 0);

    POSITION pos = chain.GetHeadPosition();

    while (pos != NULL) {
        CGameObject* o = NEXT_CHILD_FROM_LIST(chain, pos);
        LogicRecordDispatchFn dispatch = o->m_logicRecord->m_dispatch;
        if (dispatch == DispatchGruntLogic || dispatch == DispatchInGameIconLogic
            || dispatch == DispatchGruntPuddleLogic || dispatch == DispatchGruntToySpriteLogic
            || dispatch == DispatchGruntStaminaSpriteLogic
            || dispatch == DispatchGruntToyTimeSpriteLogic
            || dispatch == DispatchGruntWingzTimeSpriteLogic
            || dispatch == DispatchGruntHealthSpriteLogic
            || dispatch == DispatchGruntSelectedSpriteLogic
            || dispatch == DispatchGruntPowerupSpriteLogic
            || dispatch == DispatchStatusBarSpriteLogic || dispatch == DispatchLightFxLogic) {
            o->Render(held);
        }
    }
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

RVA(0x000d9240, 0x3c)
i32 CPlay::SetDefeatCountdown(i32 active, i32 durationMs) {
    if (active != 0) {

        m_defeatCountdownTiming.m_interval.m_lo = durationMs;
        m_defeatCountdownTiming.m_interval.m_hi = 0;
        m_defeatCountdownTiming.m_start.m_v = static_cast<u32>(g_frameTime);
    }
    m_defeatCountdownActive = active;
    return 1;
}

// @early-stop
// SIB base/index transposition in the three inlined CByteArray::GetAt loads
// ([edx+ecx] vs retail's [ecx+edx*1]); operator[]/ElementAt spellings are
// byte-identical to GetAt here. Everything else is instruction-for-instruction.
RVA(0x000d9290, 0x2a7)
i32 CPlay::ScanShuffleQuads() {
    CDDrawSurfaceMgr* v = m_world;

    CObList* pl = &v->m_childGroup->m_list;
    if (pl == NULL) {
        return 0;
    }
    POSITION pos = pl->GetHeadPosition();

    i32 perm[4];
    CByteArray arr;
    arr.SetAtGrow(arr.GetSize(), 0);
    arr.SetAtGrow(arr.GetSize(), 1);
    arr.SetAtGrow(arr.GetSize(), 2);
    arr.SetAtGrow(arr.GetSize(), 3);
    // retail keeps the degenerate empty-bag arm at every pick: cl cannot prove
    // count != 0, so both rand() calls survive.
    i32 last;
    i32 count;
    i32 r;
    last = arr.GetUpperBound();
    count = last + 1;
    if (count == 0) {
        r = (rand() & 1) != 0 ? 0 : last;
    } else {
        r = rand() % count;
    }
    perm[0] = arr.GetAt(r);
    arr.RemoveAt(r, 1);
    last = arr.GetUpperBound();
    count = last + 1;
    if (count == 0) {
        r = (rand() & 1) != 0 ? 0 : last;
    } else {
        r = rand() % count;
    }
    perm[1] = arr.GetAt(r);
    arr.RemoveAt(r, 1);
    last = arr.GetUpperBound();
    count = last + 1;
    if (count == 0) {
        r = (rand() & 1) != 0 ? 0 : last;
    } else {
        r = rand() % count;
    }
    perm[2] = arr.GetAt(r);
    arr.RemoveAt(r, 1);
    perm[3] = arr.GetAt(0);
    arr.RemoveAt(0, 1);

    while (pos != NULL) {
        CGameObject* p = static_cast<CGameObject*>(pl->GetNext(pos));
        if (p == NULL) {
            continue;
        }
        LogicRecordDispatchFn dispatch = p->m_logicRecord->m_dispatch;
        if (dispatch == DispatchGruntCreationPointLogic || dispatch == DispatchExitTriggerLogic
            || dispatch == DispatchFortressFlagLogic || dispatch == DispatchWayPointLogic
            || dispatch == DispatchGuardPointLogic) {
            p->m_smarts = perm[p->m_smarts];
        } else if (dispatch == DispatchBrickzLogic) {
            if (p->m_extent.left == COORD_UNSET) {
                p->m_extent.left = 0;
            }
            if (p->m_area.left == COORD_UNSET) {
                p->m_area.left = 0;
            }
            if (p->m_switchRect.left == COORD_UNSET) {
                p->m_switchRect.left = 0;
            }
            if (p->m_clip.left == COORD_UNSET) {
                p->m_clip.left = 0;
            }

            i32 scatter[4];
            scatter[perm[0]] = p->m_extent.left;
            scatter[perm[1]] = p->m_extent.top;
            scatter[perm[2]] = p->m_extent.right;
            scatter[perm[3]] = p->m_extent.bottom;
            p->m_extent.left = scatter[0];
            p->m_extent.top = scatter[1];
            p->m_extent.right = scatter[2];
            p->m_extent.bottom = scatter[3];
        }
    }
    return 1;
}

RVA(0x000d95f0, 0x830)
i32 CPlay::DrawLevelInfoText() {
    CString s0;
    CString s1;
    CString s2;
    CString s3;

    switch (m_levelType) {
        case AREA_ROCKY_ROADZ:
            s0.LoadString(IDS_AREA1_TITLE);
            break;
        case AREA_GRUNTZICLEZ:
            s0.LoadString(IDS_AREA2_TITLE);
            break;
        case AREA_TROUBLE_IN_THE_TROPICZ:
            s0.LoadString(IDS_AREA3_TITLE);
            break;
        case AREA_HIGH_ON_SWEETZ:
            s0.LoadString(IDS_AREA4_TITLE);
            break;
        case AREA_HIGH_ROLLERZ:
            s0.LoadString(IDS_AREA5_TITLE);
            break;
        case AREA_HONEY_I_SHRUNK_THE_GRUNTZ:
            s0.LoadString(IDS_AREA6_TITLE);
            break;
        case AREA_MINIATURE_MASTERZ:
            s0.LoadString(IDS_AREA7_TITLE);
            break;
        case AREA_GRUNTZ_IN_SPACE:
            s0.LoadString(IDS_AREA8_TITLE);
            break;
        default:
            s0 = "";
    }

    GameModeId mode = g_gameReg->m_gameMode;
    if (mode == GAMEMODE_SINGLE) {
        if (g_gameReg->m_isCustomLevel != 0) {
            s1.LoadString(IDS_CUSTOM_QUEST_LEVEL);
        } else {
            i32 stage = m_levelIndex;
            if (stage > IDX(QUESTLEVEL_LAST)) {
                switch (CurrentQuestLevel()) {
                    case QUESTLEVEL_TRAINING_FIRST:
                        s1.LoadString(IDS_TRAINING_STAGE1);
                        break;
                    case QUESTLEVEL_TRAINING_STAGE2:
                        s1.LoadString(IDS_TRAINING_STAGE2);
                        break;
                    case QUESTLEVEL_TRAINING_STAGE3:
                        s1.LoadString(IDS_TRAINING_STAGE3);
                        break;
                    case QUESTLEVEL_TRAINING_LAST:
                        s1.LoadString(IDS_TRAINING_STAGE4);
                        break;
                    default:
                        s1 = "";
                }
            } else {
                s1.Format("Stage %d", ((stage - 1) % QUESTLEVEL_PER_AREA) + 1);
            }
            switch (CurrentQuestLevel()) {
                case QUESTLEVEL_AREA1_STAGE1:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA1_STAGE1);
                    break;
                case QUESTLEVEL_AREA1_STAGE2:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA1_STAGE2);
                    break;
                case QUESTLEVEL_AREA1_STAGE3:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA1_STAGE3);
                    break;
                case QUESTLEVEL_AREA1_STAGE4:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA1_STAGE4);
                    break;
                case QUESTLEVEL_AREA2_STAGE1:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA2_STAGE1);
                    break;
                case QUESTLEVEL_AREA2_STAGE2:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA2_STAGE2);
                    break;
                case QUESTLEVEL_AREA2_STAGE3:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA2_STAGE3);
                    break;
                case QUESTLEVEL_AREA2_STAGE4:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA2_STAGE4);
                    break;
                case QUESTLEVEL_AREA3_STAGE1:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA3_STAGE1);
                    break;
                case QUESTLEVEL_AREA3_STAGE2:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA3_STAGE2);
                    break;
                case QUESTLEVEL_AREA3_STAGE3:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA3_STAGE3);
                    break;
                case QUESTLEVEL_AREA3_STAGE4:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA3_STAGE4);
                    break;
                case QUESTLEVEL_AREA4_STAGE1:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA4_STAGE1);
                    break;
                case QUESTLEVEL_AREA4_STAGE2:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA4_STAGE2);
                    break;
                case QUESTLEVEL_AREA4_STAGE3:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA4_STAGE3);
                    break;
                case QUESTLEVEL_AREA4_STAGE4:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA4_STAGE4);
                    break;
                case QUESTLEVEL_AREA5_STAGE1:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA5_STAGE1);
                    break;
                case QUESTLEVEL_AREA5_STAGE2:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA5_STAGE2);
                    break;
                case QUESTLEVEL_AREA5_STAGE3:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA5_STAGE3);
                    break;
                case QUESTLEVEL_AREA5_STAGE4:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA5_STAGE4);
                    break;
                case QUESTLEVEL_AREA6_STAGE1:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA6_STAGE1);
                    break;
                case QUESTLEVEL_AREA6_STAGE2:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA6_STAGE2);
                    break;
                case QUESTLEVEL_AREA6_STAGE3:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA6_STAGE3);
                    break;
                case QUESTLEVEL_AREA6_STAGE4:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA6_STAGE4);
                    break;
                case QUESTLEVEL_AREA7_STAGE1:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA7_STAGE1);
                    break;
                case QUESTLEVEL_AREA7_STAGE2:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA7_STAGE2);
                    break;
                case QUESTLEVEL_AREA7_STAGE3:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA7_STAGE3);
                    break;
                case QUESTLEVEL_AREA7_STAGE4:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA7_STAGE4);
                    break;
                case QUESTLEVEL_AREA8_STAGE1:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA8_STAGE1);
                    break;
                case QUESTLEVEL_AREA8_STAGE2:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA8_STAGE2);
                    break;
                case QUESTLEVEL_AREA8_STAGE3:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA8_STAGE3);
                    break;
                case QUESTLEVEL_AREA8_STAGE4:
                    s2.LoadString(IDS_LEVEL_TITLE_AREA8_STAGE4);
                    break;
                default:
                    s2.Format("");
                    break;
                case QUESTLEVEL_TRAINING_STAGE1:
                    s2.LoadString(IDS_LEVEL_TITLE_TRAINING_STAGE1);
                    break;
                case QUESTLEVEL_TRAINING_STAGE2:
                    s2.LoadString(IDS_LEVEL_TITLE_TRAINING_STAGE2);
                    break;
                case QUESTLEVEL_TRAINING_STAGE3:
                    s2.LoadString(IDS_LEVEL_TITLE_TRAINING_STAGE3);
                    break;
                case QUESTLEVEL_TRAINING_STAGE4:
                    s2.LoadString(IDS_LEVEL_TITLE_TRAINING_STAGE4);
            }
            if (g_levelBias100 != 0) {
                s1.LoadString(IDS_SECRET_LEVEL_STAGE);
                s2.LoadString(IDS_SECRET_LEVEL_TITLE);
            }
        }
    } else if (mode == GAMEMODE_REPLAY) {
        if (g_gameReg->m_isCustomLevel != 0) {
            s1.LoadString(IDS_CUSTOM_BATTLEZ_LEVEL);
        } else {
            s1.LoadString(IDS_BATTLEZ_LEVEL);
        }
    } else if (mode == GAMEMODE_MULTIPLAYER) {
        if (g_gameReg->m_isCustomLevel != 0) {
            s1.LoadString(IDS_CUSTOM_MULTIPLAYER_LEVEL);
        } else {
            s1.LoadString(IDS_MULTIPLAYER_LEVEL);
        }
    } else {
        s0.Format("");
        s2.Format("");
        s1.Format("");
    }

    if ((g_gameReg)->GetWorldFileName().GetLength() != 0) {
        char buf[128];
        wsprintfA(buf, (g_gameReg)->GetWorldFileName());
        if (strchr(buf, '.')) {
            *strchr(buf, '.') = 0;
        }
        if (strrchr(buf, '\\') != NULL) {
            s2 = strrchr(buf, '\\') + 1;
        } else {
            s2 = buf;
        }
    }

    s3.LoadString(IDS_LOADING);

    RECT r1;
    RECT r2;
    RECT r3;
    RECT r4;
    SetRect(&r1, 0, 0, SCREEN_W_PX, 0x38);
    SetRect(&r2, 0, 0x2b, SCREEN_W_PX, 0x59);
    SetRect(&r3, 0, 0x176, SCREEN_W_PX, 0x1a2);
    SetRect(&r4, 0, 0x1b8, SCREEN_W_PX, SCREEN_H_PX);
    EngStr_DrawText(m_world, &s0, &r1, 0x78, 0, 0, 0, 0, 1);
    EngStr_DrawText(m_world, &s1, &r2, 0x6e, 0, 0, 0, 0, 1);
    EngStr_DrawText(m_world, &s2, &r3, 0x6e, 0, 0, 0, 0, 1);
    EngStr_DrawText(m_world, &s3, &r4, 0x6e, 0, 0, 0, 0, 1);
    return 1;
}

// @early-stop
// Retail never materialises `result = 0`: the failed-lookup path leaves eax zero from
// its own `test eax,eax`, so cl reuses it (0xda0b5 `je` skips only the `mov eax,[out]`).
// cl here re-materialises the constant into ecx instead.  Splitting the lookup into a
// named BOOL is what recovers retail's two-`test` branch shape at all - fused into the
// `if`, cl lowers the select branchlessly as `neg/sbb/and`.
RVA(0x000da030, 0x169)
i32 CPlay::ClearPlacedObjects() {
    for (i32 blockIdx = 0; blockIdx < 4; ++blockIdx) {
        i32 i = 0;
        i32 done = 0;
        while (!done) {
            if (i < PlacedObjectCellCount(blockIdx)) {
                Coord* obj = PlacedObjectCellAt(blockIdx, i);
                CMapMgr* grid = g_gameReg->m_tileGrid;

                i32 occupantId;
                i32 cellX = obj->m_x;
                i32 cellY = obj->m_y;
                if (static_cast<u32>(cellX) < static_cast<u32>(grid->m_width)
                    && static_cast<u32>(cellY) < static_cast<u32>(grid->m_height)) {
                    occupantId = grid->m_rows[cellY][cellX].m_objectId;
                } else {
                    occupantId = 0;
                }
                if (occupantId != 0) {
                    CGameObject* out = NULL;
                    BOOL found = MapLookupById(
                        g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
                        occupantId,
                        out
                    );
                    CGameObject* result = NULL;
                    if (found) {
                        result = out;
                    }
                    if (result == NULL) {

                        CMapMgr* g = g_gameReg->m_tileGrid;
                        i32 freeX = obj->m_x;
                        i32 freeY = obj->m_y;
                        if (static_cast<u32>(freeX) < static_cast<u32>(g->m_width)
                            && static_cast<u32>(freeY) < static_cast<u32>(g->m_height)) {
                            g->m_rows[freeY][freeX].m_objectId = 0;
                            g->m_rows[freeY][freeX].m_flags &= 0xfffbffff;
                        }
                        m_placedObjectCells[blockIdx].RemoveAt(i, 1);

                        CoordPoolNode* node = g_coordPool.NodeOf(obj);
                        node->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = node;
                        return -1;
                    }
                    if (result->m_smarts != IDX(PICKUP_WARPSTONE)) {
                        done = 1;
                    }
                } else {
                    done = 1;
                }
                ++i;
            } else {
                if (i > 0) {
                    return blockIdx;
                }
                done = 1;
            }
        }
    }
    return -1;
}

RVA(0x000da200, 0x9b)
i32 CPlay::GetAmbientId() {
    CGruntzMgr* gr = g_gameReg;
    if (gr->m_gameMode == GAMEMODE_SINGLE && gr->m_isCustomLevel == 0) {
        return (m_levelIndex + 1) % 2;
    }
    // One function-local static: guard bit 1 at retail 0x24c22c, datum 0x24c26c.
    DATA(0x0024c26c)
    static i32 s_ambientCoin = GetRandomNumber() % 2;
    return s_ambientCoin;
}

RVA(0x000da2d0, 0xa5)
i32 CPlay::FlushPendingOps() {
    if (m_playerCommandPending != 0) {
        return 0;
    }
    i32 changed = 0;
    if (m_dragInhibit1 != 0) {
        CStatusBarMgr* worker = m_guts;
        m_dragInhibit1 = 0;
        worker->CommitSlot(0);
        SetCursorFrame(0);
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
    LoadCursorSprites(0, 0);
    return changed;
}

RVA(0x000da3b0, 0x6e)
i32 CPlay::CanQuickSave() {
    if (m_renderDisabled == 0 && m_inGame == 0 && m_levelOverlayOpen == 0
        && m_defeatCountdownActive == 0 && m_guts->m_hlBusy == 0
        && m_guts->m_levelOverlayActive == 0 && m_guts->m_quitConfirmationActive == 0
        && g_gameReg->m_frameGate == 0 && g_gameReg->m_cmdGrid->m_groupFlag != 0) {
        return 1;
    }
    return 0;
}

RVA(0x000da440, 0x60)
i32 CPlay::PostHudRect() {
    if (m_worldReady != 0) {
        m_mgr->m_cmdGrid->HudRect(m_hudRect, g_gameplayInput->m_heldButtons & 0x20);
    }
    m_worldReady = 0;
    m_dragSnapActive = 0;
    return 1;
}
