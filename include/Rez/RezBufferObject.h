#pragma once
#include <Ints.h>
#include <rva.h>

// A 40-byte mesh record - the element type of CRezBufferObject's CObArray (proven by
// Serialize @0x17f130, a 0x28-stride SetSize-grow buffer). Its per-element ctor
// (0x17e300, reloc-masked, declared-only) makes cl emit the `if(p) p->T::T()`
// placement-new guard the retail per-element grow loop shows.
struct RezElem40 {
    RezElem40(); // 0x17e300 (declared-only; reloc-masked)
    char m_b[0x28];
};
SIZE(RezElem40, 0x28);
