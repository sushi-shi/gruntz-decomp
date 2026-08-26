#include <rva.h>

#include <Gruntz/CursorSnapSprite.h>

#include <Bute/ButeTree.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/LogicRecordDispatchInline.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/UserLogic.h>
#include <Wwd/LogicRecordEvent.h>

RVA(0x00011890, 0x47)
i32 CCursorSnapSprite::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

RVA_COMPGEN(0x00011900, 0x1e, ??_GCCursorSnapSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00011930, 0x44, ??1CCursorSnapSprite@@UAE@XZ)

RVA(0x0003a120, 0xf1)
i32 DispatchCursorSnapSpriteLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CUserLogic* sub = new CCursorSnapSprite(owner);
            sub->Activate();
            record->m_userLogic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x0003a260, 0x16e)
CCursorSnapSprite::CCursorSnapSprite(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("GAME_CURSORSNAPSPRITE");
    SwitchAnimationByName("GAME_SINGLEIMAGEANI", 0);
    SET_ANIMATION_ACT("A");
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    Hide();
}

RVA(0x0003a4d0, 0x102)
void CCursorSnapSprite::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CCursorSnapSprite>::s_table.ResolveEntry(id));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CCursorSnapSprite>::s_table.ResolveEntry(id));
        (this->*((*e2)))();
    }
}
