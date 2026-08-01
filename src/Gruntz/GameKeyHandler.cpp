#include <Wap32/Object.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/CurPlayer.h>
#include <rva.h>

#include <Gruntz/FreeNodePool.h>

#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Play.h>
#include <Gruntz/StateMgrBZ.h>
#include <Wwd/WwdGameObjectFamily.h>

#define CLEAR_TAB_HINT(sndHost)                                                                    \
    do {                                                                                           \
        CDDrawSubMgrLeafScan* _s = (sndHost);                                                      \
        if (_s->m_emitGate == 0) {                                                                 \
            void* found = 0;                                                                       \
            _s->m_10.Lookup("GAME_TABHIGHLIGHT1", found);                                          \
            if (found != 0)                                                                        \
                static_cast<LeafCue*>(found)->PlayIfElapsed(g_sndCueTag, 0, 0, 0);                 \
        }                                                                                          \
    } while (0)

// @early-stop
RVA(0x000cbcc0, 0x1770)
i32 CPlay::Vslot0c(i32 vk, i32 lparam) {
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

            if (key == 0x59 || key == 0xd) {
                if (g_gameReg->m_134 == 1) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    if (g_gameReg->m_cmdGrid->m_phase == 1) {
                        g_gameReg->UpdateScoreHud();
                    }
                    PostMessageA(host->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
                    return 1;
                }
                CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                (static_cast<CGruntzMgr*>((host)))->AccrueScoreTime();
                return 1;
            }
            if (key == 0x4e || key == 0x1b) {
                CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                this->ReleaseLevelOverlay(0);
                return 1;
            }

        } else {

            if (key == 0x51) {
                if (g_gameReg->m_134 == 1) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    if (g_gameReg->m_cmdGrid->m_phase == 1) {
                        g_gameReg->UpdateScoreHud();
                    }
                    PostMessageA(host->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
                }
                return 1;
            }

            if (key == 0x53 && g_gameReg->m_134 == 1) {
                CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                (static_cast<CGruntzMgr*>((host)))->AccrueScoreTime();
            }
            if (key == 0x52) {
                if (host->m_134 == 1 && g_gameReg->m_cmdGrid->m_phase != 1) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    CGameWnd* r = g_gameReg->m_gameWnd;
                    PostMessageA(r->m_hwnd, 0x111, 0x806b, 0);
                }
                return 1;
            }
            if (key == 0x4e) {
                if (host->m_134 == 1 && g_gameReg->m_cmdGrid->m_phase == 1) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    (static_cast<CGruntzMgr*>((host)))->AccrueScoreTime();
                }
                return 1;
            }
            if (key == 0x4f) {
                if (host->m_134 != 1 && self->m_guts->m_578 != 0) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    this->ReleaseLevelOverlay(0);
                }
                return 1;
            }
        }
    }

    if (key == 0xd) {
        CChatBoxOwner* rec = self->m_hitTest;
        if (rec->m_10 != 0) {
            rec->ProcessCheatInput(0xd, lparam);
        } else {
            rec->m_14->EndInput();
            rec->m_10 = 1;
            self->m_hitTest->ProcessCheatInput(0xd, lparam);
        }
        return 1;
    }

    if (key == 0x1b) {
        CTriggerMgr* h68 = host->m_cmdGrid;
        CWwdGameObjectA* n = h68->m_goal;
        if (n != 0) {
            n->m_flags |= 0x10000;
            h68->m_goal = 0;
        }
        h68->m_armed = 0;
        CChatBoxOwner* rec = self->m_hitTest;
        if (rec->m_10 != 0) {
            this->FlushPendingOps();
            self->m_hitTest->m_14->EndInput();
            self->m_hitTest->m_10 = 0;
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

    if (self->m_hitTest->m_10 != 0) {
        return 1;
    }
    if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
        return 1;
    }

    StateMgrBZ* dev = g_spawnConfig;

    if (key == 0x9) {
        i32 idx = self->m_514;
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
            self->m_514 = pick;
            this->ResetGoals(area->m_focusX, area->m_focusY);
        }
    }

    if (key == 0x48) {
        GruntzPlayer* a = &g_gameReg->m_options[g_curPlayer];
        if (a == 0) {
            return 1;
        }
        this->ResetGoals(a->m_focusX, a->m_focusY);
        return 1;
    }

    if (key == 0x51) {
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
            s->m_10.Lookup("GAME_TABHIGHLIGHT1", found);
            if (found != 0) {
                static_cast<LeafCue*>(found)->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
            }
        }
        return 1;
    }

    if (key == 0x5a) {
        g_gameReg->m_cmdGrid->EnqueueGroupCells();
        return 1;
    }

    if (key == 0x43) {
        g_gameReg->m_cmdGrid->CenterOnGroup(dev->m_edgeKeys & 0x20);
        return 1;
    }

    if (key == 0x54) {
        this->FlushPendingOps();
        g_gameReg->m_cmdGrid->ToggleRegionA();
        return 1;
    }

    if (key == 0x59) {
        this->FlushPendingOps();
        g_gameReg->m_cmdGrid->ToggleRegionB();
        return 1;
    }

    if (key == 0x20) {
        if (dev->m_edgeKeys & 0x20) {
            CDDrawWorkerHost* obj = self->m_world->m_level->m_mainPlane;
            i32 v0 = obj->m_snappedX;
            i32 v1 = obj->m_snappedY;
            Coord* slot;
            if (self->arr488Count() < 4) {
                CoordPoolNode* head = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
                CoordPoolNode* nx = head->m_next;
                if (nx != 0) {
                    slot = &head->m_coord;
                    g_coordPool.m_freeHead = nx;
                } else {
                    slot = 0;
                }
            } else {

                slot = static_cast<Coord*>(self->m_488.GetAt(0));
                self->m_488.RemoveAt(0, 1);
                i32 c = self->m_49c - 1;
                self->m_49c = c;
                if (c < 0) {
                    self->m_49c = self->arr488Count() - 1;
                }
            }
            slot->m_x = v0;
            slot->m_y = v1;
            if (self->m_49c != self->arr488Count() - 1) {
                self->m_488.InsertAt(self->m_49c + 1, slot, 1);
                self->m_49c = self->m_49c + 1;
                return 1;
            }
            self->m_488.SetAtGrow(self->arr488Count(), slot);
            self->m_49c = self->m_49c + 1;
            return 1;
        }
        if (self->arr488Count() == 0) {
            return 1;
        }
        if (dev->m_edgeKeys & 1) {
            i32 c = self->m_49c - 1;
            self->m_49c = c;
            if (c < 0) {
                self->m_49c = self->arr488Count() - 1;
            }
        } else {
            i32 c = self->m_49c + 1;
            self->m_49c = c;
            if (c >= self->arr488Count()) {
                self->m_49c = 0;
            }
        }
        i32* e = static_cast<i32*>(self->m_488.GetAt(self->m_49c));
        this->ResetGoals(e[0], e[1]);
        return 1;
    }

    if (key == 0x8) {
        if (self->arr488Count() <= 0) {
            return 1;
        }
        i32 cur = self->m_49c;
        if (cur < 0) {
            return 1;
        }
        CoordPoolNode* node = g_coordPool.NodeOf(self->m_488.GetAt(cur));
        self->m_488.RemoveAt(cur, 1);
        node->m_next = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
        g_coordPool.m_freeHead = node;
        i32 c = self->m_49c - 1;
        self->m_49c = c;
        if (c != -1) {
            return 1;
        }
        if (self->arr488Count() == 0) {
            return 1;
        }
        self->m_49c = self->arr488Count() - 1;
        return 1;
    }

    if (key == 0x4d && (dev->m_edgeKeys & 0x20)) {
        g_gameReg->SetSoundLevelState(g_gameReg->m_musicEnabled == 0);
        return 1;
    }

    if (key == 0x56 && (dev->m_edgeKeys & 0x20)) {
        g_gameReg->m_isVoiceEnabled = (g_gameReg->m_isVoiceEnabled == 0);
        return 1;
    }

    if (key == 0x41) {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        CStatusBarMgr* lv = self->m_guts;
        if (lv->m_hlBusy != 0) {
            return 1;
        }
        if (lv->m_position == 2) {
            lv->RefreshState();
        }
        if (lv->m_activeTab != 2) {
            lv->SetTabState(2, 3);
            lv->Deactivate();
        } else {
            lv->Deactivate();
        }
        return 1;
    }

    if (key == 0x53) {
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
        if (lv->m_position == 2) {
            lv->RefreshState();
        }
        if (lv->m_activeTab != 3) {
            lv->SetTabState(3, 3);
            lv->Deactivate();
        } else {
            lv->Deactivate();
        }
        return 1;
    }

    if (key == 0x44) {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        CStatusBarMgr* lv = self->m_guts;
        if (lv->m_hlBusy != 0) {
            return 1;
        }
        if (lv->m_position == 2) {
            lv->RefreshState();
        }
        if (lv->m_activeTab != 1) {
            lv->SetTabState(1, 3);
            lv->Deactivate();
        } else {
            lv->Deactivate();
        }
        return 1;
    }

    if (key == 0x46) {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        if (g_gameReg->m_134 == 1) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        self->m_guts->AdvanceTab(g_spawnConfig->m_edgeKeys & 1);
        return 1;
    }

    if (key == 0x47) {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        CStatusBarMgr* lv = self->m_guts;
        if (lv->m_hlBusy != 0) {
            return 1;
        }
        if (lv->m_position == 2) {
            lv->RefreshState();
        }
        if (lv->m_activeTab != 5) {
            lv->SetTabState(5, 3);
        }
        lv->SetTab(5, 1);
        lv->Deactivate();
        return 1;
    }

    if (lparam & 0x1000000) {
        if (key == 0x25) {
            self->m_scrollEdgeLock |= 1;
            return 1;
        }
        if (key == 0x27) {
            self->m_scrollEdgeLock |= 4;
            return 1;
        }
        if (key == 0x26) {
            self->m_scrollEdgeLock |= 2;
            return 1;
        }
        if (key == 0x28) {
            self->m_scrollEdgeLock |= 8;
            return 1;
        }
        if (key == 0x2d || key == 0x2e || key == 0x24 || key == 0x23 || key == 0x21
            || key == 0x22) {
            return 1;
        }
    }

    if (key == 0x61 || key == 0x62 || key == 0x63 || key == 0x64 || key == 0x65 || key == 0x66
        || key == 0x67 || key == 0x68 || key == 0x69 || key == 0x90 || key == 0x6f || key == 0x6a
        || key == 0x24 || key == 0x23 || key == 0x21 || key == 0x22 || key == 0xc || key == 0x26
        || key == 0x28 || key == 0x25 || key == 0x27 || key == 0x2d || key == 0x2e || key == 0x6e) {
        goto recorder_place;
    }

    if (key == 0x49) {
        if (g_gruntCreation == 0) {
            return 1;
        }
        GruntzPlayer* a = &g_gameReg->m_options[g_curPlayer];
        if (a == 0) {
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

    if (key == 0x50) {
        if (g_gooPuddlez == 0) {
            return 1;
        }
        if (g_gameReg->m_134 == 2) {
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

    if (key == 0x78) {
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

    if (key == 0x4b) {
        if (g_gruntDestruction == 0) {
            return 1;
        }
        i32 outA = 0;
        i32 outB = 0;
        CGrunt* r =
            host->m_cmdGrid->ScreenToCell(self->m_cursorX, self->m_cursorY, &outB, &outA, 5);
        if (r == 0) {
            return 1;
        }
        host->m_cmdGrid->CellDispatch(outB, outA, 0, -1);
        return 1;
    }

    if (key == 0x31) {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(1);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(1);
        }
        return 1;
    }
    if (key == 0x32) {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(2);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(2);
        }
        return 1;
    }
    if (key == 0x33) {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(3);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(3);
        }
        return 1;
    }
    if (key == 0x34) {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(4);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(4);
        }
        return 1;
    }
    if (key == 0x35) {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(5);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(5);
        }
        return 1;
    }
    if (key == 0x36) {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(6);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(6);
        }
        return 1;
    }
    if (key == 0x37) {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(7);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(7);
        }
        return 1;
    }
    if (key == 0x38) {
        if (g_spawnConfig->m_edgeKeys & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(8);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(8);
        }
        return 1;
    }
    if (key == 0x39) {
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
    if (self->m_4f0 != 0) {
        return 1;
    }
    if (self->m_dragInhibit1 != 0) {
        self->m_dragInhibit1 = 0;
        self->m_guts->CommitSlot(0);
        this->SetCursorFrame(0);
        if (key != 0x2d) {
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
    if (key == 0x2e || key == 0x6e) {
        level->ReportTab(st);
        this->SetCursorFrame(0);
        return 1;
    }
    level->EnterHlRow(0, st);
    this->SetCursorFrame(0);
    if (lvl == 0) {
        if (ph == 0) {
            if (key != 0x90) {
                goto tail_default;
            }
            return 1;
        }
        if (ph == 1) {
            if (key == 0x67) {
                return 1;
            }
            if (key != 0x24) {
                goto tail_default;
            }
            return 1;
        }
        if (ph == 2) {
            if (key == 0x64) {
                return 1;
            }
            if (key != 0x25) {
                goto tail_default;
            }
            return 1;
        }
        if (key == 0x61) {
            return 1;
        }
        if (key != 0x23) {
            goto tail_default;
        }
        return 1;
    }
    if (lvl == 1) {
        if (ph == 0) {
            if (key != 0x6f) {
                goto tail_default;
            }
            return 1;
        }
        if (ph == 1) {
            if (key == 0x68) {
                return 1;
            }
            if (key != 0x26) {
                goto tail_default;
            }
            return 1;
        }
        if (ph == 2) {
            if (key != 0xc) {
                goto tail_default;
            }
            return 1;
        }
        if (key == 0x62) {
            return 1;
        }
        if (key != 0x28) {
            goto tail_default;
        }
        return 1;
    }
    if (ph == 0) {
        if (key != 0x6a) {
            goto tail_default;
        }
        return 1;
    }
    if (ph == 1) {
        if (key == 0x69) {
            return 1;
        }
        if (key != 0x21) {
            goto tail_default;
        }
        return 1;
    }
    if (ph == 2) {
        if (key == 0x66) {
            return 1;
        }
        if (key != 0x27) {
            goto tail_default;
        }
        return 1;
    }
    if (key == 0x63) {
        return 1;
    }
    if (key != 0x22) {
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
            case 0x0c:
                lv->HlClickGroup1(2);
                return 1;
            case 0x21:
                lv->HlClickGroup2(1);
                return 1;
            case 0x22:
                lv->HlClickGroup2(3);
                return 1;
            case 0x23:
                lv->HlClickGroup0(3);
                return 1;
            case 0x24:
                lv->HlClickGroup0(1);
                return 1;
            case 0x25:
                lv->HlClickGroup0(2);
                return 1;
            case 0x26:
                lv->HlClickGroup1(1);
                return 1;
            case 0x27:
                lv->HlClickGroup2(2);
                return 1;
            case 0x28:
                lv->HlClickGroup1(3);
                return 1;
            case 0x2d:
                lv->ActivateSlot(-1);
                return 1;
            case 0x61:
                lv->HlClickGroup0(3);
                return 1;
            case 0x62:
                lv->HlClickGroup1(3);
                return 1;
            case 0x63:
                lv->HlClickGroup2(3);
                return 1;
            case 0x64:
                lv->HlClickGroup0(2);
                return 1;
            case 0x65:
                lv->HlClickGroup1(2);
                return 1;
            case 0x66:
                lv->HlClickGroup2(2);
                return 1;
            case 0x67:
                lv->HlClickGroup0(1);
                return 1;
            case 0x68:
                lv->HlClickGroup1(1);
                return 1;
            case 0x69:
                lv->HlClickGroup2(1);
                return 1;
            case 0x6a:
                lv->HlClickGroup2(0);
                return 1;
            case 0x6f:
                lv->HlClickGroup1(0);
                return 1;
            case 0x90:
                lv->HlClickGroup0(0);
                return 1;
        }
    }
    return 1;
}

#undef CLEAR_TAB_HINT
