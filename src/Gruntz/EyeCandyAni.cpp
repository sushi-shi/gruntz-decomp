#include <rva.h>

#include <Gruntz/EyeCandyAni.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/BigAnimationMacros.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>

RVA_DYNINIT(0x000acb10, 0xa, CActRegPool<CEyeCandyAni>::s_table)
RVA_DYNINIT(0x000acb30, 0x15, CActRegPool<CEyeCandyAni>::s_table)
RVA_DYNINIT(0x000acb60, 0xe, CActRegPool<CEyeCandyAni>::s_table)
RVA_DYNINIT(0x000acb80, 0x1f, CActRegPool<CEyeCandyAni>::s_table)
template<> DATA(0x00246060)
CActReg CActRegPool<CEyeCandyAni>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x0000ff20, 0x47)
i32 CEyeCandyAni::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

RVA_COMPGEN(0x0000ff90, 0x1e, ??_GCEyeCandyAni@@UAEPAXI@Z)
RVA_COMPGEN(0x0000ffc0, 0x44, ??1CEyeCandyAni@@UAE@XZ)

// @early-stop
RVA(0x000ac870, 0x20e)
CEyeCandyAni::CEyeCandyAni(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    INITIALIZE_DEFAULT_CYCLE_ANIMATION
    CWwdSpriteObject* o = m_object;
    if (o->m_sortKey == 0 && o->m_frameImage != NULL) {
        i32 v = o->m_frameImage->m_anchor.y + o->m_screenPosition.m_y + 0x186a0;
        SET_SORT_KEY_IF_CHANGED(o, v)
    }
    NORMALIZE_BIG_ANIMATION_WITH_AUX(m_object->m_frameImage)
}

RVA(0x000acbb0, 0x102)
void CEyeCandyAni::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

RVA(0x000acd10, 0x18d)
void CEyeCandyAni::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CEyeCandyAni>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CEyeCandyAni::AdvanceAnim);
}

RVA(0x000acf10, 0x17)
i32 CEyeCandyAni::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}
