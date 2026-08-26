#include <rva.h>

#include <Gruntz/SingleAnimation.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Rez/FrameClock.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

RVA_DYNINIT(0x000ae970, 0xa, CActRegPool<CSingleAnimation>::s_table)
RVA_DYNINIT(0x000ae990, 0x15, CActRegPool<CSingleAnimation>::s_table)
RVA_DYNINIT(0x000ae9c0, 0xe, CActRegPool<CSingleAnimation>::s_table)
RVA_DYNINIT(0x000ae9e0, 0x1f, CActRegPool<CSingleAnimation>::s_table)
template<> DATA(0x00246ec8)
CActReg CActRegPool<CSingleAnimation>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x000104b0, 0x47)
i32 CSingleAnimation::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

RVA_COMPGEN(0x00010520, 0x1e, ??_GCSingleAnimation@@UAEPAXI@Z)
RVA_COMPGEN(0x00010550, 0x44, ??1CSingleAnimation@@UAE@XZ)

RVA(0x000ae7e0, 0x13d)
CSingleAnimation::CSingleAnimation(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    SET_ANIMATION_ACT("A");
}

RVA(0x000aea10, 0x102)
void CSingleAnimation::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CSingleAnimation>::s_table.ResolveEntry(id));
    if ((*e) != NULL) {
        (this->*(*((CActRegPool<CSingleAnimation>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x000aeb70, 0x18d)
void CSingleAnimation::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CSingleAnimation>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CSingleAnimation::AdvanceAnim);
}

RVA(0x000aed70, 0x39)
i32 CSingleAnimation::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    MARK_OBJECT_COMPLETE_IF(IsAniCursorComplete(&m_wwdObject->m_animationCursor))
    return 0;
}
