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
    FreeNodePool() : m_block(NULL), m_freeHead(NULL), m_count(0), m_linkOffset(0) {}

    ~FreeNodePool() {
        if (m_block != NULL) {
            delete[] m_block;
        }
        m_block = NULL;
        m_freeHead = NULL;
        m_count = 0;
        m_linkOffset = 0;
    }

    bool Init(i32 count, i32 linkOffset) {
        m_block = new CoordPoolNode[count];
        if (m_block == NULL) {
            return false;
        }

        m_count = count;
        CoordPoolNode* node = m_block;
        u32 i = 0;
        do {
            node->m_next = node + 1;
            node = node->m_next;
            ++i;
        } while (i < static_cast<u32>(m_count) - 1);
        node->m_next = NULL;
        m_freeHead = m_block;
        m_linkOffset = linkOffset;
        return true;
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
