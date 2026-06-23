#include <rva.h>
// CCoveredPowerupLogic.cpp - CCoveredPowerupLogic ctor (matched).
//
// 18-byte logic-ctor archetype: chain to the CTileTriggerLogic base ctor
// (engine fn, not matched -> external no-body, reloc-masked rel32 call), then
// re-stamp the derived vftable into [this].

struct CCoveredPowerupLogicBase {
    CCoveredPowerupLogicBase();
};

class CCoveredPowerupLogic : public CCoveredPowerupLogicBase {
public:
    CCoveredPowerupLogic();
};

// Derived vftable, referenced as DIR32 data (RVA = VA - 0x400000).
DATA(0x005eaef4)
extern void* g_coveredPowerupLogicVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x00112240, 0x12)
CCoveredPowerupLogic::CCoveredPowerupLogic() {
    *(void**)this = &g_coveredPowerupLogicVtbl;
}
