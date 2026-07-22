#ifndef GRUNTZ_CGRUNTCREATIONPOINT_H
#define GRUNTZ_CGRUNTCREATIONPOINT_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CGruntCreationPoint : CUserLogic)

class CGruntCreationPoint : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x000106e0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTCREATIONPOINT;
    } // slot 2
public:
    CGruntCreationPoint(CGameObject* obj); // 0x3e520 (folds CUserLogic(obj) + tail)
    // Construct the class's activation-coordinate registry (g_creationPointActReg
    // @0x644700) over the fixed [2000,2010] range; free init thunk, reloc-masked.
    static void InitActReg(); // 0x03e8e0
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry (the same archetype as CBehindCandyAni::RegisterActs).
    static void RegisterActs(); // 0x03eac0
    // Serialize (0x3e7a0): two-chain serialize (CUserLogic base + the +0x34 sub-
    // object); on the post-load tag (8) re-resolve the selected sprite from the game
    // registry's ref-index array (same selector as the ctor) into the draw trio.
    // Per-coordinate activation dispatcher: look the coordinate up in the class
    // registry (g_creationPointActReg) and, if it has a registered handler PMF,
    // dispatch it on `this`. Same archetype as CParticlez::FireActivation.
    virtual void FireActivation(i32 id) OVERRIDE; // 0x03e960
    i32 AdvanceAnim(); // 0x03ecc0 (re-target bound anim to the draw-delta; ret 0)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
};
SIZE(0x54);
VTBL(CGruntCreationPoint, 0x1e81d4);

#endif // GRUNTZ_CGRUNTCREATIONPOINT_H
