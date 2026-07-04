// GruntzMapMgr.cpp - the grunt-map manager container teardown (C:\Proj\Gruntz).
//
// ~CGruntzMapMgr (0x85d10) is a /GX dtor: cl re-stamps the derived vtable (the
// implicit stamp-first), then the body pushes every non-null CObArray element's
// node back onto the global g_freeList node-pool (the SAME idiom as
// CMapLogic::FreeNodes @0x85480), empties the array via SetSize(0,-1) and runs the
// grid Reset (0x9ec30); the implicit member dtor (~CObArray on m_arr) and the
// out-of-line base dtor (~CMapMgr @0x135c) close the frame.
#include <Gruntz/GruntzMapMgr.h>

// The intrusive free-list node allocator (head @0x645544, raw subtrahend @0x64554c);
// the node body pointer is recovered as (element - g_freeListNodeBias). Shared with
// MapLogic.cpp / Projectile.cpp (DATA-bound there; extern here).
extern void* g_freeList;       // ?g_freeList@@3PAXA       (VA 0x645544)
extern i32 g_freeListNodeBias; // ?g_freeListNodeBias@@3HA (VA 0x64554c)

RVA(0x00085d10, 0xa7)
CGruntzMapMgr::~CGruntzMapMgr() {
    for (i32 i = 0; i < m_arr.GetSize(); i++) {
        void* elem = m_arr.GetAt(i);
        if (elem != 0) {
            void** node = (void**)((char*)elem - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
        }
    }
    m_arr.SetSize(0, -1);
    Reset();
}
