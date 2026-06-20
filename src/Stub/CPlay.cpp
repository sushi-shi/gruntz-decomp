#include <rva.h>
// CPlay.cpp - engine-label stubs for CPlay (reloc-correlation).

class CPlay {
public:
    void RegionEnter();
    void RegionLeave();
};
// @confidence: high
// @source: reloc-correlation (4 callers)
// @stub
RVA(0x001b9a, 0x5)
void CPlay::RegionEnter() {}
// @confidence: high
// @source: reloc-correlation (4 callers)
// @stub
RVA(0x0019f1, 0x5)
void CPlay::RegionLeave() {}
