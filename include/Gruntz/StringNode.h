// StringNode.h - the 4-byte walking-pointer slot the type-name teardown loops
// step over (viewed as an array via (CStringNode*)g_typeColl.m_alloc). Free() is the
// per-slot CString teardown (0x1b9b93, __thiscall on the slot address). One shape
// shared by the projectile / kitchen-slime / type-key act-registry teardowns.
#ifndef GRUNTZ_GRUNTZ_CSTRINGNODE_H
#define GRUNTZ_GRUNTZ_CSTRINGNODE_H

struct CStringNode {
    char* m_0; // +0x00  the CString handle the walking pointer steps over (body ptr)
    // Free @0x1b9b93 IS CString::~CString (per-slot teardown); cast at each call.
};

#endif // GRUNTZ_GRUNTZ_CSTRINGNODE_H
