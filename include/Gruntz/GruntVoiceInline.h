#ifndef GRUNTZ_GRUNTZ_GRUNTVOICEINLINE_H
#define GRUNTZ_GRUNTZ_GRUNTVOICEINLINE_H

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntVoice.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/UserLogic.h>
#include <Image/CImage.h>
#include <Utils/MapTyped.h>
#include <Wwd/WwdGameObjectFamily.h>

inline b32 CGruntVoice::PositionIndicatorAtLogicObject() {
    CGameObject* out = NULL;
    i32 sourceObjectId = m_sourceObjectId;
    CGameObject* resolved;
    if (MapLookupById(
            g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
            sourceObjectId,
            out
        )
        == false) {
        resolved = NULL;
    } else if (out == NULL) {
        resolved = NULL;
    } else {
        resolved = (out->GetClassId() == CLASSID_SERIALREF) ? out : NULL;
    }
    if (resolved == NULL) {
        return false;
    }
    CUserLogic* logic = resolved->m_logicRecord->m_userLogic;
    if (logic == NULL) {
        return false;
    }
    m_object->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    m_object->m_screenX = logic->m_object->m_screenX;
    m_object->m_screenY = logic->m_object->m_screenY - 0x32;
    return true;
}

inline b32 CGruntVoice::PositionIndicatorAtSourceObject() {
    CGameObject* out = NULL;
    i32 sourceObjectId = m_sourceObjectId;
    CGameObject* resolved;
    if (MapLookupById(
            g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
            sourceObjectId,
            out
        )
        == false) {
        resolved = NULL;
    } else if (out == NULL) {
        resolved = NULL;
    } else {
        resolved = (out->GetClassId() == CLASSID_SERIALREF) ? out : NULL;
    }

    if (resolved != NULL) {
        m_object->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
        i32 dx = 0, dy = 0;
        CImage* layer = static_cast<CWwdSpriteObject*>(resolved)->m_frameImage;
        if (layer != NULL) {
            dx = layer->m_originX;
            dy = layer->m_originY;
        }
        m_object->m_screenX = resolved->m_screenX + dx;
        m_object->m_screenY = resolved->m_screenY + dy - 0x32;
        return true;
    }
    return false;
}

#endif // GRUNTZ_GRUNTZ_GRUNTVOICEINLINE_H
