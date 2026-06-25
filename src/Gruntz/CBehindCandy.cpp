// CBehindCandy.cpp - the behind-candy eyecandy tile-logic game-object (C:\Proj\Gruntz).
//
// Two CBehindCandy methods, defined in ascending retail-RVA order:
//   GetTypeTag      @0x00fb70 - the 6-byte per-class logic-type id accessor (0x3f0).
//   ~CBehindCandy   @0x00fc30 - the /GX leaf dtor (folds the CUserLogic teardown).
//
// CBehindCandy : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CBehindCandy.h>

// CBehindCandy::GetTypeTag @0x00fb70 - return the class's logic-type id. The same
// 6-byte `mov eax,<id>; ret` virtual archetype as CTileTriggerTransition::
// GetTypeTag (0x011730).
RVA(0x0000fb70, 0x6)
i32 CBehindCandy::GetTypeTag() {
    return 0x3f0;
}

// CBehindCandy::~CBehindCandy @0x00fc30 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CTimeBomb @0x012a70;
// the empty body is enough for cl.
RVA(0x0000fc30, 0x44)
CBehindCandy::~CBehindCandy() {}
