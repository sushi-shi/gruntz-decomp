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

// (The `Worker` / `Owner` reduced shells are DISSOLVED, 2026-07-19: Worker's own
// comment carried the proof - its "foreign vtable 0x5efb80" IS ??_7AnimWorkerObj -
// and Owner's +0x7c worker is exactly CGameObject::m_7c (`AnimWorkerObj* m_7c`,
// WwdGameObjectFamily.h). The handlers now take the REAL CGameObject* and the pump
// walks the real AnimWorkerObj (m_18==m_logic; the +0x1c state tag is the
// documented void* int|ptr role-union, cast at the int sites).)
#include <Wwd/WwdGameObjectFamily.h> // the real CGameObject (m_7c worker slot)
#include <DDrawMgr/AnimWorkerObj.h>  // the real worker (m_logic / m_1c role-union)

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
            rec->m_logic->UserLogicVfunc9(); /* slot 11 (+0x2c) */                                    \
            break;                                                                                 \
        case 0x1e:                                                                                 \
            rec->m_logic->UserLogicVfunc8(); /* slot 10 (+0x28) */                                    \
            break;                                                                                 \
        case 0x50:                                                                                 \
            rec->m_logic->UserLogicVfuncC(); /* slot 14 (+0x38) */                                    \
            break;                                                                                 \
        case 0x53:                                                                                 \
            rec->m_logic->UserLogicVfuncD(); /* slot 15 (+0x3c) */                                    \
            break;                                                                                 \
        case 0x52:                                                                                 \
            rec->m_logic->UserLogicVfuncA(); /* slot 12 (+0x30) */                                    \
            break;                                                                                 \
        case 0x51:                                                                                 \
            rec->m_logic->UserLogicVfuncB(); /* slot 13 (+0x34) */                                    \
            break;                                                                                 \
        case 0x3e8:                                                                                \
            break;                                                                                 \
        default:                                                                                   \
            Worker_DefaultPump(rec->m_logic);                                                         \
            break;                                                                                 \
    }                                                                                              \
    return 1;

#endif // GRUNTZ_GRUNTZ_WORKERHANDLER_H
