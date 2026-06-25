// CStaticHazard.h - a static hazard game-object (C:\Proj\Gruntz).
//
// CStaticHazard : CUserLogic - a tile-logic leaf in the same game-object
// hierarchy as CTimeBomb (proven by its dtor @0x012b30 stamping the CUserLogic
// vftable 0x5e705c then the CUserBase vftable 0x5e70b4, tearing down the +0x18
// link via the embedded ~EngStr at 0x16d2a0 - byte-identical in shape to
// ~CTimeBomb @0x012a70). The leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown (the /GX leaf-dtor
// archetype).
//
// FireActivation (0x0fbbf0) is the SAME per-coordinate activation-registry
// dispatcher archetype as CTimeBomb::FireActivation (0x0e1830), but a DIFFERENT
// registry instance (CStaticHazard's, at 0x64e3d0).
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CSTATICHAZARD_H
#define GRUNTZ_CSTATICHAZARD_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CStaticHazard : CUserLogic)

class CStaticHazard : public CUserLogic {
public:
    void FireActivation(i32 coord); // 0x0fbbf0
    ~CStaticHazard();               // 0x012b30 (folds the CUserLogic teardown)
};

#endif // GRUNTZ_CSTATICHAZARD_H
