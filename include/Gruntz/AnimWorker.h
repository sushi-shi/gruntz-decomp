// AnimWorker.h - the anim-worker message-pump object views shared by the anim-worker
// handler TUs (AnimWorkerHandlers.cpp = the trigger/point object handlers,
// GruntIndicatorWorkerHandlers.cpp = the grunt-HUD indicator-sprite handlers).
//
// The worker held at owner->m_7c is the same foreign-vtable-0x5efb80 object whose
// full 3-arg ctor (WorkerFull) lives in AnimWorkerCtor.cpp; `Worker` here is the
// reduced message-pump view (state tag + live sub-record). Unifying the two views
// into one class is deferred: the ctor is @early-stop on a vptr-last wall and the
// two facets sit in different TUs, so the union is a follow-up.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + emitted code bytes
// are load-bearing (campaign doctrine).
#ifndef GRUNTZ_ANIMWORKER_H
#define GRUNTZ_ANIMWORKER_H

#include <Ints.h>

class CUserLogic; // fwd; deref'd in the pump TUs via <Gruntz/UserLogic.h>

// The worker held at owner->m_7c (foreign vtable 0x5efb80). Only the message-pump
// fields are modeled here.
struct Worker {
    char _vft0[4];             // +0x00 foreign object vptr (reduced view; not dispatched)
    char m_pad04[0x18 - 0x04]; // +0x04..0x17
    CUserLogic* m_18;          // +0x18  the live sub-record
    u32 m_1c;                  // +0x1c  state tag (UNSIGNED switch key)
};

// The owner game object handed to each handler; its worker hangs at +0x7c.
struct Owner {
    char m_pad00[0x7c];
    Worker* m_7c; // +0x7c
};

// The engine default message pump run for any unhandled state (0x16e4f0, __cdecl,
// takes the sub-record). Reloc-masked rel32 - no body.
extern "C" void Worker_DefaultPump(CUserLogic* sub);

#endif // GRUNTZ_ANIMWORKER_H
