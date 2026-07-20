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
    // Find (0x16da80, == _zvec::GrowTo) is the slow id lookup, declared here so the
    // registry probes call g_typeColl.Find(key, 0) with no (_zvec*) cast.
    i32 Find(i32 key, i32 z); // 0x16da80  (external, reloc-masked)
    // Lookup @0x437c = _zdvec::IndexToPtr; still cast at its call sites.
};
SIZE_UNKNOWN(CTypeColl);

#endif // GRUNTZ_GRUNTZ_CTYPECOLL_H
