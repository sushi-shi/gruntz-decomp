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
    // 0x000133b0 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x000133b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_VOICETRIGGER;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    static void InitActReg(); // 0x11a320 (constructs g_vtrigColl @0x651500)
    virtual void FireActivation(i32 id)
        OVERRIDE;                      // 0x11a3a0 (vtable slot 4 body: per-coord PMF dispatch)
    static void RegisterActs();        // 0x11a500 (binds Tick to the activation key "A"; static:
                                       //  no this, called this-less by the game-object factory)
    i32 Tick();                        // 0x11a700
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
                                       //         the state pump's `new CVoiceTrigger` = new(0x54))
};
SIZE(0x54);

typedef void (CUserLogic::*VTrigHandler)();
struct CVTrigEntry {
    VTrigHandler m_fn; // [entry]  the registered handler PMF
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CVOICETRIGGER_H
