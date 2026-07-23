#ifndef GRUNTZ_CFRONTCANDYANI_H
#define GRUNTZ_CFRONTCANDYANI_H

#include <rva.h>
#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CFrontCandyAni : CUserLogic)

class CFrontCandyAni : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x0000fdd0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_FRONTCANDYANI;
    } // slot 2
public:
    CFrontCandyAni(CGameObject* obj); // 0x0acf40
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
    // The vtable slot-1 two-chain body (0xfdf0): the shared CUserLogic serialize
    // helper on `this`, then the +0x34 sub-object's chain. (0xfa60 was mis-attributed
    // here; the vtable read proves it is CFrontCandy::Serialize.)
    // Construct the class's activation-coordinate registry (g_frontCandyActReg
    // @0x6460b0) over the fixed [2000,2010] range; free init thunk, reloc-masked.
    // Look the activation coordinate up in the class registry; if its entry has a
    // bound handler, look it up again and dispatch it __thiscall on this. The SAME
    // archetype as CParticlez::FireActivation (0x046d30).
    virtual void FireActivation(i32 id) OVERRIDE; // 0x0ad1b0
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry; the same archetype as CBehindCandyAni::RegisterActs.
    static void RegisterActs(); // 0x0ad310
    i32 AdvanceAnim();          // 0x0ad510 (re-target bound anim to the draw-delta; ret 0)
};
SIZE_UNKNOWN();

typedef i32 (CUserLogic::*FrontCandyHandler)();
struct CFrontCandyActEntry {
    FrontCandyHandler m_fn;
};
SIZE_UNKNOWN();

#include <Gruntz/ActReg.h> // CActReg (extern below)

#endif // GRUNTZ_CFRONTCANDYANI_H
