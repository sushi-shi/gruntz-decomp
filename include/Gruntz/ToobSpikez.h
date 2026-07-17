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

class CToobSpikez : public CUserLogic, public CWapX {
public:
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
    void Register_1147e0();                       // 0x1147e0 (reserve the activation range)
    virtual void FireActivation(i32 id) OVERRIDE; // 0x114860 (vtable slot 4)
    static void RegisterActs();                   // 0x1149c0 (binds the logic handler to key "A";
    //  static: no this, called this-less by the factory)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
};
VTBL(CToobSpikez, 0x001e7774);
SIZE(CToobSpikez, 0x54);

#endif // GRUNTZ_CTOOBSPIKEZ_H
