#include <rva.h>

#include <Gruntz/ExitTrigger.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/Warlord.h>
#include <Io/FileMem.h>
#include <Utils/MapTyped.h>

RVA_COMPGEN(0x00010890, 0x1e, ??_GCExitTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x000108c0, 0x44, ??1CExitTrigger@@UAE@XZ)

VTBL(CExitTrigger, 0x001e822c);

// @early-stop
RVA(0x0003ecf0, 0x292)
CExitTrigger::CExitTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_flags |= 2;
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    if (m_object->m_sortKey != 0x124f8) {
        m_object->m_sortKey = 0x124f8;
        m_object->m_flags |= 0x20000;
    }
    m_object->m_area.left = 1;
    m_object->m_area.right = 1;
    m_object->m_area.top = 1;
    m_object->m_area.bottom = 1;
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_warlordLogic = 0;
    GruntzPlayer* slot = &g_gameReg->m_options[m_object->m_smarts];
    if (slot->m_liveGate == 0) {
        m_resolved = 0;
        return;
    }
    slot->m_focusX = m_object->m_screenX;
    slot->m_focusY = m_object->m_screenY;
    CGameObject* e =
        g_gameReg->m_world->m_childGroup
            ->CreateSprite(0, m_object->m_screenX, m_object->m_screenY, 0, "Warlord", 0x40003);
    if (e != 0) {
        e->m_smarts = m_object->m_smarts;
        e->m_animWorker->m_notify(e);

        m_warlordLogic = static_cast<CWarlord*>(e->m_animWorker->m_logic);
        if (m_object->m_smarts == g_curPlayer) {
            g_gameReg->m_cmdGrid->m_pendingFx = m_warlordLogic;
        }
        GruntzPlayer* slot2 = &g_gameReg->m_options[m_object->m_smarts];
        if (slot2 != 0) {
            slot2->m_warlordObjectId = e->m_objectId;
        }
    }
    m_resolved = 1;
}

// @identity-TODO SerializeMove (327 B) sits outside this TU's block at 0x3f040, between
// ?0CExitTrigger (exittrigger) and FireActivation (wormholeacts). No size-family and too
// large for a dtor pool - the placement is UNEXPLAINED; find its real owner.
