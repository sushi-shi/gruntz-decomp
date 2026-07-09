// SecretTeleporterTrigger.h - the secret-teleporter trigger (C:\Proj\Gruntz), a
// CUserLogic tile-logic leaf (RTTI .?AVCUserLogic@@, vftable 0x5e7564). Extracted
// from src/Gruntz/UserLogic.cpp (which still defines the out-of-line bodies) so the
// class can be `new`'d from the anim-worker dispatch handler. Size 0x54 proven from
// AnimWorkerHandlers' `new CSecretTeleporterTrigger`. Offsets + code bytes are
// load-bearing; field names are placeholders.
#ifndef GRUNTZ_CSECRETTELEPORTERTRIGGER_H
#define GRUNTZ_CSECRETTELEPORTERTRIGGER_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CSecretTeleporterTrigger : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CSecretTeleporterTrigger(CGameObject* obj); // 0x041e90
    virtual ~CSecretTeleporterTrigger() OVERRIDE;
    // Construct the class's activation-coordinate registry (g_actColl @0x644688)
    // over the fixed [2000,2010] range; a free init thunk, reloc-masked.
    static void InitActReg(); // 0x0420d0
    // Bind SpawnTeleporter to the activation key "A" via the shared name registry
    // (the same archetype as CSecretLevelTrigger::RegisterActs).
    static void RegisterActs(); // 0x0422b0
    // The two overridden CUserLogic virtuals reconstructed in UserLogic.cpp.
    i32 Serialize(i32 a, i32 b, i32 c, i32 d); // 0x010a10 (vtable slot 1)
    void FireActivation(i32 coord);            // 0x042150 (vtable slot 4)
    // The registered point-activation callback 0x042b80 stamped into the
    // coordinate registry by FireActivation. __thiscall, no args, returns int.
    i32 SpawnTeleporter(); // 0x042b80

    char m_pad40[0x54 - 0x40]; // +0x40  (unmodeled tail; size proven 0x54)
};
VTBL(CSecretTeleporterTrigger, 0x1e7564);
SIZE(CSecretTeleporterTrigger, 0x54);

#endif // GRUNTZ_CSECRETTELEPORTERTRIGGER_H
