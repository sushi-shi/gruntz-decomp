// SingleAnimation.h - a single-shot eyecandy animation game-object
// (C:\Proj\Gruntz).
//
// CSingleAnimation : CUserLogic - a tile-logic leaf in the same game-object
// hierarchy as CSimpleAnimation / CBehindCandyAni (proven by its dtor @0x010540
// stamping the CUserLogic vftable 0x5e705c then the CUserBase vftable 0x5e70b4,
// tearing down the +0x18 link via the embedded ~EngStr at 0x16d2a0 -
// byte-identical in shape to ~CSimpleAnimation @0x00f9d0 / the established
// leaf-dtor archetype). The leaf adds no destructible members beyond CUserLogic,
// so its dtor folds the bare CUserLogic teardown (the /GX leaf-dtor archetype).
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CSINGLEANIMATION_H
#define GRUNTZ_CSINGLEANIMATION_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CSingleAnimation : CUserLogic)

class CSingleAnimation : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0x104a0 (slot-1 two-chain body)
    RVA(0x00010480, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SINGLEANIMATION;
    } // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE; // slot 4
    TILE_LOGIC_TAIL
public:
    CSingleAnimation(CGameObject* obj); // 0x0ae7f0 (ctor body in UserLogic.cpp)
    static void InitActReg();   // 0x0ae9a0 (construct the activation registry over [2000,2010])
    i32 RunAct(i32 id);         // 0x0aea20 (resolve+dispatch the bound handler PMF on this)
    static void RegisterActs(); // 0x0aeb80 (bind the per-frame handler to key "A")
    // The per-frame handler (@0x0aed80); Ghidra did not carve it (recovery gap), so it
    // is declared only - RegisterActs takes its address as a reloc-masked operand.
    i32 AdvanceAnim();
    virtual ~CSingleAnimation() OVERRIDE; // 0x010540 (folds the CUserLogic teardown)
    char m_pad40
        [0x54 - 0x40]; // +0x40..0x53 (leaf tail; sizeof from `new CSingleAnimation` @0xaaaa0)
};
VTBL(CSingleAnimation, 0x1e745c);
SIZE(CSingleAnimation, 0x54);

#endif // GRUNTZ_CSINGLEANIMATION_H
