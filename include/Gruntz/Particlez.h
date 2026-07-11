// Particlez.h - the particlez effect game-object (C:\Proj\Gruntz).
//
// CParticlez : CUserLogic - a tile-logic leaf in the same game-object hierarchy
// as CTimeBomb (proven by its dtor @0x012d90 stamping the CUserLogic vftable
// 0x5e705c then the CUserBase vftable 0x5e70b4, tearing down the +0x18 link via
// the embedded ~EngStr at 0x16d2a0 - byte-identical in shape to ~CTimeBomb
// @0x012a70). The leaf adds no destructible members beyond CUserLogic, so its
// dtor folds the bare CUserLogic teardown (the /GX leaf-dtor archetype).
//
// FireActivation (0x046d30) is the SAME per-coordinate activation-registry
// dispatcher archetype as CTimeBomb::FireActivation (0x0e1830), but a DIFFERENT
// registry instance (CParticlez's, at 0x644870).
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CPARTICLEZ_H
#define GRUNTZ_CPARTICLEZ_H

#include <rva.h>
#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CParticlez : CUserLogic)

class CParticlez : public CUserLogic {
public:
    TILE_LOGIC_TAIL
    // +0x40..+0x53 unmodeled tail (sizeof proven 0x54 by the worker-pump new-site
    // `push 0x54` @0x46850 LogicDispatchC, FortressFlag.cpp).
    char m_pad40[0x54 - 0x40];

public:
    CParticlez(CGameObject* obj); // 0x046ad0 (ctor body in UserLogic.cpp)
    // 0x00012cd0 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x00012cd0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_PARTICLEZ;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0x012cf0 (vtable slot 1: serialize chain)
    static void InitActReg();       // 0x046cb0 (construct g_partColl over [2000,2010])
    void FireActivation(i32 coord); // 0x046d30
    // Bind the per-frame handler (Update) to the activation key "A" via the shared
    // name registry (the same archetype as CSecretTeleporterTrigger::RegisterActs).
    static void RegisterActs();     // 0x046e90
    i32 Update();                   // 0x047090 (advance anim + on-screen latch; ret 0)
    virtual ~CParticlez() OVERRIDE; // 0x012d90 (folds the CUserLogic teardown)
};
VTBL(CParticlez, 0x001e7614);

#endif // GRUNTZ_CPARTICLEZ_H
