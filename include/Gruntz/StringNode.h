#ifndef GRUNTZ_GRUNTZ_CSTRINGNODE_H
#define GRUNTZ_GRUNTZ_CSTRINGNODE_H

struct CStringNode {
    char* m_0; // +0x00  the CString handle the walking pointer steps over (body ptr)
    // Free @0x1b9b93 IS CString::~CString (per-slot teardown); cast at each call.
};

#endif // GRUNTZ_GRUNTZ_CSTRINGNODE_H
