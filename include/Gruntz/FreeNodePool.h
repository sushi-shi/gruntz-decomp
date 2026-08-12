#ifndef GRUNTZ_FREENODEPOOL_H
#define GRUNTZ_FREENODEPOOL_H

#include <rva.h>

#include <Gruntz/CoordNode.h>
#include <Ints.h>

#include <stddef.h>

struct CoordPoolNode {
    CoordPoolNode* m_next;
    Coord m_coord;
};

class FreeNodePool {
public:
    FreeNodePool() : m_block(0), m_freeHead(0), m_count(0), m_linkOffset(0) {}

    ~FreeNodePool() {
        if (m_block != NULL) {
            delete[] m_block;
        }
        m_block = NULL;
        m_freeHead = NULL;
        m_count = 0;
        m_linkOffset = 0;
    }

    void Push(void* p);

    CoordPoolNode* NodeOf(void* payload) {

        // Language-forced container-of adjustment; a union spelling changes codegen.
        return reinterpret_cast<CoordPoolNode*>(static_cast<char*>(payload) - m_linkOffset);
    }

    CoordPoolNode* m_block;
    CoordPoolNode* m_freeHead;
    i32 m_count;
    i32 m_linkOffset;
};

extern FreeNodePool g_coordPool;

#endif // GRUNTZ_FREENODEPOOL_H
