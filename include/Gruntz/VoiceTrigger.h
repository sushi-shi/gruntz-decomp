#ifndef GRUNTZ_CVOICETRIGGER_H
#define GRUNTZ_CVOICETRIGGER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CVoiceTrigger : CUserLogic)

class CVoiceTrigger : public CUserLogic, public CWapX {
public:
public:
    CVoiceTrigger();                 // 0x013470 (no-arg ctor; body in UserLogic.cpp)
    CVoiceTrigger(CGameObject* obj); // 0x119b50 (1-arg leaf ctor; body in VoiceTrigger.cpp)
    // GetTypeTag (0x13550, ??_7CVoiceTrigger slot 2 -> this body; returns 0x42b).
    // 0x133b0 (the old binding) is CUFO's copy - it returns 0x426.
    RVA(0x00013550, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_VOICETRIGGER;
    }
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
    static void InitActReg(); // 0x11a320 (constructs g_vtrigColl @0x651500)
    virtual void FireActivation(i32 id)
        OVERRIDE;               // 0x11a3a0 (vtable slot 4 body: per-coord PMF dispatch)
    static void RegisterActs(); // 0x11a500 (binds Tick to the activation key "A"; static:
                                //  no this, called this-less by the game-object factory)
    i32 Tick();                 // 0x11a700
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
    //         the state pump's `new CVoiceTrigger` = new(0x54))
};
SIZE(0x54);

typedef void (CUserLogic::*VTrigHandler)();
struct CVTrigEntry {
    VTrigHandler m_fn; // [entry]  the registered handler PMF
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CVOICETRIGGER_H
