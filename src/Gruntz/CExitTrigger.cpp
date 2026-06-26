// CExitTrigger.cpp - the level-exit trigger tile-logic game-object (C:\Proj\Gruntz).
//
// Two CExitTrigger methods, defined in ascending retail-RVA order:
//   GetTypeTag     @0x010870 - the 6-byte per-class logic-type id accessor (0x3f7).
//   ~CExitTrigger  @0x0108c0 - the /GX leaf dtor (folds the CUserLogic teardown).
//
// CExitTrigger : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CExitTrigger.h>

// CExitTrigger::GetTypeTag @0x010870 - return the class's logic-type id. The same
// 6-byte `mov eax,<id>; ret` virtual archetype as CTileTriggerTransition::
// GetTypeTag (0x011730).
RVA(0x00010870, 0x6)
i32 CExitTrigger::GetTypeTag() {
    return 0x3f7;
}

// CExitTrigger::~CExitTrigger @0x0108c0 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CTimeBomb @0x012a70;
// the empty body is enough for cl.
RVA(0x000108c0, 0x44)
CExitTrigger::~CExitTrigger() {}
