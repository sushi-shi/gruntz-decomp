#ifndef GRUNTZ_GRUNTZ_WORKERHANDLER_H
#define GRUNTZ_GRUNTZ_WORKERHANDLER_H

#include <DDrawMgr/AnimWorkerObj.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/XferArchive.h>
#include <Ints.h>
#include <Wwd/AnimWorkerAct.h>
#include <Wwd/WwdGameObjectFamily.h>

inline void Worker_DefaultPump(CUserLogic* sub) {
    ProjTypeXfer(sub);
}

#define LOGIC_WORKER_PUMP(LEAF)                                                                    \
    AnimWorkerObj* rec = owner->m_animWorker;                                                      \
    switch (rec->WorkerAct()) {                                                                    \
        case ACT_UNINITIALISED: {                                                                  \
            rec->SetWorkerAct(ACT_LIVE);                                                           \
            CUserLogic* sub = new LEAF(owner);                                                     \
            sub->Activate();                                                                       \
            rec->m_logic = sub;                                                                    \
            break;                                                                                 \
        }                                                                                          \
        case ACT_OBJECT_REMOVED:                                                                   \
            rec->m_logic->OnObjectRemoved();                                                       \
            break;                                                                                 \
        case ACT_LEAVE_ACTIVE_REGION:                                                              \
            rec->m_logic->OnLeaveActiveRegion();                                                   \
            break;                                                                                 \
        case ACT_PREPARE_SAVE:                                                                     \
            rec->m_logic->PrepareSave();                                                           \
            break;                                                                                 \
        case ACT_AFTER_LOAD_REFERENCES:                                                            \
            rec->m_logic->AfterLoadReferences();                                                   \
            break;                                                                                 \
        case ACT_AFTER_LOAD:                                                                       \
            rec->m_logic->AfterLoad();                                                             \
            break;                                                                                 \
        case ACT_AFTER_SAVE:                                                                       \
            rec->m_logic->AfterSave();                                                             \
            break;                                                                                 \
        case ACT_LIVE:                                                                             \
            break;                                                                                 \
        default:                                                                                   \
            Worker_DefaultPump(rec->m_logic);                                                      \
            break;                                                                                 \
    }                                                                                              \
    return 1;

#endif // GRUNTZ_GRUNTZ_WORKERHANDLER_H
