#include <rva.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/ExitTrigger.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/Play.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Warlord.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wwd/WwdGameObjectFamily.h>

// @early-stop
RVA(0x0003f5f0, 0x526)
i32 CExitTrigger::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    if (g_gameReg->m_gameMode == 1) {
        CWwdGameObjectA* trig = m_object;
        CTriggerMgr::HitSpanArg span;
        span.m_span = &trig->m_area;
        g_gameReg->m_cmdGrid->HitTestApply(trig->m_screenX, trig->m_screenY, span);
    } else if (m_resolved != 0) {
        i32 hitPlayer;
        i32 hitRow;
        CWwdGameObjectA* obj = m_object;
        CGrunt* hit =
            g_gameReg->m_cmdGrid
                ->FindGruntAt(obj->m_screenX, obj->m_screenY, &obj->m_area, &hitPlayer, &hitRow, 0);
        if (hit != 0) {
            i32 owningPlayer = m_object->m_smarts;
            if (hitPlayer == owningPlayer) {
                goto done;
            }
            m_resolved = 0;
            GruntzPlayer* loser = &g_gameReg->m_options[owningPlayer];
            GruntzPlayer* winner = &g_gameReg->m_options[hitPlayer];
            if (loser != 0) {
                g_gameReg->m_chatLog->AddItem(
                    static_cast<const char*>(
                        loser->GetName() + " was conquered by " + winner->GetName() + "!"
                    ),
                    0,
                    0x11
                );
                loser->m_clearedRound = 1;
            }
            g_gameReg->m_scoreHud->MarkFlag(hitPlayer, owningPlayer);
            g_gameReg->m_cmdGrid->ClearRowAndRefresh(owningPlayer);
            g_gameReg->m_cmdGrid->CellDispatch(hitPlayer, hitRow, DEATH_EXIT, -1);
            if (m_warlordLogic != 0) {
                m_warlordLogic->ResolveDeathAnimation();
                m_warlordLogic = 0;
            }
            GruntzPlayer* claimed = &g_gameReg->m_options[hitPlayer];
            if (claimed != 0) {
                CGameObject* found = 0;
                CGameObject* warlordObj = 0;
                if (MapLookupById(
                        g_gameReg->m_world->m_childGroup->m_map48,
                        claimed->m_warlordObjectId,
                        found
                    )) {
                    warlordObj = found;
                }
                CWarlord* wl = static_cast<CWarlord*>(warlordObj->m_animWorker->m_logic);
                if (wl != 0) {
                    wl->RaiseBattleAlert();
                }
            }
            CDDrawChildGroup* grp = g_gameReg->m_world->m_childGroup;
            POSITION pos = grp->m_list.GetHeadPosition();
            while (pos != 0) {
                CGameObject* cur = grp->NextChild(pos);
                if (cur->m_animWorker->m_notify == CreateGruntCreationPoint
                    && cur->m_smarts == owningPlayer) {
                    cur->m_smarts = hitPlayer;
                    CShadeTable* tbl = g_gameReg->m_spriteFactory->GetSel(
                        g_gameReg->m_options[owningPlayer].m_colorIndex,
                        0
                    );
                    cur->m_drawActive = 1;
                    cur->m_drawFillCmd = 0xa;
                    cur->m_drawFillArg = tbl;
                    if (hitPlayer == g_curPlayer) {
                        CoordPoolNode* head = g_coordPool.m_freeHead;
                        Coord* mark = 0;
                        if (head->m_next != 0) {
                            mark = &head->m_coord;
                            head = head->m_next;
                            g_coordPool.m_freeHead = head;
                        }
                        mark->m_x = (cur->m_screenX & ~0x1f) + 0x10;
                        mark->m_y = (cur->m_screenY & ~0x1f) + 0x10;
                        CPtrArray& marks =
                            static_cast<CPlay*>(g_gameReg->m_curState)->m_startMarkers;
                        marks.SetAtGrow(marks.GetSize(), mark);
                    }
                }
                if (cur->m_animWorker->m_notify == CreateFortressFlag
                    && cur->m_smarts == owningPlayer) {
                    cur->m_smarts = hitPlayer;
                    CShadeTable* tbl = g_gameReg->m_spriteFactory->GetSel(
                        g_gameReg->m_options[owningPlayer].m_colorIndex,
                        0
                    );
                    cur->m_drawActive = 1;
                    cur->m_drawFillCmd = 0xa;
                    cur->m_drawFillArg = tbl;
                }
            }
            if (owningPlayer == g_curPlayer) {
                g_gameReg->m_cmdGrid->LoadFinishLevelSprite(5);
            } else {
                GruntzPlayer* board = &g_gameReg->m_options[owningPlayer];
                if (board != 0 && board->m_humanControlled == 0) {
                    board->m_battlezConfig.Clear();
                }
            }
        } else {

            i32 lostPlayer = m_object->m_smarts;
            if (lostPlayer == g_curPlayer) {
                goto done;
            }
            GruntzPlayer* slot = &g_gameReg->m_options[lostPlayer];
            if (g_gameReg->m_options[lostPlayer].m_joined == 0) {
                goto done;
            }
            if (slot->m_clearedRound != 0) {
                goto done;
            }
            if (slot->m_doneFlag == 0) {
                goto done;
            }
            slot->m_clearedRound = 1;
            m_resolved = 0;
            if (m_warlordLogic != 0) {
                m_warlordLogic->ResolveDeathAnimation();
                m_warlordLogic = 0;
            }
            CDDrawChildGroup* grp = g_gameReg->m_world->m_childGroup;
            POSITION pos = grp->m_list.GetHeadPosition();
            while (pos != 0) {
                CGameObject* cur = grp->NextChild(pos);
                GameObjNotifyFn who = cur->m_animWorker->m_notify;
                if (who == CreateGruntCreationPoint || who == CreateFortressFlag) {
                    if (cur->m_smarts == m_object->m_smarts) {
                        i32 x = cur->m_screenX;
                        i32 y = cur->m_screenY;
                        if (x < g_gameReg->m_viewBounds.right && x >= g_gameReg->m_viewBounds.left
                            && y < g_gameReg->m_viewBounds.bottom
                            && y >= g_gameReg->m_viewBounds.top) {
                            CWwdGameObjectA* fx =
                                g_gameReg->m_world->m_childGroup
                                    ->CreateSprite(0, x, y, 0xf4240, "Explosion", 0x40003);
                            if (fx != 0) {
                                fx->ApplyLookupGeometry("GAME_EXPLOSION3", 0);
                                fx->m_smarts = 0;
                                fx->m_score = 0;
                            }
                        }
                        cur->m_flags |= 0x10000;
                    }
                }
            }
            g_gameReg->m_cmdGrid->ClearRow(m_object->m_smarts);
        }
    }

done:
    return 0;
}
