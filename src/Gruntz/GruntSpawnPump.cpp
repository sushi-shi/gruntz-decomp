// GruntSpawnPump.cpp - the CGrunt spawn/message-pump handler @0x5baf0, a member
// of the 0xf1/0xf4 worker-handler family (same archetype as LogicWorkerHandlers /
// InGameWorkerHandlers), but the spawned leaf is a CGrunt (size 0x8d8, ctor
// 0x47a10) rather than a small CUserLogic eye-candy leaf. A __cdecl free handler
// (owner is a stack arg; ecx untouched): reads owner->m_7c (the worker), then runs
// a /GX message pump keyed on the worker's UNSIGNED state tag worker->m_1c:
//   state 0     -> `new CGrunt(owner)`; activate it (slot 6); stow at worker->m_18;
//                  advance the state tag to 0x3e8.
//   states 0x1d/0x1e/0x50/0x51/0x52/0x53 -> the sub-record's CUserLogic slots
//                  11/10/13/12/14/15.
//   state 0x3e8 -> idle.   default -> the engine default pump (0x16e4f0).
#include <Gruntz/WorkerHandler.h>
#include <Gruntz/Grunt.h>
#include <rva.h>

RVA(0x0005baf0, 0xf4)
i32 GruntSpawnPump(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CGrunt((CGameObject*)owner);
            sub->Activate();  // slot 6 (+0x18)
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9(); // slot 11 (+0x2c)
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8(); // slot 10 (+0x28)
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC(); // slot 14 (+0x38)
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD(); // slot 15 (+0x3c)
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA(); // slot 12 (+0x30)
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB(); // slot 13 (+0x34)
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}
