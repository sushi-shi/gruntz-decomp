#include <rva.h>
// CCoveredPowerup.cpp - CCoveredPowerup ctor (matched).
//
// 25-byte object-ctor archetype: forward the single ctor arg to the CTileTrigger
// base ctor (engine fn, not matched -> external no-body, reloc-masked rel32
// call), then re-stamp the derived vftable into [this].  The arg is a 4-byte
// by-value parameter (modeled int; only its width is load-bearing).

struct CCoveredPowerupBase {
    CCoveredPowerupBase(int a);
};

class CCoveredPowerup : public CCoveredPowerupBase {
public:
    CCoveredPowerup(int a);
};

// Derived vftable, referenced as DIR32 data (RVA = VA - 0x400000).
DATA(0x5e7e0c)
extern void* g_coveredPowerupVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x10fac0, 0x19)
CCoveredPowerup::CCoveredPowerup(int a) : CCoveredPowerupBase(a) {
    *(void**)this = &g_coveredPowerupVtbl;
}
