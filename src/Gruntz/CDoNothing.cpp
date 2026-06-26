// CDoNothing.cpp - the inert "do nothing" tile-logic game-object (C:\Proj\Gruntz).
//
// Two CDoNothing methods, defined in ascending retail-RVA order:
//   GetTypeTag    @0x00f6b0 - the 6-byte per-class logic-type id accessor (0x3ec).
//   ~CDoNothing   @0x00f770 - the /GX leaf dtor (folds the CUserLogic teardown).
//
// CDoNothing : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CDoNothing.h>

// CDoNothing::GetTypeTag @0x00f6b0 - return the class's logic-type id. The same
// 6-byte `mov eax,<id>; ret` virtual archetype as CTileTriggerTransition::
// GetTypeTag (0x011730).
RVA(0x0000f6b0, 0x6)
i32 CDoNothing::GetTypeTag() {
    return 0x3ec;
}

// CDoNothing::~CDoNothing @0x00f770 - the leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown: store the CUserLogic
// vptr (0x5e705c), inline-destruct the +0x18 link (the embedded ~EngStr call
// 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible link forces the
// /GX EH frame. Byte-identical in shape to ~CTimeBomb @0x012a70; the empty body is
// enough for cl.
RVA(0x0000f770, 0x44)
CDoNothing::~CDoNothing() {}
