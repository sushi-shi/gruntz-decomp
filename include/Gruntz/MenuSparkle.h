// MenuSparkle.h - the menu-sparkle eyecandy tile-logic leaf (C:\Proj\Gruntz), a
// CUserLogic game-object (vtable 0x5e82dc). The CANONICAL CMenuSparkle (ctor +
// full vtable); extracted from the former UserLogic.cpp-local view so the leaf dtor
// (0x101b0) homes onto the real class. NOTE: distinct from the Grunt.h-world serialize
// view in <Gruntz/MenuSparkleSerial.h> (documented dual-model, never coexist in a TU).
// Only offsets / code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_CMENUSPARKLE_H
#define GRUNTZ_CMENUSPARKLE_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CMenuSparkle : CUserLogic)

class CMenuSparkle : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00010160, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_MENUSPARKLE;
    } // slot 2
    TILE_LOGIC_TAIL
public:
    CMenuSparkle(CGameObject* obj); // 0x0adbe0
    virtual ~CMenuSparkle() OVERRIDE;
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
    CAniElement* m_40;         // +0x40  saved active-anim descriptor (ctor snapshot)
    char m_pad44[0x54 - 0x44]; // +0x44..0x53 (leaf is 0x54: its only new-site, the
                               // logic-worker pump @0xaa0a0, pushes 0x54)
};
VTBL(CMenuSparkle, 0x1e82dc);

#endif // GRUNTZ_CMENUSPARKLE_H
