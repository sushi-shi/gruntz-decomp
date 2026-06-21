#include <rva.h>
// CTileSecretTriggerSwitchLogic.cpp - CTileSecretTriggerSwitchLogic ctor.
//
// 18-byte logic-ctor archetype: chain to the CTileTriggerSwitchLogic base ctor
// (engine fn, not matched -> external no-body, reloc-masked rel32 call), then
// re-stamp the derived vftable into [this].

struct CTileSecretTriggerSwitchLogicBase {
    CTileSecretTriggerSwitchLogicBase();
};

class CTileSecretTriggerSwitchLogic : public CTileSecretTriggerSwitchLogicBase {
public:
    CTileSecretTriggerSwitchLogic();
};

// Derived vftable, referenced as DIR32 data (RVA = VA - 0x400000).
DATA(0x5eaf24)
extern void* g_secretTrigSwitchVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x112790, 0x12)
CTileSecretTriggerSwitchLogic::CTileSecretTriggerSwitchLogic() {
    *(void**)this = &g_secretTrigSwitchVtbl;
}
