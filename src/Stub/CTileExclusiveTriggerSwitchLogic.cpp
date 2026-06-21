#include <rva.h>
// CTileExclusiveTriggerSwitchLogic.cpp - CTileExclusiveTriggerSwitchLogic ctor.
//
// 18-byte logic-ctor archetype: chain to the CTileTriggerSwitchLogic base ctor
// (engine fn, not matched -> external no-body, reloc-masked rel32 call), then
// re-stamp the derived vftable into [this].  Per-file base shell so this member
// of the All.cpp aggregate is self-contained.

struct CTileExclusiveTriggerSwitchLogicBase {
    CTileExclusiveTriggerSwitchLogicBase();
};

class CTileExclusiveTriggerSwitchLogic : public CTileExclusiveTriggerSwitchLogicBase {
public:
    CTileExclusiveTriggerSwitchLogic();
};

// Derived vftable, referenced as DIR32 data (RVA = VA - 0x400000).
DATA(0x5eaecc)
extern void* g_exclTrigSwitchVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x112050, 0x12)
CTileExclusiveTriggerSwitchLogic::CTileExclusiveTriggerSwitchLogic() {
    *(void**)this = &g_exclTrigSwitchVtbl;
}
