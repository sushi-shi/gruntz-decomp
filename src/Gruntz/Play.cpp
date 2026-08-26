#include <rva.h>

#include <Gruntz/Play.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
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
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GameStats.h>
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
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MgrAutoScroll.h>
#include <Gruntz/Minimap.h>
#include <Gruntz/MovieEntryId.h>
#include <Gruntz/Multi.h>
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
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>
#include <Utils/MillisPer.h>
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
i32 g_deactivateProfileMs;
DATA(0x0024c288)
i32 g_flipProfileMs;

DATA(0x00212f78)
char* g_colorNames[] =
    {"Color 0", "Color 1", "Color 2", "Color 3", "Color 4", "Color 5", "Color 6", "Color 7"};
DATA(0x00212fc0)
char* g_difficultyNames[] = {"Easy", "Normal", "Hard"};

DATA(0x0024c3f0)
i32 g_playerColorAvailable[TINT_COUNT];

DATA(0x002455f0)
i32 g_levelBias100 = 0;

DATA(0x0024c020)
char g_customLevelText[0x200];

// @early-stop
RVA(0x000c7ec0, 0x5f5)
i32 CPlay::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {
    {
        if (mgr == NULL) {
            return 0;
        }
        GruntzPlayer* sub = mgr->m_players;
        if (sub == NULL) {
            return 0;
        }
        sub->m_active = 1;
        sub->m_humanControlled = 1;
        m_region0Gate = 0;
        m_region1Gate = 0;
        m_region2Gate = 0;
        m_region3Gate = 0;
        m_viewportResizeMode = VIEW_RESIZE_IDLE;
        m_hudSuppressed = true;
        m_cameraBookmarkIndex = -1;
        m_defeatCountdownActive = false;
        m_scrollEdgeActive = 0;
        m_scrollEdgeLock = 0;
        m_levelTimer = NULL;

        if (!CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId)) {
            return 0;
        }

        CChatBoxOwner* ctl = new CChatBoxOwner;
        m_chatBox = ctl;
        if (m_chatBox->Attach(m_world, m_mgr->m_chatLog) == 0) {
            CChatBoxOwner* dead = m_chatBox;
            if (dead == NULL) {
                return 0;
            }
            dead->Deactivate();
            ::operator delete(dead);
            m_chatBox = NULL;
            return 0;
        }
        m_chatBox->m_inputActive = 0;
        m_chatBox->Configure(CHATBOX_WITH_RIGHT_STATUSBAR);

        m_statusBar = new CStatusBarMgr;
        if (m_statusBar->LoadBattlezItemConfig(m_world) == 0) {
            if (m_statusBar == NULL) {
                return 0;
            }
            delete m_statusBar;
            m_statusBar = NULL;
            return 0;
        }

        CTileTriggerContainer* r78 = new CTileTriggerContainer;
        m_tileTriggers = r78;
        if (m_tileTriggers->Initialize() == 0) {
            if (m_tileTriggers == NULL) {
                return 0;
            }
            delete m_tileTriggers;
            m_tileTriggers = NULL;
            return 0;
        }

        CTimer* r50 = new CTimer;
        m_levelTimer = r50;
        if (r50 == NULL) {
            return 0;
        }

        while (ShowCursor(false) >= 0) {
        }
        m_initialFramePending = true;
        m_notifyLatch = 0;
        m_completedFinalLevel = false;
        memset(&m_saveSlot, 0, sizeof(m_saveSlot));
        mgr->ResetClockGlobals();
        m_savedClock = 0;
        m_rngSeed = timeGetTime();
        m_minimap = NULL;
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
        CWwdSpriteObject* peer = m_cursorSnapSprite;
        if (peer) {
            peer->m_stateFlags |= SPRITE_STATE_HIDDEN;
        }
        return 1;
    }
}

RVA(0x000c8700, 0x1f4)
void CPlay::ReleaseResources() {
    i32 i;

    CMinimap* minimap = m_minimap;
    if (minimap) {
        minimap->Reset();
        ::operator delete(minimap);
        m_minimap = NULL;
    }
    OnExit();
    if (m_mgr) {
        m_mgr->m_isBuiltInBattlezLevel = 0;
        m_mgr->m_strWorldFile.Empty();
    }
    m_saveSlot.m_type = 0;
    i32 t = 0;
    do {
        g_gameReg->m_players[t].m_active = 0;
        t++;
    } while (t < 4);
    if (m_mgr && m_mgr->m_chatLog) {
        m_mgr->m_chatLog->FreeNodes();
    }
    if (m_statusBar) {
        delete m_statusBar;
        m_statusBar = NULL;
    }
    CChatBoxOwner* hit = m_chatBox;
    if (hit) {
        hit->Deactivate();
        ::operator delete(hit);
        m_chatBox = NULL;
    }
    if (m_tileTriggers) {
        delete m_tileTriggers;
        m_tileTriggers = NULL;
    }
    CTimer* fm = m_levelTimer;
    if (fm) {
        fm->Reset();
        ::operator delete(fm);
        m_levelTimer = NULL;
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
    if (ShowCursor(false) >= 0) {
        do {
        } while (ShowCursor(false) >= 0);
    }
    if (previousState == GAMESTATE_HELP) {
        g_frameTime = m_savedClock;
        if (!EnterMode(GAMESTATE_HELP)) {
            return 0;
        }
        m_stepCountdown = 2;
    } else if (m_renderDisabled == false || m_mgr->m_gameMode == GAMEMODE_MULTIPLAYER) {
        if (!EnterMode(previousState)) {
            return 0;
        }
    }
    if (ShowCursor(false) >= 0) {
        do {
        } while (ShowCursor(false) >= 0);
    }
    m_dragSnapActive = false;
    m_dragInProgress = false;
    m_dragInhibit1 = false;
    m_dragInhibit2 = false;
    m_cursorTargetValid = false;
    m_worldReady = false;
    if (m_renderDisabled == false) {
        if (previousState != GAMESTATE_HELP) {
            m_mgr->m_worldSounds->Resume();
        }
        (static_cast<CTriggerMgr*>(m_mgr->m_triggerMgr))->DestroyAllAnims();
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
        DrawTextToOverlaySurface(m_world, &s, &r, 0x78, 1, 0xff, 0xff, 0, 1);
        RetireScene(0x50, 0x3e8, 0, 1);
        if (m_mgr && m_mgr->m_triggerMgr) {
            m_mgr->m_triggerMgr->RemovePlayerUnitsImmediately(TM_ALL_PLAYERS);
        }
    }
    return 1;
}

// @early-stop
RVA(0x000c8cf0, 0xc14)
i32 CPlay::Render() {

    m_drewThisFrame = false;
    HandleDragMove(0, m_cursorX, m_cursorY);

    if (m_renderDisabled != false) {
        return 1;
    }

    if (m_inGame != false) {

        RestoreCursorSaveUnder();
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
        m_mgr->m_worldSounds->SetListenerPosition(
            m_world->m_level->m_mainPlane->m_scrollPixelX,
            m_world->m_level->m_mainPlane->m_scrollPixelY
        );
        SoundStream* stream = m_world->m_soundStream;
        if (stream != NULL) {
            u32 t = timeGetTime();
            stream->TickVolumeRamps(t);
            stream->TickStreams(t);
        }
        m_tileTriggers->UpdateTimedLogics(g_frameDelta);
        m_statusBar->LoadMainStatusBarSprite();

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

        m_levelTimer->Tick(static_cast<i32>(g_frameDelta));
        m_levelTimer->Draw(back, 1);
        AdvanceCursorAnimation(static_cast<i32>(g_frameDelta));
        SaveUnderAndDrawCursor(back);
        m_world->m_drawTarget->m_frontSurface->m_surface->Flip(NULL);
        UpdateMgrScroll(g_gameReg, m_statusBar, m_region0Gate);
        m_world->m_level->DeactivateDistantObjectsOnMainPlane();
        return 1;
    }

    if (m_mgr->m_frameGate == 0
        && (g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER || m_levelOverlayOpen == false)) {
        m_levelTimer->Tick(static_cast<i32>(g_frameDelta));
        m_mgr->m_commandMgr->ExecuteScheduledCommands(0);

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

        RestoreCursorSaveUnder();
        StepViewportResize();

        if (m_ambientInitDone == false) {
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
                m_ambientInitDone = true;
            }
        }

        if (m_region0Gate != 0) {
            m_world->m_drawTarget->m_backPair->m_surface->Fill(0);
            m_statusBar->Deactivate();
        }

        if (m_worldReady == false) {
            if (m_mgr->m_triggerMgr->m_armed != 0) {
                m_mgr->m_triggerMgr->ScrollToActiveRecord();
            }
            LoadScrollSpeedOptions();
        }

        StepScroll();

        {
            u32 dt = g_frameDelta;
            if (dt > 0x12 && dt < 0xc8) {
                UpdateWorldFixedSteps();
            } else {
                UpdateWorldFrame();
            }
        }

        m_mgr->m_worldSounds->SetListenerPosition(
            m_world->m_level->m_mainPlane->m_scrollPixelX,
            m_world->m_level->m_mainPlane->m_scrollPixelY
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
        m_tileTriggers->UpdateTimedLogics(g_frameDelta);
        m_statusBar->LoadMainStatusBarSprite();
        m_mgr->m_tileGrid->UpdateDiagonals(m_mgr);

        if (m_minimap != NULL && m_statusBar->m_position != STATUSBAR_HIDDEN
            && m_statusBar->m_activeTab != TAB_GAME) {
            RECT rc;
            if (m_statusBar->m_position == STATUSBAR_DOCK_LEFT) {
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
            m_minimap->Refresh(static_cast<i32>(g_frameDelta), 0);
            m_minimap->Draw(m_world->m_drawTarget->m_backPair, &rc);
        }

        m_mgr->m_chatLog->Scroll(static_cast<i32>(g_frameDelta));
        CDDrawSurfacePair* view =
            static_cast<CDDrawSurfacePair*>(m_world->m_drawTarget->m_backPair);
        if (view == NULL) {
            return 0;
        }

        if (m_defeatCountdownActive != false) {
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

                if (m_statusBar->m_destructButtonLocked != 0) {
                    g_gameReg->m_triggerMgr->StartPlayerDefeatSequence(5);
                } else {
                    i32 row = g_curPlayer;
                    g_gameReg->m_triggerMgr->StartPlayerDefeatSequence(row);
                }

                CTimer* marker = m_levelTimer;
                marker->m_unusedStamp.m_lo = 0;
                marker->m_unusedStamp.m_hi = 0;
                marker->m_accum.m_lo = 0;
                marker->m_accum.m_hi = 0;
                marker->m_running = 0;
                marker->m_currentMs = 0;
                m_statusBar->LockDestructButton(0);
                m_defeatCountdownActive = false;

                if (g_gameReg->m_players[0].m_warlordObjectId != 0) {
                    CGameObject* out = NULL;
                    CGameObject* object = NULL;
                    if (MapLookupById(
                            g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
                            g_gameReg->m_players[0].m_warlordObjectId,
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
                RECT lvl = g_gameReg->m_world->m_level->m_viewportRect;
                RECT box;
                CopyRect(&box, &lvl);
                DrawTextToBackSurface(g_gameReg->m_world, &tmp, &box, 0x82, 1, 0xff, 0xff, 0, 1);
            }
        }

        m_levelTimer->Draw(view, 0);
        m_chatBox->LoadChatBoxSprite(view);
        DrawDebugStats();
        m_mgr->m_triggerMgr->RenderActionOptionsMenu();

        if (m_winLoseBanner != 0 && m_statusBar->m_levelOverlayActive == 0
            && m_statusBar->m_quitConfirmationActive == 0) {

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
        SaveUnderAndDrawCursor(view);
        if (m_worldReady != false) {
            view->DrawBox(&m_hudRect, 0xff);
        }
        m_world->m_drawTarget->m_frontSurface->m_surface->Flip(NULL);
        UpdateMgrScroll(g_gameReg, m_statusBar, m_region0Gate);
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

    RestoreCursorSaveUnder();
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
        if (m_paused != false) {

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
                m_statusBar->LoadMainStatusBarSprite();
                back->m_surface->ShadeRect(0x32, NULL);
                PlayCueAt(m_lastCueId, 0x78, 0, 0xff, 0xff, 0, 1, NULL);
                m_levelTimer->Draw(back, 1);
            }
            if (m_ambientInitDone == false) {
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
                    m_ambientInitDone = true;
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
            m_statusBar->LoadMainStatusBarSprite();
            if (m_statusBar->m_levelOverlayActive == 0
                && m_statusBar->m_quitConfirmationActive == 0) {
                PlayCueAt(0x812c, 0x78, 0, 0xff, 0xff, 0, 1, NULL);
            }
            m_levelTimer->Draw(back, 1);
        }
        AdvanceCursorAnimation(static_cast<i32>(g_frameDelta));
        SaveUnderAndDrawCursor(back);
        m_world->m_drawTarget->m_frontSurface->m_surface->Flip(NULL);
    }
    return 1;
}

RVA(0x000c9c20, 0x79)
void CPlay::UpdateWorldFrame() {
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
    m_mgr->m_triggerMgr->UpdateFrame(static_cast<i32>(g_frameDelta));
    if (g_gameReg->m_gameMode == GAMEMODE_BATTLEZ) {

        (g_gameReg)->AdvanceComputerPlayerTurns();
    }
    m_statusBar->UpdateStatusBar(static_cast<i32>(g_frameDelta));
}

// @early-stop
RVA(0x000c9cc0, 0x12e)
i32 CPlay::UpdateWorldFixedSteps() {
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
            m_mgr->m_triggerMgr->UpdateFrame(static_cast<i32>(g_frameDelta));
            if (g_gameReg->m_gameMode == GAMEMODE_BATTLEZ) {
                (g_gameReg)->AdvanceComputerPlayerTurns();
            }
            m_statusBar->UpdateStatusBar(static_cast<i32>(g_frameDelta));
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
    m_mgr->m_worldSounds->SetListenerPosition(
        m_world->m_level->m_mainPlane->m_scrollPixelX,
        m_world->m_level->m_mainPlane->m_scrollPixelY
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
    m_mgr->m_triggerMgr->UpdateFrame(static_cast<i32>(g_frameDelta));
    m_statusBar->UpdateStatusBar(static_cast<i32>(g_frameDelta));
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
    m_statusBar->LoadMainStatusBarSprite();
    statusBarMs = static_cast<i32>(tg() - static_cast<u32>(statusBarMs));

    g_brickText1.Format(
        "Input=%i, Activate=%i, Deact=%i, Update=%i, HitTest=%i, Draw=%i, Fixed=%i, "
        "StatusBar=%i, Flip=%i  ",
        activateMs,
        deactMs,
        g_deactivateProfileMs,
        updateMs,
        hitTestMs,
        drawMs,
        fixedMs,
        statusBarMs,
        g_flipProfileMs
    );

    DrawDebugStats();
    g_flipProfileMs = static_cast<i32>(tg());
    m_world->m_drawTarget->m_frontSurface->m_surface->Flip(NULL);
    g_flipProfileMs = static_cast<i32>((tg() - static_cast<u32>(g_flipProfileMs)));
    g_deactivateProfileMs = static_cast<i32>(tg());
    {
        CGameLevel* lvl = m_world->m_level;
        if (lvl->m_mainPlane != NULL) {
            lvl->m_mainPlane->DeactivateDistantObjects();
        }
    }
    g_deactivateProfileMs = static_cast<i32>((tg() - static_cast<u32>(g_deactivateProfileMs)));
    UpdateMgrScroll(g_gameReg, m_statusBar, m_region0Gate);
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
        updates = UpdateWorldFixedSteps();
    } else {
        UpdateWorldFrame();
    }
    i32 renderMs = static_cast<i32>((tg() - t0));
    m_mgr->m_worldSounds->SetListenerPosition(
        m_world->m_level->m_mainPlane->m_scrollPixelX,
        m_world->m_level->m_mainPlane->m_scrollPixelY
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
    m_world->m_drawTarget->m_frontSurface->m_surface->Flip(NULL);

    CGameLevel* lvl = m_world->m_level;
    if (lvl->m_mainPlane != NULL) {
        lvl->m_mainPlane->DeactivateDistantObjects();
    }
    return 1;
}

// @early-stop
RVA(0x000ca200, 0xe54)
i32 CPlay::LoadByMode(i32 level, i32) {
    CPlay* self = this;
    CGruntzMgr* gameReg;
    CRezArchiveDir* bank;
    CRezArchiveDir* prevTiles;
    i32 reload = 0;
    i32 diff = 0;

    char nameBuf[0x20];
    i32 initScratch[0x25];

    self->m_hudSuppressed = true;
    g_frameDelta = 0;
    g_lastNow = 0;
    g_frameTime = 0;
    g_levelBias100 = 0;
    if (level > 0x64) {
        level -= 0x64;
        g_levelBias100 = 1;
    }

    CTimer* worker = self->m_levelTimer;
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

    for (i32 a = 0; a < 4; ++a) {
        self->m_anchors[a].m_x = -1;
        self->m_anchors[a].m_y = -1;
    }

    g_soundCueTimeMs = g_lastNow;
    g_engineFrameDelta = g_frameDelta;

    for (i32 t = 0; t < 4; ++t) {
        CGruntzMgr* mgr = self->m_mgr;
        gameReg = g_gameReg;
        GruntzPlayer* team = &mgr->m_players[t];
        if (gameReg->m_gameMode == GAMEMODE_QUESTZ) {
            team->SeedForSlot(t);
            if (t == 0) {
                team->m_active = 1;
                team->m_joined = 1;
            }
        } else {
            team->m_doneFlag = 0;
            team->m_joined = team->m_active;
            team->m_clearedRound = 0;
        }
    }

    i32 modeFlag = (Update() == GAMESTATE_MULTI) ? 1 : 0;
    CMulti* savedThis = modeFlag ? static_cast<CMulti*>(self) : NULL;
    self->m_initialFramePending = true;
    self->m_levelIndex = level;
    {
        i32 r = (level - 1) % 0x24;
        self->m_levelType = static_cast<LevelArea>(r / 4 + 1);
    }

    g_frameTime = 0;
    if (g_gameReg->m_gameMode == GAMEMODE_BATTLEZ) {
        srand(timeGetTime());
    }
    g_resourceInstallActive = 0;
    Cmd_ResetScroll();
    g_gameReg->m_gameStats->Reset();
    g_gameReg->m_commandMgr->m_pendingLocalCommands.RemoveAll();
    g_gameReg->m_commandMgr->RecycleQueuedCommands();
    g_frameTicks = 0;
    self->m_returnToMenuOnComplete = false;
    self->m_mgr->m_isCustomLevel = 0;

    CGruntzMgr* mgr = self->m_mgr;
    if (mgr->m_strWorldFile.GetLength() != 0) {
        CRezArchiveEntry* ins;
        char* desc;
        char* p;
        char c;
        if (mgr->m_isBuiltInBattlezLevel != 0) {

            bank = mgr->m_resourceArchive->FindDirectoryByPath("GAME_BATTLEZ");
            if (bank == NULL) {
                goto fail0;
            }
            ins = bank->FindEntry(
                static_cast<const char*>(self->m_mgr->GetWorldFileName()),
                REZ_TAG_WWD
            );
            if (ins == NULL) {
                return 0;
            }
            desc = ins->LoadData();
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
            ins->ReleaseData();
        } else if (mgr->m_isBuiltInMultiplayerLevel != 0) {

            bank = mgr->m_resourceArchive->FindDirectoryByPath("GAME_MULTI");
            if (bank == NULL) {
                goto fail0;
            }
            ins = bank->FindEntry(
                static_cast<const char*>(self->m_mgr->GetWorldFileName()),
                REZ_TAG_WWD
            );
            if (ins == NULL) {
                return 0;
            }
            desc = ins->LoadData();
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
            ins->ReleaseData();
        } else {

            level = WwdFile::ValidateMainBlock(self->m_mgr->GetWorldFileName());
            self->m_returnToMenuOnComplete = true;
            self->m_mgr->m_isCustomLevel = 1;
        }

        i32 r = (level - 1) % 0x24;
        self->m_levelIndex = level;
        self->m_levelType = static_cast<LevelArea>(r / 4 + 1);
    }

    sprintf(nameBuf, "AREA%i", IDX(self->m_levelType));
    bank = self->m_resourceArchive->FindDirectoryByPath(nameBuf);
    self->m_levelResources = bank;
    if (bank == NULL) {
        goto fail0;
    }

    {
        LevelArea page = self->m_levelType;
        switch (page) {
            case AREA_ROCKY_ROADZ:
                g_areaPitDeath = DEATH_SINK;
                break;
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
        prevTiles = self->m_stateResources;
        self->m_stateResources = (self->m_levelResources);
        UpdateWindow(self->m_mgr->m_gameWnd->m_hwnd);

        mgr = self->m_mgr;
        if (mgr->m_strWorldFile.GetLength() != 0) {
            if (mgr->m_isBuiltInBattlezLevel == 0 && mgr->m_isBuiltInMultiplayerLevel == 0) {
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
    self->m_stateResources = prevTiles;
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
        (savedThis)->SendLobbyKeepAlive();
    }
    RegisterInputBindings();

    if (!InitializeLevelArea(level)) {
        goto fail0;
    }

    {
        i32 cached = g_lastLevelNum;
        i32 eq = g_pAreaMgr->IsSameWorld(cached);
        reload = !eq;
        diff = (level != g_lastLevelNum) ? 1 : 0;
        if (g_pAreaMgr == NULL) {
            return 0;
        }
        g_lastLevelNum = level;

        BuildHelpReveal(0);
        if (modeFlag) {
            (savedThis)->SendLobbyKeepAlive();
        }
        RegisterInputBindings();

        BuildHelpReveal(0);
        if (modeFlag) {
            (savedThis)->SendLobbyKeepAlive();
        }
        RegisterInputBindings();

        if (!LoadActionTileSprites(diff)) {
            goto fail0;
        }
    }

    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->SendLobbyKeepAlive();
    }
    RegisterInputBindings();
    if (diff != 0 && (g_gameReg)->m_gameMode == GAMEMODE_QUESTZ) {
        BuildWarlordNameTable(savedThis);
    }
    BuildHelpReveal(0);
    RegisterInputBindings();
    if (!LoadLevelImages(reload)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->SendLobbyKeepAlive();
    }
    RegisterInputBindings();
    if (!LoadGameImages(reload)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->SendLobbyKeepAlive();
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
        (savedThis)->SendLobbyKeepAlive();
    }
    RegisterInputBindings();
    if (!LoadGameSounds(reload)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->SendLobbyKeepAlive();
    }
    RegisterInputBindings();
    if (!LoadGruntSoundNamespaces(NULL)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->SendLobbyKeepAlive();
    }
    RegisterInputBindings();
    SetEffectSpriteDurations();
    if (modeFlag) {
        (savedThis)->SendLobbyKeepAlive();
    }
    RegisterInputBindings();
    if (!LoadLevelAnims(reload)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->SendLobbyKeepAlive();
    }
    RegisterInputBindings();
    if (!LoadGameAnims(reload)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->SendLobbyKeepAlive();
    }
    RegisterInputBindings();
    if (!BuildAnizKeyTable(NULL)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->SendLobbyKeepAlive();
    }
    RegisterInputBindings();
    if (!BuildWorldLevelPath(reload)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->SendLobbyKeepAlive();
    }
    RegisterInputBindings();

    self->m_mgr->RecomputeViewScale();
    if (self->m_world->m_level->m_mainPlane != NULL) {
        (static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane))
            ->ActivateKeepActiveObjects();
    }
    if (self->m_world->m_level->m_mainPlane != NULL) {
        (static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane))
            ->ActivateVisibleObjects();
    }
    BuildHelpReveal(0);
    if (modeFlag) {
        (savedThis)->SendLobbyKeepAlive();
    }
    RegisterInputBindings();
    self->m_mgr->m_tileGrid->Reset();

    {
        CDDrawWorkerHost* mainPlane =
            static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane);
        CGruntzMapMgr* tileGrid = self->m_mgr->m_tileGrid;
        if (!tileGrid->BuildCellAttributes(mainPlane->m_tileColumns, mainPlane->m_tileRows)) {
            goto fail0;
        }
    }
    if (!(static_cast<CMapMgr*>(self->m_mgr->m_tileGrid))->UpdateDiagonals(self->m_mgr)) {
        goto fail0;
    }

    if (self->m_minimap == NULL) {
        CMinimap* minimap = new CMinimap;
        self->m_minimap = minimap;
        if (!minimap->Init(self->m_mgr, 0xfa)) {
            goto fail0;
        }
    }
    if (!self->m_minimap->SetAreaPalette(self->m_levelType)) {
        goto fail0;
    }

    if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
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

    if (self->m_mgr->m_gameMode == GAMEMODE_BATTLEZ) {
        self->m_mgr->InitializeBattlezPlayers();
    }
    self->m_mgr->m_saveGame
        ->InitializeLevelSlot(&self->m_saveSlot, self->m_levelIndex, self->m_mgr);
    {
        CString key;
        g_gameReg->m_triggerMgr->m_pendingFx = NULL;
        i32 count = self->m_levelIndex;
        i32 i = count - ((count - 1) % 4);
        for (; i < self->m_levelIndex; ++i) {

            key.Format("Level%i", i);
            CTriggerMgr* bm = g_gameReg->m_triggerMgr;
            i32 v = g_buteMgr.GetInt("WarpStone", static_cast<const char*>(key));
            bm->m_byteArr.SetAtGrow(bm->m_byteArr.GetSize(), static_cast<u8>(v));
        }
        self->m_statusBar->LoadMultiplayerBattlezConfig(self->m_levelIndex);

        CWwdSpriteObject* scrollSink = self->m_world->m_childGroup->CreateSprite(
            0,
            0,
            0,
            0x13880,
            "CursorSnapSprite",
            WWD_GAME_OBJECT_FLAGS_WORLD_SPACE_SKIP_COLLISION
        );
        self->m_cursorSnapSprite = scrollSink;
        if (scrollSink != NULL) {
            self->m_world->m_childGroup->TickKillCues(0);
            if (savedThis == NULL) {

                CStatusBarMgr* statusBar = self->m_statusBar;
                i32 originX = TIMER_ORIGIN_X_STATUSBAR_RIGHT_PX;
                if (statusBar->m_position != STATUSBAR_DOCK_RIGHT) {
                    originX = TIMER_ORIGIN_X_PX;
                }
                if (!self->m_levelTimer->LoadTimerSprite(originX, TIMER_ORIGIN_Y_PX)) {
                    CTimer* spr = self->m_levelTimer;
                    if (spr != NULL) {
                        spr->Reset();
                        ::operator delete(spr);
                        self->m_levelTimer = NULL;
                    }
                }
            }
            {
                if (LoadWarlordSprites(savedThis, initScratch) && ScanBuildTiles()
                    && ValidateLevelTiles() && AddLevelGruntz()) {
                    self->m_world->m_childGroup->TickKillCues(0);
                    self->m_statusBar->StartChipMachineCycle();
                    (static_cast<DirectInputMgr2*>(g_inputMgr))->ReadAll();
                    while (ShowCursor(false) >= 0)
                        ;
                    self->m_mgr->RefreshGameClock();
                    if (self->m_world->m_level->m_mainPlane != NULL) {
                        (static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane))
                            ->ActivateKeepActiveObjects();
                    }
                    if (self->m_world->m_level->m_mainPlane != NULL) {
                        (static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane))
                            ->ActivateVisibleObjects();
                    }
                    BuildHelpReveal(0);
                    if (modeFlag) {
                        (savedThis)->SendLobbyKeepAlive();
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
            (savedThis)->SendLobbyKeepAlive();
        }
        RegisterInputBindings();
        BuildHelpReveal(1);
        ActiveWait(0x64);
        if (modeFlag) {
            (savedThis)->SendLobbyKeepAlive();
        }

        gameReg = g_gameReg;
        if (gameReg->m_loadingSaveGame == 0) {
            CDDSurface* mapHost = self->m_world->m_drawTarget->m_frontSurface->m_surface;
            mapHost->ShadeRect(0x32, NULL);
            gameReg = g_gameReg;
        }

        if (gameReg->m_gameMode != GAMEMODE_MULTIPLAYER && gameReg->m_loadingSaveGame == 0) {
            CString scr;
            self->m_inGame = true;
            self->m_hudSuppressed = false;
            RECT rect;
            rect.left = 0;
            rect.top = 0;
            rect.right = SCREEN_W_PX;
            rect.bottom = SCREEN_H_PX;
            if (scr.LoadString(IDS_CONTINUE_PROMPT)) {
                DrawTextToFrontSurface(self->m_world, &scr, &rect, 0x78, 1, 0xff, 0xff, 0, 1);
            }
        } else {
            self->m_hudSuppressed = true;
        }

        self->m_scrollEdgeLock = 0;
        self->m_levelOverlayOpen = false;
        self->m_paused = false;
        self->m_playerCommandPending = false;
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
        self->m_defeatCountdownActive = false;
        self->m_focusPlayerIndex = 3;
        self->m_renderDisabled = true;
        g_playActive = 0;
        ResetViewport();
        if ((g_gameReg)->m_gameMode == GAMEMODE_MULTIPLAYER) {
            g_playActive = 1;
            self->m_renderDisabled = false;
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
RVA(0x000cb400, 0x58)
void CPlay::OnExit() {
    ForwardReady();
    FreeListTeardown();
    if (m_world) {
        m_world->m_childGroup->ClearChildren();
    }
    g_gameReg->m_isBuiltInBattlezLevel = 0;
    if (g_gameReg->m_gameMode == GAMEMODE_BATTLEZ) {
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
    if (m_mgr->m_triggerMgr != NULL) {
        m_mgr->m_triggerMgr->RemovePlayerUnitsImmediately(TM_ALL_PLAYERS);
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
    g_gameReg->m_triggerMgr->DestroyAllAnims();
    m_world->m_level->ReleaseChildren();
    (m_world->m_childGroup)->PruneList();
    if (m_statusBar != NULL) {
        m_statusBar->ResetWidgets(0);
    }
    if (m_tileTriggers != NULL) {
        m_tileTriggers->RemoveAll();
    }
    if (m_levelTimer != NULL) {
        m_levelTimer->Reset();
    }
    m_cursorSnapSprite = NULL;
    m_mgr->m_triggerMgr->CloseActionOptionsMenu();
    CTriggerMgr* tl68 = m_mgr->m_triggerMgr;

    tl68->m_byteArr.SetSize(0, -1);
    tl68->m_groupInitialized = false;
    m_mgr->m_triggerMgr->m_baseList.RemoveAll();
    m_mgr->m_triggerMgr->m_pendingFx = NULL;
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
        m_mgr->m_players[i].m_battlezConfig.FreeArrays();
        m_mgr->m_players[i].m_battlezConfig.Clear();
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
    while (ShowCursor(false) >= 0)
        ;

    CRezArchiveDir* h = m_levelResources->FindDirectoryByPath("TILEZ");
    if (!h) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(h, "", "_") == -1) {
        return 0;
    }

    h = m_levelResources->FindDirectoryByPath("IMAGEZ");
    if (!h) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(h, "LEVEL", "_") == -1) {
        return 0;
    }

    h = m_gruntResources->FindDirectoryByPath("IMAGEZ");
    if (!h) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(h, "GRUNTZ", "_") == -1) {
        return 0;
    }

    g_inputMgr->ReadAll();
    while (ShowCursor(false) >= 0)
        ;

    m_world->m_drawTarget->m_backPair->m_surface->Fill(0);
    UpdateMgrScroll(g_gameReg, m_statusBar, m_region0Gate);

    if (m_region1Gate != 0) {
        NotifyVisibleEntities();
    } else {
        m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
        m_world->m_workerList->RenderAndPruneWorkers(
            m_world->m_drawTarget->m_backPair,
            m_world->m_drawTarget->m_overlayPair
        );
    }

    m_statusBar->Deactivate();
    m_statusBar->LoadMainStatusBarSprite();
    m_stepCountdown = 2;
    m_world->m_drawTarget->TransTitle();
    RetireScene(0x50, 0x3e8, 0, 1);
    return 1;
}

// @early-stop
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
    if (m_statusBar != NULL) {
        m_statusBar->Deactivate();
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
        m_world->m_drawTarget->m_frontSurface->m_surface->Flip(NULL);
    }
    return 1;
}

RVA(0x000cbaf0, 0x16f)
i32 CPlay::OnChar(i32 charCode, i32 keyData) {
    if (m_hudSuppressed != false) {
        return 1;
    }
    if (m_renderDisabled != false) {
        m_renderDisabled = false;
        m_hudSuppressed = true;
        EnterMode(GAMESTATE_PLAY);
        m_inGame = true;
        return 1;
    }
    if (m_inGame != false) {

        if (ResetPlayState() == 0) {
            m_mgr->ReportError(IDX(IDS_INITIALIZE_GAME), 0x456);
        }
        return 1;
    }
    if (m_paused != false) {
        m_paused = false;
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_FINISH_LEVEL), 0);
        return 1;
    }

    if (m_mgr->m_frameGate == 0) {
        if (m_chatBox->m_inputActive != 0) {
            m_mgr->m_chatLog->HandleInputChar(charCode, keyData);
            return 1;
        }
        if (charCode == ']') {
            m_statusBar->DockStatusBarRight();
            return 1;
        }
        if (charCode == '[') {
            m_statusBar->DockStatusBarLeft();
            return 1;
        }
        if (charCode == '-') {
            m_statusBar->HideRect();
            return 1;
        }
        if (charCode == '=' || charCode == '+') {
            m_statusBar->RestoreStatusBar();

            if (m_statusBar->m_position == STATUSBAR_DOCK_LEFT) {
                m_chatBox->Configure(CHATBOX_WITH_LEFT_STATUSBAR);
            } else {
                m_chatBox->Configure(CHATBOX_WITH_RIGHT_STATUSBAR);
            }
            return 1;
        }
    }
    return 0;
}

// @early-stop
RVA(0x000cbcc0, 0x17c0)
i32 CPlay::OnKeyDown(i32 vk, i32 lparam) {
    if (this->m_hudSuppressed != false) {
        return 1;
    }
    if (this->m_renderDisabled != false) {
        return 1;
    }
    if (this->m_inGame != false) {
        return 1;
    }
    if (this->m_paused != false) {
        return 1;
    }
    if (this->m_mgr->m_frameGate != 0) {
        return 1;
    }

    CGruntzMgr* mgr = this->m_mgr;
    CStatusBarMgr* statusBar = this->m_statusBar;

    if (statusBar->m_levelOverlayActive != 0 || statusBar->m_quitConfirmationActive != 0) {
        if (statusBar->m_quitConfirmationActive != 0) {

            if (vk == 'Y' || vk == VK_RETURN) {
                if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                    CLEAR_TAB_HINT(mgr->m_world->m_soundRegistry);
                    if (g_gameReg->m_triggerMgr->m_phase == FINISH_STATE_VICTORY) {
                        g_gameReg->CommitSinglePlayerProgress();
                    }
                    PostMessageA(mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
                    return 1;
                }
                CLEAR_TAB_HINT(mgr->m_world->m_soundRegistry);
                mgr->FinalizeLevelAndShowResults();
                return 1;
            }
            if (vk == 'N' || vk == VK_ESCAPE) {
                CLEAR_TAB_HINT(mgr->m_world->m_soundRegistry);
                this->CloseLevelOverlay(0);
                return 1;
            }

        } else {

            if (vk == 'Q') {
                if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                    CLEAR_TAB_HINT(mgr->m_world->m_soundRegistry);
                    if (g_gameReg->m_triggerMgr->m_phase == FINISH_STATE_VICTORY) {
                        g_gameReg->CommitSinglePlayerProgress();
                    }
                    PostMessageA(mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
                }
                return 1;
            }

            if (vk == 'S' && g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                CLEAR_TAB_HINT(mgr->m_world->m_soundRegistry);
                mgr->FinalizeLevelAndShowResults();
            }
            if (vk == 'R') {
                if (mgr->m_gameMode == GAMEMODE_QUESTZ
                    && g_gameReg->m_triggerMgr->m_phase != FINISH_STATE_VICTORY) {
                    CLEAR_TAB_HINT(g_gameReg->m_world->m_soundRegistry);
                    CGameWnd* r = g_gameReg->m_gameWnd;
                    PostMessageA(r->m_hwnd, WM_COMMAND, IDX(CMD_RELOAD_LEVEL), 0);
                }
                return 1;
            }
            if (vk == 'N') {
                if (mgr->m_gameMode == GAMEMODE_QUESTZ
                    && g_gameReg->m_triggerMgr->m_phase == FINISH_STATE_VICTORY) {
                    CLEAR_TAB_HINT(g_gameReg->m_world->m_soundRegistry);
                    mgr->FinalizeLevelAndShowResults();
                }
                return 1;
            }
            if (vk == 'O') {
                if (mgr->m_gameMode != GAMEMODE_QUESTZ
                    && this->m_statusBar->m_observerTabAvailable != 0) {
                    CLEAR_TAB_HINT(g_gameReg->m_world->m_soundRegistry);
                    this->CloseLevelOverlay(0);
                }
                return 1;
            }
        }
    }

    if (vk == VK_RETURN) {
        CChatBoxOwner* rec = this->m_chatBox;
        if (rec->m_inputActive != 0) {
            rec->HandleTextInputKey('\r', lparam);
        } else {
            rec->m_fontConfig->EndInput();
            rec->m_inputActive = 1;
            this->m_chatBox->HandleTextInputKey('\r', lparam);
        }
        return 1;
    }

    if (vk == VK_ESCAPE) {
        CTriggerMgr* h68 = mgr->m_triggerMgr;
        CWwdSpriteObject* n = h68->m_goal;
        if (n != NULL) {
            n->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
            h68->m_goal = NULL;
        }
        h68->m_armed = 0;
        CChatBoxOwner* rec = this->m_chatBox;
        if (rec->m_inputActive != 0) {
            this->FlushPendingOps();
            this->m_chatBox->m_fontConfig->EndInput();
            this->m_chatBox->m_inputActive = 0;
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

    if (this->m_chatBox->m_inputActive != 0) {
        return 1;
    }
    if (g_gameReg->m_triggerMgr->m_groupFlag == 0) {
        return 1;
    }

    if (vk == VK_TAB) {
        i32 idx = this->m_focusPlayerIndex;
        i32 pick;
        GruntzPlayer* area;
        if (g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON0)) {
            pick = idx - 1;
            if (pick < 0) {
                pick = 3;
            }
            area = &g_gameReg->m_players[pick];
            while (pick != idx) {
                if (area->m_joined == 0 || (area->m_doneFlag == 0 && area->m_clearedRound == 0)) {
                    break;
                }
                pick--;
                if (pick < 0) {
                    pick = 3;
                }
                area = &g_gameReg->m_players[pick];
            }
        } else {
            pick = idx + 1;
            if (pick >= 4) {
                pick = 0;
            }
            area = &g_gameReg->m_players[pick];
            while (pick != idx) {
                if (area->m_joined == 0 || (area->m_doneFlag == 0 && area->m_clearedRound == 0)) {
                    break;
                }
                pick++;
                if (pick >= 4) {
                    pick = 0;
                }
                area = &g_gameReg->m_players[pick];
            }
        }
        if (area->m_joined != 0 && area->m_doneFlag == 0 && area->m_clearedRound == 0) {
            this->m_focusPlayerIndex = pick;
            this->ResetGoals(area->m_focusX, area->m_focusY);
        }
    }

    if (vk == 'H') {
        GruntzPlayer* a = &g_gameReg->m_players[g_curPlayer];
        if (a == NULL) {
            return 1;
        }
        this->ResetGoals(a->m_focusX, a->m_focusY);
        return 1;
    }

    if (vk == 'Q') {
        if ((g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5)) == 0) {
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
        g_gameReg->m_triggerMgr->EnqueueGroupCells();
        return 1;
    }

    if (vk == 'C') {
        g_gameReg->m_triggerMgr->CenterOnGroup(g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5));
        return 1;
    }

    if (vk == 'T') {
        this->FlushPendingOps();
        g_gameReg->m_triggerMgr->ToggleToolTargeting();
        return 1;
    }

    if (vk == 'Y') {
        this->FlushPendingOps();
        g_gameReg->m_triggerMgr->ToggleToyTargeting();
        return 1;
    }

    if (vk == VK_SPACE) {
        if (g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5)) {
            CDDrawWorkerHost* obj = this->m_world->m_level->m_mainPlane;
            i32 v0 = obj->m_scrollPixelX;
            i32 v1 = obj->m_scrollPixelY;
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
        if (g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON0)) {
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

    if (vk == 'M' && (g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5))) {
        g_gameReg->SetMusicEnabled(g_gameReg->m_musicEnabled == 0);
        return 1;
    }

    if (vk == 'V' && (g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5))) {
        g_gameReg->m_isVoiceEnabled = (g_gameReg->m_isVoiceEnabled == 0);
        return 1;
    }

    if (vk == 'A') {
        if (statusBar->m_chatBoxDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(mgr->m_world->m_soundRegistry);
        CStatusBarMgr* lv = this->m_statusBar;
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
        if (g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5)) {
            g_gameReg->SetSoundEnabled(g_gameReg->m_soundEnabled == 0);
            return 1;
        }
        if (statusBar->m_chatBoxDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(mgr->m_world->m_soundRegistry);
        CStatusBarMgr* lv = this->m_statusBar;
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
        if (statusBar->m_chatBoxDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(mgr->m_world->m_soundRegistry);
        CStatusBarMgr* lv = this->m_statusBar;
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
        if (statusBar->m_chatBoxDisabled != 0) {
            return 1;
        }
        if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
            return 1;
        }
        CLEAR_TAB_HINT(mgr->m_world->m_soundRegistry);
        this->m_statusBar->AdvanceTab(g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON0));
        return 1;
    }

    if (vk == 'G') {
        if (statusBar->m_chatBoxDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(mgr->m_world->m_soundRegistry);
        CStatusBarMgr* lv = this->m_statusBar;
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
        GruntzPlayer* a = &g_gameReg->m_players[g_curPlayer];
        if (a == NULL) {
            return 1;
        }
        if (g_gameReg->m_triggerMgr->m_unitCountByPlayer[g_curPlayer] >= a->m_maxGruntz) {
            return 1;
        }
        CGruntzMgr* h = this->m_mgr;
        i32 my = this->m_cursorY;
        LevelCoordRect* r = &h->m_world->m_level->m_viewportRect;
        i32 x0 = r->left;
        i32 y0 = r->top;
        i32 x1 = r->right;
        i32 y1 = r->bottom;
        i32 mx = this->m_cursorX;
        if (mx >= x1 || mx < x0 || my >= y1 || my < y0) {
            return 1;
        }
        h->m_commandMgr->EnqueuePlaceGruntAtScreenPoint(1, g_curPlayer, mx, my, 0);
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
        LevelCoordRect* r = &q->m_viewportRect;
        i32 x0 = r->left;
        i32 y0 = r->top;
        i32 x1 = r->right;
        i32 y1 = r->bottom;
        i32 my = this->m_cursorY;
        if (!(mx >= x1 || mx < x0 || my >= y1 || my < y0)) {
            CDDrawWorkerHost* g = q->m_mainPlane;
            RECT* view = &g->m_planeViewRect;
            i32 by = view->top - q->m_viewportRect.top + my;
            i32 bx = view->left - q->m_viewportRect.left + mx;
            mgr->m_triggerMgr->SpawnPuddle(bx, by, 0, 0, 1, 0x19);
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
        RECT* view = &g->m_planeViewRect;
        i32 by = ((view->top - q->m_viewportRect.top + my) & ~TILE_MASK_PX) + TILE_HALF_PX;
        i32 bx = ((this->m_cursorX - q->m_viewportRect.left + view->left) & ~TILE_MASK_PX)
                 + TILE_HALF_PX;
        g_gameReg->m_triggerMgr->LoadExplosionSprites(bx, by, -1, 1);
        return 1;
    }

    if (vk == 'K') {
        if (g_gruntDestruction == 0) {
            return 1;
        }
        i32 playerIndex;
        i32 unitIndex;
        CGrunt* r = mgr->m_triggerMgr->ScreenToCell(
            this->m_cursorX,
            this->m_cursorY,
            &playerIndex,
            &unitIndex,
            TM_ALL_PLAYERS
        );
        if (r == NULL) {
            return 1;
        }
        mgr->m_triggerMgr->StartUnitDeath(playerIndex, unitIndex, DEATH_DROP, -1);
        return 1;
    }

    if (vk == '1') {
        if (g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5)) {
            g_gameReg->m_triggerMgr->RebuildSelectionList(1);
        } else {
            g_gameReg->m_triggerMgr->CenterSelectionGroup(1);
        }
        return 1;
    }
    if (vk == '2') {
        if (g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5)) {
            g_gameReg->m_triggerMgr->RebuildSelectionList(2);
        } else {
            g_gameReg->m_triggerMgr->CenterSelectionGroup(2);
        }
        return 1;
    }
    if (vk == '3') {
        if (g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5)) {
            g_gameReg->m_triggerMgr->RebuildSelectionList(3);
        } else {
            g_gameReg->m_triggerMgr->CenterSelectionGroup(3);
        }
        return 1;
    }
    if (vk == '4') {
        if (g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5)) {
            g_gameReg->m_triggerMgr->RebuildSelectionList(4);
        } else {
            g_gameReg->m_triggerMgr->CenterSelectionGroup(4);
        }
        return 1;
    }
    if (vk == '5') {
        if (g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5)) {
            g_gameReg->m_triggerMgr->RebuildSelectionList(5);
        } else {
            g_gameReg->m_triggerMgr->CenterSelectionGroup(5);
        }
        return 1;
    }
    if (vk == '6') {
        if (g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5)) {
            g_gameReg->m_triggerMgr->RebuildSelectionList(6);
        } else {
            g_gameReg->m_triggerMgr->CenterSelectionGroup(6);
        }
        return 1;
    }
    if (vk == '7') {
        if (g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5)) {
            g_gameReg->m_triggerMgr->RebuildSelectionList(7);
        } else {
            g_gameReg->m_triggerMgr->CenterSelectionGroup(7);
        }
        return 1;
    }
    if (vk == '8') {
        if (g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5)) {
            g_gameReg->m_triggerMgr->RebuildSelectionList(8);
        } else {
            g_gameReg->m_triggerMgr->CenterSelectionGroup(8);
        }
        return 1;
    }
    if (vk == '9') {
        if (g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5)) {
            g_gameReg->m_triggerMgr->RebuildSelectionList(9);
        } else {
            g_gameReg->m_triggerMgr->CenterSelectionGroup(9);
        }
        return 1;
    }
    return 1;

recorder_place:

{
    if (this->m_playerCommandPending != false) {
        return 1;
    }
    if (this->m_dragInhibit1 != false) {
        this->m_dragInhibit1 = false;
        this->m_statusBar->CommitSlot(0);
        this->SetCursorFrame(0);
        if (vk != VK_INSERT) {
            goto tail_default;
        }
        return 1;
    }
    if (this->m_dragInhibit2 == false) {
        goto tail_default2;
    }
    i32 st = this->m_cursorFrame;
    StatusBarHighlightRow ph = this->m_statusBar->m_pendingHlRow;
    i32 lvl;
    if (st >= 0x22) {
        lvl = 2;
    } else {
        lvl = (st >= 0x17);
    }
    this->m_dragInhibit2 = false;
    if (vk == VK_DELETE || vk == VK_DECIMAL) {
        statusBar->ReportTab(st);
        this->SetCursorFrame(0);
        return 1;
    }
    statusBar->EnterHlRow(0, st);
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
    g_gameReg->m_triggerMgr->m_pendingFxKind = 0;
    this->LoadCursorSprites(0, 0);
}
tail_default2:

    if (this->m_statusBar->m_chatBoxDisabled != 0) {
        return 1;
    }
    {

        CStatusBarMgr* lv = this->m_statusBar;
        switch (vk) {
            case VK_END:
            case VK_NUMPAD1:
                lv->SelectToolResource(STATUS_HL_ROW_LOWER);
                return 1;
            case VK_DOWN:
            case VK_NUMPAD2:
                lv->SelectToyResource(STATUS_HL_ROW_LOWER);
                return 1;
            case VK_NEXT:
            case VK_NUMPAD3:
                lv->SelectBrickResource(STATUS_HL_ROW_LOWER);
                return 1;
            case VK_LEFT:
            case VK_NUMPAD4:
                lv->SelectToolResource(STATUS_HL_ROW_MIDDLE);
                return 1;
            case VK_CLEAR:
            case VK_NUMPAD5:
                lv->SelectToyResource(STATUS_HL_ROW_MIDDLE);
                return 1;
            case VK_RIGHT:
            case VK_NUMPAD6:
                lv->SelectBrickResource(STATUS_HL_ROW_MIDDLE);
                return 1;
            case VK_HOME:
            case VK_NUMPAD7:
                lv->SelectToolResource(STATUS_HL_ROW_UPPER);
                return 1;
            case VK_UP:
            case VK_NUMPAD8:
                lv->SelectToyResource(STATUS_HL_ROW_UPPER);
                return 1;
            case VK_PRIOR:
            case VK_NUMPAD9:
                lv->SelectBrickResource(STATUS_HL_ROW_UPPER);
                return 1;
            case VK_NUMLOCK:
                lv->SelectToolResource(STATUS_HL_ROW_CATEGORY);
                return 1;
            case VK_DIVIDE:
                lv->SelectToyResource(STATUS_HL_ROW_CATEGORY);
                return 1;
            case VK_MULTIPLY:
                lv->SelectBrickResource(STATUS_HL_ROW_CATEGORY);
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
RVA(0x000cdb10, 0x80c)
i32 CPlay::OnLButtonDown(i32 eventArg, i32 x, i32 y) {
    i32 xr;
    i32 sx;
    i32 sy;

    if (m_hudSuppressed != false) {
        return 1;
    }
    if (m_renderDisabled != false) {
        m_hudSuppressed = true;
        m_renderDisabled = false;
        EnterMode(GAMESTATE_PLAY);
        m_inGame = true;
        return 1;
    }
    if (m_inGame != false) {
        if (ResetPlayState()) {
            goto ret1;
        }
        m_mgr->ReportError(IDX(IDS_INITIALIZE_GAME), 0x457);
        return 1;
    }
    if (m_paused != false) {
        m_paused = false;
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_FINISH_LEVEL), 0);
        return 1;
    }

    if (m_levelOverlayOpen != false || g_gameReg->m_triggerMgr->m_groupFlag == 0) {
        return m_statusBar->UpdateStatusBarTabHighlight(eventArg, x, y);
    }

    xr = x;
    if (m_mgr->m_frameGate == 0) {
        if (m_minimap != NULL && m_statusBar->m_position != STATUSBAR_HIDDEN
            && m_statusBar->m_activeTab != TAB_GAME) {
            if (m_minimap->BeginMinimapPan(eventArg, xr, y)) {
                return 1;
            }
        }
        CGameLevel* geom = m_mgr->m_world->m_level;
        CDDrawWorkerHost* cam = geom->m_mainPlane;
        RECT* view = &cam->m_planeViewRect;
        sx = view->left - geom->m_viewportRect.left + xr;
        sy = view->top - geom->m_viewportRect.top + y;

        if (m_dragInhibit1 != false && m_playerCommandPending == false) {
            eventArg = 0;
            RECT* gr = &m_statusBar->m_barRect;
            if (CGameLevel::PointInRect(gr, xr, y)) {

            } else {
                if (CGameLevel::PointInRect(&geom->m_viewportRect, xr, y)) {
                    if (FindStartPointAt(sx, sy, &x, &y)) {
                        m_mgr->m_commandMgr->EnqueueSingle(
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
            m_dragInhibit1 = false;
            m_statusBar->CommitSlot(eventArg);
            SetCursorFrame(0);
            return 1;
        }

        if (m_dragInhibit2 != false && m_playerCommandPending == false) {
            {
                RECT* gr = &m_statusBar->m_barRect;
                if (CGameLevel::PointInRect(gr, xr, y)) {
                    if (m_statusBar->DropFallingItemAt(xr, y, m_cursorFrame)) {
                        m_dragInhibit2 = false;
                        SetCursorFrame(0);
                        return 1;
                    }
                    goto waypoint_cancel;
                }
                CGameLevel* geom2 = m_mgr->m_world->m_level;
                RECT* wr = (&geom2->m_viewportRect);
                if (!CGameLevel::PointInRect(wr, xr, y)) {
                    goto waypoint_cancel;
                }

                CGameLevel* ds = m_world->m_level;
                LevelCoordRect* vr2 = &ds->m_mainPlane->m_planeViewRect;
                i32 wx = vr2->left - ds->m_viewportRect.left + xr;
                i32 wy = vr2->top - ds->m_viewportRect.top + y;
                if (g_gameReg->m_triggerMgr->CellHitTest(wx, wy, &eventArg, &y, g_curPlayer)
                    != NULL) {
                    m_mgr->m_commandMgr->EnqueueSingle(
                        1,
                        static_cast<char>(eventArg),
                        static_cast<char>(y),
                        static_cast<char>(IDX(PLAYERCMD_GIVE_TOOL)),
                        0,
                        0,
                        static_cast<char>(m_cursorFrame),
                        0
                    );
                    m_playerCommandPending = true;
                    return 1;
                }

                RECT box;
                box.left = wx - 0xf;
                box.top = wy - 0xf;
                box.right = wx + 0xf;
                box.bottom = wy + 0xf;

                RECT span = {0, 0, 0, 0};
                CGrunt* p =
                    g_gameReg->m_triggerMgr->FindGruntAt(wx, wy, &span, &eventArg, &y, &box);
                if (p == NULL || g_curPlayer != p->m_playerIndex) {
                    goto waypoint_cancel;
                }
                m_mgr->m_commandMgr->EnqueueSingle(
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
            m_dragInhibit2 = false;
            m_statusBar->EnterHlRow(0, m_cursorFrame);
            SetCursorFrame(0);
            return 1;
        }
    } else {
        sx = y;
        sy = y;
    }

    {

        if (m_statusBar == NULL) {
            return 1;
        }
        if (m_statusBar->m_position == STATUSBAR_HIDDEN) {
            if (m_statusBar->HitTestLayer(xr, y)) {
                m_dragSnapActive = true;

                CGameObject* g8 = m_statusBar->m_barSprite;
                i32 dx = 0;
                if (g8 != NULL) {
                    dx = g8->m_screenX - xr;
                }
                m_snapOriginX = dx;
                CGameObject* g8b = m_statusBar->m_barSprite;
                if (g8b == NULL) {
                    m_snapOriginY = 0;
                    return 1;
                }
                m_snapOriginY = g8b->m_screenY - y;
                return 1;
            }
            goto drag_box;
        }

        RECT* gr = &m_statusBar->m_barRect;
        if (CGameLevel::PointInRect(gr, xr, y)) {
            FlushPendingOps();
            return m_statusBar->UpdateStatusBarTabHighlight(eventArg, xr, y);
        }
        if (m_chatBox->HitTest(xr, y)) {
            return 1;
        }
    }

drag_box: {
    if (m_mgr->m_frameGate != 0) {
        goto ret1;
    }
    LevelCoordRect wr = m_mgr->m_world->m_level->m_viewportRect;
    if (!(x < wr.right && x >= wr.left && y < wr.bottom)) {
        goto ret1;
    }
    if (y < wr.top) {
        return 1;
    }

    if (m_cursorTargetValid != false) {
        i32 ex = (sx & ~TILE_MASK_PX) + TILE_HALF_PX;
        i32 ey = (sy & ~TILE_MASK_PX) + TILE_HALF_PX;
        i32 lv = m_cursorId - IDX(CURSOR_TOOL_HANDZ);
        PickupType item = static_cast<PickupType>(lv);
        if (item <= PICKUP_EQUIPPABLE_LAST) {
            g_gameReg->m_triggerMgr
                ->HandleTargetSelection(ex, ey, 0, 0, 0, TARGET_SELECTION_GRUNT, 1);
        } else if (item >= PICKUP_TOYZ_FIRST && item <= PICKUP_TOYZ_LAST) {
            g_gameReg->m_triggerMgr
                ->HandleTargetSelection(ex, ey, 0, 0, 0, TARGET_SELECTION_TOY, 1);
        }
        g_gameReg->m_triggerMgr->m_pendingFxKind = 0;
        LoadCursorSprites(0, 0);
        m_dragClampMaxX = xr;
        m_dragClampMaxY = y;
        m_hudRect.left = xr;
        m_hudRect.top = y;
        m_hudRect.right = xr;
        m_hudRect.bottom = y;
        m_worldReady = true;
        return 1;
    }
    if (g_gameReg->m_triggerMgr->HandleActionOptionsPointer(sx, sy)) {
        return 1;
    }

    if (m_cursorId >= IDX(CURSOR_TOOL_HANDZ)) {
        CTriggerMgr* cg = g_gameReg->m_triggerMgr;
        CGrunt* slot;
        if (1 != cg->m_recList.GetCount()) {
            slot = NULL;
        } else {
            i32* sel = static_cast<i32*>(cg->m_recList.GetHead());
            slot = cg->m_units[sel[0] * TM_UNITS_PER_PLAYER + sel[1]];
        }
        if (slot != NULL && slot->m_entranceCommitted != false) {
            g_gameReg->m_voiceManager->PlayVoice(slot, 0x324, -1, 0, -1, -1);
        }
    }
    LoadCursorSprites(0, 0);
    i32 hit = m_statusBar->HitTest(xr, y);
    if (hit != -1) {
        m_statusBar->PlaceCursorTarget(hit, 0);
        return 1;
    }

    CGrunt* picked = static_cast<CGrunt*>(
        m_mgr->m_triggerMgr->ScreenToCell(xr, y, &eventArg, &x, TM_ALL_PLAYERS)
    );
    if (picked != NULL) {
        m_mgr->m_triggerMgr
            ->ResetCell(eventArg, x, g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5), 0);
        if (eventArg == g_curPlayer) {
            if (g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5)) {
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
    m_worldReady = true;
    goto ret1;
}

ret1:
    return 1;
}

RVA(0x000ce530, 0xe3)
i32 CPlay::OnLButtonUp(i32 keyFlags, i32 x, i32 y) {
    if (m_hudSuppressed == false) {
        if (m_minimap != NULL && m_statusBar->m_position != STATUSBAR_HIDDEN
            && m_statusBar->m_activeTab != TAB_GAME) {
            m_minimap->EndMinimapPan(keyFlags, x, y);
        }
        if (m_worldReady != false) {
            m_mgr->m_triggerMgr->HudRect(
                m_hudRect,
                g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5)
            );
        }
        m_worldReady = false;
        m_dragSnapActive = false;
        if (m_statusBar->m_position != STATUSBAR_HIDDEN) {
            LevelCoordRect vp = m_world->m_level->m_viewportRect;
            if (x < vp.left || x > vp.right || y < vp.top || y > vp.bottom) {
                return m_statusBar->OnPointerRelease(keyFlags, x, y);
            }
        }
    }
    return 1;
}

// @early-stop
RVA(0x000ce660, 0x362)
i32 CPlay::OnLButtonDblClk(i32 keyFlags, i32 x, i32 y) {
    if (m_hudSuppressed != false || m_statusBar == NULL) {
        return 1;
    }
    if (m_levelOverlayOpen != false || g_gameReg->m_triggerMgr->m_groupFlag == 0) {
        return m_statusBar->HandleDoubleClick(keyFlags, x, y);
    }
    if (m_dragInhibit1 != false || m_dragInhibit2 != false) {
        return this->OnLButtonDown(keyFlags, x, y);
    }

    if (m_statusBar->m_position == STATUSBAR_HIDDEN && m_statusBar->HitTestLayer(x, y)) {
        SoundCueRegistry* registry = m_mgr->m_world->m_soundRegistry;
        if (registry->m_silentMode == 0) {
            SoundCue* cue = NULL;
            MapLookup(registry->m_cues, "GAME_TABHIGHLIGHT1", cue);
            if (cue != NULL) {
                cue->PlayIfElapsed(g_soundVolumePercent, 0, 0, 0);
            }
        }
        m_statusBar->RestoreStatusBar();
        if (m_statusBar->m_position == STATUSBAR_DOCK_LEFT) {
            m_chatBox->Configure(CHATBOX_WITH_LEFT_STATUSBAR);
        } else {
            m_chatBox->Configure(CHATBOX_WITH_RIGHT_STATUSBAR);
        }
        return 1;
    }

    i32 idx = m_statusBar->HitTest(x, y);
    if (idx != -1) {
        m_statusBar->PlaceCursorTarget(idx, 1);
        return 1;
    }

    RECT rc = m_world->m_level->m_viewportRect;
    if (x < rc.left || x > rc.right || y < rc.top || y > rc.bottom) {
        return m_statusBar->HandleDoubleClick(keyFlags, x, y);
    }

    {
        i32 playerIndex;
        i32 unitIndex;
        if (m_mgr->m_triggerMgr->ScreenToCell(x, y, &playerIndex, &unitIndex, TM_ALL_PLAYERS)
            && g_curPlayer == playerIndex) {
            m_statusBar->ToggleStat(unitIndex);
            return 1;
        }
    }

    if (m_dragInhibit1 != false) {
        return 1;
    }
    CGameLevel* h;
    RECT* vr;
    i32 px;
    i32 py;
    i32 i;
    i32 area = g_curPlayer;
    GruntzPlayer* cfg = &g_gameReg->m_players[area];
    if (cfg == NULL || g_gameReg->m_triggerMgr->m_unitCountByPlayer[area] >= cfg->m_maxGruntz) {
        return 0;
    }

    h = m_mgr->m_world->m_level;
    vr = &h->m_mainPlane->m_planeViewRect;
    px = vr->left - h->m_viewportRect.left + x;
    py = vr->top - h->m_viewportRect.top + y;
    for (i = 0; i < StartMarkerCount(); i++) {
        Coord* e = StartMarkerAt(i);
        if (e == NULL) {
            continue;
        }
        RECT er;
        SetRect(&er, e->m_x - 0x10, e->m_y - 0x10, e->m_x + 0x10, e->m_y + 0x10);
        if (CGameLevel::PointInRect(&er, px, py)) {
            if (!m_statusBar->FindReadySlot()) {
                return 1;
            }
            char ab = static_cast<char>(g_curPlayer);
            px = (px & 0xffe0) + 0x10;
            py = (py & 0xffe0) + 0x10;
            m_mgr->m_commandMgr->EnqueueSingle(
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
RVA(0x000ceae0, 0x268)
i32 CPlay::OnRButtonDown(i32 keyFlags, i32 x, i32 y) {
    if (m_hudSuppressed != false) {
        return 1;
    }
    if (m_renderDisabled != false) {
        m_hudSuppressed = true;
        m_renderDisabled = false;
        EnterMode(GAMESTATE_PLAY);
        m_inGame = true;
        return 1;
    }
    if (m_inGame != false) {
        if (ResetPlayState()) {
            return 1;
        }
        m_mgr->ReportError(IDX(IDS_INITIALIZE_GAME), 0x458);
        return 1;
    }
    if (m_paused != false) {
        m_paused = false;
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_FINISH_LEVEL), 0);
        return 1;
    }
    if (m_levelOverlayOpen != false) {
        return 1;
    }
    if (g_gameReg->m_triggerMgr->m_groupFlag == 0) {
        return 1;
    }
    if (m_mgr->m_frameGate != 0) {
        return 1;
    }
    if (m_minimap != NULL && m_statusBar->m_position != STATUSBAR_HIDDEN
        && m_statusBar->m_activeTab != TAB_GAME && m_minimap->IssueMinimapCommand(keyFlags, x, y)) {
        return 1;
    }

    if (CGameLevel::PointInRect(&m_statusBar->m_barRect, x, y)) {
        return 1;
    }
    i32 idx = m_statusBar->HitTest(x, y);
    if (idx != -1) {
        m_statusBar->ClearStat(idx);
        CTriggerMgr* w = m_mgr->m_triggerMgr;
        if (w->m_goal != NULL) {
            w->m_goal->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
            w->m_goal = NULL;
        }
        w->m_armed = 0;
        return 1;
    }
    if (m_mgr->m_triggerMgr->m_recList.GetCount() == 0) {
        return 1;
    }
    CGameLevel* ph = m_mgr->m_world->m_level;
    LevelCoordRect pr = ph->m_viewportRect;
    if (CGameLevel::PointInRect(&pr, x, y)) {
        CGameLevel* ds = m_world->m_level;
        CDDrawWorkerHost* geom = ds->m_mainPlane;
        i32 rawX = geom->m_planeViewRect.left - ds->m_viewportRect.left + x;
        i32 rawY = geom->m_planeViewRect.top - ds->m_viewportRect.top + y;
        i32 snapX = (rawX & ~TILE_MASK_PX) + TILE_HALF_PX;
        i32 snapY = (rawY & ~TILE_MASK_PX) + TILE_HALF_PX;
        m_tileClick.m_x = snapX;
        m_tileClick.m_y = snapY;
        CTriggerMgr* w = m_mgr->m_triggerMgr;
        if (w->m_overlay != NULL && w->m_overlay->m_active != 0) {
            w->CloseActionOptionsMenu();
            return 1;
        }
        w->HandleTargetSelection(snapX, snapY, rawX, rawY, 1, TARGET_SELECTION_AUTO, 1);
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
        m_statusBar->BuildGameTabResumeButton(0);
    } else {
        m_statusBar->BuildGameTabResumeButton(1);
    }
    m_worldReady = false;
    m_dragSnapActive = false;
    m_savedClock = g_frameTime;
    return 1;
}

RVA(0x000cef00, 0x39)
i32 CPlay::ResumeGame() {
    m_statusBar->BuildGameTabPauseButton();
    g_frameTime = m_savedClock;
    m_paused = false;
    if (m_statusBar != NULL) {
        m_statusBar->Deactivate();
    }
    return 1;
}

RVA(0x000cef50, 0x46)
i32 CPlay::QuitToMenu() {

    m_mgr->m_strWorldFile.Empty();
    if (m_completedFinalLevel != false) {
        if (m_world->m_drawTarget->HasOverlay() != 0) {
            m_world->m_drawTarget->TransEnter();
        }
        m_mgr->PlayMovieEntry(IDX(MOVIE_ENTRY_ENDING));
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
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000cf0a0, 0x567)
void CPlay::DrawDebugStatsFull() {
    if (HAS(g_debugDisplayFlags, DEBUG_DISPLAY_SUPPRESS)) {
        return;
    }

    char buf[0x200];
    char fpsScratch[0x40];
    char scratch[0x40];
    buf[0] = 0;

    sprintf(fpsScratch, "Fps = %i ", m_mgr->m_fps);
    strcat(buf, fpsScratch);

    CDDrawChildGroup* group = m_world->m_childGroup;
    if (group->m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_HIT_RECT)) {
        strcat(buf, " rcHit ");
    }
    if (group->m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_ATTACK_RECT)) {
        strcat(buf, " rcAttack ");
    }
    if (group->m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_MOVE_RECT)) {
        strcat(buf, " rcMove ");
    }
    if (group->m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_ORIGIN)) {
        strcat(buf, " ptOrg ");
    }
    if (group->m_flags & IDX(DDRAW_CHILD_GROUP_FLAG_DEBUG_SORT_KEY)) {
        strcat(buf, " Z = On");
    }

    if (HAS(g_debugDisplayFlags, DEBUG_DISPLAY_OBJECT_COUNT)) {
        sprintf(scratch, " Sprites = %i ", m_world->m_childGroup->m_list.GetCount());
        strcat(buf, scratch);
    }
    if (HAS(g_debugDisplayFlags, DEBUG_DISPLAY_WORLD_POSITION)) {
        CDDrawWorkerHost* p = m_world->m_level->m_mainPlane;
        sprintf(scratch, " Pos = %i,%i", p->m_scrollPixelX, p->m_scrollPixelY);
        strcat(buf, scratch);
    }
    if (HAS(g_debugDisplayFlags, DEBUG_DISPLAY_ELAPSED_TIME)) {
        CString t = FormatElapsedTime(g_frameTime);
        t += DATA_COMPGEN(0x00212754, " ");
        strcat(buf, t);
        t += " ";
    }
    if (HAS(g_debugDisplayFlags, DEBUG_DISPLAY_NETWORK_COUNTERS)) {
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
    if (HAS(g_debugDisplayFlags, DEBUG_DISPLAY_FRAME_RATE_LIMIT)) {
        sprintf(scratch, " FpsLimit = %i ", m_mgr->m_targetFps);
        strcat(buf, scratch);
    }

    CDDSurface* surface = m_world->m_drawTarget->m_backPair->m_surface;
    HDC hdc = NULL;
    surface->m_ddSurface->GetDC(&hdc);
    if (hdc == NULL) {
        return;
    }
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkColor(hdc, RGB(0, 0, 0));
    PostSetup(hdc);

    {
        RECT* src = &m_world->m_level->m_viewportRect;
        RECT lr = *src;
        RECT dr;
        dr.left = lr.left;
        dr.top = lr.bottom - 0x1c;
        dr.right = lr.right;
        dr.bottom = lr.bottom;
        DrawTextA(hdc, buf, -1, &dr, DT_SINGLELINE);
    }

    if (HAS(g_debugDisplayFlags, DEBUG_DISPLAY_PROFILE_TEXT)) {
        SetBkMode(hdc, OPAQUE);
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
    surface->m_ddSurface->ReleaseDC(hdc);
}

RVA(0x000cf770, 0x35e)
void CPlay::DrawDebugStats() {
    if (HAS(g_debugDisplayFlags, DEBUG_DISPLAY_SUPPRESS)) {
        return;
    }

    char buf[0x200];
    char scratch[0x40];
    buf[0] = 0;

    if (HAS(g_debugDisplayFlags, DEBUG_DISPLAY_FRAME_RATE)) {
        sprintf(scratch, "Fps = %i ", m_mgr->m_fps);
        strcat(buf, scratch);
    }
    if (HAS(g_debugDisplayFlags, DEBUG_DISPLAY_OBJECT_COUNT)) {
        sprintf(scratch, " Objs = %i ", m_world->m_childGroup->m_list.GetCount());
        strcat(buf, scratch);
    }
    if (HAS(g_debugDisplayFlags, DEBUG_DISPLAY_WORLD_POSITION)) {
        CDDrawWorkerHost* p = m_world->m_level->m_mainPlane;

        sprintf(scratch, " Pos = %i,%i", p->m_scrollPixelX, p->m_scrollPixelY);
        strcat(buf, scratch);
    }
    if (HAS(g_debugDisplayFlags, DEBUG_DISPLAY_TIMING)) {
        strcat(buf, " Timing = On ");
    }
    if (HAS(g_debugDisplayFlags, DEBUG_DISPLAY_ELAPSED_TIME)) {
        CString t = FormatElapsedTime(g_frameTime);
        t += " ";
        strcat(buf, t);
        t += " ";
    }
    if (HAS(g_debugDisplayFlags, DEBUG_DISPLAY_NETWORK_COUNTERS)) {
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

    CDDSurface* surface = m_world->m_drawTarget->m_backPair->m_surface;
    HDC hdc = NULL;
    surface->m_ddSurface->GetDC(&hdc);
    if (hdc == NULL) {
        return;
    }
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkColor(hdc, RGB(0, 0, 0));
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
            DrawTextA(hdc, buf, -1, &dr, DT_SINGLELINE);
        } else {
            TextOutA(hdc, 0, dr.top, buf, strlen(buf));
        }
    }
    surface->m_ddSurface->ReleaseDC(hdc);
}

RVA(0x000cfbb0, 0x8)
void CPlay::TickStateMgrs() {
    m_mgr->TickStateMgrs();
}

RVA(0x000cfbd0, 0x8f)
i32 CPlay::CompleteLevel() {
    QuestLevel level = CurrentQuestLevel();
    if (level == QUESTLEVEL_CAMPAIGN_LAST) {
        m_completedFinalLevel = true;
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
        if (m_mgr->m_isBuiltInBattlezLevel == 0 && m_mgr->m_isBuiltInMultiplayerLevel == 0) {
            base = WwdFile::GetMapBaseName(world);
        } else {
            base = world;
        }
        if (base.IsEmpty()) {
            return;
        }
        sprintf(g_customLevelText, "Custom Level: %s", static_cast<const char*>(base));
    }
    CDDSurface* surface = m_world->m_drawTarget->m_frontSurface->m_surface;
    if (surface == NULL) {
        return;
    }
    HDC hdc = NULL;
    surface->m_ddSurface->GetDC(&hdc);
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
    surface->m_ddSurface->ReleaseDC(hdc);
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
    m_world->m_drawTarget->m_frontSurface->m_surface->Flip(static_cast<CDDSurface*>(0));
    return 1;
}

RVA(0x000cffe0, 0x3c)
i32 CPlay::LoadImageBanks() {
    CPlay* self = this;
    if (!self->m_resourceArchive) {
        return 0;
    }
    self->m_gruntResources = self->m_resourceArchive->FindDirectoryByPath("GRUNTZ");
    if (!self->m_gruntResources) {
        return 0;
    }
    self->m_gameResources = self->m_resourceArchive->FindDirectoryByPath("GAME");
    return self->m_gameResources != NULL;
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
    RECT src = *(&m_world->m_level->m_viewportRect);
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
RVA(0x000d0120, 0x65c)
i32 CPlay::LoadCursorSprites(i32 cursorId, i32 targetValid) {
    ToolCursorId cursor = static_cast<ToolCursorId>(cursorId);
    if (this->m_cursorId == cursorId && targetValid == this->m_cursorTargetValid) {
        return 1;
    }
    if (cursor >= CURSOR_CHIP_FIRST && cursor <= CURSOR_CHIP_LAST) {
        if (this->LoadCursorAnimation("GAME_INGAMEICONZ_NORMCHIPZ", cursorId, 0, 0x64, 0) == 0) {
            return 0;
        }
        if (this->m_cursorSnapSprite != NULL) {
            this->m_cursorSnapSprite->m_stateFlags |= SPRITE_STATE_HIDDEN;
        }
        this->m_cursorOffset.m_x = 0;
        this->m_cursorOffset.m_y = 0;
        this->m_dragInhibit2 = true;
        this->m_cursorTargetValid = false;
        this->m_cursorId = cursorId;
        return 1;
    }
    if (cursor == CURSOR_POINTER) {
        if (this->LoadCursorAnimation("GAME_CURSORZ_POINTER", 1, 1, 0x64, 0) == 0) {
            return 0;
        }
        if (this->m_cursorSnapSprite != NULL) {
            this->m_cursorSnapSprite->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
        }
        this->m_cursorOffset.m_x = 0x10;
        this->m_cursorOffset.m_y = 0x10;
        this->m_cursorTargetValid = false;
        this->m_cursorId = cursorId;
        return 1;
    }
    if (cursor == CURSOR_FLAILINGGRUNT) {
        if (this->LoadCursorAnimation("GAME_CURSORZ_FLAILINGGRUNT", 1, 1, 0x64, 1) == 0) {
            return 0;
        }
        if (this->m_cursorSnapSprite != NULL) {
            this->m_cursorSnapSprite->m_stateFlags |= SPRITE_STATE_HIDDEN;
        }
        this->m_cursorOffset.m_x = 0;
        this->m_cursorOffset.m_y = 0;
        this->m_dragInhibit1 = true;
        this->m_cursorTargetValid = false;
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
    if (this->m_cursorSnapSprite != NULL) {
        this->m_cursorSnapSprite->m_stateFlags |= SPRITE_STATE_HIDDEN;
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
        CShadeTable* spr = w->m_spriteFactory->GetSel(IDX(w->m_players[id].m_color), 0);
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
    if (m_cursorAnimationActive == false) {
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
RVA(0x000d0b30, 0x200)
i32 CPlay::SaveUnderAndDrawCursor(CDDrawSurfacePair* pair) {
    i32 x = m_cursorX + m_cursorOffset.m_x;
    i32 y = m_cursorY + m_cursorOffset.m_y;

    CDDSurface* savedPixels;
    RECT* screenRect;
    RECT* savedRect;
    if (m_cursorBufferIndex == 0) {
        screenRect = &m_cursorScreenRects[0];
        savedPixels = m_cursorSavedSurfaces[0];
        savedRect = &m_cursorSavedRects[0];
    } else {
        screenRect = &m_cursorScreenRects[1];
        savedPixels = m_cursorSavedSurfaces[1];
        savedRect = &m_cursorSavedRects[1];
    }

    screenRect->left = x - m_cursorImage->m_anchorX;
    screenRect->right = m_cursorImage->m_width + screenRect->left;
    screenRect->top = y - m_cursorImage->m_anchorY;
    screenRect->bottom = m_cursorImage->m_height + screenRect->top;
    tagSIZE mode = m_mgr->GetModeSize();
    if (screenRect->left < 0) {
        screenRect->left = 0;
    }
    if (screenRect->right > mode.cx) {
        screenRect->right = mode.cx;
    }
    if (screenRect->top < 0) {
        screenRect->top = 0;
    }
    if (screenRect->bottom > mode.cy) {
        screenRect->bottom = mode.cy;
    }
    savedRect->right = screenRect->right - screenRect->left;
    savedRect->bottom = screenRect->bottom - screenRect->top;

    CDDSurface* target = pair->m_surface;
    if (target == NULL) {
        return 0;
    }

    i32 result = savedPixels->BltFast(0, 0, target, screenRect, 0x10);
    if (result != 0) {
        CDDrawDeviceManager::ReportError(NULL, 0, result);
    }

    if (m_drewThisFrame != false) {
        RECT vp = m_world->m_level->m_viewportRect;
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
        m_cursorBufferIndex = m_cursorBufferIndex == 0;
    }
    return 1;
}

// @early-stop
RVA(0x000d0db0, 0x347)
i32 CPlay::HandleDragMove(i32 keyFlags, i32 x, i32 y) {

    LevelCoordRect box;
    if (m_inGame != false) {
        return 1;
    }
    if (m_paused != false) {
        return 1;
    }
    if (m_minimap != NULL && m_statusBar->m_position != STATUSBAR_HIDDEN
        && m_statusBar->m_activeTab != TAB_GAME) {
        m_minimap->ContinueMinimapPan(keyFlags, x, y);
    }

    if (m_dragSnapActive != false) {
        if (m_statusBar == NULL) {
            return 1;
        }
        m_statusBar->SetSpritePos(m_snapOriginX + x, m_snapOriginY + y);
        goto rearm;
    }

    if (m_levelOverlayOpen != false) {
        return m_statusBar->HandlePointerDrag(keyFlags, x, y);
    }

    box = m_world->m_level->m_viewportRect;
    if (x >= box.left && x <= box.right && y >= box.top && y <= box.bottom) {

        if (m_dragInProgress != false) {
            m_statusBar->ClearTabSprites(TAB_ALL);
        }
        m_dragInProgress = false;
        if (m_worldReady != false) {

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
            CWwdSpriteObject* s = m_cursorSnapSprite;
            if (s == NULL) {
                return 1;
            }
            s->m_stateFlags |= SPRITE_STATE_HIDDEN;
            return 1;
        }

        if (m_chatBox->HitTest(x, y) == 0 && m_mgr->m_frameGate == 0 && m_inGame == false
            && m_dragInhibit1 == false && m_dragInhibit2 == false) {

            if (m_cursorId != 0) {
                if (m_cursorSnapSprite != NULL) {
                    m_cursorSnapSprite->m_stateFlags |= SPRITE_STATE_HIDDEN;
                }
            } else {
                if (m_cursorSnapSprite != NULL) {
                    m_cursorSnapSprite->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
                }
            }
            CGameLevel* v = m_world->m_level;
            LevelCoordRect* vr = &v->m_mainPlane->m_planeViewRect;
            i32 wx = vr->left - v->m_viewportRect.left + x;
            i32 wy = vr->top - v->m_viewportRect.top + y;
            m_mgr->m_triggerMgr->PlaceObjectFull(wx, wy);
            return 1;
        }
        if (m_cursorSnapSprite != NULL) {
            m_cursorSnapSprite->m_stateFlags |= SPRITE_STATE_HIDDEN;
        }
        return 1;
    }

    if (m_cursorSnapSprite != NULL) {
        m_cursorSnapSprite->m_stateFlags |= SPRITE_STATE_HIDDEN;
    }
    m_dragInProgress = true;
    m_statusBar->HandlePointerDrag(keyFlags, x, y);
    if (m_worldReady != false) {

        m_hudRect.left = m_cursorX > box.left ? m_cursorX : box.left;
        m_hudRect.left = m_hudRect.left < m_dragClampMaxX ? m_hudRect.left : m_dragClampMaxX;
        m_hudRect.right = m_cursorX < box.right ? m_cursorX : box.right;
        m_hudRect.right = m_hudRect.right > m_dragClampMaxX ? m_hudRect.right : m_dragClampMaxX;
        m_hudRect.top = m_cursorY <= box.top ? box.top : m_cursorY;
        m_hudRect.top = m_hudRect.top < m_dragClampMaxY ? m_hudRect.top : m_dragClampMaxY;
        m_hudRect.bottom = m_cursorY < box.bottom ? m_cursorY : box.bottom;
        m_hudRect.bottom = m_hudRect.bottom > m_dragClampMaxY ? m_hudRect.bottom : m_dragClampMaxY;
    }
    if (m_cursorTargetValid != false && m_mgr->m_triggerMgr->m_pendingFxKind == 0) {
        FlushPendingOps();
    }
    return 1;
}

RVA(0x000d11e0, 0x9b)
i32 CPlay::RestoreCursorSaveUnder() {
    if (m_cursorSavedSurfaceValid[0] == 0) {
        m_cursorSavedSurfaceValid[0] = 1;
        return 1;
    }
    if (m_cursorSavedSurfaceValid[1] == 0) {
        m_cursorSavedSurfaceValid[1] = 1;
        return 1;
    }

    CDDSurface* savedPixels;
    RECT* screenRect;
    RECT* savedRect;
    if (m_cursorBufferIndex == 0) {
        savedPixels = m_cursorSavedSurfaces[0];
        screenRect = &m_cursorScreenRects[0];
        savedRect = &m_cursorSavedRects[0];
    } else {
        savedPixels = m_cursorSavedSurfaces[1];
        screenRect = &m_cursorScreenRects[1];
        savedRect = &m_cursorSavedRects[1];
    }

    CDDSurface* backSurface = m_world->m_drawTarget->m_backPair->m_surface;
    if (backSurface == NULL) {
        return 0;
    }

    i32 result =
        backSurface->BltFast(screenRect->left, screenRect->top, savedPixels, savedRect, 0x10);
    if (result != 0) {
        CDDrawDeviceManager::ReportError(NULL, 0, result);
    }
    return 1;
}

// @early-stop
RVA(0x000d12b0, 0x2d5)
i32 CPlay::LoadScrollSpeedOptions() {
    DATA(0x0024c274)
    static i32 s_minScrollSpeed = g_buteMgr.GetInt("Optionz", "MinScrollSpeed");
    DATA(0x0024c270)
    static i32 s_scrollSpeedRange = g_buteMgr.GetInt("Optionz", "MaxScrollSpeed")
                                    - g_buteMgr.GetInt("Optionz", "MinScrollSpeed");

    CPlay* self = this;
    CGruntzMgr* w = m_mgr;
    i32 changed = 0;
    CDDrawWorkerHost* g = w->m_world->m_level->m_mainPlane;

    i32 sx = g->m_scrollPixelX;
    i32 sy = g->m_scrollPixelY;
    double frac = static_cast<double>(w->m_scrollSpeed) * 0.01;
    i32 speed = static_cast<i32>(frac * s_scrollSpeedRange + s_minScrollSpeed);

    SIZE
    extent;
    extent.cx = w->m_modeSize.cx;
    extent.cy = w->m_modeSize.cy;

    if (self->m_cursorX < 0xc || (self->m_scrollEdgeLock & 1)) {
        if (self->m_scrollEdgeActive & 1) {
            i32 d = (timeGetTime() - self->m_lastScrollTimeX) * speed / MILLIS_PER_SECOND;
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
            i32 d = (timeGetTime() - self->m_lastScrollTimeX) * speed / MILLIS_PER_SECOND;
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
            i32 d = (timeGetTime() - self->m_lastScrollTimeY) * speed / MILLIS_PER_SECOND;
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
            i32 d = (timeGetTime() - self->m_lastScrollTimeY) * speed / MILLIS_PER_SECOND;
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
            LevelCoordRect vp = m_world->m_level->m_viewportRect;
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

    RECT vp = m_world->m_level->m_viewportRect;
    i32 bottom = vp.bottom - g_buteMgr.GetInt("Font", "TextBottomEdge");
    i32 right = vp.right - g_buteMgr.GetInt("Font", "TextRightEdge");
    i32 top = vp.top + g_buteMgr.GetInt("Font", "TextTopEdge");
    i32 left = vp.left + g_buteMgr.GetInt("Font", "TextLeftEdge");
    SetRect(&rect, left, top, right, bottom);

    DrawTextToFrontSurface(m_world, &s, &rect, 0x78, 1, 0xff, 0xff, 0, 1);
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

        if (m_cueText.LoadString(cueId) == false) {
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

        RECT vp = m_world->m_level->m_viewportRect;
        i32 bottom = vp.bottom - g_buteMgr.GetInt("Font", "TextBottomEdge");
        i32 right = vp.right - g_buteMgr.GetInt("Font", "TextRightEdge");
        i32 top = vp.top + g_buteMgr.GetInt("Font", "TextTopEdge");
        i32 left = vp.left + g_buteMgr.GetInt("Font", "TextLeftEdge");
        SetRect(&rect, left, top, right, bottom);
    }

    if (toFrontPage != 0) {
        DrawTextToFrontSurface(m_world, &m_cueText, &rect, fontSel, 1, r, g, b, flag);
    } else {
        DrawTextToBackSurface(m_world, &m_cueText, &rect, fontSel, 1, r, g, b, flag);
    }
}

RVA(0x000d1ac0, 0x4f)
void CPlay::StepScroll() {
    CGameLevel* v = m_world->m_level;

    RECT* vr = &v->m_mainPlane->m_planeViewRect;

    i32 y = m_cursorY + (vr->top - v->m_viewportRect.top);
    i32 x = vr->left + (m_cursorX - v->m_viewportRect.left);

    y = (y & ~TILE_MASK_PX) + TILE_HALF_PX;
    x = (x & ~TILE_MASK_PX) + TILE_HALF_PX;

    m_cursorSnapSprite->m_screenX = x;
    m_cursorSnapSprite->m_screenY = y;
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

            i32 r = mgr->m_triggerMgr->PlaceObject(
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
                g_gameReg->m_triggerMgr->ResetAll();
            }
            return 1;
        }

        case PLAYERCMD_MOVE: {
            u32 player = static_cast<u8>(playerIndex);
            u32 gi = static_cast<u8>(unitIndex);
            CGrunt* g = mgr->m_triggerMgr->m_units[gi + player * 0xf];
            if (g != NULL && g->m_entranceCommitted != false) {
                g->m_arrivalActive = false;
            }
            if (!m_mgr->m_triggerMgr->ClearCell(
                    player,
                    gi,
                    static_cast<u16>(targetXOrPlayerIndex),
                    static_cast<u16>(targetYOrUnitIndex),
                    0
                )) {
                if (player != static_cast<u32>(g_curPlayer) || g == NULL
                    || g->m_entranceCommitted == false) {
                    return 0;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (player != static_cast<u32>(g_curPlayer) || g == NULL
                || g->m_entranceCommitted == false) {
                return 1;
            }
            g_gameReg->m_voiceManager->PlayVoice(g, 0x323, -1, 0, -1, -1);
            return 1;
        }

        case PLAYERCMD_GUARD_BEGIN: {
            CGrunt* g =
                mgr->m_triggerMgr
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
                    g->m_arrivalActive = false;
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
                mgr->m_triggerMgr
                    ->m_units[static_cast<u8>(playerIndex) * 0xf + static_cast<u8>(unitIndex)];
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
            CGrunt* g = mgr->m_triggerMgr->m_units[gi + player * 0xf];
            if (g == NULL || g->m_entranceCommitted == false) {
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
                m_mgr->m_triggerMgr
                    ->CellHitTest(px, py, &hitPlayerIndex, &hitUnitIndex, TM_ALL_PLAYERS);
            if (node != NULL && g->m_entranceActive == false) {
                g->SetArrivalTarget(
                    hitPlayerIndex,
                    hitUnitIndex,
                    node->m_object->m_screenX,
                    node->m_object->m_screenY
                );
            } else {
                g->m_arrivalActive = false;
            }
            res = m_mgr->m_triggerMgr->UseEquippedToolAt(player, gi, px, py);
            if (res == 0) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == false) {
                    return 0;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (res == -1) {
                if (!m_mgr->m_triggerMgr->ClearCell(player, gi, px, py, 2)) {
                    if (player != static_cast<u32>(g_curPlayer)
                        || g->m_entranceCommitted == false) {
                        return 0;
                    }
                    g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                    return 0;
                }
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == false) {
                    return 1;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x323, -1, 0, -1, -1);
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == false) {
                return 1;
            }
            g_gameReg->m_voiceManager->PlayVoice(g, 0x323, -1, 0, -1, -1);
            return 1;
        }

        case PLAYERCMD_USE_TOOL_ON_GRUNT: {
            u32 player = static_cast<u8>(playerIndex);
            u32 gi = static_cast<u8>(unitIndex);
            CGrunt* g = mgr->m_triggerMgr->m_units[gi + player * 0xf];
            if (g == NULL || g->m_entranceCommitted == false) {
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
            CGrunt* g2 = m_mgr->m_triggerMgr
                             ->m_units[targetUnitIndex + targetPlayerIndex * TM_UNITS_PER_PLAYER];
            if (g2 == NULL || g->m_entranceActive != false) {
                g->m_arrivalActive = false;
                return 0;
            }
            i32 sx = g2->m_object->m_screenX;
            i32 sy = g2->m_object->m_screenY;
            g->SetArrivalTarget(targetPlayerIndex, targetUnitIndex, sx, sy);
            res = m_mgr->m_triggerMgr->UseEquippedToolAt(player, gi, sx, sy);
            if (res == 0) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == false) {
                    return 0;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (res == -1) {
                if (!m_mgr->m_triggerMgr->ClearCell(player, gi, sx, sy, 2)) {
                    if (player != static_cast<u32>(g_curPlayer)
                        || g->m_entranceCommitted == false) {
                        return 0;
                    }
                    g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                    return 0;
                }
                if (player != static_cast<u32>(g_curPlayer)
                    || static_cast<u32>(g_curPlayer) == static_cast<u32>(targetPlayerIndex)
                    || g->m_entranceCommitted == false) {
                    return 1;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x325, -1, 0, -1, -1);
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer)
                || static_cast<u32>(g_curPlayer) == static_cast<u32>(targetPlayerIndex)
                || g->m_entranceCommitted == false) {
                return 1;
            }
            g_gameReg->m_voiceManager->PlayVoice(g, 0x325, -1, 0, -1, -1);
            return 1;
        }

        case PLAYERCMD_USE_TOY_AT_POINT: {
            u32 player = static_cast<u8>(playerIndex);
            u32 gi = static_cast<u8>(unitIndex);
            CGrunt* g = mgr->m_triggerMgr->m_units[gi + player * 0xf];
            if (g == NULL || g->m_entranceCommitted == false || g->m_entranceActive != false) {
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
                m_mgr->m_triggerMgr
                    ->CellHitTest(px, py, &hitPlayerIndex, &hitUnitIndex, TM_ALL_PLAYERS);
            if (node != NULL && g->m_entranceActive == false) {
                g->SetArrivalTarget(
                    hitPlayerIndex,
                    hitUnitIndex,
                    node->m_object->m_screenX,
                    node->m_object->m_screenY
                );
            } else {
                g->m_arrivalActive = false;
            }
            res = m_mgr->m_triggerMgr->UseToyAt(player, gi, px, py);
            if (res == 0) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == false) {
                    return 0;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (res == -1) {
                if (!m_mgr->m_triggerMgr->ClearCell(player, gi, px, py, 3)) {
                    if (player != static_cast<u32>(g_curPlayer)
                        || g->m_entranceCommitted == false) {
                        return 0;
                    }
                    g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                    return 0;
                }
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == false) {
                    return 1;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x323, -1, 0, -1, -1);
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == false) {
                return 1;
            }
            g_gameReg->m_voiceManager->PlayVoice(g, 0x323, -1, 0, -1, -1);
            return 1;
        }

        case PLAYERCMD_USE_TOY_ON_GRUNT: {
            u32 player = static_cast<u8>(playerIndex);
            u32 gi = static_cast<u8>(unitIndex);
            CGrunt* g = mgr->m_triggerMgr->m_units[gi + player * 0xf];
            if (g == NULL || g->m_entranceCommitted == false || g->m_entranceActive != false) {
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
            CGrunt* g2 = m_mgr->m_triggerMgr
                             ->m_units[targetUnitIndex + targetPlayerIndex * TM_UNITS_PER_PLAYER];
            if (g2 == NULL || g->m_entranceActive != false) {
                g->m_arrivalActive = false;
                return 0;
            }
            i32 sx = g2->m_object->m_screenX;
            i32 sy = g2->m_object->m_screenY;
            g->SetArrivalTarget(targetPlayerIndex, targetUnitIndex, sx, sy);
            res = m_mgr->m_triggerMgr->UseToyAt(player, gi, sx, sy);
            if (res == 0) {
                if (player != static_cast<u32>(g_curPlayer) || g->m_entranceCommitted == false) {
                    return 0;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                return 0;
            }
            if (res == -1) {
                if (!m_mgr->m_triggerMgr->ClearCell(player, gi, sx, sy, 3)) {
                    if (player != static_cast<u32>(g_curPlayer)
                        || g->m_entranceCommitted == false) {
                        return 0;
                    }
                    g_gameReg->m_voiceManager->PlayVoice(g, 0x324, -1, 0, -1, -1);
                    return 0;
                }
                if (player != static_cast<u32>(g_curPlayer)
                    || static_cast<u32>(g_curPlayer) == static_cast<u32>(targetPlayerIndex)
                    || g->m_entranceCommitted == false) {
                    return 1;
                }
                g_gameReg->m_voiceManager->PlayVoice(g, 0x325, -1, 0, -1, -1);
                return 1;
            }
            if (player != static_cast<u32>(g_curPlayer)
                || static_cast<u32>(g_curPlayer) == static_cast<u32>(targetPlayerIndex)
                || g->m_entranceCommitted == false) {
                return 1;
            }
            g_gameReg->m_voiceManager->PlayVoice(g, 0x325, -1, 0, -1, -1);
            return 1;
        }

        case PLAYERCMD_GIVE_TOOL: {
            u32 player = static_cast<u8>(playerIndex);
            if (player == static_cast<u32>(g_curPlayer)) {
                m_playerCommandPending = false;
            }
            u32 gi = static_cast<u8>(unitIndex);
            i32 idx = gi + player * 0xf;
            CGrunt* g = mgr->m_triggerMgr->m_units[idx];
            if (g != NULL && g->m_entranceCommitted != false && g->m_tileClaimed != 0) {
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
            i32 live = (g_gameReg->m_gameMode != GAMEMODE_QUESTZ);
            CGrunt* g2 = m_mgr->m_triggerMgr->m_units[idx];
            i32 r;
            if (g2 == NULL || g2->m_entranceCommitted == false) {
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
                    m_mgr->m_triggerMgr->ResetCell(player, gi, 0, 0);
                }
                sel = 1;
            }
            if (player == static_cast<u32>(g_curPlayer)) {
                m_dragInhibit2 = false;
                m_statusBar->EnterHlRow(sel, m_cursorFrame);
                SetCursorFrame(0);
            }
            return r;
        }

        case PLAYERCMD_STOP: {
            CGrunt* g =
                mgr->m_triggerMgr
                    ->m_units[static_cast<u8>(playerIndex) * 0xf + static_cast<u8>(unitIndex)];
            if (g == NULL || g->m_entranceCommitted == false || g->m_entranceActive != false) {
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
    } else if (x >= g->m_planePixelWidth) {
        x = g->m_planePixelWidth - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= g->m_planePixelHeight) {
        y = g->m_planePixelHeight - 1;
    }
    i32 tx = x >> g->m_shiftX;
    i32 ty = y >> g->m_shiftY;
    i32 subX = x - (tx << g->m_shiftX);
    i32 subY = y - (ty << g->m_shiftY);
    i32 cell = g->GetTileHandle(tx, ty);
    if (cell == UNINIT_FILL || cell == -1) {
        return TILEKIND_PASSABLE;
    }

    CUniformTileImageSet* tc = static_cast<CUniformTileImageSet*>(
        level->m_imageSets.GetAt(cell & WWD_TILE_IMAGE_SET_INDEX_MASK)
    );
    return tc->GetCollisionAt(subX, subY);
}

static inline TileCollisionKind LookupTileTypeDirect(CGameLevel* level, i32 x, i32 y) {
    CDDrawWorkerHost* g = level->m_mainPlane;
    if (x < 0) {
        x = 0;
    } else if (x >= g->m_planePixelWidth) {
        x = g->m_planePixelWidth - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= g->m_planePixelHeight) {
        y = g->m_planePixelHeight - 1;
    }
    i32 tx = x >> g->m_shiftX;
    i32 ty = y >> g->m_shiftY;
    i32 subX = x - (tx << g->m_shiftX);
    i32 subY = y - (ty << g->m_shiftY);
    i32 cell = g->m_tileHandles[g->m_tileRowOffsets[ty] + tx];
    if (cell == UNINIT_FILL || cell == -1) {
        return TILEKIND_PASSABLE;
    }

    CUniformTileImageSet* tc = static_cast<CUniformTileImageSet*>(
        level->m_imageSets.GetAt(cell & WWD_TILE_IMAGE_SET_INDEX_MASK)
    );
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
    if (m_mgr->m_gameMode == GAMEMODE_QUESTZ) {
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
                i32 idx = m_mgr->m_triggerMgr->PlaceObject(
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
                obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
            } else if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ
                       && dispatch == DispatchGruntCreationPointLogic
                       && obj->m_smarts == g_curPlayer) {

                GruntzPlayer* e = &g_gameReg->m_players[g_curPlayer];
                if (e != NULL && counter < e->m_maxGruntz) {
                    i32 x = (obj->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
                    i32 y = (obj->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
                    m_mgr->m_commandMgr->EnqueueSingle(
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
                b32 found = false;
                i32 colOff = col << 8;
                while (found == false && col < obj->m_speedX + 2) {
                    row = obj->m_speedY - 1;
                    while (found == false && row < obj->m_speedY + 2) {
                        hit = m_tileTriggers->FindLogic(row + colOff, TRIGID_GIANT_ROCK_22);
                        if (hit != NULL) {
                            found = true;
                        }
                        if (found == false) {
                            row++;
                        }
                    }
                    if (found == false) {
                        col++;
                        colOff += 0x100;
                    }
                }
                if (found == false) {
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
                type =
                    (static_cast<CUniformTileImageSet*>(LevelOf(m_world)->m_imageSets.GetAt(tcidx)))
                        ->GetCollisionAt(0, 0);
            }
            if (type == TILEKIND_GAUNTLET_ROCK_A || type == TILEKIND_GAUNTLET_ROCK_B
                || type == TILEKIND_COVERED_POWERUP || type == TILEKIND_REVEALED_POWERUP) {

                CTileTriggerLogic* r =
                    m_tileTriggers->FindLogic(obj->m_id, TRIGID_COVERED_POWERUP_26);
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
                type =
                    (static_cast<CUniformTileImageSet*>(LevelOf(m_world)->m_imageSets.GetAt(tcidx)))
                        ->GetCollisionAt(0, 0);
            }
            switch (type) {
                case TILEKIND_MULTI_SWITCH:
                case TILEKIND_MULTI_SWITCH_UP:
                    if (!m_tileTriggers->AddSwitchLogic(
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
                    obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
                    break;
                case TILEKIND_EXCLUSIVE_SWITCH:
                case TILEKIND_EXCLUSIVE_SWITCH_UP:
                    if (!m_tileTriggers->AddSwitchLogic(
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
                    obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
                    break;
                case TILEKIND_SECRET_SWITCH:
                    g_gameReg->m_gameStats->m_secretsAvailable++;
                    // fall through
                case TILEKIND_SECRET_SWITCH_UP:
                    if (!m_tileTriggers->AddSwitchLogic(
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
                    obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
                    break;
                case TILEKIND_TIME_SWITCH:
                case TILEKIND_TIME_SWITCH_UP:
                    if (!m_tileTriggers->AddSwitchLogic(
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
                    obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
                    break;
                case TILEKIND_CHECKPOINT:
                case TILEKIND_CHECKPOINT_UP:
                    if (!m_tileTriggers->AddSwitchLogic(
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
                    obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
                    break;
                case TILEKIND_SWITCH_A:
                case TILEKIND_SWITCH_A_UP:
                    if (!m_tileTriggers->AddSwitchLogic(
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
                    obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
                    break;
                case TILEKIND_SWITCH_B:
                case TILEKIND_SWITCH_B_UP:
                    if (!m_tileTriggers->AddSwitchLogic(
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
                    obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
                    break;
                case TILEKIND_SWITCH_C:
                case TILEKIND_SWITCH_C_UP:
                    if (!m_tileTriggers->AddSwitchLogic(
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
                    obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
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
                b32 found = false;
                i32 colOff = col << 8;
                while (found == false && col < obj->m_speedX + 2) {
                    row = obj->m_speedY - 1;
                    while (found == false && row < obj->m_speedY + 2) {
                        hit = m_tileTriggers->FindLogic(row + colOff, TRIGID_GIANT_ROCK_22);
                        if (hit != NULL) {
                            found = true;
                        }
                        if (found == false) {
                            row++;
                        }
                    }
                    if (found == false) {
                        col++;
                        colOff += 0x100;
                    }
                }
                if (found == false) {
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
                type =
                    (static_cast<CUniformTileImageSet*>(LevelOf(m_world)->m_imageSets.GetAt(tcidx)))
                        ->GetCollisionAt(0, 0);
            } else if (type == TILEKIND_GAUNTLET_ROCK_A || type == TILEKIND_GAUNTLET_ROCK_B
                       || type == TILEKIND_COVERED_POWERUP) {

                CTileTriggerLogic* r =
                    m_tileTriggers->FindLogic(obj->m_id, TRIGID_COVERED_POWERUP_26);
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
                type =
                    (static_cast<CUniformTileImageSet*>(LevelOf(m_world)->m_imageSets.GetAt(tcidx)))
                        ->GetCollisionAt(0, 0);
            }
            if (type >= TILEKIND_TOGGLE_BRIDGE_FIRST && type <= TILEKIND_TOGGLE_BRIDGE_LAST) {
                if (!m_tileTriggers->AddLogic(
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
                obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
            } else {
                if (!m_tileTriggers->AddLogic(
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
                obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
            }
        } else if (dispatch == DispatchTileSecretTriggerLogic) {
            TileCollisionKind type =
                LookupTileTypeDirect(LevelOf(m_world), obj->m_screenX, obj->m_screenY);
            if (!m_tileTriggers->AddLogic(
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
            obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        } else if (dispatch == DispatchLevelTimeLogic) {

            if (m_levelTimer != NULL && m_mgr->m_gameMode != GAMEMODE_MULTIPLAYER
                && g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                i32 seconds = obj->m_points;
                i32 minutes = obj->m_score;
                seconds += seconds;
                minutes += minutes;
                if (seconds > 0x3b) {
                    minutes++;
                    seconds -= 0x3c;
                }
                m_levelTimer->SetTime(minutes, seconds);
            }
            obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        } else if (dispatch == DispatchInGameIconLogic) {
            if (obj->m_smarts == IDX(PICKUP_MEGAPHONE)) {

                m_statusBar->QueuePickupReward(obj->m_points, obj->m_score);
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
            i32 tile = pl->m_tileHandles[pl->m_tileRowOffsets[obj->m_speedY] + obj->m_speedX];
            if (tile >= 0x12f && tile <= 0x149) {
                if (m_tileTriggers->AddActionEvent(
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
                obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
            } else {
                CString s;
                s.Format("Bad brickz at: x=%d, y=%d", obj->m_screenX, obj->m_screenY);
                g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));
                return 0;
            }
        } else if (dispatch == DispatchGruntPuddleLogic) {

            m_mgr->m_triggerMgr->PlacePuddle(obj, 0);
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
            if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
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
    return m_tileHandles[m_tileRowOffsets[tileY] + tileX];
}

// @early-stop
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
            if (m_tileTriggers->AddGiantRockLogic(
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
                m_statusBar->QueuePickupReward(p->m_points, p->m_score);
            }
            p->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        } else if (dispatch == DispatchCoveredPowerupLogic) {
            CGameLevel* ds = m_world->m_level;
            i32 x = p->m_screenX;
            i32 y = p->m_screenY;
            if (x < 0) {
                x = 0;
            } else {
                i32 lim = ds->m_mainPlane->m_planePixelWidth;
                if (x >= lim) {
                    x = lim - 1;
                }
            }
            if (y < 0) {
                y = 0;
            } else {
                i32 lim = ds->m_mainPlane->m_planePixelHeight;
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
            i32 cell = g->m_tileHandles[g->m_tileRowOffsets[tileY] + tileX];
            TileCollisionKind tile;
            if (cell == UNINIT_FILL || cell == static_cast<i32>(0xffffffff)) {
                tile = TILEKIND_PASSABLE;
            } else {

                tile = (static_cast<CUniformTileImageSet*>(
                            ds->m_imageSets[cell & WWD_TILE_IMAGE_SET_INDEX_MASK]
                        ))
                           ->GetCollisionAt(subX, subY);
            }
            if (m_tileTriggers->AddLogic(
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
                m_statusBar->QueuePickupReward(p->m_points, p->m_score);
            }
            p->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        }
    }
    return 1;
}

// @early-stop
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

        i32 r = m_mgr->m_triggerMgr->PlaceObject(
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
        g->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
    }
    return 1;
}

// @early-stop
RVA(0x000d5b20, 0xbb)
i32 CPlay::PositionBridgeToggle(StatusBarDock mode, StatusBarDock) {
    CGruntzMgr* w = m_mgr;
    i32 ex = w->m_modeSize.cx;
    i32 ey = w->m_modeSize.cy;
    CTimer* pt;
    if (mode == STATUSBAR_DOCK_LEFT) {
        m_chatBox->Configure(CHATBOX_WITH_LEFT_STATUSBAR);
        pt = m_levelTimer;
        if (pt == NULL) {
            goto done;
        }
        ex -= 0x37;
        ey -= 0x16;
        pt->m_baseX = ex;
        pt->m_baseY = ey;
    } else if (mode == STATUSBAR_DOCK_RIGHT) {
        m_chatBox->Configure(CHATBOX_WITH_RIGHT_STATUSBAR);
        pt = m_levelTimer;
        if (pt == NULL) {
            goto done;
        }
        ex -= 0xd7;
        ey -= 0x16;
        pt->m_baseX = ex;
        pt->m_baseY = ey;
    } else {
        m_chatBox->Configure(CHATBOX_WITH_HIDDEN_STATUSBAR);
        pt = m_levelTimer;
        if (pt == NULL) {
            goto done;
        }
        ex -= 0x37;
        ey -= 0x16;
        pt->m_baseX = ex;
        pt->m_baseY = ey;
    }
done:

    if (m_mgr->m_triggerMgr->m_goal != NULL) {
        CTriggerMgr* g = m_mgr->m_triggerMgr;
        if (g->m_goal != NULL) {
            g->m_goal->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
            g->m_goal = NULL;
        }
        m_mgr->m_triggerMgr->LoadCameraSprite();
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000d5c10, 0x10d)
i32 CState::DrawScreenTextImage(const char* name) {
    char buf[0x40];
    sprintf(buf, "\\SCREENZ\\%sTEXT", name);
    CRezArchiveEntry* src = StateResources()->FindEntryByPath(buf, IMGTAG_DIP);
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
    CTriggerMgr* g = w->m_triggerMgr;
    if (g->m_goal != NULL) {
        g->m_goal->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
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
    GruntzPlayer* slot = &g_gameReg->m_players[id];

    if (slot != NULL && g_gameReg->m_triggerMgr->m_unitCountByPlayer[id] < slot->m_maxGruntz) {
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
RVA(0x000d60b0, 0x2cd)
i32 CPlay::ResetPlayState() {
    char sequenceName[0x40];
    if (m_mgr->m_musicEnabled != 0 && g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        m_ambientTiming.m_interval.m_lo = AMBIENT_INTRO_INTERVAL_MS;
        m_ambientTiming.m_interval.m_hi = 0;
        m_ambientTiming.m_start.m_lo = g_frameTime;
        m_ambientTiming.m_start.m_hi = 0;
        wsprintfA(sequenceName, "INTRO%d", GetAmbientId());
        if (g_gameReg->m_musicEnabled != 0) {
            m_mgr->m_midi->PlaySequence(sequenceName, 0);
        }
        m_ambientInitDone = false;
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
        if (gameManager->m_musicEnabled != 0 && gameManager->m_gameMode == GAMEMODE_BATTLEZ) {
            m_mgr->m_midi->PlaySequence(sequenceName, 1);
        }
        m_ambientTiming.m_start.m_lo = 0;
        m_ambientTiming.m_interval.m_lo = 0;
        m_ambientTiming.m_start.m_hi = 0;
        m_ambientTiming.m_interval.m_hi = 0;
        m_ambientInitDone = true;
    }
    if (m_mgr->m_gameMode == GAMEMODE_QUESTZ) {
        CGruntzMgr* reg = g_gameReg;

        if (reg->m_strWorldFile.GetLength() == 0) {
            m_mgr->m_gameStats->UpdateLevelRecord(m_levelIndex, 1);
            reg = g_gameReg;

            if (reg->m_cheatMgr->m_cheatsUsed == 0) {
                i32 id = m_levelIndex;
                if (id > 0x24 || id == 1) {
                    (static_cast<CSaveGame*>(reg->m_saveGame))
                        ->SetMaxLevel(static_cast<QuestLevel>(id));
                    reg = g_gameReg;
                }
            }
            (static_cast<CSaveGame*>(reg->m_saveGame))->Save(NULL, 0x81a6);
        }
        CGameLevel* g = m_mgr->m_world->m_level;
        ResetGoals(g->m_header.startX, g->m_header.startY);
    } else {
        GruntzPlayer* slot = &g_gameReg->m_players[g_curPlayer];
        if (slot != NULL) {
            ResetGoals(slot->m_focusX, slot->m_focusY);
        } else {
            CGameLevel* g = m_mgr->m_world->m_level;
            ResetGoals(g->m_header.startX, g->m_header.startY);
        }
    }
    if (m_cursorSnapSprite != NULL) {
        m_cursorSnapSprite->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    }
    m_inGame = false;
    if (!PlaceStartGruntz()) {
        return 0;
    }
    for (i32 i = 0; i < 4; i++) {
        g_gameReg->m_players[i].m_battlezConfig.StepAllRowSpawns();
    }
    m_winLoseBanner = 0;
    CTimer* fm = m_levelTimer;
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
    CTriggerMgr* tl = m_mgr->m_triggerMgr;
    tl->m_countdownActive = true;
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
    if (m_levelOverlayOpen != false) {
        return 1;
    }
    m_levelOverlayOpen = true;
    m_worldReady = false;
    m_dragSnapActive = false;
    FlushPendingOps();
    if (showQuitConfirmation == 0) {
        CStatusBarMgr* g = m_statusBar;
        if (g->m_position == STATUSBAR_HIDDEN) {
            g->RestoreStatusBar();
        }
        if (g->m_activeTab != TAB_GAME) {
            g->SetTabState(SBICMD_TAB_GAME, MENUITEM_SELECTED);
        }
        g->SetTab(GAME_TAB_MISSION_STATUS, 1);
        g->Deactivate();
    }
    m_statusBar->BuildGameTabResumeButton(1);
    CStatusBarMgr* g = m_statusBar;
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
    if (m_levelOverlayOpen != false) {
        CStatusBarMgr* worker = m_statusBar;
        m_levelOverlayOpen = false;
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
    if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
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
                    m_mgr->m_gameStats->m_toolzAvailable++;
                } else if (cv >= PICKUP_TOYZ_FIRST && cv <= PICKUP_TOYZ_LAST) {
                    m_mgr->m_gameStats->m_toyzAvailable++;
                } else if (cv >= PICKUP_TIMEDPOWERUP_FIRST && cv <= PICKUP_TIMEDPOWERUP_LAST) {
                    m_mgr->m_gameStats->m_powerupzAvailable++;
                } else if (cv == PICKUP_COIN) {
                    m_mgr->m_gameStats->m_coinsAvailable++;
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
                    m_mgr->m_gameStats->m_toolzAvailable++;
                } else if (cv >= PICKUP_TOYZ_FIRST && cv <= PICKUP_TOYZ_LAST) {
                    m_mgr->m_gameStats->m_toyzAvailable++;
                } else if (cv >= PICKUP_TIMEDPOWERUP_FIRST && cv <= PICKUP_TIMEDPOWERUP_LAST) {
                    m_mgr->m_gameStats->m_powerupzAvailable++;
                } else if (cv == PICKUP_COIN) {
                    m_mgr->m_gameStats->m_coinsAvailable++;
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
    m_statusBar->Deactivate();
    m_statusBar->UpdateStatusBar(0);
    m_mgr->RefreshGameClock();

    if (m_initialFramePending != false) {
        m_initialFramePending = false;
        m_world->m_drawTarget->m_backPair->m_surface->Fill(0);
        UpdateMgrScroll(g_gameReg, m_statusBar, m_region0Gate);
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
        m_statusBar->Deactivate();
        m_statusBar->LoadMainStatusBarSprite();
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
        m_statusBar->Deactivate();
        m_statusBar->LoadMainStatusBarSprite();
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
    m_cursorSavedSurfaceValid[0] = 0;
    m_cursorSavedSurfaceValid[1] = 0;
    m_cursorBufferIndex = 0;
    if (m_mgr->m_soundEnabled != 0 && mode != GAMESTATE_HELP) {
        m_mgr->m_worldSounds->Resume();
    }
    if (mode == GAMESTATE_HELP) {
        g_frameTime = m_savedClock;
    }
    m_statusBar->Deactivate();
    RegisterInputBindings();
    m_hudSuppressed = false;
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
    m_paused = true;

    PostMessageA(g_gameReg->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_FINISH_LEVEL), 0);
    if (m_cursorSnapSprite) {
        m_cursorSnapSprite->m_stateFlags |= SPRITE_STATE_HIDDEN;
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
    if (counter < 0x37 && final != 1) {
        LayerBlitFrame(m_world, static_cast<CImage*>(m_revealCapMid), col + 0xe0, 0x1a6, 1, 0);
    } else {
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
i32 CPlay::SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload) {
    if (ar == NULL) {
        return 0;
    }
    if (!SerializeHeader(ar, mode, typeId, payload)) {
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
                    IDX(gameManager->m_players[playerIndex].m_color),
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
            m_ambientInitDone = true;
            break;
        }
    }

    i32* p;
    p = &m_syncTiming.m_start.m_lo;
    SYNC_PAIR(ar, mode, p);
    if (!m_statusBar->SerializeDispatch(ar, mode, typeId, payload)) {
        return 0;
    }
    if (!m_levelTimer->SerializeDispatch(ar, mode, typeId, payload)) {
        return 0;
    }
    p = &m_cueTiming.m_start.m_lo;
    SYNC_PAIR(ar, mode, p);
    if (!m_tileTriggers->Serialize(ar, mode, typeId, payload)) {
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
        if (m_cursorSnapSprite != NULL) {
            v = m_cursorSnapSprite->m_objectId;
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
    s->Write(&m_viewportResizeMode, sizeof(m_viewportResizeMode));
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

    g_serialCounter++;
    char nameBuf[SERIAL_NAME_LEN];
    ar->Read(nameBuf, SERIAL_NAME_LEN);
    {
        i32 idx;
        ar->Read(&idx, sizeof(idx));
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
        CWwdSpriteObject* sink;
        if (MapLookup(
                res->m_childGroup->m_registeredGameObjectsById,
                static_cast<void*>(found),
                oe
            )) {
            if (oe == NULL) {
                sink = NULL;
            } else {
                sink = oe->GetClassId() == CLASSID_SERIALREF ? static_cast<CWwdSpriteObject*>(oe)
                                                             : NULL;
            }
        } else {
            sink = NULL;
        }
        m_cursorSnapSprite = sink;
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
    ar->Read(&m_viewportResizeMode, sizeof(m_viewportResizeMode));
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
        m_viewportResizeMode = VIEW_RESIZE_SHRINK;
    } else {
        m_region0Gate = 0;
        RegionLeave();
        m_viewportResizeMode = VIEW_RESIZE_EXPAND;
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
        g_gameReg->m_triggerMgr->CycleMoveIcons(-1, 0);
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
    StatusBarDock state = m_statusBar->m_position;
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
    m_viewportResizeMode = VIEW_RESIZE_IDLE;
    m_world->m_level->UpdatePlaneViewports((&r));
    m_mgr->RecomputeViewScale();
    return 1;
}

RVA(0x000d8d90, 0x1e)
i32 CPlay::StepViewportResize() {
    ViewportResizeMode mode = m_viewportResizeMode;
    if (mode == VIEW_RESIZE_IDLE) {
        return 0;
    }
    if (mode == VIEW_RESIZE_SHRINK) {
        return ShrinkViewport(4);
    }
    return ExpandViewport(4);
}

RVA(0x000d8dc0, 0xce)
i32 CPlay::ShrinkViewport(i32 step) {
    CDDrawSurfaceMgr* world = m_world;
    i32 changed = 0;
    LevelCoordRect* viewport = &world->m_level->m_viewportRect;
    RECT resized = *viewport;

    if (resized.right - resized.left > 0xc0) {
        resized.left += step;
        resized.right -= step;
        changed = 1;
    }
    if (resized.bottom - resized.top > 0xc0) {
        resized.top += step;
        resized.bottom -= step;
        changed = 1;
    }
    if (changed == 0) {
        ResetViewport();
        return 0;
    }

    m_world->m_level->UpdatePlaneViewports((&resized));
    m_world->m_drawTarget->m_backPair->m_surface->Fill(0);
    m_statusBar->Deactivate();
    m_mgr->RecomputeViewScale();
    return 1;
}

// @early-stop
RVA(0x000d8ed0, 0x128)
i32 CPlay::ExpandViewport(i32 step) {
    i32 changed = 0;
    CDDrawSurfaceMgr* world = m_world;
    CGruntzMgr* manager = m_mgr;
    CStatusBarMgr* statusBar = m_statusBar;

    LevelCoordRect* viewport = &world->m_level->m_viewportRect;
    RECT resized = *viewport;

    SIZE
    modeSize;
    modeSize.cx = manager->m_modeSize.cx;
    modeSize.cy = manager->m_modeSize.cy;

    if (resized.right - resized.left
        < (statusBar->m_position == STATUSBAR_HIDDEN ? modeSize.cx
                                                     : modeSize.cx - STATUSBAR_WIDTH_PX)) {
        resized.left -= step;
        resized.right += step;
        if (resized.left < 0) {
            resized.left = 0;
        }
        if (resized.right >= modeSize.cx) {
            resized.right = modeSize.cx - 1;
        }
        changed = 1;
    }
    if (resized.bottom - resized.top < modeSize.cy) {
        resized.top -= step;
        resized.bottom += step;
        if (resized.top < 0) {
            resized.top = 0;
        }
        if (resized.bottom >= modeSize.cy) {
            resized.bottom = modeSize.cy - 1;
        }
        changed = 1;
    }

    if (changed == 0) {
        ResetViewport();
        return 0;
    }

    m_world->m_level->UpdatePlaneViewports((&resized));
    m_world->m_drawTarget->m_backPair->m_surface->Fill(0);
    m_statusBar->Deactivate();
    m_mgr->RecomputeViewScale();
    return 1;
}

RVA(0x000d9050, 0xc7)
i32 CPlay::NotifyVisibleEntities() {
    CDDrawSurfaceMgr* v = m_world;
    const LevelCoordRect& vp = v->m_level->m_viewportRect;
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
    if (mode == GAMEMODE_QUESTZ) {
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
    } else if (mode == GAMEMODE_BATTLEZ) {
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
    DrawTextToFrontSurface(m_world, &s0, &r1, 0x78, 0, 0, 0, 0, 1);
    DrawTextToFrontSurface(m_world, &s1, &r2, 0x6e, 0, 0, 0, 0, 1);
    DrawTextToFrontSurface(m_world, &s2, &r3, 0x6e, 0, 0, 0, 0, 1);
    DrawTextToFrontSurface(m_world, &s3, &r4, 0x6e, 0, 0, 0, 0, 1);
    return 1;
}

// @early-stop
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
    if (gr->m_gameMode == GAMEMODE_QUESTZ && gr->m_isCustomLevel == 0) {
        return (m_levelIndex + 1) % 2;
    }
    DATA(0x0024c26c)
    static i32 s_ambientCoin = GetRandomNumber() % 2;
    return s_ambientCoin;
}

RVA(0x000da2d0, 0xa5)
i32 CPlay::FlushPendingOps() {
    if (m_playerCommandPending != false) {
        return 0;
    }
    i32 changed = 0;
    if (m_dragInhibit1 != false) {
        CStatusBarMgr* worker = m_statusBar;
        m_dragInhibit1 = false;
        worker->CommitSlot(0);
        SetCursorFrame(0);
        changed = 1;
    }
    if (m_dragInhibit2 != false) {
        i32 spr = m_cursorFrame;
        CStatusBarMgr* worker = m_statusBar;
        m_dragInhibit2 = false;
        worker->EnterHlRow(0, spr);
        SetCursorFrame(0);
        changed = 1;
    }
    CTriggerMgr* fx = g_gameReg->m_triggerMgr;
    if (fx->m_pendingFxKind != 0) {
        changed = 1;
    }
    fx->m_pendingFxKind = 0;
    LoadCursorSprites(0, 0);
    return changed;
}

RVA(0x000da3b0, 0x6e)
i32 CPlay::CanQuickSave() {
    if (m_renderDisabled == false && m_inGame == false && m_levelOverlayOpen == false
        && m_defeatCountdownActive == false && m_statusBar->m_hlBusy == 0
        && m_statusBar->m_levelOverlayActive == 0 && m_statusBar->m_quitConfirmationActive == 0
        && g_gameReg->m_frameGate == 0 && g_gameReg->m_triggerMgr->m_groupFlag != 0) {
        return 1;
    }
    return 0;
}

RVA(0x000da440, 0x60)
i32 CPlay::PostHudRect() {
    if (m_worldReady != false) {
        m_mgr->m_triggerMgr->HudRect(
            m_hudRect,
            g_gameplayInput->m_heldButtons & IDX(INPUT_BUTTON5)
        );
    }
    m_worldReady = false;
    m_dragSnapActive = false;
    return 1;
}
