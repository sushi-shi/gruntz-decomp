// CBehindCandyAni.h - a behind-candy eyecandy animation game-object
// (C:\Proj\Gruntz).
//
// CBehindCandyAni : CUserLogic - a tile-logic leaf in the same game-object
// hierarchy as CGruntPuddle (proven by its dtor @0x0100f0 stamping the
// CUserLogic vftable 0x5e705c then the CUserBase vftable 0x5e70b4, tearing down
// the +0x18 link via the embedded ~EngStr at 0x16d2a0 - byte-identical in shape
// to ~CSimpleAnimation @0x00f9d0 / the established leaf-dtor archetype). The leaf
// adds no destructible members beyond CUserLogic, so its dtor folds the bare
// CUserLogic teardown (the /GX leaf-dtor archetype).
//
// AdvanceAnim (0x0adbb0) is the per-frame animation-advance: re-target the bound
// object's animation sub-object (m_38 + 0x1a0) to the current draw-delta
// (g_6bf3bc) and return 0 - byte-identical to CSimpleAnimation::AdvanceAnim
// (0x0abf70) save the call displacement.
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CBEHINDCANDYANI_H
#define GRUNTZ_CBEHINDCANDYANI_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CBehindCandyAni : CUserLogic)

class CBehindCandyAni : public CUserLogic {
public:
    // Construct the class's activation-coordinate registry (g_behindCandyActReg
    // @0x645f98) over the fixed [2000,2010] range; free init thunk, reloc-masked.
    static void InitActReg(); // 0x0ad7d0
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry (the same archetype as CSecretLevelTrigger::RegisterActs).
    static void RegisterActs(); // 0x0ad9b0
    i32 AdvanceAnim();          // 0x0adbb0 (re-target bound anim to the draw-delta; ret 0)
    i32 GetTypeTag();           // 0x010030 (vtable slot 2: returns the logic-type id 0x3f3)
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0x010050 (vtable slot 1: two-chain Serialize)
    virtual ~CBehindCandyAni() OVERRIDE;          // 0x0100f0 (folds the CUserLogic teardown)
};

#endif // GRUNTZ_CBEHINDCANDYANI_H
