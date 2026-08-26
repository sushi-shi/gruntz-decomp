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

RVA_COMPGEN(0x000108a0, 0x1e, ??_GCExitTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x000108d0, 0x44, ??1CExitTrigger@@UAE@XZ)

// @early-stop
RVA(0x0003ec10, 0x292)
CExitTrigger::CExitTrigger(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    SET_ANIMATION_ACT("A");
    SNAP_OBJECT_TO_TILE_CENTER(m_object)
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_EXIT_TRIGGER)
    SET_OBJECT_AREA(1)
    SwitchAnimationByName("GAME_CYCLE100", 0);
    m_warlordLogic = NULL;
    GruntzPlayer* slot = &g_gameReg->m_players[m_object->m_smarts];
    if (slot->m_active == false) {
        m_resolved = false;
        return;
    }
    i32 focusX = m_object->m_screenX;
    i32 focusY = m_object->m_screenY;
    slot->m_focusX = focusX;
    slot->m_focusY = focusY;
    CGameObject* e = g_gameReg->m_world->m_childGroup->CreateSprite(
        0,
        m_object->m_screenX,
        m_object->m_screenY,
        0,
        "Warlord",
        WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
    );
    if (e != NULL) {
        e->m_smarts = m_object->m_smarts;
        e->m_logicRecord->m_dispatch(e);

        m_warlordLogic = static_cast<CWarlord*>(e->m_logicRecord->m_userLogic);
        if (m_object->m_smarts == g_curPlayer) {
            g_gameReg->m_triggerMgr->m_pendingFx = m_warlordLogic;
        }
        GruntzPlayer* slot2 = &g_gameReg->m_players[m_object->m_smarts];
        if (slot2 != NULL) {
            slot2->m_warlordObjectId = e->m_objectId;
        }
    }
    m_resolved = true;
}

// @early-stop
RVA(0x0003ef60, 0x147)
i32 CExitTrigger::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    CFileMemBase* arc = ar;
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_FROM_OR_RETURN(ar, arc, mode, typeId, object)

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
                m_warlordLogic = static_cast<CWarlord*>(obj->m_logicRecord->m_userLogic);
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
