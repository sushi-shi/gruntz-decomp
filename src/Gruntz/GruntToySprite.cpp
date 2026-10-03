#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GruntToySprite.h>

#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <ZTools/ZDArray.h>

#include <stddef.h>

template<>
CActReg CActRegPool<CGruntToySprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

CGruntToySprite::CGruntToySprite(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageFrameByName("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", 0);
    SET_ANIMATION_ACT("A");
    Hide();
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_HUD)
    m_lastLayer = PICKUP_NONE;
}

void CGruntToySprite::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

void CGruntToySprite::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CGruntToySprite>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntToySprite::Update);
}

i32 CGruntToySprite::BindToGrunt(i32 playerIndex, i32 unitIndex) {
    m_gruntIdentity.m_playerIndex = playerIndex;
    m_gruntIdentity.m_unitIndex = unitIndex;
    m_wwdObject->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    return 1;
}

i32 CGruntToySprite::Update() {
    CGrunt* e =
        g_gameReg->m_triggerMgr->UnitAt(m_gruntIdentity.m_playerIndex, m_gruntIdentity.m_unitIndex);
    if (e == NULL) {
        return 0;
    }
    PickupType layer = e->m_vehiclePickupType;
    if (m_lastLayer != layer) {
        m_lastLayer = layer;
        m_object->SetImageFrame(IDX(layer));
    }
    SET_SCREEN_POS(m_object, e->m_object->m_screenX, e->m_object->m_screenY - 0x20);
    return 0;
}

i32 CGruntToySprite::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_gruntIdentity, sizeof(m_gruntIdentity));
            ar->Write(&m_lastLayer, sizeof(m_lastLayer));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_gruntIdentity, sizeof(m_gruntIdentity));
            ar->Read(&m_lastLayer, sizeof(m_lastLayer));
            break;
    }
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}
