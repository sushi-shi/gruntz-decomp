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

// The worker held at owner->m_7c. Only the message-pump fields are modeled here.
struct Worker {
    virtual void VSlot0();     // +0x00  // real polymorphic vptr @+0x00 (was m_vptr)
    char m_pad04[0x18 - 0x04]; // +0x04..0x17
    CUserLogic* m_18;          // +0x18  the live sub-record (a CUserLogic game object)
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

#endif // GRUNTZ_GRUNTZ_WORKERHANDLER_H
