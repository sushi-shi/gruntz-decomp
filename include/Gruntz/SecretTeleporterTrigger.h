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
    RVA(0x000109f0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SECRETTELEPORTERTRIGGER;
    } // slot 2
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
    virtual void FireActivation(i32 id) OVERRIDE; // 0x042150 (vtable slot 4)
    // The registered point-activation callback 0x042b80 stamped into the
    // coordinate registry by FireActivation. __thiscall, no args, returns int.
    i32 SpawnTeleporter(); // 0x042b80

    char m_pad40[0x54 - 0x40]; // +0x40  (unmodeled tail; size proven 0x54)
};
VTBL(CSecretTeleporterTrigger, 0x1e7564);
SIZE(CSecretTeleporterTrigger, 0x54);

// The activation-registry entry record: its first dword is a PMF of the trigger
// class. Single inheritance -> a 4-byte code pointer, so the store is a single-word
// `mov [entry],offset handler` and the dispatch is `mov ecx,this; call [entry]`.
// FireActivation reads it as an ActHandler (void); RegisterActs stamps
// SpawnTeleporter through the i32-returning SpawnHandler view of the same slot.
// (Declared AFTER the complete class so the PMF stays 4 bytes.)
typedef void (CSecretTeleporterTrigger::*ActHandler)();
struct CActEntry {
    ActHandler m_fn; // [entry]
};
SIZE_UNKNOWN(CActEntry);
typedef i32 (CSecretTeleporterTrigger::*SpawnHandler)();
struct CTelActEntry {
    SpawnHandler m_fn;
};
SIZE_UNKNOWN(CTelActEntry);

#endif // GRUNTZ_CSECRETTELEPORTERTRIGGER_H
