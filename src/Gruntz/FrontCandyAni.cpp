#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/FrontCandyAni.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>
#include <ZTools/ZDArray.h>

#include <stddef.h>

template<>
CActReg CActRegPool<CFrontCandyAni>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

i32 CFrontCandyAni::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

CFrontCandyAni::CFrontCandyAni(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    INITIALIZE_DEFAULT_CYCLE_ANIMATION
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_OVERLAY)
}

void CFrontCandyAni::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

void CFrontCandyAni::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CFrontCandyAni>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CFrontCandyAni::AdvanceAnim);
}

i32 CFrontCandyAni::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}
