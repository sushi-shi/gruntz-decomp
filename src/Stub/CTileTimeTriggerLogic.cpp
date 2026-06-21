#include <rva.h>
// CTileTimeTriggerLogic.cpp - CTileTimeTriggerLogic ctor (matched).
//
// 18-byte logic-ctor archetype: chain to the CTileTriggerLogic base ctor
// (engine fn, not matched -> external no-body, reloc-masked rel32 call), then
// re-stamp the derived vftable into [this].

struct CTileTimeTriggerLogicBase {
    CTileTimeTriggerLogicBase();
};

class CTileTimeTriggerLogic : public CTileTimeTriggerLogicBase {
public:
    CTileTimeTriggerLogic();
};

// Derived vftable, referenced as DIR32 data (RVA = VA - 0x400000).
DATA(0x5eaf04)
extern void* g_timeTriggerLogicVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x112270, 0x12)
CTileTimeTriggerLogic::CTileTimeTriggerLogic() {
    *(void**)this = &g_timeTriggerLogicVtbl;
}
