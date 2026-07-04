// CMsgDispatch.cpp - three near-identical command dispatchers (orphan COMDATs
// @0xaa0a0 / 0xaa1e0 / 0xaa460) that route a UI message (arg->m_7c->m_1c) to the
// handler sub-object (m_18): command 0 lazily constructs the handler (new CObj(arg)
// + slot-6 init, /GX EH-framed), the mid commands fan out to its vtable slots, and
// the rest serialize through the shared default. The three differ only in their
// constructed handler class (reloc-masked ctor) + EH funclet, so one body matches
// all three. Placeholder names; only OFFSETS + code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>

struct CMsg;

// The handler object (m_18): 0x54 bytes, a polymorphic class whose vtable slots
// 6 / 10..15 are the per-command hooks (all __thiscall, no args). Real virtuals
// with placeholder slots so each `m_18->SlotNN()` emits `mov edx,[ecx];
// call [edx+0xNN]` for free (docs/patterns/dummy-virtual-slots.md). Constructed
// via the external ctor (reloc-masked); non-abstract so `new CObj` is legal.
class CObj {
public:
    CObj(CMsg* arg);       // 0x2e4b (external, no body)
    virtual void v00();    // +0x00
    virtual void v04();    // +0x04
    virtual void v08();    // +0x08
    virtual void v0c();    // +0x0c
    virtual void v10();    // +0x10
    virtual void v14();    // +0x14
    virtual void Slot18(); // +0x18 slot 6
    virtual void v1c();    // +0x1c
    virtual void v20();    // +0x20
    virtual void v24();    // +0x24
    virtual void Slot28(); // +0x28 slot 10
    virtual void Slot2c(); // +0x2c slot 11
    virtual void Slot30(); // +0x30 slot 12
    virtual void Slot34(); // +0x34 slot 13
    virtual void Slot38(); // +0x38 slot 14
    virtual void Slot3c(); // +0x3c slot 15
    char _pad[0x54 - 4];
};

struct CSub {
    char _00[0x18];
    CObj* m_18; // +0x18
    u32 m_1c;   // +0x1c  command id (unsigned switch)
};
struct CMsg {
    char _00[0x7c];
    CSub* m_7c; // +0x7c
};

// The shared default handler (cdecl, 1 arg).
extern "C" void DispatchDefault(void* obj); // 0x16e4f0

// The dispatcher body, identical across the three instantiations (the only
// retail difference is the reloc-masked ctor + EH funclet).
#define DISPATCH_BODY                                                                              \
    CSub* sub = arg->m_7c;                                                                         \
    switch (sub->m_1c) {                                                                           \
        case 0: {                                                                                  \
            sub->m_1c = 0x3e8;                                                                     \
            CObj* o = new CObj(arg);                                                               \
            o->Slot18();                                                                           \
            sub->m_18 = o;                                                                         \
            break;                                                                                 \
        }                                                                                          \
        case 0x1d:                                                                                 \
            sub->m_18->Slot2c();                                                                   \
            break;                                                                                 \
        case 0x1e:                                                                                 \
            sub->m_18->Slot28();                                                                   \
            break;                                                                                 \
        case 0x50:                                                                                 \
            sub->m_18->Slot38();                                                                   \
            break;                                                                                 \
        case 0x51:                                                                                 \
            sub->m_18->Slot34();                                                                   \
            break;                                                                                 \
        case 0x52:                                                                                 \
            sub->m_18->Slot30();                                                                   \
            break;                                                                                 \
        case 0x53:                                                                                 \
            sub->m_18->Slot3c();                                                                   \
            break;                                                                                 \
        case 0x3e8:                                                                                \
            break;                                                                                 \
        default:                                                                                   \
            DispatchDefault(sub->m_18);                                                            \
            break;                                                                                 \
    }                                                                                              \
    return 1;

RVA(0x000aa0a0, 0xf1)
i32 Dispatch_aa0a0(CMsg* arg){DISPATCH_BODY}

RVA(0x000aa1e0, 0xf1)
i32 Dispatch_aa1e0(CMsg* arg){DISPATCH_BODY}

RVA(0x000aa460, 0xf1)
i32 Dispatch_aa460(CMsg* arg){DISPATCH_BODY}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CMsg);
SIZE_UNKNOWN(CObj);
SIZE_UNKNOWN(CSub);
