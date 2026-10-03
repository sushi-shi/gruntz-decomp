#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GruntPowerupSprite.h>

#include <Globals.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/ResolveNodeInline.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <ZTools/ZDArray.h>

#include <stddef.h>

template<>
CActReg CActRegPool<CGruntPowerupSprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

CGruntPowerupSprite::CGruntPowerupSprite(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("GAME_LIGHTING_POWERUP");
    SwitchAnimationByName("GAME_CYCLE100", 0);
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_POWERUP)
    Hide();
}

void CGruntPowerupSprite::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

void CGruntPowerupSprite::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CGruntPowerupSprite>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntPowerupSprite::Update);
}

i32 CGruntPowerupSprite::BindToGrunt(i32 playerIndex, i32 unitIndex, i32 powerupId) {
    m_gruntIdentity.m_playerIndex = playerIndex;
    m_gruntIdentity.m_unitIndex = unitIndex;
    m_powerupId = powerupId;
    CShadeTable* rec = g_gameReg->m_lightFxMgr->m_tables[powerupId];
    CWwdSpriteObject* r = m_object;
    r->SetDrawFill(SHADE_DST_BY_SRC_16, rec);
    m_wwdObject->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    SET_ANIMATION_ACT("A");
    return 1;
}

i32 CGruntPowerupSprite::Update() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    CGrunt* e =
        g_gameReg->m_triggerMgr->UnitAt(m_gruntIdentity.m_playerIndex, m_gruntIdentity.m_unitIndex);
    if (e != NULL) {
        SET_SCREEN_POS(m_object, e->m_object->m_screenX, e->m_object->m_screenY);
    }
    return 0;
}

i32 CGruntPowerupSprite::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_gruntIdentity, sizeof(m_gruntIdentity));
            ar->Write(&m_powerupId, sizeof(m_powerupId));
            break;
        case SERIAL_LOAD: {
            ar->Read(&m_gruntIdentity, sizeof(m_gruntIdentity));
            ar->Read(&m_powerupId, sizeof(m_powerupId));
            i32 id = m_powerupId;
            CWwdSpriteObject* r = m_object;
            CShadeTable* v = g_gameReg->m_lightFxMgr->m_tables[id];
            r->SetDrawFillReversed(SHADE_DST_BY_SRC_16, v);
            break;
        }
    }
    return 1;
}
