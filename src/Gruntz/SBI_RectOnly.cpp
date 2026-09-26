#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/WorkerLookup.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/StreamFeeder.h>
#include <Enums.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/CurPlayer.h>
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
#include <Gruntz/SerialClockInline.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SerialRecordMacros.h>
#include <Gruntz/SerialRefLookup.h>
#include <Gruntz/SerialWorkerRefMacros.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundCueRegistryInline.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarItem.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarMgrBuilders.h>
#include <Gruntz/StatusBarMgrInline.h>
#include <Gruntz/StatusBarSerialMacros.h>
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
#include <MakeRect.h>
#include <RectMacros.h>
#include <Rez/RezList.h>
#include <Rez/RezMgr.h>
#include <SafeDelete.h>
#include <Utils/MapTyped.h>
#include <Utils/RegMgr.h>
#include <Wap32/ScreenGeometry.h>
#include <Wap32/TileGeometry.h>

#include <limits.h>
#include <math.h>
#include <new>
#include <stddef.h>
#include <string.h>

DATA(0x00244c54)
i32 g_curPlayer = 0;

// @early-stop
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
    m_rezActive = false;
    m_rezTick = 0;
    m_levelOverlayActive = false;
    m_quitConfirmationActive = false;
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
    if ((static_cast<CRegMgr*>(g_gameReg->m_settings))->Get("StatusBar Position", 0) == 1) {
        DockStatusBarLeft();
    }
    return 1;
}

RVA(0x000fe350, 0x6d)
void CStatusBarMgr::Teardown() {
    (static_cast<CRegMgr*>(g_gameReg->m_settings))->Set("StatusBar Position", IDX(m_position));
    ResetWidgets(false);
    ClearRewardQueue();
}

RVA(0x000fe3e0, 0x55)
i32 CStatusBarMgr::SetState(StatusBarDock state) {
    if (m_hlBusy != false) {
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
    if (m_hlBusy == false && m_position != STATUSBAR_DOCK_LEFT) {
        ResetWidgets(true);
        SetRect(&m_barRect, 0, 0, 0xa0, SCREEN_H_PX);
        SetState(STATUSBAR_DOCK_LEFT);
        (static_cast<CPlay*>(g_gameReg->m_curState))->ResetViewport();
        if (BuildStatusBarTabs() == 0) {
            g_gameReg->ReportError(s_activateErrId, 0x448);
            return 0;
        }
        SetTabState(static_cast<SbiCommandId>(IDX(m_activeTab)), MENUITEM_SELECTED);
    }
    return 1;
}

RVA(0x000fe520, 0xa9)
i32 CStatusBarMgr::DockStatusBarRight() {
    if (m_hlBusy != false) {
        return 1;
    }
    if (m_position == STATUSBAR_DOCK_RIGHT) {
        return 1;
    }
    ResetWidgets(true);

    tagSIZE screenSize = g_gameReg->m_modeSize;
    SetRect(&m_barRect, screenSize.cx - 0xa0, 0, screenSize.cx, SCREEN_H_PX);
    SetState(STATUSBAR_DOCK_RIGHT);
    (static_cast<CPlay*>(g_gameReg->m_curState))->ResetViewport();
    if (BuildStatusBarTabs() == 0) {
        g_gameReg->ReportError(s_activateErrId, 0x449);
        return 0;
    }
    SetTabState(static_cast<SbiCommandId>(IDX(m_activeTab)), MENUITEM_SELECTED);
    return 1;
}

RVA(0x000fe600, 0x49)
i32 CStatusBarMgr::HideRect() {
    if (m_hlBusy == false && m_position != STATUSBAR_HIDDEN) {
        ResetWidgets(true);
        SetRect(&m_barRect, -1, -1, -1, -1);
        SetState(STATUSBAR_HIDDEN);
        (static_cast<CPlay*>(g_gameReg->m_curState))->ResetViewport();
    }
    return 1;
}

RVA(0x000fe670, 0x2b)
i32 CStatusBarMgr::RestoreStatusBar() {
    if (m_hlBusy != false) {
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
            CDDrawWorker* cfg = m_world->FindWorker("GAME_STATUSBAR_MAINBAR");
            if (cfg) {
                CImage* entry = DDRAW_WORKER_FRAME_AT_UNCHECKED(cfg, cfg->m_minIndex);
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

// @early-stop
RVA(0x000fe860, 0x2d)
i32 CStatusBarMgr::SetSpritePos(i32 x, i32 y) {
    if (m_barSprite == NULL) {
        return 0;
    }
    SET_SCREEN_POS(m_barSprite, x, y);
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
            if (m_chatBoxDisabled != false) {
                break;
            }
            if (g_gameReg->m_triggerMgr->m_groupFlag == false) {
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
            if (m_levelOverlayActive != false) {
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
                    if (g_gameReg->m_frameGate != false) {
                        b32 gate = !g_gameReg->m_frameGate;
                        g_gameReg->m_frameGate = gate;
                        g_gameReg->FinishLevel(gate, true);
                    }
                    (static_cast<CPlay*>(g_gameReg->m_curState))->OpenLevelOverlay(true);
                    return 1;
                case SBICMD_GAME_TAB:
                    HiCueLookup();
                    SetTab(GAME_TAB_MENU, false);
                    return 1;
                case SBICMD_DESTRUCT:
                    if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
                        break;
                    }
                    if (m_destructButtonLocked != false) {
                        break;
                    }
                    if (m_chatBoxDisabled != false) {
                        break;
                    }
                    HiCueLookup();
                    {
                        CPlay* sm = static_cast<CPlay*>(g_gameReg->m_curState);
                        if (m_destructWarningState == DESTRUCT_WARNING_INACTIVE) {
                            m_destructWarningState = DESTRUCT_WARNING_FORWARD;
                            m_destructButtonFrame = DESTRUCT_FRAME_WARNING_FIRST;
                            m_destructWarningClock.Start(
                                g_buteMgr.GetDword("StatusBar", "DestructButtonWarningDelay", 0x32)
                            );
                            sm->SetDefeatCountdown(true, 0xbb7);
                        } else {
                            CSBI_ImageSet* destructButtonImage = m_destructButtonImage;
                            m_destructWarningState = DESTRUCT_WARNING_INACTIVE;
                            m_destructButtonFrame = DESTRUCT_FRAME_IDLE;
                            if (destructButtonImage) {
                                destructButtonImage->Notify(1);
                            }
                            sm->SetDefeatCountdown(false, 0xbb7);
                        }
                    }
                    return 1;
                default:
                    return 0;
            }
            break;

        case TAB_STATZ:
            if (m_chatBoxDisabled != false) {
                break;
            }
            if (g_gameReg->m_triggerMgr->m_groupFlag == false) {
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
            if (m_chatBoxDisabled != false) {
                break;
            }
            if (g_gameReg->m_triggerMgr->m_groupFlag == false) {
                break;
            }
            if (cmd < SBICMD_MULTIPLAYER_HEAD_FIRST || cmd > SBICMD_MULTIPLAYER_HEAD_LAST) {
                return 0;
            }
            HiCueLookup();
            m_tabCycle = IDX(cmd) - IDX(SBICMD_MULTIPLAYER_HEAD_FIRST);
            ResetWidgets(false);
            TryActivate();
            Deactivate();
            return 1;

        case TAB_GRUNTZ:
            if (m_chatBoxDisabled != false) {
                break;
            }
            if (g_gameReg->m_triggerMgr->m_groupFlag == false) {
                break;
            }
            if (cmd < SBICMD_GRUNT_SLOT_FIRST || cmd > SBICMD_GRUNT_SLOT_LAST) {
                return 0;
            }
            ActivateSlot(IDX(cmd) - IDX(SBICMD_GRUNT_SLOT_FIRST));
            return 1;

        case TAB_RESOURCE:
            if (m_chatBoxDisabled != false) {
                break;
            }
            if (g_gameReg->m_triggerMgr->m_groupFlag == false) {
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

RVA(0x000ff850, 0x121)
i32 CStatusBarMgr::HandleDoubleClick(i32 keyFlags, i32 x, i32 y) {
    CStatusBarItem* r = HitTestRects(x, y);
    if (r == NULL) {
        return 1;
    }
    r->OnDoubleClick(keyFlags, x, y);
    SbiCommandId cmd = r->m_cmd;
    switch (r->m_tab) {
        case TAB_STATZ:
            if (m_chatBoxDisabled == false && g_gameReg->m_triggerMgr->m_groupFlag != false
                && cmd >= SBICMD_CURSOR_TARGET_FIRST && cmd <= SBICMD_CURSOR_TARGET_LAST) {
                HiCueTimed();
                PlaceCursorTarget(IDX(cmd) - IDX(SBICMD_CURSOR_TARGET_FIRST), 1);
                return 1;
            }
            break;
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
    if (m_chatBoxDisabled == false) {
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
RVA(0x000ffb20, 0x13a)
i32 CStatusBarMgr::UpdateStatusBar(i32 deltaMs) {
    if (g_gameReg->m_soundEnabled != false) {
        if (m_destructWarningState != DESTRUCT_WARNING_INACTIVE
            && m_destructButtonLocked == false) {
            if (m_destructWarningSound == NULL) {

                SoundCueRegistry* registry = g_gameReg->m_world->m_soundRegistry;
                CMapStringToPtr* map = &registry->m_cues;
                SoundCue* found = MapFind<SoundCue>(*map, "GAME_DESTRUCT");
                if (found) {
                    SoundSample* sample = found->m_sound;
                    if (sample) {
                        SoundBuffer* voice = sample->AcquireInstance();
                        m_destructWarningSound = voice;
                        if (voice) {
                            voice->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, true);
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
            b32 hit = r->m_enabled;
            if (hit) {
                hit = ::PtInRect(&r->m_rect, x, y);
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
            b32 hit = r->m_enabled;
            if (hit) {
                hit = ::PtInRect(&r->m_rect, x, y);
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
            b32 hit = r->m_enabled;
            if (hit) {
                hit = ::PtInRect(&r->m_rect, x, y);
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
    if (m_tabsBuilt != false) {
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
            MakeRect(bx + 0x7c, by + 0xad, bx + 0x88, by + 0xb9),
            NULL,
            -1
        )) {
        delete dockLeft;
        return 0;
    }
    AddTabItem(0, dockLeft);

    CSBI_RectOnly* dockRight = new CSBI_RectOnly;
    if (!dockRight->Setup(
            this,
            code,
            SBICMD_DOCK_RIGHT,
            TAB_CONTROLS,
            MakeRect(bx + 0x8a, by + 0xad, bx + 0x96, by + 0xb9),
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
            MakeRect(bx + 0x83, by + 0xbb, bx + 0x8f, by + 0xc7),
            NULL,
            -1
        )) {
        delete hide;
        return 0;
    }
    AddTabItem(0, hide);

    CSBI_MenuItem* statzTab;
    NEW_STATUS_BAR_ITEM(
        statzTab,
        CSBI_MenuItem,
        code,
        SBICMD_TAB_STATZ,
        TAB_CONTROLS,
        MakeRect(bx + 0x42, by + 0x82, bx + 0x62, by + 0xad),
        "GAME_STATUSBAR_TABZ_STATZTAB",
        -1,
        0
    );
    AddTabItem(0, statzTab);
    m_statzTabButton = statzTab;

    CSBI_MenuItem* gruntzTab;
    NEW_STATUS_BAR_ITEM(
        gruntzTab,
        CSBI_MenuItem,
        code,
        SBICMD_TAB_GRUNTZ,
        TAB_CONTROLS,
        MakeRect(bx + 0x04, by + 0x82, bx + 0x24, by + 0xad),
        "GAME_STATUSBAR_TABZ_GRUNTZTAB",
        -1,
        0
    );
    AddTabItem(0, gruntzTab);
    m_gruntzTabButton = gruntzTab;

    CSBI_MenuItem* resourceTab;
    NEW_STATUS_BAR_ITEM(
        resourceTab,
        CSBI_MenuItem,
        code,
        SBICMD_TAB_RESOURCE,
        TAB_CONTROLS,
        MakeRect(bx + 0x24, by + 0x82, bx + 0x44, by + 0xad),
        "GAME_STATUSBAR_TABZ_RESOURCETAB",
        -1,
        0
    );
    AddTabItem(0, resourceTab);
    m_resourceTabButton = resourceTab;

    CSBI_MenuItem* multiTab;
    NEW_STATUS_BAR_ITEM(
        multiTab,
        CSBI_MenuItem,
        code,
        SBICMD_TAB_MULTIPLAYER,
        TAB_CONTROLS,
        MakeRect(bx + 0x60, by + 0x82, bx + 0x80, by + 0xad),
        "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB",
        -1,
        0
    );
    AddTabItem(0, multiTab);
    m_multiTabButton = multiTab;
    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        multiTab->m_state = MENUITEM_DISABLED;
        CDDrawWorker* f = multiTab->m_record;
        if (f != NULL) {
            multiTab->SetFrame(f->GetAt(IDX(MENUITEM_DISABLED)));
        }
        multiTab->SetEnabled(0);
        multiTab->RequestRedraw();
    }

    CSBI_MenuItem* gameTab;
    NEW_STATUS_BAR_ITEM(
        gameTab,
        CSBI_MenuItem,
        code,
        SBICMD_TAB_GAME,
        TAB_CONTROLS,
        MakeRect(bx + 0x7e, by + 0x82, bx + 0x9e, by + 0xad),
        "GAME_STATUSBAR_TABZ_GAMETAB",
        -1,
        0
    );
    AddTabItem(0, gameTab);
    m_gameTabButton = gameTab;

    if (BuildSideTabs() == 0) {
        return 0;
    }
    if (LoadTabSprites() == 0) {
        return 0;
    }
    if (BuildTabzDialog() == 0) {
        return 0;
    }
    m_tabsBuilt = true;
    return 1;
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
void CStatusBarMgr::ResetWidgets(b32 keepHost) {
    for (i32 t = 0; t < 8; t++) {
        DELETE_STATUS_ITEMS(m_tabLists[t])
    }
    if (keepHost) {
        if (m_barSprite) {

            m_barSprite->m_stateFlags |= SPRITE_STATE_HIDDEN;
            m_barSprite->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        }
    }
    m_statzTabButton = NULL;
    m_resourceTabButton = NULL;
    m_gruntzTabButton = NULL;
    m_multiTabButton = NULL;
    m_gameTabButton = NULL;
    m_gameResumePauseButton = NULL;
    m_gameLoadButton = NULL;
    m_gameSaveButton = NULL;
    m_gameSettingsButton = NULL;
    m_gameHelpButton = NULL;
    m_gameQuitButton = NULL;
    m_endPrimaryButton = NULL;
    m_endSecondaryButton = NULL;
    m_confirmYesButton = NULL;
    m_confirmNoButton = NULL;
    m_barSprite = NULL;
    i32 i;
    memset(m_hitRects, 0, sizeof(m_hitRects));
    memset(m_statObj, 0, sizeof(m_statObj));
    memset(m_slotNotify, 0, sizeof(m_slotNotify));
    memset(m_conveyorSprites, 0, sizeof(m_conveyorSprites));
    memset(m_resourceSlotSprites, 0, sizeof(m_resourceSlotSprites));
    memset(m_warlordHead, 0, sizeof(m_warlordHead));
    m_machineItemSprite = NULL;
    m_fallingItemSprite = NULL;
    m_destructButtonImage = NULL;
    m_resourceMainBackground = NULL;
    m_resourceUpperBackground = NULL;
    m_resourceWindowBackground = NULL;
    m_resourceMachineFramework = NULL;
    m_machineDisplay = NULL;
    m_gruntWellBackground = NULL;
    m_gruntWellGoo = NULL;
    m_tabsBuilt = false;
}

RVA(0x00100b00, 0x150)
void CStatusBarMgr::ClearTabGroup() {
    if (m_activeTab == TAB_NONE) {
        return;
    }
    DELETE_STATUS_ITEMS(m_tabLists[IDX(m_activeTab)])
    switch (m_activeTab) {
        case TAB_GAME:
            m_gameResumePauseButton = NULL;
            m_gameLoadButton = NULL;
            m_gameSaveButton = NULL;
            m_gameSettingsButton = NULL;
            m_gameHelpButton = NULL;
            m_gameQuitButton = NULL;
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
            m_gruntWellBackground = NULL;
            m_gruntWellGoo = NULL;
            break;
        }
        case TAB_RESOURCE: {

            memset(m_conveyorSprites, 0, sizeof(m_conveyorSprites));
            m_machineDisplay = NULL;

            memset(m_resourceSlotSprites, 0, sizeof(m_resourceSlotSprites));
            m_resourceMainBackground = NULL;
            m_resourceUpperBackground = NULL;
            m_resourceWindowBackground = NULL;
            m_resourceMachineFramework = NULL;
            m_machineItemSprite = NULL;
            m_fallingItemSprite = NULL;
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
    if (m_statzTabButton == NULL || m_resourceTabButton == NULL || m_gruntzTabButton == NULL
        || m_multiTabButton == NULL || m_gameTabButton == NULL) {
        return 0;
    }
    switch (cmd) {
        case SBICMD_TAB_STATZ:
            if (m_hlBusy) {
                return 1;
            }
            m_statzTabButton->SetState(state, 1);
            m_gruntzTabButton->ProbeState(state);
            m_resourceTabButton->ProbeState(state);
            m_multiTabButton->ProbeState(state);
            m_gameTabButton->ProbeState(state);
            break;
        case SBICMD_TAB_GRUNTZ:
            if (m_hlBusy) {
                return 1;
            }
            m_statzTabButton->ProbeState(state);
            m_gruntzTabButton->SetState(state, 1);
            m_resourceTabButton->ProbeState(state);
            m_multiTabButton->ProbeState(state);
            m_gameTabButton->ProbeState(state);
            break;
        case SBICMD_TAB_RESOURCE:
            if (m_hlBusy) {
                return 1;
            }
            m_statzTabButton->ProbeState(state);
            m_gruntzTabButton->ProbeState(state);
            m_resourceTabButton->SetState(state, 1);
            m_multiTabButton->ProbeState(state);
            m_gameTabButton->ProbeState(state);
            break;
        case SBICMD_TAB_MULTIPLAYER:
            if (m_hlBusy) {
                return 1;
            }
            m_statzTabButton->ProbeState(state);
            m_gruntzTabButton->ProbeState(state);
            m_resourceTabButton->ProbeState(state);
            m_multiTabButton->SetState(state, 1);
            m_gameTabButton->ProbeState(state);
            break;
        case SBICMD_TAB_GAME:
            if (m_hlBusy) {
                return 1;
            }
            m_statzTabButton->ProbeState(state);
            m_gruntzTabButton->ProbeState(state);
            m_resourceTabButton->ProbeState(state);
            m_multiTabButton->ProbeState(state);
            m_gameTabButton->SetState(state, 1);
            break;
        case SBICMD_PAUSE:
            if (m_hlBusy) {
                return 1;
            }
            m_gameResumePauseButton->SetState(state, 1);
            m_gameLoadButton->ProbeState(state);
            m_gameSaveButton->ProbeState(state);
            m_gameSettingsButton->ProbeState(state);
            m_gameHelpButton->ProbeState(state);
            m_gameQuitButton->ProbeState(state);
            break;
        case SBICMD_LOAD_GAME:
            if (m_hlBusy) {
                return 1;
            }
            m_gameResumePauseButton->ProbeState(state);
            m_gameLoadButton->SetState(state, 1);
            m_gameSaveButton->ProbeState(state);
            m_gameSettingsButton->ProbeState(state);
            m_gameHelpButton->ProbeState(state);
            m_gameQuitButton->ProbeState(state);
            break;
        case SBICMD_SAVE_GAME:
            if (m_hlBusy) {
                return 1;
            }
            m_gameResumePauseButton->ProbeState(state);
            m_gameLoadButton->ProbeState(state);
            m_gameSaveButton->SetState(state, 1);
            m_gameSettingsButton->ProbeState(state);
            m_gameHelpButton->ProbeState(state);
            m_gameQuitButton->ProbeState(state);
            break;
        case SBICMD_SETTINGS:
            if (m_hlBusy) {
                return 1;
            }
            m_gameResumePauseButton->ProbeState(state);
            m_gameLoadButton->ProbeState(state);
            m_gameSaveButton->ProbeState(state);
            m_gameSettingsButton->SetState(state, 1);
            m_gameHelpButton->ProbeState(state);
            m_gameQuitButton->ProbeState(state);
            break;
        case SBICMD_BOOTY_STATE:
            if (m_hlBusy) {
                return 1;
            }
            m_gameResumePauseButton->ProbeState(state);
            m_gameLoadButton->ProbeState(state);
            m_gameSaveButton->ProbeState(state);
            m_gameSettingsButton->ProbeState(state);
            m_gameHelpButton->SetState(state, 1);
            m_gameQuitButton->ProbeState(state);
            break;
        case SBICMD_QUIT:
            if (m_hlBusy) {
                return 1;
            }
            m_gameResumePauseButton->ProbeState(state);
            m_gameLoadButton->ProbeState(state);
            m_gameSaveButton->ProbeState(state);
            m_gameSettingsButton->ProbeState(state);
            m_gameHelpButton->ProbeState(state);
            m_gameQuitButton->SetState(state, 1);
            break;
        case SBICMD_GAME_TAB:
            if (m_hlBusy) {
                return 1;
            }
            m_gameQuitButton->SetState(state, 1);
            break;
        case SBICMD_DIALOG_PRIMARY:
            if (m_endPrimaryButton) {
                m_endPrimaryButton->SetState(state, 1);
            }
            m_endSecondaryButton->ProbeState(state);
            break;
        case SBICMD_DIALOG_SECONDARY:
            if (m_endPrimaryButton) {
                m_endPrimaryButton->ProbeState(state);
            }
            m_endSecondaryButton->SetState(state, 1);
            break;
        case SBICMD_DIALOG_YES:
            m_confirmYesButton->SetState(state, 1);
            m_confirmNoButton->ProbeState(state);
            break;
        case SBICMD_DIALOG_NO:
            m_confirmYesButton->ProbeState(state);
            m_confirmNoButton->SetState(state, 1);
            break;
    }
    return 1;
}

RVA(0x00101420, 0x110)
i32 CStatusBarMgr::ClearTabSprites(StatusBarTab idx) {
    if (idx == TAB_ALL || idx == TAB_CONTROLS) {
        if (m_statzTabButton) {
            m_statzTabButton->Blit();
        }
        if (m_gruntzTabButton) {
            m_gruntzTabButton->Blit();
        }
        if (m_resourceTabButton) {
            m_resourceTabButton->Blit();
        }
        if (m_multiTabButton) {
            m_multiTabButton->Blit();
        }
        if (m_gameTabButton) {
            m_gameTabButton->Blit();
        }
    }
    if (idx == TAB_GAME || idx == TAB_ALL) {
        if (m_gameResumePauseButton) {
            m_gameResumePauseButton->Blit();
        }
        if (m_gameLoadButton) {
            m_gameLoadButton->Blit();
        }
        if (m_gameSaveButton) {
            m_gameSaveButton->Blit();
        }
        if (m_gameSettingsButton) {
            m_gameSettingsButton->Blit();
        }
        if (m_gameHelpButton) {
            m_gameHelpButton->Blit();
        }
        if (m_gameQuitButton) {
            m_gameQuitButton->Blit();
        }
    }
    if (idx == TAB_DIALOG || idx == TAB_ALL) {
        if (m_endPrimaryButton) {
            m_endPrimaryButton->Blit();
        }
        if (m_endSecondaryButton) {
            m_endSecondaryButton->Blit();
        }
        if (m_confirmYesButton) {
            m_confirmYesButton->Blit();
        }
        if (m_confirmNoButton) {
            m_confirmNoButton->Blit();
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

        if (m_chatBoxDisabled != false && g_gameReg->m_frameGate != false) {
            CSBI_MenuItem* resume;
            NEW_STATUS_BAR_ITEM(
                resume,
                CSBI_MenuItem,
                code,
                SBICMD_PAUSE,
                TAB_GAME,
                MakeRect(bx, by + 0xd5, bx + 0x9f, by + 0xec),
                "GAME_STATUSBAR_TABZ_GAMETAB_RESUME",
                -1,
                0
            );
            AddTabItem(5, resume);
            m_gameResumePauseButton = resume;
        } else {
            CSBI_MenuItem* pause;
            NEW_STATUS_BAR_ITEM(
                pause,
                CSBI_MenuItem,
                code,
                SBICMD_PAUSE,
                TAB_GAME,
                MakeRect(bx, by + 0xd5, bx + 0x9f, by + 0xec),
                "GAME_STATUSBAR_TABZ_GAMETAB_PAUSE",
                -1,
                0
            );
            AddTabItem(5, pause);
            m_gameResumePauseButton = pause;
        }

        CSBI_MenuItem* load;
        NEW_STATUS_BAR_ITEM(
            load,
            CSBI_MenuItem,
            code,
            SBICMD_LOAD_GAME,
            TAB_GAME,
            MakeRect(bx, by + 0x125, bx + 0x9f, by + 0x13c),
            "GAME_STATUSBAR_TABZ_GAMETAB_LOAD",
            -1,
            0
        );
        AddTabItem(5, load);
        m_gameLoadButton = load;
        if (g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER) {
            load->SetEnabled(0);
        }

        CSBI_MenuItem* save;
        NEW_STATUS_BAR_ITEM(
            save,
            CSBI_MenuItem,
            code,
            SBICMD_SAVE_GAME,
            TAB_GAME,
            MakeRect(bx, by + 0xfd, bx + 0x9f, by + 0x114),
            "GAME_STATUSBAR_TABZ_GAMETAB_SAVE",
            -1,
            0
        );
        AddTabItem(5, save);
        m_gameSaveButton = save;
        if (g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER) {
            save->SetEnabled(0);
        }

        CSBI_MenuItem* settings;
        NEW_STATUS_BAR_ITEM(
            settings,
            CSBI_MenuItem,
            code,
            SBICMD_SETTINGS,
            TAB_GAME,
            MakeRect(bx, by + 0x14d, bx + 0x9f, by + 0x164),
            "GAME_STATUSBAR_TABZ_GAMETAB_SETTINGS",
            -1,
            0
        );
        AddTabItem(5, settings);
        m_gameSettingsButton = settings;

        CSBI_MenuItem* help;
        NEW_STATUS_BAR_ITEM(
            help,
            CSBI_MenuItem,
            code,
            SBICMD_BOOTY_STATE,
            TAB_GAME,
            MakeRect(bx, by + 0x175, bx + 0x9f, by + 0x18c),
            "GAME_STATUSBAR_TABZ_GAMETAB_HELP",
            -1,
            0
        );
        AddTabItem(5, help);
        m_gameHelpButton = help;
        if (g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER) {
            help->SetEnabled(0);
        }

        CSBI_MenuItem* quit;
        NEW_STATUS_BAR_ITEM(
            quit,
            CSBI_MenuItem,
            code,
            SBICMD_QUIT,
            TAB_GAME,
            MakeRect(bx, by + 0x19d, bx + 0x9f, by + 0x1b4),
            "GAME_STATUSBAR_TABZ_GAMETAB_QUIT",
            -1,
            0
        );
        AddTabItem(5, quit);
        m_gameQuitButton = quit;

        CSBI_ImageSet* destruct;
        NEW_STATUS_BAR_ITEM(
            destruct,
            CSBI_ImageSet,
            code,
            SBICMD_DESTRUCT,
            TAB_GAME,
            MakeRect(bx + 0x22, by + 0x1be, bx + 0x7d, by + 0x1d6),
            "GAME_STATUSBAR_TABZ_GAMETAB_DESTRUCT",
            IDX(m_destructButtonFrame),
            0
        );
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
        NEW_STATUS_BAR_ITEM(
            status,
            CSBI_ImageSet,
            code,
            SBICMD_MISSION_STATUS,
            TAB_GAME,
            MakeRect(bx, by + 0xd7, bx + 0x9f, by + 0x118),
            "GAME_STATUSBAR_TABZ_GAMETAB_MISSIONSTATUS",
            1,
            0
        );
        AddTabItem(5, status);
    } else {
        NEW_STATUS_BAR_ITEM(
            status,
            CSBI_ImageSet,
            code,
            SBICMD_MISSION_STATUS,
            TAB_GAME,
            MakeRect(bx, by + 0xd7, bx + 0x9f, by + 0x118),
            "GAME_STATUSBAR_TABZ_GAMETAB_MISSIONSTATUS",
            2,
            0
        );
        AddTabItem(5, status);
    }
    return 1;
}

RVA_COMPGEN(0x00101fd0, 0x1e, ??_GCSBI_ImageSet@@UAEPAXI@Z)

RVA_COMPGEN(0x00102000, 0x7f, ??1CSBI_ImageSet@@UAE@XZ)

RVA(0x001020a0, 0xae)
i32 CStatusBarMgr::SetTab(GameTabContent tab, b32 forceReload) {
    if (tab == m_itemKind && forceReload == false) {
        return 1;
    }
    DELETE_STATUS_ITEMS(m_tabLists[5])
    m_gameResumePauseButton = NULL;
    m_gameLoadButton = NULL;
    m_gameSaveButton = NULL;
    m_gameSettingsButton = NULL;
    m_gameHelpButton = NULL;
    m_gameQuitButton = NULL;
    m_itemKind = tab;

    if (!LoadTabSprites()) {
        g_gameReg->ReportError(s_activateErrId, s_setTabErrTag);
        return 0;
    }
    Deactivate();
    return 1;
}

RVA(0x00102180, 0x5f)
void CStatusBarMgr::BuildGameTabResumeButton(b32 show) {
    if (m_position == STATUSBAR_HIDDEN) {
        RestoreStatusBar();
    }
    if (show && m_activeTab != TAB_GAME) {
        SetTabState(SBICMD_TAB_GAME, MENUITEM_SELECTED);
    }
    if (m_gameResumePauseButton) {
        m_gameResumePauseButton->ResolveFrame("GAME_STATUSBAR_TABZ_GAMETAB_RESUME", 1);
        Deactivate();
        m_gameResumePauseButton->RequestRedraw();
    }
    m_chatBoxDisabled = true;
}

RVA(0x00102200, 0x37)
void CStatusBarMgr::BuildGameTabPauseButton() {
    if (m_gameResumePauseButton) {
        m_gameResumePauseButton->ResolveFrame("GAME_STATUSBAR_TABZ_GAMETAB_PAUSE", 1);
        Deactivate();
        m_gameResumePauseButton->RequestRedraw();
    }
    m_chatBoxDisabled = false;
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
            NEW_STATUS_BAR_ITEM(
                it,
                CSBI_Image,
                code,
                SBICMD_TAB_TITLE_TEXT,
                TAB_GRUNTZ,
                MakeRect(bx + 0x18, by + 0xaf, bx + 0x70, by + 0xbe),
                "GAME_STATUSBAR_TABZ_GRUNTZTAB_TITLETEXT",
                -1,
                0
            );
            AddTabItem(2, it);

            {
                CSBI_ImageSet** aptr = m_slotNotify;
                i32* bptr = &m_slots[0].m_value;
                i32 y = by + 0xfe;
                for (i = 0; i < 5; i++) {
                    CSBI_ImageSet* set;
                    NEW_STATUS_BAR_ITEM(
                        set,
                        CSBI_ImageSet,
                        code,
                        static_cast<SbiCommandId>(IDX(SBICMD_GRUNT_SLOT_FIRST) + i),
                        TAB_GRUNTZ,
                        MakeRect(bx + 0xe, y - 0x32, bx + 0x39, y),
                        "GAME_STATUSBAR_TABZ_GRUNTZTAB_GRUNTOVEN",
                        *bptr,
                        0
                    );
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
            NEW_STATUS_BAR_ITEM(
                it,
                CSBI_Image,
                code,
                SBICMD_GRUNT_WELL,
                TAB_GRUNTZ,
                MakeRect(bx + 0x4c, by + 0xc8, bx + 0x97, by + 0x1cd),
                "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELL",
                -1,
                0
            );
            AddTabItem(2, it);
            m_gruntWellBackground = it;
            it->SetEnabled(1);
            NEW_STATUS_BAR_ITEM(
                it,
                CSBI_Image,
                code,
                SBICMD_GRUNT_OVENS_TEXT,
                TAB_GRUNTZ,
                MakeRect(bx + 0x1e, by + 0xc4, bx + 0x3d, by + 0xcd),
                "GAME_STATUSBAR_TABZ_GRUNTZTAB_OVENZTEXT",
                -1,
                0
            );
            AddTabItem(2, it);
            NEW_STATUS_BAR_ITEM(
                it,
                CSBI_Image,
                code,
                SBICMD_GRUNT_WELL_TEXT,
                TAB_GRUNTZ,
                MakeRect(bx + 0x68, by + 0x1cf, bx + 0x87, by + 0x1d8),
                "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELLTEXT",
                -1,
                0
            );
            AddTabItem(2, it);
            goo = new CSBI_WellGoo;
            if (!goo->Setup(
                    this,
                    code,
                    SBICMD_GRUNT_WELL_GOO,
                    TAB_GRUNTZ,
                    MakeRect(bx + 0x6e, by + 0xf8, bx + 0x81, by + 0x1b3),
                    "GAME_STATUSBAR_TABZ_GRUNTZTAB_WELLGOO",
                    m_gruntWellLevel
                )) {
                delete goo;
                return 0;
            }
            m_gruntWellGoo = goo;
            AddTabItem(2, goo);
            return 1;

        case TAB_RESOURCE:
            NEW_STATUS_BAR_ITEM(
                it,
                CSBI_Image,
                code,
                SBICMD_TAB_TITLE_TEXT,
                TAB_RESOURCE,
                MakeRect(bx + 0x18, by + 0xaf, bx + 0x70, by + 0xbe),
                "GAME_STATUSBAR_TABZ_RESOURCETAB_TITLETEXT",
                -1,
                0
            );
            AddTabItem(3, it);
            NEW_STATUS_BAR_ITEM(
                it,
                CSBI_Image,
                code,
                SBICMD_RESOURCE_MAIN_BACKGROUND,
                TAB_RESOURCE,
                MakeRect(bx, by + 0x135, bx + 0x9f, by + 0x1be),
                "GAME_STATUSBAR_TABZ_RESOURCETAB_MAINBACKGROUND",
                -1,
                0
            );
            AddTabItem(3, it);
            m_resourceMainBackground = it;
            NEW_STATUS_BAR_ITEM(
                it,
                CSBI_Image,
                code,
                SBICMD_RESOURCE_UPPER_BACKGROUND,
                TAB_RESOURCE,
                MakeRect(bx, by + 0xfb, bx + 0x9f, by + 0x134),
                "GAME_STATUSBAR_TABZ_RESOURCETAB_UPPERBACKGROUND",
                -1,
                0
            );
            AddTabItem(3, it);
            m_resourceUpperBackground = it;
            NEW_STATUS_BAR_ITEM(
                it,
                CSBI_Image,
                code,
                SBICMD_RESOURCE_WINDOW_BACKGROUND,
                TAB_RESOURCE,
                MakeRect(bx + 0x48, by + 0xd3, bx + 0x67, by + 0xf3),
                "GAME_STATUSBAR_TABZ_RESOURCETAB_WINDOWBACKGROUND",
                -1,
                0
            );
            AddTabItem(3, it);
            m_resourceWindowBackground = it;

            NEW_STATUS_BAR_ITEM(
                imgSet,
                CSBI_ImageSet,
                code,
                SBICMD_RESOURCE_BELT_TOOLS,
                TAB_RESOURCE,
                MakeRect(bx + 0x19, by + 0x11c, bx + 0x3c, by + 0x130),
                "GAME_STATUSBAR_TABZ_RESOURCETAB_BELT",
                m_conveyorSlots[0].m_value,
                0
            );
            AddTabItem(3, imgSet);
            m_conveyorSprites[0] = imgSet;
            NEW_STATUS_BAR_ITEM(
                imgSet,
                CSBI_ImageSet,
                code,
                SBICMD_RESOURCE_BELT_TOYS,
                TAB_RESOURCE,
                MakeRect(bx + 0x40, by + 0x11c, bx + 0x63, by + 0x130),
                "GAME_STATUSBAR_TABZ_RESOURCETAB_BELT",
                m_conveyorSlots[1].m_value,
                0
            );
            AddTabItem(3, imgSet);
            m_conveyorSprites[1] = imgSet;
            NEW_STATUS_BAR_ITEM(
                imgSet,
                CSBI_ImageSet,
                code,
                SBICMD_RESOURCE_BELT_BRICKS,
                TAB_RESOURCE,
                MakeRect(bx + 0x68, by + 0x11c, bx + 0x8b, by + 0x130),
                "GAME_STATUSBAR_TABZ_RESOURCETAB_BELT",
                m_conveyorSlots[2].m_value,
                0
            );
            AddTabItem(3, imgSet);
            m_conveyorSprites[2] = imgSet;

            NEW_STATUS_BAR_ITEM(
                imgSet,
                CSBI_ImageSet,
                code,
                SBICMD_RESOURCE_CURRENT_ITEM,
                TAB_RESOURCE,
                MakeRect(
                    m_machineItemRect.left + bx,
                    m_machineItemRect.top + by,
                    m_machineItemRect.right + bx,
                    m_machineItemRect.bottom + by
                ),
                "GAME_INGAMEICONZ_GREYCHIPZ",
                m_machineItem,
                0
            );
            AddTabItem(3, imgSet);
            m_machineItemSprite = imgSet;
            imgSet->SetEnabled(0);

            {
                i32* cfgp = &m_resourceSlots[4].m_value;
                CSBI_ImageSet** cachep = &m_resourceSlotSprites[4];
                i32 y = by + 0x155;
                for (i = 0; i < 4; i++) {
                    CSBI_ImageSet* set;
                    NEW_STATUS_BAR_ITEM(
                        set,
                        CSBI_ImageSet,
                        code,
                        static_cast<SbiCommandId>(IDX(SBICMD_TOOL_RESOURCE_FIRST) + i),
                        TAB_RESOURCE,
                        MakeRect(bx + 0x1d, y - 0x17, bx + 0x34, y),
                        "GAME_INGAMEICONZ_NORMCHIPZ",
                        cfgp[-24],
                        0
                    );
                    AddTabItem(3, set);
                    cachep[-4] = set;
                    NEW_STATUS_BAR_ITEM(
                        set,
                        CSBI_ImageSet,
                        code,
                        static_cast<SbiCommandId>(IDX(SBICMD_TOY_RESOURCE_FIRST) + i),
                        TAB_RESOURCE,
                        MakeRect(bx + 0x45, y - 0x17, bx + 0x5c, y),
                        "GAME_INGAMEICONZ_NORMCHIPZ",
                        cfgp[0],
                        0
                    );
                    AddTabItem(3, set);
                    cachep[0] = set;
                    NEW_STATUS_BAR_ITEM(
                        set,
                        CSBI_ImageSet,
                        code,
                        static_cast<SbiCommandId>(IDX(SBICMD_BRICK_RESOURCE_FIRST) + i),
                        TAB_RESOURCE,
                        MakeRect(bx + 0x6d, y - 0x17, bx + 0x84, y),
                        "GAME_INGAMEICONZ_NORMCHIPZ",
                        cfgp[24],
                        0
                    );
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
                    MakeRect(bx, by + 0xc8, bx + 0x9f, by + 0xfa),
                    "GAME_STATUSBAR_TABZ_RESOURCETAB_MACHINE",
                    m_leftMachine.m_counter,
                    m_rightMachine.m_counter
                )) {
                delete mach;
                return 0;
            }
            m_machineDisplay = mach;
            AddTabItem(3, mach);

            NEW_STATUS_BAR_ITEM(
                it,
                CSBI_Image,
                code,
                SBICMD_RESOURCE_MACHINE_FOREGROUND,
                TAB_RESOURCE,
                MakeRect(bx, by + 0x135, bx + 0x9f, by + 0x1df),
                "GAME_STATUSBAR_TABZ_RESOURCETAB_FRAMEWORK",
                -1,
                0
            );
            AddTabItem(3, it);
            m_resourceMachineFramework = it;

            ani = new CSBI_ImageSetAni;
            if (!ani->Init(
                    this,
                    code,
                    SBICMD_CONVEYOR_TOP,
                    TAB_RESOURCE,
                    MakeRect(bx, by + 0x1bf, bx + 0x9f, by + 0x1cc),
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

            NEW_STATUS_BAR_ITEM(
                imgSet,
                CSBI_ImageSet,
                code,
                SBICMD_RESOURCE_FALLING_ITEM,
                TAB_RESOURCE,
                MakeRect(
                    m_fallingItemRect.left + bx,
                    m_fallingItemRect.top + by,
                    m_fallingItemRect.right + bx,
                    m_fallingItemRect.bottom + by
                ),
                "GAME_INGAMEICONZ_NORMCHIPZ",
                m_fallingItem,
                0
            );
            AddTabItem(3, imgSet);
            m_fallingItemSprite = imgSet;
            imgSet->SetEnabled(0);

            ani = new CSBI_ImageSetAni;
            if (!ani->Init(
                    this,
                    code,
                    SBICMD_CONVEYOR_BOTTOM,
                    TAB_RESOURCE,
                    MakeRect(bx, by + 0x1c7, bx + 0x9f, by + 0x1df),
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
            NEW_STATUS_BAR_ITEM(
                it,
                CSBI_Image,
                code,
                SBICMD_TAB_TITLE_TEXT,
                TAB_MULTIPLAYER,
                MakeRect(bx + 0x18, by + 0xaf, bx + 0x70, by + 0xbe),
                "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_TITLETEXT",
                -1,
                0
            );
            AddTabItem(4, it);

            NEW_STATUS_BAR_ITEM(
                head,
                CSBI_WarlordHead,
                code,
                SBICMD_MULTIPLAYER_HEAD1,
                TAB_MULTIPLAYER,
                MakeRect(bx + 0x53, by + 0xcf, bx + 0x8e, by + 0x10a),
                "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD1",
                1,
                0
            );
            m_warlordHead[0] = head;
            AddTabItem(4, head);
            NEW_STATUS_BAR_ITEM(
                head,
                CSBI_WarlordHead,
                code,
                SBICMD_MULTIPLAYER_HEAD2,
                TAB_MULTIPLAYER,
                MakeRect(bx + 0x53, by + 0x112, bx + 0x8e, by + 0x14d),
                "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD2",
                1,
                0
            );
            m_warlordHead[1] = head;
            AddTabItem(4, head);
            NEW_STATUS_BAR_ITEM(
                head,
                CSBI_WarlordHead,
                code,
                SBICMD_MULTIPLAYER_HEAD3,
                TAB_MULTIPLAYER,
                MakeRect(bx + 0x53, by + 0x155, bx + 0x8e, by + 0x190),
                "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD3",
                1,
                0
            );
            m_warlordHead[2] = head;
            AddTabItem(4, head);
            NEW_STATUS_BAR_ITEM(
                head,
                CSBI_WarlordHead,
                code,
                SBICMD_MULTIPLAYER_HEAD4,
                TAB_MULTIPLAYER,
                MakeRect(bx + 0x53, by + 0x197, bx + 0x8e, by + 0x1d2),
                "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD4",
                1,
                0
            );
            m_warlordHead[3] = head;
            AddTabItem(4, head);

            {
                CSBI_WarlordHead** slot = m_warlordHead;
                i32 pi = 0;
                do {
                    GruntzPlayer* p = &g_gameReg->m_players[pi];
                    CShadeTable* sel;
                    if (p->m_joined != false && p->m_doneFlag == false) {
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
                i32 gruntBarLeft = bx + 0x17;
                i32 gruntBarRight = bx + 0x52;
                i32 y = by + 0xd9;
                for (i = 0; i < TM_UNITS_PER_PLAYER; i++) {
                    bar = new CSBI_StatzTabGruntBar;
                    if (!bar->BuildMultiplayerTabStatusBar(
                            this,
                            code,
                            static_cast<SbiCommandId>(IDX(SBICMD_CURSOR_TARGET_FIRST) + i),
                            TAB_MULTIPLAYER,
                            MakeRect(gruntBarLeft, y - 0x11, gruntBarRight, y),
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
            NEW_STATUS_BAR_ITEM(
                it,
                CSBI_Image,
                code,
                SBICMD_TAB_TITLE_TEXT,
                TAB_STATZ,
                MakeRect(bx + 0x18, by + 0xaf, bx + 0x70, by + 0xbe),
                "GAME_STATUSBAR_TABZ_STATZTAB_TITLETEXT",
                -1,
                0
            );
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
                for (i = 0; i < TM_UNITS_PER_PLAYER; i++) {
                    SbiCommandId id =
                        static_cast<SbiCommandId>(IDX(SBICMD_CURSOR_TARGET_FIRST) + i);
                    arrow = new CSBI_StatzTabArrow;
                    if (!arrow->Init(
                            this,
                            code,
                            static_cast<SbiCommandId>(IDX(SBICMD_STAT_TOGGLE_FIRST) + i),
                            TAB_STATZ,
                            MakeRect(arrowL, y - 0x11, arrowR, y),
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
                    if (m_statFlags[i] != STATUS_SAMPLE_NONE) {
                        arrow->SetSampledDirection(m_position, false);
                    } else {
                        arrow->SetUnsampledDirection(m_position, false);
                    }
                    bar = new CSBI_StatzTabGruntBar;
                    if (!bar->BuildMultiplayerTabStatusBar(
                            this,
                            code,
                            id,
                            TAB_STATZ,
                            MakeRect(bx + 0x28, y - 0x11, bx + 0x77, y),
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
            NEW_STATUS_BAR_ITEM(
                it,
                CSBI_Image,
                code,
                SBICMD_TAB_TITLE_TEXT,
                TAB_GAME,
                MakeRect(bx + 0x18, by + 0xaf, bx + 0x70, by + 0xbe),
                "GAME_STATUSBAR_TABZ_GAMETAB_TITLETEXT",
                -1,
                0
            );
            AddTabItem(5, it);

            NEW_STATUS_BAR_ITEM(
                it,
                CSBI_ImageSet,
                code,
                SBICMD_WARPSTONE_BASE,
                TAB_GAME,
                MakeRect(bx, by, bx + 0x9f, by + 0x7f),
                "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                1,
                0
            );
            AddTabItem(5, it);
            if ((static_cast<CTriggerMgr*>(g_gameReg->m_triggerMgr))
                    ->ByteTableHas(WARPSTONE_FRAGMENT_FIRST)) {
                NEW_STATUS_BAR_ITEM(
                    it,
                    CSBI_ImageSet,
                    code,
                    SBICMD_WARPSTONE_FRAGMENT1,
                    TAB_GAME,
                    MakeRect(bx + 0x17, by + 0xe, bx + 0x52, by + 0x44),
                    "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                    2,
                    0
                );
                AddTabItem(5, it);
                if ((static_cast<CTriggerMgr*>(g_gameReg->m_triggerMgr))
                        ->ByteTableHas(WARPSTONE_FRAGMENT_SECOND)) {
                    NEW_STATUS_BAR_ITEM(
                        it,
                        CSBI_ImageSet,
                        code,
                        SBICMD_WARPSTONE_FRAGMENT2,
                        TAB_GAME,
                        MakeRect(bx + 0x4c, by + 0xf, bx + 0x87, by + 0x3e),
                        "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                        3,
                        0
                    );
                    AddTabItem(5, it);
                    if ((static_cast<CTriggerMgr*>(g_gameReg->m_triggerMgr))
                            ->ByteTableHas(WARPSTONE_FRAGMENT_THIRD)) {
                        NEW_STATUS_BAR_ITEM(
                            it,
                            CSBI_ImageSet,
                            code,
                            SBICMD_WARPSTONE_FRAGMENT3,
                            TAB_GAME,
                            MakeRect(bx + 0x1b, by + 0x3b, bx + 0x52, by + 0x71),
                            "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                            4,
                            0
                        );
                        AddTabItem(5, it);
                        if ((static_cast<CTriggerMgr*>(g_gameReg->m_triggerMgr))
                                ->ByteTableHas(WARPSTONE_FRAGMENT_FOURTH)) {
                            NEW_STATUS_BAR_ITEM(
                                it,
                                CSBI_ImageSet,
                                code,
                                SBICMD_WARPSTONE_FRAGMENT4,
                                TAB_GAME,
                                MakeRect(bx + 0x4a, by + 0x35, bx + 0x89, by + 0x74),
                                "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                                5,
                                0
                            );
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
        g_gameReg->ReportError(s_activateErrId, s_activateErrTag);
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
    CLAMP_UPPER_INPLACE(m_barX, w - 0x22);
    if (m_barY > d - 9) {
        m_barY = d - 0x22;
    }
    m_barSprite = (m_world)->m_childGroup->CreateSprite(
        0,
        m_barX,
        m_barY,
        SORTKEY_OVERLAY,
        "StatusBarSprite",
        IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION)
    );
    return m_barSprite != NULL;
}

RVA(0x00104e60, 0xed)
i32 CStatusBarMgr::LoadStatzTabToggleSprite(i32 idx, StatusSampleMode mode) {
    if (m_statFlags[idx] == mode) {
        return 1;
    }

    i32 slot = idx + TM_UNITS_PER_PLAYER * g_curPlayer;
    if (g_gameReg->m_triggerMgr->m_units[slot] == NULL) {
        return 0;
    }

    CSBI_SideTab* r = m_hitRects[idx];
    if (r != NULL) {
        r->m_sampleMode = mode;
        r->SetEnabled(1);
        if (m_activeTab == TAB_STATZ) {

            m_statObj[idx]->SetSampledDirection(m_position, true);
            PlayRegistryCueIfElapsed(g_gameReg->m_world->m_soundRegistry, "GAME_STATZTABTOGGLE");
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

            m_statObj[idx]->SetUnsampledDirection(m_position, true);
            PlayRegistryCueIfElapsed(g_gameReg->m_world->m_soundRegistry, "GAME_STATZTABTOGGLE");
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

        b32 ok = newobj->BuildStatzTabStatusBar(
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
        if (ok == false) {
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
    if (m_chatBoxDisabled == false) {
        for (i32 i = 0; i < TM_UNITS_PER_PLAYER; i++) {
            if (m_hitRects[i] && m_hitRects[i]->m_enabled) {
                CSBI_SideTab* p = m_hitRects[i];
                b32 hit = p->m_enabled ? ::PtInRect(&p->m_rect, x, y) : false;
                if (hit) {
                    return i;
                }
            }
        }
    }
    return -1;
}

// @early-stop
RVA(0x00105310, 0x11a)
void CStatusBarMgr::UpdateGruntOvenStatusBar() {

    CSBI_ImageSet** slot = m_slotNotify;
    CSbiSlot* tab = m_slots;
    i32 n = 5;
    do {
        if (tab->m_state == SLOT_FILLING) {
            i64 d = static_cast<i64>(g_frameTime) - tab->m_clock.m_start;

            i32 elapsed = (d < 0) ? 0 : static_cast<i32>(d);
            u32 delay = g_buteMgr.GetDword("StatusBar", "GruntOvenDelay", 0xc8);
            i32 frame = static_cast<i32>((static_cast<u32>(elapsed) / delay)) + 1;
            if (frame >= 0x1a) {
                tab->m_state = SLOT_READY;
                frame = 0x1a;
                PlayRegistryCueIfElapsed(
                    g_gameReg->m_world->m_soundRegistry,
                    "GAME_COOKINGCOMPLETE"
                );
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
void CStatusBarMgr::TickGruntWell() {
    b32 changed = false;
    i32 g = m_gruntWellLevel;
    i32 t = m_gruntWellTargetLevel;
    if (g < t) {
        g++;
    } else if (g <= t) {
        goto noChange;
    } else {
        g--;
    }
    m_gruntWellLevel = g;
    changed = true;
noChange:;
    if (m_gruntWellLevel == GRUNT_WELL_FULL) {
        if (AnySlotActive()) {
            changed = true;
            SetGruntWell(GRUNT_WELL_EMPTY);
        }
    }
    if (changed) {
        if (m_gruntWellGoo && m_gruntWellBackground) {
            m_gruntWellBackground->RequestRedraw();
            i32 fill = m_gruntWellLevel;
            CSBI_WellGoo* sink = m_gruntWellGoo;
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
    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ && m_hlBusy == false) {
        if (m_position == STATUSBAR_HIDDEN) {
            RestoreStatusBar();
        }
        if (m_activeTab != TAB_GRUNTZ) {
            SetTabState(SBICMD_TAB_GRUNTZ, MENUITEM_SELECTED);
        }
        Deactivate();
    }
    sp->m_state = SLOT_FILLING;

    m_slots[idx].m_clock.Start(INT_MAX);
    PlayTabCue(this, TAB_GRUNTZ, "GAME_GOOCOOKING1");
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
void CStatusBarMgr::AdvanceGruntWell(i32 delta) {
    i32 v = m_gruntWellLevel + delta;
    if (v >= GRUNT_WELL_FULL) {
        v = GRUNT_WELL_FULL;
    }
    m_gruntWellTargetLevel = v;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00105780, 0x1f)
void CStatusBarMgr::DrainGruntWell(i32 delta) {
    m_gruntWellTargetLevel =
        m_gruntWellLevel - delta > GRUNT_WELL_EMPTY ? m_gruntWellLevel - delta : GRUNT_WELL_EMPTY;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001057b0, 0xd)
void CStatusBarMgr::SetGruntWellTarget(i32 value) {
    m_gruntWellTargetLevel = value;
}

RVA(0x001057d0, 0x13)
void CStatusBarMgr::SetGruntWell(i32 value) {
    m_gruntWellTargetLevel = value;
    m_gruntWellLevel = value;
}

// @early-stop
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
                    obj->m_cameraTargetIdentity.Set(playerIndex, unitIndex);
                    obj->m_armed = true;
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
    TickGruntWell();
    UpdateRezConveyorStatusBar();
    LoadRezMachineConfig();
    LoadChipMachineConfig();
    UpdateChipGrinderStatusBar();
    UpdateDestructWarningAnimation();
}

RVA(0x00105920, 0x47)
void CStatusBarMgr::Reset() {
    ResetSlots();
    m_gruntWellTargetLevel = GRUNT_WELL_EMPTY;
    m_gruntWellLevel = GRUNT_WELL_EMPTY;
    ResetConveyorBelts();
    UpdateRezMachineSnoozeStatusBar();
    InitTabRects();
    m_destructButtonFrame = DESTRUCT_FRAME_IDLE;
    m_destructWarningState = DESTRUCT_WARNING_INACTIVE;
}

RVA(0x00105990, 0x3b4)
void CStatusBarMgr::UpdateRezConveyorStatusBar() {
    for (i32 i = 0; i < 3; i++) {
        ClockInterval* clock = &m_conveyorSlots[i].m_clock;
        SbiHlRowState state = static_cast<SbiHlRowState>(m_conveyorSlots[i].m_state);
        switch (state) {
            case HLROW_IDLE_CYCLE:
                if (++m_conveyorSlots[i].m_counter > 9) {
                    m_conveyorSlots[i].m_counter = 1;
                }
                break;
            case HLROW_RAMP_UP_LOW:
                if (clock->Expired()) {
                    if (++m_conveyorSlots[i].m_counter >= 0x12) {
                        m_conveyorSlots[i].m_counter = 0x12;
                        m_conveyorSlots[i].m_state = IDX(HLROW_HOLD_LOW);
                        clock->Start(
                            g_buteMgr.GetDword("StatusBar", "ConveyorBeltHoldDelay", 0x1f4)
                        );
                        UpdateFallingItemStatusBar(
                            m_machineItem,
                            m_machineItemRect.left + 0xc,
                            m_machineItemRect.top + 0xc
                        );
                        StartChipMachineCycle();
                    }
                }
                break;
            case HLROW_RAMP_DOWN_LOW:
                if (clock->Expired()) {
                    if (--m_conveyorSlots[i].m_counter < 0xa) {
                        m_conveyorSlots[i].m_state = IDX(HLROW_OFF);
                        m_conveyorSlots[i].m_counter = 1;
                    }
                }
                break;
            case HLROW_RAMP_UP_HIGH:
                if (clock->Expired()) {
                    if (++m_conveyorSlots[i].m_counter >= 0x18) {
                        m_conveyorSlots[i].m_counter = 0x18;
                        m_conveyorSlots[i].m_state = IDX(HLROW_HOLD_HIGH);
                        clock->Start(
                            g_buteMgr.GetDword("StatusBar", "ConveyorBeltHoldInDelay", 0x1f4)
                        );
                        m_machinePhase = BELT_FALLING_OFF;
                        m_beltClock.Start(
                            g_buteMgr.GetDword("StatusBar", "FallingItemDelay", 0x32)
                        );
                    }
                }
                break;
            case HLROW_RAMP_DOWN_HIGH:
                if (clock->Expired()) {
                    if (--m_conveyorSlots[i].m_counter < 0x13) {
                        m_conveyorSlots[i].m_state = IDX(HLROW_OFF);
                        m_conveyorSlots[i].m_counter = 1;
                    }
                }
                break;
            case HLROW_HOLD_HIGH:
                if (clock->Expired()) {
                    PlayTabCue(this, TAB_RESOURCE, "GAME_REZBELTRETURN");
                    m_conveyorSlots[i].m_state = IDX(HLROW_RAMP_DOWN_HIGH);
                }
                break;
            case HLROW_HOLD_LOW:
                if (clock->Expired()) {
                    PlayTabCue(this, TAB_RESOURCE, "GAME_REZBELTBACKUP");
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
    CSbiMachineRow* rightMachine = &m_rightMachine;
    CSbiMachineRow* leftMachine = &m_leftMachine;
    switch (static_cast<SbiMachineState>(rightMachine->m_state)) {
        case MACHINE_RIGHT_RUNNING:
            if (rightMachine->m_clock.Expired()) {
                if (++rightMachine->m_counter > 0x34) {
                    SetRightRezMachineAnimation(
                        0x2b,
                        MACHINE_RIGHT_RUNNING,
                        g_buteMgr.GetDword("StatusBar", "RightMachineRunningDelay", 0x7d)
                    );
                } else {
                    rightMachine->m_clock.Start(
                        g_buteMgr.GetDword("StatusBar", "RightMachineRunningDelay", 0x7d)
                    );
                }
            }
            break;
        case MACHINE_RIGHT_SPEWING:
            if (rightMachine->m_clock.Expired()) {
                if (++rightMachine->m_counter > 0x44) {
                    SetRightRezMachineAnimation(0x2b, MACHINE_STOPPED, INT_MAX);
                } else {
                    rightMachine->m_clock.Start(
                        g_buteMgr.GetDword("StatusBar", "RightMachineSpewingDelay", 0x7d)
                    );
                }
            }
            break;
    }

    switch (static_cast<SbiMachineState>(leftMachine->m_state)) {
        case MACHINE_SNOOZING:
            if (leftMachine->m_clock.Expired()) {
                if (++leftMachine->m_counter > 8) {
                    SetLeftRezMachineAnimation(
                        1,
                        MACHINE_SNOOZING,
                        g_buteMgr.GetDword("StatusBar", "LeftMachineSnoozingDelay", 0x64)
                    );
                } else {
                    leftMachine->m_clock.Start(
                        g_buteMgr.GetDword("StatusBar", "LeftMachineSnoozingDelay", 0x64)
                    );
                }
            }
            break;
        case MACHINE_WAKING:
            if (leftMachine->m_clock.Expired()) {
                if (++leftMachine->m_counter > 0x13) {
                    SetLeftRezMachineAnimation(
                        0x14,
                        MACHINE_TURNING_WHEEL,
                        g_buteMgr.GetDword("StatusBar", "LeftMachineTurningWheelDelay", 0x64)
                    );
                    SetRightRezMachineAnimation(
                        0x2b,
                        MACHINE_RIGHT_RUNNING,
                        g_buteMgr.GetDword("StatusBar", "RightMachineRunningDelay", 0x7d)
                    );
                    for (i32 i = 0; i < 3; i++) {
                        m_conveyorSlots[i].m_state = IDX(HLROW_IDLE_CYCLE);
                        m_conveyorSlots[i].m_value = 1;
                    }
                    m_machinePhase = BELT_IN_MACHINE;
                    m_beltClock.Start(g_buteMgr.GetDword("StatusBar", "NextItemDelay", 0x64));
                    PlayTabCue(this, TAB_RESOURCE, "GAME_REZMACHINE");
                } else {
                    leftMachine->m_clock.Start(
                        g_buteMgr.GetDword("StatusBar", "LeftMachineWakingDelay", 0x64)
                    );
                }
            }
            break;
        case MACHINE_TURNING_WHEEL:
            if (leftMachine->m_clock.Expired()) {
                if (++leftMachine->m_counter > 0x1d) {
                    SetLeftRezMachineAnimation(
                        0x14,
                        MACHINE_TURNING_WHEEL,
                        g_buteMgr.GetDword("StatusBar", "LeftMachineTurningWheelDelay", 0x64)
                    );
                } else {
                    leftMachine->m_clock.Start(
                        g_buteMgr.GetDword("StatusBar", "LeftMachineTurningWheelDelay", 0x64)
                    );
                }
            }
            break;
        case MACHINE_LEVER:
            if (leftMachine->m_clock.Expired()) {
                if (++leftMachine->m_counter == MACHINE_LEVER_RELEASE_FRAME) {
                    b32 found = false;
                    i32 r = 3;
                    i32 col;
                    PickupType which = static_cast<PickupType>(m_machineItem);
                    if (which >= PICKUP_BRICKZ_FIRST) {
                        col = 2;
                    } else {
                        col = (which >= PICKUP_TOYZ_FIRST) ? 1 : 0;
                    }
                    while (found == false) {
                        if (r < 0) {
                            break;
                        }
                        if (m_resourceSlots[col * 4 + r].m_state == IDX(HLROW_OFF)) {
                            found = true;
                        } else {
                            r--;
                        }
                    }
                    if (found) {
                        m_conveyorSlots[col].m_state = IDX(HLROW_RAMP_UP_HIGH);
                        m_conveyorSlots[col].m_counter = 0x13;
                        PlayTabCue(this, TAB_RESOURCE, "GAME_REZBELTRETRACT");
                    } else {
                        m_conveyorSlots[col].m_state = IDX(HLROW_RAMP_UP_LOW);
                        m_conveyorSlots[col].m_counter = 0xa;
                        PlayTabCue(this, TAB_RESOURCE, "GAME_REZBELTDROP");
                    }
                    m_conveyorSlots[col].m_clock.Start(
                        g_buteMgr.GetDword("StatusBar", "ConveyorBeltDelay", 0x64)
                    );
                }
                if (leftMachine->m_counter > 0x2a) {
                    SetLeftRezMachineAnimation(
                        1,
                        MACHINE_SNOOZING,
                        g_buteMgr.GetDword("StatusBar", "LeftMachineSnoozingDelay", 0x64)
                    );
                } else {
                    leftMachine->m_clock.Start(
                        g_buteMgr.GetDword("StatusBar", "LeftMachineLeverDelay", 0x64)
                    );
                }
            }
            break;
    }

    if (m_machineDisplay) {
        m_machineDisplay->SetFrames(leftMachine->m_counter, rightMachine->m_counter);
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
        g_buteMgr.GetDword("StatusBar", "LeftMachineSnoozingDelay", 100)
    );
    SetRightRezMachineAnimation(0x2b, MACHINE_STOPPED, INT_MAX);
    if (m_machineDisplay) {
        m_machineDisplay->SetFrames(m_leftMachine.m_counter, m_rightMachine.m_counter);
    }
    m_rezActive = false;
    m_rezTick = 0;
}

RVA(0x001066f0, 0x3b)
void CStatusBarMgr::SetLeftRezMachineAnimation(
    i32 initialFrame,
    SbiMachineState state,
    i32 frameDelayMs
) {
    m_leftMachine.m_counter = initialFrame;
    m_leftMachine.m_state = IDX(state);
    m_leftMachine.m_clock.Start(frameDelayMs);
}

RVA(0x00106740, 0x3b)
void CStatusBarMgr::SetRightRezMachineAnimation(
    i32 initialFrame,
    SbiMachineState state,
    i32 frameDelayMs
) {
    m_rightMachine.m_counter = initialFrame;
    m_rightMachine.m_state = IDX(state);
    m_rightMachine.m_clock.Start(frameDelayMs);
}

RVA(0x00106790, 0x62)
void CStatusBarMgr::CommitSlot(b32 active) {
    if (active) {
        ArmSlot(m_activeSlot);
        m_activeSlot = -1;
    } else {
        m_slots[m_activeSlot].m_value = s_slotCommitLevel;
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
    m_machineItem = 0;
    m_fallActive = FALLING_ITEM_INACTIVE;
    m_fallingItem = 0;
    SetRect(&m_fallingItemRect, 0, 0, 1, 1);
    SetRect(&m_machineItemRect, 0x49, 0xd7, 0x61, 0xef);
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
    if (m_resourceMainBackground) {
        m_resourceMainBackground->RequestRedraw();
    }
    if (m_resourceUpperBackground) {
        m_resourceUpperBackground->RequestRedraw();
    }
    if (m_resourceWindowBackground) {
        m_resourceWindowBackground->RequestRedraw();
    }
    if (m_machineItemSprite && m_machineItem) {
        m_machineItemSprite->Notify(m_machineItem);
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

    if (m_resourceMachineFramework) {
        m_resourceMachineFramework->RequestRedraw();
    }
    if (m_fallingItemSprite) {
        m_fallingItemSprite->Notify(m_fallingItem);
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

RVA(0x00106bb0, 0x7d8)
void CStatusBarMgr::LoadChipMachineConfig() {
    i32 refreshFlag = 0;
    i32 rectFlag = 0;
    ClockInterval* belt = &m_beltClock;
    switch (m_machinePhase) {
        case BELT_IN_MACHINE:
            if (belt->Expired()) {
                OFFSET_RECT_X_EDGES(
                    m_machineItemRect,
                    g_buteMgr.GetInt("StatusBar", "NextItemSpeed", 2),
                    g_buteMgr.GetInt("StatusBar", "NextItemSpeed", 2)
                );
                rectFlag = 1;
                belt->Start(g_buteMgr.GetDword("StatusBar", "NextItemDelay", 0x64));
            }
            if (m_machineItemRect.left >= 0x6d) {
                m_machineItemRect.left = 0x6d;
                m_machineItemRect.right = 0x84;
                rectFlag = 1;
                m_machinePhase = BELT_SPEWING;
                belt->Start(g_buteMgr.GetDword("StatusBar", "NextItemInMachineTime", 0x7d0));
            }
            refreshFlag = 1;
            break;
        case BELT_SPEWING:
            if (belt->Expired()) {
                SetRightRezMachineAnimation(
                    0x35,
                    MACHINE_RIGHT_SPEWING,
                    g_buteMgr.GetDword("StatusBar", "RightMachineSpewingDelay", 0x7d)
                );
                m_machinePhase = BELT_DROP_START;
                belt->Start(g_buteMgr.GetDword("StatusBar", "NextItemWaitTime", 0x1f4));
            }
            break;
        case BELT_DROP_START:
            if (belt->Expired()) {
                m_machinePhase = BELT_FALLING;
                PlayTabCue(this, TAB_RESOURCE, "GAME_CHIPFALLOUT");
                belt->Start(g_buteMgr.GetDword("StatusBar", "FallingItemDelay", 0x32));
            }
            break;
        case BELT_FALLING:
            if (belt->Expired()) {
                OFFSET_RECT_Y_EDGES(
                    m_machineItemRect,
                    g_buteMgr.GetInt("StatusBar", "FallingItemSpeed", 2),
                    g_buteMgr.GetInt("StatusBar", "FallingItemSpeed", 2)
                );
                rectFlag = 1;
                belt->Start(g_buteMgr.GetDword("StatusBar", "FallingItemDelay", 0x32));
            }
            if (m_machineItemRect.bottom >= 0x11c) {
                m_machineItemRect.bottom = 0x11c;
                m_machineItemRect.top = 0x104;
                rectFlag = 1;
                PlayTabCue(this, TAB_RESOURCE, "GAME_CHIPLAND");
                m_machinePhase = BELT_TRAVELLING;
                belt->Start(g_buteMgr.GetDword("StatusBar", "NextItemDelay", 0x64));
                PickupType activeItem = static_cast<PickupType>(m_machineItem);
                if (activeItem >= PICKUP_BRICKZ_FIRST) {
                    m_machineItemTargetX = 0x6d;
                } else if (activeItem >= PICKUP_TOYZ_FIRST) {
                    m_machineItemTargetX = 0x45;
                } else {
                    m_machineItemTargetX = 0x1d;
                }
            }
            refreshFlag = 1;
            break;
        case BELT_TRAVELLING:
            if (belt->Expired()) {
                OFFSET_RECT_X_EDGES(
                    m_machineItemRect,
                    -g_buteMgr.GetInt("StatusBar", "NextItemSpeed", 2),
                    -g_buteMgr.GetInt("StatusBar", "NextItemSpeed", 2)
                );
                rectFlag = 1;
                belt->Start(g_buteMgr.GetDword("StatusBar", "NextItemDelay", 0x64));
            }
            if (m_machineItemRect.left <= m_machineItemTargetX) {
                m_machineItemRect.left = m_machineItemTargetX;
                m_machineItemRect.right = m_machineItemTargetX + 0x17;
                rectFlag = 1;
                ResetConveyorBelts();
                SetLeftRezMachineAnimation(
                    0x1e,
                    MACHINE_LEVER,
                    g_buteMgr.GetDword("StatusBar", "LeftMachineLeverDelay", 0x64)
                );
                m_machinePhase = BELT_IDLE;
            }
            refreshFlag = 1;
            break;
        case BELT_FALLING_OFF: {
            if (belt->Expired()) {
                OFFSET_RECT_Y_EDGES(
                    m_machineItemRect,
                    g_buteMgr.GetInt("StatusBar", "FallingItemSpeed", 2),
                    g_buteMgr.GetInt("StatusBar", "(FallingItemSpeed", 2)
                );
                rectFlag = 1;
                belt->Start(g_buteMgr.GetDword("StatusBar", "FallingItemDelay", 0x32));
            }
            i32 col;
            PickupType item2 = static_cast<PickupType>(m_machineItem);
            if (item2 >= PICKUP_BRICKZ_FIRST) {
                col = 2;
            } else {
                col = (item2 >= PICKUP_TOYZ_FIRST) ? 1 : 0;
            }
            i32 row;
            CSbiHlRow* cell = &m_resourceSlots[col * 4 + 3];
            for (row = 3; row >= 0; row--, cell--) {
                if (cell->m_state != IDX(HLROW_IDLE_CYCLE)) {
                    break;
                }
            }
            if (m_machineItemRect.top >= row * 0x20 + 0x13e) {
                PlayTabCue(this, TAB_RESOURCE, "GAME_CHIPLAND");
                SetHlCell(col, m_machineItem, row);
                StartChipMachineCycle();
            }
            refreshFlag = 1;
            break;
        }
    }

    CSBI_ImageSet* w = m_machineItemSprite;
    if (w) {
        if (rectFlag) {
            RECT rc;
            i32 x = m_barRect.left;
            i32 y = m_barRect.top;
            SET_RECT_COMPONENTS(
                rc,
                m_machineItemRect.left + x,
                m_machineItemRect.top + y,
                m_machineItemRect.right + x,
                m_machineItemRect.bottom + y
            );
            w->m_rect = rc;
        }
        if (refreshFlag) {
            NotifyAllSlots();
        }
    }
}

// @early-stop
RVA(0x00107590, 0xc4)
i32 CStatusBarMgr::UpdateFallingItemStatusBar(i32 item, i32 x, i32 y) {
    m_fallingItem = item;
    m_fallActive = FALLING_ITEM_DESCENDING;
    m_fallClock.Start(g_buteMgr.GetDword("StatusBar", "FallingItemDelay", 0x32));
    CSBI_ImageSet* n = m_fallingItemSprite;
    i32 l = x - 0xc;
    i32 t = y - 0xc;
    i32 rr = x + 0xc;
    i32 b = y + 0xc;
    SET_RECT_COMPONENTS(m_fallingItemRect, l, t, rr, b);
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
        u32 delay = g_buteMgr.GetDword("StatusBar", "FallingItemDelay", 0x32);
        i32 speed = g_buteMgr.GetInt("StatusBar", "FallingItemSpeed", 4);

        if (m_fallingItemRect.top >= 0x1c7) {
            m_fallActive = FALLING_ITEM_INACTIVE;
            m_fallingItem = 0;
        } else if (m_fallingItemRect.bottom >= 0x1bf) {
            if (m_fallActive != FALLING_ITEM_GRINDING) {
                PlayTabCue(this, TAB_RESOURCE, "GAME_REZGRINDING");
                m_fallActive = FALLING_ITEM_GRINDING;
            }
            delay = g_buteMgr.GetDword("StatusBar", "FallingItemShredderDelay", 0x64);
            speed = g_buteMgr.GetInt("StatusBar", "FallingItemShredderSpeed", 2);
        }

        ClockInterval* clock = &m_fallClock;
        i64 d = static_cast<i64>(g_frameTime) - clock->m_start;
        if (d >= clock->m_interval) {
            OFFSET_RECT_Y_EDGES(m_fallingItemRect, speed, speed);
            CSBI_ImageSet* w = m_fallingItemSprite;
            if (w) {
                RECT rc;
                i32 sy = m_barRect.top;
                rc.bottom = sy + m_fallingItemRect.bottom;
                rc.top = sy + m_fallingItemRect.top;
                i32 sx = m_barRect.left;
                rc.left = m_fallingItemRect.left + sx;
                rc.right = m_fallingItemRect.right + sx;
                w->m_rect = rc;
            }
            clock->Start(delay);
        }
        stepped = 1;
    }

    if (m_fallingItemSprite != NULL && stepped) {
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
    if (m_rezActive == false) {
        if (m_machineItem == 0) {
            return 0;
        }
        SetLeftRezMachineAnimation(
            9,
            MACHINE_WAKING,
            g_buteMgr.GetDword("StatusBar", "LeftMachineWakingDelay", 100)
        );
        m_rezActive = true;
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
    SetTab(GAME_TAB_MENU, true);
    memset(m_statFlags, 0, sizeof(m_statFlags));
    Reset();

    GameModeId mode = g_gameReg->m_gameMode;
    if (mode == GAMEMODE_MULTIPLAYER) {
        for (i32 i = 0; i < g_buteMgr.GetInt("Multiplayer", "StartingGruntz", 0); i++) {
            m_slots[i].m_value = s_slotCommitLevel;
            m_slots[i].m_state = SLOT_READY;
        }
    } else if (mode == GAMEMODE_BATTLEZ) {
        for (i32 i = 0; i < g_buteMgr.GetInt("Battlez", "StartingGruntz", 0); i++) {
            m_slots[i].m_value = s_slotCommitLevel;
            m_slots[i].m_state = SLOT_READY;
        }
    }

    ClearRewardQueue();
    ClockInterval* clock = &m_reserved2b0;
    clock->m_start = 0;
    clock->m_interval = 0;
    m_hlBusy = false;
    SAFE_DELETE(m_retabNotify);
    ExitMode();
    m_observerTabAvailable = false;
    m_destructButtonLocked = false;
    TryActivate();
}
RVA(0x00107d00, 0x591)
i32 CStatusBarMgr::StartChipMachineCycle() {
    PickupType result;
    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        if (m_rewardQueue.GetSize() > 0) {
            Coord* p = GetReward(0);
            result = static_cast<PickupType>(p->m_x);
            g_coordPool.Push(p);
            m_rewardQueue.RemoveAt(0, 1);
        } else {
            result = PICKUP_NONE;
            if (m_machineItemSprite) {
                m_machineItemSprite->Notify(0);
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
    m_machineItem = IDX(result);
    m_machinePhase = BELT_IDLE;
    SetRect(&m_machineItemRect, 0x49, 0xd7, 0x61, 0xef);
    if (m_machineItemSprite) {
        RECT rc;
        i32 x = m_barRect.left;
        i32 y = m_barRect.top;
        SET_RECT_COMPONENTS(
            rc,
            m_machineItemRect.left + x,
            m_machineItemRect.top + y,
            m_machineItemRect.right + x,
            m_machineItemRect.bottom + y
        );
        m_machineItemSprite->m_rect = rc;
    }
    NotifyAllSlots();
    i32 c = m_rezTick;
    m_rezActive = false;
    if (c > 0) {
        m_rezTick = c - 1;
        UpdateRezMachineWakeStatusBar();
    }
    return 1;
}

RVA(0x00108410, 0x8e)
i32 CStatusBarMgr::QueuePickupReward(i32 pickupValue, i32 score) {
    Coord reward = {pickupValue, score};
    Coord* node = g_coordPool.PopCopy(reward);
    i32 n = m_rewardQueue.GetSize();
    for (i32 i = 0; i < n; i++) {
        Coord* e = GetReward(i);
        if (e != NULL && score < e->m_y) {
            m_rewardQueue.InsertAt(i, node, 1);
            return 1;
        }
    }
    m_rewardQueue.Add(node);
    return 1;
}

// @early-stop
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

    SerializeClockPair(s, mode, &m_beltClock);
    SerializeClockPair(s, mode, &m_fallClock);
    SerializeClockPair(s, mode, &m_rightMachine.m_clock);
    SerializeClockPair(s, mode, &m_leftMachine.m_clock);
    SerializeClockPair(s, mode, &m_destructWarningClock);

    CSbiSlot* p = m_slots;
    i32 n = 5;
    do {
        SerializeClockPair(s, mode, &p->m_clock);
        p++;
        n--;
    } while (n != 0);

    n = 3;
    CSbiHlRow* r = m_conveyorSlots;
    do {
        SerializeClockPair(s, mode, &r->m_clock);
        r++;
        n--;
    } while (n != 0);

    i32 outer = 3;
    CSbiHlRow* g = m_resourceSlots;
    do {
        n = 4;
        do {
            SerializeClockPair(s, mode, &g->m_clock);
            g++;
            n--;
        } while (n != 0);
        outer--;
    } while (outer != 0);

    SerializeClockPair(s, mode, &m_reserved2a0);
    SerializeClockPair(s, mode, &m_reserved2b0);
    if (mode == SERIAL_LOAD && m_position != STATUSBAR_HIDDEN) {
        BuildStatusBarTabs();
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

    SER(m_statzTabButton)
    SER(m_resourceTabButton)
    SER(m_gruntzTabButton)
    SER(m_multiTabButton)
    SER(m_gameTabButton)
    SER(m_gameResumePauseButton)
    SER(m_gameLoadButton)
    SER(m_gameSaveButton)
    SER(m_gameSettingsButton)
    SER(m_gameHelpButton)
    SER(m_gameQuitButton)
    SER(m_gameQuitButton)
    SER(m_endPrimaryButton)
    SER(m_endSecondaryButton)
    SER(m_confirmYesButton)
    SER(m_confirmNoButton)
    SER(m_gruntWellBackground)
    SER(m_gruntWellGoo)
    SER(m_machineDisplay)
    SER(m_resourceMainBackground)
    SER(m_resourceMachineFramework)
    SER(m_resourceUpperBackground)
    SER(m_resourceWindowBackground)
    SER(m_machineItemSprite)
    SER(m_fallingItemSprite)
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
    for (i32 i = 0; i < TM_UNITS_PER_PLAYER; i++) {
        s->Write(p, sizeof(*p));
        p += 1;
    }

    s->Write(&m_reserved34c, sizeof(m_reserved34c));
    s->Write(&m_reserved350, sizeof(m_reserved350));
    s->Write(&m_chatBoxDisabled, sizeof(m_chatBoxDisabled));
    s->Write(&m_activeSlot, sizeof(m_activeSlot));
    s->Write(&m_pendingHlRow, sizeof(m_pendingHlRow));
    s->Write(&m_activeTab, sizeof(m_activeTab));
    s->Write(&m_gruntWellLevel, sizeof(m_gruntWellLevel));
    s->Write(&m_gruntWellTargetLevel, sizeof(m_gruntWellTargetLevel));
    s->Write(&m_machineItemTargetX, sizeof(m_machineItemTargetX));
    s->Write(&m_rezTick, sizeof(m_rezTick));
    s->Write(&m_rezActive, sizeof(m_rezActive));
    s->Write(&m_reserved544, sizeof(m_reserved544));
    s->Write(&m_fallingItemRect, sizeof(m_fallingItemRect));
    s->Write(&m_machineItemRect, sizeof(m_machineItemRect));
    s->Write(&m_hlBusy, sizeof(m_hlBusy));
    s->Write(&m_levelOverlayActive, sizeof(m_levelOverlayActive));
    s->Write(&m_quitConfirmationActive, sizeof(m_quitConfirmationActive));
    s->Write(&m_machinePhase, sizeof(m_machinePhase));
    s->Write(&m_machineItem, sizeof(m_machineItem));
    s->Write(&m_fallActive, sizeof(m_fallActive));
    s->Write(&m_fallingItem, sizeof(m_fallingItem));
    s->Write(&m_rightMachine, 4);
    s->Write(&m_rightMachine.m_value, sizeof(m_rightMachine.m_value));
    s->Write(&m_leftMachine, 4);
    s->Write(&m_leftMachine.m_value, sizeof(m_leftMachine.m_value));
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
        s->Write(GetReward(n), sizeof(Coord));
    }
    return 1;
}

RVA(0x00109520, 0x44c)
i32 CStatusBarMgr::Deserialize(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* dir = g_gameReg->m_world;
    if (dir == NULL) {
        return 0;
    }
    m_destructWarningSound = NULL;
    ResetWidgets(false);

    ar->Read(this, 4);
    ar->Read(&m_restorePosition, sizeof(m_restorePosition));

    SERIALREF(m_barSprite);

    ar->Read(&m_barRect.left, sizeof(m_barRect));
    ar->Read(&m_redrawFrames, sizeof(m_redrawFrames));
    ar->Read(&m_barX, sizeof(m_barX));
    ar->Read(&m_barY, sizeof(m_barY));
    ar->Read(&m_itemKind, sizeof(m_itemKind));
    ar->Read(&m_tabCycle, sizeof(m_tabCycle));

    StatusSampleMode* p = m_statFlags;
    for (i32 i = 0; i < TM_UNITS_PER_PLAYER; i++) {
        ar->Read(p, sizeof(*p));
        p += 1;
    }

    ar->Read(&m_reserved34c, sizeof(m_reserved34c));
    ar->Read(&m_reserved350, sizeof(m_reserved350));
    ar->Read(&m_chatBoxDisabled, sizeof(m_chatBoxDisabled));
    ar->Read(&m_activeSlot, sizeof(m_activeSlot));
    ar->Read(&m_pendingHlRow, sizeof(m_pendingHlRow));
    ar->Read(&m_activeTab, sizeof(m_activeTab));
    ar->Read(&m_gruntWellLevel, sizeof(m_gruntWellLevel));
    ar->Read(&m_gruntWellTargetLevel, sizeof(m_gruntWellTargetLevel));
    ar->Read(&m_machineItemTargetX, sizeof(m_machineItemTargetX));
    ar->Read(&m_rezTick, sizeof(m_rezTick));
    ar->Read(&m_rezActive, sizeof(m_rezActive));
    ar->Read(&m_reserved544, sizeof(m_reserved544));
    ar->Read(&m_fallingItemRect, sizeof(m_fallingItemRect));
    ar->Read(&m_machineItemRect, sizeof(m_machineItemRect));
    ar->Read(&m_hlBusy, sizeof(m_hlBusy));
    ar->Read(&m_levelOverlayActive, sizeof(m_levelOverlayActive));
    ar->Read(&m_quitConfirmationActive, sizeof(m_quitConfirmationActive));
    ar->Read(&m_machinePhase, sizeof(m_machinePhase));
    ar->Read(&m_machineItem, sizeof(m_machineItem));
    ar->Read(&m_fallActive, sizeof(m_fallActive));
    ar->Read(&m_fallingItem, sizeof(m_fallingItem));
    ar->Read(&m_rightMachine, 4);
    ar->Read(&m_rightMachine.m_value, sizeof(m_rightMachine.m_value));
    ar->Read(&m_leftMachine, 4);
    ar->Read(&m_leftMachine.m_value, sizeof(m_leftMachine.m_value));
    ar->Read(&m_destructWarningState, sizeof(m_destructWarningState));
    ar->Read(&m_destructButtonFrame, sizeof(m_destructButtonFrame));
    ar->Read(&m_destructButtonLocked, sizeof(m_destructButtonLocked));
    ar->Read(&m_observerTabAvailable, sizeof(m_observerTabAvailable));

    for (i32 j = 0; j < 5; j++) {
        ar->Read(&m_slots[j].m_state, sizeof(m_slots[j].m_state));
        ar->Read(&m_slots[j].m_value, sizeof(m_slots[j].m_value));
    }
    for (i32 k = 0; k < 3; k++) {
        ar->Read(&m_conveyorSlots[k].m_state, sizeof(m_conveyorSlots[k].m_state));
        ar->Read(&m_conveyorSlots[k].m_value, sizeof(m_conveyorSlots[k].m_value));
    }
    CSbiHlRow* nb = m_resourceSlots;
    i32 seq = 3;
    do {
        for (i32 m = 0; m < 4; m++) {
            ar->Read(&nb[m].m_state, sizeof(nb[m].m_state));
            ar->Read(&nb[m].m_value, sizeof(nb[m].m_value));
        }
        nb += 4;
    } while (--seq);

    ClearRewardQueue();

    i32 cnt;
    ar->Read(&cnt, sizeof(cnt));
    m_rewardQueue.SetSize(cnt, -1);
    for (u32 n = 0; n < static_cast<u32>(cnt); n++) {
        Coord* node = g_coordPool.Pop();
        ar->Read(node, sizeof(Coord));
        m_rewardQueue.SetAt(n, node);
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

RVA(0x00109bd0, 0x1b5)
i32 CWarpStoneFly::Init(CStatusBarMgr* owner, i32 srcX, i32 srcY, WarpStoneFragment fragment) {
    m_owner = owner;

    i32 n = IDX(fragment) + 1;
    CImage* frame = g_gameReg->m_world->FindFrame("GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE", n);
    m_sprite = frame;
    if (frame == NULL) {

        return 0;
    }

    m_arrivalMode = fragment;
    Coord targetOffset;
    switch (fragment) {
        case WARPSTONE_FRAGMENT_SECOND:
            targetOffset.Set(0x69, 0x26);
            break;
        case WARPSTONE_FRAGMENT_THIRD:
            targetOffset.Set(0x65, 0x50);
            break;
        case WARPSTONE_FRAGMENT_FOURTH:
            targetOffset.Set(0x69, 0x54);
            break;
        default:
            targetOffset.Set(0x34, 0x29);
            break;
    }

    CStatusBarMgr* base = m_owner;
    i32 tx = base->m_barRect.left + targetOffset.m_x;
    m_targetX = tx;
    i32 ty = base->m_barRect.top + targetOffset.m_y;
    m_targetY = ty;

    i32 deltaX = tx - srcX;
    i32 dyv = ty - srcY;
    i32 dist2 = SquaredDistance(deltaX, dyv);
    double dist = sqrt(static_cast<double>(dist2));
    u32 flyTime = g_buteMgr.GetDword("WarpStone", "FlyTime", 0x5dc);

    m_velocityScale = dist / static_cast<double>(flyTime);
    m_xDirection = static_cast<double>(deltaX) / dist;
    m_yDirection = static_cast<double>(dyv) / dist;

    SoundCueRegistry* h = g_gameReg->m_world->m_soundRegistry;
    PlayRegistryCueIfElapsed(h, "GAME_WARPSTONEFLY");

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
            char name[SERIAL_NAME_LEN];
            i32 index;
            SERIAL_READ_FRAME(arc, lvl, name, index, m_sprite);
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
        arr->Add(static_cast<BYTE>(mode));
        m_owner->m_hlBusy = false;
        if (m_owner->m_position != STATUSBAR_HIDDEN && m_owner->m_activeTab == TAB_GAME) {
            m_owner->ResetWidgets(false);
            m_owner->TryActivate();
        }
        CStatusBarMgr* owner = m_owner;
        SAFE_DELETE(owner->m_retabNotify);
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
    if (m_levelOverlayActive == false) {
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

    if (m_quitConfirmationActive != false) {
        cx -= 0x5e;
        cy -= 0x3c;

        CSBI_Image* areYouSure;
        NEW_STATUS_BAR_ITEM(
            areYouSure,
            CSBI_Image,
            w,
            SBICMD_DIALOG_FRAME,
            TAB_DIALOG,
            MakeRect(cx, cy, cx + 0xbc, cy + 0x79),
            "GAME_STATUSBAR_TABZ_DIALOG_AREYOUSURE",
            -1,
            0
        );
        AddTabItem(6, areYouSure);

        CSBI_MenuItem* yes;
        NEW_STATUS_BAR_ITEM(
            yes,
            CSBI_MenuItem,
            w,
            SBICMD_DIALOG_YES,
            TAB_DIALOG,
            MakeRect(cx + 0x19, cy + 0x4d, cx + 0x4c, cy + 0x64),
            "GAME_STATUSBAR_TABZ_DIALOG_YES",
            -1,
            0
        );
        AddTabItem(6, yes);
        m_confirmYesButton = yes;

        CSBI_MenuItem* no;
        NEW_STATUS_BAR_ITEM(
            no,
            CSBI_MenuItem,
            w,
            SBICMD_DIALOG_NO,
            TAB_DIALOG,
            MakeRect(cx + 0x6b, cy + 0x4d, cx + 0x9e, cy + 0x64),
            "GAME_STATUSBAR_TABZ_DIALOG_NO",
            -1,
            0
        );
        AddTabItem(6, no);
        m_confirmNoButton = no;
        return 1;
    }

    cx -= 0x8e;
    cy -= 0x48;

    i32 reason = IDX(g_gameReg->m_triggerMgr->m_finishReasonFrame);

    CSBI_Image* dialog;
    NEW_STATUS_BAR_ITEM(
        dialog,
        CSBI_Image,
        w,
        SBICMD_DIALOG_FRAME,
        TAB_DIALOG,
        MakeRect(cx, cy, cx + 0x11c, cy + 0x90),
        "GAME_STATUSBAR_TABZ_DIALOG",
        -1,
        0
    );
    AddTabItem(6, dialog);

    if (g_gameReg->m_triggerMgr->m_phase == FINISH_STATE_VICTORY) {

        CSBI_ImageSet* status;
        NEW_STATUS_BAR_ITEM(
            status,
            CSBI_ImageSet,
            w,
            SBICMD_DIALOG_MISSION_STATUS,
            TAB_DIALOG,
            MakeRect(cx, cy + 0x17, cx + 0x11b, cy + 0x32),
            "GAME_STATUSBAR_TABZ_DIALOG_MISSIONSTATUS",
            1,
            0
        );
        AddTabItem(6, status);

        CSBI_ImageSet* rsn;
        NEW_STATUS_BAR_ITEM(
            rsn,
            CSBI_ImageSet,
            w,
            SBICMD_DIALOG_REASON,
            TAB_DIALOG,
            MakeRect(cx + 0x12, cy + 0x37, cx + 0x101, cy + 0x4c),
            "GAME_STATUSBAR_TABZ_DIALOG_REASON",
            reason,
            0
        );
        AddTabItem(6, rsn);

        if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
            CSBI_MenuItem* next;
            NEW_STATUS_BAR_ITEM(
                next,
                CSBI_MenuItem,
                w,
                SBICMD_DIALOG_PRIMARY,
                TAB_DIALOG,
                MakeRect(cx + 0x11, cy + 0x5f, cx + 0x80, cy + 0x7a),
                "GAME_STATUSBAR_TABZ_DIALOG_PLAYNEXTLEVEL",
                -1,
                0
            );
            AddTabItem(6, next);
            m_endPrimaryButton = next;

            CSBI_MenuItem* quit;
            NEW_STATUS_BAR_ITEM(
                quit,
                CSBI_MenuItem,
                w,
                SBICMD_DIALOG_SECONDARY,
                TAB_DIALOG,
                MakeRect(cx + 0x8e, cy + 0x5f, cx + 0xfd, cy + 0x7a),
                "GAME_STATUSBAR_TABZ_DIALOG_QUITTOMAINMENU",
                -1,
                0
            );
            AddTabItem(6, quit);
            m_endSecondaryButton = quit;
            return 1;
        } else {
            CSBI_MenuItem* statz;
            NEW_STATUS_BAR_ITEM(
                statz,
                CSBI_MenuItem,
                w,
                SBICMD_DIALOG_SECONDARY,
                TAB_DIALOG,
                MakeRect(cx + 0x55, cy + 0x5f, cx + 0xc4, cy + 0x7a),
                "GAME_STATUSBAR_TABZ_DIALOG_STATZ",
                -1,
                0
            );
            AddTabItem(6, statz);
            m_endSecondaryButton = statz;
            return 1;
        }
    }

    CSBI_ImageSet* status;
    NEW_STATUS_BAR_ITEM(
        status,
        CSBI_ImageSet,
        w,
        SBICMD_DIALOG_MISSION_STATUS,
        TAB_DIALOG,
        MakeRect(cx, cy + 0x17, cx + 0x11b, cy + 0x32),
        "GAME_STATUSBAR_TABZ_DIALOG_MISSIONSTATUS",
        2,
        0
    );
    AddTabItem(6, status);

    CSBI_ImageSet* rsn;
    NEW_STATUS_BAR_ITEM(
        rsn,
        CSBI_ImageSet,
        w,
        SBICMD_DIALOG_REASON,
        TAB_DIALOG,
        MakeRect(cx + 0x12, cy + 0x37, cx + 0x101, cy + 0x4c),
        "GAME_STATUSBAR_TABZ_DIALOG_REASON",
        reason,
        0
    );
    AddTabItem(6, rsn);

    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        CSBI_MenuItem* replay;
        NEW_STATUS_BAR_ITEM(
            replay,
            CSBI_MenuItem,
            w,
            SBICMD_DIALOG_PRIMARY,
            TAB_DIALOG,
            MakeRect(cx + 0x11, cy + 0x5f, cx + 0x80, cy + 0x7a),
            "GAME_STATUSBAR_TABZ_DIALOG_REPLAYLEVEL",
            -1,
            0
        );
        AddTabItem(6, replay);
        m_endPrimaryButton = replay;

        CSBI_MenuItem* quit;
        NEW_STATUS_BAR_ITEM(
            quit,
            CSBI_MenuItem,
            w,
            SBICMD_DIALOG_SECONDARY,
            TAB_DIALOG,
            MakeRect(cx + 0x8e, cy + 0x5f, cx + 0xfd, cy + 0x7a),
            "GAME_STATUSBAR_TABZ_DIALOG_QUITTOMAINMENU",
            -1,
            0
        );
        AddTabItem(6, quit);
        m_endSecondaryButton = quit;
        return 1;
    }

    i32 count = 0;
    for (i32 i = 0; i < 4; i++) {
        if (g_gameReg->m_players[i].m_joined != false && g_gameReg->m_players[i].m_doneFlag == false
            && g_gameReg->m_players[i].m_clearedRound == false) {
            count++;
        }
    }

    if (count >= 2) {
        CSBI_MenuItem* observe;
        NEW_STATUS_BAR_ITEM(
            observe,
            CSBI_MenuItem,
            w,
            SBICMD_DIALOG_PRIMARY,
            TAB_DIALOG,
            MakeRect(cx + 0x11, cy + 0x5f, cx + 0x80, cy + 0x7a),
            "GAME_STATUSBAR_TABZ_DIALOG_OBSERVE",
            -1,
            0
        );
        AddTabItem(6, observe);
        m_endPrimaryButton = observe;
        m_observerTabAvailable = true;

        CSBI_MenuItem* statz;
        NEW_STATUS_BAR_ITEM(
            statz,
            CSBI_MenuItem,
            w,
            SBICMD_DIALOG_SECONDARY,
            TAB_DIALOG,
            MakeRect(cx + 0x8e, cy + 0x5f, cx + 0xfd, cy + 0x7a),
            "GAME_STATUSBAR_TABZ_DIALOG_STATZ",
            -1,
            0
        );
        AddTabItem(6, statz);
        m_endSecondaryButton = statz;
    } else {
        m_observerTabAvailable = false;
        CSBI_MenuItem* statz;
        NEW_STATUS_BAR_ITEM(
            statz,
            CSBI_MenuItem,
            w,
            SBICMD_DIALOG_SECONDARY,
            TAB_DIALOG,
            MakeRect(cx + 0x55, cy + 0x5f, cx + 0xc4, cy + 0x7a),
            "GAME_STATUSBAR_TABZ_DIALOG_STATZ",
            -1,
            0
        );
        AddTabItem(6, statz);
        m_endSecondaryButton = statz;
    }
    return 1;
}

RVA(0x0010b210, 0xc5)
void CStatusBarMgr::ExitMode() {
    if (m_levelOverlayActive == false) {
        return;
    }
    DELETE_STATUS_ITEMS(m_tabLists[6])
    b32 wasQuitConfirmation = m_quitConfirmationActive;
    m_endPrimaryButton = NULL;
    m_endSecondaryButton = NULL;
    m_confirmYesButton = NULL;
    m_confirmNoButton = NULL;
    m_hlBusy = false;
    if (wasQuitConfirmation == false && g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
        if (m_position == STATUSBAR_HIDDEN) {
            RestoreStatusBar();
        }
        if (m_activeTab != TAB_GAME) {
            SetTabState(SBICMD_TAB_GAME, MENUITEM_SELECTED);
        }
        SetTab(GAME_TAB_MENU, true);
        Deactivate();
    } else {
        m_chatBoxDisabled = false;
    }
    m_levelOverlayActive = false;
    m_quitConfirmationActive = false;
    Deactivate();
}

RVA(0x0010b320, 0x167)
void CStatusBarMgr::UpdateDestructWarningAnimation() {

    switch (m_destructWarningState) {
        case DESTRUCT_WARNING_FORWARD: {
            ClockInterval* clock = &m_destructWarningClock;
            i64 d = static_cast<i64>(g_frameTime) - clock->m_start;
            if (d >= clock->m_interval) {
                m_destructButtonFrame = static_cast<DestructButtonFrame>(m_destructButtonFrame + 1);
                if (m_destructButtonFrame >= DESTRUCT_FRAME_WARNING_LAST) {
                    m_destructButtonFrame = DESTRUCT_FRAME_WARNING_LAST;
                    m_destructWarningState = DESTRUCT_WARNING_REVERSE;
                }
                clock->Start(g_buteMgr.GetDword("StatusBar", "DestructButtonWarningDelay", 0x32));
                CSBI_ImageSet* destructButtonImage = m_destructButtonImage;
                if (destructButtonImage) {
                    destructButtonImage->Notify(IDX(m_destructButtonFrame));
                }
            }
            break;
        }
        case DESTRUCT_WARNING_REVERSE: {
            ClockInterval* clock = &m_destructWarningClock;
            i64 d = static_cast<i64>(g_frameTime) - clock->m_start;
            if (d >= clock->m_interval) {
                m_destructButtonFrame = static_cast<DestructButtonFrame>(m_destructButtonFrame - 1);
                if (m_destructButtonFrame <= DESTRUCT_FRAME_WARNING_FIRST) {
                    m_destructButtonFrame = DESTRUCT_FRAME_WARNING_FIRST;
                    m_destructWarningState = DESTRUCT_WARNING_FORWARD;
                }
                clock->Start(g_buteMgr.GetDword("StatusBar", "DestructButtonWarningDelay", 0x32));
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
    if (m_hlBusy != false) {
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
    ResetWidgets(false);
    TryActivate();
    Deactivate();
}

RVA(0x0010b5d0, 0xdd)
i32 CStatusBarMgr::SelectToolResource(StatusBarHighlightRow row) {
    i32 rowIndex = IDX(row);
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_playerCommandPending == false
        && m_resourceSlots[rowIndex].m_state == IDX(HLROW_IDLE_CYCLE)) {
        i32 handle = m_resourceSlots[rowIndex].m_value;
        i32* slot = &m_resourceSlots[rowIndex].m_value;
        if ((static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(handle)) {
            HiCueTimed();
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
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_playerCommandPending == false
        && m_resourceSlots[rowIndex + 4].m_state == IDX(HLROW_IDLE_CYCLE)) {
        i32 handle = m_resourceSlots[rowIndex + 4].m_value;
        i32* slot = &m_resourceSlots[rowIndex + 4].m_value;
        if ((static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(handle)) {
            HiCueTimed();
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
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_playerCommandPending == false
        && m_resourceSlots[rowIndex + 8].m_state == IDX(HLROW_IDLE_CYCLE)) {
        i32 handle = m_resourceSlots[rowIndex + 8].m_value;
        i32* slot = &m_resourceSlots[rowIndex + 8].m_value;
        if ((static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(handle)) {
            HiCueTimed();
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
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_playerCommandPending == false) {
        if (idx == -1) {
            for (i32 slot = 0; slot < 5; slot++) {
                if (m_slots[slot].m_state == SLOT_READY) {
                    return ActivateReadySlot(slot);
                }
            }
            return 0;
        }
        if (m_slots[idx].m_state == SLOT_READY) {
            return ActivateReadySlot(idx);
        }
    }
    return 0;
}

RVA(0x0010bb50, 0x24)
void CStatusBarMgr::ReportTab(i32 tab) {
    UpdateFallingItemStatusBar(tab, 0x4f, 0x1b3);
    EnterHlRow(1, tab);
}

RVA(0x0010bb90, 0x3f)
void CStatusBarMgr::LockDestructButton(i32 resetWarningAnimation) {
    m_destructButtonLocked = true;
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
    if (m_rezActive == false) {
        return m_machineItem;
    }
    if (m_rewardQueue.GetSize() > 0 && m_rewardQueue.GetSize() > m_rezTick) {
        return GetReward(m_rezTick)->m_x;
    }
    return 0;
}
