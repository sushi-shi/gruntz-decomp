// GruntCreationPoint.h - the grunt creation-point game-object (C:\Proj\Gruntz).
//
// CGruntCreationPoint : CUserLogic - a tile-logic leaf in the same game-object
// hierarchy as CSimpleAnimation (proven by its dtor @0x010730 stamping the
// CUserLogic vftable 0x5e705c then the CUserBase vftable 0x5e70b4, tearing down
// the +0x18 link via the embedded ~EngStr at 0x16d2a0 - byte-identical in shape
// to ~CSimpleAnimation @0x00f9d0 / the established leaf-dtor archetype). The leaf
// adds no destructible members beyond CUserLogic, so its dtor folds the bare
// CUserLogic teardown (the /GX leaf-dtor archetype).
//
// AdvanceAnim (0x03ecc0) is the SAME per-frame animation-advance archetype as
// CSimpleAnimation::AdvanceAnim (0x0abf70): re-target the bound object's
// animation sub-object (m_38 + 0x1a0) to the current draw-delta (g_6bf3bc) and
// return 0.
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CGRUNTCREATIONPOINT_H
#define GRUNTZ_CGRUNTCREATIONPOINT_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CGruntCreationPoint : CUserLogic)

class CGruntCreationPoint : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CGruntCreationPoint(CGameObject* obj); // 0x3e520 (folds CUserLogic(obj) + tail)
    // Construct the class's activation-coordinate registry (g_creationPointActReg
    // @0x644700) over the fixed [2000,2010] range; free init thunk, reloc-masked.
    static void InitActReg(); // 0x03e8e0
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry (the same archetype as CBehindCandyAni::RegisterActs).
    static void RegisterActs(); // 0x03eac0
    i32 AdvanceAnim();          // 0x03ecc0 (re-target bound anim to the draw-delta; ret 0)
    virtual ~CGruntCreationPoint() OVERRIDE; // 0x010730 (folds the CUserLogic teardown)

    i32 m_savedGeoId; // +0x40  geometry id (m_38->m_geoId snapshot)
};
VTBL(CGruntCreationPoint, 0x1e81d4);

#endif // GRUNTZ_CGRUNTCREATIONPOINT_H
