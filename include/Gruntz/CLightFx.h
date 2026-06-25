// CLightFx.h - the "LightFx" tile-logic game object (C:\Proj\Gruntz), a
// CUserLogic leaf (RTTI .?AVCUserLogic@@ / .?AVCUserBase@@; ctor 0x9cf00).
// sizeof 0x5c = CUserLogic(0x40) + 0x1c leaf data.
//
// This header declares ONLY the two leaf methods reconstructed so far
// (Activate 0x9d520 + RebindNode 0x9d770). The class spine (CUserLogic base,
// m_14/m_30/m_38/m_3c) comes from <Gruntz/UserLogic.h>; the leaf adds the
// +0x40/+0x54/+0x58 data words those two methods touch.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + code bytes
// are load-bearing. The ctor (0x9cf00) and the other sibling methods (0x9d320
// etc.) are not yet reconstructed.
#ifndef GRUNTZ_GRUNTZ_CLIGHTFX_H
#define GRUNTZ_GRUNTZ_CLIGHTFX_H

#include <rva.h>

#include <Gruntz/UserLogic.h>

class CLightFx : public CUserLogic {
public:
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
    ~CLightFx();

    // ----- leaf layout over CUserLogic(0x40) (placeholders; offsets load-bearing)
    i32 m_40; // +0x40  cached object layer base (m_38->m_1b4)
    char m_pad44[0x54 - 0x44];
    i32 m_54; // +0x54  latched anchor A
    i32 m_58; // +0x58  latched anchor B
};
SIZE(CLightFx, 0x5c);

#endif // GRUNTZ_GRUNTZ_CLIGHTFX_H
