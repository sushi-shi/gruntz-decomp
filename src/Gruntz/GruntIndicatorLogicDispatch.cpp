#include <rva.h>

#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/GruntPowerupSprite.h>
#include <Gruntz/GruntSelectedSprite.h>
#include <Gruntz/GruntStaminaSprite.h>
#include <Gruntz/GruntToySprite.h>
#include <Gruntz/GruntToyTimeSprite.h>
#include <Gruntz/GruntWingzTimeSprite.h>
#include <Gruntz/LogicRecordDispatchInline.h>
#include <Gruntz/UserLogic.h>
#include <Wwd/LogicRecordEvent.h>

#define LOGIC_RECORD_DISPATCH(LEAF)                                                                \
    CLogicRecord* record = owner->m_logicRecord;                                                   \
    switch (record->LogicEvent()) {                                                                \
        case ACT_UNINITIALISED: {                                                                  \
            record->SetLogicEvent(ACT_LIVE);                                                       \
            CUserLogic* sub = new LEAF(owner);                                                     \
            sub->Activate();                                                                       \
            record->m_userLogic = sub;                                                             \
            break;                                                                                 \
        }                                                                                          \
        case ACT_OBJECT_REMOVED:                                                                   \
            record->m_userLogic->OnObjectRemoved();                                                \
            break;                                                                                 \
        case ACT_LEAVE_ACTIVE_REGION:                                                              \
            record->m_userLogic->OnLeaveActiveRegion();                                            \
            break;                                                                                 \
        case ACT_PREPARE_SAVE:                                                                     \
            record->m_userLogic->PrepareSave();                                                    \
            break;                                                                                 \
        case ACT_AFTER_LOAD_REFERENCES:                                                            \
            record->m_userLogic->AfterLoadReferences();                                            \
            break;                                                                                 \
        case ACT_AFTER_LOAD:                                                                       \
            record->m_userLogic->AfterLoad();                                                      \
            break;                                                                                 \
        case ACT_AFTER_SAVE:                                                                       \
            record->m_userLogic->AfterSave();                                                      \
            break;                                                                                 \
        case ACT_LIVE:                                                                             \
            break;                                                                                 \
        default:                                                                                   \
            DispatchUnhandledLogicEvent(record->m_userLogic);                                      \
            break;                                                                                 \
    }                                                                                              \
    return 1;

RVA(0x0007da40, 0xf1)
i32 DispatchGruntSelectedSpriteLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CUserLogic* sub = new CGruntSelectedSprite(owner);
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

RVA(0x0007db80, 0xf1)
i32 DispatchGruntHealthSpriteLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CGruntHealthSprite)}

RVA(0x0007dcc0, 0xf1)
i32 DispatchGruntToySpriteLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CUserLogic* sub = new CGruntToySprite(owner);
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

RVA(0x0007de00, 0xf1)
i32 DispatchGruntStaminaSpriteLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CGruntStaminaSprite)}

RVA(0x0007df40, 0xf1)
i32 DispatchGruntToyTimeSpriteLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CGruntToyTimeSprite)}

RVA(0x0007e080, 0xf1)
i32 DispatchGruntWingzTimeSpriteLogic(CGameObject* owner){
    LOGIC_RECORD_DISPATCH(CGruntWingzTimeSprite)
}

RVA(0x0007e1c0, 0xf1)
i32 DispatchGruntPowerupSpriteLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntPowerupSprite)
}
