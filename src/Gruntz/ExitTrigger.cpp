#include <Gruntz/ExitTrigger.h>
#include <Gruntz/Warlord.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/TypeKeyColl.h>
#include <Io/FileMem.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/LogicTypeId.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialArchive.h>

#include <rva.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Utils/MapTyped.h>
RVA_COMPGEN(0x00010890, 0x1e, ??_GCExitTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x000108c0, 0x44, ??1CExitTrigger@@UAE@XZ)

VTBL(CExitTrigger, 0x001e822c);

// @early-stop
RVA(0x0003ecf0, 0x292)
CExitTrigger::CExitTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_flags |= 2;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
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
            slot2->m_00c = e->m_188;
        }
    }
    m_resolved = 1;
}

// @early-stop
RVA(0x0003f040, 0x147)
i32 CExitTrigger::SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* pObj) {
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    CFileMemBase* arc = ar;
    if (!Chain(arc, mode, typeId, pObj)) {
        return 0;
    }

    CDDrawSurfaceMgr* holder = g_gameReg->m_world;
    switch (mode) {
        case 7: {
            arc->Read(&m_resolved, 4);
            i32 key = 0;
            arc->Read(&key, 4);
            if (key != 0) {
                CGameObject* found = 0;

                CGameObject* obj = 0;

                if (MapLookupById(holder->m_childGroup->m_map48, key, found)) {
                    obj = found;
                }
                m_warlordLogic = static_cast<CWarlord*>(obj->m_animWorker->m_logic);
                if (m_warlordLogic == 0) {
                    return 0;
                }
            } else {
                m_warlordLogic = 0;
            }
            break;
        }
        case 4: {
            arc->Write(&m_resolved, 4);
            if (m_warlordLogic == 0) {
                g_serialCounter++;
                i32 id = 0;
                arc->Write(&id, 4);
            } else {
                g_serialCounter++;
                i32 id = 0;
                if (m_warlordLogic->m_object != 0) {
                    id = m_warlordLogic->m_object->m_188;
                }
                arc->Write(&id, 4);
            }
            break;
        }
    }
    return 1;
}
