#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/AniCycle.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Rez/FrameClock.h>
#include <ZTools/ZDArray.h>

#include <stddef.h>

template<>
CActReg CActRegPool<CAniCycle>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

CAniCycle::CAniCycle(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));
    if (m_wwdObject->m_animationCursor.m_animation == NULL) {
        SwitchAnimationByName("GAME_CYCLE100", 0);
    }
    SET_ANIMATION_ACT("A");
}

void CAniCycle::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

void CAniCycle::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CAniCycle>::s_table[id]) = static_cast<CActHandler>(&CAniCycle::AdvanceAnim);
}

i32 CAniCycle::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}
