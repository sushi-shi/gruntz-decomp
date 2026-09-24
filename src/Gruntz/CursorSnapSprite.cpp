#include <rva.h>

#include <Gruntz/CursorSnapSprite.h>

#include <Bute/ButeTree.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/LogicEventDispatch.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/UserLogic.h>
#include <Wwd/LogicRecordEvent.h>

RVA(0x00011880, 0x47)
i32 CCursorSnapSprite::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

RVA_COMPGEN(0x000118f0, 0x1e, ??_GCCursorSnapSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00011920, 0x44, ??1CCursorSnapSprite@@UAE@XZ)

RVA(0x0003a200, 0xf1)
i32 DispatchCursorSnapSpriteLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CCursorSnapSprite)}

RVA(0x0003a340, 0x16e)
CCursorSnapSprite::CCursorSnapSprite(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("GAME_CURSORSNAPSPRITE");
    SwitchAnimationByName("GAME_SINGLEIMAGEANI", 0);
    SET_ANIMATION_ACT("A");
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    Hide();
}

RVA(0x0003a5b0, 0x102)
void CCursorSnapSprite::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}
