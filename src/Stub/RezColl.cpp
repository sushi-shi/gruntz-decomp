#include <rva.h>
// RezColl.cpp - engine-label stubs for RezColl (reloc-correlation).

class RezColl {
public:
    struct RezNode * First();
};
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x184ae0, 0x24)
struct RezNode * RezColl::First() { return 0; }
