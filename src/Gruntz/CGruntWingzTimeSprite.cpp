// CGruntWingzTimeSprite.cpp - the grunt "wingz" time eyecandy sprite (C:\Proj\Gruntz).
//
// Two trace-discovered CGruntWingzTimeSprite methods, defined in ascending
// retail-RVA order:
//   ~CGruntWingzTimeSprite @0x0121f0 - the /GX leaf dtor (folds the CUserLogic teardown).
//   GetWingzTime           @0x07fd90 - a tiny __stdcall +0x3f8 accessor (ret 4).
//
// CGruntWingzTimeSprite : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CGruntWingzTimeSprite.h>

// CGruntWingzTimeSprite::GetTypeTag @0x0121a0 - return the class's logic-type id.
// The same 6-byte `mov eax,<id>; ret` virtual archetype as CBehindCandy::GetTypeTag
// (0x00fb70); precedes the [scalar @0x121c0, plain @0x121f0] dtor pair.
RVA(0x000121a0, 0x6)
i32 CGruntWingzTimeSprite::GetTypeTag() {
    return 0x417;
}

// CGruntWingzTimeSprite::~CGruntWingzTimeSprite @0x0121f0 - the leaf adds no
// destructible members beyond CUserLogic, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to the
// established leaf dtors; the empty body is enough for cl.
RVA(0x000121f0, 0x44)
CGruntWingzTimeSprite::~CGruntWingzTimeSprite() {}

// CGruntWingzTimeSprite::GetWingzTime @0x07fd90 - read the bound object's +0x3f8
// wingz-timer field and return it. __stdcall (single stack arg, callee cleanup -
// `mov eax,[esp+4]; mov eax,[eax+0x3f8]; ret 4`).
RVA(0x0007fd90, 0xd)
i32 __stdcall CGruntWingzTimeSprite::GetWingzTime(CGrunt* o) {
    return o->m_wingzTime;
}
