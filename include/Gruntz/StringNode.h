#ifndef GRUNTZ_GRUNTZ_CSTRINGNODE_H
#define GRUNTZ_GRUNTZ_CSTRINGNODE_H
#include <rva.h>

struct CStringNode {
    char* m_0; // +0x00  the CString handle the walking pointer steps over (body ptr)
    // Free @0x1b9b93 IS CString::~CString (per-slot teardown); cast at each call.
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_CSTRINGNODE_H
