#include <rva.h>

#include <Gruntz/Play.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
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
#include <Dsndmgr/GruntzSoundZ.h>
#include <EmptyString.h>
#include <Enums.h>
#include <Gruntz/ActionOptionsMenuBar.h>
#include <Gruntz/AreaMgr.h>
#include <Gruntz/BankMgr.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CBrickz.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/CheatMgr.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/DrawDebugStats.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GameText.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/ImageSets.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/LightFxRender.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MgrAutoScroll.h>
#include <Gruntz/Multi.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/PlayerCommandKind.h>
#include <Gruntz/PlayPlaneScan.h>
#include <Gruntz/Random.h>
#include <Gruntz/SBI_Image.h>
#include <Gruntz/SbiMenuItemState.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/StateMgrBZ.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarTab.h>
#include <Gruntz/String.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/Timer.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/View.h>
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
#include <Wap32/EngStr.h>
#include <Wap32/Object.h>
#include <Wwd/WwdFile.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <ddraw.h>
#include <stdio.h>
#include <string.h>

inline void* operator new(u32, void* p) {
    return p;
}

class CImage;

GZ_ENUM_CONST_BEGIN(PlayIntervalMs)
    CUE_INTERVAL_MS = 0x1f4,
    BOOTY_INTERVAL_MS = 0x2710,
    REGION_INTERVAL_MS = 0x7530,
    FIXED_SUBSTEP_MS = 0x12,
    AMBIENT_INTRO_INTERVAL_MS = 0x1f40
GZ_ENUM_CONST_END(PlayIntervalMs)

GZ_ENUM_BEGIN(PlayViewMode)
    VIEW_MODE_IDLE = 0,
    VIEW_MODE_A = 1,
    VIEW_MODE_B = 2
GZ_ENUM_END(PlayViewMode)

GZ_ENUM_BEGIN(ToolCursorId)
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
GZ_ENUM_END(ToolCursorId)

// @early-stop
#define CLEAR_TAB_HINT(sndHost)                                                                    \
    do {                                                                                           \
        CDDrawSubMgrLeafScan* _s = (sndHost);                                                      \
        if (_s->m_emitGate == 0) {                                                                 \
            void* found = 0;                                                                       \
            _s->m_cues.Lookup("GAME_TABHIGHLIGHT1", found);                                        \
            if (found != 0)                                                                        \
                static_cast<LeafCue*>(found)->PlayIfElapsed(g_sndCueTag, 0, 0, 0);                 \
        }                                                                                          \
    } while (0)

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
        m_viewMode = 0;
        m_hudSuppressed = 1;
        m_cameraBookmarkIndex = -1;
        m_snapshotActive = 0;
        m_scrollEdgeActive = 0;
        m_scrollEdgeLock = 0;
        m_frameMarker = NULL;

        if (!CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId)) {
            return 0;
        }

        CChatBoxOwner* ctl = static_cast<CChatBoxOwner*>(::operator new(0x1c));
        if (ctl) {

            ctl->m_world = NULL;
            ctl->m_fontConfig = NULL;
            ctl->m_attached = false;
            ctl->m_inputActive = 0;
            ctl->m_originX = 0;
            ctl->m_originY = 0;
            ctl->m_mode = 1;
        } else {
            ctl = NULL;
        }
        m_hitTest = ctl;
        if (m_hitTest->Attach(m_world, m_mgr->m_chatLog) == 0) {
            if (m_hitTest) {
                m_hitTest->Deactivate();
                ::operator delete(m_hitTest);
            }
            m_hitTest = NULL;
            return 0;
        }
        m_hitTest->m_inputActive = 0;
        m_hitTest->Configure(1);

        m_guts = new CStatusBarMgr;
        if (m_guts->LoadBattlezItemConfig(m_world) == 0) {
            CStatusBarMgr* w2 = m_guts;
            if (w2 == NULL) {
                return 0;
            }

            delete w2;
            m_guts = NULL;
            return 0;
        }

        CTileTriggerContainer* r78 = static_cast<CTileTriggerContainer*>(::operator new(0x78));
        if (r78) {

            new (&r78->m_base) CPtrList(0xa);
            new (&r78->m_list1) CPtrList(0xa);
            new (&r78->m_list2) CPtrList(0xa);
            new (&r78->m_list3) CPtrList(0xa);
            r78->m_built = 0;
        } else {
            r78 = NULL;
        }
        m_beginMarker = r78;
        if (m_beginMarker->GetFlag74() == 0) {

            delete m_beginMarker;
            m_beginMarker = NULL;
            return 0;
        }

        CTimer* r50 = static_cast<CTimer*>(::operator new(0x50));
        if (r50) {
            r50->Init();
        } else {
            r50 = NULL;
        }
        m_frameMarker = r50;
        if (r50 == NULL) {
            return 0;
        }

        if (ShowCursor(0) >= 0) {
            while (ShowCursor(0) >= 0) {
            }
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
            peer->m_stateFlags |= 1;
        }
        return 1;
    }
}

RVA(0x000c8b80, 0x11b)
i32 CPlay::LeaveState(GameStateId arg) {
    m_mgr->m_cueSink->PauseAllVoices();
    m_savedClock = static_cast<i32>(g_frameTime);
    if (m_notifyLatch) {
        QuitToMenu();
    }
    if (arg == GAMESTATE_HELP) {
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
    ShowHudMessage(m_world, &s, &r, 0x78, 1, 0xff, 0xff, 0, 1);
    RetireScene(0x50, 0x3e8, 0, 1);
    if (m_mgr && m_mgr->m_cmdGrid) {
        m_mgr->m_cmdGrid->ClearGridRange(5);
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

        g_killCueClock = g_lastNow;
        g_engineFrameDelta = g_frameDelta;

        m_world->m_childGroup->TickKillCues(0);
        m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
        m_world->m_workerList->PruneWorkers(
            m_world->m_drawTarget->m_backPair,
            m_world->m_drawTarget->m_overlayPair
        );
        m_mgr->m_inputState->Retune(
            m_world->m_level->m_mainPlane->m_snappedX,
            m_world->m_level->m_mainPlane->m_snappedY
        );
        SoundStream* stream = m_world->m_soundStream;
        if (stream != NULL) {
            u32 t = timeGetTime();
            stream->PurgeVoiceList(t);
            stream->TickSubManagers(t);
        }
        m_beginMarker->FilterList2(g_frameDelta);
        m_guts->LoadMainStatusBarSprite();

        {
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_cueTimer64.m_v
                >= m_cueInterval64.m_v) {
                m_cueToggle = (m_cueToggle == 0);
                m_cueInterval = CUE_INTERVAL_MS;
                m_cueIntervalHi = 0;
                m_cueTimerLo = static_cast<i32>(g_frameTime);
                m_cueTimerHi = 0;
            }
            if (m_cueToggle != 0) {
                PlayCueAt(0x8128, 0x78, 0, 0xff, 0xff, 0, 1, 0);
            }
        }

        CDDrawSurfacePair* back =
            static_cast<CDDrawSurfacePair*>(m_world->m_drawTarget->m_backPair);
        if (back == NULL) {
            return 0;
        }

        m_frameMarker->Tick(static_cast<i32>(g_frameDelta));
        m_frameMarker->Draw(back, 1);
        StepGridWalk(static_cast<i32>(g_frameDelta));
        DrawCursorSaveUnder(back);
        m_world->m_drawTarget->m_frontPair->m_surface->Flip(0);
        UpdateMgrScroll(g_gameReg, m_guts, m_region0Gate);
        m_world->m_level->DeactivateDistantObjectsOnMainPlane();
        return 1;
    }

    if (m_mgr->m_frameGate == 0
        && (g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER || m_overlayDrag == 0)) {
        m_frameMarker->Tick(static_cast<i32>(g_frameDelta));
        m_mgr->m_cmdSubMgr->ScanTargets(0);

        if (m_levelId == IDX(CURSOR_FLAILINGGRUNT)) {
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_bootyTimer64.m_v
                >= m_bootyInterval64.m_v) {
                g_gameReg->m_cueSink->SpawnVoiceDriver(0, 0x33e, -1, 1, -1, -1);
                m_bootyInterval = BOOTY_INTERVAL_MS;
                m_bootyIntervalHi = 0;
                m_bootyTimerLo = static_cast<i32>(g_frameTime);
                m_bootyTimerHi = 0;
            }
        }

        StepInputA();
        StepViewportResize();

        if (m_ambientInitDone == 0) {
            if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_ambientTimer64.m_v
                >= m_ambientInterval64.m_v) {
                i32 id = GetAmbientId();
                char buf[0x40];
                wsprintfA(buf, "AMBIENT%d", id);
                if (g_gameReg->m_musicEnabled != 0) {
                    m_mgr->m_sound->PlayByName(buf, 1);
                } else {
                    CGruntzSoundZ* snd = m_mgr->m_sound;
                    CGruntzSoundInnerZ* found = snd->FindBank(buf);
                    if (found != NULL) {
                        snd->m_pCurrent = found;
                    }
                    if (m_mgr->m_sound->m_pCurrent != NULL) {
                        m_mgr->m_sound->m_pCurrent->SetLoop(1);
                    }
                }
                m_ambientInitDone = 1;
            }
        }

        if (m_region0Gate != 0) {
            m_world->m_drawTarget->m_frontPair->m_surface->Fill(0);
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

        m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
        m_world->m_workerList->PruneWorkers(
            m_world->m_drawTarget->m_backPair,
            m_world->m_drawTarget->m_overlayPair
        );
        if (m_region1Gate != 0) {
            StepViewportResize();
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
        m_beginMarker->FilterList2(g_frameDelta);
        m_guts->LoadDestructButtonSprite(static_cast<i32>(g_frameDelta));
        m_mgr->m_tileGrid->UpdateDiagonals(m_mgr);

        if (m_lightFx != NULL && m_guts->m_position != STATUSBAR_HIDDEN
            && m_guts->m_activeTab != TAB_GAME) {
            RECT rc;
            if (m_guts->m_position == STATUSBAR_DOCK_LEFT) {
                SetRect(&rc, 20, 5, 140, 125);
            } else {
                i32 cx = g_gameReg->m_modeH;
                i32 cy = g_gameReg->m_modeW;
                rc.top = cx;
                SetRect(&rc, cy - 140, 5, cy - 20, 125);
            }
            m_lightFx->Resize(static_cast<i32>(g_frameDelta), 0);
            m_lightFx->ComputeRect(m_world->m_drawTarget->m_backPair, &rc);
        }

        m_mgr->m_inputState->Retune(
            m_world->m_level->m_mainPlane->m_snappedX,
            m_world->m_level->m_mainPlane->m_snappedY
        );
        if (m_world->m_drawTarget->m_backPair == NULL) {
            return 1;
        }

        if (m_snapshotActive != 0) {
            u32 now = g_frameTime;
            u32 dur = static_cast<u32>((m_snapDur + m_snapBaseLo)) - now;
            if (static_cast<i32>(dur) >= 0) {

                if (m_guts->m_modeArmed != 0) {
                    g_gameReg->m_cmdGrid->ClearRowAndRefresh(5);
                } else {
                    g_gameReg->m_cmdGrid->ClearRowAndRefresh(g_curPlayer);
                }

                m_frameMarker->Draw(
                    static_cast<CDDrawSurfacePair*>(m_world->m_drawTarget->m_backPair),
                    0
                );
                m_guts->SetMode(0);
                m_snapshotActive = 0;

                if (g_gameReg->m_options[0].m_warlordObjectId != 0) {
                    void* out = 0;
                    if (MapLookupById(
                            g_gameReg->m_world->m_childGroup->m_map48,
                            g_gameReg->m_options[0].m_warlordObjectId,
                            out
                        )) {
                        CGameObject* object = static_cast<CGameObject*>(out);
                        if (object != NULL && object->m_animWorker->m_logic != NULL) {
                            static_cast<CWarlord*>(object->m_animWorker->m_logic)
                                ->ResolveDeathAnimation();
                        }
                    }
                }
            } else {

                CString tmp;
                tmp.Format("%d", 0);
                RECT lvl = g_gameReg->m_world->m_level->m_planeCtx;
                RECT box;
                CopyRect(&box, &lvl);
                ShowHudMessageAlt(g_gameReg->m_world, &tmp, &box, 0x82, 1, 0xff, 0xff, 0, 1);
            }
        }

        CDDrawSurfacePair* view =
            static_cast<CDDrawSurfacePair*>(m_world->m_drawTarget->m_backPair);
        m_frameMarker->Draw(view, 0);
        m_hitTest->LoadChatBoxSprite(view);
        DrawDebugStats();
        m_mgr->m_cmdGrid->OverlayRelease();

        if (m_winLoseBanner != 0 && m_guts->m_toggleActive == 0 && m_guts->m_toggleHandle == 0) {

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

        m_beginMarker->FilterList2(g_frameDelta);
        DrawCursorSaveUnder(0);
        m_world->m_drawTarget->m_frontPair->m_surface->Flip(0);

        if (m_region0Gate != 0) {
            u32 e = g_frameTime - static_cast<u32>(m_region0TimerLo);
            if (e >= static_cast<u32>(m_region0Interval)) {
                SetTinyViewportCurse(static_cast<i32>(g_frameTime));
            }
        }
        if (m_region1Gate != 0) {
            u32 e = g_frameTime - static_cast<u32>(m_region1TimerLo);
            if (e >= static_cast<u32>(m_region1Interval)) {
                SetDarknessCurse(static_cast<i32>(g_frameTime));
            }
        }
        if (m_region2Gate != 0) {
            u32 e = g_frameTime - static_cast<u32>(m_region2TimerLo);
            if (e >= static_cast<u32>(m_region2Interval)) {
                SetMonitorCurse(static_cast<i32>(g_frameTime));
            }
        }
        if (m_region3Gate != 0) {
            u32 e = g_frameTime - static_cast<u32>(m_region3TimerLo);
            if (e >= static_cast<u32>(m_region3Interval)) {
                SetRandomMoveIconsCurse(static_cast<i32>(g_frameTime));
            }
        }
        return 1;
    }

    StepInputA();
    if (m_world->m_drawTarget->m_backPair == NULL) {
        return 1;
    }
    {
        SoundStream* stream = m_world->m_soundStream;
        if (stream != NULL) {
            u32 t = timeGetTime();
            stream->PurgeVoiceList(t);
            stream->TickSubManagers(t);
        }
        if (m_paused != 0) {

            m_world->m_level->VisitVisible(
                m_world->m_drawTarget->m_backPair,
                m_world->m_childGroup
            );
            m_world->m_workerList->PruneWorkers(
                m_world->m_drawTarget->m_backPair,
                m_world->m_drawTarget->m_overlayPair
            );
            m_guts->LoadDestructButtonSprite(static_cast<i32>(g_frameDelta));
            if (m_guts->m_toggleActive == 0 && m_guts->m_toggleHandle == 0) {
                PlayCueAt(0x812c, 0x78, 0, 0xff, 0xff, 0, 1, 0);
            }
            m_frameMarker->Draw(
                static_cast<CDDrawSurfacePair*>(m_world->m_drawTarget->m_backPair),
                1
            );
        } else {

            if (m_stepCountdown > 0) {
                m_stepCountdown = m_stepCountdown - 1;
                m_world->m_level->VisitVisible(
                    m_world->m_drawTarget->m_backPair,
                    m_world->m_childGroup
                );
                m_world->m_workerList->PruneWorkers(
                    m_world->m_drawTarget->m_backPair,
                    m_world->m_drawTarget->m_overlayPair
                );
                m_guts->LoadDestructButtonSprite(static_cast<i32>(g_frameDelta));
                m_world->m_drawTarget->m_backPair->m_surface->ShadeRect(0x32, 0);
                PlayCueAt(m_lastCueId, 0x78, 0, 0xff, 0xff, 0, 1, 0);
                m_frameMarker->Draw(
                    static_cast<CDDrawSurfacePair*>(m_world->m_drawTarget->m_backPair),
                    1
                );
            }
            if (m_ambientInitDone == 0) {
                if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_ambientTimer64.m_v
                    >= m_ambientInterval64.m_v) {
                    i32 id = GetAmbientId();
                    char buf[0x40];
                    wsprintfA(buf, "AMBIENT%d", id);
                    if (g_gameReg->m_musicEnabled != 0) {
                        m_mgr->m_sound->PlayByName(buf, 1);
                    } else {
                        CGruntzSoundZ* snd = m_mgr->m_sound;
                        CGruntzSoundInnerZ* found = snd->FindBank(buf);
                        if (found != NULL) {
                            snd->m_pCurrent = found;
                        }
                        if (m_mgr->m_sound->m_pCurrent != NULL) {
                            m_mgr->m_sound->m_pCurrent->SetLoop(1);
                        }
                    }
                    m_ambientInitDone = 1;
                }
            }
        }
        m_beginMarker->FilterList2(g_frameDelta);
        DrawCursorSaveUnder(0);
        m_world->m_drawTarget->m_frontPair->m_surface->Flip(0);
    }
    return 1;
}

DATA(0x002bf3bc)
u32 g_engineFrameDelta = 0;
DATA(0x002bf3c0)
u32 g_killCueClock = 0;

RVA(0x000ca200, 0xe54)
i32 CPlay::LoadByMode(i32 level, i32) {
    CPlay* self = this;
    CGruntzMgr* gameReg;
    void* set;
    i32 reload = 0;

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

    CTimer* worker = self->m_frameMarker;
    if (worker != NULL) {
        worker->m_unusedStamp.m_v = 0;
        worker->m_accum.m_v = 0;
        worker->m_running = 0;
        worker->m_currentMs = 0;
    }

    SoundStream* grid = self->m_world->m_soundRegistry->m_soundStream;
    if (grid != NULL) {
        grid->Stop();
    }
    self->m_mgr->m_sound->StopAndFlush();
    self->m_mgr->m_inputState->Teardown();
    self->m_mgr->m_cueSink->PauseAllVoices();
    self->m_mgr->m_cueSink->ClearSprites();
    self->m_mgr->RestoreVideoMode(0);

    gameReg = g_gameReg;
    if (gameReg->m_gameMode != GAMEMODE_MULTIPLAYER) {
        g_curPlayer = 0;
        if (gameReg->m_frameGate != 0) {
            i32 v = gameReg->m_frameGate ^ 1;
            gameReg->m_frameGate = v;
            gameReg->FinishLevel(v, 1);
        }
    }

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

    for (i32 t = 0; t < 4; ++t) {
        CGruntzMgr* hostBase = self->m_mgr;
        gameReg = g_gameReg;
        GruntzPlayer* team = &hostBase->m_options[t];
        if (gameReg->m_gameMode == GAMEMODE_SINGLE) {
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

    i32 modeFlag = (Update() == GAMESTATE_MULTI) ? 1 : 0;
    CMulti* savedThis = modeFlag ? static_cast<CMulti*>(self) : 0;
    self->m_initialFramePending = 1;
    self->m_levelIndex = level;
    {
        i32 r = (level - 1) % 0x24;
        self->m_levelType = r / 4 + 1;
    }

    gameReg = g_gameReg;
    g_frameTime = 0;
    if (gameReg->m_gameMode == GAMEMODE_REPLAY) {
        srand(timeGetTime());
    }
    g_resourceInstallActive = 0;
    Cmd_ResetScroll();
    gameReg->m_scoreHud->Init();
    gameReg->m_cmdSubMgr->m_pendingCommands.RemoveAll();
    gameReg->m_cmdSubMgr->DrainBase();
    g_frameTicks = 0;
    self->m_returnToMenuOnComplete = 0;
    self->m_mgr->m_isCustomLevel = 0;

    CGruntzMgr* host = self->m_mgr;
    if (host->m_strWorldFile.GetLength() != 0) {
        if (host->m_isBattlezLevel != 0) {

            set = host->m_symParser->ResolvePath("GAME_BATTLEZ");
            if (set == NULL) {
                goto fail0;
            }
            CParseSource* ins = (static_cast<CSymTab*>(set))
                                    ->Insert(
                                        static_cast<const char*>(self->m_mgr->GetWorldFileName()),
                                        REZ_TAG_NONE
                                    );
            if (ins == NULL) {
                return 0;
            }
            void* desc = (static_cast<CParseSource*>(set))->BeginParse();
            if (desc == NULL) {
                goto fail0;
            }
            char* p = static_cast<char*>(desc) + 0x10;
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
        } else if (host->m_isMultiLevel != 0) {

            set = host->m_symParser->ResolvePath("GAME_MULTI");
            if (set == NULL) {
                goto fail0;
            }
            CParseSource* ins = (static_cast<CSymTab*>(set))
                                    ->Insert(
                                        static_cast<const char*>(self->m_mgr->GetWorldFileName()),
                                        REZ_TAG_NONE
                                    );
            if (ins == NULL) {
                return 0;
            }
            void* desc = (static_cast<CParseSource*>(set))->BeginParse();
            if (desc == NULL) {
                goto fail0;
            }
            char* p = static_cast<char*>(desc) + 0x10;
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

            level = WwdFile::ValidateMainBlock(self->m_mgr->GetWorldFileName());
            self->m_returnToMenuOnComplete = 0;
            self->m_mgr->m_isCustomLevel = 0;
        }

        i32 r = (level - 1) % 0x24;
        self->m_levelIndex = level;
        self->m_levelType = r / 4 + 1;
    }

    sprintf(nameBuf, "AREA%i", self->m_levelType);
    set = self->m_symParser->ResolvePath(nameBuf);
    self->m_levelBank = static_cast<CSymTab*>(set);
    if (set == NULL) {
        goto fail0;
    }

    {
        i32 page = self->m_levelType - 1;
        switch (static_cast<u32>(page)) {
            case 0:
                g_areaPitDeath = DEATH_SINK;
                break;
            case 1:
                g_areaHazardDeath = DEATH_DROP;
                g_areaPitDeath = DEATH_DROP;
                break;
            case 2:
                g_areaHazardDeath = DEATH_DROP;
                g_areaPitDeath = DEATH_FALL;
                break;
            case 3:
                g_areaPitDeath = DEATH_FALL;
                g_areaHazardDeath = DEATH_QUICKFALL;
                break;
            case 4:
                g_areaPitDeath = DEATH_MELT;
                g_areaHazardDeath = DEATH_ELECTROCUTE;
                break;
            case 5:
                g_areaPitDeath = DEATH_SINK;
                g_areaHazardDeath = DEATH_EXPLODE;
                break;
            case 6:
                g_areaPitDeath = DEATH_FALL2;
                g_areaHazardDeath = DEATH_EXPLODE;
                break;
            case 7:
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
        CSymTab* prevTiles = self->m_stateBank;
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
        static_cast<void>(prevTiles);
    }

    if (!FadeInTitle(nameBuf, 0, 0, 0, 0, 1)) {
        goto fail0;
    }
    RetireScene(0x50, 0x3e8, 0, 1);
    DrawLevelInfoText();
    self->m_stateBank = NULL;
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
    if (savedThis != NULL) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();

    if (!BuildAnizKeyTable(0)) {
        goto fail0;
    }

    {
        i32 cached = g_lastLevelNum;
        i32 eq = g_pAreaMgr->IsSameWorld(cached);
        reload = (eq == 0) ? 1 : 0;
        i32 diff = (level != g_lastLevelNum) ? 1 : 0;
        if (g_pAreaMgr == NULL) {
            return 0;
        }
        g_lastLevelNum = level;

        BuildHelpReveal(0);
        if (savedThis != NULL) {
            (savedThis)->AckJoinFailure();
        }
        RegisterInputBindings();

        BuildHelpReveal(0);
        if (savedThis != NULL) {
            (savedThis)->AckJoinFailure();
        }
        RegisterInputBindings();

        if (!LoadActionTileSprites(diff)) {
            goto fail0;
        }
    }

    BuildHelpReveal(0);
    if (savedThis != NULL) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (modeFlag != 0 && (g_gameReg)->m_gameMode == GAMEMODE_SINGLE) {
        BuildWarlordNameTable(savedThis);
    }
    BuildHelpReveal(0);
    RegisterInputBindings();
    if (!LoadLevelImages(1)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != NULL) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadGameImages(1)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != NULL) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!BuildSpriteImageKeyTable(savedThis)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    RegisterInputBindings();
    if (!LoadLevelSounds(1)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != NULL) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadGameSounds(1)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != NULL) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadGruntSoundNamespaces(0)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != NULL) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    SetEffectSpriteDurations();
    if (savedThis != NULL) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadLevelAnims(1)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != NULL) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!LoadGameAnims(1)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != NULL) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    if (!BuildWorldLevelPath(1)) {
        goto fail0;
    }
    BuildHelpReveal(0);
    if (savedThis != NULL) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();

    self->m_mgr->RecomputeViewScale();
    if (self->m_world->m_level->m_mainPlane != NULL) {
        (static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane))->GetSize();
    }
    if (self->m_world->m_level->m_mainPlane != NULL) {
        (static_cast<CDDrawWorkerHost*>(self->m_world->m_level->m_mainPlane))->GetSize();
    }
    BuildHelpReveal(0);
    if (savedThis != NULL) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();

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
        CLightFxRender* ctx = static_cast<CLightFxRender*>(::operator new(0x43c));
        if (ctx != NULL) {
            ctx->m_mgr = NULL;
            ctx->m_cmdGrid = NULL;
            ctx->m_tileGrid = NULL;
            ctx->m_world = NULL;
            ctx->m_surface = NULL;
            ctx->m_handle = 0;
            ctx->m_refreshInterval = 0;
            ctx->m_refreshRemaining = 0;
        } else {
            ctx = NULL;
        }
        self->m_lightFx = ctx;
        if (!ctx->Init(self->m_mgr, 0xfa)) {
            goto fail0;
        }
    }
    if (!self->m_lightFx->BuildShape(self->m_levelType)) {
        goto fail0;
    }

    gameReg = g_gameReg;
    if (gameReg->m_gameMode != GAMEMODE_SINGLE) {
        CString warp;
        i32 same = 0;
        if (warp.LoadString(0x81ab)) {
            char* a = const_cast<char*>(static_cast<const char*>(gameReg->GetWorldFileName()));
            char* b = const_cast<char*>(static_cast<const char*>(warp));
            i32 eq = 1;
            while (*a == *b) {
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
            ScanShuffleQuads();
        }
    }

    if (self->m_mgr->m_gameMode == GAMEMODE_REPLAY) {
        self->m_mgr->SyncOptionsState();
    }
    self->m_mgr->m_saveSink->InitializeLevelSlot(&self->m_saveSlot, self->m_levelIndex, 0);
    {
        CString key;
        gameReg = g_gameReg;
        gameReg->m_cmdGrid->m_pendingFx = NULL;
        i32 count = self->m_levelIndex;
        i32 i = count - ((count - 1) % 4);
        for (; i < count; ++i) {

            key.Format("Level%i", i);
            CTriggerMgr* bm = gameReg->m_cmdGrid;
            i32 v = g_buteMgr.GetInt(static_cast<const char*>(key), "WarpStone");
            bm->m_byteArr.SetAtGrow(bm->m_byteArr.GetSize(), static_cast<u8>(v));
        }
    }
    self->m_guts->LoadMultiplayerBattlezConfig(self->m_levelIndex);

    set = self->m_world->m_childGroup->CreateSprite(0, 0, 0, 0x13880, "CursorSnapSprite", 0x40001);
    self->m_scrollSink = static_cast<CWwdGameObjectA*>(set);
    if (set != NULL) {
        self->m_world->m_childGroup->TickKillCues(0);
        if (savedThis == NULL) {

            CStatusBarMgr* tiles = self->m_guts;
            i32 id = (tiles->m_position == STATUSBAR_DOCK_RIGHT) ? 0x1a9 : 0x249;
            if (!self->m_frameMarker->LoadTimerSprite(id, 0x1ca)) {
                CTimer* spr = self->m_frameMarker;
                if (spr != NULL) {
                    spr->Reset();
                    ::operator delete(spr);
                    self->m_frameMarker = NULL;
                }
            }
        } else {

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
                        ->GetSize();
                }
                BuildHelpReveal(0);
                if (savedThis != NULL) {
                    (savedThis)->AckJoinFailure();
                }
                RegisterInputBindings();
                if (BuildMusicCategoryTable(reload)) {
                    goto okContinue;
                }
            }
            goto fail1;
        }
    }

okContinue:
    BuildHelpReveal(0);
    if (savedThis != NULL) {
        (savedThis)->AckJoinFailure();
    }
    RegisterInputBindings();
    BuildHelpReveal(1);
    ActiveWait(0x64);
    if (savedThis != NULL) {
        (savedThis)->AckJoinFailure();
    }

    gameReg = g_gameReg;
    if (gameReg->m_loadingSaveGame == 0) {
        CDDSurface* mapHost = self->m_world->m_drawTarget->m_frontPair->m_surface;
        mapHost->ShadeRect(0x32, 0);
        gameReg = g_gameReg;
    }

    if (gameReg->m_gameMode != GAMEMODE_MULTIPLAYER && gameReg->m_loadingSaveGame == 0) {
        CString scr;
        self->m_inGame = 1;
        self->m_hudSuppressed = 0;
        RECT rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = 0x280;
        rect.bottom = 0x1e0;
        if (scr.LoadString(0x8128)) {
            EngStr_DrawText(self->m_world, &scr, &rect, 0x78, 1, 0xff, 0xff, 0, 1);
        }
    } else {
        self->m_hudSuppressed = 1;
    }

    self->m_scrollEdgeLock = 0;
    self->m_overlayDrag = 0;
    self->m_paused = 0;
    self->m_playerCommandPending = 0;
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

fail1:
    return 0;
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
    g_gameReg->m_isBattlezLevel = 0;
    if (g_gameReg->m_gameMode == GAMEMODE_REPLAY) {
        g_gameReg->m_gameMode = GAMEMODE_NONE;
    }
    g_gameReg->m_tileGrid->Reset();
}

RVA(0x000cb740, 0x8f)
void CPlay::ModeCleanup() {
    if (m_world) {
        {

            CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
            if (reg->m_soundStream) {
                reg->m_soundStream->Stop();
            }
        }
        m_world->m_soundRegistry->ClearMap();
    }
    if (m_mgr) {
        m_mgr->m_sound->StopAndFlush();

        m_mgr->m_inputState->Teardown();
    }
    if (m_world) {
        m_world->m_imageRegistry->MapTeardown();
    }
    if (m_world) {
        m_world->m_animRegistry->FreeAll();
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

    void* h = m_levelBank->ResolvePath("TILEZ");
    if (!h) {
        return 0;
    }
    if (m_world->m_imageRegistry->LoadNamespace(h, g_emptyString, "_") == -1) {
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
        m_world->m_workerList->PruneWorkers(
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

RVA(0x000cbaf0, 0x16f)
i32 CPlay::OnChar(i32 key, i32 flag) {
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

        if (ResetPlayState() == 0) {
            m_mgr->ReportError(IDX(CMD_TOGGLE_MUSIC), 0x456);
        }
        return 1;
    }
    if (m_paused != 0) {
        m_paused = 0;
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, 0x816e, 0);
        return 1;
    }

    if (m_mgr->m_frameGate == 0) {
        if (m_hitTest->m_inputActive != 0) {
            m_mgr->m_chatLog->TypeChar(key, flag);
            return 1;
        }
        if (key == ']') {
            m_guts->DockStatusBarRight();
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

            if (m_guts->m_position == STATUSBAR_DOCK_LEFT) {
                m_hitTest->Configure(2);
            } else {
                m_hitTest->Configure(1);
            }
            return 1;
        }
    }
    return 0;
}

RVA(0x000cbcc0, 0x1770)
i32 CPlay::OnKeyDown(i32 vk, i32 lparam) {
    CPlay* self = this;

    if (self->m_hudSuppressed != 0) {
        return 1;
    }
    if (self->m_renderDisabled != 0) {
        return 1;
    }
    if (self->m_inGame != 0) {
        return 1;
    }
    if (self->m_paused != 0) {
        return 1;
    }
    if (self->m_mgr->m_frameGate != 0) {
        return 1;
    }

    CGruntzMgr* host = self->m_mgr;
    CStatusBarMgr* level = self->m_guts;
    i32 key = vk;

    if (level->m_toggleActive != 0 || level->m_toggleHandle != 0) {
        if (level->m_toggleHandle != 0) {

            if (key == 'Y' || key == VK_RETURN) {
                if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    if (g_gameReg->m_cmdGrid->m_phase == 1) {
                        g_gameReg->UpdateScoreHud();
                    }
                    PostMessageA(host->m_gameWnd->m_hwnd, WM_COMMAND, 0x8023, 0);
                    return 1;
                }
                CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                (static_cast<CGruntzMgr*>((host)))->AccrueScoreTime();
                return 1;
            }
            if (key == 'N' || key == VK_ESCAPE) {
                CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                this->ReleaseLevelOverlay(0);
                return 1;
            }

        } else {

            if (key == 'Q') {
                if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    if (g_gameReg->m_cmdGrid->m_phase == 1) {
                        g_gameReg->UpdateScoreHud();
                    }
                    PostMessageA(host->m_gameWnd->m_hwnd, WM_COMMAND, 0x8023, 0);
                }
                return 1;
            }

            if (key == 'S' && g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                (static_cast<CGruntzMgr*>((host)))->AccrueScoreTime();
            }
            if (key == 'R') {
                if (host->m_gameMode == GAMEMODE_SINGLE && g_gameReg->m_cmdGrid->m_phase != 1) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    CGameWnd* r = g_gameReg->m_gameWnd;
                    PostMessageA(r->m_hwnd, WM_COMMAND, 0x806b, 0);
                }
                return 1;
            }
            if (key == 'N') {
                if (host->m_gameMode == GAMEMODE_SINGLE && g_gameReg->m_cmdGrid->m_phase == 1) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    (static_cast<CGruntzMgr*>((host)))->AccrueScoreTime();
                }
                return 1;
            }
            if (key == 'O') {
                if (host->m_gameMode != GAMEMODE_SINGLE
                    && self->m_guts->m_observerTabAvailable != 0) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    this->ReleaseLevelOverlay(0);
                }
                return 1;
            }
        }
    }

    if (key == VK_RETURN) {
        CChatBoxOwner* rec = self->m_hitTest;
        if (rec->m_inputActive != 0) {
            rec->ProcessCheatInput(0xd, lparam);
        } else {
            rec->m_fontConfig->EndInput();
            rec->m_inputActive = 1;
            self->m_hitTest->ProcessCheatInput(0xd, lparam);
        }
        return 1;
    }

    if (key == VK_ESCAPE) {
        CTriggerMgr* h68 = host->m_cmdGrid;
        CWwdGameObjectA* n = h68->m_goal;
        if (n != NULL) {
            n->m_flags |= 0x10000;
            h68->m_goal = NULL;
        }
        h68->m_armed = 0;
        CChatBoxOwner* rec = self->m_hitTest;
        if (rec->m_inputActive != 0) {
            this->FlushPendingOps();
            self->m_hitTest->m_fontConfig->EndInput();
            self->m_hitTest->m_inputActive = 0;
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
        this->EnterOverlayDrag(1);
        return 1;
    }

    if (self->m_hitTest->m_inputActive != 0) {
        return 1;
    }
    if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
        return 1;
    }

    StateMgrBZ* dev = g_spawnConfig;

    if (key == VK_TAB) {
        i32 idx = self->m_focusPlayerIndex;
        i32 pick;
        GruntzPlayer* area;
        if (dev->m_edgeKeys & 1) {
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
            self->m_focusPlayerIndex = pick;
            this->ResetGoals(area->m_focusX, area->m_focusY);
        }
    }

    if (key == 'H') {
        GruntzPlayer* a = &g_gameReg->m_options[g_curPlayer];
        if (a == NULL) {
            return 1;
        }
        this->ResetGoals(a->m_focusX, a->m_focusY);
        return 1;
    }

    if (key == 'Q') {
        if ((dev->m_edgeKeys & 0x20) == 0) {
            return 1;
        }
        CGruntzMgr* h = self->m_mgr;
        if (h->m_frameGate != 0) {
            h->m_frameGate ^= 1;
            self->m_mgr->FinishLevel(h->m_frameGate, 1);
        }
        CDDrawSubMgrLeafScan* s = self->m_mgr->m_world->m_soundRegistry;
        if (s->m_emitGate == 0) {
            void* found = 0;
            s->m_cues.Lookup("GAME_TABHIGHLIGHT1", found);
            if (found != NULL) {
                static_cast<LeafCue*>(found)->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
            }
        }
        return 1;
    }

    if (key == 'Z') {
        g_gameReg->m_cmdGrid->EnqueueGroupCells();
        return 1;
    }

    if (key == 'C') {
        g_gameReg->m_cmdGrid->CenterOnGroup(dev->m_edgeKeys & 0x20);
        return 1;
    }

    if (key == 'T') {
        this->FlushPendingOps();
        g_gameReg->m_cmdGrid->ToggleRegionA();
        return 1;
    }

    if (key == 'Y') {
        this->FlushPendingOps();
        g_gameReg->m_cmdGrid->ToggleRegionB();
        return 1;
    }

    if (key == VK_SPACE) {
        if (dev->m_edgeKeys & 0x20) {
            CDDrawWorkerHost* obj = self->m_world->m_level->m_mainPlane;
            i32 v0 = obj->m_snappedX;
            i32 v1 = obj->m_snappedY;
            Coord* slot;
            if (self->CameraBookmarkCount() < 4) {
                CoordPoolNode* head = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
                CoordPoolNode* nx = head->m_next;
                if (nx != NULL) {
                    slot = &head->m_coord;
                    g_coordPool.m_freeHead = nx;
                } else {
                    slot = NULL;
                }
            } else {

                slot = static_cast<Coord*>(self->m_cameraBookmarks.GetAt(0));
                self->m_cameraBookmarks.RemoveAt(0, 1);
                i32 c = self->m_cameraBookmarkIndex - 1;
                self->m_cameraBookmarkIndex = c;
                if (c < 0) {
                    self->m_cameraBookmarkIndex = self->CameraBookmarkCount() - 1;
                }
            }
            slot->m_x = v0;
            slot->m_y = v1;
            if (self->m_cameraBookmarkIndex != self->CameraBookmarkCount() - 1) {
                self->m_cameraBookmarks.InsertAt(self->m_cameraBookmarkIndex + 1, slot, 1);
                self->m_cameraBookmarkIndex = self->m_cameraBookmarkIndex + 1;
                return 1;
            }
            self->m_cameraBookmarks.SetAtGrow(self->CameraBookmarkCount(), slot);
            self->m_cameraBookmarkIndex = self->m_cameraBookmarkIndex + 1;
            return 1;
        }
        if (self->CameraBookmarkCount() == 0) {
            return 1;
        }
        if (dev->m_edgeKeys & 1) {
            i32 c = self->m_cameraBookmarkIndex - 1;
            self->m_cameraBookmarkIndex = c;
            if (c < 0) {
                self->m_cameraBookmarkIndex = self->CameraBookmarkCount() - 1;
            }
        } else {
            i32 c = self->m_cameraBookmarkIndex + 1;
            self->m_cameraBookmarkIndex = c;
            if (c >= self->CameraBookmarkCount()) {
                self->m_cameraBookmarkIndex = 0;
            }
        }
        i32* e = static_cast<i32*>(self->m_cameraBookmarks.GetAt(self->m_cameraBookmarkIndex));
        this->ResetGoals(e[0], e[1]);
        return 1;
    }

    if (key == VK_BACK) {
        if (self->CameraBookmarkCount() <= 0) {
            return 1;
        }
        i32 cur = self->m_cameraBookmarkIndex;
        if (cur < 0) {
            return 1;
        }
        CoordPoolNode* node = g_coordPool.NodeOf(self->m_cameraBookmarks.GetAt(cur));
        self->m_cameraBookmarks.RemoveAt(cur, 1);
        node->m_next = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
        g_coordPool.m_freeHead = node;
        i32 c = self->m_cameraBookmarkIndex - 1;
        self->m_cameraBookmarkIndex = c;
        if (c != -1) {
            return 1;
        }
        if (self->CameraBookmarkCount() == 0) {
            return 1;
        }
        self->m_cameraBookmarkIndex = self->CameraBookmarkCount() - 1;
        return 1;
    }

    if (key == 'M' && (dev->m_edgeKeys & 0x20)) {
        g_gameReg->SetSoundLevelState(g_gameReg->m_musicEnabled == 0);
        return 1;
    }

    if (key == 'V' && (dev->m_edgeKeys & 0x20)) {
        g_gameReg->m_isVoiceEnabled = (g_gameReg->m_isVoiceEnabled == 0);
        return 1;
    }

    if (key == 'A') {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        CStatusBarMgr* lv = self->m_guts;
        if (lv->m_hlBusy != 0) {
            return 1;
        }
        if (lv->m_position == STATUSBAR_HIDDEN) {
            lv->RefreshState();
        }
        if (lv->m_activeTab != TAB_GRUNTZ) {
            lv->SetTabState(TAB_GRUNTZ, MENUITEM_SELECTED);
            lv->Deactivate();
        } else {
            lv->Deactivate();
        }
        return 1;
    }

    if (key == 'S') {
        if (dev->m_edgeKeys & 0x20) {
            g_gameReg->SetRunState(g_gameReg->m_soundEnabled == 0);
            return 1;
        }
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        CStatusBarMgr* lv = self->m_guts;
        if (lv->m_hlBusy != 0) {
            return 1;
        }
        if (lv->m_position == STATUSBAR_HIDDEN) {
            lv->RefreshState();
        }
        if (lv->m_activeTab != TAB_RESOURCE) {
            lv->SetTabState(TAB_RESOURCE, MENUITEM_SELECTED);
            lv->Deactivate();
        } else {
            lv->Deactivate();
        }
        return 1;
    }

    if (key == 'D') {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        CStatusBarMgr* lv = self->m_guts;
        if (lv->m_hlBusy != 0) {
            return 1;
        }
        if (lv->m_position == STATUSBAR_HIDDEN) {
            lv->RefreshState();
        }
        if (lv->m_activeTab != TAB_STATZ) {
            lv->SetTabState(TAB_STATZ, MENUITEM_SELECTED);
            lv->Deactivate();
        } else {
            lv->Deactivate();
        }
        return 1;
    }

    if (key == 'F') {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        self->m_guts->AdvanceTab(g_spawnConfig->m_edgeKeys & 1);
        return 1;
    }

    if (key == 'G') {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        CStatusBarMgr* lv = self->m_guts;
        if (lv->m_hlBusy != 0) {
            return 1;
        }
        if (lv->m_position == STATUSBAR_HIDDEN) {
            lv->RefreshState();
        }
        if (lv->m_activeTab != TAB_GAME) {
            lv->SetTabState(TAB_GAME, MENUITEM_SELECTED);
        }
        lv->SetTab(5, 1);
        lv->Deactivate();
        return 1;
    }

    if (lparam & 0x1000000) {
        if (key == VK_LEFT) {
            self->m_scrollEdgeLock |= 1;
            return 1;
        }
        if (key == VK_RIGHT) {
            self->m_scrollEdgeLock |= 4;
            return 1;
        }
        if (key == VK_UP) {
            self->m_scrollEdgeLock |= 2;
            return 1;
        }
        if (key == VK_DOWN) {
            self->m_scrollEdgeLock |= 8;
            return 1;
        }
        if (key == VK_INSERT || key == VK_DELETE || key == VK_HOME || key == VK_END
            || key == VK_PRIOR || key == VK_NEXT) {
            return 1;
        }
    }

    if (key == VK_NUMPAD1 || key == VK_NUMPAD2 || key == VK_NUMPAD3 || key == VK_NUMPAD4
        || key == VK_NUMPAD5 || key == VK_NUMPAD6 || key == VK_NUMPAD7 || key == VK_NUMPAD8
        || key == VK_NUMPAD9 || key == VK_NUMLOCK || key == VK_DIVIDE || key == VK_MULTIPLY
        || key == VK_HOME || key == VK_END || key == VK_PRIOR || key == VK_NEXT || key == VK_CLEAR
        || key == VK_UP || key == VK_DOWN || key == VK_LEFT || key == VK_RIGHT || key == VK_INSERT
        || key == VK_DELETE || key == VK_DECIMAL) {
        goto recorder_place;
    }

    if (key == 'I') {
        if (g_gruntCreation == 0) {
            return 1;
        }
        GruntzPlayer* a = &g_gameReg->m_options[g_curPlayer];
        if (a == NULL) {
            return 1;
        }
        if (g_gameReg->m_cmdGrid->m_rowCount[g_curPlayer] >= a->m_comboSel) {
            return 1;
        }
        CGruntzMgr* h = self->m_mgr;
        i32 my = self->m_cursorY;
        LevelCoordRect* r = &h->m_world->m_level->m_planeCtx;
        i32 x0 = r->left;
        i32 y0 = r->top;
        i32 x1 = r->right;
        i32 y1 = r->bottom;
        i32 mx = self->m_cursorX;
        if (mx >= x1 || mx < x0 || my >= y1 || my < y0) {
            return 1;
        }
        h->m_cmdSubMgr->BlitTileMarker(
            1,
            g_curPlayer,
            static_cast<i16>(self->m_cursorX),
            static_cast<i16>(self->m_cursorY),
            0
        );
        return 1;
    }

    if (key == 'P') {
        if (g_gooPuddlez == 0) {
            return 1;
        }
        if (g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER) {
            return 1;
        }
        CGruntzMgr* h = self->m_mgr;
        i32 mx = self->m_cursorX;
        CGameLevel* q = h->m_world->m_level;
        LevelCoordRect* r = &q->m_planeCtx;
        i32 x0 = r->left;
        i32 y0 = r->top;
        i32 x1 = r->right;
        i32 y1 = r->bottom;
        i32 my = self->m_cursorY;
        if (!(mx >= x1 || mx < x0 || my >= y1 || my < y0)) {
            CDDrawWorkerHost* g = q->m_mainPlane;
            i32 by = g->m_viewRect.top - q->m_planeCtx.top + my;
            i32 bx = g->m_viewRect.left - q->m_planeCtx.left + mx;
            host->m_cmdGrid->SpawnPuddle(bx, by, 0, 0, 1, 0x19);
        }
    }

    if (key == VK_F9) {
        if (g_explosionz == 0) {
            return 1;
        }
        CGruntzMgr* h = self->m_mgr;
        i32 my = self->m_cursorY;
        CGameLevel* q = h->m_world->m_level;
        CDDrawWorkerHost* g = q->m_mainPlane;
        i32 by = ((g->m_viewRect.top - q->m_planeCtx.top + my) & ~0x1f) + 0x10;
        i32 bx = ((self->m_cursorX - q->m_planeCtx.left + g->m_viewRect.left) & ~0x1f) + 0x10;
        g_gameReg->m_cmdGrid->LoadExplosionSprites(bx, by, -1, 1);
        return 1;
    }

    if (key == 'K') {
        if (g_gruntDestruction == 0) {
            return 1;
        }
        i32 outA = 0;
        i32 outB = 0;
        CGrunt* r =
            host->m_cmdGrid->ScreenToCell(self->m_cursorX, self->m_cursorY, &outB, &outA, 5);
        if (r == NULL) {
            return 1;
        }
        host->m_cmdGrid->CellDispatch(outB, outA, DEATH_DROP, -1);
        return 1;
    }

    if (key == '1') {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(1);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(1);
        }
        return 1;
    }
    if (key == '2') {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(2);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(2);
        }
        return 1;
    }
    if (key == '3') {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(3);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(3);
        }
        return 1;
    }
    if (key == '4') {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(4);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(4);
        }
        return 1;
    }
    if (key == '5') {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(5);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(5);
        }
        return 1;
    }
    if (key == '6') {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(6);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(6);
        }
        return 1;
    }
    if (key == '7') {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(7);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(7);
        }
        return 1;
    }
    if (key == '8') {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(8);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(8);
        }
        return 1;
    }
    if (key == '9') {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(9);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(9);
        }
        return 1;
    }
    return 1;

recorder_place:

{
    if (self->m_playerCommandPending != 0) {
        return 1;
    }
    if (self->m_dragInhibit1 != 0) {
        self->m_dragInhibit1 = 0;
        self->m_guts->CommitSlot(0);
        this->SetCursorFrame(0);
        if (key != VK_INSERT) {
            goto tail_default;
        }
        return 1;
    }
    if (self->m_dragInhibit2 == 0) {
        goto tail_default2;
    }
    i32 st = self->m_cursorFrame;
    i32 ph = self->m_guts->m_pendingHlRow;
    i32 lvl;
    if (st >= 0x22) {
        lvl = 2;
    } else {
        lvl = (st >= 0x17);
    }
    self->m_dragInhibit2 = 0;
    if (key == VK_DELETE || key == VK_DECIMAL) {
        level->ReportTab(st);
        this->SetCursorFrame(0);
        return 1;
    }
    level->EnterHlRow(0, st);
    this->SetCursorFrame(0);
    if (lvl == 0) {
        if (ph == 0) {
            if (key != VK_NUMLOCK) {
                goto tail_default;
            }
            return 1;
        }
        if (ph == 1) {
            if (key == VK_NUMPAD7) {
                return 1;
            }
            if (key != VK_HOME) {
                goto tail_default;
            }
            return 1;
        }
        if (ph == 2) {
            if (key == VK_NUMPAD4) {
                return 1;
            }
            if (key != VK_LEFT) {
                goto tail_default;
            }
            return 1;
        }
        if (key == VK_NUMPAD1) {
            return 1;
        }
        if (key != VK_END) {
            goto tail_default;
        }
        return 1;
    }
    if (lvl == 1) {
        if (ph == 0) {
            if (key != VK_DIVIDE) {
                goto tail_default;
            }
            return 1;
        }
        if (ph == 1) {
            if (key == VK_NUMPAD8) {
                return 1;
            }
            if (key != VK_UP) {
                goto tail_default;
            }
            return 1;
        }
        if (ph == 2) {
            if (key != VK_CLEAR) {
                goto tail_default;
            }
            return 1;
        }
        if (key == VK_NUMPAD2) {
            return 1;
        }
        if (key != VK_DOWN) {
            goto tail_default;
        }
        return 1;
    }
    if (ph == 0) {
        if (key != VK_MULTIPLY) {
            goto tail_default;
        }
        return 1;
    }
    if (ph == 1) {
        if (key == VK_NUMPAD9) {
            return 1;
        }
        if (key != VK_PRIOR) {
            goto tail_default;
        }
        return 1;
    }
    if (ph == 2) {
        if (key == VK_NUMPAD6) {
            return 1;
        }
        if (key != VK_RIGHT) {
            goto tail_default;
        }
        return 1;
    }
    if (key == VK_NUMPAD3) {
        return 1;
    }
    if (key != VK_NEXT) {
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

    if (self->m_guts->m_hitTestDisabled != 0) {
        return 1;
    }
    {

        CStatusBarMgr* lv = self->m_guts;
        switch (key) {
            case VK_CLEAR:
                lv->HlClickGroup1(2);
                return 1;
            case VK_PRIOR:
                lv->HlClickGroup2(1);
                return 1;
            case VK_NEXT:
                lv->HlClickGroup2(3);
                return 1;
            case VK_END:
                lv->HlClickGroup0(3);
                return 1;
            case VK_HOME:
                lv->HlClickGroup0(1);
                return 1;
            case VK_LEFT:
                lv->HlClickGroup0(2);
                return 1;
            case VK_UP:
                lv->HlClickGroup1(1);
                return 1;
            case VK_RIGHT:
                lv->HlClickGroup2(2);
                return 1;
            case VK_DOWN:
                lv->HlClickGroup1(3);
                return 1;
            case VK_INSERT:
                lv->ActivateSlot(-1);
                return 1;
            case VK_NUMPAD1:
                lv->HlClickGroup0(3);
                return 1;
            case VK_NUMPAD2:
                lv->HlClickGroup1(3);
                return 1;
            case VK_NUMPAD3:
                lv->HlClickGroup2(3);
                return 1;
            case VK_NUMPAD4:
                lv->HlClickGroup0(2);
                return 1;
            case VK_NUMPAD5:
                lv->HlClickGroup1(2);
                return 1;
            case VK_NUMPAD6:
                lv->HlClickGroup2(2);
                return 1;
            case VK_NUMPAD7:
                lv->HlClickGroup0(1);
                return 1;
            case VK_NUMPAD8:
                lv->HlClickGroup1(1);
                return 1;
            case VK_NUMPAD9:
                lv->HlClickGroup2(1);
                return 1;
            case VK_MULTIPLY:
                lv->HlClickGroup2(0);
                return 1;
            case VK_DIVIDE:
                lv->HlClickGroup1(0);
                return 1;
            case VK_NUMLOCK:
                lv->HlClickGroup0(0);
                return 1;
        }
    }
    return 1;
}

#undef CLEAR_TAB_HINT

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
void CPlay::PostSetup(void* dc) {
    RECT src = *(&m_world->m_level->m_planeCtx);
    RECT dst;
    CopyRect(&dst, &src);
    m_mgr->m_chatLog->DrawTextLines(8, static_cast<HDC>(dc), &dst, 0x10);
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

RVA(0x000d7440, 0xad)
i32 CPlay::LoadLoadingBarSprite() {
    CObject* spr_ob = 0;
    m_world->m_imageRegistry->m_10map.Lookup("GAME_LOADINGBAR", spr_ob);
    CDDrawWorker* spr = static_cast<CDDrawWorker*>(spr_ob);
    if (!spr) {
        return 0;
    }

    m_revealCapStart = (spr->m_minIndex <= 1 && spr->m_maxIndex >= 1)
                           ? static_cast<CImage*>(spr->m_items.GetAt(1))
                           : 0;
    m_revealCapMid = (spr->m_minIndex <= 2 && spr->m_maxIndex >= 2)
                         ? static_cast<CImage*>(spr->m_items.GetAt(2))
                         : 0;
    m_revealCapEnd = (spr->m_minIndex <= 3 && spr->m_maxIndex >= 3)
                         ? static_cast<CImage*>(spr->m_items.GetAt(3))
                         : 0;
    m_revealFrame = 1;
    return 1;
}

RVA(0x000d7520, 0x3b9)
i32 CPlay::SyncState(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 pObj) {
    if (ar == NULL) {
        return 0;
    }
    if (!HeaderSerialize(ar, mode, typeId, pObj)) {
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
            if (m_gridHasSprite) {
                CGruntzMgr* w = m_mgr;
                i32 id = g_curPlayer;
                CShadeTable* spr = w->m_spriteFactory->GetSel(w->m_options[id].m_colorIndex, 0);
                if (spr == NULL) {
                    spr = g_gameReg->m_spriteFactory->GetSel(1, 0);
                }
                m_grid->SetAllTypes(SHADE_PAL_16);
                m_grid->SetAllFormats(spr);
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
    if (!m_guts->Sync(ar, mode, typeId, pObj)) {
        return 0;
    }
    if (!m_frameMarker->HandleEvent(ar, mode, typeId, pObj)) {
        return 0;
    }
    p = &m_cueTimerLo;
    SYNC_PAIR(ar, mode, p);
    if (!m_beginMarker->Serialize(ar, mode, typeId, pObj)) {
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
    s->Write(&m_levelId, sizeof(m_levelId));
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
        char buf[0x80];
        memset(buf, 0, sizeof(buf));

        CImage* frame = m_gridCurFrame;
        i32 v = 0;
        if (frame != NULL) {
            mc->m_imageRegistry->AnyValueMatches(frame, buf, &v);
        }
        s->Write(buf, 0x80);
        s->Write(&v, sizeof(v));
    }

    g_serialCounter++;
    {
        char buf[0x80];
        memset(buf, 0, sizeof(buf));
        if (m_grid != NULL) {
            strcpy(buf, m_grid->m_name);
        }
        s->Write(buf, 0x80);
    }

    s->Write(&m_gridDelayBase, sizeof(m_gridDelayBase));
    s->Write(&m_gridDelayCount, sizeof(m_gridDelayCount));
    s->Write(&m_gridRow, sizeof(m_gridRow));

    g_serialCounter++;
    {
        i32 v = 0;
        if (m_scrollSink != NULL) {
            v = m_scrollSink->m_objectId;
        }
        s->Write(&v, sizeof(v));
    }

    s->Write(&m_gridWalkActive, sizeof(m_gridWalkActive));
    s->Write(&m_renderDisabled, sizeof(m_renderDisabled));
    s->Write(&m_winLoseBanner, sizeof(m_winLoseBanner));
    s->Write(&m_initialFramePending, sizeof(m_initialFramePending));
    s->Write(&m_hudSuppressed, sizeof(m_hudSuppressed));
    s->Write(&m_inGame, sizeof(m_inGame));
    s->Write(&m_overlayDrag, sizeof(m_overlayDrag));
    s->Write(&m_paused, sizeof(m_paused));
    s->Write(&m_playerCommandPending, sizeof(m_playerCommandPending));
    s->Write(&m_dragEndNotify, sizeof(m_dragEndNotify));
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
    s->Write(&m_snapshotActive, sizeof(m_snapshotActive));
    s->Write(&m_gridHasSprite, sizeof(m_gridHasSprite));
    s->Write(&m_cameraBookmarkIndex, sizeof(m_cameraBookmarkIndex));
    s->Write(&m_focusPlayerIndex, sizeof(m_focusPlayerIndex));

    count = CameraBookmarkCount();
    s->Write(&count, sizeof(count));
    for (i32 fi = 0; fi < CameraBookmarkCount(); fi++) {
        void* el = CameraBookmarkData()[fi];
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
    ar->Read(&m_levelId, sizeof(m_levelId));
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
        m_startMarkers.SetSize(0, -1);
        i32 n;
        ar->Read(&n, sizeof(n));
        for (u32 j = 0; j < static_cast<u32>(n); j++) {
            Coord* node = 0;
            CoordPoolNode* head = g_coordPool.m_freeHead;
            CoordPoolNode* next = head->m_next;
            if (next) {
                node = &head->m_coord;
                g_coordPool.m_freeHead = next;
            }
            ar->Read(node, sizeof(*node));
            m_startMarkers.SetAtGrow(StartMarkerCount(), node);
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
            CPtrArray* coll = &m_placedObjectCells[k];
            for (i32 i = 0; i < PlacedObjectCellCount(k); i++) {
                Coord* node = PlacedObjectCellAt(k, i);
                if (node) {
                    CoordPoolNode* q = g_coordPool.NodeOf(node);
                    q->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = q;
                }
            }
            coll->SetSize(0, -1);
            i32 n;
            ar->Read(&n, sizeof(n));
            for (u32 j = 0; j < static_cast<u32>(n); j++) {
                Coord* node = 0;
                CoordPoolNode* head = g_coordPool.m_freeHead;
                CoordPoolNode* next = head->m_next;
                if (next) {
                    node = &head->m_coord;
                    g_coordPool.m_freeHead = next;
                }
                ar->Read(node, sizeof(*node));
                coll->SetAtGrow(PlacedObjectCellCount(k), node);
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
    char buf80a[0x80];
    ar->Read(buf80a, 0x80);
    i32 idx;
    ar->Read(&idx, sizeof(idx));
    if (strlen(buf80a) == 0) {
        m_gridCurFrame = NULL;
    } else {
        CObject* found = 0;
        res->m_imageRegistry->m_10map.Lookup(static_cast<const char*>(buf80a), found);
        CDDrawWorker* set = static_cast<CDDrawWorker*>(found);
        if (set == NULL || idx < set->m_minIndex || idx > set->m_maxIndex) {
            m_gridCurFrame = NULL;
        } else {
            m_gridCurFrame = static_cast<CImage*>(set->m_items.GetAt(idx));
        }
    }

    g_serialCounter++;
    char buf80b[0x80];
    ar->Read(buf80b, 0x80);
    CObject* gridObj = 0;
    if (strlen(buf80b) == 0) {
        m_grid = NULL;
    } else {
        res->m_imageRegistry->m_10map.Lookup(buf80b, gridObj);
        m_grid = static_cast<CDDrawWorker*>(gridObj);
    }

    ar->Read(&m_gridDelayBase, sizeof(m_gridDelayBase));
    ar->Read(&m_gridDelayCount, sizeof(m_gridDelayCount));
    ar->Read(&m_gridRow, sizeof(m_gridRow));
    g_serialCounter++;
    i32 v;
    ar->Read(&v, sizeof(v));

    CGameObject* oe = 0;
    MapLookup(res->m_childGroup->m_map48, gridObj, oe);
    CWwdGameObjectA* sink;
    if (oe == NULL) {
        sink = NULL;
    } else {

        sink = oe->GetClassId() == CLASSID_SERIALREF ? static_cast<CWwdGameObjectA*>(oe) : 0;
    }
    m_scrollSink = sink;
    if (sink == NULL && gridObj != NULL) {
        return 0;
    }

    ar->Read(&m_gridWalkActive, sizeof(m_gridWalkActive));
    ar->Read(&m_renderDisabled, sizeof(m_renderDisabled));
    ar->Read(&m_winLoseBanner, sizeof(m_winLoseBanner));
    ar->Read(&m_initialFramePending, sizeof(m_initialFramePending));
    ar->Read(&m_hudSuppressed, sizeof(m_hudSuppressed));
    ar->Read(&m_inGame, sizeof(m_inGame));
    ar->Read(&m_overlayDrag, sizeof(m_overlayDrag));
    ar->Read(&m_paused, sizeof(m_paused));
    ar->Read(&m_playerCommandPending, sizeof(m_playerCommandPending));
    ar->Read(&m_dragEndNotify, sizeof(m_dragEndNotify));
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
    ar->Read(&m_snapshotActive, sizeof(m_snapshotActive));
    ar->Read(&m_gridHasSprite, sizeof(m_gridHasSprite));
    ar->Read(&m_cameraBookmarkIndex, sizeof(m_cameraBookmarkIndex));
    m_stepCountdown = 2;
    ar->Read(&m_focusPlayerIndex, sizeof(m_focusPlayerIndex));

    i32 n488;
    ar->Read(&n488, sizeof(n488));
    {
        for (i32 i = 0; i < CameraBookmarkCount(); i++) {
            void* node = CameraBookmarkData()[i];
            if (node) {
                CoordPoolNode* q = g_coordPool.NodeOf(node);
                q->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = q;
            }
        }
        m_cameraBookmarks.SetSize(0, -1);
        m_cameraBookmarks.SetSize(n488, -1);
        for (u32 j = 0; j < static_cast<u32>(n488); j++) {
            void* node = 0;
            CoordPoolNode* head = g_coordPool.m_freeHead;
            CoordPoolNode* next = head->m_next;
            if (next) {
                node = &head->m_coord;
                g_coordPool.m_freeHead = next;
            }
            ar->Read(node, 8);
            CameraBookmarkData()[j] = node;
        }
    }
    return 1;
}

// @early-stop
RVA(0x000d8c60, 0xea)
i32 CPlay::ResetViewport() {
    CGruntzMgr* w = m_mgr;
    i32 right = w->m_modeW;
    StatusBarDock state = m_guts->m_position;
    i32 bottom = w->m_modeH;
    RECT r;
    if (state == STATUSBAR_DOCK_LEFT) {
        SetRect(&r, 0xa0, 0, right - 1, bottom - 1);
    } else if (state == STATUSBAR_DOCK_RIGHT) {
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
    m_viewMode = IDX(VIEW_MODE_IDLE);
    m_world->m_level->BuildAllPlanes((&r));
    m_mgr->RecomputeViewScale();
    return 1;
}

RVA(0x000d8d90, 0x1e)
i32 CPlay::StepViewportResize() {
    PlayViewMode mode = static_cast<PlayViewMode>(m_viewMode);
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
    limit.cx = w->m_modeW;
    limit.cy = w->m_modeH;

    if (r.right - r.left < (guts->m_position == STATUSBAR_HIDDEN ? limit.cx : limit.cx - 0xa0)) {
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

RVA(0x000d88f0, 0x44)
void CPlay::RegionEnter() {
    if (m_savedZonedSound == NULL) {
        CGruntzMgr* w = m_mgr;
        m_savedZonedSound = w->m_sound->m_pCurrent;
        w->m_sound->StopAll();
    }
    if (g_gameReg->m_musicEnabled != 0) {
        m_mgr->m_sound->PlayByName("CURSE", 0);
    }
}

RVA(0x000d8960, 0x75)
void CPlay::RegionLeave() {
    if (m_region0Gate == 0 && m_region1Gate == 0 && m_region2Gate == 0 && m_region3Gate == 0
        && m_savedZonedSound != NULL) {
        m_mgr->m_sound->IsPlaying();
        m_mgr->m_sound->m_pCurrent = m_savedZonedSound;
        if (g_gameReg->m_musicEnabled != 0) {
            m_mgr->m_sound->Restart(1);
        }
        m_savedZonedSound = NULL;
    }
}

RVA(0x000d8a00, 0x73)
i32 CPlay::SetTinyViewportCurse(i32 active) {
    if (active != 0) {
        m_region0Gate = 1;
        RegionEnter();
        m_viewMode = IDX(VIEW_MODE_A);
    } else {
        m_region0Gate = 0;
        RegionLeave();
        m_viewMode = IDX(VIEW_MODE_B);
    }
    m_region0Interval = REGION_INTERVAL_MS;
    m_region0IntervalHi = 0;
    m_region0Timer64.m_v = g_frameTime;
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
    m_region1Interval = REGION_INTERVAL_MS;
    m_region1IntervalHi = 0;
    m_region1Timer64.m_v = g_frameTime;
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
    m_region2Interval = REGION_INTERVAL_MS;
    m_region2IntervalHi = 0;
    m_region2Timer64.m_v = g_frameTime;
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
    m_region3Interval = REGION_INTERVAL_MS;
    m_region3IntervalHi = 0;
    m_region3Timer64.m_v = g_frameTime;
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
        CGameObject* o = static_cast<CGameObject*>(chain.GetNext(pos));
        GameObjNotifyFn id = o->m_animWorker->m_notify;
        if (id == CreateGrunt || id == CreateInGameIcon || id == CreateGruntPuddle
            || id == CreateGruntToySprite || id == CreateGruntStaminaSprite
            || id == CreateGruntToyTimeSprite || id == CreateGruntWingzTimeSprite
            || id == CreateGruntHealthSprite || id == CreateGruntSelectedSprite
            || id == CreateGruntPowerupSprite || id == CreateStatusBarSprite
            || id == CreateLightFx) {
            o->Render(held);
        }
    }
    return 1;
}

RVA(0x000d1ac0, 0x4f)
void CPlay::StepScroll() {
    CGameLevel* v = m_world->m_level;

    RECT* vr = &v->m_mainPlane->m_viewRect;

    i32 y = m_cursorY + (vr->top - v->m_planeCtx.top);
    i32 x = vr->left + (m_cursorX - v->m_planeCtx.left);

    y = (y & ~0x1f) + 0x10;
    x = (x & ~0x1f) + 0x10;

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
        CDDrawPtrCollections::GetErrorString(0, 0, r);
    }
    return 1;
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
    i32 toBackPage,
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

    if (toBackPage != 0) {
        ShowHudMessageAlt(m_world, &m_cueText, &rect, fontSel, 1, r, g, b, flag);
    } else {
        EngStr_DrawText(m_world, &m_cueText, &rect, fontSel, 1, r, g, b, flag);
    }
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
    g_killCueClock = g_lastNow;
    g_engineFrameDelta = g_frameDelta;
    m_world->m_childGroup->TickKillCues(0);
    m_mgr->m_cmdGrid->LoadTeleporterGooConfig(static_cast<i32>(g_frameDelta));
    if (g_gameReg->m_gameMode == GAMEMODE_REPLAY) {

        (g_gameReg)->AdvanceOptionsCycle();
    }
    m_guts->LoadDestructButtonSprite(static_cast<i32>(g_frameDelta));
}

// @early-stop
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
    if (steps > 0) {
        i32 last = steps - 1;
        i32 i = 0;
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
            m_mgr->m_cmdGrid->LoadTeleporterGooConfig(static_cast<i32>(g_frameDelta));
            if (g_gameReg->m_gameMode == GAMEMODE_REPLAY) {
                (g_gameReg)->AdvanceOptionsCycle();
            }
            m_guts->LoadDestructButtonSprite(static_cast<i32>(g_frameDelta));
            i++;
        } while (i < steps);
    }
    m_mgr->SetGameClock(saveNow, saveDelta, saveAccum);
    return steps;
}

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
    m_mgr->m_inputState->Retune(
        m_world->m_level->m_mainPlane->m_snappedX,
        m_world->m_level->m_mainPlane->m_snappedY
    );
    u32 t2 = tg();
    m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
    m_world->m_workerList->PruneWorkers(
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
    m_world->m_drawTarget->m_frontPair->m_surface->Flip(0);

    CGameLevel* lvl = m_world->m_level;
    if (lvl->m_mainPlane != NULL) {
        lvl->m_mainPlane->DeactivateDistantObjects();
    }
    return 1;
}

DATA(0x0024c284)
i32 g_profAccA;
DATA(0x0024c288)
i32 g_profAccB;

RVA(0x000c9e40, 0x1d7)
i32 CPlay::ProfileInputFrame() {
    m_mgr->m_inputState->Retune(
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
    m_mgr->m_cmdGrid->LoadTeleporterGooConfig(static_cast<i32>(g_frameDelta));
    m_guts->LoadDestructButtonSprite(static_cast<i32>(g_frameDelta));
    updateMs = static_cast<i32>(tg() - static_cast<u32>(updateMs));

    i32 hitTestMs = static_cast<i32>(tg());
    hitTestMs = static_cast<i32>(tg() - static_cast<u32>(hitTestMs));

    i32 drawMs = static_cast<i32>(tg());
    m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
    drawMs = static_cast<i32>(tg() - static_cast<u32>(drawMs));

    i32 fixedMs = static_cast<i32>(tg());
    m_world->m_workerList->PruneWorkers(
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
    m_world->m_drawTarget->m_frontPair->m_surface->Flip(0);
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

RVA(0x000d9240, 0x3c)
i32 CPlay::ArmSnapshot(i32 active, i32 dur) {
    if (active != 0) {

        m_snapDur = dur;
        m_snapDurHi = 0;
        m_snapBase64.m_v = static_cast<u32>(g_frameTime);
    }
    m_snapshotActive = active;
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
    i32 r;
    r = rand() % arr.GetSize();
    perm[0] = arr.GetAt(r);
    arr.RemoveAt(r, 1);
    r = rand() % arr.GetSize();
    perm[1] = arr.GetAt(r);
    arr.RemoveAt(r, 1);
    r = rand() % arr.GetSize();
    perm[2] = arr.GetAt(r);
    arr.RemoveAt(r, 1);
    perm[3] = arr.GetAt(0);
    arr.RemoveAt(0, 1);

    while (pos != NULL) {
        CGameObject* p = static_cast<CGameObject*>(pl->GetNext(pos));
        if (p == NULL) {
            continue;
        }
        GameObjNotifyFn vf = p->m_animWorker->m_notify;
        if (vf == CreateGruntCreationPoint || vf == CreateExitTrigger || vf == CreateFortressFlag
            || vf == CreateWayPoint || vf == CreateGuardPoint) {
            p->m_smarts = perm[p->m_smarts];
        } else if (vf == CreateBrickz) {
            if (p->m_extent.left == static_cast<i32>(0x80000000)) {
                p->m_extent.left = 0;
            }
            if (p->m_area.left == static_cast<i32>(0x80000000)) {
                p->m_area.left = 0;
            }
            if (p->m_switchRect.left == static_cast<i32>(0x80000000)) {
                p->m_switchRect.left = 0;
            }
            if (p->m_clip.left == static_cast<i32>(0x80000000)) {
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
            s0.LoadString(0x81ae);
            break;
        case AREA_GRUNTZICLEZ:
            s0.LoadString(0x81af);
            break;
        case AREA_TROUBLE_IN_THE_TROPICZ:
            s0.LoadString(0x81b0);
            break;
        case AREA_HIGH_ON_SWEETZ:
            s0.LoadString(0x81b1);
            break;
        case AREA_HIGH_ROLLERZ:
            s0.LoadString(0x81b2);
            break;
        case AREA_HONEY_I_SHRUNK_THE_GRUNTZ:
            s0.LoadString(0x81b3);
            break;
        case AREA_MINIATURE_MASTERZ:
            s0.LoadString(0x81b4);
            break;
        case AREA_GRUNTZ_IN_SPACE:
            s0.LoadString(0x81b5);
            break;
        default:
            s0 = g_emptyString;
    }

    GameModeId mode = g_gameReg->m_gameMode;
    if (mode == GAMEMODE_SINGLE) {
        if (g_gameReg->m_isCustomLevel != 0) {
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
    } else if (mode == GAMEMODE_REPLAY) {
        if (g_gameReg->m_isCustomLevel != 0) {
            s1.LoadString(0x819f);
        } else {
            s1.LoadString(0x819e);
        }
    } else if (mode == GAMEMODE_MULTIPLAYER) {
        if (g_gameReg->m_isCustomLevel != 0) {
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
        if (strrchr(buf, '\\') != NULL) {
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
    EngStr_DrawText(m_world, &s0, &r1, 0x78, 0, 0, 0, 0, 1);
    EngStr_DrawText(m_world, &s1, &r2, 0x6e, 0, 0, 0, 0, 1);
    EngStr_DrawText(m_world, &s2, &r3, 0x6e, 0, 0, 0, 0, 1);
    EngStr_DrawText(m_world, &s3, &r4, 0x6e, 0, 0, 0, 0, 1);
    return 1;
}

// @early-stop
RVA(0x000da030, 0x169)
i32 CPlay::ClearPlacedObjects() {
    for (i32 blockIdx = 0; blockIdx < 4; ++blockIdx) {
        CPtrArray* rec = &m_placedObjectCells[blockIdx];
        i32 i = 0;
        i32 restart = 0;
        while (i < rec->GetSize()) {
            Coord* obj = static_cast<Coord*>(rec->GetAt(i));
            CMapMgr* grid = g_gameReg->m_tileGrid;

            i32 occupantId = 0;
            if (static_cast<u32>(obj->m_x) < static_cast<u32>(grid->m_width)
                && static_cast<u32>(obj->m_y) < static_cast<u32>(grid->m_height)) {
                i32 stride = obj->m_x * 7;
                i32* row = grid->m_rowInts[obj->m_y];
                occupantId = row[stride + 2];
            }
            i32 stillPlaced = 0;
            if (occupantId != 0) {
                void* out = 0;

                CMapPtrToPtr* map = &g_gameReg->m_world->m_childGroup->m_map48;

                // Byte-evidenced shipped pointer/id bug.
                CGameObject* result = reinterpret_cast<CGameObject*>(occupantId);
                if (MapLookupById(*map, occupantId, out)) {
                    result = static_cast<CGameObject*>(out);
                }
                if (result == NULL) {

                    CMapMgr* g = g_gameReg->m_tileGrid;
                    if (static_cast<u32>(obj->m_x) < static_cast<u32>(g->m_width)
                        && static_cast<u32>(obj->m_y) < static_cast<u32>(g->m_height)) {
                        i32 stride = obj->m_x * 7;
                        i32* row = g->m_rowInts[obj->m_y];
                        row[stride + 2] = 0;
                        i32* row2 = g->m_rowInts[obj->m_y];
                        row2[stride] &= 0xfffbffff;
                    }
                    rec->RemoveAt(i, 1);

                    CoordPoolNode* node = g_coordPool.NodeOf(obj);
                    node->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = node;
                    return -1;
                }
                stillPlaced = (result->m_smarts == PICKUP_WARPSTONE);
            }

            if (!stillPlaced) {
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

RVA(0x000da200, 0x9b)
i32 CPlay::GetAmbientId() {
    CGruntzMgr* gr = g_gameReg;
    if (gr->m_gameMode == GAMEMODE_SINGLE && gr->m_isCustomLevel == 0) {
        return (m_levelIndex + 1) % 2;
    }
    if (!(g_coinRolled & 1)) {
        i32 seed;
        g_coinRolled |= 1;
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            seed = timeGetTime();
        } else {
            seed = g_randSeed;
        }
        g_randSeed = seed * 214013 + 2531011;
        g_coinValue = ((g_randSeed >> 0x10) & 0x7fff) % 2;
    }
    return g_coinValue;
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
        m_mgr->m_cmdGrid->HudRect(m_hudRect, g_spawnConfig->m_edgeKeys & 0x20);
    }
    m_worldReady = 0;
    m_dragSnapActive = 0;
    return 1;
}

RVA(0x000da790, 0xb0)
GruntzPlayer::GruntzPlayer() {
    m_playerIndex = -1;
    m_slotKey = -2;
    m_liveGate = 0;
    m_joined = 0;
    m_humanControlled = 1;
    m_name = g_emptyString;
    m_colorIndex = 0;
    m_configId = 0;
    m_focusX = 0;
    m_focusY = 0;
    m_comboSel = 0xf;
    m_doneFlag = 0;
    m_presenceCounted = 0;
    m_latency.Clear();
}

RVA(0x00083260, 0x57)
GruntzPlayer::~GruntzPlayer() {
    Clear();
}

RVA(0x000da870, 0xb8)
i32 GruntzPlayer::SeedForSlot(i32 index) {
    m_playerIndex = index;
    m_slotKey = -2;
    m_liveGate = 0;
    m_joined = 0;
    m_humanControlled = 1;
    m_name = g_emptyString;

    m_colorIndex = index;
    m_configId = 0;
    m_focusX = 0;
    m_focusY = 0;
    m_comboSel = 0xf;
    m_doneFlag = 0;
    m_presenceCounted = 0;
    m_name = GetDefaultName(0);
    m_latency.Clear();
    return 1;
}

RVA(0x000da960, 0x5b)
void GruntzPlayer::Clear() {
    m_playerIndex = -1;
    m_slotKey = -2;
    m_liveGate = 0;
    m_humanControlled = 1;
    m_name = g_emptyString;
    m_colorIndex = 0;
    m_configId = 0;
    m_focusX = 0;
    m_focusY = 0;
    m_comboSel = 0xf;
    m_doneFlag = 0;
    m_presenceCounted = 0;
    m_latency.Clear();
}

RVA(0x000da9e0, 0x60)
i32 GruntzPlayer::Reset() {
    m_playerIndex = -1;
    m_slotKey = -2;
    m_liveGate = 0;
    m_humanControlled = 1;
    m_name = g_emptyString;
    m_colorIndex = 0;
    m_configId = 0;
    m_focusX = 0;
    m_focusY = 0;
    m_comboSel = 0xf;
    m_doneFlag = 0;
    m_presenceCounted = 0;
    m_latency.Clear();
    return 1;
}

RVA(0x000daa60, 0x24)
i32 GruntzPlayer::ClearRoundState() {
    m_liveGate = 1;
    m_readyFlag = 0;
    m_doneFlag = 0;
    m_presenceCounted = 0;
    m_latency.Clear();
    return 1;
}

RVA(0x000daaa0, 0xd3)
i32 FillColorCombo(HWND hDlg, i32 nID, i32 curSel) {
    if (hDlg == NULL) {
        return 0;
    }
    HWND cb = GetDlgItem(hDlg, nID);
    if (cb == NULL) {
        return 0;
    }
    LRESULT(WINAPI * pSend)(HWND, UINT, WPARAM, LPARAM) = SendMessageA;
    pSend(cb, CB_RESETCONTENT, 0, 0);
    for (i32 i = 0; i < 0x11; i++) {
        pSend(
            cb,
            CB_ADDSTRING,
            0,
            reinterpret_cast<LPARAM>(static_cast<const char*>(GetColorName(i, 0)))
        );
    }
    if (curSel >= 0) {
        pSend(cb, CB_SETCURSEL, curSel, 0);
    }
    return 1;
}

RVA(0x000dabc0, 0xd3)
i32 FillDifficultyCombo(HWND hDlg, i32 nID, i32 curSel) {
    if (hDlg == NULL) {
        return 0;
    }
    HWND cb = GetDlgItem(hDlg, nID);
    if (cb == NULL) {
        return 0;
    }
    LRESULT(WINAPI * pSend)(HWND, UINT, WPARAM, LPARAM) = SendMessageA;
    pSend(cb, CB_RESETCONTENT, 0, 0);
    for (i32 i = 0; i < 3; i++) {
        pSend(
            cb,
            CB_ADDSTRING,
            0,
            reinterpret_cast<LPARAM>(static_cast<const char*>(GetDifficultyName(i, 0)))
        );
    }
    if (curSel >= 0) {
        pSend(cb, CB_SETCURSEL, curSel, 0);
    }
    return 1;
}

RVA(0x000dace0, 0x239)
i32 GruntzPlayer::Serialize(CFileMemBase* ar, SerialMode kind, LogicTypeId typeId, i32 pObj) {
    char tmp[0x80];

    if (kind != SERIAL_SAVE) {
        if (kind == SERIAL_LOAD) {

            ar->Read(&m_playerIndex, sizeof(m_playerIndex));
            ar->Read(&m_colorIndex, sizeof(m_colorIndex));
            ar->Read(&m_warlordObjectId, sizeof(m_warlordObjectId));
            ar->Read(&m_configId, sizeof(m_configId));
            ar->Read(&m_humanControlled, sizeof(m_humanControlled));
            ar->Read(&m_slotKey, sizeof(m_slotKey));
            ar->Read(&m_readyFlag, sizeof(m_readyFlag));
            ar->Read(&m_liveGate, sizeof(m_liveGate));
            ar->Read(&m_joined, sizeof(m_joined));
            ar->Read(&m_clearedRound, sizeof(m_clearedRound));
            g_serialCounter++;
            ar->Read(tmp, 0x80);
            m_name = tmp;
            ar->Read(&m_focusX, sizeof(m_focusX));
            ar->Read(&m_focusY, sizeof(m_focusY));
            ar->Read(&m_comboSel, sizeof(m_comboSel));
        }
    } else {

        ar->Write(&m_playerIndex, sizeof(m_playerIndex));
        ar->Write(&m_colorIndex, sizeof(m_colorIndex));
        ar->Write(&m_warlordObjectId, sizeof(m_warlordObjectId));
        ar->Write(&m_configId, sizeof(m_configId));
        ar->Write(&m_humanControlled, sizeof(m_humanControlled));
        ar->Write(&m_slotKey, sizeof(m_slotKey));
        ar->Write(&m_readyFlag, sizeof(m_readyFlag));
        ar->Write(&m_liveGate, sizeof(m_liveGate));
        ar->Write(&m_joined, sizeof(m_joined));
        ar->Write(&m_clearedRound, sizeof(m_clearedRound));
        g_serialCounter++;
        memset(tmp, 0, sizeof(tmp));
        strcpy(tmp, static_cast<const char*>(m_name));
        ar->Write(tmp, 0x80);
        ar->Write(&m_focusX, sizeof(m_focusX));
        ar->Write(&m_focusY, sizeof(m_focusY));
        ar->Write(&m_comboSel, sizeof(m_comboSel));
    }
    return (static_cast<CBattlezMapConfig*>(&m_battlezConfig))
               ->SerializeState(ar, kind, typeId, pObj)
           != 0;
}

RVA(0x000dafb0, 0x71)
CString GruntzPlayer::GetDefaultName(i32) {

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

// NOT sound channels, despite the name this was reconstructed under: it is one
// slot per ColorTint saying whether that colour is still AVAILABLE to a player.
// The multiplayer roster proves both halves - it calls FindFree(), assigns the
// result to s->m_colorIndex, then Set(free, false) to take the colour, and
// Set(s->m_colorIndex, true) to hand it back when the slot empties. The size is
// TINT_COUNT, not a coincidence.
DATA(0x0024c3f0)
i32 g_soundChannelInUse[TINT_COUNT];

RVA(0x000db1d0, 0x14)
void ChannelSlots_InitAll() {
    for (i32 i = 0; i < TINT_COUNT; i++) {
        g_soundChannelInUse[i] = true;
    }
}

RVA(0x000db200, 0x51)
i32 GruntzPlayer::SwapChannel(i32 channel) {
    if (m_colorIndex == channel) {
        return 1;
    }
    if (ChannelSlots_Get(channel)) {
        ChannelSlots_Set(m_colorIndex, 1);
        ChannelSlots_Set(channel, 0);
        m_colorIndex = channel;
        return 1;
    }
    return 0;
}

RVA(0x000db280, 0x1b)
i32 ChannelSlots_FindFree() {
    for (i32 i = 0; i < TINT_COUNT; i++) {
        if (g_soundChannelInUse[i] != false) {
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
    if (m_humanControlled == 0) {
        (static_cast<CBattlezMapConfig*>(&m_battlezConfig))->Clear();
    }
    m_liveGate = 0;
    return 1;
}

// @early-stop
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
    CDDrawWorker* g = m_grid;
    CImage* frame;
    if (idx >= g->m_minIndex && idx <= g->m_maxIndex) {
        frame = static_cast<CImage*>(g->m_items.GetAt(idx));
    } else {
        frame = NULL;
    }
    m_gridCurFrame = frame;
    if (frame == NULL) {
        m_gridCurFrame = static_cast<CImage*>(g->m_items.GetAt(g->m_minIndex));
        m_gridRow = g->m_minIndex;
    }
    return 1;
}

// @early-stop
RVA(0x000ce530, 0xe3)
i32 CPlay::OnLButtonUp(i32 a, i32 x, i32 y) {
    if (m_hudSuppressed != 0) {
        return 1;
    }
    if (m_lightFx != NULL && m_guts->m_position != STATUSBAR_HIDDEN
        && m_guts->m_activeTab != TAB_GAME) {
        m_lightFx->EndMinimapPan(a, x, y);
    }
    if (m_worldReady != 0) {
        m_mgr->m_cmdGrid->HudRect(m_hudRect, g_spawnConfig->m_edgeKeys & 0x20);
    }
    m_worldReady = 0;
    m_dragSnapActive = 0;
    if (m_guts->m_position == STATUSBAR_HIDDEN) {
        return 1;
    }

    LevelCoordRect vp = m_world->m_level->m_planeCtx;
    if (x >= vp.left && x <= vp.right && y >= vp.top && y <= vp.bottom) {
        return 1;
    }
    m_guts->OnPointerRelease(a, x, y);
    return 1;
}

// @early-stop
RVA(0x000ce660, 0x362)
i32 CPlay::OnLButtonDblClk(i32 msg, i32 x, i32 y) {
    if (m_hudSuppressed != 0 || m_guts == NULL) {
        return 1;
    }
    if (m_overlayDrag != 0 || g_gameReg->m_cmdGrid->m_groupFlag == 0) {
        return m_guts->ClickHilite(msg, x, y);
    }
    if (m_dragInhibit1 != 0 || m_dragInhibit2 != 0) {
        return this->OnLButtonDown(msg, x, y);
    }

    if (m_guts->m_position == STATUSBAR_HIDDEN && m_guts->HitTestLayer(x, y)) {
        CDDrawSubMgrLeafScan* set = m_mgr->m_world->m_soundRegistry;
        if (set->m_emitGate == 0) {
            LeafCue* e = 0;
            MapLookup(set->m_cues, "GAME_TABHIGHLIGHT1", e);
            if (e != NULL) {
                e->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
            }
        }
        m_guts->RefreshState();
        if (m_guts->m_position == STATUSBAR_DOCK_LEFT) {
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
    if (cfg == NULL) {
        return 0;
    }
    if (g_gameReg->m_cmdGrid->m_rowCount[area] >= cfg->m_comboSel) {
        return 0;
    }

    CGameLevel* h = m_mgr->m_world->m_level;
    i32 px = h->m_mainPlane->m_viewRect.left - h->m_planeCtx.left + x;
    i32 py = h->m_mainPlane->m_viewRect.top - h->m_planeCtx.top + y;
    for (i32 i = 0; i < StartMarkerCount(); i++) {
        Coord* e = StartMarkerAt(i);
        if (e == NULL) {
            continue;
        }
        RECT er;
        SetRect(&er, e->m_x - 0x10, e->m_y - 0x10, e->m_x + 0x10, e->m_y + 0x10);
        if (px < er.right && px >= er.left && py < er.bottom && py >= er.top) {
            if (!m_guts->FindReadySlot()) {
                return 1;
            }
            char ab = static_cast<char>(g_curPlayer);
            px = (px & 0xffe0) + 0x10;
            py = (py & 0xffe0) + 0x10;
            m_mgr->m_cmdSubMgr->EnqueueSingle(1, ab, 0, PLAYERCMD_PLACE_GRUNT, px, py, 0, 0);
            return 1;
        }
    }
    return 1;
}

// @early-stop
RVA(0x000d0920, 0xfe)
i32 CPlay::BeginGridWalk(const char* key, i32 index, i32 e8, i32 delay, i32 hasGrid) {
    if (m_world == NULL) {
        return 0;
    }
    CDDrawWorker* grid = 0;
    CObject* gridOb = 0;

    m_world->m_imageRegistry->m_10map.Lookup(key, gridOb);
    grid = static_cast<CDDrawWorker*>(gridOb);
    m_grid = grid;
    if (grid == NULL) {
        return 0;
    }
    m_gridHasSprite = hasGrid;
    if (hasGrid != 0) {
        CGruntzMgr* w = m_mgr;
        i32 id = g_curPlayer;
        CShadeTable* spr = w->m_spriteFactory->GetSel(w->m_options[id].m_colorIndex, 0);
        if (spr == NULL) {
            spr = g_gameReg->m_spriteFactory->GetSel(1, 0);
        }
        m_grid->SetAllTypes(SHADE_PAL_16);
        m_grid->SetAllFormats(spr);
    }
    CDDrawWorker* g = m_grid;
    CImage* frame;
    if (index >= g->m_minIndex && index <= g->m_maxIndex) {
        frame = static_cast<CImage*>(g->m_items.GetAt(index));
    } else {
        frame = NULL;
    }
    m_gridCurFrame = frame;
    if (frame == NULL) {
        return 0;
    }
    m_gridRow = index;
    m_gridWalkActive = e8;
    m_gridDelayBase = delay;
    m_gridDelayCount = delay;
    return 1;
}

// @early-stop
RVA(0x000d0db0, 0x347)
i32 CPlay::HandleDragMove(i32 a, i32 x, i32 y) {

    i32 left, top, right, bottom;
    if (m_inGame != 0) {
        return 1;
    }
    if (m_paused != 0) {
        return 1;
    }
    if (m_lightFx != NULL && m_guts->m_position != STATUSBAR_HIDDEN
        && m_guts->m_activeTab != TAB_GAME) {
        m_lightFx->ContinueMinimapPan(a, x, y);
    }

    if (m_dragSnapActive != 0) {
        if (m_guts == NULL) {
            return 1;
        }
        m_guts->SetSpritePos(m_snapOriginX + x, m_snapOriginY + y);
        goto rearm;
    }

    if (m_overlayDrag != 0) {
        m_guts->ClickToggle(a, x, y);
        return 1;
    }

    LevelCoordRect box = m_world->m_level->m_planeCtx;
    left = box.left;
    top = box.top;
    right = box.right;
    bottom = box.bottom;
    if (x >= left && x <= right && y >= top && y <= bottom) {

        if (m_dragInProgress != 0) {
            m_guts->ClearTabSprites(-1);
        }
        m_dragInProgress = 0;
        if (m_worldReady != 0) {

            m_hudRect.left = m_cursorX < m_dragClampMaxX ? m_cursorX : m_dragClampMaxX;
            m_hudRect.right = m_cursorX > m_dragClampMaxX ? m_cursorX : m_dragClampMaxX;
            m_hudRect.top = m_cursorY < m_dragClampMaxY ? m_cursorY : m_dragClampMaxY;
            m_hudRect.bottom = m_cursorY > m_dragClampMaxY ? m_cursorY : m_dragClampMaxY;
            goto rearm;
        }

        if (m_hitTest->HitTest(x, y) != 0 || m_mgr->m_frameGate != 0 || m_inGame != 0
            || m_dragInhibit1 != 0 || m_dragInhibit2 != 0) {

            CWwdGameObjectA* s2 = m_scrollSink;
            if (s2 == NULL) {
                return 1;
            }
            s2->m_stateFlags |= 1;
            return 1;
        }
        if (m_levelId != 0) {
            if (m_scrollSink != NULL) {
                m_scrollSink->m_stateFlags |= 1;
            }
        } else {
            if (m_scrollSink != NULL) {
                m_scrollSink->m_stateFlags &= ~1;
            }
        }
        CGameLevel* v = m_world->m_level;
        i32 wx = v->m_mainPlane->m_viewRect.left - v->m_planeCtx.left + x;
        i32 wy = v->m_mainPlane->m_viewRect.top - v->m_planeCtx.top + y;
        m_mgr->m_cmdGrid->PlaceObjectFull(wx, wy);
        return 1;
    }

    if (m_scrollSink != NULL) {
        m_scrollSink->m_stateFlags |= 1;
    }
    m_dragInProgress = 1;
    m_guts->ClickToggle(a, x, y);
    if (m_worldReady != 0) {

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
        FlushPendingOps();
    }
    return 1;

rearm:
    CWwdGameObjectA* s = m_scrollSink;
    if (s == NULL) {
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

    PostMessageA(g_gameReg->m_gameWnd->m_hwnd, WM_COMMAND, 0x816e, 0);
    if (m_scrollSink) {
        m_scrollSink->m_stateFlags |= 1;
    }
    return 1;
}

// @early-stop
RVA(0x000d72c0, 0x128)
i32 CPlay::BuildHelpReveal(i32 final) {
    static_cast<void>(final);

    CDDrawSurfacePair* view = m_world->m_drawTarget->m_backPair;
    if (view == NULL) {
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

// @interleaver UnusedPlayQuery - 3 B, sits in this class's destructor-COMDAT pool at
// 0x8c930 rather than in the TU's own .text block.
RVA(0x0008c930, 0x3)
i32 CPlay::UnusedPlayQuery() {
    return 0;
}

// @interleaver GetFrame - 3 B, sits in this class's destructor-COMDAT pool at
// 0x8c950 rather than in the TU's own .text block.
RVA(0x0008c950, 0x3)
i32 CPlay::GetFrame() {
    return 0;
}

// @interleaver OnMouseMove - 28 B, sits in this class's destructor-COMDAT pool at
// 0x8c970 rather than in the TU's own .text block.
RVA(0x0008c970, 0x1c)
i32 CPlay::OnMouseMove(i32 unused, i32 cursorX, i32 cursorY) {
    m_cursorX = cursorX;
    m_cursorY = cursorY;
    return 1;
}

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
i32 CPlay::OnLButtonDown(i32 a, i32 x, i32 y) {

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
        m_mgr->ReportError(IDX(CMD_TOGGLE_MUSIC), 0x457);
        return 1;
    }
    if (m_paused != 0) {
        m_paused = 0;
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, 0x816e, 0);
        return 1;
    }

    if (m_overlayDrag == 0 && g_gameReg->m_cmdGrid->m_groupFlag != 0) {
        if (m_mgr->m_frameGate != 0) {
            goto drag_path;
        }

        if (m_lightFx != NULL && m_guts->m_position != STATUSBAR_HIDDEN
            && m_guts->m_activeTab != TAB_GAME) {
            if (m_lightFx->BeginMinimapPan(a, xr, y)) {
                return 1;
            }
        }
        CGruntzMgr* w = m_mgr;
        CGameLevel* geom = w->m_world->m_level;
        CDDrawWorkerHost* cam = geom->m_mainPlane;
        i32 sx = cam->m_viewRect.left - geom->m_planeCtx.left + xr;
        i32 sy = cam->m_viewRect.top - geom->m_planeCtx.top + y;
        if (m_dragInhibit1 == 0) {
            goto mode_36c;
        }
        if (m_playerCommandPending != 0) {
            goto mode_36c;
        }
        i32 placed = 0;
        RECT* gr = &m_guts->m_rect10;
        if (xr < gr->right && xr >= gr->left && y < gr->bottom && y >= gr->top) {

        } else {
            RECT* wr = (&geom->m_planeCtx);
            if (xr < wr->right && xr >= wr->left && y < wr->bottom && y >= wr->top) {
                if (FindStartPointAt(sx, sy, &x, &y)) {
                    char tok = static_cast<char>(g_curPlayer);
                    w->m_cmdSubMgr->EnqueueSingle(
                        1,
                        tok,
                        0,
                        PLAYERCMD_PLACE_GRUNT,
                        static_cast<i16>(x),
                        static_cast<i16>(y),
                        0,
                        0
                    );
                    placed = 1;
                }
            }
        }
        if (placed == 0) {
            g_gameReg->m_cueSink->SpawnVoiceDriver(0, 0x340, -1, 1, -1, -1);
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
    if (m_playerCommandPending != 0) {
        goto drag_path;
    }
    {
        RECT* gr = &m_guts->m_rect10;
        if (xr < gr->right && xr >= gr->left && y < gr->bottom && y >= gr->top) {
            if (m_guts->SetFallRect(xr, y, static_cast<char>(m_cursorFrame))) {
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

        CGameLevel* ds = m_world->m_level;
        CDDrawWorkerHost* cam = ds->m_mainPlane;
        i32 wx = cam->m_viewRect.left - ds->m_planeCtx.left + xr;
        i32 wy = cam->m_viewRect.top - ds->m_planeCtx.top + y;
        i32 tok = static_cast<char>(m_cursorFrame);
        if (g_gameReg->m_cmdGrid->CellHitTest(wx, wy, &x, &y, tok) != NULL) {
            w->m_cmdSubMgr->EnqueueSingle(
                1,
                static_cast<char>(a),
                static_cast<char>(y),
                PLAYERCMD_GIVE_TOOL,
                0,
                0,
                static_cast<char>(tok),
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
        i32 col = 0;
        CGrunt* p = g_gameReg->m_cmdGrid->FindGruntAt(wx, wy, &span, &col, &y, &box);
        if (p == NULL || g_curPlayer != p->m_tileOwnerHi) {
            goto waypoint_cancel;
        }
        w->m_cmdSubMgr->EnqueueSingle(
            1,
            static_cast<char>(a),
            static_cast<char>(y),
            PLAYERCMD_GIVE_TOOL,
            0,
            0,
            static_cast<char>(tok),
            0
        );
        return 1;
    }

waypoint_cancel:
    m_dragInhibit2 = 0;
    m_guts->EnterHlRow(0, static_cast<char>(m_cursorFrame));
    SetCursorFrame(0);
    return 1;

drag_path: {

    static_cast<void>(y);
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

    RECT* gr = &m_guts->m_rect10;
    if (xr < gr->right && xr >= gr->left && y < gr->bottom && y >= gr->top) {
        FlushPendingOps();
        return m_guts->UpdateStatusBarTabHighlight(a, xr, y);
    }
    if (m_hitTest->HitTest(xr, y)) {
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

    if (m_levelId >= IDX(CURSOR_TOOL_HANDZ)) {
        CTriggerMgr* cg = g_gameReg->m_cmdGrid;
        CGrunt* slot = 0;
        if (1 == cg->m_recList.GetCount()) {

            i32* sel = static_cast<i32*>(cg->m_recList.GetHead());
            slot = cg->m_grid[sel[1] * 15 + sel[0]];
        }
        if (slot != NULL && slot->m_entranceCommitted != 0) {
            g_gameReg->m_cueSink->SpawnVoiceDriver(slot, 0x324, -1, 0, -1, -1);
        }
    }
    LoadCursorSprites(0, 0);
    i32 hit = m_guts->HitTest(xr, y);
    if (hit != -1) {
        m_guts->PlaceCursorTarget(hit, 0);
        return 1;
    }

    i32 slot38 = 0;
    CGrunt* picked =
        static_cast<CGrunt*>(m_mgr->m_cmdGrid->ScreenToCell(xr, y, &slot38, &slot38, 5));
    if (picked == NULL) {
        m_dragClampMaxX = xr;
        m_dragClampMaxY = y;
        m_hudRect.left = xr;
        m_hudRect.right = xr;
        m_hudRect.top = y;
        m_hudRect.bottom = y;
        m_worldReady = 1;
        goto ret1;
    }
    m_mgr->m_cmdGrid->ResetCell(slot38, slot38, g_spawnConfig->m_edgeKeys & 0x20, 0);
    if (a == g_curPlayer) {
        if (0 != (g_spawnConfig->m_edgeKeys & 0x20)) {
            goto ret1;
        }
        picked->OnStruck(1);
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
i32 CPlay::OnRButtonDblClk(i32 a, i32 b, i32 c) {
    return OnRButtonDown(a, b, c);
}

// @early-stop
RVA(0x000ceae0, 0x268)
i32 CPlay::OnRButtonDown(i32 a, i32 x, i32 y) {
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
        m_mgr->ReportError(IDX(CMD_TOGGLE_MUSIC), 0x458);
        return 1;
    }
    if (m_paused != 0) {
        m_paused = 0;
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, 0x816e, 0);
        return 1;
    }
    if (m_overlayDrag != 0) {
        return 1;
    }
    if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
        return 1;
    }
    if (m_mgr->m_frameGate != 0) {
        return 1;
    }
    if (m_lightFx != NULL && m_guts->m_position != STATUSBAR_HIDDEN
        && m_guts->m_activeTab != TAB_GAME) {
        if (m_lightFx->IssueMinimapCommand(a, x, y)) {
            return 1;
        }
    }

    if (x < m_guts->m_rect10.right && x >= m_guts->m_rect10.left && y < m_guts->m_rect10.bottom
        && y >= m_guts->m_rect10.top) {
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
    if (x < ph->m_planeCtx.right && x >= ph->m_planeCtx.left && y < ph->m_planeCtx.bottom
        && y >= ph->m_planeCtx.top) {
        CGameLevel* ds = m_world->m_level;
        CDDrawWorkerHost* geom = ds->m_mainPlane;
        i32 rawX = geom->m_viewRect.left - ds->m_planeCtx.left + x;
        i32 rawY = geom->m_viewRect.top - ds->m_planeCtx.top + y;
        i32 snapX = (rawX & ~0x1f) + 0x10;
        i32 snapY = (rawY & ~0x1f) + 0x10;
        m_tileClick.m_x = snapX;
        m_tileClick.m_y = snapY;
        CTriggerMgr* w = m_mgr->m_cmdGrid;
        if (w->m_overlay != NULL && w->m_overlay->m_active != 0) {
            w->OverlayTick();
            return 1;
        }
        w->ResetGroup(snapX, snapY, rawX, rawY, 1, 0, 1);
    }
    return 1;
}

// @early-stop
RVA(0x000d0b30, 0x200)
i32 CPlay::DrawCursorSaveUnder(CDDrawSurfacePair* pair) {
    i32 x = m_cursorX + m_cursorOffset.m_x;
    i32 y = m_cursorY + m_cursorOffset.m_y;

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

    dst->left = x - m_gridCurFrame->m_anchorX;
    dst->right = m_gridCurFrame->m_width + dst->left;
    dst->top = y - m_gridCurFrame->m_anchorY;
    dst->bottom = m_gridCurFrame->m_height + dst->top;
    if (dst->left < 0) {
        dst->left = 0;
    }
    if (dst->right > m_mgr->m_modeW) {
        dst->right = m_mgr->m_modeW;
    }
    if (dst->top < 0) {
        dst->top = 0;
    }
    if (dst->bottom > m_mgr->m_modeH) {
        dst->bottom = m_mgr->m_modeH;
    }
    src->right = dst->right - dst->left;
    src->bottom = dst->bottom - dst->top;

    CDDSurface* target = pair->m_surface;
    if (target == NULL) {
        return 0;
    }

    i32 r = half->BltFast(0, 0, target, dst, 0x10);
    if (r != 0) {
        CDDrawPtrCollections::GetErrorString(0, 0, r);
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

    m_gridCurFrame->RenderFrame(pair, x, y, 0);

    DDSCAPS caps;
    i32 flipping;
    if (target->m_ddSurface->GetCaps(&caps) == 0) {
        flipping = caps.dwCaps & DDSCAPS_FLIP;
    } else {
        flipping = 0;
    }
    if (flipping == 0) {
        m_inputHalfSel = m_inputHalfSel == 0;
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
    m_world->m_workerList->PruneWorkers(
        m_world->m_drawTarget->m_backPair,
        m_world->m_drawTarget->m_overlayPair
    );
    m_mgr->RefreshGameClock();
    return 1;
}

// @early-stop
RVA(0x000cba10, 0xb0)
i32 CPlay::RestoreDisplay() {
    if (IsActive() == 0) {
        return 0;
    }
    i32 savedW = m_mgr->m_savedModeW;
    i32 liveW = m_mgr->m_modeW;
    i32 savedH = m_mgr->m_savedModeH;
    i32 liveH = m_mgr->m_modeH;
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
            m_world->m_workerList->PruneWorkers(
                m_world->m_drawTarget->m_backPair,
                m_world->m_drawTarget->m_overlayPair
            );
        }
        m_world->m_drawTarget->m_frontPair->m_surface->Flip(0);
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
        if (g->m_position == STATUSBAR_HIDDEN) {
            g->RefreshState();
        }
        if (g->m_activeTab != TAB_GAME) {
            g->SetTabState(TAB_GAME, MENUITEM_SELECTED);
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
        if (g_gameReg->m_gameMode != GAMEMODE_MULTIPLAYER) {
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
        m_mgr->ChangeState(3);
    }
    return 1;
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
    HDC hdc = 0;
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
    if (m_levelIndex == 0x20) {
        m_completedFinalLevel = 1;
        m_notifyLatch = 1;

        CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
        if (reg->m_soundStream) {
            reg->m_soundStream->Stop();
        }
        m_mgr->m_sound->StopAndFlush();
        m_mgr->m_inputState->Teardown();
        m_mgr->m_cueSink->ClearSprites();
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, 0x8023, 0);
        return 1;
    }
    if (m_returnToMenuOnComplete) {
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, 0x8023, 0);
        return 1;
    }
    m_mgr->Post(m_levelIndex + 1);
    return 1;
}

// @early-stop
RVA(0x000d0120, 0x65c)
i32 CPlay::LoadCursorSprites(i32 frame, i32 flag) {
    // Callers build the id by arithmetic (`<grunt kind> + CURSOR_TOOL_HANDZ`, the
    // chip index, ...), so the cursor domain is entered here.
    ToolCursorId cursor = static_cast<ToolCursorId>(frame);
    if (this->m_levelId == frame && flag == this->m_dragEndNotify) {
        return 1;
    }
    if (cursor >= CURSOR_CHIP_FIRST && cursor <= CURSOR_CHIP_LAST) {
        if (this->BeginGridWalk("GAME_INGAMEICONZ_NORMCHIPZ", frame, 0, 0x64, 0) == 0) {
            return 0;
        }
        if (this->m_scrollSink != NULL) {
            this->m_scrollSink->m_flags |= 1;
        }
        this->m_dragClampMaxX = 0;
        this->m_dragClampMaxY = 0;
        this->m_dragInhibit2 = 1;
        this->m_dragEndNotify = 0;
        this->m_levelId = frame;
        return 1;
    }
    if (cursor == CURSOR_POINTER) {
        if (this->BeginGridWalk("GAME_CURSORZ_POINTER", 1, 1, 0x64, 0) == 0) {
            return 0;
        }
        if (this->m_scrollSink != NULL) {
            this->m_scrollSink->m_flags &= ~1;
        }
        this->m_dragClampMaxX = 0x10;
        this->m_dragClampMaxY = 0x10;
        this->m_dragEndNotify = 0;
        this->m_levelId = frame;
        return 1;
    }
    if (cursor == CURSOR_FLAILINGGRUNT) {
        if (this->BeginGridWalk("GAME_CURSORZ_FLAILINGGRUNT", 1, 1, 0x64, 1) == 0) {
            return 0;
        }
        if (this->m_scrollSink != NULL) {
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
    switch (cursor) {
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
    if (this->m_scrollSink != NULL) {
        this->m_scrollSink->m_flags |= 1;
    }
    this->m_dragClampMaxX = 0;
    this->m_dragClampMaxY = 0;
    this->m_dragEndNotify = flag;
    this->m_levelId = frame;
    return 1;
}

// @early-stop

DATA(0x0024c01c)
u8 g_scrollLoadFlags;

DATA(0x0024c270)
i32 g_scrollSpeedRange;

DATA(0x0024c274)
i32 g_scrollMinSpeed;

RVA(0x000d12b0, 0x2d5)
i32 CPlay::LoadScrollSpeedOptions() {
    if (!(g_scrollLoadFlags & 1)) {
        g_scrollLoadFlags |= 1;
        g_scrollMinSpeed = g_buteMgr.GetInt("Optionz", "MinScrollSpeed");
    }
    if (!(g_scrollLoadFlags & 2)) {
        g_scrollLoadFlags |= 2;

        i32 maxSpeed = g_buteMgr.GetInt("Optionz", "MaxScrollSpeed");
        g_scrollSpeedRange = maxSpeed - g_buteMgr.GetInt("Optionz", "MinScrollSpeed");
    }

    CPlay* self = this;
    CGruntzMgr* w = m_mgr;
    i32 changed = 0;
    CDDrawWorkerHost* g = w->m_world->m_level->m_mainPlane;

    i32 sx = g->m_snappedX;
    i32 sy = g->m_snappedY;
    i32 speed = static_cast<i32>(
        (static_cast<double>(w->m_scrollSpeed) * 0.01 * g_scrollSpeedRange + g_scrollMinSpeed)
    );

    SIZE
    extent;
    extent.cx = w->m_modeW;
    extent.cy = w->m_modeH;

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

// @early-stop
RVA(0x000d1650, 0x90)
void CPlay::DrawMessageFrame(i32 index, i32 useFront) {
    CObject* set_ob = 0;
    m_world->m_imageRegistry->m_10map.Lookup("GAME_MESSAGEZ", set_ob);
    CDDrawWorker* set = static_cast<CDDrawWorker*>(set_ob);
    if (set != NULL) {
        CImage* frame = set->GetAt(index);
        if (frame != NULL) {
            LevelCoordRect& vp = m_world->m_level->m_planeCtx;
            i32 cx = vp.left + (vp.right - vp.left) / 2;
            i32 cy = vp.top + (vp.bottom - vp.top) / 2;
            LayerBlitFrame(m_world, frame, cx, cy, useFront, 1);
        }
    }
}

RVA(0x000dc6d0, 0x2e0)
i32 CPlay::BuildGruntTypeNameTable(
    PickupType typeIdx,
    i32 mode,
    i32 lightGate,
    CMulti* finishGate
) {
    CString name("NORMALGRUNT");
    switch (typeIdx) {
        case GRUNT_BOMB:
            name = "BOMBGRUNT";
            break;
        case GRUNT_BOOMERANG:
            name = "BOOMERANGGRUNT";
            break;
        case GRUNT_BRICK:
            name = "BRICKGRUNT";
            break;
        case GRUNT_CLUB:
            name = "CLUBGRUNT";
            break;
        case GRUNT_GAUNTLETZ:
            name = "GAUNTLETZGRUNT";
            break;
        case GRUNT_GLOVEZ:
            name = "GLOVEZGRUNT";
            break;
        case GRUNT_GOOBER:
            name = "GOOBERGRUNT";
            break;
        case GRUNT_GRAVITYBOOTZ:
            name = "GRAVITYBOOTZGRUNT";
            break;
        case GRUNT_GUNHAT:
            name = "GUNHATGRUNT";
            break;
        case GRUNT_NERFGUN:
            name = "NERFGUNGRUNT";
            break;
        case GRUNT_ROCK:
            name = "ROCKGRUNT";
            break;
        case GRUNT_SHIELD:
            name = "SHIELDGRUNT";
            break;
        case GRUNT_SHOVEL:
            name = "SHOVELGRUNT";
            break;
        case GRUNT_SPRING:
            name = "SPRINGGRUNT";
            break;
        case GRUNT_SPY:
            name = "SPYGRUNT";
            break;
        case GRUNT_SWORD:
            name = "SWORDGRUNT";
            break;
        case GRUNT_TIMEBOMB:
            name = "TIMEBOMBGRUNT";
            break;
        case GRUNT_TOOB:
            name = "TOOBGRUNT";
            if (BuildAssetNamespacePrefixes(name, mode, lightGate, finishGate) == 0) {
                return 0;
            }
            name = "TOOBWATERGRUNT";
            return BuildAssetNamespacePrefixes(name, mode, lightGate, finishGate);
        case GRUNT_WAND:
            name = "WANDGRUNT";
            break;
        case GRUNT_WARPSTONE:
            name = "WARPSTONEGRUNT";
            break;
        case GRUNT_WELDER:
            name = "WELDERGRUNT";
            break;
        case GRUNT_WINGZ:
            name = "WINGZGRUNT";
            break;
        case GRUNT_BABYWALKER:
            name = "BABYWALKERGRUNT";
            break;
        case GRUNT_BEACHBALL:
            name = "BEACHBALLGRUNT";
            break;
        case GRUNT_BIGWHEEL:
            name = "BIGWHEELGRUNT";
            break;
        case GRUNT_GOKART:
            name = "GOKARTGRUNT";
            break;
        case GRUNT_JACKINTHEBOX:
            name = "JACKINTHEBOXGRUNT";
            break;
        case GRUNT_JUMPROPE:
            name = "JUMPROPEGRUNT";
            break;
        case GRUNT_POGOSTICK:
            name = "POGOSTICKGRUNT";
            break;
        case GRUNT_SCROLL:
            name = "SCROLLGRUNT";
            break;
        case GRUNT_SQUEAKTOY:
            name = "SQUEAKTOYGRUNT";
            break;
        case GRUNT_YOYO:
            name = "YOYOGRUNT";
            break;
        case GRUNT_HAREKRISHNA:
            name = "HAREKRISHNAGRUNT";
            break;
        case GRUNT_REAPER:
            name = "REAPERGRUNT";
            break;
    }
    return BuildAssetNamespacePrefixes(name, mode, lightGate, finishGate);
}

// @early-stop
RVA(0x000cfef0, 0xbc)
i32 CPlay::DrawStateMessage() {
    Present(0x3c);

    CObject* lookup_ob = 0;
    m_world->m_imageRegistry->m_10map.Lookup("GAME_MESSAGEZ", lookup_ob);
    CDDrawWorker* lookup = static_cast<CDDrawWorker*>(lookup_ob);
    CDDrawWorker* set = lookup;
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
    self->m_gruntzBank = static_cast<CSymTab*>(self->m_symParser->ResolvePath("GRUNTZ"));
    if (!self->m_gruntzBank) {
        return 0;
    }
    self->m_gameBank = static_cast<CSymTab*>(self->m_symParser->ResolvePath("GAME"));
    return self->m_gameBank != NULL;
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
    if (m_world == NULL) {
        return 0;
    }
    if (force == 0) {
        if (m_world->m_animRegistry->HasKeyPrefix("LEVEL") != 0) {
            return 1;
        }
    }
    m_world->m_animRegistry->RemoveKeysEqual("LEVEL", "_");
    void* e = m_levelBank->ResolvePath("ANIZ");
    if (e == NULL) {
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
    if ((static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))->HasKeyEqual("GAME")) {
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
    if ((static_cast<CDDrawSubMgrLeafScan*>(self->m_world->m_soundRegistry))->HasKeyEqual("GAME")) {
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

RVA(0x000dba30, 0x1ca)
i32 CPlay::BuildMusicCategoryTable(i32) {
    m_mgr->m_sound->StopAndFlush();

    CSymTab* levelSet = static_cast<CSymTab*>(m_levelBank->ResolvePath("MIDIZ"));
    if (levelSet) {
        CParseSource* e = levelSet->Insert("AMBIENT0", IDX(REZ_TAG_XMI));
        if (e) {
            void* res = e->BeginParse();
            if (res) {
                m_mgr->m_sound->CreateBank(res, e->m_length, "AMBIENT0");
            }
        }
        e = levelSet->Insert("AMBIENT1", IDX(REZ_TAG_XMI));
        if (e) {
            void* res = e->BeginParse();
            if (res) {
                m_mgr->m_sound->CreateBank(res, e->m_length, "AMBIENT1");
            }
        }
        e = levelSet->Insert("INTRO0", IDX(REZ_TAG_XMI));
        if (e) {
            void* res = e->BeginParse();
            if (res) {
                m_mgr->m_sound->CreateBank(res, e->m_length, "INTRO0");
            }
        }
        e = levelSet->Insert("INTRO1", IDX(REZ_TAG_XMI));
        if (e) {
            void* res = e->BeginParse();
            if (res) {
                m_mgr->m_sound->CreateBank(res, e->m_length, "INTRO1");
            }
        }
    }

    CSymTab* gameSet = static_cast<CSymTab*>(m_gameBank->ResolvePath("MIDIZ"));
    if (gameSet) {
        CParseSource* e = gameSet->Insert("POWERUP", IDX(REZ_TAG_XMI));
        if (e) {
            void* res = e->BeginParse();
            if (res) {
                m_mgr->m_sound->CreateBank(res, e->m_length, "POWERUP");
            }
        }
        e = gameSet->Insert("CURSE", IDX(REZ_TAG_XMI));
        if (e) {
            void* res = e->BeginParse();
            if (res) {
                m_mgr->m_sound->CreateBank(res, e->m_length, "CURSE");
            }
        }
        e = gameSet->Insert("MONOLITH", IDX(REZ_TAG_XMI));
        if (e) {
            void* res = e->BeginParse();
            if (res) {
                m_mgr->m_sound->CreateBank(res, e->m_length, "MONOLITH");
            }
        }
    }
    return 1;
}

RVA(0x000dbc80, 0x309)
i32 CPlay::BuildWorldLevelPath(i32 unused) {
    m_world->m_level->ReleaseChildren();
    if (m_mgr->m_strWorldFile.GetLength() != 0) {
        if (m_mgr->m_isBattlezLevel != 0) {
            CString key = "BATTLEZ\\" + m_mgr->GetWorldFileName();
            CParseSource* node = m_gameBank->ResolveQualified(key, REZ_TAG_WWD);
            if (node == NULL) {
                return 0;
            }
            if (m_world->m_level->LoadFromSource(node) == 0) {
                return 0;
            }
        } else if (m_mgr->m_isMultiLevel != 0) {
            CString key = "MULTI\\" + m_mgr->GetWorldFileName();
            CParseSource* node = m_gameBank->ResolveQualified(key, REZ_TAG_WWD);
            if (node == NULL) {
                return 0;
            }
            if (m_world->m_level->LoadFromSource(node) == 0) {
                return 0;
            }
        } else {
            if (m_world->m_level->LoadFromFile(m_mgr->GetWorldFileName()) == 0) {
                return 0;
            }
        }
    } else {
        CString key;
        i32 sel = m_levelIndex;
        if (g_levelBias100 != 0) {
            sel += 0x64;
        }
        if (sel > 0x24 && sel <= 0x28) {
            key.Format("WORLDZ\\TRAINING%i", sel % 0x24);
        } else {
            key.Format("WORLDZ\\LEVEL%i", sel);
        }
        CParseSource* node = m_levelBank->ResolveQualified(key, REZ_TAG_WWD);
        if (node == NULL) {
            return 0;
        }
        if (m_world->m_level->LoadFromSource(node) == 0) {
            return 0;
        }
    }
    m_world->m_level->NotifyAllPlanes();
    m_world->m_level->m_flags |= 4;
    g_backView = m_world->m_level->FindPlaneByName("BACK");
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

// @early-stop
RVA(0x000dca70, 0x4a4)
i32 CState::BuildAssetNamespacePrefixes(
    const CString& name,
    i32 mode,
    i32 lightGate,
    CMulti* finishGate
) {
    i32 result;
    if (mode != GAMEMODE_NONE) {
        if (m_world->m_imageRegistry->HasKeyEqual("GRUNTZ_" + name) == 0) {
            g_gameReg->m_cueSink->PauseAllVoices();
            (static_cast<CTriggerMgr*>(g_gameReg->m_cmdGrid))->DestroyAllAnims();
            if (lightGate != 0) {
                CString cs;
                cs.LoadString(0x819b);
                RECT r = *(&g_gameReg->m_world->m_level->m_planeCtx);
                RECT r2;
                CopyRect(&r2, &r);
                EngStr_DrawText(g_gameReg->m_world, &cs, &r2, 0x82, 1, 0xff, 0xff, 0, 1);
            }
            g_resourceInstallActive = 1;
            void* tree = m_gruntzBank->ResolvePath("IMAGEZ_" + name);
            if (tree == NULL) {
                result = 0;
                goto done;
            }
            m_world->m_imageRegistry->InstallTree(tree, "GRUNTZ_" + name, "_");
            g_resourceInstallActive = 0;
            if (finishGate != NULL) {
                finishGate->AckJoinFailure();
            }
        }
        if (m_world->m_soundRegistry->HasKeyEqual("GRUNTZ_" + name) == 0) {
            void* tree = m_gruntzBank->ResolvePath("SOUNDZ_" + name);
            if (tree != NULL) {

                m_world->m_soundRegistry
                    ->ScanTree(static_cast<CSymTab*>(tree), "GRUNTZ_" + name, "_");
            }
        }
        if (m_world->m_animRegistry->HasKeyPrefix("GRUNTZ_" + name) == 0) {
            void* tree = m_gruntzBank->ResolvePath("ANIZ_" + name);
            if (tree == NULL) {
                result = 0;
                goto done;
            }
            m_world->m_animRegistry->ScanTree(static_cast<CSymTab*>(tree), "GRUNTZ_" + name, "_");
        }
        result = 1;
        goto done;
    }

    if (m_world->m_imageRegistry->HasKeyEqual("GRUNTZ_" + name) != 0) {
        m_world->m_imageRegistry->RemoveKeysEqual("GRUNTZ_" + name, "_");
        if (finishGate != NULL) {
            finishGate->AckJoinFailure();
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
        self->m_world->m_animRegistry->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_DEATHZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!self->m_world->m_animRegistry->HasKeyPrefix("GRUNTZ_ENTRANCEZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_ENTRANCEZ");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_ENTRANCEZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!self->m_world->m_animRegistry->HasKeyPrefix("GRUNTZ_EXITZ")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_EXITZ");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_EXITZ", "_");
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
        self->m_world->m_animRegistry->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_PICKUPS", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!self->m_world->m_animRegistry->HasKeyPrefix("GRUNTZ_BOMBGRUNT")) {
        void* s = (self->m_gruntzBank)->ResolvePath("ANIZ_BOMBGRUNT");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry->ScanTree(static_cast<CSymTab*>(s), "GRUNTZ_BOMBGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    return 1;
}

RVA(0x000c8a10, 0x119)
i32 CPlay::EnterState(GameStateId mode) {
    POINT pt;
    GetCursorPos(&pt);
    m_cursorX = pt.x;
    m_cursorY = pt.y;
    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    if (mode == GAMESTATE_HELP) {
        g_frameTime = m_savedClock;
        if (!EnterMode(9)) {
            return 0;
        }
        m_stepCountdown = 2;
    } else if (m_renderDisabled == 0 || m_mgr->m_gameMode == GAMEMODE_MULTIPLAYER) {
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
        if (mode != GAMESTATE_HELP) {
            (static_cast<CWorldSoundSet*>(m_mgr->m_inputState))->Resume();
        }
        (static_cast<CTriggerMgr*>(m_mgr->m_cmdGrid))->DestroyAllAnims();
        (static_cast<CGruntSpawnConfig*>(m_mgr->m_cueSink))->PauseAllVoices();
    }
    return 1;
}

RVA(0x000d5f90, 0xd7)
i32 CPlay::FindStartPointAt(i32 x, i32 y, i32* outX, i32* outY) {

    i32 id = g_curPlayer;
    GruntzPlayer* slot = &g_gameReg->m_options[id];

    if (slot != NULL && g_gameReg->m_cmdGrid->m_rowCount[id] < slot->m_comboSel) {
        i32 i = 0;
        if (i < StartMarkerCount()) {
            do {
                Coord* m = StartMarkerAt(i);
                if (m != NULL) {
                    RECT rc;
                    SetRect(&rc, m->m_x - 0x20, m->m_y - 0x20, m->m_x + 0x20, m->m_y + 0x20);
                    if (x < rc.right && x >= rc.left && y < rc.bottom && y >= rc.top) {
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

RVA(0x000d60b0, 0x2cd)
i32 CPlay::ResetPlayState() {
    char buf[0x40];
    if (m_mgr->m_musicEnabled != 0 && g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
        m_ambientInterval = AMBIENT_INTRO_INTERVAL_MS;
        m_ambientIntervalHi = 0;
        m_ambientTimerLo = g_frameTime;
        m_ambientTimerHi = 0;
        wsprintfA(buf, "INTRO%d", GetAmbientId());
        if (g_gameReg->m_musicEnabled != 0) {
            m_mgr->m_sound->PlayByName(buf, 0);
        }
        m_ambientInitDone = 0;
    } else {
        wsprintfA(buf, "AMBIENT%d", GetAmbientId());
        CGruntzSoundZ* snd = m_mgr->m_sound;
        CGruntzSoundInnerZ* h = snd->FindBank(buf);
        if (h != NULL) {
            snd->m_pCurrent = h;
        }
        if (m_mgr->m_sound->m_pCurrent != NULL) {
            m_mgr->m_sound->m_pCurrent->SetLoop(1);
        }
        CGruntzMgr* reg = g_gameReg;
        if (reg->m_musicEnabled != 0 && reg->m_gameMode == GAMEMODE_REPLAY) {
            m_mgr->m_sound->PlayByName(buf, 1);
        }
        m_ambientTimerLo = 0;
        m_ambientInterval = 0;
        m_ambientTimerHi = 0;
        m_ambientIntervalHi = 0;
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
                    (static_cast<CSaveGame*>(reg->m_saveSink))->SetMaxLevel(id);
                    reg = g_gameReg;
                }
            }
            (static_cast<CSaveGame*>(reg->m_saveSink))->Save(0, 0x81a6);
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
        m_scrollSink->m_stateFlags &= ~1;
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
            fm->m_startStamp.m_v = static_cast<u32>(g_frameTime);
            fm->m_accum.m_v = static_cast<u32>(fm->m_currentMs);
            fm->m_baseTime.m_v = static_cast<u32>(g_frameTime);
            fm->m_running = 1;
        } else {
            fm->m_startStamp.m_v = static_cast<u32>(g_frameTime);
        }
    }
    CTriggerMgr* tl = m_mgr->m_cmdGrid;
    tl->m_countdownActive = 1;
    tl->m_phase = 0;
    tl->m_pendingFxKind = 0;
    tl->m_gooTimerBaseLo = 0;
    tl->m_gooIntervalLo = 0;
    tl->m_gooTimerBaseHi = 0;
    tl->m_gooIntervalHi = 0;
    tl->m_resourceTimerBaseLo = 0;
    tl->m_resourceIntervalLo = 0;
    tl->m_resourceTimerBaseHi = 0;
    tl->m_resourceIntervalHi = 0;
    tl->m_finishReasonFrame = 0;
    tl->m_rollingballWanted = 0;
    tl->m_teleportWanted = 0;
    tl->m_groupFlag = 1;
    return 1;
}

DATA(0x002455f0)
i32 g_levelBias100 = 0;

DATA(0x00245270)
GruntDeathType g_areaPitDeath;

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
        m_mgr->m_cmdGrid->ClearGridRange(5);
    }
    ForwardReady();
    {

        CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
        if (reg->m_soundStream != NULL) {
            reg->m_soundStream->Stop();
        }
    }
    m_mgr->m_sound->StopAndFlush();
    m_mgr->m_inputState->Teardown();
    m_mgr->m_cueSink->ClearSprites();
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
        void* node = CameraBookmarkData()[i];
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
        void* node = CameraBookmarkData()[i];
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

RVA(0x000d6fa0, 0x1fa)
i32 CPlay::EnterMode(i32 mode) {
    (g_gameReg)->CheckSavedMode();
    m_guts->Deactivate();
    m_guts->LoadDestructButtonSprite(0);
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
    RetireScene(0x50, 0x3e8, 0, 1);

    CGameLevel* lvl = m_world->m_level;
    if (lvl->m_mainPlane != NULL) {
        lvl->m_mainPlane->DeactivateDistantObjects();
    }
    m_mgr->RefreshGameClock();
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
RVA(0x000d5960, 0x160)
i32 CPlay::AddLevelGruntz() {
    CObList& chain = m_world->m_childGroup->m_list;
    POSITION pos = chain.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* g = static_cast<CGameObject*>(chain.GetNext(pos));
        if (g == NULL) {
            continue;
        }
        if (static_cast<void*>(g->m_animWorker->m_notify)
            != static_cast<void*>(CreateGruntStartingPoint)) {
            continue;
        }
        if (g->m_smarts == g_curPlayer) {
            continue;
        }
        i32 x = ((g->m_screenX & ~0x1f) + 0x10);
        i32 y = ((g->m_screenY & ~0x1f) + 0x10);

        i32 r = m_mgr->m_cmdGrid->PlaceObject(
            g->m_smarts,
            y,
            x,
            0x186a0,
            0,
            g->m_score,
            g->m_powerup,
            g->m_damage,
            g->m_points,
            g->m_direction,
            g->m_animWorker->m_minX,
            g->m_animWorker->m_maxX,
            // Byte-evidenced kind-dependent ABI slot.

            reinterpret_cast<i32>(&g->m_extent.left)
        );
        if (r == -1) {
            CString msg;
            msg.Format("Could not add Grunt: Player=%d", g->m_smarts, y, x);

            (g_gameReg)->EnterModalUI(msg);
            return 0;
        }
        g->m_flags |= 0x10000;
    }
    return 1;
}

RVA(0x000dd050, 0x24b)
i32 CPlay::BuildGruntNamespaceList(CMulti* arg) {
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
i32 CPlay::BuildWarlordNameTable(CMulti* arg) {
    for (i32 id = IDX(GRUNT_BOOMERANG); id <= IDX(GRUNT_YOYO); id++) {
        if (!BuildGruntTypeNameTable(static_cast<PickupType>(id), 0, 0, 0)) {
            return 0;
        }
    }
    if (!BuildGruntTypeNameTable(GRUNT_HAREKRISHNA, 0, 0, arg)) {
        return 0;
    }
    if (!BuildGruntTypeNameTable(GRUNT_REAPER, 0, 0, arg)) {
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
    if (!head) {
        return 0;
    }
    CObList& chain = this->m_world->m_childGroup->m_list;
    POSITION pos = chain.GetHeadPosition();
    while (pos != NULL) {
        CGameObject* obj = static_cast<CGameObject*>(chain.GetNext(pos));
        if (obj) {
            void* marker = static_cast<void*>(obj->m_animWorker->m_notify);
            if (marker == static_cast<void*>(CreateGruntStartingPoint)) {
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
                switch (obj->m_points) {
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
            } else if (marker == static_cast<void*>(CreateInGameIcon)) {
                i32 cv = obj->m_smarts == PICKUP_MEGAPHONE ? obj->m_points : obj->m_smarts;
                if (cv >= 1 && cv <= 0x16 && cv != 0x14) {
                    m_mgr->m_scoreHud->m_toolzAvailable++;
                } else if (cv >= 0x17 && cv <= 0x20) {
                    m_mgr->m_scoreHud->m_toyzAvailable++;
                } else if (cv >= 0x36 && cv <= 0x3c) {
                    m_mgr->m_scoreHud->m_powerupzAvailable++;
                } else if (cv == 0x50) {
                    m_mgr->m_scoreHud->m_coinsAvailable++;
                }
                i32 d = obj->m_smarts;
                if (d <= 0x20) {
                    if (!BuildGruntTypeNameTable(static_cast<PickupType>(d), 1, 0, ctx)) {
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
                } else if (d == 0x55 || d == 0x32) {
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
            } else if (marker == static_cast<void*>(CreateCoveredPowerup)
                       || marker == static_cast<void*>(CreateGiantRock)) {
                i32 cv = obj->m_powerup == 0x32 ? obj->m_points : obj->m_powerup;
                if (cv >= 1 && cv <= 0x16 && cv != 0x14) {
                    m_mgr->m_scoreHud->m_toolzAvailable++;
                } else if (cv >= 0x17 && cv <= 0x20) {
                    m_mgr->m_scoreHud->m_toyzAvailable++;
                } else if (cv >= 0x36 && cv <= 0x3c) {
                    m_mgr->m_scoreHud->m_powerupzAvailable++;
                } else if (cv == 0x50) {
                    m_mgr->m_scoreHud->m_coinsAvailable++;
                }
                i32 e = obj->m_powerup;
                if (e <= 0x20) {
                    if (!BuildGruntTypeNameTable(static_cast<PickupType>(e), 1, 0, ctx)) {
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
                } else if (e == 0x55 || e == 0x32) {
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

// @early-stop
RVA(0x000dc060, 0x51b)
i32 CPlay::SetEffectSpriteDurations() {
    LeafCue* d;
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "GAME_PYRAMIDMOVE", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "GAME_TELEPORTEROPEN", d);
    if (d != NULL) {
        d->m_replayDelay = 1000;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "GAME_TELEPORTERCLOSE", d);
    if (d != NULL) {
        d->m_replayDelay = 1000;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "GAME_TELEPORTERALL", d);
    if (d != NULL) {
        d->m_replayDelay = 4000;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "GAME_BRICKBREAK", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "LEVEL_DEATHBRIDGEMOVE", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "LEVEL_WATERBRIDGEMOVE", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "LEVEL_ROCKBREAK", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "LEVEL_LAVAGEYSER", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "LEVEL_TRAPDOORCLOSE", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "LEVEL_TRAPDOOROPEN", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "LEVEL_CANDLEIGNITE", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "LEVEL_CANDLEUP", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "LEVEL_CANDLEDOWN", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "LEVEL_GOLFBALLAIR2", d);
    if (d != NULL) {
        d->m_replayDelay = 250;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "LEVEL_GOLFBALLHOLE", d);
    if (d != NULL) {
        d->m_replayDelay = 250;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "LEVEL_GOLFBALLSINK", d);
    if (d != NULL) {
        d->m_replayDelay = 250;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "GAME_EXPLOSION1", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "LEVEL_OUTLETHAZARD", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "GRUNTZ_DEATHZ_DEATHZFREEZE1A", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "GRUNTZ_DEATHZ_DEATHZFREEZE2A", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "GRUNTZ_DEATHZ_DEATHZUNFREEZE1A", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "GRUNTZ_DEATHZ_DEATHZUNFREEZE1A", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "GRUNTZ_DEATHZ_RESSURECT", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "GRUNTZ_DEATHZ_DEATHZSQUASH1A", d);
    if (d != NULL) {
        d->m_replayDelay = 100;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "LEVEL_CLOUDHAZARDMOVE", d);
    if (d != NULL) {
        d->m_replayDelay = 10000;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "LEVEL_CLOUDHAZARDKILL", d);
    if (d != NULL) {
        d->m_replayDelay = 3000;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "GRUNTZ_DEATHZ_DEATHZELECTROCUTE1A", d);
    if (d != NULL) {
        d->m_replayDelay = 1000;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "GRUNTZ_NERFGUNGRUNT_NERFGUNZGRUNTP1AS1", d);
    if (d != NULL) {
        d->m_replayDelay = 1000;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "GRUNTZ_GUNHATGRUNT_GUNHATGRUNTP1AS1", d);
    if (d != NULL) {
        d->m_replayDelay = 1000;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "GRUNTZ_WELDERGRUNT_WELDERZGRUNTP1AS1", d);
    if (d != NULL) {
        d->m_replayDelay = 1000;
    }
    d = NULL;
    MapLookup(m_world->m_soundRegistry->m_cues, "LEVEL_PLANEHAZARDFLY", d);
    if (d != NULL) {
        d->m_replayDelay = 5000;
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
// @early-stop
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
        strcat(buf, " rcMove ");
    }
    if (group->m_flags & 0x20000) {
        strcat(buf, " rcAttack ");
    }
    if (group->m_flags & 0x40000) {
        strcat(buf, " rcHit ");
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
    if (g_debugDisplayFlags & 0x200) {
        sprintf(scratch, " FpsLimit = %i ", m_mgr->m_pacingGate);
        strcat(buf, scratch);
    }

    CDDSurface* host = m_world->m_drawTarget->m_backPair->m_surface;
    HDC hdc = 0;
    host->m_ddSurface->GetDC(&hdc);
    if (hdc == NULL) {
        return;
    }
    SetBkMode(hdc, 1);
    SetTextColor(hdc, 0xffffff);
    SetBkColor(hdc, 0);
    PostSetup(hdc);

    RECT* src = &m_world->m_level->m_planeCtx;
    RECT lr;
    lr.left = src->left;
    lr.top = src->top;
    lr.right = src->right;
    lr.bottom = src->bottom;
    RECT dr;
    dr.left = lr.left;
    dr.top = lr.bottom - 0x1c;
    dr.right = lr.right;
    dr.bottom = lr.bottom;
    DrawTextA(hdc, buf, -1, &dr, 0x20);

    if (g_debugDisplayFlags & 0x8) {
        SetBkMode(hdc, 2);
        if (g_brickText1.GetLength() != 0) {
            TextOutA(hdc, 0, 0x00, g_brickText1, g_brickText1.GetLength());
        }
        if (g_brickText2.GetLength() != 0) {
            TextOutA(hdc, 0, 0x10, g_brickText2, g_brickText2.GetLength());
        }
        if (g_str64552c.GetLength() != 0) {
            TextOutA(hdc, 0, 0x20, g_str64552c, g_str64552c.GetLength());
        }
        if (g_str645530.GetLength() != 0) {
            TextOutA(hdc, 0, 0x30, g_str645530, g_str645530.GetLength());
        }
        if (g_str645514.GetLength() != 0) {
            TextOutA(hdc, 0, 0x40, g_str645514, g_str645514.GetLength());
        }
        if (g_str645518.GetLength() != 0) {
            TextOutA(hdc, 0, 0x50, g_str645518, g_str645518.GetLength());
        }
        if (g_str64551c.GetLength() != 0) {
            TextOutA(hdc, 0, 0x60, g_str64551c, g_str64551c.GetLength());
        }
        if (g_str645520.GetLength() != 0) {
            TextOutA(hdc, 0, 0x70, g_str645520, g_str645520.GetLength());
        }
    }
    host->m_ddSurface->ReleaseDC(hdc);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
DATA(0x0024c020)
char g_customLevelText[0x200];

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
    HDC hdc = 0;
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
