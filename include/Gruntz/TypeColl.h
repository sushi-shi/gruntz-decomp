// TypeColl.h - the shared type-name collection. Lookup (0x40437c, __thiscall)
// resolves a type id to its CTypeNode (whose m_0 is the type-name string); Find
// (0x16da80, __thiscall ret 8) is the slow id lookup. Modeled NO-body so the calls
// reloc-mask. Placeholder names; only the signatures + offsets are load-bearing.
#ifndef GRUNTZ_GRUNTZ_CTYPECOLL_H
#define GRUNTZ_GRUNTZ_CTYPECOLL_H

#include <rva.h>

struct CTypeNode {
    char* m_0;
};
SIZE_UNKNOWN(CTypeNode);

struct CTypeColl {
    // Lookup @0x437c = zDArray::IndexToPtr, Find @0x16da80 = _zvec::GrowTo; cast at each call.
};
SIZE_UNKNOWN(CTypeColl);

#endif // GRUNTZ_GRUNTZ_CTYPECOLL_H
