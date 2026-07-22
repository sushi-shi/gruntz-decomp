#ifndef GRUNTZ_CSINGLEANIMATION_H
#define GRUNTZ_CSINGLEANIMATION_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CSingleAnimation : CUserLogic)

class CSingleAnimation : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
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
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
    // No own data: the two bases already span the whole object -
    // CUserLogic (0x34) + CWapX (0x20) == 0x54 == SIZE (sizeof corroborated by
    // `new CSingleAnimation` @0xaaaa0). The ex `m_pad40[0x54-0x40]` was the CWapX
    // base spelled as PADDING (the pre-CWapX model thought the base ended at +0x40
    // and the leaf added 0x14). It survived the CWapX sweep because the divergent
    // registrar shell in ObjTypeRegistrars.h made this class multiply-defined, and
    // class_sizes reports UNVERIFIABLE (not a mismatch) for a duplicated name - the
    // same way CBrickz::m_own hid. Same fix, same evidence.
};
SIZE(0x54);

typedef i32 (CUserLogic::*SingleAnimHandler)();
struct CSingleAnimActEntry {
    SingleAnimHandler m_fn;
};
SIZE_UNKNOWN();

#include <Gruntz/ActReg.h> // CSingleAnimActReg (extern below)
extern CSingleAnimActReg g_singleAnimActReg; // 0x00245f70

#endif // GRUNTZ_CSINGLEANIMATION_H
