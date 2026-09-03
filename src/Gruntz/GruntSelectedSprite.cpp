#include <rva.h>

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
#include <Wap32/ZVec.h>

#include <stddef.h>

RVA_DYNINIT(0x0007e5c0, 0xa, CActRegPool<CGruntSelectedSprite>::s_table)
RVA_DYNINIT(0x0007e5e0, 0x15, CActRegPool<CGruntSelectedSprite>::s_table)
RVA_DYNINIT(0x0007e610, 0xe, CActRegPool<CGruntSelectedSprite>::s_table)
RVA_DYNINIT(0x0007e630, 0x1f, CActRegPool<CGruntSelectedSprite>::s_table)
template<> DATA(0x00244da8)
CActReg CActRegPool<CGruntSelectedSprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_COMPGEN(0x00011e50, 0x1e, ??_GCGruntSelectedSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00011e80, 0x44, ??1CGruntSelectedSprite@@UAE@XZ)

RVA(0x0007e3e0, 0x178)
CGruntSelectedSprite::CGruntSelectedSprite(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("GAME_GRUNTSELECTEDSPRITE");
    SwitchAnimationByName("GAME_GRUNTSELECTEDSPRITE", 0);
    SET_ANIMATION_ACT("A");
    CWwdSpriteObject* o = m_object;
    o->SetSortKey(SORTKEY_GRUNT_SELECTED);
}

RVA(0x0007e660, 0x102)
void CGruntSelectedSprite::FireActivation(i32 id) {
    if ((*((CActRegPool<CGruntSelectedSprite>::s_table.ResolveEntry(id)))) != NULL) {
        (this->*(*((CActRegPool<CGruntSelectedSprite>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x0007e7c0, 0x18d)
void CGruntSelectedSprite::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CGruntSelectedSprite>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntSelectedSprite::Update);
}

RVA(0x0007e9c0, 0x16)
i32 CGruntSelectedSprite::BindToGrunt(i32 playerIndex, i32 unitIndex) {
    m_gruntIdentity.m_playerIndex = playerIndex;
    m_gruntIdentity.m_unitIndex = unitIndex;
    return 1;
}

// @early-stop
RVA(0x0007e9f0, 0x5f)
i32 CGruntSelectedSprite::Update() {
    CGruntzMgr* reg = g_gameReg;
    CGrunt* e =
        reg->m_triggerMgr->m_units
            [m_gruntIdentity.m_unitIndex + m_gruntIdentity.m_playerIndex * TM_UNITS_PER_PLAYER];
    if (e != NULL && e->m_arrived != false) {
        m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
        m_object->SetScreenPos(e->m_object->ScreenPos());
    }
    return 0;
}

RVA(0x0007ea70, 0x6f)
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
