#include <rva.h>
// CGiantRockLogic.cpp - CGiantRockLogic ctor (matched).
//
// 18-byte logic-ctor archetype: chain to the CTileTriggerLogic base ctor
// (engine fn, not matched -> external no-body, reloc-masked rel32 call), then
// re-stamp the derived vftable into [this].

struct CGiantRockLogicBase {
    CGiantRockLogicBase();
};

class CGiantRockLogic : public CGiantRockLogicBase {
public:
    CGiantRockLogic();
};

// Derived vftable, referenced as DIR32 data (RVA = VA - 0x400000).
DATA(0x5eaee4)
extern void* g_giantRockLogicVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x112210, 0x12)
CGiantRockLogic::CGiantRockLogic() {
    *(void**)this = &g_giantRockLogicVtbl;
}
