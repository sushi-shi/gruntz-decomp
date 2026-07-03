// CGruntStaminaSprite.cpp - the grunt stamina-bar eyecandy sprite (C:\Proj\Gruntz).
//
// CGruntStaminaSprite : CUserLogic (the base hierarchy comes from
// <Gruntz/UserLogic.h>). Only offsets / code bytes are load-bearing; names are
// placeholders for the recovered engine identities.
#include <Gruntz/CGruntStaminaSprite.h>
#include <Gruntz/LogicTypeId.h>

// CGruntStaminaSprite::GetTypeTag @0x00012020 - return the class's logic-type id.
// The same 6-byte `mov eax,<id>; ret` virtual archetype as CBehindCandy::GetTypeTag
// (0x00fb70); precedes the [scalar @0x12040, plain @0x12070] dtor pair.
RVA(0x00012020, 0x6)
LogicTypeId CGruntStaminaSprite::GetTypeTag() {
    return LOGIC_GRUNTSTAMINASPRITE; // 0x410
}

// CGruntStaminaSprite::~CGruntStaminaSprite @0x00012070 - the leaf adds no
// destructible members beyond CUserLogic, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to
// ~CGruntHealthSprite @0x00011fb0; the empty body is enough for cl.
RVA(0x00012070, 0x44)
CGruntStaminaSprite::~CGruntStaminaSprite() {}

// GetStaminaTime @0x0007fbb0 - free __stdcall accessor: read the bound grunt's
// +0x3f0 stamina-timer field and return it (single stack arg, callee cleanup -
// `mov eax,[esp+4]; mov eax,[eax+0x3f0]; ret 4`). Not a sprite member: the ecx
// trace mis-homed this __stdcall callee (stale-ecx owner); it reads a foreign
// CGrunt and is never stored as a fn pointer.
RVA(0x0007fbb0, 0xd)
i32 __stdcall GetStaminaTime(CGrunt* o) {
    return o->m_stamina;
}
