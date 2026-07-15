// GruntzMapMgr.cpp - the grunt-map manager container teardown (C:\Proj\Gruntz).
//
// ~CGruntzMapMgr (0x85d10) is a /GX dtor: cl re-stamps the derived vtable (the
// implicit stamp-first), then the body pushes every non-null CObArray element's
// node back onto the global g_coordPool.m_freeHead node-pool (the SAME idiom as
// CMapLogic::FreeNodes @0x85480), empties the array via SetSize(0,-1) and runs the
// grid Reset (0x9ec30); the implicit member dtor (~CObArray on m_arr) and the
// out-of-line base dtor (~CMapMgr @0x135c) close the frame.
#include <Gruntz/GruntzMapMgr.h>

// The intrusive free-list node allocator (head @0x645544, raw subtrahend @0x64554c);
// the node body pointer is recovered as (element - g_coordPool.m_linkOffset). Shared with
// MapLogic.cpp / Projectile.cpp (DATA-bound there; extern here).
#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540
// The pool's INTERIOR FIELDS - m_freeHead (+0x04) and m_linkOffset (+0x0c) - used to be
// declared here as the standalone globals g_coordPool.m_freeHead / g_coordPool.m_linkOffset. They are not
// globals: they are fields of g_coordPool (DEFINED in src/Gruntz/GameText.cpp), which is
// why the free-list push/pop code reads exactly [pool+4] and [pool+0xc].

RVA(0x00085d10, 0xa7)
CGruntzMapMgr::~CGruntzMapMgr() {
    for (i32 i = 0; i < m_arr.GetSize(); i++) {
        void* elem = m_arr.GetAt(i);
        if (elem != 0) {
            void** node = (void**)((char*)elem - g_coordPool.m_linkOffset);
            *node = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_arr.SetSize(0, -1);
    CMapMgr::Reset(); // @0x9ec30 (base slot-0 grid cleanup, direct call)
}
