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

class CSingleAnimation : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00010480, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SINGLEANIMATION;
    } // slot 2
public:
    CSingleAnimation(CGameObject* obj); // 0x0ae7f0 (ctor body in UserLogic.cpp)
    static void InitActReg(); // 0x0ae9a0 (construct the activation registry over [2000,2010])
    virtual void FireActivation(i32 id)
        OVERRIDE;               // 0x0aea20 (resolve+dispatch the bound handler PMF on this)
    static void RegisterActs(); // 0x0aeb80 (bind the per-frame handler to key "A")
    // The per-frame handler (@0x0aed80); Ghidra did not carve it (recovery gap), so it
    // is declared only - RegisterActs takes its address as a reloc-masked operand.
    i32 AdvanceAnim();
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
    char m_pad40
        [0x54 - 0x40]; // +0x40..0x53 (leaf tail; sizeof from `new CSingleAnimation` @0xaaaa0)
};
VTBL(CSingleAnimation, 0x1e745c);
SIZE(CSingleAnimation, 0x54);

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance
// class). Defined after the complete class (the FortressFlag.h record pattern).
typedef i32 (CUserLogic::*SingleAnimHandler)();
struct CSingleAnimActEntry {
    SingleAnimHandler m_fn;
};

#endif // GRUNTZ_CSINGLEANIMATION_H
