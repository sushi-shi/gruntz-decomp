#ifndef GRUNTZ_GRUNTZ_CLIGHTFX_H
#define GRUNTZ_GRUNTZ_CLIGHTFX_H

#include <rva.h>

#include <Gruntz/UserLogic.h>

class CLightFx : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x000123e0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_LIGHTFX;
    } // slot 2
public:
    CLightFx(CGameObject* obj); // 0x9cf00
    // 0x9d1c0  RunAct - the slot-4 impl: resolve the class registry entry for id and,
    // if a handler PMF is bound, re-resolve + dispatch it on this (double inline
    // ResolveEntry), else return the entry pointer. Same archetype as CEyeCandyAni::RunAct.
    virtual void FireActivation(i32 id) OVERRIDE;
    // 0x9d660  SerializeMove (slot 1): chain the base serialize + the +0x34 object
    // reference, then per mode read/write the (anchorA, anchorB) pair or (mode 8) re-push
    // the effect into the logic pump. (definition uses the slot-1 virtual below)
    // 0x9d140  InitActReg - construct the class's coordinate registry singleton
    // (g_lightFxActReg @0x645ad0) over the fixed [2000, 2010] range. Static.
    static void InitActReg();
    // 0x9d320  RegisterActs - intern the activation key "A" and bind the per-frame
    // handler (AdvanceAnim) into the class's coordinate registry. Static.
    static void RegisterActs();
    // 0x9d7b0  AdvanceAnim - the per-frame animation-advance handler PMF.
    i32 AdvanceAnim();
    // 0x9d520  Activate - look the effect spec up in the object's bute map, prime
    // the bound object's layer descriptor, latch the cell anchor, push the effect
    // node, then rebind. __thiscall, 4 args, ret 0x10.
    i32 Activate(i32 spec, i32 anchorA, i32 effect, i32 anchorB);
    // 0x9d770  RebindNode - save the object map's current bute node, re-point it
    // at the "A" section, return 0. __thiscall, no args.
    i32 RebindNode();

    // 0x12430  the /GX leaf dtor - shares CUserLogic's vtable (adds no own
    // virtuals), so the most-derived vptr store is dead-eliminated and only the
    // folded CUserLogic teardown remains (the 0x44 leaf-dtor archetype).
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).

    // ----- leaf layout over CUserLogic(0x40) (offsets load-bearing)
    i32 m_anchorA; // +0x54  latched anchor A
    i32 m_anchorB; // +0x58  latched anchor B
};
SIZE(0x5c);
VTBL(CLightFx, 0x1e7af4);

typedef i32 (CUserLogic::*LightFxHandler)();
struct CLightFxActEntry {
    LightFxHandler m_fn;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_CLIGHTFX_H
