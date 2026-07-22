#ifndef GRUNTZ_FREENODEPOOL_H
#define GRUNTZ_FREENODEPOOL_H

#include <Ints.h>
#include <rva.h>

#include <Gruntz/CoordNode.h> // Coord - the {x,y} the pool's nodes carry inline

struct CoordPoolNode {
    CoordPoolNode* m_next; // +0x00  free-list link
    Coord m_coord;         // +0x04  the {x,y} payload handed to the caller
};
SIZE(0xc);

class FreeNodePool {
public:
    void Push(void* p); // 0x0311b0

    // The payload->node back-step every recycle site performs: the element pointer
    // minus m_linkOffset is its CoordPoolNode. m_linkOffset is a RUNTIME field (the
    // pool's design), so the arithmetic lives here once, not open-coded per site.
    CoordPoolNode* NodeOf(void* payload) {
        return reinterpret_cast<CoordPoolNode*>(reinterpret_cast<char*>(payload) - m_linkOffset);
    }

    CoordPoolNode* m_block; // +0x00  owned backing block (RezAlloc'd; freed by ClearCoordPool)
    CoordPoolNode* m_freeHead; // +0x04  free-list head
    i32 m_count;      // +0x08  element count of m_block
    i32 m_linkOffset; // +0x0c  payload offset inside a node (Push subtracts it)
};
SIZE_UNKNOWN();

extern FreeNodePool g_coordPool;

#endif // GRUNTZ_FREENODEPOOL_H
