#include <rva.h>
// CTileSecretTrigger.cpp - CTileSecretTrigger ctor (matched).
//
// 25-byte object-ctor archetype: forward the single ctor arg to the CTileTrigger
// base ctor (engine fn, not matched -> external no-body, reloc-masked rel32
// call), then re-stamp the derived vftable into [this].  The arg is a 4-byte
// by-value parameter (modeled int; only its width is load-bearing).

struct CTileSecretTriggerBase {
    CTileSecretTriggerBase(int a);
};

class CTileSecretTrigger : public CTileSecretTriggerBase {
public:
    CTileSecretTrigger(int a);
};

// Derived vftable, referenced as DIR32 data (RVA = VA - 0x400000).
DATA(0x005e7e64)
extern void* g_tileSecretTriggerVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x0010fa60, 0x19)
CTileSecretTrigger::CTileSecretTrigger(int a) : CTileSecretTriggerBase(a) {
    *(void**)this = &g_tileSecretTriggerVtbl;
}
