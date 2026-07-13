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

#include <Gruntz/CoordNode.h> // Coord - the {x,y} the pool's nodes carry inline

// The pool's OWN node (the block is sizeof 0xc, m_linkOffset == 4): an intrusive free
// link at +0x00 and the {x,y} payload INLINE at +0x04..+0x0b. The allocator hands out
// &node->m_coord (node+4), which is why Push subtracts m_linkOffset to get back to the
// node - and why a recycled Coord* minus 4 is its node. Distinct from CoordNode (the
// MFC CObList cell: next@0, prev@4, data@8) that the grunts' occupied-coord lists use
// to POINT at one of these payloads. Was the .cpp-local `CCoordPair` view.
SIZE(CoordPoolNode, 0xc);
struct CoordPoolNode {
    CoordPoolNode* m_next; // +0x00  free-list link
    Coord m_coord;         // +0x04  the {x,y} payload handed to the caller
};

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
