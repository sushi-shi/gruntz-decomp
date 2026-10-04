#include <StdAfx.h>

#include <rva.h>

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
#include <Globals.h>
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
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SerialRecordMacros.h>
#include <Gruntz/SerialRecords.h>
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
i32 CStatusBarMgr::Initialize(CDDrawSurfaceMgr* world) {
    m_world = world;
    m_restorePosition = STATUSBAR_DOCK_RIGHT;
    m_position = STATUSBAR_DOCK_RIGHT;
    i32 vx = g_gameReg->m_modeSize.cx;
    i32 vy = g_gameReg->m_modeSize.cy;
    SetRect(&m_barRect, vx - 0xa0, 0, vx, SCREEN_H_PX);
    m_redrawFrames = 0;
    m_collapsedSpriteX = vx - 0x45;
    m_collapsedSpriteY = vy - 0x30;
    m_gameTabContent = GAME_TAB_MENU;
    m_multiplayerPlayerIndex = g_curPlayer;
    Reset();
    if (BuildStatusBarTabs() == 0) {
        return 0;
    }
    m_selectedGruntOvenSlot = -1;
    m_selectedResourceRow = STATUS_HL_ROW_NONE;
    m_resourceDeliveryActive = false;
    m_pendingResourceDeliveries = 0;
    m_levelOverlayActive = false;
    m_quitConfirmationActive = false;
    m_randomRewardThresholds[0] = g_buteMgr.GetInt("Multiplayer", "ToolzPercent");
    m_randomRewardThresholds[1] =
        m_randomRewardThresholds[0] + g_buteMgr.GetInt("Multiplayer", "ToyzPercent");
    m_randomRewardThresholds[2] =
        m_randomRewardThresholds[1] + g_buteMgr.GetInt("Multiplayer", "BrickzPercent");
    m_randomRewardThresholds[3] = g_buteMgr.GetInt("Multiplayer", "RedBrick");
    m_randomRewardThresholds[4] =
        m_randomRewardThresholds[3] + g_buteMgr.GetInt("Multiplayer", "BlueBrick");
    m_randomRewardThresholds[5] =
        m_randomRewardThresholds[4] + g_buteMgr.GetInt("Multiplayer", "GoldBrick");
    m_randomRewardThresholds[6] =
        m_randomRewardThresholds[5] + g_buteMgr.GetInt("Multiplayer", "BlackBrick");
    m_randomRewardThresholds[7] = g_buteMgr.GetInt("Multiplayer", "BabyWalkerz");
    m_randomRewardThresholds[8] =
        m_randomRewardThresholds[7] + g_buteMgr.GetInt("Multiplayer", "BeachBallz");
    m_randomRewardThresholds[9] =
        m_randomRewardThresholds[8] + g_buteMgr.GetInt("Multiplayer", "BigWheelz");
    m_randomRewardThresholds[10] =
        m_randomRewardThresholds[9] + g_buteMgr.GetInt("Multiplayer", "GoKartz");
    m_randomRewardThresholds[11] =
        m_randomRewardThresholds[10] + g_buteMgr.GetInt("Multiplayer", "JackInTheBoxz");
    m_randomRewardThresholds[12] =
        m_randomRewardThresholds[11] + g_buteMgr.GetInt("Multiplayer", "JumpRopez");
    m_randomRewardThresholds[13] =
        m_randomRewardThresholds[12] + g_buteMgr.GetInt("Multiplayer", "PogoStickz");
    m_randomRewardThresholds[14] =
        m_randomRewardThresholds[13] + g_buteMgr.GetInt("Multiplayer", "Scrollz");
    m_randomRewardThresholds[15] =
        m_randomRewardThresholds[14] + g_buteMgr.GetInt("Multiplayer", "SqueakToyz");
    m_randomRewardThresholds[16] =
        m_randomRewardThresholds[15] + g_buteMgr.GetInt("Multiplayer", "Yoyoz");
    m_randomRewardThresholds[17] = g_buteMgr.GetInt("Multiplayer", "Bombz");
    m_randomRewardThresholds[18] =
        m_randomRewardThresholds[17] + g_buteMgr.GetInt("Multiplayer", "Boomerangz");
    m_randomRewardThresholds[19] =
        m_randomRewardThresholds[18] + g_buteMgr.GetInt("Multiplayer", "Brickz");
    m_randomRewardThresholds[20] =
        m_randomRewardThresholds[19] + g_buteMgr.GetInt("Multiplayer", "Clubz");
    m_randomRewardThresholds[21] =
        m_randomRewardThresholds[20] + g_buteMgr.GetInt("Multiplayer", "Gauntletz");
    m_randomRewardThresholds[22] =
        m_randomRewardThresholds[21] + g_buteMgr.GetInt("Multiplayer", "Glovez");
    m_randomRewardThresholds[23] =
        m_randomRewardThresholds[22] + g_buteMgr.GetInt("Multiplayer", "Gooberz");
    m_randomRewardThresholds[24] =
        m_randomRewardThresholds[23] + g_buteMgr.GetInt("Multiplayer", "GravityBootz");
    m_randomRewardThresholds[25] =
        m_randomRewardThresholds[24] + g_buteMgr.GetInt("Multiplayer", "GunHatz");
    m_randomRewardThresholds[26] =
        m_randomRewardThresholds[25] + g_buteMgr.GetInt("Multiplayer", "NerfGunz");
    m_randomRewardThresholds[27] =
        m_randomRewardThresholds[26] + g_buteMgr.GetInt("Multiplayer", "Rockz");
    m_randomRewardThresholds[28] =
        m_randomRewardThresholds[27] + g_buteMgr.GetInt("Multiplayer", "Shieldz");
    m_randomRewardThresholds[29] =
        m_randomRewardThresholds[28] + g_buteMgr.GetInt("Multiplayer", "Shovelz");
    m_randomRewardThresholds[30] =
        m_randomRewardThresholds[29] + g_buteMgr.GetInt("Multiplayer", "Springz");
    m_randomRewardThresholds[31] =
        m_randomRewardThresholds[30] + g_buteMgr.GetInt("Multiplayer", "Spyz");
    m_randomRewardThresholds[32] =
        m_randomRewardThresholds[31] + g_buteMgr.GetInt("Multiplayer", "Swordz");
    m_randomRewardThresholds[33] =
        m_randomRewardThresholds[32] + g_buteMgr.GetInt("Multiplayer", "TimeBombz");
    m_randomRewardThresholds[34] =
        m_randomRewardThresholds[33] + g_buteMgr.GetInt("Multiplayer", "Toobz");
    m_randomRewardThresholds[35] =
        m_randomRewardThresholds[34] + g_buteMgr.GetInt("Multiplayer", "Wandz");
    m_randomRewardThresholds[36] =
        m_randomRewardThresholds[35] + g_buteMgr.GetInt("Multiplayer", "Welderz");
    m_randomRewardThresholds[37] =
        m_randomRewardThresholds[36] + g_buteMgr.GetInt("Multiplayer", "Wingz");
    SetButtonState(SBICMD_TAB_GAME, MENUITEM_SELECTED);
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
i32 CStatusBarMgr::SetDockState(StatusBarDock state) {
    if (m_layoutLocked != false) {
        return 1;
    }
    StatusBarDock old = m_position;
    if (old == state) {
        return 1;
    }
    if (state == STATUSBAR_HIDDEN) {
        if (CreateCollapsedSprite() == 0) {
            return 0;
        }
        m_restorePosition = m_position;
    } else {
        RequestRedraw();
    }
    old = m_position;
    m_position = state;
    (static_cast<CPlay*>(g_gameReg->GetCurrentState()))->OnStatusBarDockChanged(state, old);
    return 1;
}

RVA(0x000fe460, 0x83)
i32 CStatusBarMgr::DockStatusBarLeft() {
    if (m_layoutLocked == false && m_position != STATUSBAR_DOCK_LEFT) {
        ResetWidgets(true);
        SetRect(&m_barRect, 0, 0, 0xa0, SCREEN_H_PX);
        SetDockState(STATUSBAR_DOCK_LEFT);
        (static_cast<CPlay*>(g_gameReg->GetCurrentState()))->ResetViewport();
        if (BuildStatusBarTabs() == 0) {
            g_gameReg->ReportError(s_activateErrId, 0x448);
            return 0;
        }
        SetButtonState(static_cast<SbiCommandId>(IDX(m_activeTab)), MENUITEM_SELECTED);
    }
    return 1;
}

RVA(0x000fe520, 0xa9)
i32 CStatusBarMgr::DockStatusBarRight() {
    if (m_layoutLocked != false) {
        return 1;
    }
    if (m_position == STATUSBAR_DOCK_RIGHT) {
        return 1;
    }
    ResetWidgets(true);

    tagSIZE screenSize = g_gameReg->m_modeSize;
    SetRect(&m_barRect, screenSize.cx - 0xa0, 0, screenSize.cx, SCREEN_H_PX);
    SetDockState(STATUSBAR_DOCK_RIGHT);
    (static_cast<CPlay*>(g_gameReg->GetCurrentState()))->ResetViewport();
    if (BuildStatusBarTabs() == 0) {
        g_gameReg->ReportError(s_activateErrId, 0x449);
        return 0;
    }
    SetButtonState(static_cast<SbiCommandId>(IDX(m_activeTab)), MENUITEM_SELECTED);
    return 1;
}

RVA(0x000fe600, 0x49)
i32 CStatusBarMgr::HideStatusBar() {
    if (m_layoutLocked == false && m_position != STATUSBAR_HIDDEN) {
        ResetWidgets(true);
        SetRect(&m_barRect, -1, -1, -1, -1);
        SetDockState(STATUSBAR_HIDDEN);
        (static_cast<CPlay*>(g_gameReg->GetCurrentState()))->ResetViewport();
    }
    return 1;
}

RVA(0x000fe670, 0x2b)
i32 CStatusBarMgr::RestoreStatusBar() {
    if (m_layoutLocked != false) {
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
i32 CStatusBarMgr::Render() {
    if (m_position != STATUSBAR_HIDDEN) {
        if (m_redrawFrames > 0) {
            m_redrawFrames--;
            i32 v = m_displayHeight;
            if (v > SCREEN_H_PX) {
                CDDSurface* tgt = (g_gameReg->World()->m_drawTarget)->m_backPair->GetSurface();

                RECT below;
                below.left = m_barRect.left;
                below.top = m_barRect.bottom;
                below.right = m_barRect.right;
                below.bottom = v;
                tgt->Restore(&below, 0);
            }
            CImageSet* cfg = m_world->FindImageSet("GAME_STATUSBAR_MAINBAR");
            if (cfg) {
                CImage* entry = IMAGE_SET_FRAME_AT_UNCHECKED(cfg, cfg->GetMinIndex());
                if (entry) {
                    CDisplayBuffers* l1 = g_gameReg->World()->m_drawTarget;
                    entry->RenderFrame(
                        l1->m_backPair,
                        entry->GetAnchorX() + m_barRect.left,
                        entry->GetAnchorY() + m_barRect.top,
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
        if (m_warpStoneFly) {
            m_warpStoneFly->Draw();
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
i32 CStatusBarMgr::SetCollapsedSpritePosition(i32 x, i32 y) {
    if (m_collapsedSprite == NULL) {
        return 0;
    }
    SET_SCREEN_POS(m_collapsedSprite, x, y);
    m_collapsedSpriteX = x;
    m_collapsedSpriteY = y;
    return 1;
}

RVA(0x000fe8a0, 0x4e)
i32 CStatusBarMgr::HitTestCollapsedSprite(i32 x, i32 y) {
    CWwdSpriteObject* r = m_collapsedSprite;
    CImage* L = r->GetFrameImage();
    i32 xlo = r->m_screenX - L->GetAnchorX();
    i32 ylo = r->m_screenY - L->GetAnchorY();
    i32 xhi = L->GetWidth() + xlo;
    i32 yhi = L->GetHeight() + ylo;
    if (x >= xhi || x < xlo || y >= yhi || y < ylo) {
        return 0;
    }
    return 1;
}

RVA(0x000fe910, 0xc30)
i32 CStatusBarMgr::HandleClick(i32 mouseFlags, i32 x, i32 y) {
    CStatusBarItem* w = HitTestItems(x, y);
    if (w == NULL) {
        return 1;
    }
    w->OnClick(mouseFlags, x, y);
    SbiCommandId cmd = w->GetCommandId();
    switch (w->GetTab()) {
        case TAB_CONTROLS:
            if (m_gameplayControlsDisabled != false) {
                break;
            }
            if (g_gameReg->m_triggerMgr->m_playerControlEnabled == false) {
                break;
            }
            switch (cmd) {
                case SBICMD_TAB_STATZ:
                case SBICMD_TAB_GRUNTZ:
                case SBICMD_TAB_RESOURCE:
                case SBICMD_TAB_MULTIPLAYER:
                case SBICMD_TAB_GAME:
                    HiCueFind();
                    SetButtonState(cmd, MENUITEM_SELECTED);
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
                    HideStatusBar();
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
                    if (g_gameReg->GetFrameGate() != false) {
                        g_gameReg->FinishLevel(g_gameReg->ToggleFrameGate(), true);
                    }
                    (static_cast<CPlay*>(g_gameReg->m_curState))->OpenLevelOverlay(true);
                    return 1;
                case SBICMD_GAME_TAB:
                    HiCueLookup();
                    SetGameTabContent(GAME_TAB_MENU, false);
                    return 1;
                case SBICMD_DESTRUCT:
                    if (g_gameReg->GetGameMode() != GAMEMODE_QUESTZ) {
                        break;
                    }
                    if (m_destructButtonLocked != false) {
                        break;
                    }
                    if (m_gameplayControlsDisabled != false) {
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
                                destructButtonImage->SetFrameIndex(1);
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
            if (m_gameplayControlsDisabled != false) {
                break;
            }
            if (g_gameReg->m_triggerMgr->m_playerControlEnabled == false) {
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
                    ToggleUnitSample(IDX(cmd) - IDX(SBICMD_STAT_TOGGLE_FIRST));
                    return 1;
                default:
                    return 0;
            }

        case TAB_MULTIPLAYER:
            if (m_gameplayControlsDisabled != false) {
                break;
            }
            if (g_gameReg->m_triggerMgr->m_playerControlEnabled == false) {
                break;
            }
            if (cmd < SBICMD_MULTIPLAYER_HEAD_FIRST || cmd > SBICMD_MULTIPLAYER_HEAD_LAST) {
                return 0;
            }
            HiCueLookup();
            m_multiplayerPlayerIndex = IDX(cmd) - IDX(SBICMD_MULTIPLAYER_HEAD_FIRST);
            ResetWidgets(false);
            TryActivate();
            RequestRedraw();
            return 1;

        case TAB_GRUNTZ:
            if (m_gameplayControlsDisabled != false) {
                break;
            }
            if (g_gameReg->m_triggerMgr->m_playerControlEnabled == false) {
                break;
            }
            if (cmd < SBICMD_GRUNT_SLOT_FIRST || cmd > SBICMD_GRUNT_SLOT_LAST) {
                return 0;
            }
            SelectGruntOvenForPlacement(IDX(cmd) - IDX(SBICMD_GRUNT_SLOT_FIRST));
            return 1;

        case TAB_RESOURCE:
            if (m_gameplayControlsDisabled != false) {
                break;
            }
            if (g_gameReg->m_triggerMgr->m_playerControlEnabled == false) {
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
                    if (g_gameReg->m_triggerMgr->m_finishState == FINISH_STATE_VICTORY) {
                        HiCueLookup();
                        g_gameReg->FinalizeLevelAndShowResults();
                    } else if (g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
                        HiCueLookup();
                        HiPost(0x806b);
                    } else {
                        HiCueLookup();
                        (static_cast<CPlay*>(g_gameReg->m_curState))->CloseLevelOverlay(0);
                    }
                    break;
                case SBICMD_DIALOG_SECONDARY:
                    if (g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
                        if (g_gameReg->m_triggerMgr->m_finishState == FINISH_STATE_VICTORY) {
                            g_gameReg->CommitSinglePlayerProgress();
                        }
                        HiCueLookup();
                        HiPost(0x8023);
                    } else {
                        HiCueTimed();
                        g_gameReg->FinalizeLevelAndShowResults();
                    }
                    break;
                case SBICMD_DIALOG_YES:
                    if (g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
                        if (g_gameReg->m_triggerMgr->m_finishState == FINISH_STATE_VICTORY) {
                            g_gameReg->CommitSinglePlayerProgress();
                        }
                        HiCueTimed();
                        HiPost(0x8023);
                    } else {
                        HiCueTimed();
                        g_gameReg->FinalizeLevelAndShowResults();
                    }
                    break;
                case SBICMD_DIALOG_NO:
                    HiCueTimed();
                    (static_cast<CPlay*>(g_gameReg->m_curState))->CloseLevelOverlay(0);
                    break;
                default:
                    return 0;
            }
            break;

        default:
            return 0;
    }
    return 1;
}

RVA(0x000ff850, 0x121)
i32 CStatusBarMgr::HandleDoubleClick(i32 keyFlags, i32 x, i32 y) {
    CStatusBarItem* r = HitTestItems(x, y);
    if (r == NULL) {
        return 1;
    }
    r->OnDoubleClick(keyFlags, x, y);
    SbiCommandId cmd = r->GetCommandId();
    switch (r->GetTab()) {
        case TAB_STATZ:
            if (m_gameplayControlsDisabled == false
                && g_gameReg->GetTriggerMgr()->IsPlayerControlEnabled() != false
                && cmd >= SBICMD_CURSOR_TARGET_FIRST && cmd <= SBICMD_CURSOR_TARGET_LAST) {
                HiCueTimed();
                PlaceCursorTarget(IDX(cmd) - IDX(SBICMD_CURSOR_TARGET_FIRST), 1);
                return 1;
            }
            break;
    }

    return HandleClick(keyFlags, x, y);
}

RVA(0x000ff9d0, 0x8)
i32 CStatusBarMgr::OnPointerRelease(i32, i32, i32) {
    return 1;
}

RVA(0x000ff9f0, 0xe4)
i32 CStatusBarMgr::HandlePointerDrag(i32 keyFlags, i32 x, i32 y) {
    CStatusBarItem* r = HitTestItems(x, y);
    if (r == NULL) {
        ClearButtonHighlights(TAB_ALL);
        return 1;
    }
    r->OnPointerDrag(keyFlags, x, y);
    if (r->GetKind() != SBI_KIND_MENU_ITEM) {
        ClearButtonHighlights(TAB_ALL);
        return 1;
    }
    SbiCommandId cmd = r->GetCommandId();
    if (m_gameplayControlsDisabled == false) {
        if (cmd >= SBICMD_TAB_FIRST && cmd <= SBICMD_TAB_LAST) {
            SetButtonState(cmd, MENUITEM_HIGHLIGHT);
        } else {
            ClearButtonHighlights(TAB_CONTROLS);
        }
    }
    if (m_activeTab == TAB_GAME) {
        if (r->GetTab() == TAB_GAME) {
            SetButtonState(cmd, MENUITEM_HIGHLIGHT);
        } else {
            ClearButtonHighlights(TAB_GAME);
        }
    }
    if (m_levelOverlayActive) {
        if (r->GetTab() == TAB_DIALOG) {
            SetButtonState(cmd, MENUITEM_HIGHLIGHT);
            return 1;
        }
        ClearButtonHighlights(TAB_GAME);
    }
    return 1;
}

// @early-stop
RVA(0x000ffb20, 0x13a)
i32 CStatusBarMgr::UpdateStatusBar(i32 deltaMs) {
    if (g_gameReg->IsSoundEnabled() != false) {
        if (m_destructWarningState != DESTRUCT_WARNING_INACTIVE
            && m_destructButtonLocked == false) {
            if (m_destructWarningSound == NULL) {

                SoundCueRegistry* registry = g_gameReg->World()->SoundRegistry();
                SoundCue* found = registry->FindCue("GAME_DESTRUCT");
                if (found) {
                    SoundSample* sample = found->GetSound();
                    if (sample) {
                        SoundBuffer* voice = sample->AcquireInstance();
                        m_destructWarningSound = voice;
                        if (voice) {
                            voice->ApplyAndPlay(g_gameReg->GetSoundVolume(), 0, 0, true);
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
    if (m_warpStoneFly) {
        m_warpStoneFly->Tick(deltaMs);
        RequestRedraw();
    }
    return 1;
}

RVA(0x000ffcb0, 0xe2)
CStatusBarItem* CStatusBarMgr::HitTestItems(i32 x, i32 y) {
    POSITION n = m_tabLists[0].GetHeadPosition();
    while (n) {
        CStatusBarItem* r = static_cast<CStatusBarItem*>(m_tabLists[0].GetNext(n));
        if (r) {
            b32 hit = r->IsEnabled();
            if (hit) {
                hit = r->ContainsPoint(x, y);
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
            b32 hit = r->IsEnabled();
            if (hit) {
                hit = r->ContainsPoint(x, y);
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
            b32 hit = r->IsEnabled();
            if (hit) {
                hit = r->ContainsPoint(x, y);
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
            CRect(bx + 0x7c, by + 0xad, bx + 0x88, by + 0xb9),
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
            CRect(bx + 0x8a, by + 0xad, bx + 0x96, by + 0xb9),
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
            CRect(bx + 0x83, by + 0xbb, bx + 0x8f, by + 0xc7),
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
        CRect(bx + 0x42, by + 0x82, bx + 0x62, by + 0xad),
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
        CRect(bx + 0x04, by + 0x82, bx + 0x24, by + 0xad),
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
        CRect(bx + 0x24, by + 0x82, bx + 0x44, by + 0xad),
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
        CRect(bx + 0x60, by + 0x82, bx + 0x80, by + 0xad),
        "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB",
        -1,
        0
    );
    AddTabItem(0, multiTab);
    m_multiTabButton = multiTab;
    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        multiTab->m_state = MENUITEM_DISABLED;
        CImageSet* f = multiTab->m_stateFrames;
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
        CRect(bx + 0x7e, by + 0x82, bx + 0x9e, by + 0xad),
        "GAME_STATUSBAR_TABZ_GAMETAB",
        -1,
        0
    );
    AddTabItem(0, gameTab);
    m_gameTabButton = gameTab;

    if (BuildSideTabs() == 0) {
        return 0;
    }
    if (BuildActiveTabContent() == 0) {
        return 0;
    }
    if (BuildLevelOverlay() == 0) {
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
i32 CStatusBarItem::OnClick(i32, i32, i32) {
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
void CStatusBarMgr::ResetWidgets(b32 deleteCollapsedSprite) {
    for (i32 t = 0; t < 8; t++) {
        DELETE_STATUS_ITEMS(m_tabLists[t])
    }
    if (deleteCollapsedSprite) {
        if (m_collapsedSprite) {

            m_collapsedSprite->Hide();
            m_collapsedSprite->AddFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
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
    m_collapsedSprite = NULL;
    i32 i;
    memset(m_unitSideTabs, 0, sizeof(m_unitSideTabs));
    memset(m_unitSampleArrows, 0, sizeof(m_unitSampleArrows));
    memset(m_gruntOvenImages, 0, sizeof(m_gruntOvenImages));
    memset(m_conveyorSprites, 0, sizeof(m_conveyorSprites));
    memset(m_resourceSlotSprites, 0, sizeof(m_resourceSlotSprites));
    memset(m_multiplayerHeadButtons, 0, sizeof(m_multiplayerHeadButtons));
    m_deliveryItemDisplay = NULL;
    m_grinderItemDisplay = NULL;
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
void CStatusBarMgr::ClearActiveTabContent() {
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

            memset(m_unitSampleArrows, 0, sizeof(m_unitSampleArrows));
            break;
        case TAB_MULTIPLAYER:
            memset(m_multiplayerHeadButtons, 0, sizeof(m_multiplayerHeadButtons));
            break;
        case TAB_GRUNTZ: {

            memset(m_gruntOvenImages, 0, sizeof(m_gruntOvenImages));
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
            m_deliveryItemDisplay = NULL;
            m_grinderItemDisplay = NULL;
            break;
        }
    }
}

RVA(0x00100cb0, 0x8b)
i32 CStatusBarMgr::RequestRedraw() {
    if (m_position == STATUSBAR_HIDDEN) {

        i32 w = g_gameReg->m_modeSize.cx;
        i32 h = g_gameReg->m_modeSize.cy;
        m_collapsedSpriteX = w - 0x45;
        m_collapsedSpriteY = h - 0x30;
        SetCollapsedSpritePosition(w - 0x45, h - 0x30);
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

    ClearButtonHighlights(TAB_ALL);
    m_redrawFrames = 2;
    return 1;
}

RVA(0x00100d70, 0x548)
i32 CStatusBarMgr::SetButtonState(SbiCommandId cmd, SbiMenuItemState state) {
    if (m_statzTabButton == NULL || m_resourceTabButton == NULL || m_gruntzTabButton == NULL
        || m_multiTabButton == NULL || m_gameTabButton == NULL) {
        return 0;
    }
    switch (cmd) {
        case SBICMD_TAB_STATZ:
            if (m_layoutLocked) {
                return 1;
            }
            m_statzTabButton->SetState(state, 1);
            m_gruntzTabButton->ClearMatchingState(state);
            m_resourceTabButton->ClearMatchingState(state);
            m_multiTabButton->ClearMatchingState(state);
            m_gameTabButton->ClearMatchingState(state);
            break;
        case SBICMD_TAB_GRUNTZ:
            if (m_layoutLocked) {
                return 1;
            }
            m_statzTabButton->ClearMatchingState(state);
            m_gruntzTabButton->SetState(state, 1);
            m_resourceTabButton->ClearMatchingState(state);
            m_multiTabButton->ClearMatchingState(state);
            m_gameTabButton->ClearMatchingState(state);
            break;
        case SBICMD_TAB_RESOURCE:
            if (m_layoutLocked) {
                return 1;
            }
            m_statzTabButton->ClearMatchingState(state);
            m_gruntzTabButton->ClearMatchingState(state);
            m_resourceTabButton->SetState(state, 1);
            m_multiTabButton->ClearMatchingState(state);
            m_gameTabButton->ClearMatchingState(state);
            break;
        case SBICMD_TAB_MULTIPLAYER:
            if (m_layoutLocked) {
                return 1;
            }
            m_statzTabButton->ClearMatchingState(state);
            m_gruntzTabButton->ClearMatchingState(state);
            m_resourceTabButton->ClearMatchingState(state);
            m_multiTabButton->SetState(state, 1);
            m_gameTabButton->ClearMatchingState(state);
            break;
        case SBICMD_TAB_GAME:
            if (m_layoutLocked) {
                return 1;
            }
            m_statzTabButton->ClearMatchingState(state);
            m_gruntzTabButton->ClearMatchingState(state);
            m_resourceTabButton->ClearMatchingState(state);
            m_multiTabButton->ClearMatchingState(state);
            m_gameTabButton->SetState(state, 1);
            break;
        case SBICMD_PAUSE:
            if (m_layoutLocked) {
                return 1;
            }
            m_gameResumePauseButton->SetState(state, 1);
            m_gameLoadButton->ClearMatchingState(state);
            m_gameSaveButton->ClearMatchingState(state);
            m_gameSettingsButton->ClearMatchingState(state);
            m_gameHelpButton->ClearMatchingState(state);
            m_gameQuitButton->ClearMatchingState(state);
            break;
        case SBICMD_LOAD_GAME:
            if (m_layoutLocked) {
                return 1;
            }
            m_gameResumePauseButton->ClearMatchingState(state);
            m_gameLoadButton->SetState(state, 1);
            m_gameSaveButton->ClearMatchingState(state);
            m_gameSettingsButton->ClearMatchingState(state);
            m_gameHelpButton->ClearMatchingState(state);
            m_gameQuitButton->ClearMatchingState(state);
            break;
        case SBICMD_SAVE_GAME:
            if (m_layoutLocked) {
                return 1;
            }
            m_gameResumePauseButton->ClearMatchingState(state);
            m_gameLoadButton->ClearMatchingState(state);
            m_gameSaveButton->SetState(state, 1);
            m_gameSettingsButton->ClearMatchingState(state);
            m_gameHelpButton->ClearMatchingState(state);
            m_gameQuitButton->ClearMatchingState(state);
            break;
        case SBICMD_SETTINGS:
            if (m_layoutLocked) {
                return 1;
            }
            m_gameResumePauseButton->ClearMatchingState(state);
            m_gameLoadButton->ClearMatchingState(state);
            m_gameSaveButton->ClearMatchingState(state);
            m_gameSettingsButton->SetState(state, 1);
            m_gameHelpButton->ClearMatchingState(state);
            m_gameQuitButton->ClearMatchingState(state);
            break;
        case SBICMD_BOOTY_STATE:
            if (m_layoutLocked) {
                return 1;
            }
            m_gameResumePauseButton->ClearMatchingState(state);
            m_gameLoadButton->ClearMatchingState(state);
            m_gameSaveButton->ClearMatchingState(state);
            m_gameSettingsButton->ClearMatchingState(state);
            m_gameHelpButton->SetState(state, 1);
            m_gameQuitButton->ClearMatchingState(state);
            break;
        case SBICMD_QUIT:
            if (m_layoutLocked) {
                return 1;
            }
            m_gameResumePauseButton->ClearMatchingState(state);
            m_gameLoadButton->ClearMatchingState(state);
            m_gameSaveButton->ClearMatchingState(state);
            m_gameSettingsButton->ClearMatchingState(state);
            m_gameHelpButton->ClearMatchingState(state);
            m_gameQuitButton->SetState(state, 1);
            break;
        case SBICMD_GAME_TAB:
            if (m_layoutLocked) {
                return 1;
            }
            m_gameQuitButton->SetState(state, 1);
            break;
        case SBICMD_DIALOG_PRIMARY:
            if (m_endPrimaryButton) {
                m_endPrimaryButton->SetState(state, 1);
            }
            m_endSecondaryButton->ClearMatchingState(state);
            break;
        case SBICMD_DIALOG_SECONDARY:
            if (m_endPrimaryButton) {
                m_endPrimaryButton->ClearMatchingState(state);
            }
            m_endSecondaryButton->SetState(state, 1);
            break;
        case SBICMD_DIALOG_YES:
            m_confirmYesButton->SetState(state, 1);
            m_confirmNoButton->ClearMatchingState(state);
            break;
        case SBICMD_DIALOG_NO:
            m_confirmYesButton->ClearMatchingState(state);
            m_confirmNoButton->SetState(state, 1);
            break;
    }
    return 1;
}

RVA(0x00101420, 0x110)
i32 CStatusBarMgr::ClearButtonHighlights(StatusBarTab idx) {
    if (idx == TAB_ALL || idx == TAB_CONTROLS) {
        if (m_statzTabButton) {
            m_statzTabButton->ClearHighlight();
        }
        if (m_gruntzTabButton) {
            m_gruntzTabButton->ClearHighlight();
        }
        if (m_resourceTabButton) {
            m_resourceTabButton->ClearHighlight();
        }
        if (m_multiTabButton) {
            m_multiTabButton->ClearHighlight();
        }
        if (m_gameTabButton) {
            m_gameTabButton->ClearHighlight();
        }
    }
    if (idx == TAB_GAME || idx == TAB_ALL) {
        if (m_gameResumePauseButton) {
            m_gameResumePauseButton->ClearHighlight();
        }
        if (m_gameLoadButton) {
            m_gameLoadButton->ClearHighlight();
        }
        if (m_gameSaveButton) {
            m_gameSaveButton->ClearHighlight();
        }
        if (m_gameSettingsButton) {
            m_gameSettingsButton->ClearHighlight();
        }
        if (m_gameHelpButton) {
            m_gameHelpButton->ClearHighlight();
        }
        if (m_gameQuitButton) {
            m_gameQuitButton->ClearHighlight();
        }
    }
    if (idx == TAB_DIALOG || idx == TAB_ALL) {
        if (m_endPrimaryButton) {
            m_endPrimaryButton->ClearHighlight();
        }
        if (m_endSecondaryButton) {
            m_endSecondaryButton->ClearHighlight();
        }
        if (m_confirmYesButton) {
            m_confirmYesButton->ClearHighlight();
        }
        if (m_confirmNoButton) {
            m_confirmNoButton->ClearHighlight();
        }
    }
    return 1;
}

RVA(0x00101580, 0x806)
i32 CStatusBarMgr::BuildGameTabContent() {
    CDDrawSurfaceMgr* code = m_world;
    i32 bx = m_barRect.left;
    i32 by = m_barRect.top;

    switch (m_gameTabContent) {
        case GAME_TAB_MISSION_STATUS: {
            CSBI_ImageSet* status;
            if (g_gameReg->GetTriggerMgr()->GetFinishState() == FINISH_STATE_VICTORY) {
                NEW_STATUS_BAR_ITEM(
                    status,
                    CSBI_ImageSet,
                    code,
                    SBICMD_MISSION_STATUS,
                    TAB_GAME,
                    CRect(bx, by + 0xd7, bx + 0x9f, by + 0x118),
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
                    CRect(bx, by + 0xd7, bx + 0x9f, by + 0x118),
                    "GAME_STATUSBAR_TABZ_GAMETAB_MISSIONSTATUS",
                    2,
                    0
                );
                AddTabItem(5, status);
            }
            break;
        }
        default: {
            if (m_gameplayControlsDisabled != false && g_gameReg->GetFrameGate() != false) {
                CSBI_MenuItem* resume;
                NEW_STATUS_BAR_ITEM(
                    resume,
                    CSBI_MenuItem,
                    code,
                    SBICMD_PAUSE,
                    TAB_GAME,
                    CRect(bx, by + 0xd5, bx + 0x9f, by + 0xec),
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
                    CRect(bx, by + 0xd5, bx + 0x9f, by + 0xec),
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
                CRect(bx, by + 0x125, bx + 0x9f, by + 0x13c),
                "GAME_STATUSBAR_TABZ_GAMETAB_LOAD",
                -1,
                0
            );
            AddTabItem(5, load);
            m_gameLoadButton = load;
            if (g_gameReg->GetGameMode() == GAMEMODE_MULTIPLAYER) {
                load->SetEnabled(0);
            }

            CSBI_MenuItem* save;
            NEW_STATUS_BAR_ITEM(
                save,
                CSBI_MenuItem,
                code,
                SBICMD_SAVE_GAME,
                TAB_GAME,
                CRect(bx, by + 0xfd, bx + 0x9f, by + 0x114),
                "GAME_STATUSBAR_TABZ_GAMETAB_SAVE",
                -1,
                0
            );
            AddTabItem(5, save);
            m_gameSaveButton = save;
            if (g_gameReg->GetGameMode() == GAMEMODE_MULTIPLAYER) {
                save->SetEnabled(0);
            }

            CSBI_MenuItem* settings;
            NEW_STATUS_BAR_ITEM(
                settings,
                CSBI_MenuItem,
                code,
                SBICMD_SETTINGS,
                TAB_GAME,
                CRect(bx, by + 0x14d, bx + 0x9f, by + 0x164),
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
                CRect(bx, by + 0x175, bx + 0x9f, by + 0x18c),
                "GAME_STATUSBAR_TABZ_GAMETAB_HELP",
                -1,
                0
            );
            AddTabItem(5, help);
            m_gameHelpButton = help;
            if (g_gameReg->GetGameMode() == GAMEMODE_MULTIPLAYER) {
                help->SetEnabled(0);
            }

            CSBI_MenuItem* quit;
            NEW_STATUS_BAR_ITEM(
                quit,
                CSBI_MenuItem,
                code,
                SBICMD_QUIT,
                TAB_GAME,
                CRect(bx, by + 0x19d, bx + 0x9f, by + 0x1b4),
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
                CRect(bx + 0x22, by + 0x1be, bx + 0x7d, by + 0x1d6),
                "GAME_STATUSBAR_TABZ_GAMETAB_DESTRUCT",
                IDX(m_destructButtonFrame),
                0
            );
            AddTabItem(5, destruct);
            m_destructButtonImage = destruct;
            if (g_gameReg->GetGameMode() != GAMEMODE_QUESTZ) {
                destruct->SetEnabled(0);
                m_destructButtonFrame = DESTRUCT_FRAME_DISABLED;
                m_destructWarningState = DESTRUCT_WARNING_INACTIVE;
                m_destructButtonImage->SetFrameIndex(IDX(DESTRUCT_FRAME_DISABLED));
            }
            break;
        }
    }
    return 1;
}

RVA_COMPGEN(0x00101fd0, 0x1e, ??_GCSBI_ImageSet@@UAEPAXI@Z)

RVA_COMPGEN(0x00102000, 0x7f, ??1CSBI_ImageSet@@UAE@XZ)

RVA(0x001020a0, 0xae)
i32 CStatusBarMgr::SetGameTabContent(GameTabContent content, b32 forceReload) {
    if (content == m_gameTabContent && forceReload == false) {
        return 1;
    }
    DELETE_STATUS_ITEMS(m_tabLists[5])
    m_gameResumePauseButton = NULL;
    m_gameLoadButton = NULL;
    m_gameSaveButton = NULL;
    m_gameSettingsButton = NULL;
    m_gameHelpButton = NULL;
    m_gameQuitButton = NULL;
    m_gameTabContent = content;

    if (!BuildActiveTabContent()) {
        g_gameReg->ReportError(s_activateErrId, s_setTabErrTag);
        return 0;
    }
    RequestRedraw();
    return 1;
}

RVA(0x00102180, 0x5f)
void CStatusBarMgr::BuildGameTabResumeButton(b32 show) {
    if (m_position == STATUSBAR_HIDDEN) {
        RestoreStatusBar();
    }
    if (show && m_activeTab != TAB_GAME) {
        SetButtonState(SBICMD_TAB_GAME, MENUITEM_SELECTED);
    }
    if (m_gameResumePauseButton) {
        m_gameResumePauseButton->ResolveFrame("GAME_STATUSBAR_TABZ_GAMETAB_RESUME", 1);
        RequestRedraw();
        m_gameResumePauseButton->RequestRedraw();
    }
    m_gameplayControlsDisabled = true;
}

RVA(0x00102200, 0x37)
void CStatusBarMgr::BuildGameTabPauseButton() {
    if (m_gameResumePauseButton) {
        m_gameResumePauseButton->ResolveFrame("GAME_STATUSBAR_TABZ_GAMETAB_PAUSE", 1);
        RequestRedraw();
        m_gameResumePauseButton->RequestRedraw();
    }
    m_gameplayControlsDisabled = false;
}

RVA(0x00102250, 0x1de4)
i32 CStatusBarMgr::BuildActiveTabContent() {
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
                CRect(bx + 0x18, by + 0xaf, bx + 0x70, by + 0xbe),
                "GAME_STATUSBAR_TABZ_GRUNTZTAB_TITLETEXT",
                -1,
                0
            );
            AddTabItem(2, it);

            {
                CSBI_ImageSet** aptr = m_gruntOvenImages;
                GruntOvenSlot* slot = m_gruntOvenSlots;
                i32 y = by + 0xfe;
                for (i = 0; i < 5; i++) {
                    CSBI_ImageSet* set;
                    NEW_STATUS_BAR_ITEM(
                        set,
                        CSBI_ImageSet,
                        code,
                        static_cast<SbiCommandId>(IDX(SBICMD_GRUNT_SLOT_FIRST) + i),
                        TAB_GRUNTZ,
                        CRect(bx + 0xe, y - 0x32, bx + 0x39, y),
                        "GAME_STATUSBAR_TABZ_GRUNTZTAB_GRUNTOVEN",
                        slot->m_frameIndex,
                        0
                    );
                    AddTabItem(2, set);
                    *aptr = set;
                    CShadeTable* sel = g_gameReg->GruntPalettes()->GetShadeTable(
                        IDX(g_gameReg->m_players[g_curPlayer].GetColor()),
                        0
                    );
                    if (sel == NULL) {
                        sel = g_gameReg->GruntPalettes()->GetShadeTable(1, 0);
                    }
                    set->GetFrameSet()->SetAllShadeModes(SHADE_PAL_16);
                    set->GetFrameSet()->SetAllShadeTables(sel);
                    aptr++;
                    slot++;
                    y += 0x36;
                }
            }
            NEW_STATUS_BAR_ITEM(
                it,
                CSBI_Image,
                code,
                SBICMD_GRUNT_WELL,
                TAB_GRUNTZ,
                CRect(bx + 0x4c, by + 0xc8, bx + 0x97, by + 0x1cd),
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
                CRect(bx + 0x1e, by + 0xc4, bx + 0x3d, by + 0xcd),
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
                CRect(bx + 0x68, by + 0x1cf, bx + 0x87, by + 0x1d8),
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
                    CRect(bx + 0x6e, by + 0xf8, bx + 0x81, by + 0x1b3),
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
                CRect(bx + 0x18, by + 0xaf, bx + 0x70, by + 0xbe),
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
                CRect(bx, by + 0x135, bx + 0x9f, by + 0x1be),
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
                CRect(bx, by + 0xfb, bx + 0x9f, by + 0x134),
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
                CRect(bx + 0x48, by + 0xd3, bx + 0x67, by + 0xf3),
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
                CRect(bx + 0x19, by + 0x11c, bx + 0x3c, by + 0x130),
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
                CRect(bx + 0x40, by + 0x11c, bx + 0x63, by + 0x130),
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
                CRect(bx + 0x68, by + 0x11c, bx + 0x8b, by + 0x130),
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
                CRect(
                    m_deliveryItemRect.left + bx,
                    m_deliveryItemRect.top + by,
                    m_deliveryItemRect.right + bx,
                    m_deliveryItemRect.bottom + by
                ),
                "GAME_INGAMEICONZ_GREYCHIPZ",
                m_deliveryPickupType,
                0
            );
            AddTabItem(3, imgSet);
            m_deliveryItemDisplay = imgSet;
            imgSet->SetEnabled(0);

            {
                i32* cfgp = &m_resourceSlots[4].m_value;
                CSBI_ImageSet** cachep = &m_resourceSlotSprites[4];
                i32 y = by + 0x155;
                for (i = 0; i < 4; i++) {
                    CSBI_ImageSet* toolIcon;
                    NEW_STATUS_BAR_ITEM(
                        toolIcon,
                        CSBI_ImageSet,
                        code,
                        static_cast<SbiCommandId>(IDX(SBICMD_TOOL_RESOURCE_FIRST) + i),
                        TAB_RESOURCE,
                        CRect(bx + 0x1d, y - 0x17, bx + 0x34, y),
                        "GAME_INGAMEICONZ_NORMCHIPZ",
                        cfgp[-24],
                        0
                    );
                    AddTabItem(3, toolIcon);
                    cachep[-4] = toolIcon;
                    CSBI_ImageSet* toyIcon;
                    NEW_STATUS_BAR_ITEM(
                        toyIcon,
                        CSBI_ImageSet,
                        code,
                        static_cast<SbiCommandId>(IDX(SBICMD_TOY_RESOURCE_FIRST) + i),
                        TAB_RESOURCE,
                        CRect(bx + 0x45, y - 0x17, bx + 0x5c, y),
                        "GAME_INGAMEICONZ_NORMCHIPZ",
                        cfgp[0],
                        0
                    );
                    AddTabItem(3, toyIcon);
                    cachep[0] = toyIcon;
                    CSBI_ImageSet* brickIcon;
                    NEW_STATUS_BAR_ITEM(
                        brickIcon,
                        CSBI_ImageSet,
                        code,
                        static_cast<SbiCommandId>(IDX(SBICMD_BRICK_RESOURCE_FIRST) + i),
                        TAB_RESOURCE,
                        CRect(bx + 0x6d, y - 0x17, bx + 0x84, y),
                        "GAME_INGAMEICONZ_NORMCHIPZ",
                        cfgp[24],
                        0
                    );
                    AddTabItem(3, brickIcon);
                    cachep[4] = brickIcon;
                    cfgp += 6;
                    cachep += 1;
                    y += 0x20;
                }
            }

            mach = new CSBI_GruntMachine;
            if (!mach->Initialize(
                    this,
                    code,
                    SBICMD_RESOURCE_MACHINE_BACKGROUND,
                    TAB_RESOURCE,
                    CRect(bx, by + 0xc8, bx + 0x9f, by + 0xfa),
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
                CRect(bx, by + 0x135, bx + 0x9f, by + 0x1df),
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
                    CRect(bx, by + 0x1bf, bx + 0x9f, by + 0x1cc),
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
                CRect(
                    m_grinderItemRect.left + bx,
                    m_grinderItemRect.top + by,
                    m_grinderItemRect.right + bx,
                    m_grinderItemRect.bottom + by
                ),
                "GAME_INGAMEICONZ_NORMCHIPZ",
                m_grinderPickupType,
                0
            );
            AddTabItem(3, imgSet);
            m_grinderItemDisplay = imgSet;
            imgSet->SetEnabled(0);

            ani = new CSBI_ImageSetAni;
            if (!ani->Init(
                    this,
                    code,
                    SBICMD_CONVEYOR_BOTTOM,
                    TAB_RESOURCE,
                    CRect(bx, by + 0x1c7, bx + 0x9f, by + 0x1df),
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
                CRect(bx + 0x18, by + 0xaf, bx + 0x70, by + 0xbe),
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
                CRect(bx + 0x53, by + 0xcf, bx + 0x8e, by + 0x10a),
                "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD1",
                1,
                0
            );
            m_multiplayerHeadButtons[0] = head;
            AddTabItem(4, head);
            NEW_STATUS_BAR_ITEM(
                head,
                CSBI_WarlordHead,
                code,
                SBICMD_MULTIPLAYER_HEAD2,
                TAB_MULTIPLAYER,
                CRect(bx + 0x53, by + 0x112, bx + 0x8e, by + 0x14d),
                "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD2",
                1,
                0
            );
            m_multiplayerHeadButtons[1] = head;
            AddTabItem(4, head);
            NEW_STATUS_BAR_ITEM(
                head,
                CSBI_WarlordHead,
                code,
                SBICMD_MULTIPLAYER_HEAD3,
                TAB_MULTIPLAYER,
                CRect(bx + 0x53, by + 0x155, bx + 0x8e, by + 0x190),
                "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD3",
                1,
                0
            );
            m_multiplayerHeadButtons[2] = head;
            AddTabItem(4, head);
            NEW_STATUS_BAR_ITEM(
                head,
                CSBI_WarlordHead,
                code,
                SBICMD_MULTIPLAYER_HEAD4,
                TAB_MULTIPLAYER,
                CRect(bx + 0x53, by + 0x197, bx + 0x8e, by + 0x1d2),
                "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_HEAD4",
                1,
                0
            );
            m_multiplayerHeadButtons[3] = head;
            AddTabItem(4, head);

            {
                CSBI_WarlordHead** slot = m_multiplayerHeadButtons;
                i32 pi = 0;
                do {
                    GruntzPlayer* p = &g_gameReg->m_players[pi];
                    CShadeTable* sel;
                    if (p->HasJoinedRound() != false && p->HasDropped() == false) {
                        sel = g_gameReg->GruntPalettes()->GetShadeTable(IDX(p->GetColor()), 0);
                        if (pi == m_multiplayerPlayerIndex) {
                            (*slot)->SetDisplayState(1);
                        }
                    } else {
                        sel = g_gameReg->GruntPalettes()->GetShadeTable(1, 0);
                        (*slot)->SetDisplayState(2);
                    }

                    (*slot)->SetHeadShading(SHADE_PAL_16, sel);
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
                    if (!bar->Initialize(
                            this,
                            code,
                            static_cast<SbiCommandId>(IDX(SBICMD_CURSOR_TARGET_FIRST) + i),
                            TAB_MULTIPLAYER,
                            CRect(gruntBarLeft, y - 0x11, gruntBarRight, y),
                            "GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ",
                            m_multiplayerPlayerIndex,
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
                CRect(bx + 0x18, by + 0xaf, bx + 0x70, by + 0xbe),
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
                            CRect(arrowL, y - 0x11, arrowR, y),
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
                    m_unitSampleArrows[i] = arrow;
                    AddTabItem(1, arrow);
                    if (m_unitSampleModes[i] != STATUS_SAMPLE_NONE) {
                        arrow->SetSampledDirection(m_position, false);
                    } else {
                        arrow->SetUnsampledDirection(m_position, false);
                    }
                    bar = new CSBI_StatzTabGruntBar;
                    if (!bar->Initialize(
                            this,
                            code,
                            id,
                            TAB_STATZ,
                            CRect(bx + 0x28, y - 0x11, bx + 0x77, y),
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
                CRect(bx + 0x18, by + 0xaf, bx + 0x70, by + 0xbe),
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
                CRect(bx, by, bx + 0x9f, by + 0x7f),
                "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                1,
                0
            );
            AddTabItem(5, it);
            if ((static_cast<CTriggerMgr*>(g_gameReg->GetTriggerMgr()))
                    ->HasWarpStoneFragment(WARPSTONE_FRAGMENT_FIRST)) {
                NEW_STATUS_BAR_ITEM(
                    it,
                    CSBI_ImageSet,
                    code,
                    SBICMD_WARPSTONE_FRAGMENT1,
                    TAB_GAME,
                    CRect(bx + 0x17, by + 0xe, bx + 0x52, by + 0x44),
                    "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                    2,
                    0
                );
                AddTabItem(5, it);
                if ((static_cast<CTriggerMgr*>(g_gameReg->GetTriggerMgr()))
                        ->HasWarpStoneFragment(WARPSTONE_FRAGMENT_SECOND)) {
                    NEW_STATUS_BAR_ITEM(
                        it,
                        CSBI_ImageSet,
                        code,
                        SBICMD_WARPSTONE_FRAGMENT2,
                        TAB_GAME,
                        CRect(bx + 0x4c, by + 0xf, bx + 0x87, by + 0x3e),
                        "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                        3,
                        0
                    );
                    AddTabItem(5, it);
                    if ((static_cast<CTriggerMgr*>(g_gameReg->GetTriggerMgr()))
                            ->HasWarpStoneFragment(WARPSTONE_FRAGMENT_THIRD)) {
                        NEW_STATUS_BAR_ITEM(
                            it,
                            CSBI_ImageSet,
                            code,
                            SBICMD_WARPSTONE_FRAGMENT3,
                            TAB_GAME,
                            CRect(bx + 0x1b, by + 0x3b, bx + 0x52, by + 0x71),
                            "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                            4,
                            0
                        );
                        AddTabItem(5, it);
                        if ((static_cast<CTriggerMgr*>(g_gameReg->GetTriggerMgr()))
                                ->HasWarpStoneFragment(WARPSTONE_FRAGMENT_FOURTH)) {
                            NEW_STATUS_BAR_ITEM(
                                it,
                                CSBI_ImageSet,
                                code,
                                SBICMD_WARPSTONE_FRAGMENT4,
                                TAB_GAME,
                                CRect(bx + 0x4a, by + 0x35, bx + 0x89, by + 0x74),
                                "GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE",
                                5,
                                0
                            );
                            AddTabItem(5, it);
                        }
                    }
                }
            }
            BuildGameTabContent();
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
        return CreateCollapsedSprite();
    }
    if (!BuildStatusBarTabs()) {
        g_gameReg->ReportError(s_activateErrId, s_activateErrTag);
        return 0;
    }
    SetButtonState(static_cast<SbiCommandId>(IDX(m_activeTab)), MENUITEM_SELECTED);
    return 1;
}

RVA(0x00104dd0, 0x6b)
i32 CStatusBarMgr::CreateCollapsedSprite() {
    if (m_collapsedSprite != NULL) {
        return 0;
    }
    i32 w = g_gameReg->m_modeSize.cx;
    i32 d = g_gameReg->m_modeSize.cy;
    CLAMP_UPPER_INPLACE(m_collapsedSpriteX, w - 0x22);
    if (m_collapsedSpriteY > d - 9) {
        m_collapsedSpriteY = d - 0x22;
    }
    m_collapsedSprite = (m_world)->ChildGroup()->CreateSprite(
        0,
        m_collapsedSpriteX,
        m_collapsedSpriteY,
        SORTKEY_OVERLAY,
        "StatusBarSprite",
        IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION)
    );
    return m_collapsedSprite != NULL;
}

RVA(0x00104e60, 0xed)
i32 CStatusBarMgr::SetUnitSampleMode(i32 unitIndex, StatusSampleMode sampleMode) {
    if (m_unitSampleModes[unitIndex] == sampleMode) {
        return 1;
    }

    if (g_gameReg->GetTriggerMgr()->UnitAt(g_curPlayer, unitIndex) == NULL) {
        return 0;
    }

    CSBI_SideTab* r = m_unitSideTabs[unitIndex];
    if (r != NULL) {
        r->m_sampleMode = sampleMode;
        r->SetEnabled(1);
        if (m_activeTab == TAB_STATZ) {

            m_unitSampleArrows[unitIndex]->SetSampledDirection(m_position, true);
            PlayRegistryCueIfElapsed(g_gameReg->World()->SoundRegistry(), "GAME_STATZTABTOGGLE");
        }
    }
    m_unitSampleModes[unitIndex] = sampleMode;
    return 1;
}

RVA(0x00104f90, 0xa8)
i32 CStatusBarMgr::ClearUnitSample(i32 unitIndex) {
    CSBI_SideTab* r = m_unitSideTabs[unitIndex];
    if (r != NULL) {
        r->m_sampleMode = STATUS_SAMPLE_NONE;
        r->SetEnabled(0);
        if (m_activeTab == TAB_STATZ) {

            m_unitSampleArrows[unitIndex]->SetUnsampledDirection(m_position, true);
            PlayRegistryCueIfElapsed(g_gameReg->World()->SoundRegistry(), "GAME_STATZTABTOGGLE");
        }
    }
    m_unitSampleModes[unitIndex] = STATUS_SAMPLE_NONE;
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

        b32 ok = newobj->Initialize(
            this,
            g_gameReg->World(),
            static_cast<SbiCommandId>(IDX(SBICMD_SIDE_TAB_FIRST) + i),
            TAB_CONTROLS,
            rc,
            "GAME_STATUSBAR_TABZ_STATZTAB_TABONLEFT",
            g_curPlayer,
            i,
            m_unitSampleModes[i],
            m_position == STATUSBAR_DOCK_RIGHT
        );
        if (ok == false) {
            delete newobj;
            return 0;
        }
        AddTabItem(0, newobj);
        m_unitSideTabs[i] = newobj;
        i++;
    }
    return 1;
}

RVA(0x00105280, 0x61)
i32 CStatusBarMgr::HitTestSideTabs(i32 x, i32 y) {
    if (m_gameplayControlsDisabled == false) {
        for (i32 i = 0; i < TM_UNITS_PER_PLAYER; i++) {
            if (m_unitSideTabs[i] && m_unitSideTabs[i]->IsEnabled()) {
                CSBI_SideTab* p = m_unitSideTabs[i];
                b32 hit = p->IsEnabled() ? p->ContainsPoint(x, y) : false;
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

    CSBI_ImageSet** slot = m_gruntOvenImages;
    GruntOvenSlot* tab = m_gruntOvenSlots;
    i32 n = 5;
    do {
        if (tab->m_state == GRUNT_OVEN_COOKING) {
            i64 d = static_cast<i64>(g_frameTime) - tab->m_cookingClock.GetStartTime();

            i32 elapsed = static_cast<i32>(max(0, d));
            u32 delay = g_buteMgr.GetDword("StatusBar", "GruntOvenDelay", 0xc8);
            i32 frame = static_cast<i32>((static_cast<u32>(elapsed) / delay)) + 1;
            if (frame >= 0x1a) {
                tab->m_state = GRUNT_OVEN_READY;
                frame = 0x1a;
                PlayRegistryCueIfElapsed(
                    g_gameReg->World()->SoundRegistry(),
                    "GAME_COOKINGCOMPLETE"
                );
            }
            if (frame != tab->m_frameIndex) {
                tab->m_frameIndex = frame;
                CSBI_ImageSet* w = *slot;
                if (w) {
                    w->SetFrameIndex(frame);
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
        if (StartAvailableGruntOven()) {
            changed = true;
            SetGruntWell(GRUNT_WELL_EMPTY);
        }
    }
    if (changed) {
        if (m_gruntWellGoo && m_gruntWellBackground) {
            m_gruntWellBackground->RequestRedraw();
            i32 fill = m_gruntWellLevel;
            CSBI_WellGoo* sink = m_gruntWellGoo;
            sink->m_fillPercent = fill;
            sink->RequestRedraw();
        }
    }
}

RVA(0x00105520, 0x21)
void CStatusBarMgr::ResetGruntOvens() {
    for (i32 i = 0; i < 5; i++) {
        EmptyGruntOven(i);
    }
    m_selectedGruntOvenSlot = -1;
}

RVA(0x00105560, 0x33)
void CStatusBarMgr::EmptyGruntOven(i32 idx) {
    m_gruntOvenSlots[idx].m_state = GRUNT_OVEN_EMPTY;
    m_gruntOvenSlots[idx].m_frameIndex = 1;
    if (m_gruntOvenImages[idx]) {
        m_gruntOvenImages[idx]->SetFrameIndex(1);
    }
}

RVA(0x001055b0, 0x109)
i32 CStatusBarMgr::StartGruntOven(i32 idx) {
    GruntOvenSlot* sp = &m_gruntOvenSlots[idx];
    if (sp->m_state != GRUNT_OVEN_EMPTY) {
        return 0;
    }
    if (g_gameReg->GetGameMode() == GAMEMODE_QUESTZ && m_layoutLocked == false) {
        if (m_position == STATUSBAR_HIDDEN) {
            RestoreStatusBar();
        }
        if (m_activeTab != TAB_GRUNTZ) {
            SetButtonState(SBICMD_TAB_GRUNTZ, MENUITEM_SELECTED);
        }
        RequestRedraw();
    }
    sp->m_state = GRUNT_OVEN_COOKING;

    m_gruntOvenSlots[idx].m_cookingClock.Start(INT_MAX);
    PlayTabCue(this, TAB_GRUNTZ, "GAME_GOOCOOKING1");
    return 1;
}

RVA(0x00105710, 0x23)
i32 CStatusBarMgr::StartAvailableGruntOven() {
    for (i32 i = 0; i < 5; i++) {
        if (StartGruntOven(i)) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00105750, 0x1f)
void CStatusBarMgr::AdvanceGruntWell(i32 delta) {
    i32 v = m_gruntWellLevel + delta;
    v = min(v, GRUNT_WELL_FULL);
    m_gruntWellTargetLevel = v;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00105780, 0x1f)
void CStatusBarMgr::DrainGruntWell(i32 delta) {
    m_gruntWellTargetLevel = max(m_gruntWellLevel - delta, GRUNT_WELL_EMPTY);
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

RVA(0x00105800, 0x9e)
i32 CStatusBarMgr::PlaceCursorTarget(i32 unitIndex, i32 activateCamera) {
    i32 playerIndex = g_curPlayer;
    if (g_gameReg->GetTriggerMgr()->SelectUnit(playerIndex, unitIndex, 0, 0) != 0) {

        CGrunt* entry = g_gameReg->GetTriggerMgr()->UnitAt(playerIndex, unitIndex);
        if (entry != NULL) {
            (static_cast<CPlay*>(g_gameReg->GetCurrentState()))
                ->SetCameraPosition(
                    entry->GetSpriteObject()->m_screenX,
                    entry->GetSpriteObject()->m_screenY
                );
            if (activateCamera != 0) {
                CTriggerMgr* obj = g_gameReg->GetTriggerMgr();
                if (obj->IsUnitSelected(playerIndex, unitIndex)) {
                    obj->SetCameraTarget(playerIndex, unitIndex);
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
    UpdateResourceMachineAnimation();
    UpdateResourceDeliveryAnimation();
    UpdateChipGrinderStatusBar();
    UpdateDestructWarningAnimation();
}

RVA(0x00105920, 0x47)
void CStatusBarMgr::Reset() {
    ResetGruntOvens();
    m_gruntWellTargetLevel = GRUNT_WELL_EMPTY;
    m_gruntWellLevel = GRUNT_WELL_EMPTY;
    ResetConveyorBelts();
    ResetResourceMachine();
    ResetResourceSlots();
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
                        StartResourceGrinderDrop(
                            m_deliveryPickupType,
                            m_deliveryItemRect.left + 0xc,
                            m_deliveryItemRect.top + 0xc
                        );
                        PrepareNextResource();
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
                        m_resourceDeliveryPhase = BELT_FALLING_OFF;
                        m_resourceDeliveryClock.Start(
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
            m_conveyorSprites[i]->SetFrameIndex(m_conveyorSlots[i].m_counter);
        }
    }
}

RVA(0x00105e40, 0x63c)
void CStatusBarMgr::UpdateResourceMachineAnimation() {
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
                    m_resourceDeliveryPhase = BELT_IN_MACHINE;
                    m_resourceDeliveryClock.Start(
                        g_buteMgr.GetDword("StatusBar", "NextItemDelay", 0x64)
                    );
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
                    PickupType which = static_cast<PickupType>(m_deliveryPickupType);
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
            m_conveyorSprites[i]->SetFrameIndex(-1);
        }
    }
}

RVA(0x00106660, 0x68)
void CStatusBarMgr::ResetResourceMachine() {
    SetLeftRezMachineAnimation(
        1,
        MACHINE_SNOOZING,
        g_buteMgr.GetDword("StatusBar", "LeftMachineSnoozingDelay", 100)
    );
    SetRightRezMachineAnimation(0x2b, MACHINE_STOPPED, INT_MAX);
    if (m_machineDisplay) {
        m_machineDisplay->SetFrames(m_leftMachine.m_counter, m_rightMachine.m_counter);
    }
    m_resourceDeliveryActive = false;
    m_pendingResourceDeliveries = 0;
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
void CStatusBarMgr::FinishGruntPlacement(b32 placed) {
    if (placed) {
        EmptyGruntOven(m_selectedGruntOvenSlot);
        m_selectedGruntOvenSlot = -1;
    } else {
        m_gruntOvenSlots[m_selectedGruntOvenSlot].m_frameIndex = s_gruntOvenReadyFrame;
        if (m_gruntOvenImages[m_selectedGruntOvenSlot]) {
            m_gruntOvenImages[m_selectedGruntOvenSlot]->SetFrameIndex(
                m_gruntOvenSlots[m_selectedGruntOvenSlot].m_frameIndex
            );
        }
        m_selectedGruntOvenSlot = -1;
    }
}

RVA(0x00106820, 0xa8)
void CStatusBarMgr::FinishResourcePlacement(i32 consumed, i32 pickupValue) {
    if (m_selectedResourceRow == STATUS_HL_ROW_NONE) {
        return;
    }
    PickupType item = static_cast<PickupType>(pickupValue);
    i32 category;
    if (item >= PICKUP_BRICKZ_FIRST) {
        category = 2;
    } else {
        category = (item >= PICKUP_TOYZ_FIRST);
    }
    if (consumed != 0) {
        ClearResourceSlot(category, m_selectedResourceRow);
        for (i32 row = IDX(m_selectedResourceRow) - 1; row >= 0; row--) {
            CSbiHlRow* cell = &m_resourceSlots[row + category * 4];
            if (cell->m_state == IDX(HLROW_IDLE_CYCLE)) {
                m_resourceSlots[row + category * 4 + 1].m_state = IDX(HLROW_IDLE_CYCLE);
                cell[1].m_value = cell->m_value;
                cell->m_state = IDX(HLROW_OFF);
                cell->m_value = 0;
            }
        }
    } else {
        m_resourceSlots[IDX(m_selectedResourceRow) + category * 4].m_value = pickupValue;
    }
    RefreshResourceImages();
    m_selectedResourceRow = STATUS_HL_ROW_NONE;
}

RVA(0x00106900, 0x8d)
void CStatusBarMgr::ResetResourceSlots() {
    for (i32 i = 0; i < 4; i++) {
        StatusBarHighlightRow row = static_cast<StatusBarHighlightRow>(i);
        ClearResourceSlot(0, row);
        ClearResourceSlot(1, row);
        ClearResourceSlot(2, row);
    }
    m_resourceDeliveryPhase = BELT_IDLE;
    m_deliveryPickupType = 0;
    m_grinderState = FALLING_ITEM_INACTIVE;
    m_grinderPickupType = 0;
    SetRect(&m_grinderItemRect, 0, 0, 1, 1);
    SetRect(&m_deliveryItemRect, 0x49, 0xd7, 0x61, 0xef);
    m_selectedResourceRow = STATUS_HL_ROW_NONE;
}

RVA(0x001069c0, 0x2e)
void CStatusBarMgr::ClearResourceSlot(i32 category, StatusBarHighlightRow row) {
    i32 idx = IDX(row) + category * 4;
    m_resourceSlots[idx].m_state = IDX(HLROW_OFF);
    m_resourceSlots[idx].m_value = 0;
    RefreshResourceImages();
}

RVA(0x00106a00, 0xbf)
void CStatusBarMgr::RefreshResourceImages() {
    if (m_resourceMainBackground) {
        m_resourceMainBackground->RequestRedraw();
    }
    if (m_resourceUpperBackground) {
        m_resourceUpperBackground->RequestRedraw();
    }
    if (m_resourceWindowBackground) {
        m_resourceWindowBackground->RequestRedraw();
    }
    if (m_deliveryItemDisplay && m_deliveryPickupType) {
        m_deliveryItemDisplay->SetFrameIndex(m_deliveryPickupType);
    }

    CSBI_ImageSet** p = &m_resourceSlotSprites[4];
    for (i32 n = 0; n < 4; n++) {
        if (p[-4]) {
            p[-4]->SetFrameIndex(m_resourceSlots[n].m_value);
        }
        if (p[0]) {
            p[0]->SetFrameIndex(m_resourceSlots[n + 4].m_value);
        }
        if (p[4]) {
            p[4]->SetFrameIndex(m_resourceSlots[n + 8].m_value);
        }
        p++;
    }

    if (m_resourceMachineFramework) {
        m_resourceMachineFramework->RequestRedraw();
    }
    if (m_grinderItemDisplay) {
        m_grinderItemDisplay->SetFrameIndex(m_grinderPickupType);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00106af0, 0x37)
i32 CStatusBarMgr::AddResourceToRow(i32 pickupValue, i32 row) {
    PickupType item = static_cast<PickupType>(pickupValue);
    i32 category;
    if (item >= PICKUP_BRICKZ_FIRST) {
        category = 2;
    } else {
        category = (item >= PICKUP_TOYZ_FIRST);
    }
    return AddResourceToSlot(category, pickupValue, row);
}

RVA(0x00106b40, 0x44)
i32 CStatusBarMgr::AddResourceToSlot(i32 category, i32 pickupValue, i32 row) {
    i32 idx = row + category * 4;
    if (m_resourceSlots[idx].m_state != IDX(HLROW_OFF)) {
        return 0;
    }
    m_resourceSlots[idx].m_value = pickupValue;
    m_resourceSlots[idx].m_state = IDX(HLROW_IDLE_CYCLE);
    RefreshResourceImages();
    return 1;
}

RVA(0x00106bb0, 0x7d8)
void CStatusBarMgr::UpdateResourceDeliveryAnimation() {
    i32 refreshFlag = 0;
    i32 rectFlag = 0;
    ClockInterval* belt = &m_resourceDeliveryClock;
    switch (m_resourceDeliveryPhase) {
        case BELT_IN_MACHINE:
            if (belt->Expired()) {
                OFFSET_RECT_X_EDGES(
                    m_deliveryItemRect,
                    g_buteMgr.GetInt("StatusBar", "NextItemSpeed", 2),
                    g_buteMgr.GetInt("StatusBar", "NextItemSpeed", 2)
                );
                rectFlag = 1;
                belt->Start(g_buteMgr.GetDword("StatusBar", "NextItemDelay", 0x64));
            }
            if (m_deliveryItemRect.left >= 0x6d) {
                m_deliveryItemRect.left = 0x6d;
                m_deliveryItemRect.right = 0x84;
                rectFlag = 1;
                m_resourceDeliveryPhase = BELT_SPEWING;
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
                m_resourceDeliveryPhase = BELT_DROP_START;
                belt->Start(g_buteMgr.GetDword("StatusBar", "NextItemWaitTime", 0x1f4));
            }
            break;
        case BELT_DROP_START:
            if (belt->Expired()) {
                m_resourceDeliveryPhase = BELT_FALLING;
                PlayTabCue(this, TAB_RESOURCE, "GAME_CHIPFALLOUT");
                belt->Start(g_buteMgr.GetDword("StatusBar", "FallingItemDelay", 0x32));
            }
            break;
        case BELT_FALLING:
            if (belt->Expired()) {
                OFFSET_RECT_Y_EDGES(
                    m_deliveryItemRect,
                    g_buteMgr.GetInt("StatusBar", "FallingItemSpeed", 2),
                    g_buteMgr.GetInt("StatusBar", "FallingItemSpeed", 2)
                );
                rectFlag = 1;
                belt->Start(g_buteMgr.GetDword("StatusBar", "FallingItemDelay", 0x32));
            }
            if (m_deliveryItemRect.bottom >= 0x11c) {
                m_deliveryItemRect.bottom = 0x11c;
                m_deliveryItemRect.top = 0x104;
                rectFlag = 1;
                PlayTabCue(this, TAB_RESOURCE, "GAME_CHIPLAND");
                m_resourceDeliveryPhase = BELT_TRAVELLING;
                belt->Start(g_buteMgr.GetDword("StatusBar", "NextItemDelay", 0x64));
                PickupType activeItem = static_cast<PickupType>(m_deliveryPickupType);
                if (activeItem >= PICKUP_BRICKZ_FIRST) {
                    m_deliveryTargetX = 0x6d;
                } else if (activeItem >= PICKUP_TOYZ_FIRST) {
                    m_deliveryTargetX = 0x45;
                } else {
                    m_deliveryTargetX = 0x1d;
                }
            }
            refreshFlag = 1;
            break;
        case BELT_TRAVELLING:
            if (belt->Expired()) {
                OFFSET_RECT_X_EDGES(
                    m_deliveryItemRect,
                    -g_buteMgr.GetInt("StatusBar", "NextItemSpeed", 2),
                    -g_buteMgr.GetInt("StatusBar", "NextItemSpeed", 2)
                );
                rectFlag = 1;
                belt->Start(g_buteMgr.GetDword("StatusBar", "NextItemDelay", 0x64));
            }
            if (m_deliveryItemRect.left <= m_deliveryTargetX) {
                m_deliveryItemRect.left = m_deliveryTargetX;
                m_deliveryItemRect.right = m_deliveryTargetX + 0x17;
                rectFlag = 1;
                ResetConveyorBelts();
                SetLeftRezMachineAnimation(
                    0x1e,
                    MACHINE_LEVER,
                    g_buteMgr.GetDword("StatusBar", "LeftMachineLeverDelay", 0x64)
                );
                m_resourceDeliveryPhase = BELT_IDLE;
            }
            refreshFlag = 1;
            break;
        case BELT_FALLING_OFF: {
            if (belt->Expired()) {
                OFFSET_RECT_Y_EDGES(
                    m_deliveryItemRect,
                    g_buteMgr.GetInt("StatusBar", "FallingItemSpeed", 2),
                    g_buteMgr.GetInt("StatusBar", "(FallingItemSpeed", 2)
                );
                rectFlag = 1;
                belt->Start(g_buteMgr.GetDword("StatusBar", "FallingItemDelay", 0x32));
            }
            i32 col;
            PickupType item2 = static_cast<PickupType>(m_deliveryPickupType);
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
            if (m_deliveryItemRect.top >= row * 0x20 + 0x13e) {
                PlayTabCue(this, TAB_RESOURCE, "GAME_CHIPLAND");
                AddResourceToSlot(col, m_deliveryPickupType, row);
                PrepareNextResource();
            }
            refreshFlag = 1;
            break;
        }
    }

    CSBI_ImageSet* w = m_deliveryItemDisplay;
    if (w) {
        if (rectFlag) {
            RECT rc;
            i32 x = m_barRect.left;
            i32 y = m_barRect.top;
            SET_RECT_COMPONENTS(
                rc,
                m_deliveryItemRect.left + x,
                m_deliveryItemRect.top + y,
                m_deliveryItemRect.right + x,
                m_deliveryItemRect.bottom + y
            );
            w->m_rect = rc;
        }
        if (refreshFlag) {
            RefreshResourceImages();
        }
    }
}

// @early-stop
RVA(0x00107590, 0xc4)
i32 CStatusBarMgr::StartResourceGrinderDrop(i32 item, i32 x, i32 y) {
    m_grinderPickupType = item;
    m_grinderState = FALLING_ITEM_DESCENDING;
    m_grinderClock.Start(g_buteMgr.GetDword("StatusBar", "FallingItemDelay", 0x32));
    CSBI_ImageSet* n = m_grinderItemDisplay;
    i32 l = x - 0xc;
    i32 t = y - 0xc;
    i32 rr = x + 0xc;
    i32 b = y + 0xc;
    SET_RECT_COMPONENTS(m_grinderItemRect, l, t, rr, b);
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
    RefreshResourceImages();
    return 1;
}

RVA(0x001076a0, 0x1f3)
void CStatusBarMgr::UpdateChipGrinderStatusBar() {

    if (m_grinderState == FALLING_ITEM_INACTIVE) {
        return;
    }

    i32 stepped = 0;
    if (m_grinderState == FALLING_ITEM_DESCENDING || m_grinderState == FALLING_ITEM_GRINDING) {
        u32 delay = g_buteMgr.GetDword("StatusBar", "FallingItemDelay", 0x32);
        i32 speed = g_buteMgr.GetInt("StatusBar", "FallingItemSpeed", 4);

        if (m_grinderItemRect.top >= 0x1c7) {
            m_grinderState = FALLING_ITEM_INACTIVE;
            m_grinderPickupType = 0;
        } else if (m_grinderItemRect.bottom >= 0x1bf) {
            if (m_grinderState != FALLING_ITEM_GRINDING) {
                PlayTabCue(this, TAB_RESOURCE, "GAME_REZGRINDING");
                m_grinderState = FALLING_ITEM_GRINDING;
            }
            delay = g_buteMgr.GetDword("StatusBar", "FallingItemShredderDelay", 0x64);
            speed = g_buteMgr.GetInt("StatusBar", "FallingItemShredderSpeed", 2);
        }

        ClockInterval* clock = &m_grinderClock;
        i64 d = static_cast<i64>(g_frameTime) - clock->GetStartTime();
        if (d >= clock->GetInterval()) {
            OFFSET_RECT_Y_EDGES(m_grinderItemRect, speed, speed);
            CSBI_ImageSet* w = m_grinderItemDisplay;
            if (w) {
                RECT rc;
                i32 sy = m_barRect.top;
                rc.bottom = sy + m_grinderItemRect.bottom;
                rc.top = sy + m_grinderItemRect.top;
                i32 sx = m_barRect.left;
                rc.left = m_grinderItemRect.left + sx;
                rc.right = m_grinderItemRect.right + sx;
                w->m_rect = rc;
            }
            clock->Start(delay);
        }
        stepped = 1;
    }

    if (m_grinderItemDisplay != NULL && stepped) {
        RefreshResourceImages();
    }
}

RVA(0x00107920, 0xb7)
i32 CStatusBarMgr::DropFallingItemAt(i32 screenX, i32 screenY, i32 itemFrame) {
    if (m_selectedResourceRow == STATUS_HL_ROW_NONE) {
        return 0;
    }
    CStatusBarItem* r = HitTestItems(screenX, screenY);
    if (r == NULL) {
        return 0;
    }
    SbiCommandId cmd = r->GetCommandId();
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
    StartResourceGrinderDrop(itemFrame, localX, localY);
    FinishResourcePlacement(1, itemFrame);
    return 1;
}

RVA(0x00107a10, 0x62)
i32 CStatusBarMgr::RequestResourceDelivery() {
    if (m_resourceDeliveryActive == false) {
        if (m_deliveryPickupType == 0) {
            return 0;
        }
        SetLeftRezMachineAnimation(
            9,
            MACHINE_WAKING,
            g_buteMgr.GetDword("StatusBar", "LeftMachineWakingDelay", 100)
        );
        m_resourceDeliveryActive = true;
    } else {
        m_pendingResourceDeliveries++;
    }
    return 1;
}

RVA(0x00107aa0, 0x23)
void CStatusBarMgr::ToggleUnitSample(i32 unitIndex) {
    if (m_unitSampleModes[unitIndex] != STATUS_SAMPLE_NONE) {
        ClearUnitSample(unitIndex);
    } else {
        SetUnitSampleMode(unitIndex, STATUS_SAMPLE_HEALTH);
    }
}

RVA(0x00107ae0, 0x1aa)
void CStatusBarMgr::ResetForLevel(i32) {
    BuildGameTabPauseButton();
    if (m_position == STATUSBAR_HIDDEN) {
        RestoreStatusBar();
    }
    if (m_activeTab != TAB_GAME) {
        ClearActiveTabContent();
        m_activeTab = TAB_GAME;
    }
    SetGameTabContent(GAME_TAB_MENU, true);
    memset(m_unitSampleModes, 0, sizeof(m_unitSampleModes));
    Reset();

    GameModeId mode = g_gameReg->GetGameMode();
    if (mode == GAMEMODE_MULTIPLAYER) {
        for (i32 i = 0; i < g_buteMgr.GetInt("Multiplayer", "StartingGruntz", 0); i++) {
            m_gruntOvenSlots[i].m_frameIndex = s_gruntOvenReadyFrame;
            m_gruntOvenSlots[i].m_state = GRUNT_OVEN_READY;
        }
    } else if (mode == GAMEMODE_BATTLEZ) {
        for (i32 i = 0; i < g_buteMgr.GetInt("Battlez", "StartingGruntz", 0); i++) {
            m_gruntOvenSlots[i].m_frameIndex = s_gruntOvenReadyFrame;
            m_gruntOvenSlots[i].m_state = GRUNT_OVEN_READY;
        }
    }

    ClearRewardQueue();
    ClockInterval* clock = &m_reserved2b0;
    clock->Clear();
    m_layoutLocked = false;
    SAFE_DELETE(m_warpStoneFly);
    CloseLevelOverlay();
    m_observerTabAvailable = false;
    m_destructButtonLocked = false;
    TryActivate();
}
RVA(0x00107d00, 0x591)
i32 CStatusBarMgr::PrepareNextResource() {
    PickupType result;
    if (g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
        if (m_rewardQueue.GetSize() > 0) {
            Coord* p = GetReward(0);
            result = static_cast<PickupType>(p->m_x);
            g_coordPool.Push(p);
            m_rewardQueue.RemoveAt(0, 1);
        } else {
            result = PICKUP_NONE;
            if (m_deliveryItemDisplay) {
                m_deliveryItemDisplay->SetFrameIndex(0);
            }
        }
    } else {
        i32 r1 = WapRand(m_randomRewardThresholds[2]);
        if (r1 <= m_randomRewardThresholds[0]) {
            i32 r = WapRand(m_randomRewardThresholds[37]);
            if (r <= m_randomRewardThresholds[17]) {
                result = PICKUP_BOMB;
            } else if (r <= m_randomRewardThresholds[18]) {
                result = PICKUP_BOOMERANG;
            } else if (r <= m_randomRewardThresholds[19]) {
                result = PICKUP_BRICK;
            } else if (r <= m_randomRewardThresholds[20]) {
                result = PICKUP_CLUB;
            } else if (r <= m_randomRewardThresholds[21]) {
                result = PICKUP_GAUNTLETZ;
            } else if (r <= m_randomRewardThresholds[22]) {
                result = PICKUP_GLOVEZ;
            } else if (r <= m_randomRewardThresholds[23]) {
                result = PICKUP_GOOBER;
            } else if (r <= m_randomRewardThresholds[24]) {
                result = PICKUP_GRAVITYBOOTZ;
            } else if (r <= m_randomRewardThresholds[25]) {
                result = PICKUP_GUNHAT;
            } else if (r <= m_randomRewardThresholds[26]) {
                result = PICKUP_NERFGUN;
            } else if (r <= m_randomRewardThresholds[27]) {
                result = PICKUP_ROCK;
            } else if (r <= m_randomRewardThresholds[28]) {
                result = PICKUP_SHIELD;
            } else if (r <= m_randomRewardThresholds[29]) {
                result = PICKUP_SHOVEL;
            } else if (r <= m_randomRewardThresholds[30]) {
                result = PICKUP_SPRING;
            } else if (r <= m_randomRewardThresholds[31]) {
                result = PICKUP_SPY;
            } else if (r <= m_randomRewardThresholds[32]) {
                result = PICKUP_SWORD;
            } else if (r <= m_randomRewardThresholds[33]) {
                result = PICKUP_TIMEBOMB;
            } else if (r <= m_randomRewardThresholds[34]) {
                result = PICKUP_TOOB;
            } else if (r <= m_randomRewardThresholds[35]) {
                result = PICKUP_WAND;
            } else {
                result = r > m_randomRewardThresholds[36] ? PICKUP_WINGZ : PICKUP_WELDER;
            }
        } else if (r1 <= m_randomRewardThresholds[1]) {
            i32 r = WapRand(m_randomRewardThresholds[16]);
            if (r <= m_randomRewardThresholds[7]) {
                result = PICKUP_BABYWALKER;
            } else if (r <= m_randomRewardThresholds[8]) {
                result = PICKUP_BEACHBALL;
            } else if (r <= m_randomRewardThresholds[9]) {
                result = PICKUP_BIGWHEEL;
            } else if (r <= m_randomRewardThresholds[10]) {
                result = PICKUP_GOKART;
            } else if (r <= m_randomRewardThresholds[11]) {
                result = PICKUP_JACKINTHEBOX;
            } else if (r <= m_randomRewardThresholds[12]) {
                result = PICKUP_JUMPROPE;
            } else if (r <= m_randomRewardThresholds[13]) {
                result = PICKUP_POGOSTICK;
            } else if (r <= m_randomRewardThresholds[14]) {
                result = PICKUP_SCROLL;
            } else {
                result = r > m_randomRewardThresholds[15] ? PICKUP_YOYO : PICKUP_SQUEAKTOY;
            }
        } else {
            i32 r = WapRand(m_randomRewardThresholds[6]);
            if (r <= m_randomRewardThresholds[3]) {
                result = PICKUP_REDBRICK;
            } else if (r <= m_randomRewardThresholds[4]) {
                result = PICKUP_BLUEBRICK;
            } else {
                result = r > m_randomRewardThresholds[5] ? PICKUP_BLACKBRICK : PICKUP_GOLDBRICK;
            }
        }
        if (result == PICKUP_WARPSTONE) {
            result = PICKUP_GAUNTLETZ;
        }
    }
    m_deliveryPickupType = IDX(result);
    m_resourceDeliveryPhase = BELT_IDLE;
    SetRect(&m_deliveryItemRect, 0x49, 0xd7, 0x61, 0xef);
    if (m_deliveryItemDisplay) {
        RECT rc;
        i32 x = m_barRect.left;
        i32 y = m_barRect.top;
        SET_RECT_COMPONENTS(
            rc,
            m_deliveryItemRect.left + x,
            m_deliveryItemRect.top + y,
            m_deliveryItemRect.right + x,
            m_deliveryItemRect.bottom + y
        );
        m_deliveryItemDisplay->m_rect = rc;
    }
    RefreshResourceImages();
    i32 c = m_pendingResourceDeliveries;
    m_resourceDeliveryActive = false;
    if (c > 0) {
        m_pendingResourceDeliveries = c - 1;
        RequestResourceDelivery();
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
            (static_cast<CPlay*>(g_gameReg->GetCurrentState()))->ResetViewport();
            if (m_position == STATUSBAR_DOCK_RIGHT) {
                DockStatusBarLeft();
                DockStatusBarRight();
            }
            break;
    }

    if (m_warpStoneFly != NULL) {
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
                m_warpStoneFly = c;
                c->m_owner = this;
            }
        }
    }

    if (m_warpStoneFly != NULL) {
        if (m_warpStoneFly->SerializeDispatch(s, mode, typeId, payload) == 0) {
            return 0;
        }
    }

    SerializeClockPair(s, mode, &m_resourceDeliveryClock);
    SerializeClockPair(s, mode, &m_grinderClock);
    SerializeClockPair(s, mode, &m_rightMachine.m_clock);
    SerializeClockPair(s, mode, &m_leftMachine.m_clock);
    SerializeClockPair(s, mode, &m_destructWarningClock);

    GruntOvenSlot* p = m_gruntOvenSlots;
    i32 n = 5;
    do {
        SerializeClockPair(s, mode, &p->m_cookingClock);
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
            SER(m_unitSideTabs[i])
            SER(m_unitSampleArrows[i])
            i++;
        } while (i < 0xf);
    }
    {
        i32 i = 0;
        CSBI_ImageSet** q = m_gruntOvenImages;
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
        CSBI_WarlordHead** q = m_multiplayerHeadButtons;
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
    SER(m_deliveryItemDisplay)
    SER(m_grinderItemDisplay)
    SER(m_destructButtonImage)
#undef SER

    RequestRedraw();
    return 1;
}

RVA(0x001090a0, 0x38f)
i32 CStatusBarMgr::Serialize(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    if (g_gameReg->World() == NULL) {
        return 0;
    }

    s->Write(this, 4);
    s->Write(&m_restorePosition, sizeof(m_restorePosition));

    g_serialCounter++;

    {
        i32 tmp = 0;
        if (m_collapsedSprite) {
            tmp = m_collapsedSprite->GetObjectId();
        }
        s->Write(&tmp, sizeof(tmp));
    }

    s->Write(&m_barRect.left, sizeof(m_barRect));
    s->Write(&m_redrawFrames, sizeof(m_redrawFrames));
    s->Write(&m_collapsedSpriteX, sizeof(m_collapsedSpriteX));
    s->Write(&m_collapsedSpriteY, sizeof(m_collapsedSpriteY));
    s->Write(&m_gameTabContent, sizeof(m_gameTabContent));
    s->Write(&m_multiplayerPlayerIndex, sizeof(m_multiplayerPlayerIndex));

    StatusSampleMode* p = m_unitSampleModes;
    for (i32 i = 0; i < TM_UNITS_PER_PLAYER; i++) {
        s->Write(p, sizeof(*p));
        p += 1;
    }

    s->Write(&m_reserved34c, sizeof(m_reserved34c));
    s->Write(&m_reserved350, sizeof(m_reserved350));
    s->Write(&m_gameplayControlsDisabled, sizeof(m_gameplayControlsDisabled));
    s->Write(&m_selectedGruntOvenSlot, sizeof(m_selectedGruntOvenSlot));
    s->Write(&m_selectedResourceRow, sizeof(m_selectedResourceRow));
    s->Write(&m_activeTab, sizeof(m_activeTab));
    s->Write(&m_gruntWellLevel, sizeof(m_gruntWellLevel));
    s->Write(&m_gruntWellTargetLevel, sizeof(m_gruntWellTargetLevel));
    s->Write(&m_deliveryTargetX, sizeof(m_deliveryTargetX));
    s->Write(&m_pendingResourceDeliveries, sizeof(m_pendingResourceDeliveries));
    s->Write(&m_resourceDeliveryActive, sizeof(m_resourceDeliveryActive));
    s->Write(&m_reserved544, sizeof(m_reserved544));
    s->Write(&m_grinderItemRect, sizeof(m_grinderItemRect));
    s->Write(&m_deliveryItemRect, sizeof(m_deliveryItemRect));
    s->Write(&m_layoutLocked, sizeof(m_layoutLocked));
    s->Write(&m_levelOverlayActive, sizeof(m_levelOverlayActive));
    s->Write(&m_quitConfirmationActive, sizeof(m_quitConfirmationActive));
    s->Write(&m_resourceDeliveryPhase, sizeof(m_resourceDeliveryPhase));
    s->Write(&m_deliveryPickupType, sizeof(m_deliveryPickupType));
    s->Write(&m_grinderState, sizeof(m_grinderState));
    s->Write(&m_grinderPickupType, sizeof(m_grinderPickupType));
    s->Write(&m_rightMachine, 4);
    s->Write(&m_rightMachine.m_value, sizeof(m_rightMachine.m_value));
    s->Write(&m_leftMachine, 4);
    s->Write(&m_leftMachine.m_value, sizeof(m_leftMachine.m_value));
    s->Write(&m_destructWarningState, sizeof(m_destructWarningState));
    s->Write(&m_destructButtonFrame, sizeof(m_destructButtonFrame));
    s->Write(&m_destructButtonLocked, sizeof(m_destructButtonLocked));
    s->Write(&m_observerTabAvailable, sizeof(m_observerTabAvailable));

    for (i32 j = 0; j < 5; j++) {
        s->Write(&m_gruntOvenSlots[j].m_state, sizeof(m_gruntOvenSlots[j].m_state));
        s->Write(&m_gruntOvenSlots[j].m_frameIndex, sizeof(m_gruntOvenSlots[j].m_frameIndex));
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
    CDDrawSurfaceMgr* dir = g_gameReg->World();
    if (dir == NULL) {
        return 0;
    }
    m_destructWarningSound = NULL;
    ResetWidgets(false);

    ar->Read(this, 4);
    ar->Read(&m_restorePosition, sizeof(m_restorePosition));

    SERIALREF(m_collapsedSprite);

    ar->Read(&m_barRect.left, sizeof(m_barRect));
    ar->Read(&m_redrawFrames, sizeof(m_redrawFrames));
    ar->Read(&m_collapsedSpriteX, sizeof(m_collapsedSpriteX));
    ar->Read(&m_collapsedSpriteY, sizeof(m_collapsedSpriteY));
    ar->Read(&m_gameTabContent, sizeof(m_gameTabContent));
    ar->Read(&m_multiplayerPlayerIndex, sizeof(m_multiplayerPlayerIndex));

    StatusSampleMode* p = m_unitSampleModes;
    for (i32 i = 0; i < TM_UNITS_PER_PLAYER; i++) {
        ar->Read(p, sizeof(*p));
        p += 1;
    }

    ar->Read(&m_reserved34c, sizeof(m_reserved34c));
    ar->Read(&m_reserved350, sizeof(m_reserved350));
    ar->Read(&m_gameplayControlsDisabled, sizeof(m_gameplayControlsDisabled));
    ar->Read(&m_selectedGruntOvenSlot, sizeof(m_selectedGruntOvenSlot));
    ar->Read(&m_selectedResourceRow, sizeof(m_selectedResourceRow));
    ar->Read(&m_activeTab, sizeof(m_activeTab));
    ar->Read(&m_gruntWellLevel, sizeof(m_gruntWellLevel));
    ar->Read(&m_gruntWellTargetLevel, sizeof(m_gruntWellTargetLevel));
    ar->Read(&m_deliveryTargetX, sizeof(m_deliveryTargetX));
    ar->Read(&m_pendingResourceDeliveries, sizeof(m_pendingResourceDeliveries));
    ar->Read(&m_resourceDeliveryActive, sizeof(m_resourceDeliveryActive));
    ar->Read(&m_reserved544, sizeof(m_reserved544));
    ar->Read(&m_grinderItemRect, sizeof(m_grinderItemRect));
    ar->Read(&m_deliveryItemRect, sizeof(m_deliveryItemRect));
    ar->Read(&m_layoutLocked, sizeof(m_layoutLocked));
    ar->Read(&m_levelOverlayActive, sizeof(m_levelOverlayActive));
    ar->Read(&m_quitConfirmationActive, sizeof(m_quitConfirmationActive));
    ar->Read(&m_resourceDeliveryPhase, sizeof(m_resourceDeliveryPhase));
    ar->Read(&m_deliveryPickupType, sizeof(m_deliveryPickupType));
    ar->Read(&m_grinderState, sizeof(m_grinderState));
    ar->Read(&m_grinderPickupType, sizeof(m_grinderPickupType));
    ar->Read(&m_rightMachine, 4);
    ar->Read(&m_rightMachine.m_value, sizeof(m_rightMachine.m_value));
    ar->Read(&m_leftMachine, 4);
    ar->Read(&m_leftMachine.m_value, sizeof(m_leftMachine.m_value));
    ar->Read(&m_destructWarningState, sizeof(m_destructWarningState));
    ar->Read(&m_destructButtonFrame, sizeof(m_destructButtonFrame));
    ar->Read(&m_destructButtonLocked, sizeof(m_destructButtonLocked));
    ar->Read(&m_observerTabAvailable, sizeof(m_observerTabAvailable));

    for (i32 j = 0; j < 5; j++) {
        ar->Read(&m_gruntOvenSlots[j].m_state, sizeof(m_gruntOvenSlots[j].m_state));
        ar->Read(&m_gruntOvenSlots[j].m_frameIndex, sizeof(m_gruntOvenSlots[j].m_frameIndex));
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
i32 CStatusBarMgr::ConsumeReadyGrunt() {
    for (i32 i = 0; i < 5; i++) {
        if (m_gruntOvenSlots[i].m_state == GRUNT_OVEN_READY) {
            EmptyGruntOven(i);
            return 1;
        }
    }
    return 0;
}

RVA(0x00109ad0, 0xa9)
i32 CStatusBarMgr::StartWarpStoneFly(i32 srcX, i32 srcY, WarpStoneFragment fragment) {
    if (m_warpStoneFly) {
        return 0;
    }
    CWarpStoneFly* o = new CWarpStoneFly();
    m_warpStoneFly = o;
    if (o == NULL) {
        return 0;
    }
    return o->Init(this, srcX, srcY, fragment);
}

RVA(0x00109bb0, 0xb)
CWarpStoneFly::CWarpStoneFly() {
    m_frameImage = NULL;
    m_owner = NULL;
}

RVA(0x00109bd0, 0x1b5)
i32 CWarpStoneFly::Init(CStatusBarMgr* owner, i32 srcX, i32 srcY, WarpStoneFragment fragment) {
    m_owner = owner;

    i32 n = IDX(fragment) + 1;
    CImage* frame = g_gameReg->World()->FindFrame("GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE", n);
    m_frameImage = frame;
    if (frame == NULL) {

        return 0;
    }

    m_fragment = fragment;
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
    m_targetX = base->GetBarRect()->left + targetOffset.m_x;
    m_targetY = base->GetBarRect()->top + targetOffset.m_y;

    i32 deltaX = m_targetX - srcX;
    i32 dyv = m_targetY - srcY;
    i32 dist2 = SquaredDistance(deltaX, dyv);
    double dist = sqrt(static_cast<double>(dist2));
    u32 flyTime = g_buteMgr.GetDword("WarpStone", "FlyTime", 0x5dc);

    m_speedPixelsPerMs = dist / static_cast<double>(flyTime);
    m_xDirection = static_cast<double>(deltaX) / dist;
    m_yDirection = static_cast<double>(dyv) / dist;

    SoundCueRegistry* h = g_gameReg->World()->SoundRegistry();
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
    CDDrawSurfaceMgr* lvl = g_gameReg->World();
    if (lvl == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_LOAD: {

            arc->Read(&m_fragment, sizeof(m_fragment));
            arc->Read(&m_targetX, sizeof(m_targetX));
            arc->Read(&m_targetY, sizeof(m_targetY));
            arc->Read(&m_currentX, sizeof(m_currentX));
            arc->Read(&m_currentY, sizeof(m_currentY));
            arc->Read(&m_speedPixelsPerMs, sizeof(m_speedPixelsPerMs));
            arc->Read(&m_xDirection, sizeof(m_xDirection));
            arc->Read(&m_yDirection, sizeof(m_yDirection));
            char name[SERIAL_NAME_LEN];
            i32 index;
            SERIAL_READ_FRAME(arc, lvl, name, index, m_frameImage);
            return 1;
        }
        case SERIAL_SAVE: {

            arc->Write(&m_fragment, sizeof(m_fragment));
            arc->Write(&m_targetX, sizeof(m_targetX));
            arc->Write(&m_targetY, sizeof(m_targetY));
            arc->Write(&m_currentX, sizeof(m_currentX));
            arc->Write(&m_currentY, sizeof(m_currentY));
            arc->Write(&m_speedPixelsPerMs, sizeof(m_speedPixelsPerMs));
            arc->Write(&m_xDirection, sizeof(m_xDirection));
            arc->Write(&m_yDirection, sizeof(m_yDirection));
            g_serialCounter++;

            CImage* obj = m_frameImage;
            char name[SERIAL_NAME_LEN];
            i32 index = 0;
            memset(name, 0, SERIAL_NAME_LEN);
            if (obj != NULL) {
                lvl->GetImageRegistry()->FindFrameIdentity(obj, name, &index);
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
    i32 currentY = static_cast<i32>(m_currentY);
    i32 currentX = static_cast<i32>(m_currentX);
    if (currentX == m_targetX && currentY == m_targetY) {
        WarpStoneFragment fragment = m_fragment;
        g_gameReg->GetTriggerMgr()->AddWarpStoneFragment(fragment);
        m_owner->m_layoutLocked = false;
        if (m_owner->GetDockState() != STATUSBAR_HIDDEN && m_owner->GetActiveTab() == TAB_GAME) {
            m_owner->ResetWidgets(false);
            m_owner->TryActivate();
        }
        CStatusBarMgr* owner = m_owner;
        SAFE_DELETE(owner->m_warpStoneFly);
        return 1;
    }

    double t = static_cast<double>(dt);
    double newX = m_currentX + (t * m_speedPixelsPerMs) * m_xDirection;
    double newY = m_currentY + (t * m_yDirection) * m_speedPixelsPerMs;
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
    m_frameImage->RenderFrame(
        g_gameReg->World()->GetDrawTarget()->GetBackPair(),
        static_cast<i32>(m_currentX),
        static_cast<i32>(m_currentY),
        0
    );
    return 1;
}

RVA(0x0010a340, 0xbcb)
i32 CStatusBarMgr::BuildLevelOverlay() {
    if (m_levelOverlayActive == false) {
        return 1;
    }

    CDDrawSurfaceMgr* w = m_world;
    i32 cx;
    i32 cy;
    CRect dst(w->GetLevel()->GetViewportRect());
    cx = dst.left + dst.Width() / 2;
    cy = dst.top + dst.Height() / 2;

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
            CRect(cx, cy, cx + 0xbc, cy + 0x79),
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
            CRect(cx + 0x19, cy + 0x4d, cx + 0x4c, cy + 0x64),
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
            CRect(cx + 0x6b, cy + 0x4d, cx + 0x9e, cy + 0x64),
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

    FinishLevelReason reason = g_gameReg->GetTriggerMgr()->GetFinishReason();

    CSBI_Image* dialog;
    NEW_STATUS_BAR_ITEM(
        dialog,
        CSBI_Image,
        w,
        SBICMD_DIALOG_FRAME,
        TAB_DIALOG,
        CRect(cx, cy, cx + 0x11c, cy + 0x90),
        "GAME_STATUSBAR_TABZ_DIALOG",
        -1,
        0
    );
    AddTabItem(6, dialog);

    if (g_gameReg->GetTriggerMgr()->GetFinishState() == FINISH_STATE_VICTORY) {

        CSBI_ImageSet* status;
        NEW_STATUS_BAR_ITEM(
            status,
            CSBI_ImageSet,
            w,
            SBICMD_DIALOG_MISSION_STATUS,
            TAB_DIALOG,
            CRect(cx, cy + 0x17, cx + 0x11b, cy + 0x32),
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
            CRect(cx + 0x12, cy + 0x37, cx + 0x101, cy + 0x4c),
            "GAME_STATUSBAR_TABZ_DIALOG_REASON",
            IDX(reason),
            0
        );
        AddTabItem(6, rsn);

        if (g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
            CSBI_MenuItem* next;
            NEW_STATUS_BAR_ITEM(
                next,
                CSBI_MenuItem,
                w,
                SBICMD_DIALOG_PRIMARY,
                TAB_DIALOG,
                CRect(cx + 0x11, cy + 0x5f, cx + 0x80, cy + 0x7a),
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
                CRect(cx + 0x8e, cy + 0x5f, cx + 0xfd, cy + 0x7a),
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
                CRect(cx + 0x55, cy + 0x5f, cx + 0xc4, cy + 0x7a),
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
        CRect(cx, cy + 0x17, cx + 0x11b, cy + 0x32),
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
        CRect(cx + 0x12, cy + 0x37, cx + 0x101, cy + 0x4c),
        "GAME_STATUSBAR_TABZ_DIALOG_REASON",
        IDX(reason),
        0
    );
    AddTabItem(6, rsn);

    if (g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
        CSBI_MenuItem* replay;
        NEW_STATUS_BAR_ITEM(
            replay,
            CSBI_MenuItem,
            w,
            SBICMD_DIALOG_PRIMARY,
            TAB_DIALOG,
            CRect(cx + 0x11, cy + 0x5f, cx + 0x80, cy + 0x7a),
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
            CRect(cx + 0x8e, cy + 0x5f, cx + 0xfd, cy + 0x7a),
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
        if (g_gameReg->m_players[i].HasJoinedRound() != false
            && g_gameReg->m_players[i].HasDropped() == false
            && g_gameReg->m_players[i].IsEliminated() == false) {
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
            CRect(cx + 0x11, cy + 0x5f, cx + 0x80, cy + 0x7a),
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
            CRect(cx + 0x8e, cy + 0x5f, cx + 0xfd, cy + 0x7a),
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
            CRect(cx + 0x55, cy + 0x5f, cx + 0xc4, cy + 0x7a),
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
void CStatusBarMgr::CloseLevelOverlay() {
    if (m_levelOverlayActive == false) {
        return;
    }
    DELETE_STATUS_ITEMS(m_tabLists[6])
    b32 wasQuitConfirmation = m_quitConfirmationActive;
    m_endPrimaryButton = NULL;
    m_endSecondaryButton = NULL;
    m_confirmYesButton = NULL;
    m_confirmNoButton = NULL;
    m_layoutLocked = false;
    if (wasQuitConfirmation == false && g_gameReg->GetGameMode() != GAMEMODE_QUESTZ) {
        if (m_position == STATUSBAR_HIDDEN) {
            RestoreStatusBar();
        }
        if (m_activeTab != TAB_GAME) {
            SetButtonState(SBICMD_TAB_GAME, MENUITEM_SELECTED);
        }
        SetGameTabContent(GAME_TAB_MENU, true);
        RequestRedraw();
    } else {
        m_gameplayControlsDisabled = false;
    }
    m_levelOverlayActive = false;
    m_quitConfirmationActive = false;
    RequestRedraw();
}

RVA(0x0010b320, 0x167)
void CStatusBarMgr::UpdateDestructWarningAnimation() {

    switch (m_destructWarningState) {
        case DESTRUCT_WARNING_FORWARD: {
            ClockInterval* clock = &m_destructWarningClock;
            i64 d = static_cast<i64>(g_frameTime) - clock->GetStartTime();
            if (d >= clock->GetInterval()) {
                m_destructButtonFrame = static_cast<DestructButtonFrame>(m_destructButtonFrame + 1);
                if (m_destructButtonFrame >= DESTRUCT_FRAME_WARNING_LAST) {
                    m_destructButtonFrame = DESTRUCT_FRAME_WARNING_LAST;
                    m_destructWarningState = DESTRUCT_WARNING_REVERSE;
                }
                clock->Start(g_buteMgr.GetDword("StatusBar", "DestructButtonWarningDelay", 0x32));
                CSBI_ImageSet* destructButtonImage = m_destructButtonImage;
                if (destructButtonImage) {
                    destructButtonImage->SetFrameIndex(IDX(m_destructButtonFrame));
                }
            }
            break;
        }
        case DESTRUCT_WARNING_REVERSE: {
            ClockInterval* clock = &m_destructWarningClock;
            i64 d = static_cast<i64>(g_frameTime) - clock->GetStartTime();
            if (d >= clock->GetInterval()) {
                m_destructButtonFrame = static_cast<DestructButtonFrame>(m_destructButtonFrame - 1);
                if (m_destructButtonFrame <= DESTRUCT_FRAME_WARNING_FIRST) {
                    m_destructButtonFrame = DESTRUCT_FRAME_WARNING_FIRST;
                    m_destructWarningState = DESTRUCT_WARNING_FORWARD;
                }
                clock->Start(g_buteMgr.GetDword("StatusBar", "DestructButtonWarningDelay", 0x32));
                CSBI_ImageSet* destructButtonImage = m_destructButtonImage;
                if (destructButtonImage) {
                    destructButtonImage->SetFrameIndex(IDX(m_destructButtonFrame));
                }
            }
            break;
        }
    }
}

RVA(0x0010b4f0, 0xaa)
void CStatusBarMgr::CycleMultiplayerPlayer(i32 reverse) {
    if (m_layoutLocked != false) {
        return;
    }
    if (g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
        return;
    }
    if (m_position == STATUSBAR_HIDDEN) {
        RestoreStatusBar();
    }
    if (m_activeTab != TAB_MULTIPLAYER) {
        SetButtonState(SBICMD_TAB_MULTIPLAYER, MENUITEM_SELECTED);
        RequestRedraw();
        return;
    }
    if (reverse != 0) {
        if (++m_multiplayerPlayerIndex < 0) {
            m_multiplayerPlayerIndex = 3;
        }
    } else {
        if (++m_multiplayerPlayerIndex >= 4) {
            m_multiplayerPlayerIndex = 0;
        }
    }
    ResetWidgets(false);
    TryActivate();
    RequestRedraw();
}

RVA(0x0010b5d0, 0xdd)
i32 CStatusBarMgr::SelectToolResource(StatusBarHighlightRow row) {
    i32 rowIndex = IDX(row);
    if ((static_cast<CPlay*>(g_gameReg->GetCurrentState()))->m_playerCommandPending == false
        && m_resourceSlots[rowIndex].m_state == IDX(HLROW_IDLE_CYCLE)) {
        i32 handle = m_resourceSlots[rowIndex].m_value;
        i32* slot = &m_resourceSlots[rowIndex].m_value;
        if ((static_cast<CPlay*>(g_gameReg->GetCurrentState()))->SelectCursor(handle)) {
            HiCueTimed();
            m_selectedResourceRow = row;
            *slot = 0;
            RefreshResourceImages();
            return 1;
        }
    }
    return 0;
}

RVA(0x0010b6f0, 0xdd)
i32 CStatusBarMgr::SelectToyResource(StatusBarHighlightRow row) {
    i32 rowIndex = IDX(row);
    if ((static_cast<CPlay*>(g_gameReg->GetCurrentState()))->m_playerCommandPending == false
        && m_resourceSlots[rowIndex + 4].m_state == IDX(HLROW_IDLE_CYCLE)) {
        i32 handle = m_resourceSlots[rowIndex + 4].m_value;
        i32* slot = &m_resourceSlots[rowIndex + 4].m_value;
        if ((static_cast<CPlay*>(g_gameReg->GetCurrentState()))->SelectCursor(handle)) {
            HiCueTimed();
            m_selectedResourceRow = row;
            *slot = 0;
            RefreshResourceImages();
            return 1;
        }
    }
    return 0;
}

RVA(0x0010b810, 0xdd)
i32 CStatusBarMgr::SelectBrickResource(StatusBarHighlightRow row) {
    i32 rowIndex = IDX(row);
    if ((static_cast<CPlay*>(g_gameReg->GetCurrentState()))->m_playerCommandPending == false
        && m_resourceSlots[rowIndex + 8].m_state == IDX(HLROW_IDLE_CYCLE)) {
        i32 handle = m_resourceSlots[rowIndex + 8].m_value;
        i32* slot = &m_resourceSlots[rowIndex + 8].m_value;
        if ((static_cast<CPlay*>(g_gameReg->GetCurrentState()))->SelectCursor(handle)) {
            HiCueTimed();
            m_selectedResourceRow = row;
            *slot = 0;
            RefreshResourceImages();
            return 1;
        }
    }
    return 0;
}

RVA(0x0010b930, 0x1a7)
i32 CStatusBarMgr::SelectGruntOvenForPlacement(i32 idx) {
    if ((static_cast<CPlay*>(g_gameReg->GetCurrentState()))->m_playerCommandPending == false) {
        if (idx == -1) {
            for (i32 slot = 0; slot < 5; slot++) {
                if (m_gruntOvenSlots[slot].m_state == GRUNT_OVEN_READY) {
                    return BeginGruntPlacement(slot);
                }
            }
            return 0;
        }
        if (m_gruntOvenSlots[idx].m_state == GRUNT_OVEN_READY) {
            return BeginGruntPlacement(idx);
        }
    }
    return 0;
}

RVA(0x0010bb50, 0x24)
void CStatusBarMgr::DiscardSelectedResource(i32 pickupValue) {
    StartResourceGrinderDrop(pickupValue, 0x4f, 0x1b3);
    FinishResourcePlacement(1, pickupValue);
}

RVA(0x0010bb90, 0x3f)
void CStatusBarMgr::LockDestructButton(i32 resetWarningAnimation) {
    m_destructButtonLocked = true;
    if (resetWarningAnimation && m_destructButtonFrame != DESTRUCT_FRAME_DISABLED) {
        m_destructWarningState = DESTRUCT_WARNING_INACTIVE;
        m_destructButtonFrame = DESTRUCT_FRAME_IDLE;
        if (m_destructButtonImage) {
            m_destructButtonImage->SetFrameIndex(1);
        }
    }
}

RVA(0x0010bbe0, 0x34)
i32 CStatusBarMgr::GetNextResourcePickup() {
    if (m_resourceDeliveryActive == false) {
        return m_deliveryPickupType;
    }
    if (m_rewardQueue.GetSize() > 0 && m_rewardQueue.GetSize() > m_pendingResourceDeliveries) {
        return GetReward(m_pendingResourceDeliveries)->m_x;
    }
    return 0;
}
