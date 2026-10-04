#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/WormholeActs.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Globals.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/ExitTrigger.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStats.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/Play.h>
#include <Gruntz/ResolveNodeInline.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/Warlord.h>
#include <Gruntz/Wormhole.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/WwdGameObjectFamily.h>
#include <Wwd/WwdObjMgrInline.h>
#include <ZTools/ZDArray.h>

#include <stddef.h>

RVA_DYNINIT(0x0003f1f0, 0xa, CActRegPool<CExitTrigger>::s_table)
RVA_DYNINIT(0x0003f210, 0x15, CActRegPool<CExitTrigger>::s_table)
RVA_DYNINIT(0x0003f240, 0xe, CActRegPool<CExitTrigger>::s_table)
RVA_DYNINIT(0x0003f260, 0x1f, CActRegPool<CExitTrigger>::s_table)
template<> DATA(0x002445c0)
CActReg CActRegPool<CExitTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x0003f290, 0x102)
void CExitTrigger::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

RVA(0x0003f3f0, 0x18d)
void CExitTrigger::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CExitTrigger>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CExitTrigger::AdvanceAnim);
}

RVA(0x0003f5f0, 0x526)
i32 CExitTrigger::AdvanceAnim() {
    m_wwdObject->GetAnimationCursor().Advance(g_engineFrameDelta);
    if (g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
        CWwdSpriteObject* trig = m_object;
        CTriggerMgr::HitSpanArg span;
        span.m_span = &trig->m_area;
        g_gameReg->GetTriggerMgr()->CheckWarpStoneExit(trig->m_screenX, trig->m_screenY, span);
    } else if (m_resolved != false) {
        i32 hitPlayerIndex;
        i32 hitUnitIndex;
        CWwdSpriteObject* obj = m_object;
        if (g_gameReg->GetTriggerMgr()->FindGruntInArea(
                obj->m_screenX,
                obj->m_screenY,
                &obj->m_area,
                &hitPlayerIndex,
                &hitUnitIndex,
                NULL
            )
            != NULL) {
            i32 owningPlayer = m_object->GetSmarts();
            if (hitPlayerIndex == owningPlayer) {
                return 0;
            }
            m_resolved = false;
            GruntzPlayer* loser = &g_gameReg->GetPlayer(owningPlayer);
            GruntzPlayer* winner = &g_gameReg->GetPlayer(hitPlayerIndex);
            if (loser != NULL) {
                g_gameReg->ChatLog()->AddMessage(
                    static_cast<const char*>(
                        loser->GetName() + " was conquered by " + winner->GetName()
                            + DATA_COMPGEN(0x0020d168, "!")
                        ),
                        GAME_TEXT_FLAGS_NONE,
                        0x11
                );
                loser->SetEliminated(true);
            }
            g_gameReg->GetGameStats()->RecordFlagCapture(hitPlayerIndex, owningPlayer);
            g_gameReg->GetTriggerMgr()->StartPlayerDefeatSequence(owningPlayer);
            g_gameReg->GetTriggerMgr()
                ->StartUnitDeath(hitPlayerIndex, hitUnitIndex, DEATH_EXIT, -1);
            if (m_warlordLogic != NULL) {
                m_warlordLogic->ResolveDeathAnimation();
                m_warlordLogic = NULL;
            }
            GruntzPlayer* claimed = &g_gameReg->GetPlayer(hitPlayerIndex);
            if (claimed != NULL) {
                CGameObject* warlordObj = LookupObjectById(
                    g_gameReg->World()->ChildGroup()->m_registeredGameObjectsById,
                    claimed->GetWarlordObjectId()
                );
                CWarlord* wl = static_cast<CWarlord*>(warlordObj->GetLogicRecord()->UserLogic());
                if (wl != NULL) {
                    wl->ResolveJoyAnimation();
                }
            }
            CDDrawChildGroup* grp = g_gameReg->World()->ChildGroup();
            POSITION pos = grp->GetHeadPosition();
            while (pos != NULL) {
                CGameObject* cur = grp->NextChild(pos);
                if (cur->GetLogicRecord()->GetDispatch() == DispatchGruntCreationPointLogic
                    && cur->GetSmarts() == owningPlayer) {
                    cur->SetSmarts(hitPlayerIndex);
                    CShadeTable* tbl = g_gameReg->GruntPalettes()->GetShadeTable(
                        IDX(g_gameReg->GetPlayer(hitPlayerIndex).GetColor()),
                        0
                    );
                    cur->SetDrawFill(SHADE_PAL_16, tbl);
                    if (hitPlayerIndex == g_curPlayer) {
                        Coord* mark = g_coordPool.Pop();
                        mark->m_x = (cur->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
                        mark->m_y = (cur->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
                        CPtrArray& marks =
                            static_cast<CPlay*>(g_gameReg->GetCurrentState())->m_startMarkers;
                        marks.Add(mark);
                    }
                }
                if (cur->GetLogicRecord()->GetDispatch() == DispatchFortressFlagLogic
                    && cur->GetSmarts() == owningPlayer) {
                    cur->SetSmarts(hitPlayerIndex);
                    CShadeTable* tbl = g_gameReg->GruntPalettes()->GetShadeTable(
                        IDX(g_gameReg->GetPlayer(hitPlayerIndex).GetColor()),
                        0
                    );
                    cur->SetDrawFill(SHADE_PAL_16, tbl);
                }
            }
            if (owningPlayer == g_curPlayer) {
                g_gameReg->GetTriggerMgr()->BeginLevelFinish(FINISH_REASON_BATTLEZ_DEFEAT);
            } else {
                GruntzPlayer* board = &g_gameReg->GetPlayer(owningPlayer);
                if (board != NULL && board->IsHumanControlled() == false) {
                    board->GetBattlezAiController()->Deactivate();
                }
            }
        } else {

            i32 lostPlayer = m_object->GetSmarts();
            if (lostPlayer == g_curPlayer) {
                return 0;
            }
            GruntzPlayer* slot = &g_gameReg->GetPlayer(lostPlayer);
            if (slot->HasJoinedRound() == false) {
                return 0;
            }
            if (slot->IsEliminated() != false) {
                return 0;
            }
            if (slot->HasDropped() == false) {
                return 0;
            }
            slot->SetEliminated(true);
            m_resolved = false;
            if (m_warlordLogic != NULL) {
                m_warlordLogic->ResolveDeathAnimation();
                m_warlordLogic = NULL;
            }
            CDDrawChildGroup* grp = g_gameReg->World()->ChildGroup();
            POSITION pos = grp->GetHeadPosition();
            while (pos != NULL) {
                CGameObject* cur = grp->NextChild(pos);
                LogicRecordDispatchFn dispatch = cur->GetLogicRecord()->GetDispatch();
                if (dispatch == DispatchGruntCreationPointLogic
                    || dispatch == DispatchFortressFlagLogic) {
                    if (cur->GetSmarts() == m_object->GetSmarts()) {
                        i32 x = cur->m_screenX;
                        i32 y = cur->m_screenY;
                        if (::PtInRect(g_gameReg->GetViewBounds(), x, y)) {
                            CWwdSpriteObject* fx = g_gameReg->World()->ChildGroup()->CreateSprite(
                                0,
                                x,
                                y,
                                SORTKEY_OVERLAY,
                                "Explosion",
                                WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
                            );
                            if (fx != NULL) {
                                fx->SetAnimationByName("GAME_EXPLOSION3", 0);
                                fx->SetSmarts(0);
                                fx->SetScore(0);
                            }
                        }
                        cur->AddFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
                    }
                }
            }
            g_gameReg->GetTriggerMgr()->StartPlayerVictorySequence(m_object->GetSmarts());
        }
    }
    return 0;
}
