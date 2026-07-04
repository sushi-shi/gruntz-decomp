// InGameWorkerHandlers.cpp - the in-game-HUD worker message-handler family, the
// twin of AnimWorkerHandlers (src/Gruntz/AnimWorkerHandlers.cpp). Same /GX
// message-pump shape; these dispatch on the in-game HUD worker hanging at
// owner->m_7c instead of the anim worker.
//
// The three handlers (0x095750 / 0x095890 / 0x0aa6e0) are __cdecl FREE functions
// (the owner is a stack arg at [esp+0x18], ecx is never `this` - the trace
// clusterer's tomalla-65 grouping was a false grouping; these are not class
// members). Each reads owner->m_7c (the worker), then runs a /GX message pump
// keyed on the worker's state tag worker->m_1c:
//   state 0      -> `new <leaf>(owner)`; activate it (sub->vtbl[0x18]); stow
//                   it at worker->m_18; advance the state tag to 0x3e8.
//   state 0x1d   -> sub->vtbl[0x2c]()      state 0x1e -> sub->vtbl[0x28]()
//   state 0x50   -> sub->vtbl[0x38]()      state 0x53 -> sub->vtbl[0x3c]()
//   state 0x52   -> sub->vtbl[0x30]()      state 0x51 -> sub->vtbl[0x34]()
//   state 0x3e8  -> idle (no-op).          default     -> the engine default pump
//                   (0x16e4f0, __cdecl, taking the sub-record).
// The three handlers are byte-identical bar the sub-record TYPE (hence the `new`
// size + ctor target): 0x80 (CInGameIcon, ctor 0x95b10), 0x5c (CInGameText, ctor
// 0x99110), 0x54 (CEyeCandy, ctor 0xac620).
//
// KEY to 100% (not 97.86%): the worker state tag is `u32 m_1c` (UNSIGNED). MSVC5
// emits the switch range-checks as unsigned `ja`/`jbe` for an unsigned key; a
// signed `i32` key would emit `jg`/`jle` and diverge (the only difference vs the
// retail bytes). See docs/patterns/switch-key-unsigned-ja-vs-jg.md.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + emitted code bytes
// are load-bearing (campaign doctrine).
#include <rva.h>

#include <Gruntz/WorkerHandler.h> // shared Worker / Owner archetype + CUserLogic base

// The three engine HUD sub-records the handlers `new`. Each is a real CUserLogic
// leaf (its own most-derived ctor is matched in another TU); here it is a thin
// size-view - the inherited CUserLogic base + the leaf's own tail as m_body. The
// 1-arg __thiscall ctor is declared with no body so `new T(owner)` lowers to
// push sizeof(T); call operator new; mov ecx,raw; push owner; call <ctor>, all
// reloc-masked. Deriving the real CUserLogic base lets the post-construction
// activate + pump dispatches lower to `mov eax,[obj]; call [eax+N]` through the
// inherited 16-slot CUserLogic vtable (no fabricated view class).
struct CInGameIcon : public CUserLogic {
    CInGameIcon(Owner* owner); // 0x095b10
    char m_body[0x80 - 0x40];
}; // sizeof = 0x80

struct CInGameText : public CUserLogic {
    CInGameText(Owner* owner); // 0x099110
    char m_body[0x5c - 0x40];
}; // sizeof = 0x5c

struct CEyeCandy : public CUserLogic {
    CEyeCandy(Owner* owner); // 0x0ac620
    char m_body[0x54 - 0x40];
}; // sizeof = 0x54

// ---------------------------------------------------------------------------
RVA(0x00095750, 0xf4)
i32 Handler095750(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CInGameIcon(owner);
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

RVA(0x00095890, 0xf1)
i32 Handler095890(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CInGameText(owner);
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

RVA(0x000aa6e0, 0xf1)
i32 Handler0aa6e0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CEyeCandy(owner);
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
