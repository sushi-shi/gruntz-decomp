#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GruntHealthSprite.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntCellInline.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HealthPct.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Lith/BDefs.h>
#include <ZTools/ZDArray.h>

#include <stddef.h>

template<>
CActReg CActRegPool<CGruntHealthSprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

CGruntHealthSprite::CGruntHealthSprite(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageFrameByName("GAME_GRUNTHEALTHSPRITE", 1);
    SET_ANIMATION_ACT("A");
    m_displayedValue = HEALTH_FULL;
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_HUD)
    m_yOffset = -0x19;
}

void CGruntHealthSprite::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

void CGruntHealthSprite::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CGruntHealthSprite>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntHealthSprite::HealthUpdate);
}

i32 CGruntHealthSprite::BindToGrunt(i32 playerIndex, i32 unitIndex, i32 displayedValue) {
    m_gruntIdentity.m_playerIndex = playerIndex;
    m_gruntIdentity.m_unitIndex = unitIndex;
    i32 slot = 0x15 - ROUND(static_cast<double>(displayedValue) * 0.2);
    m_object->SetImageFrame(slot);
    m_displayedValue = displayedValue;
    return 1;
}

i32 CGruntHealthSprite::GetDisplayedValue(CGrunt* g) {
    return g->m_health;
}

i32 CGruntHealthSprite::HealthUpdate() {

    CGrunt* e = FindGruntByIdentity(g_gameReg, m_gruntIdentity);
    if (e == NULL) {
        return 0;
    }
    i32 result = GetDisplayedValue(e);
    if (m_displayedValue != result) {
        i32 slot = 0x15 - ROUND(static_cast<double>(result) * 0.2);
        m_object->SetImageFrame(slot);
        m_displayedValue = result;
    }
    m_object->m_screenX = e->m_object->m_screenX;
    m_object->m_screenY = m_yOffset + e->m_object->m_screenY;
    return 0;
}

i32 CGruntHealthSprite::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_gruntIdentity, sizeof(m_gruntIdentity));
            ar->Write(&m_displayedValue, sizeof(m_displayedValue));
            ar->Write(&m_yOffset, sizeof(m_yOffset));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_gruntIdentity, sizeof(m_gruntIdentity));
            ar->Read(&m_displayedValue, sizeof(m_displayedValue));
            ar->Read(&m_yOffset, sizeof(m_yOffset));
            break;
    }
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}
