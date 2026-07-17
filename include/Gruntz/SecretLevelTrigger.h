// SecretLevelTrigger.h - the secret-level trigger tile-logic game object
// (C:\Proj\Gruntz).
//
// CSecretLevelTrigger : CUserLogic - a method-only leaf (no data members beyond
// the CUserLogic base). Its methods are split across two TUs: the no-arg/1-arg
// ctors in src/Gruntz/UserLogic.cpp, and Init/RegisterActs + Tick + dtor in
// src/Gruntz/SecretTeleporterTrigger.cpp (its TU, wave3-J). This header unifies the two per-TU
// redeclarations (matching-neutral: no members, only the CUserLogic dtor slot is
// overridden). Only offsets/code bytes are load-bearing; names are placeholders.
#ifndef GRUNTZ_CSECRETLEVELTRIGGER_H
#define GRUNTZ_CSECRETLEVELTRIGGER_H

#include <rva.h>

#include <Gruntz/UserLogic.h> // CUserLogic base (CSecretLevelTrigger : CUserLogic)

SIZE(CSecretLevelTrigger, 0x54);
class CSecretLevelTrigger : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00010b90, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SECRETLEVELTRIGGER;
    } // slot 2
public:
    CSecretLevelTrigger();                 // 0x010b20 (no-arg ctor; body in UserLogic.cpp)
    CSecretLevelTrigger(CGameObject* obj); // 0x0424b0 (1-arg ctor; body in UserLogic.cpp)
    static void InitActReg();              // 0x0426e0 (construct g_secretActReg over [2000,2010])
    static void RegisterActs();            // 0x0428c0 (register the class's activation handlers)
    virtual void FireActivation(i32 id) OVERRIDE; // 0x042760 (per-coord PMF dispatcher)
    i32 Tick();                                   // 0x042ac0
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
                               //         AnimWorkerHandlers `new CSecretLevelTrigger`)
};
VTBL(CSecretLevelTrigger, 0x1e8804);

// The secret-level trigger's activation-registry entry: its first dword is the
// Tick handler PMF (a 4-byte code pointer on this single-inheritance class).
// Declared AFTER the complete class so the PMF stays 4 bytes.
typedef i32 (CUserLogic::*SecretActHandler)();
struct CSecretActEntry {
    SecretActHandler m_fn;
};
SIZE_UNKNOWN(CSecretActEntry);

#endif // GRUNTZ_CSECRETLEVELTRIGGER_H
