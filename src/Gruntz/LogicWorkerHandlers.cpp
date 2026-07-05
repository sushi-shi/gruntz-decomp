// LogicWorkerHandlers.cpp - a fourth family of the 0xf1 logic-worker message
// handlers (the same archetype as AnimWorkerHandlers / InGameWorkerHandlers).
//
// The four handlers (0x0a9cc0 / 0x0aa5a0 / 0x0aa960 / 0x0af0a0) are __cdecl FREE
// functions (the owner is a stack arg at [esp+0x18]; ecx is never touched - the
// trace-clusterer's tomalla-75/76/109/106 "classes" are a false grouping,
// just like tomalla-72). Each reads owner->m_7c (the worker), then runs a
// /GX message pump keyed on the worker's state tag worker->m_1c:
//   state 0      -> `new <leaf>(owner)`; activate it (sub->vtbl[0x18]); stow
//                   it at worker->m_18; advance the state tag to 0x3e8.
//   state 0x1d   -> sub->vtbl[0x2c]()      state 0x1e -> sub->vtbl[0x28]()
//   state 0x50   -> sub->vtbl[0x38]()      state 0x53 -> sub->vtbl[0x3c]()
//   state 0x52   -> sub->vtbl[0x30]()      state 0x51 -> sub->vtbl[0x34]()
//   state 0x3e8  -> idle (no-op).          default     -> the engine default pump
//                   (0x16e4f0, __cdecl, taking the sub-record).
// The four handlers are byte-identical bar the sub-record TYPE (hence the `new`
// size + ctor target). The created records are CUserLogic-derived game objects
// (shared base vptr 0x5e70b4):
//   0xa9cc0 -> CDoNothing       (ctor 0xac1d0, size 0x54)
//   0xaa5a0 -> CBehindCandyAni  (ctor 0xad540, size 0x54)
//   0xaa960 -> CWayPoint        (ctor 0xae3f0, size 0x54)
//   0xaf0a0 -> CRollingBall     (ctor 0xaf820, size 0xa0 -> 0xf4-byte handler)
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).
#include <rva.h>

#include <Gruntz/WorkerHandler.h> // shared Worker / Owner archetype + CUserLogic base

// The four engine logic sub-records the handlers `new`. Each is a real CUserLogic
// leaf (its own most-derived ctor is matched in another TU); here it is a thin
// size-view - the inherited CUserLogic base + the leaf's own tail as m_body. The
// 1-arg __thiscall ctor is declared with no body so `new T(owner)` lowers to
// push sizeof(T); call operator new; mov ecx,raw; push owner; call <ctor>, all
// reloc-masked. Deriving the real CUserLogic base lets the post-construction
// activate + pump dispatches lower to `mov eax,[obj]; call [eax+N]` through the
// inherited 16-slot CUserLogic vtable (no fabricated view class).
struct CDoNothing : public CTileLogic {
    CDoNothing(Owner* owner); // 0x0ac1d0
    char m_body[0x54 - 0x40];
}; // sizeof = 0x54

struct CBehindCandyAni : public CTileLogic {
    CBehindCandyAni(Owner* owner); // 0x0ad540
    char m_body[0x54 - 0x40];
}; // sizeof = 0x54

struct CWayPoint : public CTileLogic {
    CWayPoint(Owner* owner); // 0x0ae3f0
    char m_body[0x54 - 0x40];
}; // sizeof = 0x54

struct CRollingBall : public CTileLogic {
    CRollingBall(Owner* owner); // 0x0af820
    char m_body[0xa0 - 0x40];
}; // sizeof = 0xa0

// ---------------------------------------------------------------------------
// The switch key worker->m_1c is UNSIGNED (u32); MSVC5 then emits the range
// checks as unsigned ja/jbe, matching retail byte-for-byte. A signed i32 key
// emits jg/jle and caps the function at 97.86%. See
// docs/patterns/switch-key-unsigned-ja-vs-jg.md.
RVA(0x000a9cc0, 0xf1)
i32 HandlerA9CC0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CDoNothing(owner);
            sub->UserLogicVfunc4(); // slot 6 (+0x18): activate
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

RVA(0x000aa5a0, 0xf1)
i32 HandlerAA5A0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CBehindCandyAni(owner);
            sub->UserLogicVfunc4(); // slot 6 (+0x18): activate
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

RVA(0x000aa960, 0xf1)
i32 HandlerAA960(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CWayPoint(owner);
            sub->UserLogicVfunc4(); // slot 6 (+0x18): activate
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

RVA(0x000af0a0, 0xf4)
i32 HandlerAF0A0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CRollingBall(owner);
            sub->UserLogicVfunc4(); // slot 6 (+0x18): activate
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
