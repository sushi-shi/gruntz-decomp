#include "../rva.h"
// UnknownSirius.cpp - engine-label stubs for UnknownSirius.

class UnknownSirius {
public:
    void VirtualMethodUnknown20();
    ~UnknownSirius();
    void VirtualMethod_157720();
    void VirtualMethodUnknown24();
};

// @confidence: high
// @source: tomalla
// @stub
RVA(0x1576f0, 0x6)
void UnknownSirius::VirtualMethodUnknown20() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x157700, 0x1e)
UnknownSirius::~UnknownSirius() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x157720, 0x68)
void UnknownSirius::VirtualMethod_157720() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x1652c0, 0x92)
void UnknownSirius::VirtualMethodUnknown24() {}
