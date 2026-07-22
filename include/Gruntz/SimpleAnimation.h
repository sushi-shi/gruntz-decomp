#ifndef GRUNTZ_CSIMPLEANIMATION_H
#define GRUNTZ_CSIMPLEANIMATION_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CSimpleAnimation : CUserLogic)

class CSimpleAnimation : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x0000f910, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SIMPLEANIMATION;
    } // slot 2
public:
    CSimpleAnimation(CGameObject* obj); // 0x0ab940 (ctor body in UserLogic.cpp)
    i32 AdvanceAnim();                  // 0x0abf70 (re-target bound anim to the draw-delta; ret 0)
    // Index g_simpleAnimDispatch by idx; if the resolved slot holds a handler,
    // invoke it as a PMF on this (ResolveSlot inlined twice). 0x0abc10.
    virtual void FireActivation(i32 id) OVERRIDE;
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
};
SIZE(0x54);
VTBL(CSimpleAnimation, 0x1e8544);

#endif // GRUNTZ_CSIMPLEANIMATION_H
