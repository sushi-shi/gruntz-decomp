#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GruntSelectedSprite.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <ZTools/ZDArray.h>

#include <stddef.h>

template<>
CActReg CActRegPool<CGruntSelectedSprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

CGruntSelectedSprite::CGruntSelectedSprite(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("GAME_GRUNTSELECTEDSPRITE");
    SwitchAnimationByName("GAME_GRUNTSELECTEDSPRITE", 0);
    SET_ANIMATION_ACT("A");
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_SELECTED)
}

void CGruntSelectedSprite::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

void CGruntSelectedSprite::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CGruntSelectedSprite>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntSelectedSprite::Update);
}

i32 CGruntSelectedSprite::BindToGrunt(i32 playerIndex, i32 unitIndex) {
    m_gruntIdentity.m_playerIndex = playerIndex;
    m_gruntIdentity.m_unitIndex = unitIndex;
    return 1;
}

i32 CGruntSelectedSprite::Update() {
    CGrunt* e =
        g_gameReg->m_triggerMgr->UnitAt(m_gruntIdentity.m_playerIndex, m_gruntIdentity.m_unitIndex);
    if (e != NULL && e->m_arrived != false) {
        m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
        m_object->m_screenX = e->m_object->m_screenX;
        m_object->m_screenY = e->m_object->m_screenY;
    }
    return 0;
}

i32 CGruntSelectedSprite::SerializeDispatch(
    CFileMemBase* arc,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    CFileMemBase* sa = static_cast<CFileMemBase*>(arc);

    if (mode != SERIAL_SAVE) {
        if (mode == SERIAL_LOAD) {
            sa->Read(&m_gruntIdentity, sizeof(m_gruntIdentity));
        }
    } else {
        sa->Write(&m_gruntIdentity, sizeof(m_gruntIdentity));
    }
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_FROM(arc, sa, mode, typeId, object)
}
