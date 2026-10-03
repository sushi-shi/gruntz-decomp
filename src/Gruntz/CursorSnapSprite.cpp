#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/CursorSnapSprite.h>

#include <Gruntz/ActRegistry.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/LogicEventDispatch.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/UserLogic.h>
#include <Wwd/LogicRecordEvent.h>

i32 CCursorSnapSprite::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

i32 DispatchCursorSnapSpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CCursorSnapSprite)
}

CCursorSnapSprite::CCursorSnapSprite(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("GAME_CURSORSNAPSPRITE");
    SwitchAnimationByName("GAME_SINGLEIMAGEANI", 0);
    SET_ANIMATION_ACT("A");
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    Hide();
}

void CCursorSnapSprite::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}
