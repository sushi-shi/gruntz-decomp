// CTimeBomb.h - the timez-bomb pickup game-object (C:\Proj\Gruntz).
//
// CTimeBomb : CUserLogic - a tile-logic leaf in the same game-object hierarchy
// as CKitchenSlime (proven by its dtor stamping the CUserLogic vftable 0x5e705c
// then the CUserBase vftable 0x5e70b4, tearing down the +0x18 link via the
// embedded ~EngStr at 0x16d2a0 - byte-identical in shape to ~CKitchenSlime
// @0x013100). The leaf adds no destructible members beyond CUserLogic, so its
// dtor folds the bare CUserLogic teardown (the /GX leaf-dtor archetype).
//
// FireActivation (0x0e1830) is the SAME per-coordinate activation-registry
// dispatcher archetype as CKitchenSlime::FireActivation (0x0b2940), but a
// DIFFERENT registry instance (CTimeBomb's, at 0x64c780 vs the slime's at
// 0x646228). This is the archetype for the CParticlez / CStaticHazard /
// CToobSpikez / CDroppedObject 0x102-sized FireActivation siblings.
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CTIMEBOMB_H
#define GRUNTZ_CTIMEBOMB_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CTimeBomb : CUserLogic)

class CTimeBomb : public CUserLogic {
public:
    void FireActivation(i32 coord); // 0x0e1830
    void RegisterActs();            // 0x0e1990 (binds the logic handler to key "A")
    ~CTimeBomb();                   // 0x012a70 (folds the CUserLogic teardown)
};

#endif // GRUNTZ_CTIMEBOMB_H
