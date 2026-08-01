#ifndef GRUNTZ_GRUNTZ_WORKERHANDLER_H
#define GRUNTZ_GRUNTZ_WORKERHANDLER_H

#include <Ints.h>

#include <Gruntz/UserLogic.h>
#include <Gruntz/XferArchive.h>

#include <Wwd/WwdGameObjectFamily.h>
#include <DDrawMgr/AnimWorkerObj.h>

inline void Worker_DefaultPump(CUserLogic* sub) {
    ProjTypeXfer(sub);
}

#define LOGIC_WORKER_PUMP(LEAF)                                                                    \
    AnimWorkerObj* rec = owner->m_animWorker;                                                      \
    switch (static_cast<u32>(rec->m_1c)) {                                                         \
        case 0: {                                                                                  \
            rec->m_1c = 0x3e8;                                                                     \
            CUserLogic* sub = new LEAF(owner);                                                     \
            sub->Activate();                                                                       \
            rec->m_logic = sub;                                                                    \
            break;                                                                                 \
        }                                                                                          \
        case 0x1d:                                                                                 \
            rec->m_logic->UserLogicVfunc9();                                                       \
            break;                                                                                 \
        case 0x1e:                                                                                 \
            rec->m_logic->UserLogicVfunc8();                                                       \
            break;                                                                                 \
        case 0x50:                                                                                 \
            rec->m_logic->UserLogicVfuncC();                                                       \
            break;                                                                                 \
        case 0x53:                                                                                 \
            rec->m_logic->UserLogicVfuncD();                                                       \
            break;                                                                                 \
        case 0x52:                                                                                 \
            rec->m_logic->UserLogicVfuncA();                                                       \
            break;                                                                                 \
        case 0x51:                                                                                 \
            rec->m_logic->UserLogicVfuncB();                                                       \
            break;                                                                                 \
        case 0x3e8:                                                                                \
            break;                                                                                 \
        default:                                                                                   \
            Worker_DefaultPump(rec->m_logic);                                                      \
            break;                                                                                 \
    }                                                                                              \
    return 1;

#endif // GRUNTZ_GRUNTZ_WORKERHANDLER_H
