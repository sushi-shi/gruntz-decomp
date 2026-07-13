// SimpleAnimation.h - a simple eyecandy animation game-object (C:\Proj\Gruntz).
//
// CSimpleAnimation : CUserLogic - a tile-logic leaf in the same game-object
// hierarchy as CGruntPuddle (proven by its dtor @0x00f9d0 stamping the
// CUserLogic vftable 0x5e705c then the CUserBase vftable 0x5e70b4, tearing down
// the +0x18 link via the embedded ~EngStr at 0x16d2a0 - byte-identical in shape
// to ~CGruntPuddle @0x010d10 / the established leaf-dtor archetype). The leaf
// adds no destructible members beyond CUserLogic, so its dtor folds the bare
// CUserLogic teardown (the /GX leaf-dtor archetype).
//
// AdvanceAnim (0x0abf70) is the per-frame animation-advance: re-target the bound
// object's animation sub-object (m_38 + 0x1a0) to the current draw-delta
// (g_engineFrameDelta) and return 0 - the SAME archetype as CGruntPuddle's remove-path
// notify (((CGruntPuddleSink*)((char*)m_38 + 0x1a0))->Notify(g_engineFrameDelta)) and
// CProjectile::DetachRenderObj's SetAnim(g_engineFrameDelta).
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CSIMPLEANIMATION_H
#define GRUNTZ_CSIMPLEANIMATION_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CSimpleAnimation : CUserLogic)

class CSimpleAnimation : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x0000f910, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SIMPLEANIMATION;
    } // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE; // slot 4
    TILE_LOGIC_TAIL
public:
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0x00f930 (slot-1 body)
    CSimpleAnimation(CGameObject* obj);           // 0x0ab940 (ctor body in UserLogic.cpp)
    i32 AdvanceAnim(); // 0x0abf70 (re-target bound anim to the draw-delta; ret 0)
    // Index g_simpleAnimDispatch by idx; if the resolved slot holds a handler,
    // invoke it as a PMF on this (ResolveSlot inlined twice). 0x0abc10.
    void Dispatch(i32 idx);
    virtual ~CSimpleAnimation() OVERRIDE; // 0x00f9d0 (folds the CUserLogic teardown)
    char m_pad40
        [0x54 - 0x40]; // +0x40..0x53 (leaf tail; sizeof from `new CSimpleAnimation` @0xa9f60)
};
VTBL(CSimpleAnimation, 0x1e8544);
SIZE(CSimpleAnimation, 0x54);

#endif // GRUNTZ_CSIMPLEANIMATION_H
