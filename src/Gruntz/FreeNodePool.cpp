// FreeNodePool.cpp - the typed intrusive free-list pool's Push (0x0311b0), the grunt
// coord-node recycler (global g_coordPool @0x645540, instance in Grunt.cpp). Re-homed
// from src/Stub/DiscoveredSmall.cpp: Push is the canonical FreeNodePool method
// (RTTI-named ?Push@FreeNodePool), a self-contained pointer-arithmetic leaf with no
// external calls. See <Gruntz/FreeNodePool.h> for the shared class shape.
#include <Gruntz/FreeNodePool.h>
#include <rva.h>

// FreeNodePool::Push - subtract the node's link offset (m_c) from the freed element
// pointer and chain the raw node onto the free-list head (m_4). __thiscall, 1 stack arg.
RVA(0x000311b0, 0x14)
void FreeNodePool::Push(void* p) {
    char* node = (char*)p - m_c;
    *(void**)node = m_4;
    m_4 = node;
}
