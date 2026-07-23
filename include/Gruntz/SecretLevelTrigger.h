#ifndef GRUNTZ_CSECRETLEVELTRIGGER_H
#define GRUNTZ_CSECRETLEVELTRIGGER_H

#include <rva.h>

#include <Gruntz/UserLogic.h> // CUserLogic base (CSecretLevelTrigger : CUserLogic)

class CSecretLevelTrigger : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
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
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
    //         AnimWorkerHandlers `new CSecretLevelTrigger`)
};
SIZE(0x54);

typedef i32 (CUserLogic::*SecretActHandler)();
struct CSecretActEntry {
    SecretActHandler m_fn;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CSECRETLEVELTRIGGER_H
