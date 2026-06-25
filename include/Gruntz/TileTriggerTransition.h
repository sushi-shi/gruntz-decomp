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
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#ifndef GRUNTZ_TILETRIGGERTRANSITION_H
#define GRUNTZ_TILETRIGGERTRANSITION_H

#include <Ints.h>
#include <rva.h>

// CButeTree + g_buteTree come via UserLogic.h (pulls <Bute/ButeMgr.h>); the leaf
// data layout (CGameObject/CGameObjAux/CUserLogic) is modeled there too.
#include <Gruntz/UserLogic.h>

// The +0x1b4 animation descriptor CGameObject hangs (ApplyAnimation walks it):
// a count at +0x10, a data-array pointer at +0x0c whose first element's +0x14 is
// the seed frame, and the descriptor's own +0x14 is the geometry token.
struct CAnimDesc {
    char m_pad00[0x0c];
    void** m_0c; // +0x0c  data array (element[0] is a CAnimDesc*)
    i32 m_10;    // +0x10  count
    i32 m_14;    // +0x14  geometry token
};

// ---------------------------------------------------------------------------
// CTileTransitionController - the per-object state machine that lives in the
// CGameObject's aux sub-object (CGameObject::m_7c). StepController dispatches on
// the state id and, for state 0, builds the CTileTriggerTransition state object.
//   +0x18  the current state object (a CTileTriggerTransition-family leaf)
//   +0x1c  the state id (0, 0x1d, 0x1e, 0x50..0x53, 0x3e8, ...)
// ---------------------------------------------------------------------------
// The state object reached through the controller: the pump dispatches by state
// id to its vtable slots at byte offsets 0x18/0x28/0x2c/0x30/0x34/0x38/0x3c
// (vtable indices 6, 10, 11, 12, 13, 14, 15). Modeled polymorphically: declaring
// the 16 virtuals makes MSVC lower `state->SlotN()` to the right `call [vptr+N]`
// thiscall with no cast. The bodies are external (no-body) so the calls reloc-mask.
class CTileTransitionState {
public:
    virtual void V0();
    virtual void V1();
    virtual void V2();
    virtual void V3();
    virtual void V4();
    virtual void V5();
    virtual void Enter(); // index 6  (+0x18)
    virtual void V7();
    virtual void V8();
    virtual void V9();
    virtual void Slot28(); // index 10 (+0x28)
    virtual void Slot2c(); // index 11 (+0x2c)
    virtual void Slot30(); // index 12 (+0x30)
    virtual void Slot34(); // index 13 (+0x34)
    virtual void Slot38(); // index 14 (+0x38)
    virtual void Slot3c(); // index 15 (+0x3c)
};

struct CTileTransitionController {
    char m_pad00[0x18];
    CTileTransitionState* m_18; // +0x18  current state object
    u32 m_1c;                   // +0x1c  state id (unsigned: the pump uses ja/jbe)
};

// The default-state engine helper (FUN_0056e4f0, __cdecl, takes the state object).
extern "C" void TileTransitionDefaultStep(CTileTransitionState* obj);

#endif // GRUNTZ_TILETRIGGERTRANSITION_H
