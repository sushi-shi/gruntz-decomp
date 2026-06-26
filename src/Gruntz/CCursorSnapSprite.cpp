// CCursorSnapSprite.cpp - the cursor-snap sprite game object (C:\Proj\Gruntz).
//
// Two trace-discovered CCursorSnapSprite methods, defined in ascending retail-RVA
// order:
//   Serialize         @0x011880 - the two-chain Serialize override.
//   ~CCursorSnapSprite @0x011920 - the /GX leaf dtor (folds the CUserLogic teardown).
//
// CCursorSnapSprite : CUserLogic (RTTI .?AVCCursorSnapSprite@@). Only offsets /
// code bytes are load-bearing; names are placeholders for the recovered engine
// identities.
#include <Gruntz/CCursorSnapSprite.h>

// CCursorSnapSprite::Serialize @0x011880 - chain the shared CUserLogic serialize
// helper on `this`, and (only on success) the +0x34 sub-object's chain; both run
// the same (ar, tag, c, d) tuple. Returns the second chain's success normalized
// to a bool (the retail neg/sbb/neg idiom). The SAME archetype as
// CFortressFlag::Serialize (0x46410), minus the tag-8 sprite fixup.
RVA(0x00011880, 0x47)
i32 CCursorSnapSprite::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialSub34*)((char*)this + 0x34))->Chain(ar, tag, c, d) != 0;
}

// CCursorSnapSprite::~CCursorSnapSprite @0x011920 - the leaf adds no destructible
// members beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store
// the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CFortressFlag
// (0x010e90) / ~CTeleporter (0x010dd0); the empty body is enough for cl.
RVA(0x00011920, 0x44)
CCursorSnapSprite::~CCursorSnapSprite() {}
