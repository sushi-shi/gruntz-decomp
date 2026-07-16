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
class CTileTriggerSwitch : public CUserLogic {
    virtual LogicTypeId GetTypeTag() OVERRIDE; // slot 2
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CTileTriggerSwitch(CGameObject* obj); // 0x10dc40
    virtual ~CTileTriggerSwitch() OVERRIDE;
    static void InitActReg();       // 0x10de20
    void FireActivation(i32 coord); // 0x10dea0 (vtable slot 4 body: per-coord PMF dispatch)
    static void RegisterActs();     // 0x10e000
    i32 AdvanceAnim();              // 0x10e200 (declared-only; recovery gap)
    char m_pad40[0x54 - 0x40];      // +0x40 (unmodeled leaf tail; size 0x54 from `new(0x54)`)
};

// The activation-registry entry record (the .data CActReg row; 4-byte PMF).
typedef i32 (CTileTriggerSwitch::*TileTriggerSwitchHandler)();
struct CTileTriggerSwitchActEntry {
    TileTriggerSwitchHandler m_fn;
};

#endif // GRUNTZ_CTILETRIGGERSWITCH_H
