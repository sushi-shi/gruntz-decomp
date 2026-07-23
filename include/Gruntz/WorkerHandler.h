#ifndef GRUNTZ_GRUNTZ_WORKERHANDLER_H
#define GRUNTZ_GRUNTZ_WORKERHANDLER_H

#include <Ints.h>

#include <Gruntz/UserLogic.h>
#include <Gruntz/XferArchive.h> // the real 0x16e4f0 = ProjTypeXfer(CXferArchive*)

#include <Wwd/WwdGameObjectFamily.h> // the real CGameObject (m_7c worker slot)
#include <DDrawMgr/AnimWorkerObj.h>  // the real worker (m_logic / m_1c role-union)

inline void Worker_DefaultPump(CUserLogic* sub) {
    ProjTypeXfer(sub);
}

#define LOGIC_WORKER_PUMP(LEAF)                                                                    \
    AnimWorkerObj* rec = owner->m_7c;                                                              \
    switch (reinterpret_cast<u32>(rec->m_1c)) {                                                    \
        case 0: {                                                                                  \
            rec->m_1c = reinterpret_cast<void*>(0x3e8);                                            \
            CUserLogic* sub = new LEAF(owner);                                                     \
            sub->Activate(); /* slot 6 (+0x18): activate */                                        \
            rec->m_logic = sub;                                                                    \
            break;                                                                                 \
        }                                                                                          \
        case 0x1d:                                                                                 \
            rec->m_logic->UserLogicVfunc9(); /* slot 11 (+0x2c) */                                 \
            break;                                                                                 \
        case 0x1e:                                                                                 \
            rec->m_logic->UserLogicVfunc8(); /* slot 10 (+0x28) */                                 \
            break;                                                                                 \
        case 0x50:                                                                                 \
            rec->m_logic->UserLogicVfuncC(); /* slot 14 (+0x38) */                                 \
            break;                                                                                 \
        case 0x53:                                                                                 \
            rec->m_logic->UserLogicVfuncD(); /* slot 15 (+0x3c) */                                 \
            break;                                                                                 \
        case 0x52:                                                                                 \
            rec->m_logic->UserLogicVfuncA(); /* slot 12 (+0x30) */                                 \
            break;                                                                                 \
        case 0x51:                                                                                 \
            rec->m_logic->UserLogicVfuncB(); /* slot 13 (+0x34) */                                 \
            break;                                                                                 \
        case 0x3e8:                                                                                \
            break;                                                                                 \
        default:                                                                                   \
            Worker_DefaultPump(rec->m_logic);                                                      \
            break;                                                                                 \
    }                                                                                              \
    return 1;

#endif // GRUNTZ_GRUNTZ_WORKERHANDLER_H
