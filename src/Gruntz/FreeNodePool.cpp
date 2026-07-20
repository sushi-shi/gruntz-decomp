#include <Gruntz/FreeNodePool.h>
#include <rva.h>

RVA(0x000311b0, 0x14)
void FreeNodePool::Push(void* p) {
    CoordPoolNode* node = NodeOf(p);
    node->m_next = m_freeHead;
    m_freeHead = node;
}
