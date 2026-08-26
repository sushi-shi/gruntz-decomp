#include <rva.h>

#include <Gruntz/FrontCandyAni.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/BehindCandy.h>
#include <Gruntz/BigAnimationMacros.h>
#include <Gruntz/DoNothing.h>
#include <Gruntz/EyeCandy.h>
#include <Gruntz/EyeCandyAni.h>
#include <Gruntz/FrontCandy.h>
#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

RVA_DYNINIT(0x000ad100, 0xa, CActRegPool<CFrontCandyAni>::s_table)
RVA_DYNINIT(0x000ad120, 0x15, CActRegPool<CFrontCandyAni>::s_table)
RVA_DYNINIT(0x000ad150, 0xe, CActRegPool<CFrontCandyAni>::s_table)
RVA_DYNINIT(0x000ad170, 0x1f, CActRegPool<CFrontCandyAni>::s_table)
template<> DATA(0x00247008)
CActReg CActRegPool<CFrontCandyAni>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x0000fa70, 0x47)
i32 CFrontCandy::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

RVA_COMPGEN(0x0000fae0, 0x1e, ??_GCFrontCandy@@UAEPAXI@Z)
RVA_COMPGEN(0x0000fb10, 0x44, ??1CFrontCandy@@UAE@XZ)

RVA(0x0000fe00, 0x47)
i32 CFrontCandyAni::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

RVA_COMPGEN(0x0000fe70, 0x1e, ??_GCFrontCandyAni@@UAEPAXI@Z)
RVA_COMPGEN(0x0000fea0, 0x44, ??1CFrontCandyAni@@UAE@XZ)

RVA(0x0000ff30, 0x47)
i32 CEyeCandyAni::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

RVA_COMPGEN(0x0000ffa0, 0x1e, ??_GCEyeCandyAni@@UAEPAXI@Z)
RVA_COMPGEN(0x0000ffd0, 0x44, ??1CEyeCandyAni@@UAE@XZ)

// @early-stop
RVA(0x000abf90, 0x1b6)
CFrontCandy::CFrontCandy(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_OVERLAY)
    NORMALIZE_BIG_ANIMATION_WITH_AUX(m_object->m_frameImage)
}

// @early-stop
RVA(0x000ac1c0, 0x1a5)
CDoNothing::CDoNothing(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));
    NORMALIZE_BIG_ANIMATION_WITH_AUX(m_object->m_frameImage)
}

// @early-stop
RVA(0x000ac3e0, 0x1b1)
CBehindCandy::CBehindCandy(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, 0)
    NORMALIZE_BIG_ANIMATION_WITH_AUX(m_object->m_frameImage)
}

// @early-stop
RVA(0x000ac610, 0x1cf)
CEyeCandy::CEyeCandy(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    CWwdSpriteObject* o = m_object;
    if (o->m_sortKey == 0 && o->m_frameImage != NULL) {
        i32 v = o->m_frameImage->m_anchorY + o->m_screenY + 0x186a0;
        SET_SORT_KEY_IF_CHANGED(o, v)
    }
    NORMALIZE_BIG_ANIMATION_WITH_AUX(m_object->m_frameImage)
}

// @early-stop
RVA(0x000ac860, 0x20e)
CEyeCandyAni::CEyeCandyAni(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    INITIALIZE_DEFAULT_CYCLE_ANIMATION
    CWwdSpriteObject* o = m_object;
    if (o->m_sortKey == 0 && o->m_frameImage != NULL) {
        i32 v = o->m_frameImage->m_anchorY + o->m_screenY + 0x186a0;
        SET_SORT_KEY_IF_CHANGED(o, v)
    }
    NORMALIZE_BIG_ANIMATION_WITH_AUX(m_object->m_frameImage)
}

RVA(0x000acba0, 0x102)
void CEyeCandyAni::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CEyeCandyAni>::s_table.ResolveEntry(id));
    if ((*e) != NULL) {
        (this->*(*((CActRegPool<CEyeCandyAni>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x000acd00, 0x18d)
void CEyeCandyAni::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CEyeCandyAni>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CEyeCandyAni::AdvanceAnim);
}

RVA(0x000acf00, 0x17)
i32 CEyeCandyAni::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}

RVA(0x000acf30, 0x16e)
CFrontCandyAni::CFrontCandyAni(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    INITIALIZE_DEFAULT_CYCLE_ANIMATION
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_OVERLAY)
}

RVA(0x000ad1a0, 0x102)
void CFrontCandyAni::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CFrontCandyAni>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CFrontCandyAni>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x000ad300, 0x18d)
void CFrontCandyAni::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CFrontCandyAni>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CFrontCandyAni::AdvanceAnim);
}

RVA(0x000ad500, 0x17)
i32 CFrontCandyAni::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}
