#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/SimpleAnimation.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/BigAnimationMacros.h>
#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TypeKeyColl.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>
#include <ZTools/BitVec.h>
#include <ZTools/ZDArray.h>

i32 CSimpleAnimation::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

template<>
CActReg CActRegPool<CSimpleAnimation>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

CSimpleAnimation::CSimpleAnimation(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SET_ANIMATION_ACT("A");
    NORMALIZE_BIG_ANIMATION_WITH_AUX(m_object->m_frameImage)
}

void CSimpleAnimation::FireActivation(i32 idx) {
    DispatchRegisteredAct(this, idx);
}

void RegisterSimpleAnimLogic() {
    ACT_NAME_ID(idx, "A")
    CActHandler* dslot = &CActRegPool<CSimpleAnimation>::s_table[idx];
    *dslot = static_cast<CActHandler>(&CSimpleAnimation::AdvanceAnim);
}

i32 CSimpleAnimation::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}
