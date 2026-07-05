// AnimWorkerHandlers.cpp - the anim-worker message-handler family + the
// out-of-line worker constructor (foreign vtable 0x5efb80, the same 0x17c-byte
// worker family as CDDrawWorkerCache's AnimWorkerObj and the CWwdGameObject
// +0x7c AnimWorker).
//
// The three 0xf1 handlers (0x03d670 / 0x07db20 / 0x07dda0) are __cdecl FREE
// functions (the owner is a stack arg at [esp+0x18], ecx is never touched - the
// trace-clusterer's tomalla-72 "class" is a false grouping; see the report).
// Each reads owner->m_7c (the worker), then runs a /GX message pump keyed on the
// worker's state tag worker->m_1c:
//   state 0      -> `new <leaf>(owner)`; activate it (sub->vtbl[0x18]); stow
//                   it at worker->m_18; advance the state tag to 0x3e8.
//   state 0x1d   -> sub->vtbl[0x2c]()      state 0x1e -> sub->vtbl[0x28]()
//   state 0x50   -> sub->vtbl[0x38]()      state 0x53 -> sub->vtbl[0x3c]()
//   state 0x52   -> sub->vtbl[0x30]()      state 0x51 -> sub->vtbl[0x34]()
//   state 0x3e8  -> idle (no-op).          default     -> the engine default pump
//                   (0x16e4f0, __cdecl, taking the sub-record).
// The three handlers are byte-identical bar the sub-record TYPE (hence the
// `new` size + ctor target): 0x54 (CWormhole::Stub_03fc70 family), 0x5c
// (CGruntSelectedSprite), 0x60 (CGruntToySprite).
//
// The 0x15b300 ctor is the out-of-line 3-arg AnimWorker constructor (the body
// the 0x150eb0 factory / CreateWorker24 inline): stamp the foreign vptr,
// seed +0x04/+0x08/+0x0c from the args, zero the working fields.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).
#include <rva.h>

// The dispatched sub-records are real CUserLogic leaves (vftable 0x5e705c, 16
// slots); the worker pump calls their inherited slots (slot 6 activate @+0x18,
// slots 10..15 @+0x28..+0x3c) directly - no fabricated view class.
#include <Gruntz/UserLogic.h>

// The owner game object handed to each handler; forward-declared for the leaf
// ctors, fully defined below.
struct Owner;

// The three engine sprite sub-records the handlers `new` (real CUserLogic leaves;
// each most-derived ctor is matched in another TU). Here each is a thin size-view
// - inherited CUserLogic base + the leaf's own tail as m_body - so `new T(owner)`
// lowers to push sizeof(T); call operator new; mov ecx,raw; push owner; call
// <ctor>, all reloc-masked, and the post-construction activate + pump dispatches
// lower to `mov eax,[obj]; call [eax+N]` through the inherited 16-slot vtable.
struct CWormhole : public CTileLogic {
    CWormhole(Owner* owner); // 0x03fc70
    char m_body[0x54 - 0x40];
}; // sizeof = 0x54

struct CGruntSelectedSprite : public CTileLogic {
    CGruntSelectedSprite(Owner* owner); // 0x07e3e0
    char m_body[0x5c - 0x40];
}; // sizeof = 0x5c

struct CGruntToySprite : public CTileLogic {
    CGruntToySprite(Owner* owner); // 0x07f350
    char m_body[0x60 - 0x40];
}; // sizeof = 0x60

// The worker held at owner->m_7c (foreign vtable 0x5efb80). Only the message-
// pump fields are modeled here.
struct Worker {
    void* m_vptr;              // +0x00
    char m_pad04[0x18 - 0x04]; // +0x04..0x17
    CUserLogic* m_18;          // +0x18  the live sub-record
    u32 m_1c;                  // +0x1c  state tag (UNSIGNED switch key)
};

// The owner game object handed to each handler; its worker hangs at +0x7c.
struct Owner {
    char m_pad00[0x7c];
    Worker* m_7c; // +0x7c
};

// The engine default message pump run for any unhandled state (0x16e4f0,
// __cdecl, takes the sub-record). Reloc-masked rel32 - no body.
extern "C" void Worker_DefaultPump(CUserLogic* sub);

// ---------------------------------------------------------------------------
// The switch key worker->m_1c is UNSIGNED (u32); MSVC5 then emits the range
// checks as unsigned ja/jbe, matching retail byte-for-byte. A signed i32 key
// emits jg/jle and caps the function at 97.86%. See
// docs/patterns/switch-key-unsigned-ja-vs-jg.md.
RVA(0x0003d670, 0xf1)
i32 Handler03d670(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CWormhole(owner);
            sub->Activate(); // slot 6 (+0x18): activate
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

RVA(0x0007db20, 0xf1)
i32 Handler07db20(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CGruntSelectedSprite(owner);
            sub->Activate(); // slot 6 (+0x18): activate
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

RVA(0x0007dda0, 0xf1)
i32 Handler07dda0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CGruntToySprite(owner);
            sub->Activate(); // slot 6 (+0x18): activate
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

// ---------------------------------------------------------------------------
// AnimWorker 3-arg ctor (0x15b300, __thiscall, ret 0xc). Real-polymorphic: cl
// auto-stamps the vptr (??_7WorkerFull@@6B@; retail vtable 0x5efb80, 10 declared-
// only slots reloc-mask) and seeds the three context fields (b -> +0x04, c ->
// +0x08, a -> +0x0c), zeroing the rest. Manual `m_vptr = &g_animWorkerVtbl`
// stamp + the extern removed per the all-vtables mandate.
struct WorkerFull {
    virtual void Slot00(); // +0x00  vptr
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    WorkerFull(i32 a, i32 b, i32 c);
    i32 m_04; // +0x04  <- b
    i32 m_08; // +0x08  <- c
    i32 m_0c; // +0x0c  <- a
    i32 m_10;
    i32 m_14;
    i32 m_18;
    i32 m_1c;
    char m_pad20[0x170 - 0x20];
    i32 m_170;
    i32 m_174;
    i32 m_178;
};

// The arg-store order (b,c,a into m_04/m_08/m_0c) is load-bearing.
// @early-stop
// vptr-last wall: retail stamps the vptr AFTER m_04/m_08/m_0c, but a real-virtual
// class forces cl's implicit vptr-first store at ctor entry. Field-store order
// preserved; only the vptr position diverges (mandate: convert anyway).
RVA(0x0015b300, 0x40)
WorkerFull::WorkerFull(i32 a, i32 b, i32 c) {
    m_04 = b;
    m_08 = c;
    m_0c = a;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
    m_170 = 0;
    m_1c = 0;
    m_174 = 0;
    m_178 = 0;
}
SIZE_UNKNOWN(WorkerFull);
