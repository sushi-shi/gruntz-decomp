// CToobSpikez.h - the toob-spikez hazard game-object (C:\Proj\Gruntz).
//
// CToobSpikez : CUserLogic - a tile-logic leaf in the same game-object
// hierarchy as CTimeBomb (proven by its dtor @0x012c60 stamping the CUserLogic
// vftable 0x5e705c then the CUserBase vftable 0x5e70b4, tearing down the +0x18
// link via the embedded ~EngStr at 0x16d2a0 - byte-identical in shape to
// ~CTimeBomb @0x012a70; CToobSpikez is one of the CUserLogic tile-logic leaves
// listed in UserLogic.h). The leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown (the /GX leaf-dtor
// archetype).
//
// FireActivation (0x114860) is the SAME per-coordinate activation-registry
// dispatcher archetype as CTimeBomb::FireActivation (0x0e1830), but a DIFFERENT
// registry instance (CToobSpikez's, at 0x64e978).
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CTOOBSPIKEZ_H
#define GRUNTZ_CTOOBSPIKEZ_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CToobSpikez : CUserLogic)

class CToobSpikez : public CUserLogic {
public:
    // The class's own CUserLogic slot overrides, reconstructed as regular methods
    // (the fat base models slots 1/2 with placeholder signatures; see the .cpp).
    i32 GetTypeTag();                          // 0x012ba0 (vtable slot 2: per-class logic-type id)
    i32 Serialize(i32 a, i32 b, i32 c, i32 d); // 0x012bc0 (vtable slot 1: serialize chain)
    void Register_1147e0();                    // 0x1147e0 (reserve the activation range)
    void FireActivation(i32 coord);            // 0x114860 (vtable slot 4)
    void RegisterActs();                       // 0x1149c0 (binds the logic handler to key "A")
    virtual ~CToobSpikez() OVERRIDE;           // 0x012c60 (folds the CUserLogic teardown)
};

#endif // GRUNTZ_CTOOBSPIKEZ_H
