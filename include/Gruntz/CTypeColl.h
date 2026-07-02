// CTypeColl.h - the shared type-name collection. Lookup (0x40437c, __thiscall)
// resolves a type id to its CTypeNode (whose m_0 is the type-name string); Find
// (0x16da80, __thiscall ret 8) is the slow id lookup. Modeled NO-body so the calls
// reloc-mask. Placeholder names; only the signatures + offsets are load-bearing.
#ifndef GRUNTZ_GRUNTZ_CTYPECOLL_H
#define GRUNTZ_GRUNTZ_CTYPECOLL_H

#include <rva.h>

struct CTypeNode {
    char* m_0;
};

struct CTypeColl {
    CTypeNode* Lookup(i32 key); // 0x40437c (__thiscall)
    i32 Find(i32 key, i32 z);   // 0x16da80 (__thiscall ret 8)
};

#endif // GRUNTZ_GRUNTZ_CTYPECOLL_H
