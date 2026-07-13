// TileTriggerTransition.h - the CTileTriggerTransition tile-trigger state object
// and its state-machine pump (C:\Proj\Gruntz).
//
// CTileTriggerTransition is a CUserLogic leaf (vftable 0x5e7db4); its ctor/dtor
// live with the rest of the CUserLogic family in src/Gruntz/UserLogic.cpp. This
// header adds the three leaf methods + the controller's state pump that the
// trace lumped under the class but that live in their own retail TU:
//
//   0x011730  GetTypeTag      -> the per-class logic-type id (0x405)
//   0x110070  ApplyAnimation  -> seed geometry/sprite + swap in the "A" bute node
//   0x10d150  StepController   -> the aux-controller state pump (free __cdecl)
//
// The leaf/controller/state-object fields are named from usage; only the OFFSETS
// + the emitted code bytes are load-bearing (campaign doctrine).
#ifndef GRUNTZ_TILETRIGGERTRANSITION_H
#define GRUNTZ_TILETRIGGERTRANSITION_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/XferArchive.h> // the real 0x16e4f0 = ProjTypeXfer(CXferArchive*)

// CButeTree + g_buteTree come via UserLogic.h (pulls <Bute/ButeMgr.h>); the leaf
// data layout (CGameObject/AnimWorkerObj/CUserLogic) is modeled there too.
#include <Gruntz/UserLogic.h>

// The +0x1b4 animation-descriptor collection CGameObject hangs (ApplyAnimation
// walks it, same shape CGrunt's anim resolvers use - see <Gruntz/Grunt.h>):
// element[0]'s +0x14 is the seed frame.
struct CAnimElem {
    char m_pad00[0x14];
    i32 m_14; // +0x14  seed frame
};

// (The former CTileTransitionController/CTileTransitionState pump views are
// DISSOLVED (2026-07-13 worker fold): the controller IS the canonical
// AnimWorkerObj (m_state == m_logic, m_stateId == m_1c) and the state object a
// 16-slot lens of the real CUserLogic vtable (Activate slot 6, the 0x1d/0x1e/
// 0x50..0x53 states = UserLogicVfunc9/8/C/B/A/D at slots 11/10/14/13/12/15).
// The pumps now dispatch the real classes; the default step calls ProjTypeXfer
// (0x16e4f0) directly.)

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_TILETRIGGERTRANSITION_H
