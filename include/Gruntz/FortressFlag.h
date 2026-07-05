// FortressFlag.h - a fortress-flag game-object (C:\Proj\Gruntz).
//
// CFortressFlag : CUserLogic - a tile-logic leaf in the same game-object
// hierarchy as CSecretTeleporterTrigger (proven by its dtor @0x010e90 stamping
// the CUserLogic vftable 0x5e705c then the CUserBase vftable 0x5e70b4, tearing
// down the +0x18 link via the embedded ~EngStr at 0x16d2a0 - byte-identical in
// shape to ~CSecretTeleporterTrigger @0x010ab0). The leaf adds no destructible
// members beyond CUserLogic, so its dtor folds the bare CUserLogic teardown (the
// /GX leaf-dtor archetype).
//
// Serialize (0x046410) is the SAME chain-then-+0x34-subobject Serialize archetype
// as CSecretTeleporterTrigger::Serialize (0x010a10), plus a per-class tail that,
// when the serialize tag is 8 (a post-load fixup), looks the flag's sprite
// selector up in the level's sprite-ref table and re-seeds the bound sprite's
// state fields.
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CFORTRESSFLAG_H
#define GRUNTZ_CFORTRESSFLAG_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CFortressFlag : CUserLogic)

class CFortressFlag : public CTileLogic {
public:
    CFortressFlag(CGameObject* obj); // 0x045d30
    // Construct the class's activation-coordinate registry (g_fortressFlagActReg
    // @0x644638) over the fixed [2000,2010] range; free init thunk, reloc-masked.
    static void InitActReg(); // 0x046000
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry (the same archetype as CBehindCandyAni::RegisterActs).
    static void RegisterActs(); // 0x0461e0
    i32 AdvanceAnim();          // 0x0463e0 (re-target bound anim to the draw-delta; ret 0)
    void HandleFortConquered(); // 0x03f5f0 (per-frame fort-conquest check)
    // vtable slot 2 (per-class logic-type id); regular method - the fat CUserLogic
    // base models this slot with a placeholder signature (see CGuardPoint.cpp).
    LogicTypeId GetTypeTag();                     // 0x010e40
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0x046410 (vtable slot 1)
    virtual ~CFortressFlag() OVERRIDE;            // 0x010e90 (folds the CUserLogic teardown)

    i32 m_prevAnimNode; // +0x40  geometry id (m_38->m_1b4 snapshot)
};

#endif // GRUNTZ_CFORTRESSFLAG_H
