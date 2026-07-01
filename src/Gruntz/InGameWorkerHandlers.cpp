// InGameWorkerHandlers.cpp - the in-game-HUD worker message-handler family, the
// twin of SiriusWorkerHandlers (src/Gruntz/SiriusWorkerHandlers.cpp). Same /GX
// message-pump shape; these dispatch on the in-game HUD worker hanging at
// owner->m_7c instead of the Sirius worker.
//
// The three handlers (0x095750 / 0x095890 / 0x0aa6e0) are __cdecl FREE functions
// (the owner is a stack arg at [esp+0x18], ecx is never `this` - the trace
// clusterer's ClassUnknown_65 grouping was a false grouping; these are not class
// members). Each reads owner->m_7c (the worker), then runs a /GX message pump
// keyed on the worker's state tag worker->m_1c:
//   state 0      -> `new <SubRecord>(owner)`; activate it (sub->vtbl[0x18]); stow
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

#include <Gruntz/WorkerHandler.h> // shared SubRecord / Worker / Owner archetype

// The three engine HUD sub-records the handlers `new`. Each is an opaque engine
// object of its exact retail size with a 1-arg __thiscall constructor matched in
// another TU; declared with no body so `new T(owner)` lowers to
// push sizeof(T); call operator new; mov ecx,raw; push owner; call <ctor>, all
// reloc-masked. The leading SubRecord base lets the post-construction Activate()
// dispatch lower to `mov eax,[obj]; call [eax+0x18]`.
struct CInGameIcon : public SubRecord {
    CInGameIcon(Owner* owner); // 0x095b10
    char m_body[0x80 - 0x04];
}; // sizeof = 0x80

struct CInGameText : public SubRecord {
    CInGameText(Owner* owner); // 0x099110
    char m_body[0x5c - 0x04];
}; // sizeof = 0x5c

struct CEyeCandy : public SubRecord {
    CEyeCandy(Owner* owner); // 0x0ac620
    char m_body[0x54 - 0x04];
}; // sizeof = 0x54

// ---------------------------------------------------------------------------
RVA(0x00095750, 0xf4)
i32 Handler095750(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            SubRecord* sub = new CInGameIcon(owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->Vfunc2C();
            break;
        case 0x1e:
            rec->m_18->Vfunc28();
            break;
        case 0x50:
            rec->m_18->Vfunc38();
            break;
        case 0x53:
            rec->m_18->Vfunc3C();
            break;
        case 0x52:
            rec->m_18->Vfunc30();
            break;
        case 0x51:
            rec->m_18->Vfunc34();
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
            SubRecord* sub = new CInGameText(owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->Vfunc2C();
            break;
        case 0x1e:
            rec->m_18->Vfunc28();
            break;
        case 0x50:
            rec->m_18->Vfunc38();
            break;
        case 0x53:
            rec->m_18->Vfunc3C();
            break;
        case 0x52:
            rec->m_18->Vfunc30();
            break;
        case 0x51:
            rec->m_18->Vfunc34();
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
            SubRecord* sub = new CEyeCandy(owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->Vfunc2C();
            break;
        case 0x1e:
            rec->m_18->Vfunc28();
            break;
        case 0x50:
            rec->m_18->Vfunc38();
            break;
        case 0x53:
            rec->m_18->Vfunc3C();
            break;
        case 0x52:
            rec->m_18->Vfunc30();
            break;
        case 0x51:
            rec->m_18->Vfunc34();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

SIZE_UNKNOWN(SubRecord);
