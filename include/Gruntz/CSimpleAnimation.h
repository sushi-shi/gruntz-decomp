// CSimpleAnimation.h - a simple eyecandy animation game-object (C:\Proj\Gruntz).
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
// (g_6bf3bc) and return 0 - the SAME archetype as CGruntPuddle's remove-path
// notify (((CGruntPuddleSink*)((char*)m_38 + 0x1a0))->Notify(g_6bf3bc)) and
// CProjectile::DetachRenderObj's SetAnim(g_6bf3bc).
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CSIMPLEANIMATION_H
#define GRUNTZ_CSIMPLEANIMATION_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CSimpleAnimation : CUserLogic)

class CSimpleAnimation : public CUserLogic {
public:
    i32 AdvanceAnim();   // 0x0abf70 (re-target bound anim to the draw-delta; ret 0)
    ~CSimpleAnimation(); // 0x00f9d0 (folds the CUserLogic teardown)
};

#endif // GRUNTZ_CSIMPLEANIMATION_H
