// FreeNodePool.h - THE canonical shape of FreeNodePool, a typed intrusive free-list
// pool (the grunt coord-node recycler, global g_coordPool @0x645540). Push subtracts
// the node's link offset (m_c) from the element pointer and links the raw node onto
// the free-list head (m_4). Unifies DiscoveredSmall.cpp's full class with the
// method-only views in BattlezMapConfig.cpp (called Recycle) and BoundaryLowerThunks.cpp.
//
// Field names are placeholders; only offsets + code bytes are load-bearing.
#ifndef GRUNTZ_FREENODEPOOL_H
#define GRUNTZ_FREENODEPOOL_H

#include <Ints.h>
#include <rva.h>

// The real method (RTTI-named ?Push@FreeNodePool) is generic over the element type -
// void* so the CoordNode*/payload call sites convert implicitly (no per-site cast);
// the body reinterprets to char* for the m_c link-offset subtraction.
class FreeNodePool {
public:
    void Push(void* p); // 0x0311b0

    i32 m_0;   // +0x00  scratch/count
    void* m_4; // +0x04  free-list head
    i32 m_8;   // +0x08  scratch
    i32 m_c;   // +0x0c  node link offset
};
SIZE_UNKNOWN(FreeNodePool);

#endif // GRUNTZ_FREENODEPOOL_H
