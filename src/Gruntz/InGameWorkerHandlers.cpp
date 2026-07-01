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

// ---------------------------------------------------------------------------
// The polymorphic sub-record the worker `new`s and dispatches. Slots are laid
// out so the message pump's vtable calls land at the right byte offsets
// (0x18 = slot 6 activate, 0x28..0x3c = slots 10..15). Declarations only - never
// defined here, so no ??_7 is emitted; the constructed object's real vtable is
// the engine HUD class's, stamped by its own (extern) constructor.
class SubRecord {
public:
    virtual void Slot00();   // +0x00
    virtual void Slot04();   // +0x04
    virtual void Slot08();   // +0x08
    virtual void Slot0C();   // +0x0c
    virtual void Slot10();   // +0x10
    virtual void Slot14();   // +0x14
    virtual void Activate(); // +0x18  (slot 6)
    virtual void Slot1C();   // +0x1c
    virtual void Slot20();   // +0x20
    virtual void Slot24();   // +0x24
    virtual void Vfunc28();  // +0x28  (state 0x1e)
    virtual void Vfunc2C();  // +0x2c  (state 0x1d)
    virtual void Vfunc30();  // +0x30  (state 0x52)
    virtual void Vfunc34();  // +0x34  (state 0x51)
    virtual void Vfunc38();  // +0x38  (state 0x50)
    virtual void Vfunc3C();  // +0x3c  (state 0x53)
};

// The three engine HUD sub-records the handlers `new`. Each is an opaque engine
// object of its exact retail size with a 1-arg __thiscall constructor matched in
// another TU; declared with no body so `new T(owner)` lowers to
// push sizeof(T); call operator new; mov ecx,raw; push owner; call <ctor>, all
// reloc-masked. The leading SubRecord base lets the post-construction Activate()
// dispatch lower to `mov eax,[obj]; call [eax+0x18]`.
struct Owner;

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

// The worker held at owner->m_7c. Only the message-pump fields are modeled here.
struct Worker {
    void* m_vptr;              // +0x00
    char m_pad04[0x18 - 0x04]; // +0x04..0x17
    SubRecord* m_18;           // +0x18  the live sub-record
    u32 m_1c;                  // +0x1c  state tag (UNSIGNED switch key)
};

// The owner game object handed to each handler; its worker hangs at +0x7c.
struct Owner {
    char m_pad00[0x7c];
    Worker* m_7c; // +0x7c
};

// The engine default message pump run for any unhandled state (0x16e4f0,
// __cdecl, takes the sub-record). Reloc-masked rel32 - no body.
extern "C" void Worker_DefaultPump(SubRecord* sub);

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

// H-N misc-Gruntz class-metadata sweep (SIZE).
SIZE_UNKNOWN(SubRecord);
