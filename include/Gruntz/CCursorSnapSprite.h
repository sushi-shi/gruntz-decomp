// CCursorSnapSprite.h - the cursor-snap sprite game object (C:\Proj\Gruntz).
//
// CCursorSnapSprite : CUserLogic (RTTI: .?AVCCursorSnapSprite@@ at 0x609b70). A
// tile-logic leaf in the same game-object hierarchy as CFortressFlag, proven by
// its dtor (0x11920) stamping the CUserLogic vftable 0x5e705c then the CUserBase
// vftable 0x5e70b4, tearing down the +0x18 link via the embedded ~EngStr at
// 0x16d2a0 (the /GX leaf-dtor archetype). The leaf adds no destructible members
// beyond CUserLogic, so the dtor folds the bare teardown.
//
// Serialize (0x11880) is the bare two-chain Serialize override: the shared
// CUserLogic serialize helper on `this`, then the +0x34 sub-object's chain - the
// SAME archetype as CFortressFlag::Serialize, minus the tag-8 fixup.
//
// Field names are placeholders; only the OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CCURSORSNAPSPRITE_H
#define GRUNTZ_CCURSORSNAPSPRITE_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CCursorSnapSprite : CUserLogic)

// The +0x34 serializable sub-object Serialize chains into after the shared
// CUserLogic::SerializeChain (the SAME archetype as CFortressFlag::Serialize). Its
// Chain (0x8c00, via the 0x1aff thunk) is __thiscall ret 0x10; modeled NO-body so
// the call reloc-masks.
struct CSerialSub34 {
    i32 Chain(i32 a, i32 b, i32 c, i32 d); // 0x8c00 (via 0x1aff thunk)
};

// ---------------------------------------------------------------------------
// CCursorSnapSprite : CUserLogic - the cursor-snap sprite leaf. Adds no data
// members the matched methods touch; the +0x34 serializable sub-object overlays
// the CUserLogic layout. The CUserLogic base gives the +0x18 destructible link,
// so the dtor folds the shared teardown.
// ---------------------------------------------------------------------------
class CCursorSnapSprite : public CUserLogic {
public:
    // Serialize (0x11880): chain the shared CUserLogic serialize helper on `this`,
    // then (only on success) the +0x34 sub-object's chain; both run the same
    // (ar, tag, c, d) tuple. Returns the second chain's success as a bool.
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d);
    ~CCursorSnapSprite(); // 0x11920 (folds the CUserLogic teardown)
};

#endif // GRUNTZ_CCURSORSNAPSPRITE_H
