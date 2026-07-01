// WorkerHandler.h - the shared archetype for the 0xf1-family worker message
// handlers (InGameWorkerHandlers / LogicWorkerHandlers / SiriusWorkerHandlers).
// Each handler reads owner->m_7c (the worker), then runs a /GX message pump keyed
// on the worker's UNSIGNED state tag worker->m_1c, `new`ing a polymorphic
// SubRecord on state 0 and dispatching vtable slots on the other states. Only the
// TU-local engine sub-record types (CInGameIcon / CDoNothing / ...) differ per
// family; the SubRecord base, Worker and Owner shapes are shared verbatim.
#ifndef GRUNTZ_GRUNTZ_WORKERHANDLER_H
#define GRUNTZ_GRUNTZ_WORKERHANDLER_H

#include <Ints.h>

// The polymorphic sub-record the worker `new`s and dispatches. Slots are laid
// out so the message pump's vtable calls land at the right byte offsets
// (0x18 = slot 6 activate, 0x28..0x3c = slots 10..15). Declarations only - never
// defined, so no ??_7 is emitted; the constructed object's real vtable is the
// engine class's, stamped by its own (extern) constructor.
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

#endif // GRUNTZ_GRUNTZ_WORKERHANDLER_H
