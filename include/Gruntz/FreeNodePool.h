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
// The four fields are all evidence-named from RezSync::Init (which builds the pool),
// FreeNodePool::Push (which recycles into it) and ClearCoordPool (which frees it):
//   m_block      RezSync::Init stores RezAlloc(0x3a980) here; ClearCoordPool
//                ::operator delete()s it. 0x3a980 / sizeof(CoordNode) 0xc == 0x4e20.
//   m_freeHead   Push chains the raw node onto it (the intrusive free-list head).
//   m_count      RezSync::Init sets 0x4e20 - the element count of that block.
//   m_linkOffset Push subtracts it from the element pointer to reach the raw node
//                (RezSync::Init sets 4 - CoordNode's payload sits at node+4).
class FreeNodePool {
public:
    void Push(void* p); // 0x0311b0

    void* m_block;    // +0x00  owned backing block (RezAlloc'd; freed by ClearCoordPool)
    void* m_freeHead; // +0x04  free-list head
    i32 m_count;      // +0x08  element count of m_block
    i32 m_linkOffset; // +0x0c  payload offset inside a node (Push subtracts it)
};
SIZE_UNKNOWN(FreeNodePool);

#endif // GRUNTZ_FREENODEPOOL_H
