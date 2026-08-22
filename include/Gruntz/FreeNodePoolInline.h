#ifndef GRUNTZ_FREENODEPOOLINLINE_H
#define GRUNTZ_FREENODEPOOLINLINE_H

#include <Gruntz/FreeNodePool.h>

inline void PushFreeNode(FreeNodePool* pool, void* p) {
    CoordPoolNode* node = pool->NodeOf(p);
    node->m_next = pool->m_freeHead;
    pool->m_freeHead = node;
}

#endif // GRUNTZ_FREENODEPOOLINLINE_H
