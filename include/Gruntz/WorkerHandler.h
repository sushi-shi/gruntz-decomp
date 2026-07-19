// WorkerHandler.h - the shared archetype for the 0xf1-family worker message
// handlers (InGameWorkerHandlers / LogicWorkerHandlers). Each handler reads
// owner->m_7c (the worker), then runs a /GX message pump keyed on the worker's
// UNSIGNED state tag worker->m_1c, `new`ing a CUserLogic-derived game object on
// state 0 and dispatching CUserLogic vtable slots on the other states.
//
// The dispatched records are real CUserLogic leaves (RTTI-proven: e.g. the
// CDoNothing ctor 0xac1d0 stamps ??_7CUserBase / ??_7CUserLogic / ??_7CDoNothing).
// So the "sub-record" IS a CUserLogic: there is no separate fake polymorphic base
// - the pump dispatches the real inherited CUserLogic slots (slot 6 activate @
// +0x18, slots 10..15 @ +0x28..+0x3c). Only the TU-local leaf types differ per
// family; the Worker/Owner shapes are shared verbatim.
#ifndef GRUNTZ_GRUNTZ_WORKERHANDLER_H
#define GRUNTZ_GRUNTZ_WORKERHANDLER_H

#include <Ints.h>

// The real base the worker's sub-records derive: CUserLogic (vftable 0x5e705c,
// 16 slots). The pump dispatches its inherited slots directly - no per-TU view.
#include <Gruntz/UserLogic.h>
#include <Gruntz/XferArchive.h> // the real 0x16e4f0 = ProjTypeXfer(CXferArchive*)

// The worker held at owner->m_7c. Only the message-pump fields are modeled here.
struct Worker {
    char _vft0[4];             // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    char m_pad04[0x18 - 0x04]; // +0x04..0x17
    CUserLogic* m_18;          // +0x18  the live sub-record (a CUserLogic game object)
    u32 m_1c;                  // +0x1c  state tag (UNSIGNED switch key)
};

// The owner game object handed to each handler; its worker hangs at +0x7c.
struct Owner {
    char m_pad00[0x7c];
    Worker* m_7c; // +0x7c
};

// The engine default message pump run for any unhandled state IS the real shared
// coordinate/type-registry resolve at 0x16e4f0 (?ProjTypeXfer@@YAHPAUCXferArchive@@@Z,
// __cdecl). Thin forwarder so callers emit the bound rel32 (was fake _Worker_DefaultPump).
inline void Worker_DefaultPump(CUserLogic* sub) {
    ProjTypeXfer(reinterpret_cast<CXferArchive*>(sub));
}

// The shared logic-worker message pump. Each per-type handler is `{ LOGIC_WORKER_PUMP(LEAF); }`
// where LEAF is the CUserLogic-derived game object this handler owns (the pump `new`s it on
// state 0). Reads owner->m_7c (the worker), switches on the UNSIGNED state tag worker->m_1c
// (u32 -> unsigned ja/jbe range-checks, matching retail; a signed key caps at ~97.86%, see
// docs/patterns/switch-key-unsigned-ja-vs-jg.md).
#define LOGIC_WORKER_PUMP(LEAF)                                                                    \
    Worker* rec = owner->m_7c;                                                                     \
    switch (rec->m_1c) {                                                                           \
        case 0: {                                                                                  \
            rec->m_1c = 0x3e8;                                                                     \
            CUserLogic* sub = new LEAF((CGameObject*)owner);                                       \
            sub->Activate(); /* slot 6 (+0x18): activate */                                        \
            rec->m_18 = sub;                                                                       \
            break;                                                                                 \
        }                                                                                          \
        case 0x1d:                                                                                 \
            rec->m_18->UserLogicVfunc9(); /* slot 11 (+0x2c) */                                    \
            break;                                                                                 \
        case 0x1e:                                                                                 \
            rec->m_18->UserLogicVfunc8(); /* slot 10 (+0x28) */                                    \
            break;                                                                                 \
        case 0x50:                                                                                 \
            rec->m_18->UserLogicVfuncC(); /* slot 14 (+0x38) */                                    \
            break;                                                                                 \
        case 0x53:                                                                                 \
            rec->m_18->UserLogicVfuncD(); /* slot 15 (+0x3c) */                                    \
            break;                                                                                 \
        case 0x52:                                                                                 \
            rec->m_18->UserLogicVfuncA(); /* slot 12 (+0x30) */                                    \
            break;                                                                                 \
        case 0x51:                                                                                 \
            rec->m_18->UserLogicVfuncB(); /* slot 13 (+0x34) */                                    \
            break;                                                                                 \
        case 0x3e8:                                                                                \
            break;                                                                                 \
        default:                                                                                   \
            Worker_DefaultPump(rec->m_18);                                                         \
            break;                                                                                 \
    }                                                                                              \
    return 1;

#endif // GRUNTZ_GRUNTZ_WORKERHANDLER_H
