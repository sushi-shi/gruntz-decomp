#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/EyeCandyAni.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/BigAnimationMacros.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>

template<>
CActReg CActRegPool<CEyeCandyAni>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

i32 CEyeCandyAni::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

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

void CEyeCandyAni::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

void CEyeCandyAni::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CEyeCandyAni>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CEyeCandyAni::AdvanceAnim);
}

i32 CEyeCandyAni::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}
