// ToobSpikez.h - the toob-spikez hazard game-object (C:\Proj\Gruntz).
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

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CToobSpikez : CUserLogic)

class CToobSpikez : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    CToobSpikez(CGameObject* obj); // 0x1145c0 (ctor body in UserLogic.cpp)
    // The class's own CUserLogic slot overrides, reconstructed as regular methods
    // (the fat base models slots 1/2 with placeholder signatures; see the .cpp).
    // 0x00012ba0 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x00012ba0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TOOBSPIKEZ;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    i32 Serialize(i32 a, i32 b, i32 c, i32 d); // 0x012bc0 (vtable slot 1: serialize chain)
    void Register_1147e0();                    // 0x1147e0 (reserve the activation range)
    void FireActivation(i32 coord);            // 0x114860 (vtable slot 4)
    static void RegisterActs();                // 0x1149c0 (binds the logic handler to key "A";
                                               //  static: no this, called this-less by the factory)
    virtual ~CToobSpikez() OVERRIDE;           // 0x012c60 (folds the CUserLogic teardown)

    CAniElement* m_40;         // +0x40  saved active-anim descriptor (ctor snapshot)
    char m_pad44[0x54 - 0x44]; // +0x44  leaf tail (true object size 0x54, per the
                               //         `operator new(0x54)` at the phase-0 factory)
};
VTBL(CToobSpikez, 0x001e7774);
SIZE(CToobSpikez, 0x54);

#endif // GRUNTZ_CTOOBSPIKEZ_H
