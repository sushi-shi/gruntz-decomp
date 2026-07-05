// SecretLevelTrigger.h - the secret-level trigger tile-logic game object
// (C:\Proj\Gruntz).
//
// CSecretLevelTrigger : CUserLogic - a method-only leaf (no data members beyond
// the CUserLogic base). Its methods are split across two TUs: the no-arg/1-arg
// ctors in src/Gruntz/UserLogic.cpp, and Init/RegisterActs + Tick + dtor in
// src/Gruntz/SecretLevelTrigger.cpp. This header unifies the two per-TU
// redeclarations (matching-neutral: no members, only the CUserLogic dtor slot is
// overridden). Only offsets/code bytes are load-bearing; names are placeholders.
#ifndef GRUNTZ_CSECRETLEVELTRIGGER_H
#define GRUNTZ_CSECRETLEVELTRIGGER_H

#include <rva.h>

#include <Gruntz/UserLogic.h> // CUserLogic base (CSecretLevelTrigger : CUserLogic)

SIZE_UNKNOWN(CSecretLevelTrigger);
class CSecretLevelTrigger : public CTileLogic {
public:
    CSecretLevelTrigger();                   // 0x010b20 (no-arg ctor; body in UserLogic.cpp)
    CSecretLevelTrigger(CGameObject* obj);   // 0x0424b0 (1-arg ctor; body in UserLogic.cpp)
    static void InitActReg();                // 0x0426e0 (construct g_secretActReg over [2000,2010])
    static void RegisterActs();              // 0x0428c0 (register the class's activation handlers)
    i32 Tick();                              // 0x042ac0
    virtual ~CSecretLevelTrigger() OVERRIDE; // 0x010c50 (folds the CUserLogic teardown)
};

#endif // GRUNTZ_CSECRETLEVELTRIGGER_H
