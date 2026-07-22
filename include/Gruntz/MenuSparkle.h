#ifndef GRUNTZ_CMENUSPARKLE_H
#define GRUNTZ_CMENUSPARKLE_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CMenuSparkle : CUserLogic)

class CMenuSparkle : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00010160, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_MENUSPARKLE;
    } // slot 2
public:
    CMenuSparkle(CGameObject* obj); // 0x0adbe0
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
    // Dispatch (0x0ade60) IS this class's vtable slot 4 (??_7CMenuSparkle@@6B@+0x10 ->
    // 0xade60 via ILT thunk 0x19b0; vtable_hierarchy: slot 4 `override`, origin
    // CUserLogic). It is a plain method, not the OVERRIDE, because the CUserLogic base
    // models slot 4 with the no-arg UserLogicVfunc2() placeholder above while the real
    // slot is int-arg (retail's base body thunk 0x246e -> 0x8b70 and this override both
    // `ret 4`) - the same documented workaround the ~40 sibling leaves use for their
    // RunAct/FireActivation slot-4 bodies. Per-coordinate activation dispatch over this
    // leaf's own table g_logicActReg_646010 (0x646010).
    virtual void FireActivation(i32 id) OVERRIDE; // 0x0ade60
    // The per-frame handler (@0x0ae2a0): tick the aux flicker countdown, advance the
    // +0x1a0 anim on expiry, then re-arm the random flicker delay.
    i32 AdvanceAnim();
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CMENUSPARKLE_H
