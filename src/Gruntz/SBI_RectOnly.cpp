#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/StreamFeeder.h>
#include <Enums.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameMenuMgrBuilders.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/SbGeom.h>
#include <Gruntz/SBI_GruntMachine.h>
#include <Gruntz/SBI_ImageSet.h>
#include <Gruntz/SBI_ImageSetAni.h>
#include <Gruntz/SBI_MenuItem.h>
#include <Gruntz/SBI_SideTab.h>
#include <Gruntz/SBI_WarlordHead.h>
#include <Gruntz/SBI_WellGoo.h>
#include <Gruntz/SbiBeltPhase.h>
#include <Gruntz/SbiCommandId.h>
#include <Gruntz/SbiHlRowState.h>
#include <Gruntz/SbiMachineState.h>
#include <Gruntz/SbiMenuItemState.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarItem.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarMgrBuilders.h>
#include <Gruntz/StatusBarTab.h>
#include <Gruntz/StatusBarTabWidgets.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/Timer.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WarpStoneFly.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <Rez/RezList.h>
#include <Rez/RezMgr.h>
#include <Utils/MapTyped.h>
#include <Utils/MfcTyped.h>
#include <Utils/RegistryHelper.h>
#include <Wap32/ScreenGeometry.h>

#include <limits.h>
#include <math.h>
#include <new>
#include <stddef.h>
#include <string.h>

DATA(0x00244c54)
i32 g_curPlayer = 0;

// @early-stop
// The three cumulative runs are settled (retail restarts the running total at [3],
// [7] and [17]).  What is left is the rotating temp register of the accumulate chain:
// retail cycles edx/ebp/ecx where cl cycles ebp/ecx/edx, one phase out of step.
RVA(0x000fdc00, 0x5c2)
i32 CStatusBarMgr::LoadBattlezItemConfig(CDDrawSurfaceMgr* world) {
    m_world = world;
    m_restorePosition = STATUSBAR_DOCK_RIGHT;
    m_position = STATUSBAR_DOCK_RIGHT;
    i32 vx = g_gameReg->m_modeSize.cx;
    i32 vy = g_gameReg->m_modeSize.cy;
    SetRect(&m_barRect, vx - 0xa0, 0, vx, SCREEN_H_PX);
    m_redrawFrames = 0;
    m_barX = vx - 0x45;
    m_barY = vy - 0x30;
    m_itemKind = GAME_TAB_MENU;
    m_tabCycle = g_curPlayer;
    Reset();
    if (BuildStatusBarTabs() == 0) {
        return 0;
    }
    m_activeSlot = -1;
    m_pendingHlRow = STATUS_HL_ROW_NONE;
    m_rezActive = 0;
    m_rezTick = 0;
    m_levelOverlayActive = 0;
    m_quitConfirmationActive = 0;
    m_battlezPct[0] = g_buteMgr.GetInt("Multiplayer", "ToolzPercent");
    m_battlezPct[1] = m_battlezPct[0] + g_buteMgr.GetInt("Multiplayer", "ToyzPercent");
    m_battlezPct[2] = m_battlezPct[1] + g_buteMgr.GetInt("Multiplayer", "BrickzPercent");
    m_battlezPct[3] = g_buteMgr.GetInt("Multiplayer", "RedBrick");
    m_battlezPct[4] = m_battlezPct[3] + g_buteMgr.GetInt("Multiplayer", "BlueBrick");
    m_battlezPct[5] = m_battlezPct[4] + g_buteMgr.GetInt("Multiplayer", "GoldBrick");
    m_battlezPct[6] = m_battlezPct[5] + g_buteMgr.GetInt("Multiplayer", "BlackBrick");
    m_battlezPct[7] = g_buteMgr.GetInt("Multiplayer", "BabyWalkerz");
    m_battlezPct[8] = m_battlezPct[7] + g_buteMgr.GetInt("Multiplayer", "BeachBallz");
    m_battlezPct[9] = m_battlezPct[8] + g_buteMgr.GetInt("Multiplayer", "BigWheelz");
    m_battlezPct[10] = m_battlezPct[9] + g_buteMgr.GetInt("Multiplayer", "GoKartz");
    m_battlezPct[11] = m_battlezPct[10] + g_buteMgr.GetInt("Multiplayer", "JackInTheBoxz");
    m_battlezPct[12] = m_battlezPct[11] + g_buteMgr.GetInt("Multiplayer", "JumpRopez");
    m_battlezPct[13] = m_battlezPct[12] + g_buteMgr.GetInt("Multiplayer", "PogoStickz");
    m_battlezPct[14] = m_battlezPct[13] + g_buteMgr.GetInt("Multiplayer", "Scrollz");
    m_battlezPct[15] = m_battlezPct[14] + g_buteMgr.GetInt("Multiplayer", "SqueakToyz");
    m_battlezPct[16] = m_battlezPct[15] + g_buteMgr.GetInt("Multiplayer", "Yoyoz");
    m_battlezPct[17] = g_buteMgr.GetInt("Multiplayer", "Bombz");
    m_battlezPct[18] = m_battlezPct[17] + g_buteMgr.GetInt("Multiplayer", "Boomerangz");
    m_battlezPct[19] = m_battlezPct[18] + g_buteMgr.GetInt("Multiplayer", "Brickz");
    m_battlezPct[20] = m_battlezPct[19] + g_buteMgr.GetInt("Multiplayer", "Clubz");
    m_battlezPct[21] = m_battlezPct[20] + g_buteMgr.GetInt("Multiplayer", "Gauntletz");
    m_battlezPct[22] = m_battlezPct[21] + g_buteMgr.GetInt("Multiplayer", "Glovez");
    m_battlezPct[23] = m_battlezPct[22] + g_buteMgr.GetInt("Multiplayer", "Gooberz");
    m_battlezPct[24] = m_battlezPct[23] + g_buteMgr.GetInt("Multiplayer", "GravityBootz");
    m_battlezPct[25] = m_battlezPct[24] + g_buteMgr.GetInt("Multiplayer", "GunHatz");
    m_battlezPct[26] = m_battlezPct[25] + g_buteMgr.GetInt("Multiplayer", "NerfGunz");
    m_battlezPct[27] = m_battlezPct[26] + g_buteMgr.GetInt("Multiplayer", "Rockz");
    m_battlezPct[28] = m_battlezPct[27] + g_buteMgr.GetInt("Multiplayer", "Shieldz");
    m_battlezPct[29] = m_battlezPct[28] + g_buteMgr.GetInt("Multiplayer", "Shovelz");
    m_battlezPct[30] = m_battlezPct[29] + g_buteMgr.GetInt("Multiplayer", "Springz");
    m_battlezPct[31] = m_battlezPct[30] + g_buteMgr.GetInt("Multiplayer", "Spyz");
    m_battlezPct[32] = m_battlezPct[31] + g_buteMgr.GetInt("Multiplayer", "Swordz");
    m_battlezPct[33] = m_battlezPct[32] + g_buteMgr.GetInt("Multiplayer", "TimeBombz");
    m_battlezPct[34] = m_battlezPct[33] + g_buteMgr.GetInt("Multiplayer", "Toobz");
    m_battlezPct[35] = m_battlezPct[34] + g_buteMgr.GetInt("Multiplayer", "Wandz");
    m_battlezPct[36] = m_battlezPct[35] + g_buteMgr.GetInt("Multiplayer", "Welderz");
    m_battlezPct[37] = m_battlezPct[36] + g_buteMgr.GetInt("Multiplayer", "Wingz");
    SetTabState(SBICMD_TAB_GAME, MENUITEM_SELECTED);
    if ((static_cast<Utils::RegistryHelper*>(g_gameReg->m_settings))
            ->GetValueDword("StatusBar Position", 0)
        == 1) {
        DockStatusBarLeft();
    }
    return 1;
}

RVA(0x000fe350, 0x6d)
void CStatusBarMgr::Teardown() {
    (static_cast<Utils::RegistryHelper*>(g_gameReg->m_settings))
        ->SetValueDword("StatusBar Position", IDX(m_position));
    ResetWidgets(0);
    for (i32 i = 0; i < m_rewardQueue.GetSize(); i++) {
        Coord* p = static_cast<Coord*>(m_rewardQueue.GetData()[i]);
        if (p) {
            CoordPoolNode* node = g_coordPool.NodeOf(p);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }

    m_rewardQueue.SetSize(0, -1);
}

RVA(0x000fe3e0, 0x55)
i32 CStatusBarMgr::SetState(StatusBarDock state) {
    if (m_hlBusy != 0) {
        return 1;
    }
    StatusBarDock old = m_position;
    if (old == state) {
        return 1;
    }
    if (state == STATUSBAR_HIDDEN) {
        if (Activate() == 0) {
            return 0;
        }
        m_restorePosition = m_position;
    } else {
        Deactivate();
    }
    old = m_position;
    m_position = state;
    (static_cast<CPlay*>(g_gameReg->m_curState))->PositionBridgeToggle(state, old);
    return 1;
}

RVA(0x000fe460, 0x83)
i32 CStatusBarMgr::DockStatusBarLeft() {
    if (m_hlBusy == 0 && m_position != STATUSBAR_DOCK_LEFT) {
        ResetWidgets(1);
        SetRect(&m_barRect, 0, 0, 0xa0, SCREEN_H_PX);
        SetState(STATUSBAR_DOCK_LEFT);
        (static_cast<CPlay*>(g_gameReg->m_curState))->ResetViewport();
        if (BuildStatusBarTabs() == 0) {
            g_gameReg->ReportError(kActivateErrId, 0x448);
            return 0;
        }
        SetTabState(static_cast<SbiCommandId>(IDX(m_activeTab)), MENUITEM_SELECTED);
    }
    return 1;
}

RVA(0x000fe520, 0xa9)
i32 CStatusBarMgr::DockStatusBarRight() {
    if (m_hlBusy != 0) {
        return 1;
    }
    if (m_position == STATUSBAR_DOCK_RIGHT) {
        return 1;
    }
    ResetWidgets(1);

    tagSIZE screenSize = g_gameReg->m_modeSize;
    SetRect(&m_barRect, screenSize.cx - 0xa0, 0, screenSize.cx, SCREEN_H_PX);
    SetState(STATUSBAR_DOCK_RIGHT);
    (static_cast<CPlay*>(g_gameReg->m_curState))->ResetViewport();
    if (BuildStatusBarTabs() == 0) {
        g_gameReg->ReportError(kActivateErrId, 0x449);
        return 0;
    }
    SetTabState(static_cast<SbiCommandId>(IDX(m_activeTab)), MENUITEM_SELECTED);
    return 1;
}

RVA(0x000fe600, 0x49)
i32 CStatusBarMgr::HideRect() {
    if (m_hlBusy == 0 && m_position != STATUSBAR_HIDDEN) {
        ResetWidgets(1);
        SetRect(&m_barRect, -1, -1, -1, -1);
        SetState(STATUSBAR_HIDDEN);
        (static_cast<CPlay*>(g_gameReg->m_curState))->ResetViewport();
    }
    return 1;
}

RVA(0x000fe670, 0x2b)
i32 CStatusBarMgr::RestoreStatusBar() {
    if (m_hlBusy != 0) {
        return 1;
    }
    if (m_position != STATUSBAR_HIDDEN) {
        return 1;
    }
    if (m_restorePosition == STATUSBAR_DOCK_LEFT) {
        return DockStatusBarLeft();
    }
    return DockStatusBarRight();
}

// @early-stop
// One register-choice rotation; flips to exact under TU-state (hist 99.82 at
// the same fingerprint, reproduced by leading-declaration probe states).
RVA(0x000fe6b0, 0x145)
i32 CStatusBarMgr::LoadMainStatusBarSprite() {
    if (m_position != STATUSBAR_HIDDEN) {
        if (m_redrawFrames > 0) {
            m_redrawFrames--;
            i32 v = m_barFrameGate;
            if (v > SCREEN_H_PX) {
                CDDSurface* tgt = (g_gameReg->m_world->m_drawTarget)->m_backPair->m_surface;

                RECT below;
                below.left = m_barRect.left;
                below.top = m_barRect.bottom;
                below.right = m_barRect.right;
                below.bottom = v;
                tgt->Restore(&below, 0);
            }
            CMapStringToOb* map = &m_world->m_imageRegistry->m_workersByName;
            CObject* found = NULL;

            map->Lookup("GAME_STATUSBAR_MAINBAR", found);
            if (found) {

                CDDrawWorker* cfg = static_cast<CDDrawWorker*>(found);
                CImage* entry = static_cast<CImage*>(cfg->m_items.GetAt(cfg->m_minIndex));
                if (entry) {
                    CDDrawSubMgrPages* l1 = g_gameReg->m_world->m_drawTarget;
                    entry->RenderFrame(
                        l1->m_backPair,
                        entry->m_anchorX + m_barRect.left,
                        entry->m_anchorY + m_barRect.top,
                        0
                    );
                }
            }
        }

        POSITION n = m_tabLists[0].GetHeadPosition();
        while (n) {
            CStatusBarItem* cur = static_cast<CStatusBarItem*>(m_tabLists[0].GetNext(n));
            if (cur) {
                cur->Render();
            }
        }
        CPtrList& tab = m_tabLists[IDX(m_activeTab)];
        POSITION m = tab.GetHeadPosition();
        while (m) {
            CStatusBarItem* cur = static_cast<CStatusBarItem*>(tab.GetNext(m));
            if (cur) {
                cur->Render();
            }
        }
        if (m_retabNotify) {
            m_retabNotify->Draw();
        }
    }

    POSITION k = m_tabLists[6].GetHeadPosition();
    while (k) {
        CStatusBarItem* p = static_cast<CStatusBarItem*>(m_tabLists[6].GetNext(k));
        if (p) {
            p->RequestRedraw();
            p->Render();
        }
    }
    return 1;
}

static __inline void HiCueFind() {
    SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
    if (registry->m_silentMode == 0) {
        CObject* obj = registry->Lookup("GAME_TABHIGHLIGHT1");
        if (obj) {
            static_cast<SoundCue*>(obj)->PlayIfElapsed(g_soundVolumePercent, 0, 0, 0);
        }
    }
}

static __inline void HiCueLookup() {
    SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
    if (registry->m_silentMode == 0) {
        SoundCue* out = NULL;
        MapLookup(registry->m_cues, "GAME_TABHIGHLIGHT1", out);
        if (out) {
            out->PlayIfElapsed(g_soundVolumePercent, 0, 0, 0);
        }
    }
}

static __inline void HiCueTimed() {
    SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
    if (registry->m_silentMode == 0) {
        SoundCue* found = NULL;
        MapLookup(registry->m_cues, "GAME_TABHIGHLIGHT1", found);
        if (found) {
            i32 soundEnabled = g_soundEnabled;
            i32 volumePercent = g_soundVolumePercent;
            if (soundEnabled != 0) {
                SoundCue* p = found;
                if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                    >= static_cast<u32>(p->m_replayDelayMs)) {
                    p->m_lastPlayTimeMs = g_soundCueTimeMs;
                    p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                }
            }
        }
    }
}

static __inline void HiPost(i32 cmdId) {
    PostMessageA(g_gameReg->m_gameWnd->m_hwnd, WM_COMMAND, cmdId, 0);
}

// @early-stop
// Retail loads `y` into esi in the prologue region and reloads m_barSprite into edx;
// cl swaps the pair (sprite in esi, y loaded late into edx). Same 45 bytes, same
// instruction selection. Permuter exhausted: 25 graded declaration counts, 80
// tu_state_* islands and 7 body spellings, every cell 96.6875.
RVA(0x000fe860, 0x2d)
i32 CStatusBarMgr::SetSpritePos(i32 x, i32 y) {
    if (m_barSprite == NULL) {
        return 0;
    }
    m_barSprite->m_screenX = x;
    m_barSprite->m_screenY = y;
    m_barX = x;
    m_barY = y;
    return 1;
}

RVA(0x000fe8a0, 0x4e)
i32 CStatusBarMgr::HitTestLayer(i32 x, i32 y) {
    CWwdSpriteObject* r = m_barSprite;
    CImage* L = r->m_frameImage;
    i32 xlo = r->m_screenX - L->m_anchorX;
    i32 ylo = r->m_screenY - L->m_anchorY;
    i32 xhi = L->m_width + xlo;
    i32 yhi = L->m_height + ylo;
    if (x >= xhi || x < xlo || y >= yhi || y < ylo) {
        return 0;
    }
    return 1;
}

// @early-stop
// Cross-jumping: retail shares one more PostMessageA tail than cl does, because it
// schedules the `g_gameReg->m_gameWnd` load AFTER the two argument pushes and cl
// hoists it above them, so cl's shareable suffix starts one instruction later.
RVA(0x000fe910, 0xc30)
i32 CStatusBarMgr::UpdateStatusBarTabHighlight(i32 mouseFlags, i32 x, i32 y) {
    CStatusBarItem* w = HitTestRects(x, y);
    if (w == NULL) {
        return 1;
    }
    w->OnPointerMove(mouseFlags, x, y);
    SbiCommandId cmd = w->m_cmd;
    switch (w->m_tab) {
        case TAB_CONTROLS:
            if (m_chatBoxDisabled != 0) {
                break;
            }
            if (g_gameReg->m_triggerMgr->m_groupFlag == 0) {
                break;
            }
            switch (cmd) {
                case SBICMD_TAB_STATZ:
                case SBICMD_TAB_GRUNTZ:
                case SBICMD_TAB_RESOURCE:
                case SBICMD_TAB_MULTIPLAYER:
                case SBICMD_TAB_GAME:
                    HiCueFind();
                    SetTabState(cmd, MENUITEM_SELECTED);
                    return 1;
                case SBICMD_DOCK_LEFT:
                    HiCueFind();
                    DockStatusBarLeft();
                    return 1;
                case SBICMD_DOCK_RIGHT:
                    HiCueFind();
                    DockStatusBarRight();
                    return 1;
                case SBICMD_HIDE:
                    HiCueFind();
                    HideRect();
                    return 1;
                default:
                    return 0;
            }

        case TAB_GAME:
            if (m_levelOverlayActive != 0) {
                break;
            }
            switch (cmd) {
                case SBICMD_PAUSE:
                    HiCueFind();
                    HiPost(0x8007);
                    return 1;
                case SBICMD_LOAD_GAME:
                    HiCueFind();
                    HiPost(0x80ce);
                    return 1;
                case SBICMD_SAVE_GAME:
                    HiCueFind();
                    HiPost(0x80cf);
                    return 1;
                case SBICMD_BOOTY_STATE:
                    HiCueFind();
                    HiPost(0x8035);
                    return 1;
                case SBICMD_SETTINGS:
                    HiCueLookup();
                    HiPost(0x80e2);
                    return 1;
                case SBICMD_QUIT:
                    HiCueLookup();
                    if (g_gameReg->m_frameGate != 0) {
                        i32 gate = g_gameReg->m_frameGate ^ 1;
                        g_gameReg->m_frameGate = gate;
                        g_gameReg->FinishLevel(gate, 1);
                    }
                    (static_cast<CPlay*>(g_gameReg->m_curState))->OpenLevelOverlay(1);
                    return 1;
                case SBICMD_GAME_TAB:
                    HiCueLookup();
                    SetTab(GAME_TAB_MENU, 0);
                    return 1;
                case SBICMD_DESTRUCT:
                    if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
                        break;
                    }
                    if (m_destructButtonLocked != 0) {
                        break;
                    }
                    if (m_chatBoxDisabled != 0) {
                        break;
                    }
                    HiCueLookup();
                    {
                        CPlay* sm = static_cast<CPlay*>(g_gameReg->m_curState);
                        if (m_destructWarningState == DESTRUCT_WARNING_INACTIVE) {
                            m_destructWarningState = DESTRUCT_WARNING_FORWARD;
                            m_destructButtonFrame = DESTRUCT_FRAME_WARNING_FIRST;
                            SbiClockPair* clock = &m_destructWarningClock;
                            clock->m_interval = g_buteMgr.GetDwordDef(
                                "StatusBar",
                                "DestructButtonWarningDelay",
                                0x32
                            );
                            clock->m_last = static_cast<u32>(g_frameTime);
                            sm->SetDefeatCountdown(1, 0xbb7);
                        } else {
                            CSBI_ImageSet* destructButtonImage = m_destructButtonImage;
                            m_destructWarningState = DESTRUCT_WARNING_INACTIVE;
                            m_destructButtonFrame = DESTRUCT_FRAME_IDLE;
                            if (destructButtonImage) {
                                destructButtonImage->Notify(1);
                            }
                            sm->SetDefeatCountdown(0, 0xbb7);
                        }
                    }
                    return 1;
                default:
                    return 0;
            }
            break;

        case TAB_STATZ:
            if (m_chatBoxDisabled != 0) {
                break;
            }
            if (g_gameReg->m_triggerMgr->m_groupFlag == 0) {
                break;
            }
            switch (cmd) {
                case SBICMD_CURSOR_TARGET_FIRST + 0x0:
                case SBICMD_CURSOR_TARGET_FIRST + 0x1:
                case SBICMD_CURSOR_TARGET_FIRST + 0x2:
                case SBICMD_CURSOR_TARGET_FIRST + 0x3:
                case SBICMD_CURSOR_TARGET_FIRST + 0x4:
                case SBICMD_CURSOR_TARGET_FIRST + 0x5:
                case SBICMD_CURSOR_TARGET_FIRST + 0x6:
                case SBICMD_CURSOR_TARGET_FIRST + 0x7:
                case SBICMD_CURSOR_TARGET_FIRST + 0x8:
                case SBICMD_CURSOR_TARGET_FIRST + 0x9:
                case SBICMD_CURSOR_TARGET_FIRST + 0xa:
                case SBICMD_CURSOR_TARGET_FIRST + 0xb:
                case SBICMD_CURSOR_TARGET_FIRST + 0xc:
                case SBICMD_CURSOR_TARGET_FIRST + 0xd:
                case SBICMD_CURSOR_TARGET_FIRST + 0xe:
                    HiCueLookup();
                    PlaceCursorTarget(IDX(cmd) - IDX(SBICMD_CURSOR_TARGET_FIRST), 0);
                    return 1;
                case SBICMD_STAT_TOGGLE_FIRST + 0x0:
                case SBICMD_STAT_TOGGLE_FIRST + 0x1:
                case SBICMD_STAT_TOGGLE_FIRST + 0x2:
                case SBICMD_STAT_TOGGLE_FIRST + 0x3:
                case SBICMD_STAT_TOGGLE_FIRST + 0x4:
                case SBICMD_STAT_TOGGLE_FIRST + 0x5:
                case SBICMD_STAT_TOGGLE_FIRST + 0x6:
                case SBICMD_STAT_TOGGLE_FIRST + 0x7:
                case SBICMD_STAT_TOGGLE_FIRST + 0x8:
                case SBICMD_STAT_TOGGLE_FIRST + 0x9:
                case SBICMD_STAT_TOGGLE_FIRST + 0xa:
                case SBICMD_STAT_TOGGLE_FIRST + 0xb:
                case SBICMD_STAT_TOGGLE_FIRST + 0xc:
                case SBICMD_STAT_TOGGLE_FIRST + 0xd:
                case SBICMD_STAT_TOGGLE_FIRST + 0xe:
                    HiCueLookup();
                    ToggleStat(IDX(cmd) - IDX(SBICMD_STAT_TOGGLE_FIRST));
                    return 1;
                default:
                    return 0;
            }

        case TAB_MULTIPLAYER:
            if (m_chatBoxDisabled != 0) {
                break;
            }
            if (g_gameReg->m_triggerMgr->m_groupFlag == 0) {
                break;
            }
            if (cmd < SBICMD_MULTIPLAYER_HEAD_FIRST || cmd > SBICMD_MULTIPLAYER_HEAD_LAST) {
                return 0;
            }
            HiCueLookup();
            m_tabCycle = IDX(cmd) - IDX(SBICMD_MULTIPLAYER_HEAD_FIRST);
            ResetWidgets(0);
            TryActivate();
            Deactivate();
            return 1;

        case TAB_GRUNTZ:
            if (m_chatBoxDisabled != 0) {
                break;
            }
            if (g_gameReg->m_triggerMgr->m_groupFlag == 0) {
                break;
            }
            if (cmd < SBICMD_GRUNT_SLOT_FIRST || cmd > SBICMD_GRUNT_SLOT_LAST) {
                return 0;
            }
            ActivateSlot(IDX(cmd) - IDX(SBICMD_GRUNT_SLOT_FIRST));
            return 1;

        case TAB_RESOURCE:
            if (m_chatBoxDisabled != 0) {
                break;
            }
            if (g_gameReg->m_triggerMgr->m_groupFlag == 0) {
                break;
            }
            switch (cmd) {
                case SBICMD_TOOL_RESOURCE_CATEGORY:
                case SBICMD_TOOL_RESOURCE_UPPER:
                case SBICMD_TOOL_RESOURCE_MIDDLE:
                case SBICMD_TOOL_RESOURCE_LOWER:
                    SelectToolResource(
                        static_cast<StatusBarHighlightRow>(
                            IDX(cmd) - IDX(SBICMD_TOOL_RESOURCE_FIRST)
                        )
                    );
                    return 1;
                case SBICMD_TOY_RESOURCE_CATEGORY:
                case SBICMD_TOY_RESOURCE_UPPER:
                case SBICMD_TOY_RESOURCE_MIDDLE:
                case SBICMD_TOY_RESOURCE_LOWER:
                    SelectToyResource(
                        static_cast<StatusBarHighlightRow>(
                            IDX(cmd) - IDX(SBICMD_TOY_RESOURCE_FIRST)
                        )
                    );
                    return 1;
                case SBICMD_BRICK_RESOURCE_CATEGORY:
                case SBICMD_BRICK_RESOURCE_UPPER:
                case SBICMD_BRICK_RESOURCE_MIDDLE:
                case SBICMD_BRICK_RESOURCE_LOWER:
                    SelectBrickResource(
                        static_cast<StatusBarHighlightRow>(
                            IDX(cmd) - IDX(SBICMD_BRICK_RESOURCE_FIRST)
                        )
                    );
                    return 1;
            }
            break;

        case TAB_DIALOG:
            switch (cmd) {
                case SBICMD_DIALOG_PRIMARY:
                    if (g_gameReg->m_triggerMgr->m_phase == FINISH_STATE_VICTORY) {
                        HiCueLookup();
                        g_gameReg->FinalizeLevelAndShowResults();
                    } else if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                        HiCueLookup();
                        HiPost(0x806b);
                    } else {
                        HiCueLookup();
                        (static_cast<CPlay*>(g_gameReg->m_curState))->CloseLevelOverlay(0);
                    }
                    return 1;
                case SBICMD_DIALOG_SECONDARY:
                    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                        if (g_gameReg->m_triggerMgr->m_phase == FINISH_STATE_VICTORY) {
                            g_gameReg->CommitSinglePlayerProgress();
                        }
                        HiCueLookup();
                        HiPost(0x8023);
                    } else {
                        HiCueTimed();
                        g_gameReg->FinalizeLevelAndShowResults();
                    }
                    return 1;
                case SBICMD_DIALOG_YES:
                    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                        if (g_gameReg->m_triggerMgr->m_phase == FINISH_STATE_VICTORY) {
                            g_gameReg->CommitSinglePlayerProgress();
                        }
                        HiCueTimed();
                        HiPost(0x8023);
                    } else {
                        HiCueTimed();
                        g_gameReg->FinalizeLevelAndShowResults();
                    }
                    return 1;
                case SBICMD_DIALOG_NO:
                    HiCueTimed();
                    (static_cast<CPlay*>(g_gameReg->m_curState))->CloseLevelOverlay(0);
                    return 1;
                default:
                    return 0;
            }

        default:
            return 0;
    }
    return 1;
}

// @early-stop
RVA(0x000ff850, 0x121)
i32 CStatusBarMgr::HandleDoubleClick(i32 keyFlags, i32 x, i32 y) {
    CStatusBarItem* r = HitTestRects(x, y);
    if (r == NULL) {
        return 1;
    }
    r->OnDoubleClick(keyFlags, x, y);
    SbiCommandId cmd = r->m_cmd;
    if (r->m_tab == TAB_STATZ && m_chatBoxDisabled == 0 && g_gameReg->m_triggerMgr->m_groupFlag != 0
        && cmd >= SBICMD_CURSOR_TARGET_FIRST && cmd <= SBICMD_CURSOR_TARGET_LAST) {
        SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
        if (registry->m_silentMode == 0) {
            SoundCue* found = NULL;
            CMapStringToPtr* map = &registry->m_cues;
            MapLookup(*map, "GAME_TABHIGHLIGHT1", found);
            if (found) {
                i32 soundEnabled = g_soundEnabled;
                i32 volumePercent = g_soundVolumePercent;
                if (soundEnabled != 0) {
                    SoundCue* p = found;
                    if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                        >= static_cast<u32>(p->m_replayDelayMs)) {
                        p->m_lastPlayTimeMs = g_soundCueTimeMs;
                        p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                    }
                }
            }
        }
        PlaceCursorTarget(IDX(cmd) - IDX(SBICMD_CURSOR_TARGET_FIRST), 1);
        return 1;
    }

    return UpdateStatusBarTabHighlight(keyFlags, x, y);
}

RVA(0x000ff9d0, 0x8)
i32 CStatusBarMgr::OnPointerRelease(i32, i32, i32) {
    return 1;
}

RVA(0x000ff9f0, 0xe4)
i32 CStatusBarMgr::HandlePointerDrag(i32 keyFlags, i32 x, i32 y) {
    CStatusBarItem* r = HitTestRects(x, y);
    if (r == NULL) {
        ClearTabSprites(TAB_ALL);
        return 1;
    }
    r->OnPointerDrag(keyFlags, x, y);
    if (r->m_kind != SBI_KIND_MENU_ITEM) {
        ClearTabSprites(TAB_ALL);
        return 1;
    }
    SbiCommandId cmd = r->m_cmd;
    if (m_chatBoxDisabled == 0) {
        if (cmd >= SBICMD_TAB_FIRST && cmd <= SBICMD_TAB_LAST) {
            SetTabState(cmd, MENUITEM_HIGHLIGHT);
        } else {
            ClearTabSprites(TAB_CONTROLS);
        }
    }
    if (m_activeTab == TAB_GAME) {
        if (r->m_tab == TAB_GAME) {
            SetTabState(cmd, MENUITEM_HIGHLIGHT);
        } else {
            ClearTabSprites(TAB_GAME);
        }
    }
    if (m_levelOverlayActive) {
        if (r->m_tab == TAB_DIALOG) {
            SetTabState(cmd, MENUITEM_HIGHLIGHT);
            return 1;
        }
        ClearTabSprites(TAB_GAME);
    }
    return 1;
}

// @early-stop
// Sub-point scheduling residue only; probe-inert (C1 state).
RVA(0x000ffb20, 0x13a)
i32 CStatusBarMgr::UpdateStatusBar(i32 deltaMs) {
    if (g_gameReg->m_soundEnabled != 0) {
        if (m_destructWarningState != DESTRUCT_WARNING_INACTIVE && m_destructButtonLocked == 0) {
            if (m_destructWarningSound == NULL) {

                SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
                CMapStringToPtr* map = &registry->m_cues;
                SoundCue* found = NULL;
                MapLookup(*map, "GAME_DESTRUCT", found);
                if (found) {
                    SoundSample* sample = found->m_sound;
                    if (sample) {
                        SoundBuffer* voice = sample->AcquireInstance();
                        m_destructWarningSound = voice;
                        if (voice) {
                            voice->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
                        }
                    }
                }
            }
        } else {
            if (m_destructWarningSound) {
                m_destructWarningSound->StopAndRewind();
                m_destructWarningSound = NULL;
            }
        }
    }
    UpdateStatusSystems();

    POSITION n = m_tabLists[0].GetHeadPosition();
    while (n) {
        CStatusBarItem* cur = static_cast<CStatusBarItem*>(m_tabLists[0].GetNext(n));
        if (cur) {
            cur->Refresh(deltaMs);
        }
    }
    CPtrList& tab = m_tabLists[IDX(m_activeTab)];
    POSITION m = tab.GetHeadPosition();
    while (m) {
        CStatusBarItem* cur = static_cast<CStatusBarItem*>(tab.GetNext(m));
        if (cur) {
            cur->Refresh(deltaMs);
        }
    }
    POSITION k = m_tabLists[6].GetHeadPosition();
    while (k) {
        CStatusBarItem* cur = static_cast<CStatusBarItem*>(m_tabLists[6].GetNext(k));
        if (cur) {
            cur->Refresh(deltaMs);
        }
    }
    if (m_retabNotify) {
        m_retabNotify->Tick(deltaMs);
        Deactivate();
    }
    return 1;
}

RVA(0x000ffcb0, 0xe2)
CStatusBarItem* CStatusBarMgr::HitTestRects(i32 x, i32 y) {
    POSITION n = m_tabLists[0].GetHeadPosition();
    while (n) {
        CStatusBarItem* r = static_cast<CStatusBarItem*>(m_tabLists[0].GetNext(n));
        if (r) {
            i32 hit = r->m_enabled;
            if (hit) {
                hit = CGameLevel::PointInRect(&r->m_rect, x, y);
            }
            if (hit) {
                return r;
            }
        }
    }
    CPtrList& tab = m_tabLists[IDX(m_activeTab)];
    n = tab.GetHeadPosition();
    while (n) {
        CStatusBarItem* r = static_cast<CStatusBarItem*>(tab.GetNext(n));
        if (r) {
            i32 hit = r->m_enabled;
            if (hit) {
                hit = CGameLevel::PointInRect(&r->m_rect, x, y);
            }
            if (hit) {
                return r;
            }
        }
    }
    n = m_tabLists[6].GetHeadPosition();
    while (n) {
        CStatusBarItem* r = static_cast<CStatusBarItem*>(m_tabLists[6].GetNext(n));
        if (r) {
            i32 hit = r->m_enabled;
            if (hit) {
                hit = CGameLevel::PointInRect(&r->m_rect, x, y);
            }
            if (hit) {
                return r;
            }
        }
    }
    return NULL;
}

RVA(0x000ffde0, 0x5b1)
i32 CStatusBarMgr::BuildStatusBarTabs() {
    if (m_tabsBuilt != 0) {
        return 1;
    }
    if (m_world == NULL) {
        return 0;
    }
    i32 bx = m_barRect.left;
    i32 by = m_barRect.top;
    CDDrawSurfaceMgr* code = m_world;

    CSBI_RectOnly* dockLeft = new CSBI_RectOnly;
    if (!dockLeft->Setup(
            this,
            code,
            SBICMD_DOCK_LEFT,
            TAB_CONTROLS,
            SbGeom(bx + 0x7c, by + 0xad, bx + 0x88, by + 0xb9),
            NULL,
            -1
        )) {
        delete dockLeft;
        return 0;
    }
    AddTabItem(0, dockLeft);

    // Same vertical band as dockLeft: retail does not recompute the pair here, it
    // RELOADS the two frame slots dockLeft's `by + 0xad` / `by + 0xb9` were CSE'd into
    // (`mov eax,[esp+0x34]` / `[esp+0x3c]` at +0x138), so the two dock buttons sit side
    // by side rather than stacked.
    CSBI_RectOnly* dockRight = new CSBI_RectOnly;
    if (!dockRight->Setup(
            this,
            code,
            SBICMD_DOCK_RIGHT,
            TAB_CONTROLS,
            SbGeom(bx + 0x8a, by + 0xad, bx + 0x96, by + 0xb9),
            NULL,
            -1
        )) {
        delete dockRight;
        return 0;
    }
    AddTabItem(0, dockRight);

    CSBI_RectOnly* hide = new CSBI_RectOnly;
    if (!hide->Setup(
            this,
            code,
            SBICMD_HIDE,
            TAB_CONTROLS,
            SbGeom(bx + 0x83, by + 0xbb, bx + 0x8f, by + 0xc7),
            NULL,
            -1
        )) {
        delete hide;
        return 0;
    }
    AddTabItem(0, hide);

    // The five tab buttons share ONE band, `by + 0x82 .. by + 0xad` - retail computes
    // `by + 0x82` once here and reloads dockLeft's `by + 0xad` slot for the bottom, and
    // `by + 0x99` appears in no immediate anywhere in the function (immediates sieve).
    CSBI_MenuItem* statzTab = new CSBI_MenuItem;
    if (!statzTab->SetupImage(
            this,
            code,
            SBICMD_TAB_STATZ,
            TAB_CONTROLS,
            SbGeom(bx + 0x42, by + 0x82, bx + 0x62, by + 0xad),
            "GAME_STATUSBAR_TABZ_STATZTAB",
            -1,
            0
        )) {
        delete statzTab;
        return 0;
    }
    AddTabItem(0, statzTab);
    m_tabSprite0 = statzTab;

    CSBI_MenuItem* gruntzTab = new CSBI_MenuItem;
    if (!gruntzTab->SetupImage(
            this,
            code,
            SBICMD_TAB_GRUNTZ,
            TAB_CONTROLS,
            SbGeom(bx + 0x04, by + 0x82, bx + 0x24, by + 0xad),
            "GAME_STATUSBAR_TABZ_GRUNTZTAB",
            -1,
            0
        )) {
        delete gruntzTab;
        return 0;
    }
    AddTabItem(0, gruntzTab);
    m_tabSprite2 = gruntzTab;

    CSBI_MenuItem* resourceTab = new CSBI_MenuItem;
    if (!resourceTab->SetupImage(
            this,
            code,
            SBICMD_TAB_RESOURCE,
            TAB_CONTROLS,
            SbGeom(bx + 0x24, by + 0x82, bx + 0x44, by + 0xad),
            "GAME_STATUSBAR_TABZ_RESOURCETAB",
            -1,
            0
        )) {
        delete resourceTab;
        return 0;
    }
    AddTabItem(0, resourceTab);
    m_tabSprite1 = resourceTab;

    CSBI_MenuItem* multiTab = new CSBI_MenuItem;
    if (!multiTab->SetupImage(
            this,
            code,
            SBICMD_TAB_MULTIPLAYER,
            TAB_CONTROLS,
            SbGeom(bx + 0x60, by + 0x82, bx + 0x80, by + 0xad),
            "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB",
            -1,
            0
        )) {
        delete multiTab;
        return 0;
    }
    AddTabItem(0, multiTab);
    m_tabSprite3 = multiTab;
    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        multiTab->m_state = MENUITEM_DISABLED;
        CDDrawWorker* f = multiTab->m_record;
        if (f != NULL) {
            multiTab->SetFrame(f->GetAt(IDX(MENUITEM_DISABLED)));
        }
        multiTab->SetEnabled(0);
        multiTab->RequestRedraw();
    }

    CSBI_MenuItem* gameTab = new CSBI_MenuItem;
    if (!gameTab->SetupImage(
            this,
            code,
            SBICMD_TAB_GAME,
            TAB_CONTROLS,
            SbGeom(bx + 0x7e, by + 0x82, bx + 0x9e, by + 0xad),
            "GAME_STATUSBAR_TABZ_GAMETAB",
            -1,
            0
        )) {
        delete gameTab;
        return 0;
    }
    AddTabItem(0, gameTab);
    m_tabSprite4 = gameTab;

    if (BuildSideTabs() == 0) {
        return 0;
    }
    if (LoadTabSprites() == 0) {
        return 0;
    }
    if (BuildTabzDialog() == 0) {
        return 0;
    }
    m_tabsBuilt = 1;
    return 1;
}

static __inline i32 WapRand(i32 range) {
    if (range == 0) {
        return GetRandomNumber() & 1;
    }
    return GetRandomNumber() % range + 1;
}

RVA(0x00100510, 0x6)
i32 CStatusBarItem::Render() {
    return 1;
}

RVA(0x00100530, 0x5)
i32 CStatusBarItem::OnPointerMove(i32, i32, i32) {
    return 0;
}
RVA(0x00100550, 0x5)
i32 CStatusBarItem::OnDoubleClick(i32, i32, i32) {
    return 0;
}
RVA(0x00100570, 0x5)
i32 CStatusBarItem::UnusedPointerAction(i32, i32, i32) {
    return 0;
}
RVA(0x00100590, 0x5)
i32 CStatusBarItem::OnPointerDrag(i32, i32, i32) {
    return 0;
}

RVA(0x001005b0, 0x8)
void CStatusBarItem::RequestRedraw() {
    m_redrawFrames = 2;
}

RVA(0x00100600, 0x8)
i32 CStatusBarItem::Refresh(i32) {
    return 1;
}

RVA_COMPGEN(0x00100620, 0x24, ??_GCStatusBarItem@@UAEPAXI@Z)

RVA(0x00100660, 0x50)
i32 CStatusBarItem::Setup(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT rc,
    const char* key,
    i32 unusedFrame
) {
    if (host == NULL || owner == NULL) {
        return 0;
    }
    m_owner = owner;
    m_host = host;
    m_tab = tab;
    m_rect = rc;
    m_cmd = cmd;
    return 1;
}

RVA_COMPGEN(0x001006d0, 0x1e, ??_GCSBI_RectOnly@@UAEPAXI@Z)
RVA_COMPGEN(0x00100700, 0x55, ??1CSBI_RectOnly@@UAE@XZ)
RVA_COMPGEN(0x00100780, 0xb, ??1CStatusBarItem@@UAE@XZ)
RVA_COMPGEN(0x001007a0, 0x1e, ??_GCSBI_MenuItem@@UAEPAXI@Z)
RVA_COMPGEN(0x001007d0, 0x7f, ??1CSBI_MenuItem@@UAE@XZ)
RVA_COMPGEN(0x00100870, 0x6a, ??1CSBI_Image@@UAE@XZ)
RVA_COMPGEN(0x00100900, 0x1e, ??_GCSBI_Image@@UAEPAXI@Z)
RVA(0x00100930, 0x16c)
void CStatusBarMgr::ResetWidgets(i32 keepHost) {
    for (i32 t = 0; t < 8; t++) {
        POSITION n = m_tabLists[t].GetHeadPosition();
        while (n) {
            CStatusBarItem* cur = static_cast<CStatusBarItem*>(m_tabLists[t].GetNext(n));
            delete cur;
        }
        m_tabLists[t].RemoveAll();
    }
    if (keepHost) {
        if (m_barSprite) {

            m_barSprite->m_stateFlags |= SPRITE_STATE_HIDDEN;
            m_barSprite->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        }
    }
    m_tabSprite0 = NULL;
    m_tabSprite1 = NULL;
    m_tabSprite2 = NULL;
    m_tabSprite3 = NULL;
    m_tabSprite4 = NULL;
    m_tabSprite5 = NULL;
    m_tabSprite6 = NULL;
    m_tabSprite7 = NULL;
    m_tabSprite8 = NULL;
    m_tabSprite9 = NULL;
    m_tabSprite10 = NULL;
    m_tabSprite11 = NULL;
    m_tabSprite12 = NULL;
    m_tabSprite13 = NULL;
    m_tabSprite14 = NULL;
    m_barSprite = NULL;
    i32 i;
    memset(m_hitRects, 0, sizeof(m_hitRects));
    memset(m_statObj, 0, sizeof(m_statObj));
    memset(m_slotNotify, 0, sizeof(m_slotNotify));
    memset(m_conveyorSprites, 0, sizeof(m_conveyorSprites));
    memset(m_resourceSlotSprites, 0, sizeof(m_resourceSlotSprites));
    memset(m_warlordHead, 0, sizeof(m_warlordHead));
    m_extraNotify0 = NULL;
    m_extraNotify1 = NULL;
    m_destructButtonImage = NULL;
    m_notify0 = NULL;
    m_notify2 = NULL;
    m_notify3 = NULL;
    m_notify1 = NULL;
    m_machineDisplay = NULL;
    m_gaugeNotify = NULL;
    m_gaugeSink = NULL;
    m_tabsBuilt = 0;
}

RVA(0x00100b00, 0x150)
void CStatusBarMgr::ClearTabGroup() {
    if (m_activeTab == TAB_NONE) {
        return;
    }
    CPtrList& tab = m_tabLists[IDX(m_activeTab)];
    POSITION n = tab.GetHeadPosition();
    while (n) {
        CStatusBarItem* cur = static_cast<CStatusBarItem*>(tab.GetNext(n));
        delete cur;
    }
    m_tabLists[IDX(m_activeTab)].RemoveAll();
    switch (m_activeTab) {
        case TAB_GAME:
            m_tabSprite5 = NULL;
            m_tabSprite6 = NULL;
            m_tabSprite7 = NULL;
            m_tabSprite8 = NULL;
            m_tabSprite9 = NULL;
            m_tabSprite10 = NULL;
            m_destructButtonImage = NULL;
            break;
        case TAB_STATZ:

            memset(m_statObj, 0, sizeof(m_statObj));
            break;
        case TAB_MULTIPLAYER:
            memset(m_warlordHead, 0, sizeof(m_warlordHead));
            break;
        case TAB_GRUNTZ: {

            memset(m_slotNotify, 0, sizeof(m_slotNotify));
            m_gaugeNotify = NULL;
            m_gaugeSink = NULL;
            break;
        }
        case TAB_RESOURCE: {

            memset(m_conveyorSprites, 0, sizeof(m_conveyorSprites));
            m_machineDisplay = NULL;

            memset(m_resourceSlotSprites, 0, sizeof(m_resourceSlotSprites));
            m_notify0 = NULL;
            m_notify2 = NULL;
            m_notify3 = NULL;
            m_notify1 = NULL;
            m_extraNotify0 = NULL;
            m_extraNotify1 = NULL;
            break;
        }
    }
}

RVA(0x00100cb0, 0x8b)
i32 CStatusBarMgr::Deactivate() {
    if (m_position == STATUSBAR_HIDDEN) {

        i32 w = g_gameReg->m_modeSize.cx;
        i32 h = g_gameReg->m_modeSize.cy;
        m_barX = w - 0x45;
        m_barY = h - 0x30;
        SetSpritePos(w - 0x45, h - 0x30);
    }

    POSITION n = m_tabLists[0].GetHeadPosition();
    while (n) {
        CSBI_ImageSet* cur = static_cast<CSBI_ImageSet*>(m_tabLists[0].GetNext(n));
        if (cur) {
            cur->RequestRedraw();
        }
    }

    CPtrList& tab = m_tabLists[IDX(m_activeTab)];
    POSITION m = tab.GetHeadPosition();
    while (m) {
        CSBI_ImageSet* cur = static_cast<CSBI_ImageSet*>(tab.GetNext(m));
        if (cur) {
            cur->RequestRedraw();
        }
    }

    ClearTabSprites(TAB_ALL);
    m_redrawFrames = 2;
    return 1;
}

RVA(0x00100d70, 0x548)
i32 CStatusBarMgr::SetTabState(SbiCommandId cmd, SbiMenuItemState state) {
    if (m_tabSprite0 == NULL || m_tabSprite1 == NULL || m_tabSprite2 == NULL || m_tabSprite3 == NULL
        || m_tabSprite4 == NULL) {
        return 0;
    }
    switch (cmd) {
        case SBICMD_TAB_STATZ:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->SetState(state, 1);
            m_tabSprite2->ProbeState(state);
            m_tabSprite1->ProbeState(state);
            m_tabSprite3->ProbeState(state);
            m_tabSprite4->ProbeState(state);
            break;
        case SBICMD_TAB_GRUNTZ:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->ProbeState(state);
            m_tabSprite2->SetState(state, 1);
            m_tabSprite1->ProbeState(state);
            m_tabSprite3->ProbeState(state);
            m_tabSprite4->ProbeState(state);
            break;
        case SBICMD_TAB_RESOURCE:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->ProbeState(state);
            m_tabSprite2->ProbeState(state);
            m_tabSprite1->SetState(state, 1);
            m_tabSprite3->ProbeState(state);
            m_tabSprite4->ProbeState(state);
            break;
        case SBICMD_TAB_MULTIPLAYER:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->ProbeState(state);
            m_tabSprite2->ProbeState(state);
            m_tabSprite1->ProbeState(state);
            m_tabSprite3->SetState(state, 1);
            m_tabSprite4->ProbeState(state);
            break;
        case SBICMD_TAB_GAME:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->ProbeState(state);
            m_tabSprite2->ProbeState(state);
            m_tabSprite1->ProbeState(state);
            m_tabSprite3->ProbeState(state);
            m_tabSprite4->SetState(state, 1);
            break;
        case SBICMD_PAUSE:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->SetState(state, 1);
            m_tabSprite6->ProbeState(state);
            m_tabSprite7->ProbeState(state);
            m_tabSprite8->ProbeState(state);
            m_tabSprite9->ProbeState(state);
            m_tabSprite10->ProbeState(state);
            break;
        case SBICMD_LOAD_GAME:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->ProbeState(state);
            m_tabSprite6->SetState(state, 1);
            m_tabSprite7->ProbeState(state);
            m_tabSprite8->ProbeState(state);
            m_tabSprite9->ProbeState(state);
            m_tabSprite10->ProbeState(state);
            break;
        case SBICMD_SAVE_GAME:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->ProbeState(state);
            m_tabSprite6->ProbeState(state);
            m_tabSprite7->SetState(state, 1);
            m_tabSprite8->ProbeState(state);
            m_tabSprite9->ProbeState(state);
            m_tabSprite10->ProbeState(state);
            break;
        case SBICMD_SETTINGS:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->ProbeState(state);
            m_tabSprite6->ProbeState(state);
            m_tabSprite7->ProbeState(state);
            m_tabSprite8->SetState(state, 1);
            m_tabSprite9->ProbeState(state);
            m_tabSprite10->ProbeState(state);
            break;
        case SBICMD_BOOTY_STATE:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->ProbeState(state);
            m_tabSprite6->ProbeState(state);
            m_tabSprite7->ProbeState(state);
            m_tabSprite8->ProbeState(state);
            m_tabSprite9->SetState(state, 1);
            m_tabSprite10->ProbeState(state);
            break;
        case SBICMD_QUIT:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite5->ProbeState(state);
            m_tabSprite6->ProbeState(state);
            m_tabSprite7->ProbeState(state);
            m_tabSprite8->ProbeState(state);
            m_tabSprite9->ProbeState(state);
            m_tabSprite10->SetState(state, 1);
            break;
        case SBICMD_GAME_TAB:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite10->SetState(state, 1);
            break;
        case SBICMD_DIALOG_PRIMARY:
            if (m_tabSprite11) {
                m_tabSprite11->SetState(state, 1);
            }
            m_tabSprite12->ProbeState(state);
            break;
        case SBICMD_DIALOG_SECONDARY:
            if (m_tabSprite11) {
                m_tabSprite11->ProbeState(state);
            }
            m_tabSprite12->SetState(state, 1);
            break;
        case SBICMD_DIALOG_YES:
            m_tabSprite13->SetState(state, 1);
            m_tabSprite14->ProbeState(state);
            break;
        case SBICMD_DIALOG_NO:
            m_tabSprite13->ProbeState(state);
            m_tabSprite14->SetState(state, 1);
            break;
    }
    return 1;
}

RVA(0x00101420, 0x110)
i32 CStatusBarMgr::ClearTabSprites(StatusBarTab idx) {
    if (idx == TAB_ALL || idx == TAB_CONTROLS) {
        if (m_tabSprite0) {
            m_tabSprite0->Blit();
        }
        if (m_tabSprite2) {
            m_tabSprite2->Blit();
        }
        if (m_tabSprite1) {
            m_tabSprite1->Blit();
        }
        if (m_tabSprite3) {
            m_tabSprite3->Blit();
        }
        if (m_tabSprite4) {
            m_tabSprite4->Blit();
        }
    }
    if (idx == TAB_GAME || idx == TAB_ALL) {
        if (m_tabSprite5) {
            m_tabSprite5->Blit();
        }
        if (m_tabSprite6) {
            m_tabSprite6->Blit();
        }
        if (m_tabSprite7) {
            m_tabSprite7->Blit();
        }
        if (m_tabSprite8) {
            m_tabSprite8->Blit();
        }
        if (m_tabSprite9) {
            m_tabSprite9->Blit();
        }
        if (m_tabSprite10) {
            m_tabSprite10->Blit();
        }
    }
    if (idx == TAB_DIALOG || idx == TAB_ALL) {
        if (m_tabSprite11) {
            m_tabSprite11->Blit();
        }
        if (m_tabSprite12) {
            m_tabSprite12->Blit();
        }
        if (m_tabSprite13) {
            m_tabSprite13->Blit();
        }
        if (m_tabSprite14) {
            m_tabSprite14->Blit();
        }
    }
    return 1;
}

// @early-stop
RVA(0x00101580, 0x806)
i32 CStatusBarMgr::BuildGameMenu() {
    CDDrawSurfaceMgr* code = m_world;
    i32 bx = m_barRect.left;
    i32 by = m_barRect.top;

    if (m_itemKind != GAME_TAB_MISSION_STATUS) {

        if (m_chatBoxDisabled != 0 && g_gameReg->m_frameGate != 0) {
            CSBI_MenuItem* resume = new CSBI_MenuItem;
            if (!resume->SetupImage(
                    this,
                    code,
                    SBICMD_PAUSE,
                    TAB_GAME,
                    SbGeom(bx, by + 0xd5, bx + 0x9f, by + 0xec),
                    "GAME_STATUSBAR_TABZ_GAMETAB_RESUME",
                    -1,
                    0
                )) {
                delete resume;
                return 0;
            }
            AddTabItem(5, resume);
            m_tabSprite5 = resume;
        } else {
            CSBI_MenuItem* pause = new CSBI_MenuItem;
            if (!pause->SetupImage(
                    this,
                    code,
                    SBICMD_PAUSE,
                    TAB_GAME,
                    SbGeom(bx, by + 0xd5, bx + 0x9f, by + 0xec),
                    "GAME_STATUSBAR_TABZ_GAMETAB_PAUSE",
                    -1,
                    0
                )) {
                delete pause;
                return 0;
            }
            AddTabItem(5, pause);
            m_tabSprite5 = pause;
        }

        CSBI_MenuItem* load = new CSBI_MenuItem;
        if (!load->SetupImage(
                this,
                code,
                SBICMD_LOAD_GAME,
                TAB_GAME,
                SbGeom(bx, by + 0x125, bx + 0x9f, by + 0x13c),
                "GAME_STATUSBAR_TABZ_GAMETAB_LOAD",
                -1,
                0
            )) {
            delete load;
            return 0;
        }
        AddTabItem(5, load);
        m_tabSprite6 = load;
        if (g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER) {
            load->SetEnabled(0);
        }

        CSBI_MenuItem* save = new CSBI_MenuItem;
        if (!save->SetupImage(
                this,
                code,
                SBICMD_SAVE_GAME,
                TAB_GAME,
                SbGeom(bx, by + 0xfd, bx + 0x9f, by + 0x114),
                "GAME_STATUSBAR_TABZ_GAMETAB_SAVE",
                -1,
                0
            )) {
            delete save;
            return 0;
        }
        AddTabItem(5, save);
        m_tabSprite7 = save;
        if (g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER) {
            save->SetEnabled(0);
        }

        CSBI_MenuItem* settings = new CSBI_MenuItem;
        if (!settings->SetupImage(
                this,
                code,
                SBICMD_SETTINGS,
                TAB_GAME,
                SbGeom(bx, by + 0x14d, bx + 0x9f, by + 0x164),
                "GAME_STATUSBAR_TABZ_GAMETAB_SETTINGS",
                -1,
                0
            )) {
            delete settings;
            return 0;
        }
        AddTabItem(5, settings);
        m_tabSprite8 = settings;

        CSBI_MenuItem* help = new CSBI_MenuItem;
        if (!help->SetupImage(
                this,
                code,
                SBICMD_BOOTY_STATE,
                TAB_GAME,
                SbGeom(bx, by + 0x175, bx + 0x9f, by + 0x18c),
                "GAME_STATUSBAR_TABZ_GAMETAB_HELP",
                -1,
                0
            )) {
            delete help;
            return 0;
        }
        AddTabItem(5, help);
        m_tabSprite9 = help;
        if (g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER) {
            help->SetEnabled(0);
        }

        CSBI_MenuItem* quit = new CSBI_MenuItem;
        if (!quit->SetupImage(
                this,
                code,
                SBICMD_QUIT,
                TAB_GAME,
                SbGeom(bx, by + 0x19d, bx + 0x9f, by + 0x1b4),
                "GAME_STATUSBAR_TABZ_GAMETAB_QUIT",
                -1,
                0
            )) {
            delete quit;
            return 0;
        }
        AddTabItem(5, quit);
        m_tabSprite10 = quit;

        CSBI_ImageSet* destruct = new CSBI_ImageSet;
        if (!destruct->SetupImage(
                this,
                code,
                SBICMD_DESTRUCT,
                TAB_GAME,
                SbGeom(bx + 0x22, by + 0x1be, bx + 0x7d, by + 0x1d6),
                "GAME_STATUSBAR_TABZ_GAMETAB_DESTRUCT",
                IDX(m_destructButtonFrame),
                0
            )) {
            delete destruct;
            return 0;
        }
        AddTabItem(5, destruct);
        m_destructButtonImage = destruct;
        if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
            destruct->SetEnabled(0);
            m_destructButtonFrame = DESTRUCT_FRAME_DISABLED;
            m_destructWarningState = DESTRUCT_WARNING_INACTIVE;
            m_destructButtonImage->Notify(IDX(DESTRUCT_FRAME_DISABLED));
        }
        return 1;
    }

    CSBI_ImageSet* status;
    if (g_gameReg->m_triggerMgr->m_phase == FINISH_STATE_VICTORY) {
        status = new CSBI_ImageSet;
        if (!status->SetupImage(
                this,
                code,
                SBICMD_MISSION_STATUS,
                TAB_GAME,
                SbGeom(bx, by + 0xd7, bx + 0x9f, by + 0x118),
                "GAME_STATUSBAR_TABZ_GAMETAB_MISSIONSTATUS",
                1,
                0
            )) {
            delete status;
            return 0;
        }
    } else {
        status = new CSBI_ImageSet;
        if (!status->SetupImage(
                this,
                code,
                SBICMD_MISSION_STATUS,
                TAB_GAME,
                SbGeom(bx, by + 0xd7, bx + 0x9f, by + 0x118),
                "GAME_STATUSBAR_TABZ_GAMETAB_MISSIONSTATUS",
                2,
                0
            )) {
            delete status;
            return 0;
        }
    }
    AddTabItem(5, status);
    return 1;
}

RVA_COMPGEN(0x00101fd0, 0x1e, ??_GCSBI_ImageSet@@UAEPAXI@Z)

RVA_COMPGEN(0x00102000, 0x7f, ??1CSBI_ImageSet@@UAE@XZ)

RVA(0x001020a0, 0xae)
i32 CStatusBarMgr::SetTab(GameTabContent tab, i32 flag) {
    if (tab == m_itemKind && flag == 0) {
        return 1;
    }
    POSITION n = m_tabLists[5].GetHeadPosition();
    while (n) {
        CStatusBarItem* cur = static_cast<CStatusBarItem*>(m_tabLists[5].GetNext(n));
        delete cur;
    }
    m_tabLists[5].RemoveAll();
    m_tabSprite5 = NULL;
    m_tabSprite6 = NULL;
    m_tabSprite7 = NULL;
    m_tabSprite8 = NULL;
    m_tabSprite9 = NULL;
    m_tabSprite10 = NULL;
    m_itemKind = tab;

    if (!LoadTabSprites()) {
        g_gameReg->ReportError(kActivateErrId, kSetTabErrTag);
        return 0;
    }
    Deactivate();
    return 1;
}

RVA(0x00102180, 0x5f)
void CStatusBarMgr::BuildGameTabResumeButton(i32 show) {
    if (m_position == STATUSBAR_HIDDEN) {
        RestoreStatusBar();
    }
    if (show && m_activeTab != TAB_GAME) {
        SetTabState(SBICMD_TAB_GAME, MENUITEM_SELECTED);
    }
    if (m_tabSprite5) {
        m_tabSprite5->ResolveFrame("GAME_STATUSBAR_TABZ_GAMETAB_RESUME", 1);
        Deactivate();
        m_tabSprite5->RequestRedraw();
    }
    m_chatBoxDisabled = 1;
}

RVA(0x00102200, 0x37)
void CStatusBarMgr::BuildGameTabPauseButton() {
    if (m_tabSprite5) {
        m_tabSprite5->ResolveFrame("GAME_STATUSBAR_TABZ_GAMETAB_PAUSE", 1);
        Deactivate();
        m_tabSprite5->RequestRedraw();
    }
    m_chatBoxDisabled = 0;
}

// @early-stop
RVA(0x00102250, 0x1de4)
i32 CStatusBarMgr::LoadTabSprites() {
    CDDrawSurfaceMgr* code = m_world;
    i32 bx = m_barRect.left;
    i32 by = m_barRect.top;

    CSBI_Image* it;
    CSBI_ImageSet* imgSet;
    CSBI_WellGoo* goo;
    CSBI_WarlordHead* head;
    CSBI_ImageSetAni* ani;
    CSBI_StatzTabArrow* arrow;
    CSBI_GruntMachine* mach;
    CSBI_StatzTabGruntBar* bar;
    i32 i;

    switch (m_activeTab) {
        case TAB_GRUNTZ:
            it = new CSBI_Image;
            if (!it->SetupImage(
                    this,
                    code,
                    SBICMD_TAB_TITLE_TEXT,
                    TAB_GRUNTZ,
                    SbGeom(bx + 0x18, by + 0xaf, bx + 0x70, by + 0xbe),
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_TITLETEXT",
                    -1,
                    0
                )) {
                delete it;
                return 0;
            }
            AddTabItem(2, it);

            {
                CSBI_ImageSet** aptr = m_slotNotify;
                i32* bptr = &m_slots[0].m_value;
                i32 y = by + 0xfe;
                for (i = 0; i < 5; i++) {
                    CSBI_ImageSet* set = new CSBI_ImageSet;
                    if (!set->SetupImage(
                            this,
                            code,
                            static_cast<SbiCommandId>(IDX(SBICMD_GRUNT_SLOT_FIRST) + i),
                            TAB_GRUNTZ,
                            SbGeom(bx + 0xe, y - 0x32, bx + 0x39, y),
                            "GAME_STATUSBAR_TABZ_GRUNTZTAB_GRUNTOVEN",
                            *bptr,
                            0
                        )) {
                        delete set;
                        return 0;
                    }
                    AddTabItem(2, set);
                    *aptr = set;
                    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(
                        IDX(g_gameReg->m_players[g_curPlayer].m_color),
                        0
                    );
                    if (sel == NULL) {
                        sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
                    }
                    set->m_frameSet->SetAllTypes(SHADE_PAL_16);
                    set->m_frameSet->SetAllFormats(sel);
                    aptr++;
                    bptr += 6;
                    y += 0x36;
                }
            }
            it = new CSBI_Image;
            if (!it->SetupImage(
                    this,
                    code,
                    SBICMD_GRUNT_WELL,
                    TAB_GRUNTZ,
                    SbGeom(bx + 0x4c, by + 0xc8, bx + 0x97, by + 0x1cd),
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELL",
                    -1,
                    0
                )) {
                delete it;
                return 0;
            }
            AddTabItem(2, it);
            m_gaugeNotify = it;
            it->SetEnabled(1);
            it = new CSBI_Image;
            if (!it->SetupImage(
                    this,
                    code,
                    SBICMD_GRUNT_OVENS_TEXT,
                    TAB_GRUNTZ,
                    SbGeom(bx + 0x1e, by + 0xc4, bx + 0x3d, by + 0xcd),
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_OVENZTEXT",
                    -1,
                    0
                )) {
                delete it;
                return 0;
            }
            AddTabItem(2, it);
            it = new CSBI_Image;
            if (!it->SetupImage(
                    this,
                    code,
                    SBICMD_GRUNT_WELL_TEXT,
                    TAB_GRUNTZ,
                    SbGeom(bx + 0x68, by + 0x1cf, bx + 0x87, by + 0x1d8),
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELLTEXT",
                    -1,
                    0
                )) {
                delete it;
                return 0;
            }
            AddTabItem(2, it);
            // CSBI_WellGoo overrides the 7-arg base Setup (vtable slot 2), not
            // CSBI_Image::SetupImage - retail dispatches through [edx+0x8] here.
            goo = new CSBI_WellGoo;
            if (!goo->Setup(
                    this,
                    code,
                    SBICMD_GRUNT_WELL_GOO,
                    TAB_GRUNTZ,
                    SbGeom(bx + 0x6e, by + 0xf8, bx + 0x81, by + 0x1b3),
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELLGOO",
                    m_gauge
                )) {
                delete goo;
                return 0;
            }
            m_gaugeSink = goo;
            AddTabItem(2, goo);
            return 1;

        case TAB_RESOURCE:
            it = new CSBI_Image;
            if (!it->SetupImage(
                    this,
                    code,
                    SBICMD_TAB_TITLE_TEXT,
                    TAB_RESOURCE,
                    SbGeom(bx + 0x18, by + 0xaf, bx + 0x70, by + 0xbe),
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_TITLETEXT",
                    -1,
                    0
                )) {
                delete it;
                return 0;
            }
            AddTabItem(3, it);
            it = new CSBI_Image;
            if (!it->SetupImage(
                    this,
                    code,
                    SBICMD_RESOURCE_MAIN_BACKGROUND,
                    TAB_RESOURCE,
                    SbGeom(bx, by + 0x135, bx + 0x9f, by + 0x1be),
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_MAINBACKGROUND",
                    -1,
                    0
                )) {
                delete it;
                return 0;
            }
            AddTabItem(3, it);
            m_notify0 = it;
            it = new CSBI_Image;
            if (!it->SetupImage(
                    this,
                    code,
                    SBICMD_RESOURCE_UPPER_BACKGROUND,
                    TAB_RESOURCE,
                    SbGeom(bx, by + 0xfb, bx + 0x9f, by + 0x134),
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_UPPERBACKGROUND",
                    -1,
                    0
                )) {
                delete it;
                return 0;
            }
            AddTabItem(3, it);
            m_notify2 = it;
            it = new CSBI_Image;
            if (!it->SetupImage(
                    this,
                    code,
                    SBICMD_RESOURCE_WINDOW_BACKGROUND,
                    TAB_RESOURCE,
                    SbGeom(bx + 0x48, by + 0xd3, bx + 0x67, by + 0xf3),
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_WINDOWBACKGROUND",
                    -1,
                    0
                )) {
                delete it;
                return 0;
            }
            AddTabItem(3, it);
            m_notify3 = it;

            imgSet = new CSBI_ImageSet;
            if (!imgSet->SetupImage(
                    this,
                    code,
                    SBICMD_RESOURCE_BELT_TOOLS,
                    TAB_RESOURCE,
                    SbGeom(bx + 0x19, by + 0x11c, bx + 0x3c, by + 0x130),
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_BELT",
                    m_conveyorSlots[0].m_value,
                    0
                )) {
                delete imgSet;
                return 0;
            }
            AddTabItem(3, imgSet);
            m_conveyorSprites[0] = imgSet;
            imgSet = new CSBI_ImageSet;
            if (!imgSet->SetupImage(
                    this,
                    code,
                    SBICMD_RESOURCE_BELT_TOYS,
                    TAB_RESOURCE,
                    SbGeom(bx + 0x40, by + 0x11c, bx + 0x63, by + 0x130),
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_BELT",
                    m_conveyorSlots[1].m_value,
                    0
                )) {
                delete imgSet;
                return 0;
            }
            AddTabItem(3, imgSet);
            m_conveyorSprites[1] = imgSet;
            imgSet = new CSBI_ImageSet;
            if (!imgSet->SetupImage(
                    this,
                    code,
                    SBICMD_RESOURCE_BELT_BRICKS,
                    TAB_RESOURCE,
                    SbGeom(bx + 0x68, by + 0x11c, bx + 0x8b, by + 0x130),
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_BELT",
                    m_conveyorSlots[2].m_value,
                    0
                )) {
                delete imgSet;
                return 0;
            }
            AddTabItem(3, imgSet);
            m_conveyorSprites[2] = imgSet;

            imgSet = new CSBI_ImageSet;
            if (!imgSet->SetupImage(
                    this,
                    code,
                    SBICMD_RESOURCE_CURRENT_ITEM,
                    TAB_RESOURCE,
                    SbGeom(
                        m_itemRect.left + bx,
                        m_itemRect.top + by,
                        m_itemRect.right + bx,
                        m_itemRect.bottom + by
                    ),
                    "GAME_INGAMEICONZ_GREYCHIPZ",
                    m_extraNotifyArg0,
                    0
                )) {
                delete imgSet;
                return 0;
            }
            AddTabItem(3, imgSet);
            m_extraNotify0 = imgSet;
            imgSet->SetEnabled(0);

            {
                i32* cfgp = &m_resourceSlots[4].m_value;
                CSBI_ImageSet** cachep = &m_resourceSlotSprites[4];
                i32 y = by + 0x155;
                for (i = 0; i < 4; i++) {
                    CSBI_ImageSet* set = new CSBI_ImageSet;
                    if (!set->SetupImage(
                            this,
                            code,
                            static_cast<SbiCommandId>(IDX(SBICMD_TOOL_RESOURCE_FIRST) + i),
                            TAB_RESOURCE,
                            SbGeom(bx + 0x1d, y - 0x17, bx + 0x34, y),
                            "GAME_INGAMEICONZ_NORMCHIPZ",
                            cfgp[-24],
                            0
                        )) {
                        delete set;
                        return 0;
                    }
                    AddTabItem(3, set);
                    cachep[-4] = set;
                    set = new CSBI_ImageSet;
                    if (!set->SetupImage(
                            this,
                            code,
                            static_cast<SbiCommandId>(IDX(SBICMD_TOY_RESOURCE_FIRST) + i),
                            TAB_RESOURCE,
                            SbGeom(bx + 0x45, y - 0x17, bx + 0x5c, y),
                            "GAME_INGAMEICONZ_NORMCHIPZ",
                            cfgp[0],
                            0
                        )) {
                        delete set;
                        return 0;
                    }
                    AddTabItem(3, set);
                    cachep[0] = set;
                    set = new CSBI_ImageSet;
                    if (!set->SetupImage(
                            this,
                            code,
                            static_cast<SbiCommandId>(IDX(SBICMD_BRICK_RESOURCE_FIRST) + i),
                            TAB_RESOURCE,
                            SbGeom(bx + 0x6d, y - 0x17, bx + 0x84, y),
                            "GAME_INGAMEICONZ_NORMCHIPZ",
                            cfgp[24],
                            0
                        )) {
                        delete set;
                        return 0;
                    }
                    AddTabItem(3, set);
                    cachep[4] = set;
                    cfgp += 6;
                    cachep += 1;
                    y += 0x20;
                }
            }

            mach = new CSBI_GruntMachine;
            if (!mach->BuildResourceTabStatusBar(
                    this,
                    code,
                    SBICMD_RESOURCE_MACHINE_BACKGROUND,
                    TAB_RESOURCE,
                    SbGeom(bx, by + 0xc8, bx + 0x9f, by + 0xfa),
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_MACHINE",
                    m_machineA.m_counter,
                    m_machineB.m_counter
                )) {
                delete mach;
                return 0;
            }
            m_machineDisplay = mach;
            AddTabItem(3, mach);

            it = new CSBI_Image;
            // Retail does not compute this one: at +0xea8 it RELOADS the slot the first
            // TAB_RESOURCE image's `by + 0x135` was CSE'd into, and `by + 0x1a6` (what
            // used to stand here) appears in no immediate anywhere in the function.
            if (!it->SetupImage(
                    this,
                    code,
                    SBICMD_RESOURCE_MACHINE_FOREGROUND,
                    TAB_RESOURCE,
                    SbGeom(bx, by + 0x135, bx + 0x9f, by + 0x1df),
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_FRAMEWORK",
                    -1,
                    0
                )) {
                delete it;
                return 0;
            }
            AddTabItem(3, it);
            m_notify1 = it;

            ani = new CSBI_ImageSetAni;
            if (!ani->Init(
                    this,
                    code,
                    SBICMD_CONVEYOR_TOP,
                    TAB_RESOURCE,
                    SbGeom(bx, by + 0x1bf, bx + 0x9f, by + 0x1cc),
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_TOPSHREDDER",
                    -1,
                    -1,
                    0x64,
                    1,
                    1
                )) {
                delete ani;
                return 0;
            }
            AddTabItem(3, ani);

            imgSet = new CSBI_ImageSet;
            if (!imgSet->SetupImage(
                    this,
                    code,
                    SBICMD_RESOURCE_FALLING_ITEM,
                    TAB_RESOURCE,
                    SbGeom(
                        m_fallRect.left + bx,
                        m_fallRect.top + by,
                        m_fallRect.right + bx,
                        m_fallRect.bottom + by
                    ),
                    "GAME_INGAMEICONZ_NORMCHIPZ",
                    m_extraNotifyArg1,
                    0
                )) {
                delete imgSet;
                return 0;
            }
            AddTabItem(3, imgSet);
            m_extraNotify1 = imgSet;
            imgSet->SetEnabled(0);

            ani = new CSBI_ImageSetAni;
            if (!ani->Init(
                    this,
                    code,
                    SBICMD_CONVEYOR_BOTTOM,
                    TAB_RESOURCE,
                    SbGeom(bx, by + 0x1c7, bx + 0x9f, by + 0x1df),
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_BOTTOMSHREDDER",
                    -1,
                    -1,
                    0x64,
                    1,
                    1
                )) {
                delete ani;
                return 0;
            }
            AddTabItem(3, ani);
            return 1;

        case TAB_MULTIPLAYER:
            it = new CSBI_Image;
            if (!it->SetupImage(
                    this,
                    code,
                    SBICMD_TAB_TITLE_TEXT,
                    TAB_MULTIPLAYER,
                    SbGeom(bx + 0x18, by + 0xaf, bx + 0x70, by + 0xbe),
                    "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_TITLETEXT",
                    -1,
                    0
                )) {
                delete it;
                return 0;
            }
            AddTabItem(4, it);

            head = new CSBI_WarlordHead;
            if (!head->SetupImage(
                    this,
                    code,
                    SBICMD_MULTIPLAYER_HEAD1,
                    TAB_MULTIPLAYER,
                    SbGeom(bx + 0x53, by + 0xcf, bx + 0x8e, by + 0x10a),
                    "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD1",
                    1,
                    0
                )) {
                delete head;
                return 0;
            }
            m_warlordHead[0] = head;
            AddTabItem(4, head);
            head = new CSBI_WarlordHead;
            if (!head->SetupImage(
                    this,
                    code,
                    SBICMD_MULTIPLAYER_HEAD2,
                    TAB_MULTIPLAYER,
                    SbGeom(bx + 0x53, by + 0x112, bx + 0x8e, by + 0x14d),
                    "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD2",
                    1,
                    0
                )) {
                delete head;
                return 0;
            }
            m_warlordHead[1] = head;
            AddTabItem(4, head);
            head = new CSBI_WarlordHead;
            if (!head->SetupImage(
                    this,
                    code,
                    SBICMD_MULTIPLAYER_HEAD3,
                    TAB_MULTIPLAYER,
                    SbGeom(bx + 0x53, by + 0x155, bx + 0x8e, by + 0x190),
                    "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD3",
                    1,
                    0
                )) {
                delete head;
                return 0;
            }
            m_warlordHead[2] = head;
            AddTabItem(4, head);
            head = new CSBI_WarlordHead;
            if (!head->SetupImage(
                    this,
                    code,
                    SBICMD_MULTIPLAYER_HEAD4,
                    TAB_MULTIPLAYER,
                    SbGeom(bx + 0x53, by + 0x197, bx + 0x8e, by + 0x1d2),
                    "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD4",
                    1,
                    0
                )) {
                delete head;
                return 0;
            }
            m_warlordHead[3] = head;
            AddTabItem(4, head);

            {
                CSBI_WarlordHead** slot = m_warlordHead;
                i32 pi = 0;
                do {
                    // Indexed, not a walking pointer: retail's exit test is the
                    // strength-reduced BYTE offset (`add eax,0x238; cmp eax,0x8e0;
                    // jl`), which cl only produces from `m_players[pi]` - a
                    // `p < m_players + 4` pointer guard reloads g_gameReg and
                    // compares unsigned against a computed limit instead.
                    GruntzPlayer* p = &g_gameReg->m_players[pi];
                    CShadeTable* sel;
                    if (p->m_joined != 0 && p->m_doneFlag == 0) {
                        sel = g_gameReg->m_spriteFactory->GetSel(IDX(p->m_color), 0);
                        if (pi == m_tabCycle) {
                            (*slot)->SetState(1);
                        }
                    } else {
                        sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
                        (*slot)->SetState(2);
                    }

                    (*slot)->ShowFrames(SHADE_PAL_16, sel);
                    slot++;
                    pi++;
                } while (pi < 4);
            }

            {
                i32 by17 = bx + 0x17;
                i32 by52 = bx + 0x52;
                i32 y = by + 0xd9;
                for (i = 0; i < 15; i++) {
                    bar = new CSBI_StatzTabGruntBar;
                    if (!bar->BuildMultiplayerTabStatusBar(
                            this,
                            code,
                            static_cast<SbiCommandId>(IDX(SBICMD_CURSOR_TARGET_FIRST) + i),
                            TAB_MULTIPLAYER,
                            SbGeom(by17, y - 0x11, by52, y),
                            "GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ",
                            m_tabCycle,
                            i,
                            0
                        )) {
                        delete bar;
                        return 0;
                    }
                    AddTabItem(4, bar);
                    y += 0x12;
                }
            }
            return 1;

        case TAB_STATZ:
            it = new CSBI_Image;
            if (!it->SetupImage(
                    this,
                    code,
                    SBICMD_TAB_TITLE_TEXT,
                    TAB_STATZ,
                    SbGeom(bx + 0x18, by + 0xaf, bx + 0x70, by + 0xbe),
                    "GAME_STATUSBAR_TABZ_STATZTAB_TITLETEXT",
                    -1,
                    0
                )) {
                delete it;
                return 0;
            }
            AddTabItem(1, it);

            {
                i32 aOff = 0xa;
                i32 cOff = 0x21;
                if (m_position == STATUSBAR_DOCK_LEFT) {
                    aOff = 0x7d;
                    cOff = 0x95;
                }
                i32 arrowL = bx + aOff;
                i32 arrowR = bx + cOff;
                i32 y = by + 0xd9;
                for (i = 0; i < 15; i++) {
                    SbiCommandId id =
                        static_cast<SbiCommandId>(IDX(SBICMD_CURSOR_TARGET_FIRST) + i);
                    arrow = new CSBI_StatzTabArrow;
                    if (!arrow->Init(
                            this,
                            code,
                            static_cast<SbiCommandId>(IDX(SBICMD_STAT_TOGGLE_FIRST) + i),
                            TAB_STATZ,
                            SbGeom(arrowL, y - 0x11, arrowR, y),
                            "GAME_STATUSBAR_TABZ_STATZTAB_ARROW",
                            -1,
                            -1,
                            0x64,
                            0,
                            0
                        )) {
                        delete arrow;
                        return 0;
                    }
                    m_statObj[i] = arrow;
                    AddTabItem(1, arrow);
                    // Both arms take the same `0` (retail pushes it BEFORE the
                    // branch); only the entry point differs - a sampled stat gets
                    // the alternate arrow art (0xea170), an unsampled one the
                    // plain one (0xea0f0).  This arm was empty, so a sampled stat
                    // toggle kept whatever direction the ctor left.
                    if (m_statFlags[i] != STATUS_SAMPLE_NONE) {
                        arrow->SetSampledDirection(m_position, 0);
                    } else {
                        arrow->SetUnsampledDirection(m_position, 0);
                    }
                    bar = new CSBI_StatzTabGruntBar;
                    if (!bar->BuildMultiplayerTabStatusBar(
                            this,
                            code,
                            id,
                            TAB_STATZ,
                            SbGeom(bx + 0x28, y - 0x11, bx + 0x77, y),
                            "GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ",
                            g_curPlayer,
                            i,
                            1
                        )) {
                        delete bar;
                        return 0;
                    }
                    AddTabItem(1, bar);
                    y += 0x12;
                }
            }
            return 1;

        case TAB_GAME:
            it = new CSBI_Image;
            if (!it->SetupImage(
                    this,
                    code,
                    SBICMD_TAB_TITLE_TEXT,
                    TAB_GAME,
                    SbGeom(bx + 0x18, by + 0xaf, bx + 0x70, by + 0xbe),
                    "GAME_STATUSBAR_TABZ_GAMETAB_TITLETEXT",
                    -1,
                    0
                )) {
                delete it;
                return 0;
            }
            AddTabItem(5, it);

            it = new CSBI_ImageSet;
            if (!it->SetupImage(
                    this,
                    code,
                    SBICMD_WARPSTONE_BASE,
                    TAB_GAME,
                    SbGeom(bx, by, bx + 0x9f, by + 0x7f),
                    "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                    1,
                    0
                )) {
                delete it;
                return 0;
            }
            AddTabItem(5, it);
            if ((static_cast<CTriggerMgr*>(g_gameReg->m_triggerMgr))
                    ->ByteTableHas(WARPSTONE_FRAGMENT_FIRST)) {
                it = new CSBI_ImageSet;
                if (!it->SetupImage(
                        this,
                        code,
                        SBICMD_WARPSTONE_FRAGMENT1,
                        TAB_GAME,
                        SbGeom(bx + 0x17, by + 0xe, bx + 0x52, by + 0x44),
                        "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                        2,
                        0
                    )) {
                    delete it;
                    return 0;
                }
                AddTabItem(5, it);
                if ((static_cast<CTriggerMgr*>(g_gameReg->m_triggerMgr))
                        ->ByteTableHas(WARPSTONE_FRAGMENT_SECOND)) {
                    it = new CSBI_ImageSet;
                    if (!it->SetupImage(
                            this,
                            code,
                            SBICMD_WARPSTONE_FRAGMENT2,
                            TAB_GAME,
                            SbGeom(bx + 0x4c, by + 0xf, bx + 0x87, by + 0x3e),
                            "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                            3,
                            0
                        )) {
                        delete it;
                        return 0;
                    }
                    AddTabItem(5, it);
                    if ((static_cast<CTriggerMgr*>(g_gameReg->m_triggerMgr))
                            ->ByteTableHas(WARPSTONE_FRAGMENT_THIRD)) {
                        it = new CSBI_ImageSet;
                        if (!it->SetupImage(
                                this,
                                code,
                                SBICMD_WARPSTONE_FRAGMENT3,
                                TAB_GAME,
                                SbGeom(bx + 0x1b, by + 0x3b, bx + 0x52, by + 0x71),
                                "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                                4,
                                0
                            )) {
                            delete it;
                            return 0;
                        }
                        AddTabItem(5, it);
                        if ((static_cast<CTriggerMgr*>(g_gameReg->m_triggerMgr))
                                ->ByteTableHas(WARPSTONE_FRAGMENT_FOURTH)) {
                            it = new CSBI_ImageSet;
                            if (!it->SetupImage(
                                    this,
                                    code,
                                    SBICMD_WARPSTONE_FRAGMENT4,
                                    TAB_GAME,
                                    SbGeom(bx + 0x4a, by + 0x35, bx + 0x89, by + 0x74),
                                    "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                                    5,
                                    0
                                )) {
                                delete it;
                                return 0;
                            }
                            AddTabItem(5, it);
                        }
                    }
                }
            }
            BuildGameMenu();
            return 1;
    }
    return 1;
}

RVA_COMPGEN(0x001047c0, 0x1e, ??_GCSBI_ImageSetAni@@UAEPAXI@Z)
RVA_COMPGEN(0x001047f0, 0x94, ??1CSBI_ImageSetAni@@UAE@XZ)

RVA_COMPGEN(0x001048c0, 0x1e, ??_GCSBI_StatzTabArrow@@UAEPAXI@Z)
RVA(0x001048f0, 0xa9)
CSBI_StatzTabArrow::~CSBI_StatzTabArrow() {
    Reset();
}

RVA_COMPGEN(0x00104cb0, 0x1e, ??_GCSBI_GruntMachine@@UAEPAXI@Z)

RVA(0x00104ce0, 0x55)
CSBI_GruntMachine::~CSBI_GruntMachine() {
    Reset();
}

RVA(0x00104d60, 0x48)
i32 CStatusBarMgr::TryActivate() {

    if (m_position == STATUSBAR_HIDDEN) {
        return Activate();
    }
    if (!BuildStatusBarTabs()) {
        g_gameReg->ReportError(kActivateErrId, kActivateErrTag);
        return 0;
    }
    SetTabState(static_cast<SbiCommandId>(IDX(m_activeTab)), MENUITEM_SELECTED);
    return 1;
}

RVA(0x00104dd0, 0x6b)
i32 CStatusBarMgr::Activate() {
    if (m_barSprite != NULL) {
        return 0;
    }
    i32 w = g_gameReg->m_modeSize.cx;
    i32 d = g_gameReg->m_modeSize.cy;
    if (m_barX > w - 0x22) {
        m_barX = w - 0x22;
    }
    if (m_barY > d - 9) {
        m_barY = d - 0x22;
    }
    m_barSprite =
        (m_world)
            ->m_childGroup->CreateSprite(0, m_barX, m_barY, SORTKEY_OVERLAY, "StatusBarSprite", 1);
    return m_barSprite != NULL;
}

RVA(0x00104e60, 0xed)
i32 CStatusBarMgr::LoadStatzTabToggleSprite(i32 idx, StatusSampleMode mode) {
    if (m_statFlags[idx] == mode) {
        return 1;
    }

    i32 slot = idx + 15 * g_curPlayer;
    if (g_gameReg->m_triggerMgr->m_units[slot] == NULL) {
        return 0;
    }

    CSBI_SideTab* r = m_hitRects[idx];
    if (r != NULL) {
        r->m_sampleMode = mode;
        r->SetEnabled(1);
        if (m_activeTab == TAB_STATZ) {

            m_statObj[idx]->SetSampledDirection(m_position, 1);
            SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
            if (registry->m_silentMode == 0) {
                SoundCue* found = NULL;
                CMapStringToPtr* map = &registry->m_cues;
                MapLookup(*map, "GAME_STATZTABTOGGLE", found);
                if (found) {
                    i32 soundEnabled = g_soundEnabled;
                    i32 volumePercent = g_soundVolumePercent;
                    if (soundEnabled != 0) {
                        SoundCue* p = found;
                        if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                            >= static_cast<u32>(p->m_replayDelayMs)) {
                            p->m_lastPlayTimeMs = g_soundCueTimeMs;
                            p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                        }
                    }
                }
            }
        }
    }
    m_statFlags[idx] = mode;
    return 1;
}

RVA(0x00104f90, 0xa8)
i32 CStatusBarMgr::ClearStat(i32 idx) {
    CSBI_SideTab* r = m_hitRects[idx];
    if (r != NULL) {
        r->m_sampleMode = STATUS_SAMPLE_NONE;
        r->SetEnabled(0);
        if (m_activeTab == TAB_STATZ) {

            m_statObj[idx]->SetUnsampledDirection(m_position, 1);
            SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
            if (registry->m_silentMode == 0) {
                SoundCue* found = NULL;
                CMapStringToPtr* map = &registry->m_cues;
                MapLookup(*map, "GAME_STATZTABTOGGLE", found);
                if (found) {
                    i32 soundEnabled = g_soundEnabled;
                    i32 volumePercent = g_soundVolumePercent;
                    if (soundEnabled != 0) {
                        SoundCue* p = found;
                        if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                            >= static_cast<u32>(p->m_replayDelayMs)) {
                            p->m_lastPlayTimeMs = g_soundCueTimeMs;
                            p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                        }
                    }
                }
            }
        }
    }
    m_statFlags[idx] = STATUS_SAMPLE_NONE;
    return 1;
}

RVA(0x00105070, 0x10e)
i32 CStatusBarMgr::BuildSideTabs() {
    i32 i = 0;
    for (i32 strid = 0xd9; strid < 0x1e7; strid += 0x12) {
        RECT rc;
        if (m_position == STATUSBAR_DOCK_RIGHT) {
            rc.left = m_barRect.left - 0x1c;
            rc.right = m_barRect.left;
        } else {
            rc.left = m_barRect.right;
            rc.right = m_barRect.right + 0x1c;
        }
        rc.top = strid - 0x11;
        rc.bottom = strid;
        CSBI_SideTab* newobj = new CSBI_SideTab;

        i32 ok = newobj->BuildStatzTabStatusBar(
            this,
            g_gameReg->m_world,
            static_cast<SbiCommandId>(IDX(SBICMD_SIDE_TAB_FIRST) + i),
            TAB_CONTROLS,
            rc,
            "GAME_STATUSBAR_TABZ_STATZTAB_TABONLEFT",
            g_curPlayer,
            i,
            m_statFlags[i],
            m_position == STATUSBAR_DOCK_RIGHT
        );
        if (ok == 0) {
            delete newobj;
            return 0;
        }
        AddTabItem(0, newobj);
        m_hitRects[i] = newobj;
        i++;
    }
    return 1;
}

RVA(0x00105280, 0x61)
i32 CStatusBarMgr::HitTest(i32 x, i32 y) {
    if (m_chatBoxDisabled == 0) {
        for (i32 i = 0; i < 15; i++) {
            if (m_hitRects[i] && m_hitRects[i]->m_enabled) {
                CSBI_SideTab* p = m_hitRects[i];
                i32 hit = p->m_enabled ? CGameLevel::PointInRect(&p->m_rect, x, y) : 0;
                if (hit) {
                    return i;
                }
            }
        }
    }
    return -1;
}

// @early-stop
// Sub-point scheduling residue only; probe-inert (C1 state).
RVA(0x00105310, 0x11a)
void CStatusBarMgr::UpdateGruntOvenStatusBar() {

    CSBI_ImageSet** slot = m_slotNotify;
    CSbiSlot* tab = m_slots;
    i32 n = 5;
    do {
        if (tab->m_state == SLOT_FILLING) {
            i64 d = static_cast<i64>(g_frameTime) - tab->m_startTime;

            i32 elapsed = (d < 0) ? 0 : static_cast<i32>(d);
            u32 delay = g_buteMgr.GetDwordDef("StatusBar", "GruntOvenDelay", 0xc8);
            i32 frame = static_cast<i32>((static_cast<u32>(elapsed) / delay)) + 1;
            if (frame >= 0x1a) {
                tab->m_state = SLOT_READY;
                frame = 0x1a;
                SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
                if (registry->m_silentMode == 0) {
                    SoundCue* found = NULL;
                    CMapStringToPtr* map = &registry->m_cues;
                    MapLookup(*map, "GAME_COOKINGCOMPLETE", found);
                    if (found) {
                        i32 soundEnabled = g_soundEnabled;
                        i32 volumePercent = g_soundVolumePercent;
                        if (soundEnabled != 0) {
                            SoundCue* p = found;
                            if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                                >= static_cast<u32>(p->m_replayDelayMs)) {
                                p->m_lastPlayTimeMs = g_soundCueTimeMs;
                                p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                            }
                        }
                    }
                }
            }
            if (frame != tab->m_value) {
                tab->m_value = frame;
                CSBI_ImageSet* w = *slot;
                if (w) {
                    w->Notify(frame);
                }
            }
        }
        ++slot;
        ++tab;
    } while (--n != 0);
}

RVA(0x00105480, 0x7d)
void CStatusBarMgr::TickGauge() {
    i32 changed = 0;
    i32 g = m_gauge;
    i32 t = m_gaugeTarget;
    if (g < t) {
        g++;
    } else if (g <= t) {
        goto noChange;
    } else {
        g--;
    }
    m_gauge = g;
    changed = 1;
noChange:;
    if (m_gauge == SBI_GAUGE_FULL) {
        if (AnySlotActive()) {
            changed = 1;
            SetGauge(SBI_GAUGE_EMPTY);
        }
    }
    if (changed) {
        if (m_gaugeSink && m_gaugeNotify) {
            m_gaugeNotify->RequestRedraw();
            i32 fill = m_gauge;
            CSBI_WellGoo* sink = m_gaugeSink;
            sink->m_fillScale = fill;
            sink->RequestRedraw();
        }
    }
}

RVA(0x00105520, 0x21)
void CStatusBarMgr::ResetSlots() {
    for (i32 i = 0; i < 5; i++) {
        ArmSlot(i);
    }
    m_activeSlot = -1;
}

RVA(0x00105560, 0x33)
void CStatusBarMgr::ArmSlot(i32 idx) {
    m_slots[idx].m_state = SLOT_ARMED;
    m_slots[idx].m_value = 1;
    if (m_slotNotify[idx]) {
        m_slotNotify[idx]->Notify(1);
    }
}

RVA(0x001055b0, 0x109)
i32 CStatusBarMgr::LoadGooCookingSprite(i32 idx) {
    CSbiSlot* sp = &m_slots[idx];
    if (sp->m_state != SLOT_ARMED) {
        return 0;
    }
    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ && m_hlBusy == 0) {
        if (m_position == STATUSBAR_HIDDEN) {
            RestoreStatusBar();
        }
        if (m_activeTab != TAB_GRUNTZ) {
            SetTabState(SBICMD_TAB_GRUNTZ, MENUITEM_SELECTED);
        }
        Deactivate();
    }
    sp->m_state = SLOT_FILLING;

    // The slot tail is the {startTime, interval} clock pair SyncClockPair walks;
    // retail arms it through one pair pointer (a fresh `&m_slots[idx]` lea with
    // the member offset index-folded), not through sp's member displacements.
    i64* clock = &m_slots[idx].m_startTime;
    clock[1] = INT_MAX;
    clock[0] = g_frameTime;
    if (m_activeTab == TAB_GRUNTZ && m_position != STATUSBAR_HIDDEN) {
        SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
        if (registry->m_silentMode == 0) {
            SoundCue* found = NULL;
            CMapStringToPtr* map = &registry->m_cues;
            MapLookup(*map, "GAME_GOOCOOKING1", found);
            if (found) {
                i32 soundEnabled = g_soundEnabled;
                i32 volumePercent = g_soundVolumePercent;
                if (soundEnabled != 0) {
                    SoundCue* p = found;
                    if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                        >= static_cast<u32>(p->m_replayDelayMs)) {
                        p->m_lastPlayTimeMs = g_soundCueTimeMs;
                        p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                    }
                }
            }
        }
    }
    return 1;
}

RVA(0x00105710, 0x23)
i32 CStatusBarMgr::AnySlotActive() {
    for (i32 i = 0; i < 5; i++) {
        if (LoadGooCookingSprite(i)) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00105750, 0x1f)
void CStatusBarMgr::AdvanceGauge(i32 delta) {
    i32 v = m_gauge + delta;
    if (v >= SBI_GAUGE_FULL) {
        v = SBI_GAUGE_FULL;
    }
    m_gaugeTarget = v;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00105780, 0x1f)
void CStatusBarMgr::DrainGauge(i32 delta) {
    m_gaugeTarget = m_gauge - delta > SBI_GAUGE_EMPTY ? m_gauge - delta : SBI_GAUGE_EMPTY;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001057b0, 0xd)
void CStatusBarMgr::SetGaugeTarget(i32 value) {
    m_gaugeTarget = value;
}

RVA(0x001057d0, 0x13)
void CStatusBarMgr::SetGauge(i32 value) {
    m_gaugeTarget = value;
    m_gauge = value;
}

// @early-stop
// One scheduling slot: retail emits `mov ecx,edi` (the LoadCameraSprite receiver)
// after the first m_cameraTargetIdentity store, cl before it. 5 spellings measured.
RVA(0x00105800, 0x9e)
i32 CStatusBarMgr::PlaceCursorTarget(i32 unitIndex, i32 activateCamera) {
    i32 playerIndex = g_curPlayer;
    if (g_gameReg->m_triggerMgr->ResetCell(playerIndex, unitIndex, 0, 0) != 0) {

        CGrunt* entry =
            g_gameReg->m_triggerMgr->m_units[unitIndex + playerIndex * TM_UNITS_PER_PLAYER];
        if (entry != NULL) {
            (static_cast<CPlay*>(g_gameReg->m_curState))
                ->ResetGoals(entry->m_object->m_screenX, entry->m_object->m_screenY);
            if (activateCamera != 0) {
                CTriggerMgr* obj = g_gameReg->m_triggerMgr;
                if (obj->RecordListHas(playerIndex, unitIndex)) {
                    obj->m_cameraTargetIdentity.m_x = playerIndex;
                    obj->m_cameraTargetIdentity.m_y = unitIndex;
                    obj->m_armed = 1;
                    obj->LoadCameraSprite();
                }
            }
            return 1;
        }
    }
    return 0;
}

RVA(0x001058d0, 0x34)
void CStatusBarMgr::UpdateStatusSystems() {
    UpdateGruntOvenStatusBar();
    TickGauge();
    UpdateRezConveyorStatusBar();
    LoadRezMachineConfig();
    LoadChipMachineConfig();
    UpdateChipGrinderStatusBar();
    UpdateDestructWarningAnimation();
}

RVA(0x00105920, 0x47)
void CStatusBarMgr::Reset() {
    ResetSlots();
    m_gaugeTarget = SBI_GAUGE_EMPTY;
    m_gauge = SBI_GAUGE_EMPTY;
    ResetConveyorBelts();
    UpdateRezMachineSnoozeStatusBar();
    InitTabRects();
    m_destructButtonFrame = DESTRUCT_FRAME_IDLE;
    m_destructWarningState = DESTRUCT_WARNING_INACTIVE;
}

RVA(0x00105990, 0x3b4)
void CStatusBarMgr::UpdateRezConveyorStatusBar() {
    for (i32 i = 0; i < 3; i++) {
        i64* clock = &m_conveyorSlots[i].m_last;
        SbiHlRowState state = static_cast<SbiHlRowState>(m_conveyorSlots[i].m_state);
        switch (state) {
            case HLROW_IDLE_CYCLE:
                if (++m_conveyorSlots[i].m_counter > 9) {
                    m_conveyorSlots[i].m_counter = 1;
                }
                break;
            case HLROW_RAMP_UP_LOW:
                if (static_cast<i64>(g_frameTime) - clock[0] >= clock[1]) {
                    if (++m_conveyorSlots[i].m_counter >= 0x12) {
                        m_conveyorSlots[i].m_counter = 0x12;
                        m_conveyorSlots[i].m_state = IDX(HLROW_HOLD_LOW);
                        clock[1] =
                            g_buteMgr.GetDwordDef("StatusBar", "ConveyorBeltHoldDelay", 0x1f4);
                        clock[0] = static_cast<u32>(g_frameTime);
                        UpdateFallingItemStatusBar(
                            m_extraNotifyArg0,
                            m_itemRect.left + 0xc,
                            m_itemRect.top + 0xc
                        );
                        StartChipMachineCycle();
                    }
                }
                break;
            case HLROW_RAMP_DOWN_LOW:
                if (static_cast<i64>(g_frameTime) - clock[0] >= clock[1]) {
                    if (--m_conveyorSlots[i].m_counter < 0xa) {
                        m_conveyorSlots[i].m_state = IDX(HLROW_OFF);
                        m_conveyorSlots[i].m_counter = 1;
                    }
                }
                break;
            case HLROW_RAMP_UP_HIGH:
                if (static_cast<i64>(g_frameTime) - clock[0] >= clock[1]) {
                    if (++m_conveyorSlots[i].m_counter >= 0x18) {
                        m_conveyorSlots[i].m_counter = 0x18;
                        m_conveyorSlots[i].m_state = IDX(HLROW_HOLD_HIGH);
                        clock[1] =
                            g_buteMgr.GetDwordDef("StatusBar", "ConveyorBeltHoldInDelay", 0x1f4);
                        clock[0] = static_cast<u32>(g_frameTime);
                        m_machinePhase = BELT_FALLING_OFF;
                        m_beltClock.m_interval =
                            g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
                        m_beltClock.m_last = static_cast<u32>(g_frameTime);
                    }
                }
                break;
            case HLROW_RAMP_DOWN_HIGH:
                if (static_cast<i64>(g_frameTime) - clock[0] >= clock[1]) {
                    if (--m_conveyorSlots[i].m_counter < 0x13) {
                        m_conveyorSlots[i].m_state = IDX(HLROW_OFF);
                        m_conveyorSlots[i].m_counter = 1;
                    }
                }
                break;
            case HLROW_HOLD_HIGH:
                if (static_cast<i64>(g_frameTime) - clock[0] >= clock[1]) {
                    if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                        SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
                        if (registry->m_silentMode == 0) {
                            SoundCue* found = NULL;
                            MapLookup(registry->m_cues, "GAME_REZBELTRETURN", found);
                            if (found) {
                                i32 soundEnabled = g_soundEnabled;
                                i32 volumePercent = g_soundVolumePercent;
                                if (soundEnabled != 0) {
                                    SoundCue* p = found;
                                    if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                                        >= static_cast<u32>(p->m_replayDelayMs)) {
                                        p->m_lastPlayTimeMs = g_soundCueTimeMs;
                                        p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                                    }
                                }
                            }
                        }
                    }
                    m_conveyorSlots[i].m_state = IDX(HLROW_RAMP_DOWN_HIGH);
                }
                break;
            case HLROW_HOLD_LOW:
                if (static_cast<i64>(g_frameTime) - clock[0] >= clock[1]) {
                    if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                        SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
                        if (registry->m_silentMode == 0) {
                            SoundCue* found = NULL;
                            MapLookup(registry->m_cues, "GAME_REZBELTBACKUP", found);
                            if (found) {
                                i32 soundEnabled = g_soundEnabled;
                                i32 volumePercent = g_soundVolumePercent;
                                if (soundEnabled != 0) {
                                    SoundCue* p = found;
                                    if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                                        >= static_cast<u32>(p->m_replayDelayMs)) {
                                        p->m_lastPlayTimeMs = g_soundCueTimeMs;
                                        p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                                    }
                                }
                            }
                        }
                    }
                    m_conveyorSlots[i].m_state = IDX(HLROW_RAMP_DOWN_LOW);
                }
                break;
        }
        if (m_conveyorSprites[i]) {
            m_conveyorSprites[i]->Notify(m_conveyorSlots[i].m_counter);
        }
    }
}

RVA(0x00105e40, 0x63c)
void CStatusBarMgr::LoadRezMachineConfig() {
    CSbiHlRow* pA = &m_machineB;
    CSbiHlRow* pB = &m_machineA;
    switch (static_cast<SbiMachineState>(pA->m_state)) {
        case MACHINE_RIGHT_RUNNING:
            if (static_cast<i64>(g_frameTime) - pA->m_last >= pA->m_interval) {
                if (++pA->m_counter > 0x34) {
                    SetRightRezMachineAnimation(
                        0x2b,
                        MACHINE_RIGHT_RUNNING,
                        g_buteMgr.GetDwordDef("StatusBar", "RightMachineRunningDelay", 0x7d)
                    );
                } else {
                    pA->m_interval =
                        g_buteMgr.GetDwordDef("StatusBar", "RightMachineRunningDelay", 0x7d);
                    pA->m_last = static_cast<u32>(g_frameTime);
                }
            }
            break;
        case MACHINE_RIGHT_SPEWING:
            if (static_cast<i64>(g_frameTime) - pA->m_last >= pA->m_interval) {
                if (++pA->m_counter > 0x44) {
                    SetRightRezMachineAnimation(0x2b, MACHINE_STOPPED, INT_MAX);
                } else {
                    pA->m_interval =
                        g_buteMgr.GetDwordDef("StatusBar", "RightMachineSpewingDelay", 0x7d);
                    pA->m_last = static_cast<u32>(g_frameTime);
                }
            }
            break;
    }

    switch (static_cast<SbiMachineState>(pB->m_state)) {
        case MACHINE_SNOOZING:
            if (static_cast<i64>(g_frameTime) - pB->m_last >= pB->m_interval) {
                if (++pB->m_counter > 8) {
                    SetLeftRezMachineAnimation(
                        1,
                        MACHINE_SNOOZING,
                        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineSnoozingDelay", 0x64)
                    );
                } else {
                    pB->m_interval =
                        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineSnoozingDelay", 0x64);
                    pB->m_last = static_cast<u32>(g_frameTime);
                }
            }
            break;
        case MACHINE_WAKING:
            if (static_cast<i64>(g_frameTime) - pB->m_last >= pB->m_interval) {
                if (++pB->m_counter > 0x13) {
                    SetLeftRezMachineAnimation(
                        0x14,
                        MACHINE_TURNING_WHEEL,
                        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineTurningWheelDelay", 0x64)
                    );
                    SetRightRezMachineAnimation(
                        0x2b,
                        MACHINE_RIGHT_RUNNING,
                        g_buteMgr.GetDwordDef("StatusBar", "RightMachineRunningDelay", 0x7d)
                    );
                    for (i32 i = 0; i < 3; i++) {
                        m_conveyorSlots[i].m_state = IDX(HLROW_IDLE_CYCLE);
                        m_conveyorSlots[i].m_value = 1;
                    }
                    m_machinePhase = BELT_IN_MACHINE;
                    i64* belt = &m_beltClock.m_last;
                    belt[1] = g_buteMgr.GetDwordDef("StatusBar", "NextItemDelay", 0x64);
                    belt[0] = static_cast<u32>(g_frameTime);
                    if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                        SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
                        if (registry->m_silentMode == 0) {
                            SoundCue* found = NULL;
                            MapLookup(registry->m_cues, "GAME_REZMACHINE", found);
                            if (found) {
                                i32 soundEnabled = g_soundEnabled;
                                i32 volumePercent = g_soundVolumePercent;
                                if (soundEnabled != 0) {
                                    SoundCue* p = found;
                                    if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                                        >= static_cast<u32>(p->m_replayDelayMs)) {
                                        p->m_lastPlayTimeMs = g_soundCueTimeMs;
                                        p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                                    }
                                }
                            }
                        }
                    }
                } else {
                    pB->m_interval =
                        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineWakingDelay", 0x64);
                    pB->m_last = static_cast<u32>(g_frameTime);
                }
            }
            break;
        case MACHINE_TURNING_WHEEL:
            if (static_cast<i64>(g_frameTime) - pB->m_last >= pB->m_interval) {
                if (++pB->m_counter > 0x1d) {
                    SetLeftRezMachineAnimation(
                        0x14,
                        MACHINE_TURNING_WHEEL,
                        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineTurningWheelDelay", 0x64)
                    );
                } else {
                    pB->m_interval =
                        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineTurningWheelDelay", 0x64);
                    pB->m_last = static_cast<u32>(g_frameTime);
                }
            }
            break;
        case MACHINE_LEVER:
            if (static_cast<i64>(g_frameTime) - pB->m_last >= pB->m_interval) {
                if (++pB->m_counter == MACHINE_LEVER_RELEASE_FRAME) {
                    i32 found = 0;
                    i32 r = 3;
                    i32 col;
                    PickupType which = static_cast<PickupType>(m_extraNotifyArg0);
                    if (which >= PICKUP_BRICKZ_FIRST) {
                        col = 2;
                    } else {
                        col = (which >= PICKUP_TOYZ_FIRST) ? 1 : 0;
                    }
                    while (found == 0) {
                        if (r < 0) {
                            break;
                        }
                        if (m_resourceSlots[col * 4 + r].m_state == IDX(HLROW_OFF)) {
                            found = 1;
                        } else {
                            r--;
                        }
                    }
                    if (found) {
                        m_conveyorSlots[col].m_state = IDX(HLROW_RAMP_UP_HIGH);
                        m_conveyorSlots[col].m_counter = 0x13;
                        if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                            SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
                            if (registry->m_silentMode == 0) {
                                SoundCue* fnd = NULL;
                                MapLookup(registry->m_cues, "GAME_REZBELTRETRACT", fnd);
                                if (fnd) {
                                    i32 soundEnabled = g_soundEnabled;
                                    i32 volumePercent = g_soundVolumePercent;
                                    if (soundEnabled != 0) {
                                        SoundCue* p = fnd;
                                        if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                                            >= static_cast<u32>(p->m_replayDelayMs)) {
                                            p->m_lastPlayTimeMs = g_soundCueTimeMs;
                                            p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        m_conveyorSlots[col].m_state = IDX(HLROW_RAMP_UP_LOW);
                        m_conveyorSlots[col].m_counter = 0xa;
                        if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                            SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
                            if (registry->m_silentMode == 0) {
                                SoundCue* fnd = NULL;
                                MapLookup(registry->m_cues, "GAME_REZBELTDROP", fnd);
                                if (fnd) {
                                    i32 soundEnabled = g_soundEnabled;
                                    i32 volumePercent = g_soundVolumePercent;
                                    if (soundEnabled != 0) {
                                        SoundCue* p = fnd;
                                        if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                                            >= static_cast<u32>(p->m_replayDelayMs)) {
                                            p->m_lastPlayTimeMs = g_soundCueTimeMs;
                                            p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    i64* rowClock = &m_conveyorSlots[col].m_last;
                    rowClock[1] = g_buteMgr.GetDwordDef("StatusBar", "ConveyorBeltDelay", 0x64);
                    rowClock[0] = static_cast<u32>(g_frameTime);
                }
                // Retail's `cmp eax,0x26 / jne 0x1063e8` skips ONLY the release block:
                // the counter re-read and this if/else are common to both paths.
                if (pB->m_counter > 0x2a) {
                    SetLeftRezMachineAnimation(
                        1,
                        MACHINE_SNOOZING,
                        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineSnoozingDelay", 0x64)
                    );
                } else {
                    pB->m_interval =
                        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineLeverDelay", 0x64);
                    pB->m_last = static_cast<u32>(g_frameTime);
                }
            }
            break;
    }

    if (m_machineDisplay) {
        m_machineDisplay->SetFrames(pB->m_counter, pA->m_counter);
    }
}

RVA(0x00106610, 0x3b)
void CStatusBarMgr::ResetConveyorBelts() {
    for (i32 i = 0; i < 3; i++) {
        m_conveyorSlots[i].m_state = IDX(HLROW_OFF);
        m_conveyorSlots[i].m_value = 1;
        if (m_conveyorSprites[i]) {
            m_conveyorSprites[i]->Notify(-1);
        }
    }
}

RVA(0x00106660, 0x68)
void CStatusBarMgr::UpdateRezMachineSnoozeStatusBar() {
    SetLeftRezMachineAnimation(
        1,
        MACHINE_SNOOZING,
        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineSnoozingDelay", 100)
    );
    SetRightRezMachineAnimation(0x2b, MACHINE_STOPPED, INT_MAX);
    if (m_machineDisplay) {
        m_machineDisplay->SetFrames(m_machineA.m_counter, m_machineB.m_counter);
    }
    m_rezActive = 0;
    m_rezTick = 0;
}

RVA(0x001066f0, 0x3b)
void CStatusBarMgr::SetLeftRezMachineAnimation(
    i32 initialFrame,
    SbiMachineState state,
    i32 frameDelayMs
) {
    i64* clock = &m_machineA.m_last;
    m_machineA.m_counter = initialFrame;
    m_machineA.m_state = IDX(state);
    clock[1] = static_cast<u32>(frameDelayMs);
    clock[0] = g_frameTime;
}

RVA(0x00106740, 0x3b)
void CStatusBarMgr::SetRightRezMachineAnimation(
    i32 initialFrame,
    SbiMachineState state,
    i32 frameDelayMs
) {
    i64* clock = &m_machineB.m_last;
    m_machineB.m_counter = initialFrame;
    m_machineB.m_state = IDX(state);
    clock[1] = static_cast<u32>(frameDelayMs);
    clock[0] = g_frameTime;
}

RVA(0x00106790, 0x62)
void CStatusBarMgr::CommitSlot(i32 active) {
    if (active) {
        ArmSlot(m_activeSlot);
        m_activeSlot = -1;
    } else {
        m_slots[m_activeSlot].m_value = kSlotCommitLevel;
        if (m_slotNotify[m_activeSlot]) {
            m_slotNotify[m_activeSlot]->Notify(m_slots[m_activeSlot].m_value);
        }
        m_activeSlot = -1;
    }
}

RVA(0x00106820, 0xa8)
void CStatusBarMgr::EnterHlRow(i32 shift, i32 key) {
    if (m_pendingHlRow == STATUS_HL_ROW_NONE) {
        return;
    }
    PickupType item = static_cast<PickupType>(key);
    i32 group;
    if (item >= PICKUP_BRICKZ_FIRST) {
        group = 2;
    } else {
        group = (item >= PICKUP_TOYZ_FIRST);
    }
    if (shift != 0) {
        ClearHlCell(group, m_pendingHlRow);
        for (i32 row = IDX(m_pendingHlRow) - 1; row >= 0; row--) {
            CSbiHlRow* cell = &m_resourceSlots[row + group * 4];
            if (cell->m_state == IDX(HLROW_IDLE_CYCLE)) {
                m_resourceSlots[row + group * 4 + 1].m_state = IDX(HLROW_IDLE_CYCLE);
                cell[1].m_value = cell->m_value;
                cell->m_state = IDX(HLROW_OFF);
                cell->m_value = 0;
            }
        }
    } else {
        m_resourceSlots[IDX(m_pendingHlRow) + group * 4].m_value = key;
    }
    NotifyAllSlots();
    m_pendingHlRow = STATUS_HL_ROW_NONE;
}

RVA(0x00106900, 0x8d)
void CStatusBarMgr::InitTabRects() {
    for (i32 i = 0; i < 4; i++) {
        StatusBarHighlightRow row = static_cast<StatusBarHighlightRow>(i);
        ClearHlCell(0, row);
        ClearHlCell(1, row);
        ClearHlCell(2, row);
    }
    m_machinePhase = BELT_IDLE;
    m_extraNotifyArg0 = 0;
    m_fallActive = FALLING_ITEM_INACTIVE;
    m_extraNotifyArg1 = 0;
    SetRect(&m_fallRect, 0, 0, 1, 1);
    SetRect(&m_itemRect, 0x49, 0xd7, 0x61, 0xef);
    m_pendingHlRow = STATUS_HL_ROW_NONE;
}

RVA(0x001069c0, 0x2e)
void CStatusBarMgr::ClearHlCell(i32 group, StatusBarHighlightRow row) {
    i32 idx = IDX(row) + group * 4;
    m_resourceSlots[idx].m_state = IDX(HLROW_OFF);
    m_resourceSlots[idx].m_value = 0;
    NotifyAllSlots();
}

RVA(0x00106a00, 0xbf)
void CStatusBarMgr::NotifyAllSlots() {
    if (m_notify0) {
        m_notify0->RequestRedraw();
    }
    if (m_notify2) {
        m_notify2->RequestRedraw();
    }
    if (m_notify3) {
        m_notify3->RequestRedraw();
    }
    if (m_extraNotify0 && m_extraNotifyArg0) {
        m_extraNotify0->Notify(m_extraNotifyArg0);
    }

    CSBI_ImageSet** p = &m_resourceSlotSprites[4];
    i32* h = &m_resourceSlots[4].m_value;
    for (i32 n = 0; n < 4; n++) {
        if (p[-4]) {
            p[-4]->Notify(h[-24]);
        }
        if (p[0]) {
            p[0]->Notify(h[0]);
        }
        if (p[4]) {
            p[4]->Notify(h[24]);
        }
        p++;
        h += 6;
    }

    if (m_notify1) {
        m_notify1->RequestRedraw();
    }
    if (m_extraNotify1) {
        m_extraNotify1->Notify(m_extraNotifyArg1);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00106af0, 0x37)
i32 CStatusBarMgr::SetHlCellByTier(i32 handle, i32 group) {
    PickupType item = static_cast<PickupType>(handle);
    i32 row;
    if (item >= PICKUP_BRICKZ_FIRST) {
        row = 2;
    } else {
        row = (item >= PICKUP_TOYZ_FIRST);
    }
    return SetHlCell(row, handle, group);
}

RVA(0x00106b40, 0x44)
i32 CStatusBarMgr::SetHlCell(i32 row, i32 handle, i32 group) {
    i32 idx = group + row * 4;
    if (m_resourceSlots[idx].m_state != IDX(HLROW_OFF)) {
        return 0;
    }
    m_resourceSlots[idx].m_value = handle;
    m_resourceSlots[idx].m_state = IDX(HLROW_IDLE_CYCLE);
    NotifyAllSlots();
    return 1;
}

// @early-stop
// Frame 0xc against retail's 0x10: the BELT_FALLING_OFF cue block spills its
// g_soundVolumePercent copy and binds both replay-clock fields, where the two earlier
// copies of the same source keep them in registers - arm-local pressure.
RVA(0x00106bb0, 0x7d8)
void CStatusBarMgr::LoadChipMachineConfig() {
    i32 rectFlag = 0;
    i32 refreshFlag = 0;
    i64* belt = &m_beltClock.m_last;
    switch (m_machinePhase) {
        case BELT_IN_MACHINE:
            if (static_cast<i64>(g_frameTime) - belt[0] >= belt[1]) {
                m_itemRect.left += g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_itemRect.right += g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                rectFlag = 1;
                belt[1] = g_buteMgr.GetDwordDef("StatusBar", "NextItemDelay", 0x64);
                belt[0] = static_cast<u32>(g_frameTime);
            }
            if (m_itemRect.left >= 0x6d) {
                m_itemRect.left = 0x6d;
                m_itemRect.right = 0x84;
                rectFlag = 1;
                m_machinePhase = BELT_SPEWING;
                belt[1] = g_buteMgr.GetDwordDef("StatusBar", "NextItemInMachineTime", 0x7d0);
                belt[0] = static_cast<u32>(g_frameTime);
            }
            refreshFlag = 1;
            break;
        case BELT_SPEWING:
            if (static_cast<i64>(g_frameTime) - belt[0] >= belt[1]) {
                SetRightRezMachineAnimation(
                    0x35,
                    MACHINE_RIGHT_SPEWING,
                    g_buteMgr.GetDwordDef("StatusBar", "RightMachineSpewingDelay", 0x7d)
                );
                m_machinePhase = BELT_DROP_START;
                belt[1] = g_buteMgr.GetDwordDef("StatusBar", "NextItemWaitTime", 0x1f4);
                belt[0] = static_cast<u32>(g_frameTime);
            }
            break;
        case BELT_DROP_START:
            if (static_cast<i64>(g_frameTime) - belt[0] >= belt[1]) {
                m_machinePhase = BELT_FALLING;
                if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                    SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
                    if (registry->m_silentMode == 0) {
                        SoundCue* found = NULL;
                        CMapStringToPtr* map = &registry->m_cues;
                        MapLookup(*map, "GAME_CHIPFALLOUT", found);
                        if (found) {
                            i32 soundEnabled = g_soundEnabled;
                            i32 volumePercent = g_soundVolumePercent;
                            if (soundEnabled != 0) {
                                SoundCue* p = found;
                                if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                                    >= static_cast<u32>(p->m_replayDelayMs)) {
                                    p->m_lastPlayTimeMs = g_soundCueTimeMs;
                                    p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                                }
                            }
                        }
                    }
                }
                belt[1] = g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
                belt[0] = static_cast<u32>(g_frameTime);
            }
            break;
        case BELT_FALLING:
            if (static_cast<i64>(g_frameTime) - belt[0] >= belt[1]) {
                m_itemRect.top += g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 2);
                m_itemRect.bottom += g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 2);
                rectFlag = 1;
                belt[1] = g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
                belt[0] = static_cast<u32>(g_frameTime);
            }
            if (m_itemRect.bottom >= 0x11c) {
                m_itemRect.bottom = 0x11c;
                m_itemRect.top = 0x104;
                rectFlag = 1;
                if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                    SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
                    if (registry->m_silentMode == 0) {
                        SoundCue* found = NULL;
                        CMapStringToPtr* map = &registry->m_cues;
                        MapLookup(*map, "GAME_CHIPLAND", found);
                        if (found) {
                            i32 soundEnabled = g_soundEnabled;
                            i32 volumePercent = g_soundVolumePercent;
                            if (soundEnabled != 0) {
                                SoundCue* p = found;
                                if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                                    >= static_cast<u32>(p->m_replayDelayMs)) {
                                    p->m_lastPlayTimeMs = g_soundCueTimeMs;
                                    p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                                }
                            }
                        }
                    }
                }
                m_machinePhase = BELT_TRAVELLING;
                belt[1] = g_buteMgr.GetDwordDef("StatusBar", "NextItemDelay", 0x64);
                belt[0] = static_cast<u32>(g_frameTime);
                PickupType activeItem = static_cast<PickupType>(m_extraNotifyArg0);
                if (activeItem >= PICKUP_BRICKZ_FIRST) {
                    m_itemBaseX = 0x6d;
                } else if (activeItem >= PICKUP_TOYZ_FIRST) {
                    m_itemBaseX = 0x45;
                } else {
                    m_itemBaseX = 0x1d;
                }
            }
            refreshFlag = 1;
            break;
        case BELT_TRAVELLING:
            if (static_cast<i64>(g_frameTime) - belt[0] >= belt[1]) {
                m_itemRect.left -= g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_itemRect.right -= g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                rectFlag = 1;
                belt[1] = g_buteMgr.GetDwordDef("StatusBar", "NextItemDelay", 0x64);
                belt[0] = static_cast<u32>(g_frameTime);
            }
            if (m_itemRect.left <= m_itemBaseX) {
                m_itemRect.left = m_itemBaseX;
                m_itemRect.right = m_itemBaseX + 0x17;
                rectFlag = 1;
                ResetConveyorBelts();
                SetLeftRezMachineAnimation(
                    0x1e,
                    MACHINE_LEVER,
                    g_buteMgr.GetDwordDef("StatusBar", "LeftMachineLeverDelay", 0x64)
                );
                m_machinePhase = BELT_IDLE;
            }
            refreshFlag = 1;
            break;
        case BELT_FALLING_OFF: {
            if (static_cast<i64>(g_frameTime) - belt[0] >= belt[1]) {
                m_itemRect.top += g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 2);
                // RETAIL'S TYPO, kept: the shipped EXE's string table carries both
                // "(FallingItemSpeed" and "FallingItemSpeed" as separate strings, so
                // this lookup always misses and takes the default. Correcting it would
                // drop a string from .rdata and stop matching.
                m_itemRect.bottom += g_buteMgr.GetIntDef("StatusBar", "(FallingItemSpeed", 2);
                rectFlag = 1;
                belt[1] = g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
                belt[0] = static_cast<u32>(g_frameTime);
            }
            i32 col;
            PickupType item2 = static_cast<PickupType>(m_extraNotifyArg0);
            if (item2 >= PICKUP_BRICKZ_FIRST) {
                col = 2;
            } else {
                col = (item2 >= PICKUP_TOYZ_FIRST) ? 1 : 0;
            }
            i32 row;
            CSbiHlRow* cell = &m_resourceSlots[col * 4 + 3];
            // The row guard is the LOOP CONDITION, not a trailing `if (row < 0) break;`:
            // cl rotates `for (row = 3; row >= 0; ...)` into retail's `jne exit` / `jge
            // top` pair, where a `while (state == IDLE)` head duplicates the state test
            // at the bottom (the extra branch).
            for (row = 3; row >= 0; row--, cell--) {
                if (cell->m_state != IDX(HLROW_IDLE_CYCLE)) {
                    break;
                }
            }
            if (m_itemRect.top >= row * 0x20 + 0x13e) {
                if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                    SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
                    if (registry->m_silentMode == 0) {
                        SoundCue* found = NULL;
                        CMapStringToPtr* map = &registry->m_cues;
                        MapLookup(*map, "GAME_CHIPLAND", found);
                        if (found) {
                            i32 soundEnabled = g_soundEnabled;
                            i32 volumePercent = g_soundVolumePercent;
                            if (soundEnabled != 0) {
                                SoundCue* p = found;
                                if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                                    >= static_cast<u32>(p->m_replayDelayMs)) {
                                    p->m_lastPlayTimeMs = g_soundCueTimeMs;
                                    p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                                }
                            }
                        }
                    }
                }
                SetHlCell(col, m_extraNotifyArg0, row);
                StartChipMachineCycle();
            }
            refreshFlag = 1;
            break;
        }
    }

    CSBI_ImageSet* w = m_extraNotify0;
    if (w) {
        if (rectFlag) {
            RECT rc;
            i32 x = m_barRect.left;
            i32 y = m_barRect.top;
            rc.left = m_itemRect.left + x;
            rc.top = m_itemRect.top + y;
            rc.right = m_itemRect.right + x;
            rc.bottom = m_itemRect.bottom + y;
            w->m_rect = rc;
        }
        if (refreshFlag) {
            NotifyAllSlots();
        }
    }
}

// @early-stop
// Frame and instruction selection are retail's; the residue is which callee-saved
// pair the x/y parameters take (retail edi/ebx, cl ebx/ebp), which rotates every
// operand below the GetDwordDef call.
RVA(0x00107590, 0xc4)
i32 CStatusBarMgr::UpdateFallingItemStatusBar(i32 item, i32 x, i32 y) {
    m_extraNotifyArg1 = item;
    m_fallActive = FALLING_ITEM_DESCENDING;
    i64* clock = &m_fallClock.m_last;
    clock[1] = g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
    clock[0] = static_cast<u32>(g_frameTime);
    CSBI_ImageSet* n = m_extraNotify1;
    i32 l = x - 0xc;
    i32 t = y - 0xc;
    i32 rr = x + 0xc;
    i32 b = y + 0xc;
    m_fallRect.left = l;
    m_fallRect.top = t;
    m_fallRect.right = rr;
    m_fallRect.bottom = b;
    if (n) {

        RECT rc;
        i32 x = m_barRect.left;
        rc.left = l + x;
        i32 y = m_barRect.top;
        rc.top = t + y;
        rc.bottom = y + b;
        rc.right = x + rr;
        n->m_rect = rc;
    }
    NotifyAllSlots();
    return 1;
}

RVA(0x001076a0, 0x1f3)
void CStatusBarMgr::UpdateChipGrinderStatusBar() {

    if (m_fallActive == FALLING_ITEM_INACTIVE) {
        return;
    }

    i32 stepped = 0;
    if (m_fallActive == FALLING_ITEM_DESCENDING || m_fallActive == FALLING_ITEM_GRINDING) {
        u32 delay = g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
        i32 speed = g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 4);

        if (m_fallRect.top >= 0x1c7) {
            m_fallActive = FALLING_ITEM_INACTIVE;
            m_extraNotifyArg1 = 0;
        } else if (m_fallRect.bottom >= 0x1bf) {
            if (m_fallActive != FALLING_ITEM_GRINDING) {
                if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                    SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
                    if (registry->m_silentMode == 0) {
                        SoundCue* found = NULL;
                        CMapStringToPtr* map = &registry->m_cues;
                        MapLookup(*map, "GAME_REZGRINDING", found);
                        if (found) {
                            i32 soundEnabled = g_soundEnabled;
                            i32 volumePercent = g_soundVolumePercent;
                            if (soundEnabled != 0) {
                                SoundCue* p = found;
                                if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                                    >= static_cast<u32>(p->m_replayDelayMs)) {
                                    p->m_lastPlayTimeMs = g_soundCueTimeMs;
                                    p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                                }
                            }
                        }
                    }
                }
                m_fallActive = FALLING_ITEM_GRINDING;
            }
            delay = g_buteMgr.GetDwordDef("StatusBar", "FallingItemShredderDelay", 0x64);
            speed = g_buteMgr.GetIntDef("StatusBar", "FallingItemShredderSpeed", 2);
        }

        i64* clock = &m_fallClock.m_last;
        i64 d = static_cast<i64>(g_frameTime) - clock[0];
        if (d >= clock[1]) {
            m_fallRect.top += speed;
            m_fallRect.bottom += speed;
            CSBI_ImageSet* w = m_extraNotify1;
            if (w) {
                RECT rc;
                i32 sy = m_barRect.top;
                rc.bottom = sy + m_fallRect.bottom;
                rc.top = sy + m_fallRect.top;
                i32 sx = m_barRect.left;
                rc.left = m_fallRect.left + sx;
                rc.right = m_fallRect.right + sx;
                w->m_rect = rc;
            }
            clock[1] = delay;
            clock[0] = g_frameTime;
        }
        // Retail's `mov ecx,1` at +0x1d5 is the JOIN of the timer test's taken and
        // fall-through edges, so the flag is set whenever the item is active - not
        // only when the step ran.  That is why it never needs a frame slot.
        stepped = 1;
    }

    if (m_extraNotify1 != NULL && stepped) {
        NotifyAllSlots();
    }
}

RVA(0x00107920, 0xb7)
i32 CStatusBarMgr::DropFallingItemAt(i32 screenX, i32 screenY, i32 itemFrame) {
    if (m_pendingHlRow == STATUS_HL_ROW_NONE) {
        return 0;
    }
    CStatusBarItem* r = HitTestRects(screenX, screenY);
    if (r == NULL) {
        return 0;
    }
    SbiCommandId cmd = r->m_cmd;
    if (cmd != SBICMD_CONVEYOR_TOP && cmd != SBICMD_CONVEYOR_BOTTOM) {
        return 0;
    }

    i32 cx = screenX;
    RECT rc = r->m_rect;
    i32 lo = rc.left + 0x1b;
    i32 xHi = rc.right;
    if (screenX < lo) {
        cx = lo;
    } else if (screenX > xHi - 0x1a) {
        cx = xHi - 0x1a;
    }
    i32 localX = cx - m_barRect.left;
    i32 localY = 0x1b3 - m_barRect.top;
    UpdateFallingItemStatusBar(itemFrame, localX, localY);
    EnterHlRow(1, itemFrame);
    return 1;
}

RVA(0x00107a10, 0x62)
i32 CStatusBarMgr::UpdateRezMachineWakeStatusBar() {
    if (m_rezActive == 0) {
        if (m_extraNotifyArg0 == 0) {
            return 0;
        }
        SetLeftRezMachineAnimation(
            9,
            MACHINE_WAKING,
            g_buteMgr.GetDwordDef("StatusBar", "LeftMachineWakingDelay", 100)
        );
        m_rezActive = 1;
    } else {
        m_rezTick++;
    }
    return 1;
}

RVA(0x00107aa0, 0x23)
void CStatusBarMgr::ToggleStat(i32 idx) {
    if (m_statFlags[idx] != STATUS_SAMPLE_NONE) {
        ClearStat(idx);
    } else {
        LoadStatzTabToggleSprite(idx, STATUS_SAMPLE_HEALTH);
    }
}

RVA(0x00107ae0, 0x1aa)
void CStatusBarMgr::LoadMultiplayerBattlezConfig(i32) {
    BuildGameTabPauseButton();
    if (m_position == STATUSBAR_HIDDEN) {
        RestoreStatusBar();
    }
    if (m_activeTab != TAB_GAME) {
        ClearTabGroup();
        m_activeTab = TAB_GAME;
    }
    SetTab(GAME_TAB_MENU, 1);
    memset(m_statFlags, 0, sizeof(m_statFlags));
    Reset();

    GameModeId mode = g_gameReg->m_gameMode;
    if (mode == GAMEMODE_MULTIPLAYER) {
        for (i32 i = 0; i < g_buteMgr.GetIntDef("Multiplayer", "StartingGruntz", 0); i++) {
            m_slots[i].m_value = kSlotCommitLevel;
            m_slots[i].m_state = SLOT_READY;
        }
    } else if (mode == GAMEMODE_BATTLEZ) {
        for (i32 i = 0; i < g_buteMgr.GetIntDef("Battlez", "StartingGruntz", 0); i++) {
            m_slots[i].m_value = kSlotCommitLevel;
            m_slots[i].m_state = SLOT_READY;
        }
    }

    for (i32 j = 0; j < m_rewardQueue.GetSize(); j++) {
        Coord* p = static_cast<Coord*>(m_rewardQueue.GetData()[j]);
        if (p) {
            CoordPoolNode* node = g_coordPool.NodeOf(p);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_rewardQueue.SetSize(0, -1);
    i64* clock = &m_reserved2b0.m_last;
    clock[0] = 0;
    clock[1] = 0;
    m_hlBusy = 0;
    if (m_retabNotify) {
        ::operator delete(m_retabNotify);
        m_retabNotify = NULL;
    }
    ExitMode();
    m_observerTabAvailable = 0;
    m_destructButtonLocked = 0;
    TryActivate();
}
// @early-stop
// The four `range == 0` WapRand expansions shift the LCG seed with `sar` where retail
// uses `shr`, against the same `holdrand` relocation as the four `% range` arms, which
// are `sar` on both sides. NO spelling of the arm can produce that: cl 5.0 takes the
// shift's signedness from the shifted operand's declared type only, and 16 measured
// spellings (result casts, an unsigned return type, an unsigned intermediate, `& 1u`,
// `% 2u`, the macro form, a mask discarding every sign bit) all keep `sar` - see
// docs/patterns/shift-signedness-is-the-operands-declared-type.md. So retail's coin
// site reads the seed through an UNSIGNED lvalue, which needs the seed to leave
// GetRandomNumber's function-local static that 12 game TUs share. The rest is register
// rotation around the SetRect call.
RVA(0x00107d00, 0x591)
i32 CStatusBarMgr::StartChipMachineCycle() {
    PickupType result;
    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        if (m_rewardQueue.GetSize() > 0) {
            Coord* p = static_cast<Coord*>(m_rewardQueue.GetData()[0]);
            result = static_cast<PickupType>(p->m_x);
            CoordPoolNode* node = g_coordPool.NodeOf(p);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
            m_rewardQueue.RemoveAt(0, 1);
        } else {
            result = PICKUP_NONE;
            if (m_extraNotify0) {
                m_extraNotify0->Notify(0);
            }
        }
    } else {
        i32 r1 = WapRand(m_battlezPct[2]);
        if (r1 <= m_battlezPct[0]) {
            i32 r = WapRand(m_battlezPct[37]);
            if (r <= m_battlezPct[17]) {
                result = PICKUP_BOMB;
            } else if (r <= m_battlezPct[18]) {
                result = PICKUP_BOOMERANG;
            } else if (r <= m_battlezPct[19]) {
                result = PICKUP_BRICK;
            } else if (r <= m_battlezPct[20]) {
                result = PICKUP_CLUB;
            } else if (r <= m_battlezPct[21]) {
                result = PICKUP_GAUNTLETZ;
            } else if (r <= m_battlezPct[22]) {
                result = PICKUP_GLOVEZ;
            } else if (r <= m_battlezPct[23]) {
                result = PICKUP_GOOBER;
            } else if (r <= m_battlezPct[24]) {
                result = PICKUP_GRAVITYBOOTZ;
            } else if (r <= m_battlezPct[25]) {
                result = PICKUP_GUNHAT;
            } else if (r <= m_battlezPct[26]) {
                result = PICKUP_NERFGUN;
            } else if (r <= m_battlezPct[27]) {
                result = PICKUP_ROCK;
            } else if (r <= m_battlezPct[28]) {
                result = PICKUP_SHIELD;
            } else if (r <= m_battlezPct[29]) {
                result = PICKUP_SHOVEL;
            } else if (r <= m_battlezPct[30]) {
                result = PICKUP_SPRING;
            } else if (r <= m_battlezPct[31]) {
                result = PICKUP_SPY;
            } else if (r <= m_battlezPct[32]) {
                result = PICKUP_SWORD;
            } else if (r <= m_battlezPct[33]) {
                result = PICKUP_TIMEBOMB;
            } else if (r <= m_battlezPct[34]) {
                result = PICKUP_TOOB;
            } else if (r <= m_battlezPct[35]) {
                result = PICKUP_WAND;
            } else {
                result = r > m_battlezPct[36] ? PICKUP_WINGZ : PICKUP_WELDER;
            }
        } else if (r1 <= m_battlezPct[1]) {
            i32 r = WapRand(m_battlezPct[16]);
            if (r <= m_battlezPct[7]) {
                result = PICKUP_BABYWALKER;
            } else if (r <= m_battlezPct[8]) {
                result = PICKUP_BEACHBALL;
            } else if (r <= m_battlezPct[9]) {
                result = PICKUP_BIGWHEEL;
            } else if (r <= m_battlezPct[10]) {
                result = PICKUP_GOKART;
            } else if (r <= m_battlezPct[11]) {
                result = PICKUP_JACKINTHEBOX;
            } else if (r <= m_battlezPct[12]) {
                result = PICKUP_JUMPROPE;
            } else if (r <= m_battlezPct[13]) {
                result = PICKUP_POGOSTICK;
            } else if (r <= m_battlezPct[14]) {
                result = PICKUP_SCROLL;
            } else {
                result = r > m_battlezPct[15] ? PICKUP_YOYO : PICKUP_SQUEAKTOY;
            }
        } else {
            i32 r = WapRand(m_battlezPct[6]);
            if (r <= m_battlezPct[3]) {
                result = PICKUP_REDBRICK;
            } else if (r <= m_battlezPct[4]) {
                result = PICKUP_BLUEBRICK;
            } else {
                result = r > m_battlezPct[5] ? PICKUP_BLACKBRICK : PICKUP_GOLDBRICK;
            }
        }
        if (result == PICKUP_WARPSTONE) {
            result = PICKUP_GAUNTLETZ;
        }
    }
    m_extraNotifyArg0 = IDX(result);
    m_machinePhase = BELT_IDLE;
    SetRect(&m_itemRect, 0x49, 0xd7, 0x61, 0xef);
    if (m_extraNotify0) {
        RECT rc;
        i32 x = m_barRect.left;
        i32 y = m_barRect.top;
        rc.left = m_itemRect.left + x;
        rc.top = m_itemRect.top + y;
        rc.right = m_itemRect.right + x;
        rc.bottom = m_itemRect.bottom + y;
        m_extraNotify0->m_rect = rc;
    }
    NotifyAllSlots();
    i32 c = m_rezTick;
    m_rezActive = 0;
    if (c > 0) {
        m_rezTick = c - 1;
        UpdateRezMachineWakeStatusBar();
    }
    return 1;
}

// @early-stop
RVA(0x00108410, 0x8e)
i32 CStatusBarMgr::QueuePickupReward(i32 pickupValue, i32 score) {
    CoordPoolNode* head = g_coordPool.m_freeHead;
    Coord* node = NULL;
    if (head->m_next != NULL) {
        node = &head->m_coord;
        node->m_x = pickupValue;
        node->m_y = score;
        g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
    }
    i32 n = m_rewardQueue.GetSize();
    i32 i = 0;
    if (i < n) {
        Coord** t = MfcPtrArrayData<Coord>(m_rewardQueue);
        while (i < n) {
            Coord* e = *t;
            if (e != NULL && score < e->m_y) {
                goto insert;
            }
            i++;
            t++;
        }
    }
    m_rewardQueue.Add(node);
    return 1;
insert:
    m_rewardQueue.InsertAt(i, node, 1);
    return 1;
}

// Retail serializes every {i64 last; i64 interval} pair through one inlined
// helper: the base address is hoisted into a register before the mode test and
// the second field reached with `add reg,8`.
static inline void SyncClockPair(CFileMemBase* s, SerialMode mode, i64* pair) {
    if (mode != SERIAL_SAVE) {
        if (mode == SERIAL_LOAD) {
            s->Read(pair, sizeof(*pair));
            s->Read(pair + 1, sizeof(*pair));
        }
    } else {
        s->Write(pair, sizeof(*pair));
        s->Write(pair + 1, sizeof(*pair));
    }
}

// @early-stop
// Nothing is missing: `--branches --diff` reports the same branch and ret counts, and
// the only reloc delta is the __except_list/fs:0 typing artifact. Retail spills `this`
// to [esp+0x10] and re-reads typeId/payload from their parameter homes at every
// SerializeFields site, which leaves ebp for the loop counters; cl instead enregisters
// payload in ebp and
// spills the counters. Reusing one counter variable across the loops is byte-neutral
// (measured), so the allocation choice is not decl-scope-driven.
RVA(0x001084d0, 0x96c)
i32 CStatusBarMgr::SerializeDispatch(
    CFileMemBase* s,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (s == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            if (Serialize(s) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (Deserialize(s) == 0) {
                return 0;
            }
            break;
        case SERIAL_POSTLOAD:
            (static_cast<CPlay*>(g_gameReg->m_curState))->ResetViewport();
            if (m_position == STATUSBAR_DOCK_RIGHT) {
                DockStatusBarLeft();
                DockStatusBarRight();
            }
            break;
    }

    if (m_retabNotify != NULL) {
        i32 tmp = 1;
        if (mode == SERIAL_SAVE) {
            s->Write(&tmp, sizeof(tmp));
        }
    } else {
        i32 tmp = 0;
        if (mode == SERIAL_SAVE) {
            s->Write(&tmp, sizeof(tmp));
        } else if (mode == SERIAL_LOAD) {
            s->Read(&tmp, sizeof(tmp));
            if (tmp != 0) {
                CWarpStoneFly* c = new CWarpStoneFly();
                m_retabNotify = c;
                c->m_owner = this;
            }
        }
    }

    if (m_retabNotify != NULL) {
        if (m_retabNotify->SerializeDispatch(s, mode, typeId, payload) == 0) {
            return 0;
        }
    }

    SyncClockPair(s, mode, &m_beltClock.m_last);
    SyncClockPair(s, mode, &m_fallClock.m_last);
    SyncClockPair(s, mode, &m_machineB.m_last);
    SyncClockPair(s, mode, &m_machineA.m_last);
    SyncClockPair(s, mode, &m_destructWarningClock.m_last);

    CSbiSlot* p = m_slots;
    i32 n = 5;
    do {
        SyncClockPair(s, mode, &p->m_startTime);
        p++;
        n--;
    } while (n != 0);

    n = 3;
    CSbiHlRow* r = m_conveyorSlots;
    do {
        SyncClockPair(s, mode, &r->m_last);
        r++;
        n--;
    } while (n != 0);

    i32 outer = 3;
    CSbiHlRow* g = m_resourceSlots;
    do {
        n = 4;
        do {
            SyncClockPair(s, mode, &g->m_last);
            g++;
            n--;
        } while (n != 0);
        outer--;
    } while (outer != 0);

    SyncClockPair(s, mode, &m_reserved2a0.m_last);
    SyncClockPair(s, mode, &m_reserved2b0.m_last);
    if (mode == SERIAL_LOAD && m_position != STATUSBAR_HIDDEN) {
        BuildStatusBarTabs();
    }

#define SER(field)                                                                                 \
    if (field) {                                                                                   \
        if ((field)->SerializeFields(s, mode, typeId, payload) == 0)                               \
            return 0;                                                                              \
    }

    {
        i32 i = 0;
        do {
            SER(m_hitRects[i])
            SER(m_statObj[i])
            i++;
        } while (i < 0xf);
    }
    {
        i32 i = 0;
        CSBI_ImageSet** q = m_slotNotify;
        do {
            SER(*q)
            i++;
            q++;
        } while (i < 5);
    }
    {
        i32 i = 0;
        CSBI_ImageSet** q = m_conveyorSprites;
        do {
            SER(*q)
            i++;
            q++;
        } while (i < 3);
    }
    {
        i32 row = 0;
        CSBI_ImageSet** base = m_resourceSlotSprites;
        do {
            i32 i = 0;
            CSBI_ImageSet** q = base;
            do {
                SER(*q)
                i++;
                q++;
            } while (i < 4);
            row++;
            base += 4;
        } while (row < 3);
    }
    {
        i32 i = 0;
        CSBI_WarlordHead** q = m_warlordHead;
        do {
            SER(*q)
            i++;
            q++;
        } while (i < 4);
    }

    SER(m_tabSprite0)
    SER(m_tabSprite1)
    SER(m_tabSprite2)
    SER(m_tabSprite3)
    SER(m_tabSprite4)
    SER(m_tabSprite5)
    SER(m_tabSprite6)
    SER(m_tabSprite7)
    SER(m_tabSprite8)
    SER(m_tabSprite9)
    SER(m_tabSprite10)
    SER(m_tabSprite10)
    SER(m_tabSprite11)
    SER(m_tabSprite12)
    SER(m_tabSprite13)
    SER(m_tabSprite14)
    SER(m_gaugeNotify)
    SER(m_gaugeSink)
    SER(m_machineDisplay)
    SER(m_notify0)
    SER(m_notify1)
    SER(m_notify2)
    SER(m_notify3)
    SER(m_extraNotify0)
    SER(m_extraNotify1)
    SER(m_destructButtonImage)
#undef SER

    Deactivate();
    return 1;
}

RVA(0x001090a0, 0x38f)
i32 CStatusBarMgr::Serialize(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    if (g_gameReg->m_world == NULL) {
        return 0;
    }

    s->Write(this, 4);
    s->Write(&m_restorePosition, sizeof(m_restorePosition));

    g_serialCounter++;

    {
        i32 tmp = 0;
        if (m_barSprite) {
            tmp = m_barSprite->m_objectId;
        }
        s->Write(&tmp, sizeof(tmp));
    }

    s->Write(&m_barRect.left, sizeof(m_barRect));
    s->Write(&m_redrawFrames, sizeof(m_redrawFrames));
    s->Write(&m_barX, sizeof(m_barX));
    s->Write(&m_barY, sizeof(m_barY));
    s->Write(&m_itemKind, sizeof(m_itemKind));
    s->Write(&m_tabCycle, sizeof(m_tabCycle));

    StatusSampleMode* p = m_statFlags;
    for (i32 i = 0; i < 15; i++) {
        s->Write(p, sizeof(*p));
        p += 1;
    }

    s->Write(&m_reserved34c, sizeof(m_reserved34c));
    s->Write(&m_reserved350, sizeof(m_reserved350));
    s->Write(&m_chatBoxDisabled, sizeof(m_chatBoxDisabled));
    s->Write(&m_activeSlot, sizeof(m_activeSlot));
    s->Write(&m_pendingHlRow, sizeof(m_pendingHlRow));
    s->Write(&m_activeTab, sizeof(m_activeTab));
    s->Write(&m_gauge, sizeof(m_gauge));
    s->Write(&m_gaugeTarget, sizeof(m_gaugeTarget));
    s->Write(&m_itemBaseX, sizeof(m_itemBaseX));
    s->Write(&m_rezTick, sizeof(m_rezTick));
    s->Write(&m_rezActive, sizeof(m_rezActive));
    s->Write(&m_reserved544, sizeof(m_reserved544));
    s->Write(&m_fallRect, sizeof(m_fallRect));
    s->Write(&m_itemRect, sizeof(m_itemRect));
    s->Write(&m_hlBusy, sizeof(m_hlBusy));
    s->Write(&m_levelOverlayActive, sizeof(m_levelOverlayActive));
    s->Write(&m_quitConfirmationActive, sizeof(m_quitConfirmationActive));
    s->Write(&m_machinePhase, sizeof(m_machinePhase));
    s->Write(&m_extraNotifyArg0, sizeof(m_extraNotifyArg0));
    s->Write(&m_fallActive, sizeof(m_fallActive));
    s->Write(&m_extraNotifyArg1, sizeof(m_extraNotifyArg1));
    s->Write(&m_machineB, 4);
    s->Write(&m_machineB.m_value, sizeof(m_machineB.m_value));
    s->Write(&m_machineA, 4);
    s->Write(&m_machineA.m_value, sizeof(m_machineA.m_value));
    s->Write(&m_destructWarningState, sizeof(m_destructWarningState));
    s->Write(&m_destructButtonFrame, sizeof(m_destructButtonFrame));
    s->Write(&m_destructButtonLocked, sizeof(m_destructButtonLocked));
    s->Write(&m_observerTabAvailable, sizeof(m_observerTabAvailable));

    for (i32 j = 0; j < 5; j++) {
        s->Write(&m_slots[j].m_state, sizeof(m_slots[j].m_state));
        s->Write(&m_slots[j].m_value, sizeof(m_slots[j].m_value));
    }
    for (i32 k = 0; k < 3; k++) {
        s->Write(&m_conveyorSlots[k].m_state, sizeof(m_conveyorSlots[k].m_state));
        s->Write(&m_conveyorSlots[k].m_value, sizeof(m_conveyorSlots[k].m_value));
    }
    {
        CSbiHlRow* nb = m_resourceSlots;
        i32 cnt = 3;
        do {
            for (i32 m = 0; m < 4; m++) {
                s->Write(&nb[m].m_state, sizeof(nb[m].m_state));
                s->Write(&nb[m].m_value, sizeof(nb[m].m_value));
            }
            nb += 4;
        } while (--cnt);
    }

    i32 ptrCount = m_rewardQueue.GetSize();
    s->Write(&ptrCount, sizeof(ptrCount));
    for (u32 n = 0; n < static_cast<u32>(ptrCount); n++) {
        s->Write(m_rewardQueue.GetData()[n], 8);
    }
    return 1;
}

// @early-stop
RVA(0x00109520, 0x44c)
i32 CStatusBarMgr::Deserialize(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* gm = g_gameReg->m_world;
    if (gm == NULL) {
        return 0;
    }
    m_destructWarningSound = NULL;
    ResetWidgets(0);

    s->Read(this, 4);
    s->Read(&m_restorePosition, sizeof(m_restorePosition));

    g_serialCounter++;

    i32 seq;
    s->Read(&seq, sizeof(seq));

    CGameObject* obj = NULL;
    CWwdSpriteObject* m8;
    if (MapLookupById(gm->m_childGroup->m_registeredGameObjectsById, seq, obj) == 0) {
        m8 = NULL;
    } else if (obj == NULL) {
        m8 = NULL;
    } else {
        m8 = (obj->GetClassId() == CLASSID_SERIALREF) ? static_cast<CWwdSpriteObject*>(obj) : NULL;
    }
    m_barSprite = m8;
    if (m8 == NULL && seq != 0) {
        return 0;
    }

    s->Read(&m_barRect.left, sizeof(m_barRect));
    s->Read(&m_redrawFrames, sizeof(m_redrawFrames));
    s->Read(&m_barX, sizeof(m_barX));
    s->Read(&m_barY, sizeof(m_barY));
    s->Read(&m_itemKind, sizeof(m_itemKind));
    s->Read(&m_tabCycle, sizeof(m_tabCycle));

    StatusSampleMode* p = m_statFlags;
    for (i32 i = 0; i < 15; i++) {
        s->Read(p, sizeof(*p));
        p += 1;
    }

    s->Read(&m_reserved34c, sizeof(m_reserved34c));
    s->Read(&m_reserved350, sizeof(m_reserved350));
    s->Read(&m_chatBoxDisabled, sizeof(m_chatBoxDisabled));
    s->Read(&m_activeSlot, sizeof(m_activeSlot));
    s->Read(&m_pendingHlRow, sizeof(m_pendingHlRow));
    s->Read(&m_activeTab, sizeof(m_activeTab));
    s->Read(&m_gauge, sizeof(m_gauge));
    s->Read(&m_gaugeTarget, sizeof(m_gaugeTarget));
    s->Read(&m_itemBaseX, sizeof(m_itemBaseX));
    s->Read(&m_rezTick, sizeof(m_rezTick));
    s->Read(&m_rezActive, sizeof(m_rezActive));
    s->Read(&m_reserved544, sizeof(m_reserved544));
    s->Read(&m_fallRect, sizeof(m_fallRect));
    s->Read(&m_itemRect, sizeof(m_itemRect));
    s->Read(&m_hlBusy, sizeof(m_hlBusy));
    s->Read(&m_levelOverlayActive, sizeof(m_levelOverlayActive));
    s->Read(&m_quitConfirmationActive, sizeof(m_quitConfirmationActive));
    s->Read(&m_machinePhase, sizeof(m_machinePhase));
    s->Read(&m_extraNotifyArg0, sizeof(m_extraNotifyArg0));
    s->Read(&m_fallActive, sizeof(m_fallActive));
    s->Read(&m_extraNotifyArg1, sizeof(m_extraNotifyArg1));
    s->Read(&m_machineB, 4);
    s->Read(&m_machineB.m_value, sizeof(m_machineB.m_value));
    s->Read(&m_machineA, 4);
    s->Read(&m_machineA.m_value, sizeof(m_machineA.m_value));
    s->Read(&m_destructWarningState, sizeof(m_destructWarningState));
    s->Read(&m_destructButtonFrame, sizeof(m_destructButtonFrame));
    s->Read(&m_destructButtonLocked, sizeof(m_destructButtonLocked));
    s->Read(&m_observerTabAvailable, sizeof(m_observerTabAvailable));

    for (i32 j = 0; j < 5; j++) {
        s->Read(&m_slots[j].m_state, sizeof(m_slots[j].m_state));
        s->Read(&m_slots[j].m_value, sizeof(m_slots[j].m_value));
    }
    for (i32 k = 0; k < 3; k++) {
        s->Read(&m_conveyorSlots[k].m_state, sizeof(m_conveyorSlots[k].m_state));
        s->Read(&m_conveyorSlots[k].m_value, sizeof(m_conveyorSlots[k].m_value));
    }
    CSbiHlRow* nb = m_resourceSlots;
    seq = 3;
    do {
        for (i32 m = 0; m < 4; m++) {
            s->Read(&nb[m].m_state, sizeof(nb[m].m_state));
            s->Read(&nb[m].m_value, sizeof(nb[m].m_value));
        }
        nb += 4;
    } while (--seq);

    for (i32 t = 0; t < m_rewardQueue.GetSize(); t++) {
        Coord* pp = static_cast<Coord*>(m_rewardQueue.GetData()[t]);
        if (pp) {
            CoordPoolNode* node = g_coordPool.NodeOf(pp);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_rewardQueue.SetSize(0, -1);

    i32 cnt;
    s->Read(&cnt, sizeof(cnt));
    m_rewardQueue.SetSize(cnt, -1);
    for (u32 n = 0; n < static_cast<u32>(cnt); n++) {
        CoordPoolNode* head = g_coordPool.m_freeHead;
        Coord* node = NULL;
        if (head->m_next != NULL) {
            node = &head->m_coord;
            g_coordPool.m_freeHead = head->m_next;
        }
        s->Read(node, 8);
        m_rewardQueue.GetData()[n] = node;
    }
    return 1;
}

RVA(0x00109a90, 0x25)
i32 CStatusBarMgr::FindReadySlot() {
    for (i32 i = 0; i < 5; i++) {
        if (m_slots[i].m_state == SLOT_READY) {
            ArmSlot(i);
            return 1;
        }
    }
    return 0;
}

RVA(0x00109ad0, 0xa9)
i32 CStatusBarMgr::StartWarpStoneFly(i32 srcX, i32 srcY, WarpStoneFragment fragment) {
    if (m_retabNotify) {
        return 0;
    }
    CWarpStoneFly* o = new CWarpStoneFly();
    m_retabNotify = o;
    if (o == NULL) {
        return 0;
    }
    return o->Init(this, srcX, srcY, fragment);
}

RVA(0x00109bb0, 0xb)
CWarpStoneFly::CWarpStoneFly() {
    m_sprite = NULL;
    m_owner = NULL;
}

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

// @early-stop
// Frame and branch skeleton are retail's; the residue is the register pair the
// fragment table's two constants are materialised into (retail ecx/edx, cl edx/edi)
// across all four arms.
RVA(0x00109bd0, 0x1b5)
i32 CWarpStoneFly::Init(CStatusBarMgr* owner, i32 srcX, i32 srcY, WarpStoneFragment fragment) {
    m_owner = owner;

    i32 n = IDX(fragment) + 1;
    CDDrawWorker* spr = LookupWorker(
        g_gameReg->m_world->m_imageRegistry->m_workersByName,
        "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE"
    );
    CImage* frame = spr ? spr->GetAt(n) : NULL;
    m_sprite = frame;
    if (frame == NULL) {

        return 0;
    }

    m_arrivalMode = fragment;
    i32 cx, dy;
    switch (fragment) {
        case WARPSTONE_FRAGMENT_SECOND:
            cx = 0x69;
            dy = 0x26;
            break;
        case WARPSTONE_FRAGMENT_THIRD:
            cx = 0x65;
            dy = 0x50;
            break;
        case WARPSTONE_FRAGMENT_FOURTH:
            cx = 0x69;
            dy = 0x54;
            break;
        default:
            cx = 0x34;
            dy = 0x29;
            break;
    }

    CStatusBarMgr* base = m_owner;
    i32 tx = base->m_barRect.left + cx;
    m_targetX = tx;
    i32 ty = base->m_barRect.top + dy;
    m_targetY = ty;

    i32 deltaX = tx - srcX;
    i32 dyv = ty - srcY;
    i32 dist2 = deltaX * deltaX + dyv * dyv;
    double dist = sqrt(static_cast<double>(dist2));
    u32 flyTime = g_buteMgr.GetDwordDef("WarpStone", "FlyTime", 0x5dc);

    m_velocityScale = dist / static_cast<double>(flyTime);
    m_xDirection = static_cast<double>(deltaX) / dist;
    m_yDirection = static_cast<double>(dyv) / dist;

    SoundCueRegistry* h = g_gameReg->m_world->m_soundRegistry;
    if (h->m_silentMode == 0) {
        SoundCue* found = NULL;
        MapLookup(h->m_cues, "GAME_WARPSTONEFLY", found);
        if (found) {
            SoundCue* fly = found;
            i32 soundEnabled = g_soundEnabled;
            i32 volumePercent = g_soundVolumePercent;
            if (soundEnabled != 0
                && g_soundCueTimeMs - fly->m_lastPlayTimeMs >= fly->m_replayDelayMs) {
                fly->m_lastPlayTimeMs = g_soundCueTimeMs;
                fly->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
            }
        }
    }

    m_currentX = static_cast<double>(srcX);
    m_currentY = static_cast<double>(srcY);
    return 1;
}

RVA(0x00109e00, 0x245)
i32 CWarpStoneFly::SerializeDispatch(
    CFileMemBase* arc,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (arc == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* lvl = g_gameReg->m_world;
    if (lvl == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_LOAD: {

            arc->Read(&m_arrivalMode, sizeof(m_arrivalMode));
            arc->Read(&m_targetX, sizeof(m_targetX));
            arc->Read(&m_targetY, sizeof(m_targetY));
            arc->Read(&m_currentX, sizeof(m_currentX));
            arc->Read(&m_currentY, sizeof(m_currentY));
            arc->Read(&m_velocityScale, sizeof(m_velocityScale));
            arc->Read(&m_xDirection, sizeof(m_xDirection));
            arc->Read(&m_yDirection, sizeof(m_yDirection));
            g_serialCounter++;

            char name[SERIAL_NAME_LEN];
            i32 index;
            arc->Read(name, SERIAL_NAME_LEN);
            arc->Read(&index, sizeof(index));
            if (strlen(name) != 0) {
                i32 i = index;
                CObject* out = NULL;
                lvl->m_imageRegistry->m_workersByName.Lookup(name, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r = rec != NULL ? rec->GetAt(i) : NULL;
                m_sprite = r;
            } else {
                m_sprite = NULL;
            }
            return 1;
        }
        case SERIAL_SAVE: {

            arc->Write(&m_arrivalMode, sizeof(m_arrivalMode));
            arc->Write(&m_targetX, sizeof(m_targetX));
            arc->Write(&m_targetY, sizeof(m_targetY));
            arc->Write(&m_currentX, sizeof(m_currentX));
            arc->Write(&m_currentY, sizeof(m_currentY));
            arc->Write(&m_velocityScale, sizeof(m_velocityScale));
            arc->Write(&m_xDirection, sizeof(m_xDirection));
            arc->Write(&m_yDirection, sizeof(m_yDirection));
            g_serialCounter++;

            CImage* obj = m_sprite;
            char name[SERIAL_NAME_LEN];
            i32 index = 0;
            memset(name, 0, SERIAL_NAME_LEN);
            if (obj != NULL) {
                lvl->m_imageRegistry->AnyValueMatches(obj, name, &index);
            }
            arc->Write(name, SERIAL_NAME_LEN);
            arc->Write(&index, sizeof(index));
            break;
        }
    }
    return 1;
}

// @early-stop
RVA(0x0010a0f0, 0x184)
i32 CWarpStoneFly::Tick(u32 dt) {
    i32 cellY = static_cast<i32>(m_currentY);
    i32 cellX = static_cast<i32>(m_currentX);
    if (cellX == m_targetX && cellY == m_targetY) {
        i32 mode = m_arrivalMode;
        CByteArray* arr = &g_gameReg->m_triggerMgr->m_byteArr;
        arr->SetAtGrow(arr->GetSize(), static_cast<BYTE>(mode));
        m_owner->m_hlBusy = 0;
        if (m_owner->m_position != STATUSBAR_HIDDEN && m_owner->m_activeTab == TAB_GAME) {
            m_owner->ResetWidgets(0);
            m_owner->TryActivate();
        }
        CStatusBarMgr* owner = m_owner;
        if (owner->m_retabNotify != NULL) {
            ::operator delete(owner->m_retabNotify);
            owner->m_retabNotify = NULL;
        }
        return 1;
    }

    double t = static_cast<double>(dt);
    double newX = m_currentX + (t * m_velocityScale) * m_xDirection;
    double newY = m_currentY + (t * m_yDirection) * m_velocityScale;
    m_currentX = newX;
    m_currentY = newY;

    if (m_xDirection > 0.0) {
        if (static_cast<i32>(newX) > m_targetX) {
            m_currentX = static_cast<double>(m_targetX);
        }
    } else if (m_xDirection < 0.0) {
        if (static_cast<i32>(newX) < m_targetX) {
            m_currentX = static_cast<double>(m_targetX);
        }
    }

    if (m_yDirection > 0.0) {
        if (static_cast<i32>(newY) > m_targetY) {
            m_currentY = static_cast<double>(m_targetY);
        }
    } else if (m_yDirection < 0.0) {
        if (static_cast<i32>(newY) < m_targetY) {
            m_currentY = static_cast<double>(m_targetY);
        }
    }
    return 1;
}

RVA(0x0010a2f0, 0x35)
i32 CWarpStoneFly::Draw() {
    m_sprite->RenderFrame(
        g_gameReg->m_world->m_drawTarget->m_backPair,
        static_cast<i32>(m_currentX),
        static_cast<i32>(m_currentY),
        0
    );
    return 1;
}

// @early-stop
RVA(0x0010a340, 0xbcb)
i32 CStatusBarMgr::BuildTabzDialog() {
    if (m_levelOverlayActive == 0) {
        return 1;
    }

    CDDrawSurfaceMgr* w = m_world;
    i32 cx;
    i32 cy;
    {
        RECT src = w->m_level->m_viewportRect;
        RECT dst;
        CopyRect(&dst, &src);
        cx = dst.left + (dst.right - dst.left) / 2;
        cy = dst.top + (dst.bottom - dst.top) / 2;
    }

    if (m_quitConfirmationActive != 0) {
        // every child rect below is relative to this dialog's top-left corner
        cx -= 0x5e;
        cy -= 0x3c;

        CSBI_Image* areYouSure = new CSBI_Image;
        if (!areYouSure->SetupImage(
                this,
                w,
                SBICMD_DIALOG_FRAME,
                TAB_DIALOG,
                SbGeom(cx, cy, cx + 0xbc, cy + 0x79),
                "GAME_STATUSBAR_TABZ_DIALOG_AREYOUSURE",
                -1,
                0
            )) {
            delete areYouSure;
            return 0;
        }
        AddTabItem(6, areYouSure);

        CSBI_MenuItem* yes = new CSBI_MenuItem;
        if (!yes->SetupImage(
                this,
                w,
                SBICMD_DIALOG_YES,
                TAB_DIALOG,
                SbGeom(cx + 0x19, cy + 0x4d, cx + 0x4c, cy + 0x64),
                "GAME_STATUSBAR_TABZ_DIALOG_YES",
                -1,
                0
            )) {
            delete yes;
            return 0;
        }
        AddTabItem(6, yes);
        m_tabSprite13 = yes;

        CSBI_MenuItem* no = new CSBI_MenuItem;
        if (!no->SetupImage(
                this,
                w,
                SBICMD_DIALOG_NO,
                TAB_DIALOG,
                SbGeom(cx + 0x6b, cy + 0x4d, cx + 0x9e, cy + 0x64),
                "GAME_STATUSBAR_TABZ_DIALOG_NO",
                -1,
                0
            )) {
            delete no;
            return 0;
        }
        AddTabItem(6, no);
        m_tabSprite14 = no;
        return 1;
    }

    // every child rect below is relative to this dialog's top-left corner
    cx -= 0x8e;
    cy -= 0x48;

    i32 reason = IDX(g_gameReg->m_triggerMgr->m_finishReasonFrame);

    CSBI_Image* dialog = new CSBI_Image;
    if (!dialog->SetupImage(
            this,
            w,
            SBICMD_DIALOG_FRAME,
            TAB_DIALOG,
            SbGeom(cx, cy, cx + 0x11c, cy + 0x90),
            "GAME_STATUSBAR_TABZ_DIALOG",
            -1,
            0
        )) {
        delete dialog;
        return 0;
    }
    AddTabItem(6, dialog);

    if (g_gameReg->m_triggerMgr->m_phase == FINISH_STATE_VICTORY) {

        CSBI_ImageSet* status = new CSBI_ImageSet;
        if (!status->SetupImage(
                this,
                w,
                SBICMD_DIALOG_MISSION_STATUS,
                TAB_DIALOG,
                SbGeom(cx, cy + 0x17, cx + 0x11b, cy + 0x32),
                "GAME_STATUSBAR_TABZ_DIALOG_MISSIONSTATUS",
                1,
                0
            )) {
            delete status;
            return 0;
        }
        AddTabItem(6, status);

        CSBI_ImageSet* rsn = new CSBI_ImageSet;
        if (!rsn->SetupImage(
                this,
                w,
                SBICMD_DIALOG_REASON,
                TAB_DIALOG,
                SbGeom(cx + 0x12, cy + 0x37, cx + 0x101, cy + 0x4c),
                "GAME_STATUSBAR_TABZ_DIALOG_REASON",
                reason,
                0
            )) {
            delete rsn;
            return 0;
        }
        AddTabItem(6, rsn);

        if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
            CSBI_MenuItem* next = new CSBI_MenuItem;
            if (!next->SetupImage(
                    this,
                    w,
                    SBICMD_DIALOG_PRIMARY,
                    TAB_DIALOG,
                    SbGeom(cx + 0x11, cy + 0x5f, cx + 0x80, cy + 0x7a),
                    "GAME_STATUSBAR_TABZ_DIALOG_PLAYNEXTLEVEL",
                    -1,
                    0
                )) {
                delete next;
                return 0;
            }
            AddTabItem(6, next);
            m_tabSprite11 = next;

            CSBI_MenuItem* quit = new CSBI_MenuItem;
            if (!quit->SetupImage(
                    this,
                    w,
                    SBICMD_DIALOG_SECONDARY,
                    TAB_DIALOG,
                    SbGeom(cx + 0x8e, cy + 0x5f, cx + 0xfd, cy + 0x7a),
                    "GAME_STATUSBAR_TABZ_DIALOG_QUITTOMAINMENU",
                    -1,
                    0
                )) {
                delete quit;
                return 0;
            }
            AddTabItem(6, quit);
            m_tabSprite12 = quit;
        } else {
            CSBI_MenuItem* statz = new CSBI_MenuItem;
            if (!statz->SetupImage(
                    this,
                    w,
                    SBICMD_DIALOG_SECONDARY,
                    TAB_DIALOG,
                    SbGeom(cx + 0x55, cy + 0x5f, cx + 0xc4, cy + 0x7a),
                    "GAME_STATUSBAR_TABZ_DIALOG_STATZ",
                    -1,
                    0
                )) {
                delete statz;
                return 0;
            }
            AddTabItem(6, statz);
            m_tabSprite12 = statz;
        }
        return 1;
    }

    CSBI_ImageSet* status = new CSBI_ImageSet;
    if (!status->SetupImage(
            this,
            w,
            SBICMD_DIALOG_MISSION_STATUS,
            TAB_DIALOG,
            SbGeom(cx, cy + 0x17, cx + 0x11b, cy + 0x32),
            "GAME_STATUSBAR_TABZ_DIALOG_MISSIONSTATUS",
            2,
            0
        )) {
        delete status;
        return 0;
    }
    AddTabItem(6, status);

    CSBI_ImageSet* rsn = new CSBI_ImageSet;
    if (!rsn->SetupImage(
            this,
            w,
            SBICMD_DIALOG_REASON,
            TAB_DIALOG,
            SbGeom(cx + 0x12, cy + 0x37, cx + 0x101, cy + 0x4c),
            "GAME_STATUSBAR_TABZ_DIALOG_REASON",
            reason,
            0
        )) {
        delete rsn;
        return 0;
    }
    AddTabItem(6, rsn);

    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        CSBI_MenuItem* replay = new CSBI_MenuItem;
        if (!replay->SetupImage(
                this,
                w,
                SBICMD_DIALOG_PRIMARY,
                TAB_DIALOG,
                SbGeom(cx + 0x11, cy + 0x5f, cx + 0x80, cy + 0x7a),
                "GAME_STATUSBAR_TABZ_DIALOG_REPLAYLEVEL",
                -1,
                0
            )) {
            delete replay;
            return 0;
        }
        AddTabItem(6, replay);
        m_tabSprite11 = replay;

        CSBI_MenuItem* quit = new CSBI_MenuItem;
        if (!quit->SetupImage(
                this,
                w,
                SBICMD_DIALOG_SECONDARY,
                TAB_DIALOG,
                SbGeom(cx + 0x8e, cy + 0x5f, cx + 0xfd, cy + 0x7a),
                "GAME_STATUSBAR_TABZ_DIALOG_QUITTOMAINMENU",
                -1,
                0
            )) {
            delete quit;
            return 0;
        }
        AddTabItem(6, quit);
        m_tabSprite12 = quit;
        return 1;
    }

    i32 count = 0;
    for (i32 i = 0; i < 4; i++) {
        if (g_gameReg->m_players[i].m_joined != 0 && g_gameReg->m_players[i].m_doneFlag == 0
            && g_gameReg->m_players[i].m_clearedRound == 0) {
            count++;
        }
    }

    if (count >= 2) {
        CSBI_MenuItem* observe = new CSBI_MenuItem;
        if (!observe->SetupImage(
                this,
                w,
                SBICMD_DIALOG_PRIMARY,
                TAB_DIALOG,
                SbGeom(cx + 0x11, cy + 0x5f, cx + 0x80, cy + 0x7a),
                "GAME_STATUSBAR_TABZ_DIALOG_OBSERVE",
                -1,
                0
            )) {
            delete observe;
            return 0;
        }
        AddTabItem(6, observe);
        m_tabSprite11 = observe;
        m_observerTabAvailable = 1;

        CSBI_MenuItem* statz = new CSBI_MenuItem;
        if (!statz->SetupImage(
                this,
                w,
                SBICMD_DIALOG_SECONDARY,
                TAB_DIALOG,
                SbGeom(cx + 0x8e, cy + 0x5f, cx + 0xfd, cy + 0x7a),
                "GAME_STATUSBAR_TABZ_DIALOG_STATZ",
                -1,
                0
            )) {
            delete statz;
            return 0;
        }
        AddTabItem(6, statz);
        m_tabSprite12 = statz;
    } else {
        m_observerTabAvailable = 0;
        CSBI_MenuItem* statz = new CSBI_MenuItem;
        if (!statz->SetupImage(
                this,
                w,
                SBICMD_DIALOG_SECONDARY,
                TAB_DIALOG,
                SbGeom(cx + 0x55, cy + 0x5f, cx + 0xc4, cy + 0x7a),
                "GAME_STATUSBAR_TABZ_DIALOG_STATZ",
                -1,
                0
            )) {
            delete statz;
            return 0;
        }
        AddTabItem(6, statz);
        m_tabSprite12 = statz;
    }
    return 1;
}

RVA(0x0010b210, 0xc5)
void CStatusBarMgr::ExitMode() {
    if (m_levelOverlayActive == 0) {
        return;
    }
    POSITION n = m_tabLists[6].GetHeadPosition();
    while (n) {
        CStatusBarItem* cur = static_cast<CStatusBarItem*>(m_tabLists[6].GetNext(n));
        delete cur;
    }
    m_tabLists[6].RemoveAll();
    i32 wasQuitConfirmation = m_quitConfirmationActive;
    m_tabSprite11 = NULL;
    m_tabSprite12 = NULL;
    m_tabSprite13 = NULL;
    m_tabSprite14 = NULL;
    m_hlBusy = 0;
    if (wasQuitConfirmation == 0 && g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
        if (m_position == STATUSBAR_HIDDEN) {
            RestoreStatusBar();
        }
        if (m_activeTab != TAB_GAME) {
            SetTabState(SBICMD_TAB_GAME, MENUITEM_SELECTED);
        }
        SetTab(GAME_TAB_MENU, 1);
        Deactivate();
    } else {
        m_chatBoxDisabled = 0;
    }
    m_levelOverlayActive = 0;
    m_quitConfirmationActive = 0;
    Deactivate();
}

RVA(0x0010b320, 0x167)
void CStatusBarMgr::UpdateDestructWarningAnimation() {

    switch (m_destructWarningState) {
        case DESTRUCT_WARNING_FORWARD: {
            SbiClockPair* clock = &m_destructWarningClock;
            i64 d = static_cast<i64>(g_frameTime) - clock->m_last;
            if (d >= clock->m_interval) {
                m_destructButtonFrame = static_cast<DestructButtonFrame>(m_destructButtonFrame + 1);
                if (m_destructButtonFrame >= DESTRUCT_FRAME_WARNING_LAST) {
                    m_destructButtonFrame = DESTRUCT_FRAME_WARNING_LAST;
                    m_destructWarningState = DESTRUCT_WARNING_REVERSE;
                }
                clock->m_interval = static_cast<u32>(
                    g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32)
                );
                clock->m_last = static_cast<u32>(g_frameTime);
                CSBI_ImageSet* destructButtonImage = m_destructButtonImage;
                if (destructButtonImage) {
                    destructButtonImage->Notify(IDX(m_destructButtonFrame));
                }
            }
            break;
        }
        case DESTRUCT_WARNING_REVERSE: {
            SbiClockPair* clock = &m_destructWarningClock;
            i64 d = static_cast<i64>(g_frameTime) - clock->m_last;
            if (d >= clock->m_interval) {
                m_destructButtonFrame = static_cast<DestructButtonFrame>(m_destructButtonFrame - 1);
                if (m_destructButtonFrame <= DESTRUCT_FRAME_WARNING_FIRST) {
                    m_destructButtonFrame = DESTRUCT_FRAME_WARNING_FIRST;
                    m_destructWarningState = DESTRUCT_WARNING_FORWARD;
                }
                clock->m_interval = static_cast<u32>(
                    g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32)
                );
                clock->m_last = static_cast<u32>(g_frameTime);
                CSBI_ImageSet* destructButtonImage = m_destructButtonImage;
                if (destructButtonImage) {
                    destructButtonImage->Notify(IDX(m_destructButtonFrame));
                }
            }
            break;
        }
    }
}

RVA(0x0010b4f0, 0xaa)
void CStatusBarMgr::AdvanceTab(i32 reverse) {
    if (m_hlBusy != 0) {
        return;
    }
    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        return;
    }
    if (m_position == STATUSBAR_HIDDEN) {
        RestoreStatusBar();
    }
    if (m_activeTab != TAB_MULTIPLAYER) {
        SetTabState(SBICMD_TAB_MULTIPLAYER, MENUITEM_SELECTED);
        Deactivate();
        return;
    }
    if (reverse != 0) {
        if (++m_tabCycle < 0) {
            m_tabCycle = 3;
        }
    } else {
        if (++m_tabCycle >= 4) {
            m_tabCycle = 0;
        }
    }
    ResetWidgets(0);
    TryActivate();
    Deactivate();
}

RVA(0x0010b5d0, 0xdd)
i32 CStatusBarMgr::SelectToolResource(StatusBarHighlightRow row) {
    i32 rowIndex = IDX(row);
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_playerCommandPending == 0
        && m_resourceSlots[rowIndex].m_state == IDX(HLROW_IDLE_CYCLE)) {
        i32 handle = m_resourceSlots[rowIndex].m_value;
        i32* slot = &m_resourceSlots[rowIndex].m_value;
        if ((static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(handle)) {
            SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
            if (registry->m_silentMode == 0) {
                SoundCue* found = NULL;
                CMapStringToPtr* map = &registry->m_cues;
                MapLookup(*map, "GAME_TABHIGHLIGHT1", found);
                if (found) {
                    i32 soundEnabled = g_soundEnabled;
                    i32 volumePercent = g_soundVolumePercent;
                    if (soundEnabled != 0) {
                        SoundCue* p = found;
                        if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                            >= static_cast<u32>(p->m_replayDelayMs)) {
                            p->m_lastPlayTimeMs = g_soundCueTimeMs;
                            p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                        }
                    }
                }
            }
            m_pendingHlRow = row;
            *slot = 0;
            NotifyAllSlots();
            return 1;
        }
    }
    return 0;
}

RVA(0x0010b6f0, 0xdd)
i32 CStatusBarMgr::SelectToyResource(StatusBarHighlightRow row) {
    i32 rowIndex = IDX(row);
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_playerCommandPending == 0
        && m_resourceSlots[rowIndex + 4].m_state == IDX(HLROW_IDLE_CYCLE)) {
        i32 handle = m_resourceSlots[rowIndex + 4].m_value;
        i32* slot = &m_resourceSlots[rowIndex + 4].m_value;
        if ((static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(handle)) {
            SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
            if (registry->m_silentMode == 0) {
                SoundCue* found = NULL;
                CMapStringToPtr* map = &registry->m_cues;
                MapLookup(*map, "GAME_TABHIGHLIGHT1", found);
                if (found) {
                    i32 soundEnabled = g_soundEnabled;
                    i32 volumePercent = g_soundVolumePercent;
                    if (soundEnabled != 0) {
                        SoundCue* p = found;
                        if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                            >= static_cast<u32>(p->m_replayDelayMs)) {
                            p->m_lastPlayTimeMs = g_soundCueTimeMs;
                            p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                        }
                    }
                }
            }
            m_pendingHlRow = row;
            *slot = 0;
            NotifyAllSlots();
            return 1;
        }
    }
    return 0;
}

RVA(0x0010b810, 0xdd)
i32 CStatusBarMgr::SelectBrickResource(StatusBarHighlightRow row) {
    i32 rowIndex = IDX(row);
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_playerCommandPending == 0
        && m_resourceSlots[rowIndex + 8].m_state == IDX(HLROW_IDLE_CYCLE)) {
        i32 handle = m_resourceSlots[rowIndex + 8].m_value;
        i32* slot = &m_resourceSlots[rowIndex + 8].m_value;
        if ((static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(handle)) {
            SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
            if (registry->m_silentMode == 0) {
                SoundCue* found = NULL;
                CMapStringToPtr* map = &registry->m_cues;
                MapLookup(*map, "GAME_TABHIGHLIGHT1", found);
                if (found) {
                    i32 soundEnabled = g_soundEnabled;
                    i32 volumePercent = g_soundVolumePercent;
                    if (soundEnabled != 0) {
                        SoundCue* p = found;
                        if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                            >= static_cast<u32>(p->m_replayDelayMs)) {
                            p->m_lastPlayTimeMs = g_soundCueTimeMs;
                            p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                        }
                    }
                }
            }
            m_pendingHlRow = row;
            *slot = 0;
            NotifyAllSlots();
            return 1;
        }
    }
    return 0;
}

RVA(0x0010b930, 0x1a7)
i32 CStatusBarMgr::ActivateSlot(i32 idx) {

    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_playerCommandPending != 0) {
        goto notActivated;
    }
    if (idx == -1) {
        i32 slot;
        for (slot = 0; slot < 5; slot++) {
            if (m_slots[slot].m_state == SLOT_READY) {
                goto slotFound;
            }
        }
        return 0;

    slotFound:
        if (!(static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(0x66)) {
            goto notActivated;
        }
        SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
        if (registry->m_silentMode == 0) {
            SoundCue* found = NULL;
            CMapStringToPtr* map = &registry->m_cues;
            MapLookup(*map, "GAME_TABHIGHLIGHT1", found);
            if (found) {
                i32 soundEnabled = g_soundEnabled;
                i32 volumePercent = g_soundVolumePercent;
                if (soundEnabled != 0) {
                    SoundCue* p = found;
                    if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                        >= static_cast<u32>(p->m_replayDelayMs)) {
                        p->m_lastPlayTimeMs = g_soundCueTimeMs;
                        p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                    }
                }
            }
        }
        m_activeSlot = slot;
        m_slots[slot].m_value = 1;
        if (m_slotNotify[slot]) {
            m_slotNotify[slot]->Notify(1);
        }
        return 1;
    }
    {
        if (m_slots[idx].m_state != SLOT_READY) {
            goto notActivated;
        }
        if ((static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(0x66)) {
            SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
            if (registry->m_silentMode == 0) {
                SoundCue* found = NULL;
                CMapStringToPtr* map = &registry->m_cues;
                MapLookup(*map, "GAME_TABHIGHLIGHT1", found);
                if (found) {
                    i32 soundEnabled = g_soundEnabled;
                    i32 volumePercent = g_soundVolumePercent;
                    if (soundEnabled != 0) {
                        SoundCue* p = found;
                        if (g_soundCueTimeMs - static_cast<u32>(p->m_lastPlayTimeMs)
                            >= static_cast<u32>(p->m_replayDelayMs)) {
                            p->m_lastPlayTimeMs = g_soundCueTimeMs;
                            p->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                        }
                    }
                }
            }
            m_activeSlot = idx;
            m_slots[idx].m_value = 1;
            if (m_slotNotify[idx]) {
                m_slotNotify[idx]->Notify(1);
            }
            return 1;
        }
    }
notActivated:
    return 0;
}

RVA(0x0010bb50, 0x24)
void CStatusBarMgr::ReportTab(i32 tab) {
    UpdateFallingItemStatusBar(tab, 0x4f, 0x1b3);
    EnterHlRow(1, tab);
}

RVA(0x0010bb90, 0x3f)
void CStatusBarMgr::LockDestructButton(i32 resetWarningAnimation) {
    m_destructButtonLocked = 1;
    if (resetWarningAnimation && m_destructButtonFrame != DESTRUCT_FRAME_DISABLED) {
        m_destructWarningState = DESTRUCT_WARNING_INACTIVE;
        m_destructButtonFrame = DESTRUCT_FRAME_IDLE;
        if (m_destructButtonImage) {
            m_destructButtonImage->Notify(1);
        }
    }
}

RVA(0x0010bbe0, 0x34)
i32 CStatusBarMgr::GetActiveValue() {
    if (m_rezActive == 0) {
        return m_extraNotifyArg0;
    }
    if (m_rewardQueue.GetSize() > 0 && m_rewardQueue.GetSize() > m_rezTick) {
        return *static_cast<i32*>(m_rewardQueue.GetAt(m_rezTick));
    }
    return 0;
}
