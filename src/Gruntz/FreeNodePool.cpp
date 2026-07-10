// FreeNodePool.cpp - the typed intrusive free-list pool's Push (0x0311b0), the grunt
// coord-node recycler (global g_coordPool @0x645540, instance in Grunt.cpp). Re-homed
// from src/Stub/DiscoveredSmall.cpp: Push is the canonical FreeNodePool method
// (RTTI-named ?Push@FreeNodePool), a self-contained pointer-arithmetic leaf with no
// external calls. See <Gruntz/FreeNodePool.h> for the shared class shape.
#include <Gruntz/FreeNodePool.h>
#include <Rez/RezMgr.h> // RezFree - the engine allocator the pool's backing block uses
#include <Mfc.h>        // real MFC CString (the g_str6455xx init thunks below)
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

// ---------------------------------------------------------------------------
// 0x082da0..0x082f20 (RVA-homed from src/Stub/BoundaryThunks.cpp) - the four
// low-band CString dynamic-initializer (_$E) thunks of the debug-overlay/profiler
// text-sink global set (0x645514..0x645520), RVA-contiguous with ResetCoordPool /
// ClearCoordPool below. Each tail-constructs its empty CString in place
// (`mov ecx,&g; jmp CString::CString()` @0x1b9b93, the NAFXCW default ctor - reloc-
// masked, so only the OFFSETS + code bytes are load-bearing).
// ---------------------------------------------------------------------------
DATA(0x00245514)
extern CString g_str645514; // 0x645514
DATA(0x00245518)
extern CString g_str645518; // 0x645518
DATA(0x0024551c)
extern CString g_str64551c; // 0x64551c
DATA(0x00245520)
extern CString g_str645520; // 0x645520

RVA(0x00082da0, 0xa)
void InitStr645514() {
    g_str645514.CString::CString();
}
RVA(0x00082e20, 0xa)
void InitStr645518() {
    g_str645518.CString::CString();
}
RVA(0x00082ea0, 0xa)
void InitStr64551c() {
    g_str64551c.CString::CString();
}
RVA(0x00082f20, 0xa)
void InitStr645520() {
    g_str645520.CString::CString();
}

// ResetCoordPool (0x082fa0, __cdecl; RVA-homed from src/Stub/BoundaryLowerThunks.cpp):
// zero the four words of the global coord-node pool WITHOUT freeing its backing block
// (the non-freeing counterpart of ClearCoordPool). The pool's m_4 free-list head is
// also known engine-wide as ?g_freeList@@ (Projectile.cpp); modeled here through the
// canonical g_coordPool so the store sequence needs no offset cast.
RVA(0x00082fa0, 0x17)
void ResetCoordPool() {
    g_coordPool.m_0 = 0;
    g_coordPool.m_4 = 0;
    g_coordPool.m_8 = 0;
    g_coordPool.m_c = 0;
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
