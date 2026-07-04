// CStringNode.h - the 4-byte walking-pointer slot the type-name teardown loops
// step over (viewed as an array via (CStringNode*)g_typeNodes). Free() is the
// per-slot CString teardown (0x1b9b93, __thiscall on the slot address). One shape
// shared by the projectile / kitchen-slime / type-key act-registry teardowns.
#ifndef GRUNTZ_GRUNTZ_CSTRINGNODE_H
#define GRUNTZ_GRUNTZ_CSTRINGNODE_H

struct CStringNode {
    void* m_0;   // +0x00  the slot the walking pointer steps over (4-byte stride)
    void Free(); // 0x1b9b93  CString teardown (__thiscall on the slot address)
};

#endif // GRUNTZ_GRUNTZ_CSTRINGNODE_H
