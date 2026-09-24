#ifndef GRUNTZ_GRUNTZ_GRUNTVOICEINLINE_H
#define GRUNTZ_GRUNTZ_GRUNTVOICEINLINE_H

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntVoice.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SerialRefLookup.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/UserLogic.h>
#include <Image/CImage.h>
#include <Utils/MapTyped.h>
#include <Wwd/WwdGameObjectFamily.h>

inline b32 CGruntVoice::PositionIndicatorAtLogicObject() {
    CGameObject* resolved = LookupSerialRef(
        g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
        m_sourceObjectId
    );
    if (resolved == NULL) {
        return false;
    }
    CUserLogic* logic = resolved->m_logicRecord->m_userLogic;
    if (logic == NULL) {
        return false;
    }
    m_object->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    SET_VECTOR2_COMPONENTS(
        m_object->m_screenPosition,
        logic->m_object->m_screenPosition.m_x,
        logic->m_object->m_screenPosition.m_y - 0x32
    );
    return true;
}

inline b32 CGruntVoice::PositionIndicatorAtSourceObject() {
    CGameObject* resolved = LookupSerialRef(
        g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
        m_sourceObjectId
    );

    if (resolved != NULL) {
        m_object->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
        i32 dx = 0, dy = 0;
        CImage* layer = static_cast<CWwdSpriteObject*>(resolved)->m_frameImage;
        if (layer != NULL) {
            dx = layer->m_origin.x;
            dy = layer->m_origin.y;
        }
        SET_VECTOR2_COMPONENTS(
            m_object->m_screenPosition,
            resolved->m_screenPosition.m_x + dx,
            resolved->m_screenPosition.m_y + dy - 0x32
        );
        return true;
    }
    return false;
}

#endif // GRUNTZ_GRUNTZ_GRUNTVOICEINLINE_H
