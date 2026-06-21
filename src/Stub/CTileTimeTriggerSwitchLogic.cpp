#include <rva.h>
// CTileTimeTriggerSwitchLogic.cpp - CTileTimeTriggerSwitchLogic ctor (matched).
//
// 18-byte logic-ctor archetype: chain to the CTileTriggerSwitchLogic base ctor
// (engine fn, not matched -> external no-body, reloc-masked rel32 call), then
// re-stamp the derived vftable into [this].

struct CTileTimeTriggerSwitchLogicBase {
    CTileTimeTriggerSwitchLogicBase();
};

class CTileTimeTriggerSwitchLogic : public CTileTimeTriggerSwitchLogicBase {
public:
    CTileTimeTriggerSwitchLogic();
};

// Derived vftable, referenced as DIR32 data (RVA = VA - 0x400000).
DATA(0x5eaf3c)
extern void* g_timeTrigSwitchVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x1127c0, 0x12)
CTileTimeTriggerSwitchLogic::CTileTimeTriggerSwitchLogic() {
    *(void**)this = &g_timeTrigSwitchVtbl;
}
