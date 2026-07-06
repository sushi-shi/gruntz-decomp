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

// CButeTree + g_buteTree come via UserLogic.h (pulls <Bute/ButeMgr.h>); the leaf
// data layout (CGameObject/CGameObjAux/CUserLogic) is modeled there too.
#include <Gruntz/UserLogic.h>

// The +0x1b4 animation-descriptor collection CGameObject hangs (ApplyAnimation
// walks it, same shape CGrunt's anim resolvers use - see <Gruntz/Grunt.h>):
// element[0]'s +0x14 is the seed frame.
struct CAnimElem {
    char m_pad00[0x14];
    i32 m_14; // +0x14  seed frame
};

// ---------------------------------------------------------------------------
// CTileTransitionController - the per-object state machine that lives in the
// CGameObject's aux sub-object (CGameObject::m_7c). StepController dispatches on
// the state id and, for state 0, builds the CTileTriggerTransition state object.
//   +0x18  the current state object (a CTileTriggerTransition-family leaf)
//   +0x1c  the state id (0, 0x1d, 0x1e, 0x50..0x53, 0x3e8, ...)
// This is the same worker/pump pattern as the anim-worker family - see
// src/Gruntz/AnimWorkerHandlers.cpp for the canonical model.
// ---------------------------------------------------------------------------
// The state object reached through the controller: the pump dispatches by state
// id to its vtable slots at byte offsets 0x18/0x28/0x2c/0x30/0x34/0x38/0x3c
// (vtable indices 6, 10, 11, 12, 13, 14, 15 of the shared CUserLogic interface;
// the leaf doesn't override them - they are empty base stubs, so the slots carry
// no recoverable verb beyond their offset). Modeled polymorphically: declaring
// the 16 virtuals makes MSVC lower `state->VfuncN()` to the right `call [vptr+N]`
// thiscall with no cast. The bodies are external (no-body) so the calls reloc-mask.
class CTileTransitionState {
public:
    virtual void Vfunc00();
    virtual void Vfunc04();
    virtual void Vfunc08();
    virtual void Vfunc0C();
    virtual void Vfunc10();
    virtual void Vfunc14();
    virtual void Activate(); // index 6  (+0x18)  post-construction enter
    virtual void Vfunc1C();
    virtual void Vfunc20();
    virtual void Vfunc24();
    virtual void Vfunc28(); // index 10 (+0x28)  state 0x1e
    virtual void Vfunc2C(); // index 11 (+0x2c)  state 0x1d
    virtual void Vfunc30(); // index 12 (+0x30)  state 0x52
    virtual void Vfunc34(); // index 13 (+0x34)  state 0x51
    virtual void Vfunc38(); // index 14 (+0x38)  state 0x50
    virtual void Vfunc3C(); // index 15 (+0x3c)  state 0x53
};
SIZE_UNKNOWN(CTileTransitionState);

struct CTileTransitionController {
    char m_pad00[0x18];
    CTileTransitionState* m_state; // +0x18  current state object
    u32 m_stateId;                 // +0x1c  state id (unsigned: the pump uses ja/jbe)
};
SIZE_UNKNOWN(CTileTransitionController);

// The default-state engine helper (FUN_0056e4f0, __cdecl, takes the state object).
extern "C" void TileTransitionDefaultStep(CTileTransitionState* obj);

#endif // GRUNTZ_TILETRIGGERTRANSITION_H
