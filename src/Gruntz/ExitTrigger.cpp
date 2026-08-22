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
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/TileSnapMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/Warlord.h>
#include <Io/FileMem.h>
#include <Utils/MapTyped.h>
#include <Wap32/TileGeometry.h>

#include <stddef.h>

RVA_COMPGEN(0x00010890, 0x1e, ??_GCExitTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x000108c0, 0x44, ??1CExitTrigger@@UAE@XZ)

// @early-stop
RVA(0x0003ecf0, 0x292)
CExitTrigger::CExitTrigger(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(2);
    SET_ANIMATION_ACT("A");
    SNAP_OBJECT_TO_TILE_CENTER(m_object)
    CWwdGameObjectA* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_EXIT_TRIGGER)
    m_object->m_area.left = 1;
    m_object->m_area.right = 1;
    m_object->m_area.top = 1;
    m_object->m_area.bottom = 1;
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_warlordLogic = NULL;
    GruntzPlayer* slot = &g_gameReg->m_options[m_object->m_smarts];
    if (slot->m_liveGate == 0) {
        m_resolved = 0;
        return;
    }
    i32 focusX = m_object->m_screenX;
    i32 focusY = m_object->m_screenY;
    slot->m_focusX = focusX;
    slot->m_focusY = focusY;
    CGameObject* e =
        g_gameReg->m_world->m_childGroup
            ->CreateSprite(0, m_object->m_screenX, m_object->m_screenY, 0, "Warlord", 0x40003);
    if (e != NULL) {
        e->m_smarts = m_object->m_smarts;
        e->m_animWorker->m_notify(e);

        m_warlordLogic = static_cast<CWarlord*>(e->m_animWorker->m_logic);
        if (m_object->m_smarts == g_curPlayer) {
            g_gameReg->m_cmdGrid->m_pendingFx = m_warlordLogic;
        }
        GruntzPlayer* slot2 = &g_gameReg->m_options[m_object->m_smarts];
        if (slot2 != NULL) {
            slot2->m_warlordObjectId = e->m_objectId;
        }
    }
    m_resolved = 1;
}

// @early-stop
// Retail merges the failed-lookup path into eax (`test/je join/mov eax,[found]`), so
// the NULL default costs nothing; cl gives `obj` a callee-saved register and pays one
// `xor esi,esi`. The value-returning and if/else spellings both get if-converted to
// `neg/sbb/and` instead, which is worse.
RVA(0x0003f040, 0x147)
i32 CExitTrigger::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    CFileMemBase* arc = ar;
    if (!Chain(arc, mode, typeId, pObj)) {
        return 0;
    }

    CDDrawSurfaceMgr* holder = g_gameReg->m_world;
    switch (mode) {
        case SERIAL_LOAD: {
            CGameObject* found;
            i32 key;
            arc->Read(&m_resolved, sizeof(m_resolved));
            arc->Read(&key, sizeof(key));
            if (key != 0) {
                found = NULL;
                CGameObject* obj = NULL;
                if (MapLookupById(holder->m_childGroup->m_registeredGameObjectsById, key, found)) {
                    obj = found;
                }
                m_warlordLogic = static_cast<CWarlord*>(obj->m_animWorker->m_logic);
                if (m_warlordLogic == NULL) {
                    return 0;
                }
            } else {
                m_warlordLogic = NULL;
            }
            break;
        }
        case SERIAL_SAVE: {
            arc->Write(&m_resolved, sizeof(m_resolved));
            if (m_warlordLogic == NULL) {
                g_serialCounter++;
                i32 id = 0;
                arc->Write(&id, sizeof(id));
            } else {
                g_serialCounter++;
                i32 id = 0;
                if (m_warlordLogic->m_object != NULL) {
                    id = m_warlordLogic->m_object->m_objectId;
                }
                arc->Write(&id, sizeof(id));
            }
            break;
        }
    }
    return 1;
}
