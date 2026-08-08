#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/StreamFeeder.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameMenuMgrBuilders.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
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
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarTab.h>
#include <Gruntz/StatusBarTabWidgets.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/Timer.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WarpStoneFly.h>
#include <Image/CImage.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <Rez/RezList.h>
#include <Rez/RezMgr.h>
#include <Utils/MapTyped.h>
#include <Utils/RegistryHelper.h>
#include <Wap32/ScreenGeometry.h>

#include <limits.h>
#include <math.h>
#include <new>
#include <string.h>

DATA(0x00244c54)
i32 g_curPlayer = 0;

RVA(0x000c86d0, 0x11)
CSbiHlRow::CSbiHlRow() {

    m_lastLo = 0;
    m_intervalLo = 0;
    m_lastHi = 0;
    m_intervalHi = 0;
}

RVA(0x000fdc00, 0x5c2)
i32 CStatusBarMgr::LoadBattlezItemConfig(CDDrawSurfaceMgr* world) {
    m_world = world;
    m_restorePosition = STATUSBAR_DOCK_RIGHT;
    m_position = STATUSBAR_DOCK_RIGHT;
    i32 vx = g_gameReg->m_modeSize.cx;
    i32 vy = g_gameReg->m_modeSize.cy;
    SetRect(&m_rect10, vx - 0xa0, 0, vx, SCREEN_H_PX);
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
    m_toggleActive = 0;
    m_toggleHandle = 0;
    m_battlezPct[0] = g_buteMgr.GetInt("Multiplayer", "ToolzPercent");
    m_battlezPct[1] = m_battlezPct[0] + g_buteMgr.GetInt("Multiplayer", "ToyzPercent");
    m_battlezPct[2] = m_battlezPct[1] + g_buteMgr.GetInt("Multiplayer", "BrickzPercent");
    m_battlezPct[3] = m_battlezPct[2] + g_buteMgr.GetInt("Multiplayer", "RedBrick");
    m_battlezPct[4] = m_battlezPct[3] + g_buteMgr.GetInt("Multiplayer", "BlueBrick");
    m_battlezPct[5] = m_battlezPct[4] + g_buteMgr.GetInt("Multiplayer", "GoldBrick");
    m_battlezPct[6] = m_battlezPct[5] + g_buteMgr.GetInt("Multiplayer", "BlackBrick");
    m_battlezPct[7] = m_battlezPct[6] + g_buteMgr.GetInt("Multiplayer", "BabyWalkerz");
    m_battlezPct[8] = m_battlezPct[7] + g_buteMgr.GetInt("Multiplayer", "BeachBallz");
    m_battlezPct[9] = m_battlezPct[8] + g_buteMgr.GetInt("Multiplayer", "BigWheelz");
    m_battlezPct[10] = m_battlezPct[9] + g_buteMgr.GetInt("Multiplayer", "GoKartz");
    m_battlezPct[11] = m_battlezPct[10] + g_buteMgr.GetInt("Multiplayer", "JackInTheBoxz");
    m_battlezPct[12] = m_battlezPct[11] + g_buteMgr.GetInt("Multiplayer", "JumpRopez");
    m_battlezPct[13] = m_battlezPct[12] + g_buteMgr.GetInt("Multiplayer", "PogoStickz");
    m_battlezPct[14] = m_battlezPct[13] + g_buteMgr.GetInt("Multiplayer", "Scrollz");
    m_battlezPct[15] = m_battlezPct[14] + g_buteMgr.GetInt("Multiplayer", "SqueakToyz");
    m_battlezPct[16] = m_battlezPct[15] + g_buteMgr.GetInt("Multiplayer", "Yoyoz");
    m_battlezPct[17] = m_battlezPct[16] + g_buteMgr.GetInt("Multiplayer", "Bombz");
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
        RefreshA();
    }
    return 1;
}

RVA(0x000fe350, 0x6d)
void CStatusBarMgr::Teardown() {
    (static_cast<Utils::RegistryHelper*>(g_gameReg->m_settings))
        ->SetValueDword("StatusBar Position", IDX(m_position));
    ResetWidgets(0);
    for (i32 i = 0; i < m_ptrPool.GetSize(); i++) {
        void* p = m_ptrPool.GetData()[i];
        if (p) {
            CoordPoolNode* node = g_coordPool.NodeOf(p);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }

    m_ptrPool.SetSize(0, -1);
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
i32 CStatusBarMgr::RefreshA() {
    if (m_hlBusy == 0 && m_position != STATUSBAR_DOCK_LEFT) {
        ResetWidgets(1);
        SetRect(&m_rect10, 0, 0, 0xa0, SCREEN_H_PX);
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

    i32 w = g_gameReg->m_modeSize.cx;
    volatile POINT pt;
    pt.y = g_gameReg->m_modeSize.cy;
    SetRect(&m_rect10, w - 0xa0, 0, w, SCREEN_H_PX);
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
        SetRect(&m_rect10, -1, -1, -1, -1);
        SetState(STATUSBAR_HIDDEN);
        (static_cast<CPlay*>(g_gameReg->m_curState))->ResetViewport();
    }
    return 1;
}

RVA(0x000fe670, 0x2b)
i32 CStatusBarMgr::RefreshState() {
    if (m_hlBusy != 0) {
        return 1;
    }
    if (m_position != STATUSBAR_HIDDEN) {
        return 1;
    }
    if (m_restorePosition == STATUSBAR_DOCK_LEFT) {
        return RefreshA();
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
                below.left = m_rect10.left;
                below.top = m_rect10.bottom;
                below.right = m_rect10.right;
                below.bottom = v;
                tgt->Restore(&below, 0);
            }
            CMapStringToOb* map = &m_world->m_imageRegistry->m_10map;
            CObject* found = 0;

            map->Lookup("GAME_STATUSBAR_MAINBAR", found);
            if (found) {

                CDDrawWorker* cfg = static_cast<CDDrawWorker*>(found);
                CImage* entry = static_cast<CImage*>(cfg->m_items.GetAt(cfg->m_minIndex));
                if (entry) {
                    CDDrawSubMgrPages* l1 = g_gameReg->m_world->m_drawTarget;
                    entry->RenderFrame(
                        l1->m_backPair,
                        entry->m_anchorX + m_rect10.left,
                        entry->m_anchorY + m_rect10.top,
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
            p->SetSubtype();
            p->Render();
        }
    }
    return 1;
}

static __inline void HiCueFind() {
    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
    if (host->m_emitGate == 0) {
        void* obj = ((host))->Lookup("GAME_TABHIGHLIGHT1");
        if (obj) {
            (static_cast<LeafCue*>(obj))->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
        }
    }
}

static __inline void HiCueLookup() {
    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
    if (host->m_emitGate == 0) {
        void* out = 0;
        host->m_cues.Lookup("GAME_TABHIGHLIGHT1", out);
        if (out) {
            (static_cast<LeafCue*>(out))->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
        }
    }
}

static __inline void HiCueTimed() {
    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
    if (host->m_emitGate == 0) {
        void* found = 0;
        host->m_cues.Lookup("GAME_TABHIGHLIGHT1", found);
        if (found && g_sndEnabled != 0) {
            i32 item = g_sndCueTag;
            LeafCue* p = static_cast<LeafCue*>(found);
            if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                >= static_cast<u32>(p->m_replayDelay)) {
                p->m_lastPlayTime = g_killCueClock;
                p->m_sound->ConfigureItem(item, 0, 0, 0);
            }
        }
    }
}

static __inline void HiPost(i32 cmdId) {
    PostMessageA(g_gameReg->m_gameWnd->m_hwnd, WM_COMMAND, cmdId, 0);
}

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
    CWwdGameObjectA* r = m_barSprite;
    CImage* L = r->m_layer;
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
// The TAB_RESOURCE arm is a 12-case jump table in retail (`lea eax,[ebx-0xd3];
// cmp eax,0xb; ja; mov cl,[eax+idx]; jmp [ecx*4+tbl]` over three group bodies), not
// the range chain written here. Writing the switch needs the twelve SBICMD_HL_*
// enumerators, and only the six GROUP<n>_FIRST/LAST bounds are proven, so the labels
// would have to be invented.
RVA(0x000fe910, 0xc2c)
i32 CStatusBarMgr::UpdateStatusBarTabHighlight(i32 a1, i32 a2, i32 a3) {
    CStatusBarItem* w = HitTestRects(a2, a3);
    if (w == NULL) {
        return 1;
    }
    w->OnPointerMove(a1, a2, a3);
    SbiCommandId cmd = w->m_cmd;
    switch (w->m_tab) {
        case TAB_CONTROLS:
            if (m_hitTestDisabled != 0) {
                break;
            }
            if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
                break;
            }
            if (cmd < SBICMD_DOCK_FIRST) {
                if (cmd <= SBICMD_NONE || cmd > SBICMD_TAB_LAST) {
                    return 0;
                }
                HiCueFind();
                SetTabState(cmd, MENUITEM_SELECTED);
                return 1;
            } else if (cmd == SBICMD_DOCK_LEFT) {
                HiCueFind();
                RefreshA();
                return 1;
            } else {
                switch (cmd) {
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
            }

        case TAB_GAME:
            if (m_toggleActive != 0) {
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
                        i32 flipped = g_gameReg->m_frameGate ^ 1;
                        g_gameReg->m_frameGate = flipped;
                        g_gameReg->FinishLevel(flipped, 1);
                    }
                    (static_cast<CPlay*>(g_gameReg->m_curState))->EnterOverlayDrag(1);
                    return 1;
                case SBICMD_GAME_TAB:
                    HiCueLookup();
                    SetTab(GAME_TAB_MENU, 0);
                    return 1;
                case SBICMD_DESTRUCT:
                    if (g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
                        break;
                    }
                    if (m_modeArmed != 0) {
                        break;
                    }
                    if (m_hitTestDisabled != 0) {
                        break;
                    }
                    HiCueLookup();
                    {
                        CPlay* sm = static_cast<CPlay*>(g_gameReg->m_curState);
                        if (m_destructWarnActive == DESTRUCT_WARNING_INACTIVE) {
                            m_destructWarnActive = DESTRUCT_WARNING_FORWARD;
                            m_modeState = DESTRUCT_FRAME_WARNING_FIRST;
                            m_destructWarnDelay = g_buteMgr.GetDwordDef(
                                "StatusBar",
                                "DestructButtonWarningDelay",
                                0x32
                            );
                            m_destructWarnLast = static_cast<u32>(g_frameTime);
                            sm->ArmSnapshot(1, 0xbb7);
                        } else {
                            CSBI_ImageSet* n = m_modeNotify;
                            m_destructWarnActive = DESTRUCT_WARNING_INACTIVE;
                            m_modeState = DESTRUCT_FRAME_IDLE;
                            if (n) {
                                n->Notify(1);
                            }
                            sm->ArmSnapshot(0, 0xbb7);
                        }
                    }
                    return 1;
                default:
                    return 0;
            }
            break;

        case TAB_STATZ:
            if (m_hitTestDisabled != 0) {
                break;
            }
            if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
                break;
            }
            if (cmd < SBICMD_STAT_TOGGLE_FIRST || cmd > SBICMD_CURSOR_TARGET_LAST) {
                return 0;
            }
            if (cmd <= SBICMD_STAT_TOGGLE_LAST) {
                HiCueLookup();
                ToggleStat(IDX(cmd) - IDX(SBICMD_STAT_TOGGLE_FIRST));
            } else {
                HiCueLookup();
                PlaceCursorTarget(IDX(cmd) - IDX(SBICMD_CURSOR_TARGET_FIRST), 0);
            }
            return 1;

        case TAB_MULTIPLAYER:
            if (m_hitTestDisabled != 0) {
                break;
            }
            if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
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
            if (m_hitTestDisabled != 0) {
                break;
            }
            if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
                break;
            }
            if (cmd < SBICMD_GRUNT_SLOT_FIRST || cmd > SBICMD_GRUNT_SLOT_LAST) {
                return 0;
            }
            ActivateSlot(IDX(cmd) - IDX(SBICMD_GRUNT_SLOT_FIRST));
            return 1;

        case TAB_RESOURCE:
            if (m_hitTestDisabled != 0) {
                break;
            }
            if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
                break;
            }
            if (cmd < SBICMD_HL_GROUP0_FIRST || cmd > SBICMD_HL_GROUP2_LAST) {
                break;
            }
            if (cmd <= SBICMD_HL_GROUP0_LAST) {
                HlClickGroup0(
                    static_cast<StatusBarHighlightRow>(IDX(cmd) - IDX(SBICMD_HL_GROUP0_FIRST))
                );
            } else if (cmd <= SBICMD_HL_GROUP1_LAST) {
                HlClickGroup1(
                    static_cast<StatusBarHighlightRow>(IDX(cmd) - IDX(SBICMD_HL_GROUP1_FIRST))
                );
            } else {
                HlClickGroup2(
                    static_cast<StatusBarHighlightRow>(IDX(cmd) - IDX(SBICMD_HL_GROUP2_FIRST))
                );
            }
            return 1;

        case TAB_DIALOG:
            switch (cmd) {
                case SBICMD_DIALOG_PRIMARY:
                    if (g_gameReg->m_cmdGrid->m_phase == FINISH_STATE_VICTORY) {
                        HiCueLookup();
                        g_gameReg->AccrueScoreTime();
                    } else if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                        HiCueLookup();
                        HiPost(0x806b);
                    } else {
                        HiCueLookup();
                    }
                    return 1;
                case SBICMD_DIALOG_SECONDARY:
                    if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                        if (g_gameReg->m_cmdGrid->m_phase == FINISH_STATE_VICTORY) {
                            g_gameReg->UpdateScoreHud();
                        }
                        HiCueLookup();
                        HiPost(0x8023);
                    } else {
                        HiCueTimed();
                        g_gameReg->AccrueScoreTime();
                    }
                    return 1;
                case SBICMD_DIALOG_YES:
                    if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                        if (g_gameReg->m_cmdGrid->m_phase == FINISH_STATE_VICTORY) {
                            g_gameReg->UpdateScoreHud();
                        }
                        HiCueTimed();
                        HiPost(0x8023);
                    } else {
                        HiCueTimed();
                        g_gameReg->AccrueScoreTime();
                    }
                    return 1;
                case SBICMD_DIALOG_NO:
                    HiCueTimed();
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
i32 CStatusBarMgr::ClickHilite(i32 a, i32 x, i32 y) {
    CStatusBarItem* r = HitTestRects(x, y);
    if (r == NULL) {
        return 1;
    }
    r->Click1c(a, x, y);
    SbiCommandId cmd = r->m_cmd;
    if (r->m_tab == TAB_STATZ && m_hitTestDisabled == 0 && g_gameReg->m_cmdGrid->m_groupFlag != 0
        && cmd >= SBICMD_CURSOR_TARGET_FIRST && cmd <= SBICMD_CURSOR_TARGET_LAST) {
        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
        if (host->m_emitGate == 0) {
            void* found = 0;
            CMapStringToPtr* map = &host->m_cues;
            map->Lookup("GAME_TABHIGHLIGHT1", found);
            if (found) {
                i32 gate = g_sndEnabled;
                i32 item = g_sndCueTag;
                if (gate != 0) {
                    LeafCue* p = static_cast<LeafCue*>(found);
                    if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                        >= static_cast<u32>(p->m_replayDelay)) {
                        p->m_lastPlayTime = g_killCueClock;
                        p->m_sound->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
        PlaceCursorTarget(IDX(cmd) - IDX(SBICMD_CURSOR_TARGET_FIRST), 1);
        return 1;
    }

    return UpdateStatusBarTabHighlight(a, x, y);
}

RVA(0x000ff9d0, 0x8)
i32 CStatusBarMgr::OnPointerRelease(i32, i32, i32) {
    return 1;
}

RVA(0x000ff9f0, 0xe4)
i32 CStatusBarMgr::ClickToggle(i32 btn, i32 x, i32 y) {
    CStatusBarItem* r = HitTestRects(x, y);
    if (r == NULL) {
        ClearTabSprites(TAB_ALL);
        return 1;
    }
    r->Click24(btn, x, y);
    if (r->m_kind != SBI_KIND_MENU_ITEM) {
        ClearTabSprites(TAB_ALL);
        return 1;
    }
    SbiCommandId cmd = r->m_cmd;
    if (m_hitTestDisabled == 0) {
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
    if (m_toggleActive) {
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
i32 CStatusBarMgr::LoadDestructButtonSprite(i32 arg) {
    if (g_gameReg->m_soundEnabled != 0) {
        if (m_destructWarnActive != DESTRUCT_WARNING_INACTIVE && m_modeArmed == 0) {
            if (m_destructButton == NULL) {

                CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                CMapStringToPtr* map = &host->m_cues;
                void* found = 0;
                map->Lookup("GAME_DESTRUCT", found);
                if (found) {
                    DSoundCloneInst* f = (static_cast<LeafCue*>(found))->m_sound;
                    if (f) {
                        DirectSoundMgr* obj = f->GetItem();
                        m_destructButton = obj;
                        if (obj) {
                            obj->ApplyAndPlay(g_gameReg->m_soundVolume, 0, 0, 1);
                        }
                    }
                }
            }
        } else {
            if (m_destructButton) {
                m_destructButton->StopAndRewind();
                m_destructButton = NULL;
            }
        }
    }
    RefreshAll();

    POSITION n = m_tabLists[0].GetHeadPosition();
    while (n) {
        CStatusBarItem* cur = static_cast<CStatusBarItem*>(m_tabLists[0].GetNext(n));
        if (cur) {
            cur->Refresh(arg);
        }
    }
    CPtrList& tab = m_tabLists[IDX(m_activeTab)];
    POSITION m = tab.GetHeadPosition();
    while (m) {
        CStatusBarItem* cur = static_cast<CStatusBarItem*>(tab.GetNext(m));
        if (cur) {
            cur->Refresh(arg);
        }
    }
    POSITION k = m_tabLists[6].GetHeadPosition();
    while (k) {
        CStatusBarItem* cur = static_cast<CStatusBarItem*>(m_tabLists[6].GetNext(k));
        if (cur) {
            cur->Refresh(arg);
        }
    }
    if (m_retabNotify) {
        m_retabNotify->Tick(arg);
        Deactivate();
    }
    return 1;
}

RVA(0x000ffcb0, 0xe2)
CStatusBarItem* CStatusBarMgr::HitTestRects(i32 x, i32 y) {
    POSITION n = m_tabLists[0].GetHeadPosition();
    while (n) {
        CStatusBarItem* r = static_cast<CStatusBarItem*>(m_tabLists[0].GetNext(n));
        if (r && r->m_enabled) {
            i32 hit = CGameLevel::PointInRect(&r->m_rect14, x, y);
            if (hit) {
                return r;
            }
        }
    }
    CPtrList& tab = m_tabLists[IDX(m_activeTab)];
    n = tab.GetHeadPosition();
    while (n) {
        CStatusBarItem* r = static_cast<CStatusBarItem*>(tab.GetNext(n));
        if (r && r->m_enabled) {
            i32 hit = CGameLevel::PointInRect(&r->m_rect14, x, y);
            if (hit) {
                return r;
            }
        }
    }
    n = m_tabLists[6].GetHeadPosition();
    while (n) {
        CStatusBarItem* r = static_cast<CStatusBarItem*>(m_tabLists[6].GetNext(n));
        if (r && r->m_enabled) {
            i32 hit = CGameLevel::PointInRect(&r->m_rect14, x, y);
            if (hit) {
                return r;
            }
        }
    }
    return 0;
}

// @early-stop
// Retail CALLS ??0CStatusBarItem@@QAE@XZ at the five `new CSBI_MenuItem` sites and
// inlines it at the three CSBI_RectOnly ones, which is what gives retail its /GX EH
// frame and 0x20 of locals; cl inlines the whole chain for us. The cut depth varies
// per new-site, so no declaration form expresses it - see
// docs/patterns/ctor-inline-cut-depth-varies-per-new-site.md
RVA(0x000ffde0, 0x5b1)
i32 CStatusBarMgr::BuildStatusBarTabs() {
    if (m_tabsBuilt != 0) {
        return 1;
    }
    if (m_world == NULL) {
        return 0;
    }
    i32 bx = m_rect10.left;
    i32 by = m_rect10.top;
    CDDrawSurfaceMgr* code = m_world;

    CSBI_RectOnly* dockLeft = new CSBI_RectOnly;
    if (!dockLeft->Setup(
            this,
            code,
            SBICMD_DOCK_LEFT,
            TAB_CONTROLS,
            SbGeom(bx + 0x7c, by + 0xad, bx + 0x88, by + 0xb9),
            0,
            -1
        )) {
        if (dockLeft) {
            delete dockLeft;
        }
        return 0;
    }
    m_tabLists[0].AddTail(dockLeft);

    CSBI_RectOnly* dockRight = new CSBI_RectOnly;
    if (!dockRight->Setup(
            this,
            code,
            SBICMD_DOCK_RIGHT,
            TAB_CONTROLS,
            SbGeom(bx + 0x8a, by + 0xb9, bx + 0x96, by + 0xc7),
            0,
            -1
        )) {
        if (dockRight) {
            delete dockRight;
        }
        return 0;
    }
    m_tabLists[0].AddTail(dockRight);

    CSBI_RectOnly* hide = new CSBI_RectOnly;
    if (!hide->Setup(
            this,
            code,
            SBICMD_HIDE,
            TAB_CONTROLS,
            SbGeom(bx + 0x83, by + 0xbb, bx + 0x8f, by + 0xc7),
            0,
            -1
        )) {
        if (hide) {
            delete hide;
        }
        return 0;
    }
    m_tabLists[0].AddTail(hide);

    CSBI_MenuItem* statzTab = new CSBI_MenuItem;
    if (!statzTab->SetupImage(
            this,
            code,
            SBICMD_TAB_STATZ,
            TAB_CONTROLS,
            SbGeom(bx + 0x42, by + 0x82, bx + 0x62, by + 0x99),
            "GAME_STATUSBAR_TABZ_STATZTAB",
            -1,
            0
        )) {
        if (statzTab) {
            delete statzTab;
        }
        return 0;
    }
    m_tabLists[0].AddTail(statzTab);
    m_tabSprite0 = statzTab;

    CSBI_MenuItem* gruntzTab = new CSBI_MenuItem;
    if (!gruntzTab->SetupImage(
            this,
            code,
            SBICMD_TAB_GRUNTZ,
            TAB_CONTROLS,
            SbGeom(bx + 0x04, by + 0x82, bx + 0x24, by + 0x99),
            "GAME_STATUSBAR_TABZ_GRUNTZTAB",
            -1,
            0
        )) {
        if (gruntzTab) {
            delete gruntzTab;
        }
        return 0;
    }
    m_tabLists[0].AddTail(gruntzTab);
    m_tabSprite2 = gruntzTab;

    CSBI_MenuItem* resourceTab = new CSBI_MenuItem;
    if (!resourceTab->SetupImage(
            this,
            code,
            SBICMD_TAB_RESOURCE,
            TAB_CONTROLS,
            SbGeom(bx + 0x24, by + 0x82, bx + 0x44, by + 0x99),
            "GAME_STATUSBAR_TABZ_RESOURCETAB",
            -1,
            0
        )) {
        if (resourceTab) {
            delete resourceTab;
        }
        return 0;
    }
    m_tabLists[0].AddTail(resourceTab);
    m_tabSprite1 = resourceTab;

    CSBI_MenuItem* multiTab = new CSBI_MenuItem;
    if (!multiTab->SetupImage(
            this,
            code,
            SBICMD_TAB_MULTIPLAYER,
            TAB_CONTROLS,
            SbGeom(bx + 0x60, by + 0x82, bx + 0x80, by + 0x99),
            "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB",
            -1,
            0
        )) {
        if (multiTab) {
            delete multiTab;
        }
        return 0;
    }
    m_tabLists[0].AddTail(multiTab);
    m_tabSprite3 = multiTab;
    if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
        multiTab->m_state = MENUITEM_DISABLED;
        CDDrawWorker* f = multiTab->m_record;
        CImage* v;
        if (f != NULL && f->m_minIndex <= 4 && f->m_maxIndex >= 4) {
            v = static_cast<CImage*>(f->m_items.GetAt(4));
        } else {
            v = NULL;
        }
        multiTab->m_frame = v;
        multiTab->m_enabled = 0;
        multiTab->SetSubtype();
    }

    CSBI_MenuItem* gameTab = new CSBI_MenuItem;
    if (!gameTab->SetupImage(
            this,
            code,
            SBICMD_TAB_GAME,
            TAB_CONTROLS,
            SbGeom(bx + 0x7e, by + 0x82, bx + 0x9e, by + 0x99),
            "GAME_STATUSBAR_TABZ_GAMETAB",
            -1,
            0
        )) {
        if (gameTab) {
            delete gameTab;
        }
        return 0;
    }
    m_tabLists[0].AddTail(gameTab);
    m_tabSprite4 = gameTab;

    if (BuildSideTabs() == 0) {
        return 0;
    }
    if (RefreshState() == 0) {
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
i32 CStatusBarItem::Click1c(i32, i32, i32) {
    return 0;
}
RVA(0x00100570, 0x5)
i32 CStatusBarItem::UnusedPointerAction(i32, i32, i32) {
    return 0;
}
RVA(0x00100590, 0x5)
i32 CStatusBarItem::Click24(i32, i32, i32) {
    return 0;
}

RVA_COMPGEN(0x00100620, 0x24, ??_GCStatusBarItem@@UAEPAXI@Z)
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
            if (cur) {
                delete cur;
            }
        }
        m_tabLists[t].RemoveAll();
    }
    if (keepHost) {
        if (m_barSprite) {

            m_barSprite->m_stateFlags |= SPRITE_STATE_HIDDEN;
            m_barSprite->m_flags |= 0x10000;
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
    memset(m_groupNotify, 0, sizeof(m_groupNotify));
    memset(m_hlNotify, 0, sizeof(m_hlNotify));
    memset(m_warlordHead, 0, sizeof(m_warlordHead));
    m_extraNotify0 = NULL;
    m_extraNotify1 = NULL;
    m_modeNotify = NULL;
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
        if (cur) {
            delete cur;
        }
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
            m_modeNotify = NULL;
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

            memset(m_groupNotify, 0, sizeof(m_groupNotify));
            m_machineDisplay = NULL;

            memset(m_hlNotify, 0, sizeof(m_hlNotify));
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
            cur->SetSubtype();
        }
    }

    CPtrList& tab = m_tabLists[IDX(m_activeTab)];
    POSITION m = tab.GetHeadPosition();
    while (m) {
        CSBI_ImageSet* cur = static_cast<CSBI_ImageSet*>(tab.GetNext(m));
        if (cur) {
            cur->SetSubtype();
        }
    }

    ClearTabSprites(TAB_ALL);
    m_redrawFrames = 2;
    return 1;
}

// @early-stop
// cl cross-jumps the switch arms' identical ProbeState suffix (each arm ends in a
// `jmp` to a shared tail); retail duplicates the whole tail plus the `mov eax,1` /
// pops / `ret 8` in every arm. That is the entire 10-instruction shortfall - the
// arm bodies, the SetState/ProbeState order and the member offsets all match.
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
            return 1;
        case SBICMD_TAB_GRUNTZ:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->ProbeState(state);
            m_tabSprite2->SetState(state, 1);
            m_tabSprite1->ProbeState(state);
            m_tabSprite3->ProbeState(state);
            m_tabSprite4->ProbeState(state);
            return 1;
        case SBICMD_TAB_RESOURCE:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->ProbeState(state);
            m_tabSprite2->ProbeState(state);
            m_tabSprite1->SetState(state, 1);
            m_tabSprite3->ProbeState(state);
            m_tabSprite4->ProbeState(state);
            return 1;
        case SBICMD_TAB_MULTIPLAYER:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->ProbeState(state);
            m_tabSprite2->ProbeState(state);
            m_tabSprite1->ProbeState(state);
            m_tabSprite3->SetState(state, 1);
            m_tabSprite4->ProbeState(state);
            return 1;
        case SBICMD_TAB_GAME:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite0->ProbeState(state);
            m_tabSprite2->ProbeState(state);
            m_tabSprite1->ProbeState(state);
            m_tabSprite3->ProbeState(state);
            m_tabSprite4->SetState(state, 1);
            return 1;
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
            return 1;
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
            return 1;
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
            return 1;
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
            return 1;
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
            return 1;
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
            return 1;
        case SBICMD_GAME_TAB:
            if (m_hlBusy) {
                return 1;
            }
            m_tabSprite10->SetState(state, 1);
            return 1;
        case SBICMD_DIALOG_PRIMARY:
            if (m_tabSprite11) {
                m_tabSprite11->SetState(state, 1);
            }
            m_tabSprite12->ProbeState(state);
            return 1;
        case SBICMD_DIALOG_SECONDARY:
            if (m_tabSprite11) {
                m_tabSprite11->ProbeState(state);
            }
            m_tabSprite12->SetState(state, 1);
            return 1;
        case SBICMD_DIALOG_YES:
            m_tabSprite13->SetState(state, 1);
            m_tabSprite14->ProbeState(state);
            return 1;
        case SBICMD_DIALOG_NO:
            m_tabSprite13->ProbeState(state);
            m_tabSprite14->SetState(state, 1);
            return 1;
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
// Same ctor inline-cut wall as BuildStatusBarTabs: retail calls ??0CSBI_RectOnly
// @0x101fa0 at three `new` sites and ??0CStatusBarItem @0x1005d0 at a fourth, so it
// carries a /GX EH frame we do not. docs/patterns/ctor-inline-cut-depth-varies-per-new-site.md
RVA(0x00101580, 0x806)
i32 CStatusBarMgr::BuildGameMenu() {
    CDDrawSurfaceMgr* code = m_world;
    i32 bx = m_rect10.left;
    i32 by = m_rect10.top;
    CSBI_Image* it;
    RECT r;

    if (m_itemKind != GAME_TAB_MISSION_STATUS) {

        if (m_hitTestDisabled != 0 && g_gameReg->m_frameGate != 0) {
            it = new CSBI_MenuItem;
            r.left = bx;
            r.top = by + 0xd5;
            r.right = bx + 0x9f;
            r.bottom = by + 0xec;
            if (!it->SetupImage(
                    this,
                    code,
                    SBICMD_PAUSE,
                    TAB_GAME,
                    r,
                    "GAME_STATUSBAR_TABZ_GAMETAB_RESUME",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[5].AddTail(it);
        } else {
            it = new CSBI_MenuItem;
            r.left = bx;
            r.top = by + 0xd5;
            r.right = bx + 0x9f;
            r.bottom = by + 0xec;
            if (!it->SetupImage(
                    this,
                    code,
                    SBICMD_PAUSE,
                    TAB_GAME,
                    r,
                    "GAME_STATUSBAR_TABZ_GAMETAB_PAUSE",
                    -1,
                    0
                )) {
                if (it) {
                    delete it;
                }
                return 0;
            }
            m_tabLists[5].AddTail(it);
        }
        m_tabSprite5 = static_cast<CSBI_MenuItem*>(it);

        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0x125;
        r.right = bx + 0x9f;
        r.bottom = by + 0x13c;
        if (!it->SetupImage(
                this,
                code,
                SBICMD_LOAD_GAME,
                TAB_GAME,
                r,
                "GAME_STATUSBAR_TABZ_GAMETAB_LOAD",
                -1,
                0
            )) {
            if (it) {
                delete it;
            }
            return 0;
        }
        m_tabLists[5].AddTail(it);
        m_tabSprite6 = static_cast<CSBI_MenuItem*>(it);
        if (g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER) {
            it->m_enabled = 0;
        }

        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0xfd;
        r.right = bx + 0x9f;
        r.bottom = by + 0x114;
        if (!it->SetupImage(
                this,
                code,
                SBICMD_SAVE_GAME,
                TAB_GAME,
                r,
                "GAME_STATUSBAR_TABZ_GAMETAB_SAVE",
                -1,
                0
            )) {
            if (it) {
                delete it;
            }
            return 0;
        }
        m_tabLists[5].AddTail(it);
        m_tabSprite7 = static_cast<CSBI_MenuItem*>(it);
        if (g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER) {
            it->m_enabled = 0;
        }

        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0x14d;
        r.right = bx + 0x9f;
        r.bottom = by + 0x164;
        if (!it->SetupImage(
                this,
                code,
                SBICMD_SETTINGS,
                TAB_GAME,
                r,
                "GAME_STATUSBAR_TABZ_GAMETAB_SETTINGS",
                -1,
                0
            )) {
            if (it) {
                delete it;
            }
            return 0;
        }
        m_tabLists[5].AddTail(it);
        m_tabSprite8 = static_cast<CSBI_MenuItem*>(it);

        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0x175;
        r.right = bx + 0x9f;
        r.bottom = by + 0x18c;
        if (!it->SetupImage(
                this,
                code,
                SBICMD_BOOTY_STATE,
                TAB_GAME,
                r,
                "GAME_STATUSBAR_TABZ_GAMETAB_HELP",
                -1,
                0
            )) {
            if (it) {
                delete it;
            }
            return 0;
        }
        m_tabLists[5].AddTail(it);
        m_tabSprite9 = static_cast<CSBI_MenuItem*>(it);
        if (g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER) {
            it->m_enabled = 0;
        }

        it = new CSBI_MenuItem;
        r.left = bx;
        r.top = by + 0x19d;
        r.right = bx + 0x9f;
        r.bottom = by + 0x1b4;
        if (!it->SetupImage(
                this,
                code,
                SBICMD_QUIT,
                TAB_GAME,
                r,
                "GAME_STATUSBAR_TABZ_GAMETAB_QUIT",
                -1,
                0
            )) {
            if (it) {
                delete it;
            }
            return 0;
        }
        m_tabLists[5].AddTail(it);
        m_tabSprite10 = static_cast<CSBI_MenuItem*>(it);

        it = new CSBI_ImageSet;
        r.left = bx + 0x22;
        r.top = by + 0x1be;
        r.right = bx + 0x7d;
        r.bottom = by + 0x1d6;
        if (!it->SetupImage(
                this,
                code,
                SBICMD_DESTRUCT,
                TAB_GAME,
                r,
                "GAME_STATUSBAR_TABZ_GAMETAB_DESTRUCT",
                IDX(m_modeState),
                0
            )) {
            if (it) {
                delete it;
            }
            return 0;
        }
        m_tabLists[5].AddTail(it);
        m_modeNotify = static_cast<CSBI_ImageSet*>(it);
        if (g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
            it->m_enabled = 0;
            m_modeState = DESTRUCT_FRAME_DISABLED;
            m_destructWarnActive = DESTRUCT_WARNING_INACTIVE;
            m_modeNotify->Notify(IDX(DESTRUCT_FRAME_DISABLED));
        }
        return 1;
    }

    if (g_gameReg->m_cmdGrid->m_phase == FINISH_STATE_VICTORY) {
        it = new CSBI_ImageSet;
        r.left = bx;
        r.top = by + 0xd7;
        r.right = bx + 0x9f;
        r.bottom = by + 0x118;
        if (!it->SetupImage(
                this,
                code,
                SBICMD_MISSION_STATUS,
                TAB_GAME,
                r,
                "GAME_STATUSBAR_TABZ_GAMETAB_MISSIONSTATUS",
                1,
                0
            )) {
            if (it) {
                delete it;
            }
            return 0;
        }
    } else {
        it = new CSBI_ImageSet;
        r.left = bx;
        r.top = by + 0xd7;
        r.right = bx + 0x9f;
        r.bottom = by + 0x118;
        if (!it->SetupImage(
                this,
                code,
                SBICMD_MISSION_STATUS,
                TAB_GAME,
                r,
                "GAME_STATUSBAR_TABZ_GAMETAB_MISSIONSTATUS",
                2,
                0
            )) {
            if (it) {
                delete it;
            }
            return 0;
        }
    }
    m_tabLists[5].AddTail(it);
    return 1;
}

// ??0CSBI_RectOnly: BuildGameMenu's construction set is complete; retail keeps
// 5 base-ctor calls where our cl flattens the chains (variable per-site inline
// depth, see docs/patterns/msvc5-variable-ctor-inline-depth.md), so nothing
// emits this COMDAT and the pin dangles.

RVA_COMPGEN(0x00101fa0, 0x1b, ??0CSBI_RectOnly@@QAE@XZ)

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
        if (cur) {
            delete cur;
        }
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
        RefreshState();
    }
    if (show && m_activeTab != TAB_GAME) {
        SetTabState(SBICMD_TAB_GAME, MENUITEM_SELECTED);
    }
    if (m_tabSprite5) {
        m_tabSprite5->ResolveFrame("GAME_STATUSBAR_TABZ_GAMETAB_RESUME", 1);
        Deactivate();
        m_tabSprite5->SetSubtype();
    }
    m_hitTestDisabled = 1;
}

RVA(0x00102200, 0x37)
void CStatusBarMgr::BuildGameTabPauseButton() {
    if (m_tabSprite5) {
        m_tabSprite5->ResolveFrame("GAME_STATUSBAR_TABZ_GAMETAB_PAUSE", 1);
        Deactivate();
        m_tabSprite5->SetSubtype();
    }
    m_hitTestDisabled = 0;
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

// @early-stop
// Retail keeps `mode` in ebx across the calls and reads the LeafCue fields as memory
// operands; we spill mode to the frame and hoist both fields into registers, which
// costs the extra reload before the final m_statFlags store.
RVA(0x00104e60, 0xed)
i32 CStatusBarMgr::LoadStatzTabToggleSprite(i32 idx, i32 value) {
    StatusSampleMode mode = static_cast<StatusSampleMode>(value);
    if (m_statFlags[idx] == mode) {
        return 1;
    }

    i32 slot = idx + 15 * g_curPlayer;
    if (g_gameReg->m_cmdGrid->m_grid[slot] == NULL) {
        return 0;
    }

    CSBI_SideTab* item = m_hitRects[idx];
    i32 one = 1;
    if (item) {
        item->m_sampleMode = mode;
        item->m_enabled = one;
        if (m_activeTab == TAB_STATZ) {
            m_statObj[idx]->SetDirectionAlt(m_position, one);
            CDDrawSubMgrLeafScan* h = g_gameReg->m_world->m_soundRegistry;
            if (h->m_emitGate == 0) {
                void* spr_ob = 0;
                h->m_cues.Lookup("GAME_STATZTABTOGGLE", spr_ob);
                LeafCue* spr = static_cast<LeafCue*>(spr_ob);
                if (spr) {

                    i32 gate = g_sndEnabled;
                    i32 item = g_sndCueTag;
                    if (gate != 0 && g_killCueClock - spr->m_lastPlayTime >= spr->m_replayDelay) {
                        spr->m_lastPlayTime = g_killCueClock;
                        spr->m_sound->ConfigureItem(item, 0, 0, 0);
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
        r->m_enabled = 0;
        if (m_activeTab == TAB_STATZ) {

            m_statObj[idx]->SetDirection(m_position, 1);
            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
            if (host->m_emitGate == 0) {
                void* found = 0;
                CMapStringToPtr* map = &host->m_cues;
                map->Lookup("GAME_STATZTABTOGGLE", found);
                if (found) {
                    i32 gate = g_sndEnabled;
                    i32 item = g_sndCueTag;
                    if (gate != 0) {
                        LeafCue* p = static_cast<LeafCue*>(found);
                        if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                            >= static_cast<u32>(p->m_replayDelay)) {
                            p->m_lastPlayTime = g_killCueClock;
                            p->m_sound->ConfigureItem(item, 0, 0, 0);
                        }
                    }
                }
            }
        }
    }
    m_statFlags[idx] = STATUS_SAMPLE_NONE;
    return 1;
}

RVA(0x00105280, 0x61)
i32 CStatusBarMgr::HitTest(i32 x, i32 y) {
    if (m_hitTestDisabled == 0) {
        for (i32 i = 0; i < 15; i++) {
            if (m_hitRects[i] && m_hitRects[i]->m_enabled) {
                CSBI_SideTab* p = m_hitRects[i];
                i32 hit = p->m_enabled ? CGameLevel::PointInRect(&p->m_rect14, x, y) : 0;
                if (hit) {
                    return i;
                }
            }
        }
    }
    return -1;
}

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
                CDDrawSubMgrLeafScan* h = g_gameReg->m_world->m_soundRegistry;
                if (h->m_emitGate == 0) {
                    void* spr_ob = 0;
                    h->m_cues.Lookup("GAME_COOKINGCOMPLETE", spr_ob);
                    LeafCue* spr = static_cast<LeafCue*>(spr_ob);
                    if (spr) {

                        i32 gate = g_sndEnabled;
                        i32 item = g_sndCueTag;
                        if (gate != 0
                            && g_killCueClock - spr->m_lastPlayTime >= spr->m_replayDelay) {
                            spr->m_lastPlayTime = g_killCueClock;
                            spr->m_sound->ConfigureItem(item, 0, 0, 0);
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
            m_gaugeNotify->SetSubtype();
            i32 fill = m_gauge;
            CSBI_WellGoo* sink = m_gaugeSink;
            sink->m_fillScale = fill;
            sink->SetSubtype();
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

// @early-stop
// cl CSEs the slot address as `this + idx*24` where retail forms `&m_slots[idx]`
// (`lea ebx,[esi+eax*8+0x220]`), so our four m_startTime/m_interval stores use
// +0x22c..+0x234 displacements instead of retail's +0..+0xc off one re-derived lea.
RVA(0x001055b0, 0x109)
i32 CStatusBarMgr::LoadGooCookingSprite(i32 idx) {
    CSbiSlot* sp = &m_slots[idx];
    if (sp->m_state != SLOT_ARMED) {
        return 0;
    }
    if (g_gameReg->m_gameMode == GAMEMODE_SINGLE && m_hlBusy == 0) {
        if (m_position == STATUSBAR_HIDDEN) {
            RefreshState();
        }
        if (m_activeTab != TAB_GRUNTZ) {
            SetTabState(SBICMD_TAB_GRUNTZ, MENUITEM_SELECTED);
        }
        Deactivate();
    }
    sp->m_state = SLOT_FILLING;

    m_slots[idx].m_interval = INT_MAX;
    m_slots[idx].m_startTimeLo = g_frameTime;
    m_slots[idx].m_startTimeHi = 0;
    if (m_activeTab == TAB_GRUNTZ && m_position != STATUSBAR_HIDDEN) {
        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
        if (host->m_emitGate == 0) {
            void* found = 0;
            CMapStringToPtr* map = &host->m_cues;
            map->Lookup("GAME_GOOCOOKING1", found);
            if (found) {
                i32 gate = g_sndEnabled;
                i32 item = g_sndCueTag;
                if (gate != 0) {
                    LeafCue* p = static_cast<LeafCue*>(found);
                    if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                        >= static_cast<u32>(p->m_replayDelay)) {
                        p->m_lastPlayTime = g_killCueClock;
                        p->m_sound->ConfigureItem(item, 0, 0, 0);
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

RVA(0x00105780, 0x1f)
void CStatusBarMgr::DrainGauge(i32 delta) {
    m_gaugeTarget = m_gauge - delta > SBI_GAUGE_EMPTY ? m_gauge - delta : SBI_GAUGE_EMPTY;
}

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
RVA(0x00105800, 0x9e)
i32 CStatusBarMgr::PlaceCursorTarget(i32 row, i32 commit) {
    i32 col = g_curPlayer;
    if (g_gameReg->m_cmdGrid->ResetCell(col, row, 0, 0) != 0) {

        CGrunt* entry = g_gameReg->m_cmdGrid->m_grid[row + col * TM_GRID_COLS];
        if (entry != NULL) {
            (static_cast<CPlay*>(g_gameReg->m_curState))
                ->ResetGoals(entry->m_object->m_screenX, entry->m_object->m_screenY);
            if (commit != 0) {
                CTriggerMgr* obj = g_gameReg->m_cmdGrid;
                if (obj->RecordListHas(col, row)) {
                    obj->m_recordPosition.m_x = col;
                    obj->m_recordPosition.m_y = row;
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
void CStatusBarMgr::RefreshAll() {
    UpdateGruntOvenStatusBar();
    TickGauge();
    UpdateRezConveyorStatusBar();
    LoadRezMachineConfig();
    LoadChipMachineConfig();
    UpdateChipGrinderStatusBar();
    UpdateDestructButtonStatusBar();
}

RVA(0x00105920, 0x47)
void CStatusBarMgr::Reset() {
    ResetSlots();
    m_gaugeTarget = SBI_GAUGE_EMPTY;
    m_gauge = SBI_GAUGE_EMPTY;
    ResetGroupA();
    UpdateRezMachineSnoozeStatusBar();
    InitTabRects();
    m_modeState = DESTRUCT_FRAME_IDLE;
    m_destructWarnActive = DESTRUCT_WARNING_INACTIVE;
}

// @early-stop
RVA(0x00105990, 0x3b4)
void CStatusBarMgr::UpdateRezConveyorStatusBar() {
    i32 count = 3;
    CSBI_ImageSet** notify = m_groupNotify;
    CSbiHlRow* ph = m_groupSlots;
    do {
        SbiHlRowState state = static_cast<SbiHlRowState>(ph->m_state);
        switch (state) {
            case HLROW_IDLE_CYCLE:
                if (++ph->m_counter > 9) {
                    ph->m_counter = 1;
                }
                break;
            case HLROW_RAMP_UP_LOW:
                if (static_cast<i64>(g_frameTime) - ph->m_last >= ph->m_interval) {
                    if (++ph->m_counter >= 0x12) {
                        ph->m_counter = 0x12;
                        ph->m_state = IDX(HLROW_HOLD_LOW);
                        ph->m_interval =
                            g_buteMgr.GetDwordDef("StatusBar", "ConveyorBeltHoldDelay", 0x1f4);
                        ph->m_last = static_cast<u32>(g_frameTime);
                        UpdateFallingItemStatusBar(
                            m_extraNotifyArg0,
                            m_itemRect.left + 0xc,
                            m_itemRect.top + 0xc
                        );
                    }
                }
                break;
            case HLROW_RAMP_DOWN_LOW:
                if (static_cast<i64>(g_frameTime) - ph->m_last >= ph->m_interval) {
                    if (--ph->m_counter < 0xa) {
                        ph->m_state = IDX(HLROW_OFF);
                        ph->m_counter = 1;
                    }
                }
                break;
            case HLROW_RAMP_UP_HIGH:
                if (static_cast<i64>(g_frameTime) - ph->m_last >= ph->m_interval) {
                    if (++ph->m_counter >= 0x18) {
                        ph->m_counter = 0x18;
                        ph->m_state = IDX(HLROW_HOLD_HIGH);
                        ph->m_interval =
                            g_buteMgr.GetDwordDef("StatusBar", "ConveyorBeltHoldInDelay", 0x1f4);
                        ph->m_last = static_cast<u32>(g_frameTime);
                        m_machinePhase = BELT_FALLING_OFF;
                        m_beltInterval =
                            g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
                        m_beltLast = static_cast<u32>(g_frameTime);
                    }
                }
                break;
            case HLROW_RAMP_DOWN_HIGH:
                if (static_cast<i64>(g_frameTime) - ph->m_last >= ph->m_interval) {
                    if (--ph->m_counter < 0x13) {
                        ph->m_state = IDX(HLROW_OFF);
                        ph->m_counter = 1;
                    }
                }
                break;
            case HLROW_HOLD_HIGH:
                if (static_cast<i64>(g_frameTime) - ph->m_last >= ph->m_interval) {
                    if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                        if (host->m_emitGate == 0) {
                            void* found = 0;
                            host->m_cues.Lookup("GAME_REZBELTRETURN", found);
                            if (found && g_sndEnabled != 0) {
                                i32 item = g_sndCueTag;
                                LeafCue* p = static_cast<LeafCue*>(found);
                                if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                                    >= static_cast<u32>(p->m_replayDelay)) {
                                    p->m_lastPlayTime = g_killCueClock;
                                    p->m_sound->ConfigureItem(item, 0, 0, 0);
                                }
                            }
                        }
                    }
                    ph->m_state = IDX(HLROW_RAMP_DOWN_HIGH);
                }
                break;
            case HLROW_HOLD_LOW:
                if (static_cast<i64>(g_frameTime) - ph->m_last >= ph->m_interval) {
                    if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                        if (host->m_emitGate == 0) {
                            void* found = 0;
                            host->m_cues.Lookup("GAME_REZBELTBACKUP", found);
                            if (found && g_sndEnabled != 0) {
                                i32 item = g_sndCueTag;
                                LeafCue* p = static_cast<LeafCue*>(found);
                                if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                                    >= static_cast<u32>(p->m_replayDelay)) {
                                    p->m_lastPlayTime = g_killCueClock;
                                    p->m_sound->ConfigureItem(item, 0, 0, 0);
                                }
                            }
                        }
                    }
                    ph->m_state = IDX(HLROW_RAMP_DOWN_LOW);
                }
                break;
        }
        if (*notify) {
            (*notify)->Notify(ph->m_counter);
        }
        notify++;
        ph++;
    } while (--count);
}

// @early-stop
RVA(0x00105e40, 0x63c)
void CStatusBarMgr::LoadRezMachineConfig() {
    CSbiHlRow* pA = &m_machineB;
    CSbiHlRow* pB = &m_machineA;
    switch (static_cast<SbiMachineState>(pA->m_state)) {
        case MACHINE_RIGHT_RUNNING:
            if (static_cast<i64>(g_frameTime) - pA->m_last >= pA->m_interval) {
                if (++pA->m_counter > 0x34) {
                    SetHudRectB(
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
                    SetHudRectB(0x2b, MACHINE_STOPPED, INT_MAX);
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
                    SetHudRectA(
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
                    SetHudRectA(
                        0x14,
                        MACHINE_TURNING_WHEEL,
                        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineTurningWheelDelay", 0x64)
                    );
                    SetHudRectB(
                        0x2b,
                        MACHINE_RIGHT_RUNNING,
                        g_buteMgr.GetDwordDef("StatusBar", "RightMachineRunningDelay", 0x7d)
                    );
                    CSbiHlRow* s = m_groupSlots;
                    for (i32 i = 0; i < 3; i++) {
                        s->m_state = IDX(HLROW_IDLE_CYCLE);
                        s->m_value = 1;
                        s++;
                    }
                    m_machinePhase = BELT_IN_MACHINE;
                    m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "NextItemDelay", 0x64);
                    m_beltLast = static_cast<u32>(g_frameTime);
                    if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                        if (host->m_emitGate == 0) {
                            void* found = 0;
                            host->m_cues.Lookup("GAME_REZMACHINE", found);
                            if (found && g_sndEnabled != 0) {
                                i32 item = g_sndCueTag;
                                LeafCue* p = static_cast<LeafCue*>(found);
                                if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                                    >= static_cast<u32>(p->m_replayDelay)) {
                                    p->m_lastPlayTime = g_killCueClock;
                                    p->m_sound->ConfigureItem(item, 0, 0, 0);
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
                    SetHudRectA(
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
                    CSbiHlRow* g = m_groupSlots;
                    i32 col;
                    PickupType which = static_cast<PickupType>(m_extraNotifyArg0);
                    if (which >= PICKUP_BRICKZ_FIRST) {
                        col = 2;
                    } else {
                        col = (which >= PICKUP_TOYZ_FIRST) ? 1 : 0;
                    }
                    i32 found = 0;
                    i32 r = 3;
                    while (found == 0) {
                        if (r < 0) {
                            break;
                        }
                        if (m_hlGrid[col * 4 + r].m_state == IDX(HLROW_OFF)) {
                            found = 1;
                        } else {
                            r--;
                        }
                    }
                    if (found) {
                        g[col].m_state = IDX(HLROW_RAMP_UP_HIGH);
                        g[col].m_counter = 0x13;
                        if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                            if (host->m_emitGate == 0) {
                                void* fnd = 0;
                                host->m_cues.Lookup("GAME_REZBELTRETRACT", fnd);
                                if (fnd && g_sndEnabled != 0) {
                                    i32 item = g_sndCueTag;
                                    LeafCue* p = static_cast<LeafCue*>(fnd);
                                    if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                                        >= static_cast<u32>(p->m_replayDelay)) {
                                        p->m_lastPlayTime = g_killCueClock;
                                        p->m_sound->ConfigureItem(item, 0, 0, 0);
                                    }
                                }
                            }
                        }
                    } else {
                        g[col].m_state = IDX(HLROW_RAMP_UP_LOW);
                        g[col].m_counter = 0xa;
                        if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                            if (host->m_emitGate == 0) {
                                void* fnd = 0;
                                host->m_cues.Lookup("GAME_REZBELTDROP", fnd);
                                if (fnd && g_sndEnabled != 0) {
                                    i32 item = g_sndCueTag;
                                    LeafCue* p = static_cast<LeafCue*>(fnd);
                                    if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                                        >= static_cast<u32>(p->m_replayDelay)) {
                                        p->m_lastPlayTime = g_killCueClock;
                                        p->m_sound->ConfigureItem(item, 0, 0, 0);
                                    }
                                }
                            }
                        }
                    }
                    g[0].m_interval = g_buteMgr.GetDwordDef("StatusBar", "ConveyorBeltDelay", 0x64);
                    g[0].m_last = static_cast<u32>(g_frameTime);
                    if (pB->m_counter > 0x2a) {
                        SetHudRectA(
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
            }
            break;
    }

    if (m_machineDisplay) {
        m_machineDisplay->SetFrames(pB->m_counter, pA->m_counter);
    }
}

RVA(0x00106610, 0x3b)
void CStatusBarMgr::ResetGroupA() {
    for (i32 i = 0; i < 3; i++) {
        m_groupSlots[i].m_state = IDX(HLROW_OFF);
        m_groupSlots[i].m_value = 1;
        if (m_groupNotify[i]) {
            m_groupNotify[i]->Notify(-1);
        }
    }
}

RVA(0x00106660, 0x68)
void CStatusBarMgr::UpdateRezMachineSnoozeStatusBar() {
    SetHudRectA(
        1,
        MACHINE_SNOOZING,
        g_buteMgr.GetDwordDef("StatusBar", "LeftMachineSnoozingDelay", 100)
    );
    SetHudRectB(0x2b, MACHINE_STOPPED, INT_MAX);
    if (m_machineDisplay) {
        m_machineDisplay->SetFrames(m_machineA.m_counter, m_machineB.m_counter);
    }
    m_rezActive = 0;
    m_rezTick = 0;
}

RVA(0x001066f0, 0x3b)
void CStatusBarMgr::SetHudRectA(i32 y0, SbiMachineState x0, i32 z) {
    volatile CSbiHlRow& r = m_machineA;
    r.m_counter = y0;
    r.m_state = IDX(x0);
    r.m_interval = static_cast<u32>(z);
    r.m_last = g_frameTime;
}

RVA(0x00106740, 0x3b)
void CStatusBarMgr::SetHudRectB(i32 y0, SbiMachineState x0, i32 z) {
    volatile CSbiHlRow& r = m_machineB;
    r.m_counter = y0;
    r.m_state = IDX(x0);
    r.m_interval = static_cast<u32>(z);
    r.m_last = g_frameTime;
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
            CSbiHlRow* cell = &m_hlGrid[row + group * 4];
            if (cell->m_state == IDX(HLROW_IDLE_CYCLE)) {
                m_hlGrid[row + group * 4 + 1].m_state = IDX(HLROW_IDLE_CYCLE);
                cell[1].m_value = cell->m_value;
                cell->m_state = IDX(HLROW_OFF);
                cell->m_value = 0;
            }
        }
    } else {
        m_hlGrid[IDX(m_pendingHlRow) + group * 4].m_value = key;
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
    m_hlGrid[idx].m_state = IDX(HLROW_OFF);
    m_hlGrid[idx].m_value = 0;
    NotifyAllSlots();
}

RVA(0x00106a00, 0xbf)
void CStatusBarMgr::NotifyAllSlots() {
    if (m_notify0) {
        m_notify0->SetSubtype();
    }
    if (m_notify2) {
        m_notify2->SetSubtype();
    }
    if (m_notify3) {
        m_notify3->SetSubtype();
    }
    if (m_extraNotify0 && m_extraNotifyArg0) {
        m_extraNotify0->Notify(m_extraNotifyArg0);
    }

    CSBI_ImageSet** p = &m_hlNotify[4];
    i32* h = &m_hlGrid[4].m_value;
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
        m_notify1->SetSubtype();
    }
    if (m_extraNotify1) {
        m_extraNotify1->Notify(m_extraNotifyArg1);
    }
}

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
    if (m_hlGrid[idx].m_state != IDX(HLROW_OFF)) {
        return 0;
    }
    m_hlGrid[idx].m_value = handle;
    m_hlGrid[idx].m_state = IDX(HLROW_IDLE_CYCLE);
    NotifyAllSlots();
    return 1;
}

// @early-stop
RVA(0x00106bb0, 0x7d8)
void CStatusBarMgr::LoadChipMachineConfig() {
    i32 refreshFlag = 0;
    i32 rectFlag = 0;
    switch (m_machinePhase) {
        case BELT_IN_MACHINE:
            if (static_cast<i64>(g_frameTime) - m_beltLast >= m_beltInterval) {
                m_itemRect.left += g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_itemRect.right += g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "NextItemDelay", 0x64);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            if (m_itemRect.left >= 0x6d) {
                m_itemRect.left = 0x6d;
                m_itemRect.right = 0x84;
                m_machinePhase = BELT_SPEWING;
                m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "NextItemInMachineTime", 0x7d0);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            refreshFlag = 1;
            break;
        case BELT_SPEWING:
            if (static_cast<i64>(g_frameTime) - m_beltLast >= m_beltInterval) {
                SetHudRectB(
                    0x35,
                    MACHINE_RIGHT_SPEWING,
                    g_buteMgr.GetDwordDef("StatusBar", "RightMachineSpewingDelay", 0x7d)
                );
                m_machinePhase = BELT_DROP_START;
                m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "NextItemWaitTime", 0x1f4);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            break;
        case BELT_DROP_START:
            if (static_cast<i64>(g_frameTime) - m_beltLast >= m_beltInterval) {
                m_machinePhase = BELT_FALLING;
                if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                    if (host->m_emitGate == 0) {
                        void* found = 0;
                        host->m_cues.Lookup("GAME_CHIPFALLOUT", found);
                        LeafCue* p = static_cast<LeafCue*>(found);
                        if (p) {
                            i32 gate = g_sndEnabled;
                            i32 item = g_sndCueTag;
                            if (gate != 0
                                && g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                                       >= static_cast<u32>(p->m_replayDelay)) {
                                p->m_lastPlayTime = g_killCueClock;
                                p->m_sound->ConfigureItem(item, 0, 0, 0);
                            }
                        }
                    }
                }
                m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            break;
        case BELT_FALLING:
            if (static_cast<i64>(g_frameTime) - m_beltLast >= m_beltInterval) {
                m_itemRect.top += g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 2);
                m_itemRect.bottom += g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 2);
                m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            if (m_itemRect.bottom >= 0x11c) {
                m_itemRect.bottom = 0x11c;
                m_itemRect.top = 0x104;
                rectFlag = 1;
                if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                    if (host->m_emitGate == 0) {
                        void* found = 0;
                        host->m_cues.Lookup("GAME_CHIPLAND", found);
                        LeafCue* p = static_cast<LeafCue*>(found);
                        if (p) {
                            i32 gate = g_sndEnabled;
                            i32 item = g_sndCueTag;
                            if (gate != 0
                                && g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                                       >= static_cast<u32>(p->m_replayDelay)) {
                                p->m_lastPlayTime = g_killCueClock;
                                p->m_sound->ConfigureItem(item, 0, 0, 0);
                            }
                        }
                    }
                }
                m_machinePhase = BELT_TRAVELLING;
                m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "NextItemDelay", 0x64);
                m_beltLast = static_cast<u32>(g_frameTime);
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
            if (static_cast<i64>(g_frameTime) - m_beltLast >= m_beltInterval) {
                m_itemRect.left -= g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_itemRect.right -= g_buteMgr.GetIntDef("StatusBar", "NextItemSpeed", 2);
                m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "NextItemDelay", 0x64);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            if (m_itemRect.left <= m_itemBaseX) {
                m_itemRect.left = m_itemBaseX;
                m_itemRect.right = m_itemBaseX + 0x17;
                rectFlag = 1;
                ResetGroupA();
                SetHudRectA(
                    0x1e,
                    MACHINE_LEVER,
                    g_buteMgr.GetDwordDef("StatusBar", "LeftMachineLeverDelay", 0x64)
                );
                m_machinePhase = BELT_IDLE;
            }
            refreshFlag = 1;
            break;
        case BELT_FALLING_OFF: {
            if (static_cast<i64>(g_frameTime) - m_beltLast >= m_beltInterval) {
                m_itemRect.top += g_buteMgr.GetIntDef("StatusBar", "FallingItemSpeed", 2);
                // RETAIL'S TYPO, kept: the shipped EXE's string table carries both
                // "(FallingItemSpeed" and "FallingItemSpeed" as separate strings, so
                // this lookup always misses and takes the default. Correcting it would
                // drop a string from .rdata and stop matching.
                m_itemRect.bottom += g_buteMgr.GetIntDef("StatusBar", "(FallingItemSpeed", 2);
                m_beltInterval = g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
                m_beltLast = static_cast<u32>(g_frameTime);
            }
            i32 col;
            PickupType item2 = static_cast<PickupType>(m_extraNotifyArg0);
            if (item2 >= PICKUP_BRICKZ_FIRST) {
                col = 2;
            } else {
                col = (item2 >= PICKUP_TOYZ_FIRST) ? 1 : 0;
            }
            i32 row = 3;
            CSbiHlRow* cell = &m_hlGrid[col * 4 + row];
            while (cell->m_state == IDX(HLROW_IDLE_CYCLE)) {
                row--;
                cell--;
                if (row < 0) {
                    break;
                }
            }
            if (m_itemRect.top >= row * 0x20 + 0x13e) {
                if (m_activeTab == TAB_RESOURCE && m_position != STATUSBAR_HIDDEN) {
                    CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
                    if (host->m_emitGate == 0) {
                        void* found = 0;
                        host->m_cues.Lookup("GAME_CHIPLAND", found);
                        LeafCue* p = static_cast<LeafCue*>(found);
                        if (p) {
                            i32 gate = g_sndEnabled;
                            i32 item = g_sndCueTag;
                            if (gate != 0
                                && g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                                       >= static_cast<u32>(p->m_replayDelay)) {
                                p->m_lastPlayTime = g_killCueClock;
                                p->m_sound->ConfigureItem(item, 0, 0, 0);
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
            w->m_rect14.left = m_itemRect.left + m_rect10.left;
            w->m_rect14.top = m_itemRect.top + m_rect10.top;
            w->m_rect14.right = m_itemRect.right + m_rect10.left;
            w->m_rect14.bottom = m_itemRect.bottom + m_rect10.top;
        }
        if (refreshFlag) {
            NotifyAllSlots();
        }
    }
}

// @early-stop
// Retail reserves the 16-byte home of the `RECT rc` local (`sub esp,0x10`) and spills
// rc.left through it; cl scalar-replaces the whole struct for us and needs no frame.
// A shared rc for both m_fallRect and m_rect14, `RECT rc = SbGeom(...)` and a
// function-scope declaration were all measured and none allocate the home.
RVA(0x00107590, 0xc4)
i32 CStatusBarMgr::UpdateFallingItemStatusBar(i32 a1, i32 a2, i32 a3) {
    m_extraNotifyArg1 = a1;
    m_fallActive = FALLING_ITEM_DESCENDING;
    m_fallDelay = g_buteMgr.GetDwordDef("StatusBar", "FallingItemDelay", 0x32);
    m_fallLast = static_cast<u32>(g_frameTime);
    CSBI_ImageSet* n = m_extraNotify1;
    i32 l = a2 - 0xc;
    i32 t = a3 - 0xc;
    i32 rr = a2 + 0xc;
    i32 b = a3 + 0xc;
    m_fallRect.left = l;
    m_fallRect.top = t;
    m_fallRect.right = rr;
    m_fallRect.bottom = b;
    if (n) {

        RECT rc;
        i32 x = m_rect10.left;
        rc.left = l + x;
        rc.right = x + rr;
        i32 y = m_rect10.top;
        rc.top = t + y;
        rc.bottom = y + b;
        n->m_rect14 = rc;
    }
    NotifyAllSlots();
    return 1;
}

// @early-stop
// `stepped` gets a second frame slot here (`sub esp,0x8` vs retail's one-dword
// `push ecx`) plus a reload after ConfigureItem; retail constant-propagates it per
// path and keeps only the Lookup out-param in the frame.
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
                    CDDrawSubMgrLeafScan* h = g_gameReg->m_world->m_soundRegistry;
                    if (h->m_emitGate == 0) {
                        void* spr_ob = 0;
                        h->m_cues.Lookup("GAME_REZGRINDING", spr_ob);
                        LeafCue* spr = static_cast<LeafCue*>(spr_ob);
                        if (spr) {

                            i32 gate = g_sndEnabled;
                            i32 item = g_sndCueTag;
                            if (gate != 0
                                && g_killCueClock - spr->m_lastPlayTime >= spr->m_replayDelay) {
                                spr->m_lastPlayTime = g_killCueClock;
                                spr->m_sound->ConfigureItem(item, 0, 0, 0);
                            }
                        }
                    }
                }
                m_fallActive = FALLING_ITEM_GRINDING;
            }
            delay = g_buteMgr.GetDwordDef("StatusBar", "FallingItemShredderDelay", 0x64);
            speed = g_buteMgr.GetIntDef("StatusBar", "FallingItemShredderSpeed", 2);
        }

        i64 d = static_cast<i64>(g_frameTime) - m_fallLast;
        if (d >= m_fallDelay) {
            i32 newLo = m_fallRect.top + speed;
            m_fallRect.top = newLo;
            i32 newHi = m_fallRect.bottom + speed;
            m_fallRect.bottom = newHi;
            CSBI_ImageSet* w = m_extraNotify1;
            if (w) {
                i32 sx = m_rect10.left;
                i32 sy = m_rect10.top;
                w->m_rect14.left = m_fallRect.left + sx;
                w->m_rect14.top = sy + newLo;
                w->m_rect14.right = m_fallRect.right + sx;
                w->m_rect14.bottom = sy + newHi;
            }
            m_fallDelay = delay;
            m_fallLast = g_frameTime;
            stepped = 1;
        }
    }

    if (m_extraNotify1 != NULL && stepped) {
        NotifyAllSlots();
    }
}

RVA(0x00107920, 0xb7)
i32 CStatusBarMgr::SetFallRect(i32 x, i32 y, i32 item) {
    if (m_pendingHlRow == STATUS_HL_ROW_NONE) {
        return 0;
    }
    CStatusBarItem* r = HitTestRects(x, y);
    if (r == NULL) {
        return 0;
    }
    SbiCommandId cmd = r->m_cmd;
    if (cmd != SBICMD_CONVEYOR_TOP && cmd != SBICMD_CONVEYOR_BOTTOM) {
        return 0;
    }

    i32 cx = x;
    RECT rc = r->m_rect14;
    i32 lo = rc.left + 0x1b;
    i32 xHi = rc.right;
    if (x < lo) {
        cx = lo;
    } else if (x > xHi - 0x1a) {
        cx = xHi - 0x1a;
    }
    i32 localX = cx - m_rect10.left;
    i32 localY = 0x1b3 - m_rect10.top;
    UpdateFallingItemStatusBar(item, localX, localY);
    EnterHlRow(1, item);
    return 1;
}

RVA(0x00107a10, 0x62)
i32 CStatusBarMgr::UpdateRezMachineWakeStatusBar() {
    if (m_rezActive == 0) {
        if (m_extraNotifyArg0 == 0) {
            return 0;
        }
        SetHudRectA(
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
        LoadStatzTabToggleSprite(idx, IDX(STATUS_SAMPLE_HEALTH));
    }
}

// @early-stop
RVA(0x00107ae0, 0x1aa)
void CStatusBarMgr::LoadMultiplayerBattlezConfig(i32) {
    BuildGameTabPauseButton();
    if (m_position == STATUSBAR_HIDDEN) {
        RefreshState();
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
    } else if (mode == GAMEMODE_REPLAY) {
        for (i32 i = 0; i < g_buteMgr.GetIntDef("Battlez", "StartingGruntz", 0); i++) {
            m_slots[i].m_value = kSlotCommitLevel;
            m_slots[i].m_state = SLOT_READY;
        }
    }

    for (i32 j = 0; j < m_ptrPool.GetSize(); j++) {
        void* p = m_ptrPool.GetData()[j];
        if (p) {
            CoordPoolNode* node = g_coordPool.NodeOf(p);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_ptrPool.SetSize(0, -1);
    m_reserved2b0 = 0;
    m_reserved2b8 = 0;
    m_hlBusy = 0;
    if (m_retabNotify) {
        free(m_retabNotify);
        m_retabNotify = NULL;
    }
    ExitMode();
    m_observerTabAvailable = 0;
    m_modeArmed = 0;
    TryActivate();
}
// @early-stop
RVA(0x00107d00, 0x591)
i32 CStatusBarMgr::StartChipMachineCycle() {
    PickupType result;
    if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
        if (m_ptrPool.GetSize() > 0) {
            void* p = m_ptrPool.GetData()[0];
            result = static_cast<PickupType>(*static_cast<i32*>(p));
            CoordPoolNode* node = g_coordPool.NodeOf(p);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
            m_ptrPool.RemoveAt(0, 1);
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
    m_extraNotifyArg1 = IDX(result);
    m_machinePhase = BELT_IDLE;
    SetRect(&m_itemRect, 0x49, 0xd7, 0x61, 0xef);
    if (m_extraNotify0) {
        i32 x = m_rect10.left;
        i32 y = m_rect10.top;
        m_extraNotify0->m_rect14.left = m_itemRect.left + x;
        m_extraNotify0->m_rect14.top = m_itemRect.top + y;
        m_extraNotify0->m_rect14.right = m_itemRect.right + x;
        m_extraNotify0->m_rect14.bottom = m_itemRect.bottom + y;
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
i32 CStatusBarMgr::InsertPtr(i32 a, i32 b) {
    CoordPoolNode* head = g_coordPool.m_freeHead;
    Coord* node = 0;
    if (head->m_next != NULL) {
        node = &head->m_coord;
        node->m_x = a;
        node->m_y = b;
        g_coordPool.m_freeHead = g_coordPool.m_freeHead->m_next;
    }
    i32 n = m_ptrPool.GetSize();
    i32 i = 0;
    if (i < n) {
        void** t = m_ptrPool.GetData();
        while (i < n) {
            Coord* e = static_cast<Coord*>(*t);
            if (e != NULL && b < e->m_y) {
                goto insert;
            }
            i++;
            t++;
        }
    }
    m_ptrPool.Add(node);
    return 1;
insert:
    m_ptrPool.InsertAt(i, node, 1);
    return 1;
}

// Retail serializes every {i64 last; i64 interval} pair through one inlined
// helper: the base address is hoisted into a register before the mode test and
// the second field reached with `add reg,8`.
static inline void SyncClockPair(CFileMemBase* s, SerialMode op, i64* pair) {
    if (op != SERIAL_SAVE) {
        if (op == SERIAL_LOAD) {
            s->Read(pair, sizeof(*pair));
            s->Read(pair + 1, sizeof(*pair));
        }
    } else {
        s->Write(pair, sizeof(*pair));
        s->Write(pair + 1, sizeof(*pair));
    }
}

// @early-stop
// Retail never enregisters p4/p5: every one of the ~30 SerializeFields sites reloads
// both from their parameter homes into eax, which frees ebp for the loop counters.
// We cache p5 in ebp and spill each counter, costing 2 instructions per site - the
// whole 42-instruction shortfall; every other block is byte-identical. Frames are
// identical (sub esp,0x8 both), and the member-offset SET is verified identical to
// retail's, including the duplicated 0x1f0 (m_tabSprite10 really is serialized twice).
RVA(0x001084d0, 0x96c)
i32 CStatusBarMgr::Sync(CFileMemBase* s, SerialMode op, LogicTypeId p4, i32 p5) {
    if (s == NULL) {
        return 0;
    }
    switch (op) {
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
                RefreshA();
                DockStatusBarRight();
            }
            break;
    }

    if (m_retabNotify != NULL) {
        i32 tmp = 1;
        if (op == SERIAL_SAVE) {
            s->Write(&tmp, sizeof(tmp));
        }
    } else {
        i32 tmp = 0;
        if (op == SERIAL_SAVE) {
            s->Write(&tmp, sizeof(tmp));
        } else if (op == SERIAL_LOAD) {
            s->Read(&tmp, sizeof(tmp));
            if (tmp != 0) {
                CWarpStoneFly* c = new CWarpStoneFly();
                m_retabNotify = c;
                c->m_owner = this;
            }
        }
    }

    if (m_retabNotify != NULL) {
        if (m_retabNotify->Sync(s, op, p4, p5) == 0) {
            return 0;
        }
    }

    SyncClockPair(s, op, &m_beltLast);
    SyncClockPair(s, op, &m_fallLast);
    SyncClockPair(s, op, &m_machineB.m_last);
    SyncClockPair(s, op, &m_machineA.m_last);
    SyncClockPair(s, op, &m_destructWarnLast);

    CSbiSlot* p = m_slots;
    i32 n = 5;
    do {
        SyncClockPair(s, op, &p->m_startTime);
        p++;
        n--;
    } while (n != 0);

    n = 3;
    CSbiHlRow* r = m_groupSlots;
    do {
        SyncClockPair(s, op, &r->m_last);
        r++;
        n--;
    } while (n != 0);

    i32 outer = 3;
    CSbiHlRow* g = m_hlGrid;
    do {
        n = 4;
        do {
            SyncClockPair(s, op, &g->m_last);
            g++;
            n--;
        } while (n != 0);
        outer--;
    } while (outer != 0);

    SyncClockPair(s, op, &m_reserved2a0);
    SyncClockPair(s, op, &m_reserved2b0);
    if (op == SERIAL_LOAD && m_position != STATUSBAR_HIDDEN) {
        BuildStatusBarTabs();
    }

#define SER(field)                                                                                 \
    if (field) {                                                                                   \
        if ((field)->SerializeFields(s, op, p4, p5) == 0)                                          \
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
        CSBI_ImageSet** q = m_groupNotify;
        do {
            SER(*q)
            i++;
            q++;
        } while (i < 3);
    }
    {
        i32 row = 0;
        CSBI_ImageSet** base = m_hlNotify;
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
    SER(m_modeNotify)
#undef SER

    Deactivate();
    return 1;
}

// @early-stop
// The do-while counter's stack slot (0x18 vs retail's 0x10). Declaring it beside
// tmp, before tmp, or before nb are all byte-identical - the slot is not decl-order.
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

    i32 tmp = 0;

    if (m_barSprite) {
        tmp = m_barSprite->m_objectId;
    }
    s->Write(&tmp, sizeof(tmp));

    s->Write(&m_rect10.left, 0x10);
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
    s->Write(&m_hitTestDisabled, sizeof(m_hitTestDisabled));
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
    s->Write(&m_toggleActive, sizeof(m_toggleActive));
    s->Write(&m_toggleHandle, sizeof(m_toggleHandle));
    s->Write(&m_machinePhase, sizeof(m_machinePhase));
    s->Write(&m_extraNotifyArg0, sizeof(m_extraNotifyArg0));
    s->Write(&m_fallActive, sizeof(m_fallActive));
    s->Write(&m_extraNotifyArg1, sizeof(m_extraNotifyArg1));
    s->Write(&m_machineB, 4);
    s->Write(&m_machineB.m_value, sizeof(m_machineB.m_value));
    s->Write(&m_machineA, 4);
    s->Write(&m_machineA.m_value, sizeof(m_machineA.m_value));
    s->Write(&m_destructWarnActive, sizeof(m_destructWarnActive));
    s->Write(&m_modeState, sizeof(m_modeState));
    s->Write(&m_modeArmed, sizeof(m_modeArmed));
    s->Write(&m_observerTabAvailable, sizeof(m_observerTabAvailable));

    for (i32 j = 0; j < 5; j++) {
        s->Write(&m_slots[j].m_state, sizeof(m_slots[j].m_state));
        s->Write(&m_slots[j].m_value, sizeof(m_slots[j].m_value));
    }
    for (i32 k = 0; k < 3; k++) {
        s->Write(&m_groupSlots[k].m_state, sizeof(m_groupSlots[k].m_state));
        s->Write(&m_groupSlots[k].m_value, sizeof(m_groupSlots[k].m_value));
    }
    CSbiHlRow* nb = m_hlGrid;

    i32 cnt = 3;
    do {
        for (i32 m = 0; m < 4; m++) {
            s->Write(&nb[m].m_state, sizeof(nb[m].m_state));
            s->Write(&nb[m].m_value, sizeof(nb[m].m_value));
        }
        nb += 4;
    } while (--cnt);

    cnt = m_ptrPool.GetSize();
    s->Write(&cnt, sizeof(cnt));
    for (u32 n = 0; n < static_cast<u32>(cnt); n++) {
        s->Write(m_ptrPool.GetData()[n], 8);
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
    m_destructButton = NULL;
    ResetWidgets(0);

    s->Read(this, 4);
    s->Read(&m_restorePosition, sizeof(m_restorePosition));

    g_serialCounter++;

    i32 seq;
    s->Read(&seq, sizeof(seq));

    CGameObject* obj = NULL;
    CWwdGameObjectA* m8;
    if (MapLookupById(gm->m_childGroup->m_map48, seq, obj) == 0) {
        m8 = NULL;
    } else if (obj == NULL) {
        m8 = NULL;
    } else {
        m8 = (obj->GetClassId() == CLASSID_SERIALREF) ? static_cast<CWwdGameObjectA*>(obj) : NULL;
    }
    m_barSprite = m8;
    if (m8 == NULL && seq != 0) {
        return 0;
    }

    s->Read(&m_rect10.left, 0x10);
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
    s->Read(&m_hitTestDisabled, sizeof(m_hitTestDisabled));
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
    s->Read(&m_toggleActive, sizeof(m_toggleActive));
    s->Read(&m_toggleHandle, sizeof(m_toggleHandle));
    s->Read(&m_machinePhase, sizeof(m_machinePhase));
    s->Read(&m_extraNotifyArg0, sizeof(m_extraNotifyArg0));
    s->Read(&m_fallActive, sizeof(m_fallActive));
    s->Read(&m_extraNotifyArg1, sizeof(m_extraNotifyArg1));
    s->Read(&m_machineB, 4);
    s->Read(&m_machineB.m_value, sizeof(m_machineB.m_value));
    s->Read(&m_machineA, 4);
    s->Read(&m_machineA.m_value, sizeof(m_machineA.m_value));
    s->Read(&m_destructWarnActive, sizeof(m_destructWarnActive));
    s->Read(&m_modeState, sizeof(m_modeState));
    s->Read(&m_modeArmed, sizeof(m_modeArmed));
    s->Read(&m_observerTabAvailable, sizeof(m_observerTabAvailable));

    for (i32 j = 0; j < 5; j++) {
        s->Read(&m_slots[j].m_state, sizeof(m_slots[j].m_state));
        s->Read(&m_slots[j].m_value, sizeof(m_slots[j].m_value));
    }
    for (i32 k = 0; k < 3; k++) {
        s->Read(&m_groupSlots[k].m_state, sizeof(m_groupSlots[k].m_state));
        s->Read(&m_groupSlots[k].m_value, sizeof(m_groupSlots[k].m_value));
    }
    CSbiHlRow* nb = m_hlGrid;
    seq = 3;
    do {
        for (i32 m = 0; m < 4; m++) {
            s->Read(&nb[m].m_state, sizeof(nb[m].m_state));
            s->Read(&nb[m].m_value, sizeof(nb[m].m_value));
        }
        nb += 4;
    } while (--seq);

    for (i32 t = 0; t < m_ptrPool.GetSize(); t++) {
        void* pp = m_ptrPool.GetData()[t];
        if (pp) {
            CoordPoolNode* node = g_coordPool.NodeOf(pp);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_ptrPool.SetSize(0, -1);

    i32 cnt;
    s->Read(&cnt, sizeof(cnt));
    m_ptrPool.SetSize(cnt, -1);
    for (u32 n = 0; n < static_cast<u32>(cnt); n++) {
        CoordPoolNode* head = g_coordPool.m_freeHead;
        void* node = 0;
        if (head->m_next != NULL) {
            node = &head->m_coord;
            g_coordPool.m_freeHead = head->m_next;
        }
        s->Read(node, 8);
        m_ptrPool.GetData()[n] = node;
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
i32 CStatusBarMgr::EnsureSub(i32 a, i32 b, WarpStoneFragment fragment) {
    if (m_retabNotify) {
        return 0;
    }
    CWarpStoneFly* o = new CWarpStoneFly();
    m_retabNotify = o;
    if (o == NULL) {
        return 0;
    }
    return o->Init(this, a, b, fragment);
}

// @early-stop
// frame 0x18 vs retail's 0x14: retail homes one of the two delta scalars in the dead
// `fragment` parameter's slot; cl finds only the `owner` param home (both put the
// Lookup out-param there) and gives both deltas real slots. No cl 5.0 flag moves it -
// /Oa /Ow /Ox /Ob2 /Og /Gy /Oi- /Ot /G4 /G5 /Gf /GF /Op /Gd all leave `sub esp,0x18`.
RVA(0x00109bd0, 0x1b5)
i32 CWarpStoneFly::Init(void* owner, i32 srcX, i32 srcY, WarpStoneFragment fragment) {
    m_owner = static_cast<CStatusBarMgr*>(owner);

    CObject* spr_ob = 0;
    i32 n = IDX(fragment) + 1;
    g_gameReg->m_world->m_imageRegistry->m_10map.Lookup("GAME_STATUSBAR_TABZ_GAMETAB_WARP", spr_ob);
    CDDrawWorker* spr = static_cast<CDDrawWorker*>(spr_ob);
    CImage* frame = (spr && n >= spr->m_minIndex && n <= spr->m_maxIndex)
                        ? static_cast<CImage*>(spr->m_items.GetAt(n))
                        : 0;
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
    i32 tx = base->m_rect10.left + cx;
    m_targetX = tx;
    i32 ty = base->m_rect10.top + dy;
    m_targetY = ty;

    i32 deltaX = tx - srcX;
    i32 dyv = ty - srcY;
    i32 dist2 = deltaX * deltaX + dyv * dyv;
    double dist = sqrt(static_cast<double>(dist2));
    u32 flyTime = g_buteMgr.GetDwordDef("WarpStone", "FlyTime", 0x5dc);

    m_velocityScale = dist / static_cast<double>(flyTime);
    m_xDirection = static_cast<double>(deltaX) / dist;
    m_yDirection = static_cast<double>(dyv) / dist;

    CDDrawSubMgrLeafScan* h = g_gameReg->m_world->m_soundRegistry;
    if (h->m_emitGate == 0) {
        void* fly_ob = 0;
        h->m_cues.Lookup("GAME_WARPSTONEFLY", fly_ob);
        LeafCue* fly = static_cast<LeafCue*>(fly_ob);
        if (fly) {

            i32 gate = g_sndEnabled;
            i32 item = g_sndCueTag;
            if (gate != 0 && g_killCueClock - fly->m_lastPlayTime >= fly->m_replayDelay) {
                fly->m_lastPlayTime = g_killCueClock;
                fly->m_sound->ConfigureItem(item, 0, 0, 0);
            }
        }
    }

    m_currentX = static_cast<double>(srcX);
    m_currentY = static_cast<double>(srcY);
    return 1;
}

RVA(0x0010b210, 0xc5)
void CStatusBarMgr::ExitMode() {
    if (m_toggleActive == 0) {
        return;
    }
    POSITION n = m_tabLists[6].GetHeadPosition();
    while (n) {
        CStatusBarItem* cur = static_cast<CStatusBarItem*>(m_tabLists[6].GetNext(n));
        if (cur) {
            delete cur;
        }
    }
    m_tabLists[6].RemoveAll();
    i32 handle = m_toggleHandle;
    m_tabSprite11 = NULL;
    m_tabSprite12 = NULL;
    m_tabSprite13 = NULL;
    m_tabSprite14 = NULL;
    m_hlBusy = 0;
    if (handle == 0 && g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
        if (m_position == STATUSBAR_HIDDEN) {
            RefreshState();
        }
        if (m_activeTab != TAB_GAME) {
            SetTabState(SBICMD_TAB_GAME, MENUITEM_SELECTED);
        }
        SetTab(GAME_TAB_MENU, 1);
        Deactivate();
    } else {
        m_hitTestDisabled = 0;
    }
    m_toggleActive = 0;
    m_toggleHandle = 0;
    Deactivate();
}

// @early-stop
RVA(0x0010b320, 0x167)
void CStatusBarMgr::UpdateDestructButtonStatusBar() {

    switch (m_destructWarnActive) {
        case DESTRUCT_WARNING_FORWARD: {
            i64 d = static_cast<i64>(g_frameTime) - m_destructWarnLast;
            if (d >= m_destructWarnDelay) {
                m_modeState = static_cast<DestructButtonFrame>(m_modeState + 1);
                if (m_modeState >= DESTRUCT_FRAME_WARNING_LAST) {
                    m_modeState = DESTRUCT_FRAME_WARNING_LAST;
                    m_destructWarnActive = DESTRUCT_WARNING_REVERSE;
                }
                m_destructWarnDelay = static_cast<u32>(
                    g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32)
                );
                m_destructWarnLast = static_cast<u32>(g_frameTime);
                CSBI_ImageSet* w = m_modeNotify;
                if (w) {
                    w->Notify(IDX(m_modeState));
                }
            }
            break;
        }
        case DESTRUCT_WARNING_REVERSE: {
            i64 d = static_cast<i64>(g_frameTime) - m_destructWarnLast;
            if (d >= m_destructWarnDelay) {
                m_modeState = static_cast<DestructButtonFrame>(m_modeState - 1);
                if (m_modeState <= DESTRUCT_FRAME_WARNING_FIRST) {
                    m_modeState = DESTRUCT_FRAME_WARNING_FIRST;
                    m_destructWarnActive = DESTRUCT_WARNING_FORWARD;
                }
                m_destructWarnDelay = static_cast<u32>(
                    g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32)
                );
                m_destructWarnLast = static_cast<u32>(g_frameTime);
                CSBI_ImageSet* w = m_modeNotify;
                if (w) {
                    w->Notify(IDX(m_modeState));
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
    if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
        return;
    }
    if (m_position == STATUSBAR_HIDDEN) {
        RefreshState();
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
i32 CStatusBarMgr::HlClickGroup0(StatusBarHighlightRow row) {
    i32 rowIndex = IDX(row);
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_playerCommandPending == 0
        && m_hlGrid[rowIndex].m_state == IDX(HLROW_IDLE_CYCLE)) {
        i32 handle = m_hlGrid[rowIndex].m_value;
        i32* slot = &m_hlGrid[rowIndex].m_value;
        if ((static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(handle)) {
            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
            if (host->m_emitGate == 0) {
                void* found = 0;
                CMapStringToPtr* map = &host->m_cues;
                map->Lookup("GAME_TABHIGHLIGHT1", found);
                if (found) {
                    i32 gate = g_sndEnabled;
                    i32 item = g_sndCueTag;
                    if (gate != 0) {
                        LeafCue* p = static_cast<LeafCue*>(found);
                        if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                            >= static_cast<u32>(p->m_replayDelay)) {
                            p->m_lastPlayTime = g_killCueClock;
                            p->m_sound->ConfigureItem(item, 0, 0, 0);
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
i32 CStatusBarMgr::HlClickGroup1(StatusBarHighlightRow row) {
    i32 rowIndex = IDX(row);
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_playerCommandPending == 0
        && m_hlGrid[rowIndex + 4].m_state == IDX(HLROW_IDLE_CYCLE)) {
        i32 handle = m_hlGrid[rowIndex + 4].m_value;
        i32* slot = &m_hlGrid[rowIndex + 4].m_value;
        if ((static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(handle)) {
            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
            if (host->m_emitGate == 0) {
                void* found = 0;
                CMapStringToPtr* map = &host->m_cues;
                map->Lookup("GAME_TABHIGHLIGHT1", found);
                if (found) {
                    i32 gate = g_sndEnabled;
                    i32 item = g_sndCueTag;
                    if (gate != 0) {
                        LeafCue* p = static_cast<LeafCue*>(found);
                        if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                            >= static_cast<u32>(p->m_replayDelay)) {
                            p->m_lastPlayTime = g_killCueClock;
                            p->m_sound->ConfigureItem(item, 0, 0, 0);
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
i32 CStatusBarMgr::HlClickGroup2(StatusBarHighlightRow row) {
    i32 rowIndex = IDX(row);
    if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_playerCommandPending == 0
        && m_hlGrid[rowIndex + 8].m_state == IDX(HLROW_IDLE_CYCLE)) {
        i32 handle = m_hlGrid[rowIndex + 8].m_value;
        i32* slot = &m_hlGrid[rowIndex + 8].m_value;
        if ((static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(handle)) {
            CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
            if (host->m_emitGate == 0) {
                void* found = 0;
                CMapStringToPtr* map = &host->m_cues;
                map->Lookup("GAME_TABHIGHLIGHT1", found);
                if (found) {
                    i32 gate = g_sndEnabled;
                    i32 item = g_sndCueTag;
                    if (gate != 0) {
                        LeafCue* p = static_cast<LeafCue*>(found);
                        if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                            >= static_cast<u32>(p->m_replayDelay)) {
                            p->m_lastPlayTime = g_killCueClock;
                            p->m_sound->ConfigureItem(item, 0, 0, 0);
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

// @early-stop
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
        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
        if (host->m_emitGate == 0) {
            void* found = 0;
            CMapStringToPtr* map = &host->m_cues;
            map->Lookup("GAME_TABHIGHLIGHT1", found);
            if (found) {
                i32 gate = g_sndEnabled;
                i32 item = g_sndCueTag;
                if (gate != 0) {
                    LeafCue* p = static_cast<LeafCue*>(found);
                    if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                        >= static_cast<u32>(p->m_replayDelay)) {
                        p->m_lastPlayTime = g_killCueClock;
                        p->m_sound->ConfigureItem(item, 0, 0, 0);
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
        if (!(static_cast<CPlay*>(g_gameReg->m_curState))->SetCursorFrame(0x66)) {
            goto notActivated;
        }
        CDDrawSubMgrLeafScan* host = g_gameReg->m_world->m_soundRegistry;
        if (host->m_emitGate == 0) {
            void* found = 0;
            CMapStringToPtr* map = &host->m_cues;
            map->Lookup("GAME_TABHIGHLIGHT1", found);
            if (found) {
                i32 gate = g_sndEnabled;
                i32 item = g_sndCueTag;
                if (gate != 0) {
                    LeafCue* p = static_cast<LeafCue*>(found);
                    if (g_killCueClock - static_cast<u32>(p->m_lastPlayTime)
                        >= static_cast<u32>(p->m_replayDelay)) {
                        p->m_lastPlayTime = g_killCueClock;
                        p->m_sound->ConfigureItem(item, 0, 0, 0);
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
notActivated:
    return 0;
}

RVA(0x0010bb50, 0x24)
void CStatusBarMgr::ReportTab(i32 tab) {
    UpdateFallingItemStatusBar(tab, 0x4f, 0x1b3);
    EnterHlRow(1, tab);
}

RVA(0x0010bb90, 0x3f)
void CStatusBarMgr::SetMode(i32 mode) {
    m_modeArmed = 1;
    if (mode && m_modeState != DESTRUCT_FRAME_DISABLED) {
        m_destructWarnActive = DESTRUCT_WARNING_INACTIVE;
        m_modeState = DESTRUCT_FRAME_IDLE;
        if (m_modeNotify) {
            m_modeNotify->Notify(1);
        }
    }
}

RVA(0x0010bbe0, 0x34)
i32 CStatusBarMgr::GetActiveValue() {
    if (m_rezActive == 0) {
        return m_extraNotifyArg0;
    }
    if (m_ptrPool.GetSize() > 0 && m_ptrPool.GetSize() > m_rezTick) {
        return *static_cast<i32*>(m_ptrPool.GetAt(m_rezTick));
    }
    return 0;
}
