#ifndef GRUNTZ_FREENODEPOOLINLINE_H
#define GRUNTZ_FREENODEPOOLINLINE_H

#include <Gruntz/FreeNodePool.h>

inline Coord* FreeNodePool::Pop() {
    CoordPoolNode* node = m_freeHead;
    Coord* result = NULL;
    if (node->m_next != NULL) {
        result = &node->m_coord;
        m_freeHead = node->m_next;
    }
    return result;
}

inline void PushFreeNode(FreeNodePool* pool, void* p) {
    CoordPoolNode* node = pool->NodeOf(p);
    node->m_next = pool->m_freeHead;
    pool->m_freeHead = node;
}

#endif // GRUNTZ_FREENODEPOOLINLINE_H
