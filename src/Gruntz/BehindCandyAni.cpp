#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/BehindCandyAni.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/BigAnimationMacros.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyMacros.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>
#include <ZTools/ZDArray.h>

#include <stddef.h>

template<>
CActReg CActRegPool<CBehindCandyAni>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

CBehindCandyAni::CBehindCandyAni(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    INITIALIZE_DEFAULT_CYCLE_ANIMATION
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, 0)
    NORMALIZE_BIG_ANIMATION_WITH_AUX(aux)
}

void CBehindCandyAni::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

void CBehindCandyAni::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CBehindCandyAni>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CBehindCandyAni::AdvanceAnim);
}

i32 CBehindCandyAni::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}
