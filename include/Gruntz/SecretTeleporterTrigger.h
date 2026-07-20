#ifndef GRUNTZ_CSECRETTELEPORTERTRIGGER_H
#define GRUNTZ_CSECRETTELEPORTERTRIGGER_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CSecretTeleporterTrigger : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x000109f0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SECRETTELEPORTERTRIGGER;
    } // slot 2
public:
    CSecretTeleporterTrigger(CGameObject* obj); // 0x041e90
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
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
};
VTBL(CSecretTeleporterTrigger, 0x1e7564);
SIZE(CSecretTeleporterTrigger, 0x54);

typedef void (CUserLogic::*ActHandler)();
struct CActEntry {
    ActHandler m_fn; // [entry]
};
SIZE_UNKNOWN(CActEntry);
typedef i32 (CUserLogic::*SpawnHandler)();
struct CTelActEntry {
    SpawnHandler m_fn;
};
SIZE_UNKNOWN(CTelActEntry);

#endif // GRUNTZ_CSECRETTELEPORTERTRIGGER_H
