#include <rva.h>
// CCheckpointTriggerSwitchLogic.cpp - CCheckpointTriggerSwitchLogic ctor.
//
// 18-byte logic-ctor archetype: chain to the CTileTriggerSwitchLogic base ctor
// (engine fn, not matched -> external no-body, reloc-masked rel32 call), then
// re-stamp the derived vftable into [this].

struct CCheckpointTriggerSwitchLogicBase {
    CCheckpointTriggerSwitchLogicBase();
};

class CCheckpointTriggerSwitchLogic : public CCheckpointTriggerSwitchLogicBase {
public:
    CCheckpointTriggerSwitchLogic();
};

// Derived vftable, referenced as DIR32 data (RVA = VA - 0x400000).
DATA(0x5eaf54)
extern void* g_checkpointTrigSwitchVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x1127f0, 0x12)
CCheckpointTriggerSwitchLogic::CCheckpointTriggerSwitchLogic() {
    *(void**)this = &g_checkpointTrigSwitchVtbl;
}
