#ifndef GRUNTZ_GRUNTZ_WORKERHANDLER_H
#define GRUNTZ_GRUNTZ_WORKERHANDLER_H

#include <DDrawMgr/AnimWorkerObj.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/XferArchive.h>
#include <Ints.h>
#include <Wwd/WwdGameObjectFamily.h>

inline void Worker_DefaultPump(CUserLogic* sub) {
    ProjTypeXfer(sub);
}

#define LOGIC_WORKER_PUMP(LEAF)                                                                    \
    AnimWorkerObj* rec = owner->m_animWorker;                                                      \
    switch (static_cast<u32>(rec->m_actKey)) {                                                     \
        case 0: {                                                                                  \
            rec->m_actKey = 0x3e8;                                                                 \
            CUserLogic* sub = new LEAF(owner);                                                     \
            sub->Activate();                                                                       \
            rec->m_logic = sub;                                                                    \
            break;                                                                                 \
        }                                                                                          \
        case 0x1d:                                                                                 \
            rec->m_logic->OnObjectRemoved();                                                       \
            break;                                                                                 \
        case 0x1e:                                                                                 \
            rec->m_logic->OnLeaveActiveRegion();                                                   \
            break;                                                                                 \
        case 0x50:                                                                                 \
            rec->m_logic->PrepareSave();                                                           \
            break;                                                                                 \
        case 0x53:                                                                                 \
            rec->m_logic->AfterLoadReferences();                                                   \
            break;                                                                                 \
        case 0x52:                                                                                 \
            rec->m_logic->AfterLoad();                                                             \
            break;                                                                                 \
        case 0x51:                                                                                 \
            rec->m_logic->AfterSave();                                                             \
            break;                                                                                 \
        case 0x3e8:                                                                                \
            break;                                                                                 \
        default:                                                                                   \
            Worker_DefaultPump(rec->m_logic);                                                      \
            break;                                                                                 \
    }                                                                                              \
    return 1;

#endif // GRUNTZ_GRUNTZ_WORKERHANDLER_H
