#include <rva.h>
// CGiantRock.cpp - CGiantRock ctor (matched).
//
// 25-byte object-ctor archetype: forward the single ctor arg to the CTileTrigger
// base ctor (engine fn, not matched -> external no-body, reloc-masked rel32
// call), then re-stamp the derived vftable into [this].  The arg is a 4-byte
// by-value parameter (modeled int; only its width is load-bearing).

struct CGiantRockBase {
    CGiantRockBase(int a);
};

class CGiantRock : public CGiantRockBase {
public:
    CGiantRock(int a);
};

// Derived vftable, referenced as DIR32 data (RVA = VA - 0x400000).
DATA(0x5e7d5c)
extern void* g_giantRockVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x10fa90, 0x19)
CGiantRock::CGiantRock(int a) : CGiantRockBase(a) {
    *(void**)this = &g_giantRockVtbl;
}
