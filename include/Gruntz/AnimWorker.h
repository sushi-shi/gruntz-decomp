// AnimWorker.h - the anim-worker message-pump object views shared by the anim-worker
// handler TUs (AnimWorkerHandlers.cpp = the trigger/point object handlers,
// GruntIndicatorWorkerHandlers.cpp = the grunt-HUD indicator-sprite handlers).
//
// The worker held at owner->m_7c is the same foreign-vtable-0x5efb80 object whose
// full 3-arg ctor lives on the canonical AnimWorkerObj (<DDrawMgr/AnimWorkerObj.h>,
// ctor @0x15b300 in WwdFactoryObject.cpp); `Worker` here is the
// reduced message-pump view (state tag + live sub-record). Unifying the two views
// into one class is deferred: the ctor is @early-stop on a vptr-last wall and the
// two facets sit in different TUs, so the union is a follow-up.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + emitted code bytes
// are load-bearing (campaign doctrine).
#ifndef GRUNTZ_ANIMWORKER_H
#define GRUNTZ_ANIMWORKER_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/XferArchive.h> // the real 0x16e4f0 = ProjTypeXfer(CXferArchive*)

class CUserLogic; // fwd; deref'd in the pump TUs via <Gruntz/UserLogic.h>

// (The `Worker` / `Owner` shells are DISSOLVED, 2026-07-19 - the "foreign vtable
// 0x5efb80" was ??_7AnimWorkerObj all along; see WorkerHandler.h. The real classes:)
#include <Wwd/WwdGameObjectFamily.h> // CGameObject (m_7c worker slot)
#include <DDrawMgr/AnimWorkerObj.h>  // AnimWorkerObj (m_logic / m_1c role-union)

// The engine default message pump run for any unhandled state IS the real shared
// coordinate/type-registry resolve at 0x16e4f0 (?ProjTypeXfer@@YAHPAUCXferArchive@@@Z,
// __cdecl). Thin forwarder so callers emit the bound rel32 (was fake _Worker_DefaultPump).
inline void Worker_DefaultPump(CUserLogic* sub) {
    ProjTypeXfer(reinterpret_cast<CXferArchive*>(sub));
}

#endif // GRUNTZ_ANIMWORKER_H
