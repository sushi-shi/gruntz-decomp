#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/SingleAnimation.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Rez/FrameClock.h>
#include <ZTools/ZDArray.h>

#include <stddef.h>

template<>
CActReg CActRegPool<CSingleAnimation>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

i32 CSingleAnimation::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

CSingleAnimation::CSingleAnimation(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    SET_ANIMATION_ACT("A");
}

void CSingleAnimation::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

void CSingleAnimation::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CSingleAnimation>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CSingleAnimation::AdvanceAnim);
}

i32 CSingleAnimation::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    MARK_OBJECT_COMPLETE_IF(m_wwdObject->m_animationCursor.IsComplete())
    return 0;
}
