#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/StatusBarSpriteActs.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/StatusBarSprite.h>
#include <Gruntz/TileTriggerTransition.h>
#include <Gruntz/UserLogic.h>
#include <Rez/FrameClock.h>
#include <Wwd/LogicRecordEvent.h>
#include <ZTools/ZDArray.h>

#include <stddef.h>

template<>
CActReg CActRegPool<CStatusBarSprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

i32 DispatchStatusBarSpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CStatusBarSprite)
}

CStatusBarSprite::CStatusBarSprite(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("GAME_STATUSBARSPRITE");
    SwitchAnimationByName("GAME_SINGLEIMAGEANI", 0);
    SET_ANIMATION_ACT("A");
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_OVERLAY)
}

void CStatusBarSprite::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

void CStatusBarSprite::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CStatusBarSprite>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CStatusBarSprite::AdvanceAnim);
}

i32 CStatusBarSprite::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}
