// FreeNodePool.cpp - the typed intrusive free-list pool's Push (0x0311b0), the grunt
// coord-node recycler (global g_coordPool @0x645540, instance in Grunt.cpp). Re-homed
// from src/Stub/DiscoveredSmall.cpp: Push is the canonical FreeNodePool method
// (RTTI-named ?Push@FreeNodePool), a self-contained pointer-arithmetic leaf with no
// external calls. See <Gruntz/FreeNodePool.h> for the shared class shape.
#include <Gruntz/FreeNodePool.h>
#include <Rez/RezMgr.h> // RezFree - the engine allocator the pool's backing block uses
#include <rva.h>

// The shared coord-node pool object (0x645540); its +0 slot holds the RezAlloc'd
// backing block ClearCoordPool frees.
extern FreeNodePool g_coordPool;

// FreeNodePool::Push - subtract the node's link offset (m_c) from the freed element
// pointer and chain the raw node onto the free-list head (m_4). __thiscall, 1 stack arg.
RVA(0x000311b0, 0x14)
void FreeNodePool::Push(void* p) {
    char* node = (char*)p - m_c;
    *(void**)node = m_4;
    m_4 = node;
}

// ClearCoordPool (0x082ff0, __cdecl): tear down the global coord-node pool - free
// its backing block (m_0) if present, then zero all four pool words.
RVA(0x00082ff0, 0x2f)
void ClearCoordPool() {
    if (g_coordPool.m_0 != 0) {
        RezFree((void*)g_coordPool.m_0);
    }
    g_coordPool.m_0 = 0;
    g_coordPool.m_4 = 0;
    g_coordPool.m_8 = 0;
    g_coordPool.m_c = 0;
}
