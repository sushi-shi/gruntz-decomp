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
class CSecretLevelTrigger : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00010b90, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SECRETLEVELTRIGGER;
    } // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;       // slot 4
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0x010bb0 (vtable slot 1: serialize chain)
    TILE_LOGIC_TAIL
public:
    CSecretLevelTrigger();                   // 0x010b20 (no-arg ctor; body in UserLogic.cpp)
    CSecretLevelTrigger(CGameObject* obj);   // 0x0424b0 (1-arg ctor; body in UserLogic.cpp)
    static void InitActReg();                // 0x0426e0 (construct g_secretActReg over [2000,2010])
    static void RegisterActs();              // 0x0428c0 (register the class's activation handlers)
    void FireActivation(i32 coord);          // 0x042760 (per-coord PMF dispatcher)
    i32 Tick();                              // 0x042ac0
    virtual ~CSecretLevelTrigger() OVERRIDE; // 0x010c50 (folds the CUserLogic teardown)

    char m_pad40[0x54 - 0x40]; // +0x40  (unmodeled tail; size proven 0x54 from
                               //         AnimWorkerHandlers `new CSecretLevelTrigger`)
};
VTBL(CSecretLevelTrigger, 0x1e8804);

#endif // GRUNTZ_CSECRETLEVELTRIGGER_H
