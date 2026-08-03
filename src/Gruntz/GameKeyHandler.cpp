#include <rva.h>

#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/Play.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/StateMgrBZ.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Wap32/Object.h>
#include <Wwd/WwdGameObjectFamily.h>

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

// @early-stop
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
                if (g_gameReg->m_gameMode == 1) {
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
            if (key == 'N' || key == VK_ESCAPE) {
                CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                this->ReleaseLevelOverlay(0);
                return 1;
            }

        } else {

            if (key == 'Q') {
                if (g_gameReg->m_gameMode == 1) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    if (g_gameReg->m_cmdGrid->m_phase == 1) {
                        g_gameReg->UpdateScoreHud();
                    }
                    PostMessageA(host->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
                }
                return 1;
            }

            if (key == 'S' && g_gameReg->m_gameMode == 1) {
                CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                (static_cast<CGruntzMgr*>((host)))->AccrueScoreTime();
            }
            if (key == 'R') {
                if (host->m_gameMode == 1 && g_gameReg->m_cmdGrid->m_phase != 1) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    CGameWnd* r = g_gameReg->m_gameWnd;
                    PostMessageA(r->m_hwnd, 0x111, 0x806b, 0);
                }
                return 1;
            }
            if (key == 'N') {
                if (host->m_gameMode == 1 && g_gameReg->m_cmdGrid->m_phase == 1) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    (static_cast<CGruntzMgr*>((host)))->AccrueScoreTime();
                }
                return 1;
            }
            if (key == 'O') {
                if (host->m_gameMode != 1 && self->m_guts->m_observerTabAvailable != 0) {
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
        if (n != 0) {
            n->m_flags |= 0x10000;
            h68->m_goal = 0;
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
        if (a == 0) {
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
            if (found != 0) {
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
                if (nx != 0) {
                    slot = &head->m_coord;
                    g_coordPool.m_freeHead = nx;
                } else {
                    slot = 0;
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

    if (key == 'D') {
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

    if (key == 'F') {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        if (g_gameReg->m_gameMode == 1) {
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

    if (key == 'P') {
        if (g_gooPuddlez == 0) {
            return 1;
        }
        if (g_gameReg->m_gameMode == 2) {
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
        if (r == 0) {
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
