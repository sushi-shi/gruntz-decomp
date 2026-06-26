// CGruntStaminaSprite.cpp - the grunt stamina-bar eyecandy sprite (C:\Proj\Gruntz).
//
// CGruntStaminaSprite : CUserLogic (the base hierarchy comes from
// <Gruntz/UserLogic.h>). Only offsets / code bytes are load-bearing; names are
// placeholders for the recovered engine identities.
#include <Gruntz/CGruntStaminaSprite.h>

// CGruntStaminaSprite::~CGruntStaminaSprite @0x00012070 - the leaf adds no
// destructible members beyond CUserLogic, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to
// ~CGruntHealthSprite @0x00011fb0; the empty body is enough for cl.
RVA(0x00012070, 0x44)
CGruntStaminaSprite::~CGruntStaminaSprite() {}
