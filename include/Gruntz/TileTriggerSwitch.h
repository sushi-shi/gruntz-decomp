#ifndef GRUNTZ_CTILETRIGGERSWITCH_H
#define GRUNTZ_CTILETRIGGERSWITCH_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CTileTriggerSwitch : CUserLogic)

class CTileTriggerSwitch : public CUserLogic, public CWapX {
    virtual LogicTypeId GetTypeTag() OVERRIDE; // slot 2
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
public:
    CTileTriggerSwitch(CGameObject* obj); // 0x10dc40
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
    static void InitActReg(); // 0x10de20
    virtual void FireActivation(i32 id)
        OVERRIDE;               // 0x10dea0 (vtable slot 4 body: per-coord PMF dispatch)
    static void RegisterActs(); // 0x10e000
    i32 AdvanceAnim();          // 0x10e200 (declared-only; recovery gap)
};
SIZE(0x54);

typedef i32 (CUserLogic::*TileTriggerSwitchHandler)();
struct CTileTriggerSwitchActEntry {
    TileTriggerSwitchHandler m_fn;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CTILETRIGGERSWITCH_H
