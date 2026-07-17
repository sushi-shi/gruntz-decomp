// TileTriggerSwitch.h - the tile-trigger-switch tile-logic game-object
// (C:\Proj\Gruntz), a CUserLogic leaf (RTTI game class, vtable 0x5e7f6c). The
// CANONICAL CTileTriggerSwitch, extracted from the former UserLogic.cpp-local view.
// NOTE: distinct from CTileTriggerSwitchLogic (the 4-virtual switch-matrix helper in
// <Gruntz/TileTriggerSwitchLogic.h>). Only offsets / code bytes are load-bearing.
#ifndef GRUNTZ_CTILETRIGGERSWITCH_H
#define GRUNTZ_CTILETRIGGERSWITCH_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CTileTriggerSwitch : CUserLogic)

SIZE(CTileTriggerSwitch, 0x54);
VTBL(CTileTriggerSwitch, 0x001e7f6c); // vtable_names -> code (RTTI game class)
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

// The activation-registry entry record (the .data CActReg row; 4-byte PMF).
typedef i32 (CUserLogic::*TileTriggerSwitchHandler)();
struct CTileTriggerSwitchActEntry {
    TileTriggerSwitchHandler m_fn;
};

#endif // GRUNTZ_CTILETRIGGERSWITCH_H
