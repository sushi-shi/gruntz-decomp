// LogicWorkerHandlers.cpp - a fourth family of the 0xf1 logic-worker message
// handlers (the same archetype as SiriusWorkerHandlers / InGameWorkerHandlers).
//
// The four handlers (0x0a9cc0 / 0x0aa5a0 / 0x0aa960 / 0x0af0a0) are __cdecl FREE
// functions (the owner is a stack arg at [esp+0x18]; ecx is never touched - the
// trace-clusterer's ClassUnknown_75/76/109/106 "classes" are a false grouping,
// just like ClassUnknown_72). Each reads owner->m_7c (the worker), then runs a
// /GX message pump keyed on the worker's state tag worker->m_1c:
//   state 0      -> `new <SubRecord>(owner)`; activate it (sub->vtbl[0x18]); stow
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

// ---------------------------------------------------------------------------
// The polymorphic sub-record the worker `new`s and dispatches. Slots are laid
// out so the message pump's vtable calls land at the right byte offsets
// (0x18 = slot 6 activate, 0x28..0x3c = slots 10..15). Declarations only - never
// defined here, so no ??_7 is emitted; the constructed object's real vtable is
// the engine logic class's, stamped by its own (extern) constructor.
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

// The four engine logic sub-records the handlers `new`. Each is an opaque engine
// object of its exact retail size with a 1-arg __thiscall constructor matched in
// another TU; declared with no body so `new T(owner)` lowers to
// push sizeof(T); call operator new; mov ecx,raw; push owner; call <ctor>, all
// reloc-masked. The leading SubRecord base lets the post-construction Activate()
// dispatch lower to `mov eax,[obj]; call [eax+0x18]`.
struct Owner;

struct CDoNothing : public SubRecord {
    CDoNothing(Owner* owner); // 0x0ac1d0
    char m_body[0x54 - 0x04];
}; // sizeof = 0x54

struct CBehindCandyAni : public SubRecord {
    CBehindCandyAni(Owner* owner); // 0x0ad540
    char m_body[0x54 - 0x04];
}; // sizeof = 0x54

struct CWayPoint : public SubRecord {
    CWayPoint(Owner* owner); // 0x0ae3f0
    char m_body[0x54 - 0x04];
}; // sizeof = 0x54

struct CRollingBall : public SubRecord {
    CRollingBall(Owner* owner); // 0x0af820
    char m_body[0xa0 - 0x04];
}; // sizeof = 0xa0

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
            SubRecord* sub = new CDoNothing(owner);
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

RVA(0x000aa5a0, 0xf1)
i32 HandlerAA5A0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            SubRecord* sub = new CBehindCandyAni(owner);
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

RVA(0x000aa960, 0xf1)
i32 HandlerAA960(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            SubRecord* sub = new CWayPoint(owner);
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

RVA(0x000af0a0, 0xf4)
i32 HandlerAF0A0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            SubRecord* sub = new CRollingBall(owner);
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
