#include <rva.h>
// CStaticHazard.cpp - engine-label stubs for CStaticHazard.

class CStaticHazard {
public:
    CStaticHazard(int);
    void LoadAttributes2();
    void LoadAttributes();
};

// @confidence: med
// @source: rtti-vptr
// @stub
RVA(0x000fb7a0, 0x2d4)
CStaticHazard::CStaticHazard(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000fc0b0, 0xb2)
void CStaticHazard::LoadAttributes2() {}

// @confidence: high
// @source: decomp-xref
// @stub
RVA(0x000fc1a0, 0x33b)
void CStaticHazard::LoadAttributes() {}
