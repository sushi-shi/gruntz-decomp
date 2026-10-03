#ifndef GRUNTZ_GRUNTZ_LOGICRECORDHANDLER_H
#define GRUNTZ_GRUNTZ_LOGICRECORDHANDLER_H

#include <DDrawMgr/LogicRecord.h>
#include <Gruntz/LogicEventDispatch.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>
#include <Wwd/LogicRecordEvent.h>
#include <Wwd/WwdGameObjectFamily.h>

#define LOGIC_RECORD_DISPATCH(LEAF)                                                                \
    CLogicRecord* record = owner->GetLogicRecord();                                                \
    switch (record->LogicEvent()) {                                                                \
        case ACT_UNINITIALISED: {                                                                  \
            record->SetLogicEvent(ACT_LIVE);                                                       \
            CUserLogic* sub = new LEAF(owner);                                                     \
            sub->Activate();                                                                       \
            record->m_userLogic = sub;                                                             \
            break;                                                                                 \
        }                                                                                          \
        case ACT_OBJECT_REMOVED:                                                                   \
            record->UserLogic()->OnObjectRemoved();                                                \
            break;                                                                                 \
        case ACT_LEAVE_ACTIVE_REGION:                                                              \
            record->UserLogic()->OnLeaveActiveRegion();                                            \
            break;                                                                                 \
        case ACT_PREPARE_SAVE:                                                                     \
            record->UserLogic()->PrepareSave();                                                    \
            break;                                                                                 \
        case ACT_AFTER_LOAD_REFERENCES:                                                            \
            record->UserLogic()->AfterLoadReferences();                                            \
            break;                                                                                 \
        case ACT_AFTER_LOAD:                                                                       \
            record->UserLogic()->AfterLoad();                                                      \
            break;                                                                                 \
        case ACT_AFTER_SAVE:                                                                       \
            record->UserLogic()->AfterSave();                                                      \
            break;                                                                                 \
        case ACT_LIVE:                                                                             \
            break;                                                                                 \
        default:                                                                                   \
            DispatchLogicEvent(record->UserLogic());                                               \
            break;                                                                                 \
    }                                                                                              \
    return 1;

#endif // GRUNTZ_GRUNTZ_LOGICRECORDHANDLER_H
