#ifndef GRUNTZ_GRUNTZ_LOGICRECORDHANDLER_H
#define GRUNTZ_GRUNTZ_LOGICRECORDHANDLER_H

#include <DDrawMgr/LogicRecord.h>
#include <Gruntz/LogicEventDispatch.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>
#include <Wwd/LogicRecordEvent.h>
#include <Wwd/WwdGameObjectFamily.h>

inline void DispatchUnhandledLogicEvent(CUserLogic* sub) {
    DispatchLogicEvent(sub);
}

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

#endif // GRUNTZ_GRUNTZ_LOGICRECORDHANDLER_H
