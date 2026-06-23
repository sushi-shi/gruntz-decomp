#include <rva.h>
// CTileSecretTriggerLogic.cpp - CTileSecretTriggerLogic ctor (matched).
//
// 18-byte logic-ctor archetype: chain to the CTileTriggerLogic base ctor
// (engine fn, not matched -> external no-body, reloc-masked rel32 call), then
// re-stamp the derived vftable into [this].

struct CTileSecretTriggerLogicBase {
    CTileSecretTriggerLogicBase();
};

class CTileSecretTriggerLogic : public CTileSecretTriggerLogicBase {
public:
    CTileSecretTriggerLogic();
};

// Derived vftable, referenced as DIR32 data (RVA = VA - 0x400000).
DATA(0x005eaf14)
extern void* g_secretTriggerLogicVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x00112760, 0x12)
CTileSecretTriggerLogic::CTileSecretTriggerLogic() {
    *(void**)this = &g_secretTriggerLogicVtbl;
}
