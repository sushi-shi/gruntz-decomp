#include <rva.h>
// CTileMultiTriggerSwitchLogic.cpp - CTileMultiTriggerSwitchLogic ctor (matched).
//
// Derived tile-trigger "switch" logic class.  The ctor is the canonical 18-byte
// game-object archetype: chain to the CTileTriggerSwitchLogic base ctor (an
// engine fn we are NOT matching -> external no-body, reloc-masked rel32 call),
// then re-stamp the derived vftable into [this].  Field names are placeholders;
// only the offsets + the emitted code bytes are load-bearing.

// Base ctor: external, no body -> its `call` reloc-masks.  __thiscall (receiver
// in ecx); the base re-stamps its own vftable, the derived ctor overwrites it.
// (A per-file shell so this TU member of the All.cpp aggregate is self-contained;
// the reloc-masked call binds to the retail CTileTriggerSwitchLogic ctor by addr.)
struct CTileMultiTriggerSwitchLogicBase {
    CTileMultiTriggerSwitchLogicBase();
};

class CTileMultiTriggerSwitchLogic : public CTileMultiTriggerSwitchLogicBase {
public:
    CTileMultiTriggerSwitchLogic();
};

// Derived vftable, referenced as DIR32 data (RVA = VA - 0x400000).
DATA(0x005eaeb4)
extern void* g_multiTrigSwitchVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x00111f10, 0x12)
CTileMultiTriggerSwitchLogic::CTileMultiTriggerSwitchLogic() {
    *(void**)this = &g_multiTrigSwitchVtbl;
}
