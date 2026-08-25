#include <rva.h>

#include <Gruntz/WormholeActs.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/ExitTrigger.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/Play.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/Warlord.h>
#include <Gruntz/Wormhole.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <stddef.h>

RVA_DYNINIT(0x0003f1f0, 0xa, CActRegPool<CExitTrigger>::s_table)
RVA_DYNINIT(0x0003f210, 0x15, CActRegPool<CExitTrigger>::s_table)
RVA_DYNINIT(0x0003f240, 0xe, CActRegPool<CExitTrigger>::s_table)
RVA_DYNINIT(0x0003f260, 0x1f, CActRegPool<CExitTrigger>::s_table)
template<> DATA(0x002445c0)
CActReg CActRegPool<CExitTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x0003f290, 0x102)
void CExitTrigger::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CExitTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CExitTrigger>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0003f3f0, 0x18d)
void CExitTrigger::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CExitTrigger>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CExitTrigger::AdvanceAnim);
}

RVA(0x0003f5f0, 0x526)
i32 CExitTrigger::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
        CWwdGameObjectA* trig = m_object;
        CTriggerMgr::HitSpanArg span;
        span.m_span = &trig->m_area;
        g_gameReg->m_cmdGrid->HitTestApply(trig->m_screenX, trig->m_screenY, span);
    } else if (m_resolved != 0) {
        i32 hitPlayerIndex;
        i32 hitUnitIndex;
        CWwdGameObjectA* obj = m_object;
        CGrunt* hit = g_gameReg->m_cmdGrid->FindGruntAt(
            obj->m_screenX,
            obj->m_screenY,
            &obj->m_area,
            &hitPlayerIndex,
            &hitUnitIndex,
            NULL
        );
        if (hit != NULL) {
            i32 owningPlayer = m_object->m_smarts;
            if (hitPlayerIndex == owningPlayer) {
                goto done;
            }
            m_resolved = 0;
            GruntzPlayer* loser = &g_gameReg->m_options[owningPlayer];
            GruntzPlayer* winner = &g_gameReg->m_options[hitPlayerIndex];
            if (loser != NULL) {
                g_gameReg->m_chatLog->AddItem(
                    static_cast<const char*>(
                        loser->GetName() + " was conquered by " + winner->GetName()
                            + DATA_COMPGEN(0x0020d168, "!")
                        ),
                        0,
                        0x11
                );
                loser->m_clearedRound = 1;
            }
            g_gameReg->m_scoreHud->MarkFlag(hitPlayerIndex, owningPlayer);
            g_gameReg->m_cmdGrid->StartPlayerDefeatSequence(owningPlayer);
            g_gameReg->m_cmdGrid->StartUnitDeath(hitPlayerIndex, hitUnitIndex, DEATH_EXIT, -1);
            if (m_warlordLogic != NULL) {
                m_warlordLogic->ResolveDeathAnimation();
                m_warlordLogic = NULL;
            }
            GruntzPlayer* claimed = &g_gameReg->m_options[hitPlayerIndex];
            if (claimed != NULL) {
                CGameObject* found = NULL;
                CGameObject* warlordObj = NULL;
                if (MapLookupById(
                        g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
                        claimed->m_warlordObjectId,
                        found
                    )) {
                    warlordObj = found;
                }
                CWarlord* wl = static_cast<CWarlord*>(warlordObj->m_logicRecord->m_userLogic);
                if (wl != NULL) {
                    wl->RaiseBattleAlert();
                }
            }
            CDDrawChildGroup* grp = g_gameReg->m_world->m_childGroup;
            POSITION pos = grp->m_list.GetHeadPosition();
            while (pos != NULL) {
                CGameObject* cur = grp->NextChild(pos);
                if (cur->m_logicRecord->m_dispatch == CreateGruntCreationPoint
                    && cur->m_smarts == owningPlayer) {
                    cur->m_smarts = hitPlayerIndex;
                    CShadeTable* tbl = g_gameReg->m_spriteFactory->GetSel(
                        IDX(g_gameReg->m_options[hitPlayerIndex].m_colorIndex),
                        0
                    );
                    SET_DRAW_FILL(cur, SHADE_PAL_16, tbl);
                    if (hitPlayerIndex == g_curPlayer) {
                        CoordPoolNode* head = g_coordPool.m_freeHead;
                        Coord* mark = NULL;
                        if (head->m_next != NULL) {
                            mark = &head->m_coord;
                            head = head->m_next;
                            g_coordPool.m_freeHead = head;
                        }
                        mark->m_x = (cur->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
                        mark->m_y = (cur->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
                        CPtrArray& marks =
                            static_cast<CPlay*>(g_gameReg->m_curState)->m_startMarkers;
                        marks.SetAtGrow(marks.GetSize(), mark);
                    }
                }
                if (cur->m_logicRecord->m_dispatch == CreateFortressFlag
                    && cur->m_smarts == owningPlayer) {
                    cur->m_smarts = hitPlayerIndex;
                    CShadeTable* tbl = g_gameReg->m_spriteFactory->GetSel(
                        IDX(g_gameReg->m_options[hitPlayerIndex].m_colorIndex),
                        0
                    );
                    SET_DRAW_FILL(cur, SHADE_PAL_16, tbl);
                }
            }
            if (owningPlayer == g_curPlayer) {
                g_gameReg->m_cmdGrid->LoadFinishLevelSprite(FINISH_REASON_BATTLEZ_DEFEAT);
            } else {
                GruntzPlayer* board = &g_gameReg->m_options[owningPlayer];
                if (board != NULL && board->m_humanControlled == 0) {
                    board->m_battlezConfig.Clear();
                }
            }
        } else {

            i32 lostPlayer = m_object->m_smarts;
            if (lostPlayer == g_curPlayer) {
                goto done;
            }
            GruntzPlayer* slot = &g_gameReg->m_options[lostPlayer];
            if (slot->m_joined == 0) {
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
            if (m_warlordLogic != NULL) {
                m_warlordLogic->ResolveDeathAnimation();
                m_warlordLogic = NULL;
            }
            CDDrawChildGroup* grp = g_gameReg->m_world->m_childGroup;
            POSITION pos = grp->m_list.GetHeadPosition();
            while (pos != NULL) {
                CGameObject* cur = grp->NextChild(pos);
                GameObjectLogicFn who = cur->m_logicRecord->m_dispatch;
                if (who == CreateGruntCreationPoint || who == CreateFortressFlag) {
                    if (cur->m_smarts == m_object->m_smarts) {
                        i32 x = cur->m_screenX;
                        i32 y = cur->m_screenY;
                        if (CGameLevel::PointInRect(&g_gameReg->m_viewBounds, x, y)) {
                            CWwdGameObjectA* fx =
                                g_gameReg->m_world->m_childGroup
                                    ->CreateSprite(0, x, y, SORTKEY_OVERLAY, "Explosion", 0x40003);
                            if (fx != NULL) {
                                fx->ApplyLookupGeometry("GAME_EXPLOSION3", 0);
                                fx->m_smarts = 0;
                                fx->m_score = 0;
                            }
                        }
                        cur->m_flags |= 0x10000;
                    }
                }
            }
            g_gameReg->m_cmdGrid->StartPlayerVictorySequence(m_object->m_smarts);
        }
    }

done:
    return 0;
}
