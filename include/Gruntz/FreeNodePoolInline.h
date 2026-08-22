#ifndef GRUNTZ_FREENODEPOOLINLINE_H
#define GRUNTZ_FREENODEPOOLINLINE_H

#include <Gruntz/FreeNodePool.h>

// The EXPANDED spelling of FreeNodePool::Push (out of line at 0x311b0 in
// BattlezMapConfig.cpp, which wraps this helper).  Unlike the other *Inline.h
// devices this is not per-TU visibility: retail's split is per SITE, and both
// spellings occur inside single TUs - BattlezUnitStep.cpp uses
// RECYCLE_GRUNT_COORDS_IF_ANY (a call to 0x311b0) and
// RECYCLE_GRUNT_COORDS_INLINE_PUSH_IF_ANY (this expansion) a few lines apart.
// A visibility header cannot express per-site at all; two spellings can, so
// two entities are what the evidence supports.
// Collapse test 2026-08-22 (one in-class Push in FreeNodePool.h carrying the
// RVA pin, the wrapper deleted and all 21 helper calls rewritten to the
// member): 0x311b0 loses every emitter (verify unique-names FATAL, 100.00 ->
// 0.00) and ~20 grunt/battlez steps drop 1-10 points as the per-site
// distinction dies - TrackAssignedEnemy 93.45 -> 83.26, Step 87.22 -> 78.39,
// CheckQueuedSpawnTile 78.19 -> 71.58 (-175 total, -1 exact).
inline void PushFreeNode(FreeNodePool* pool, void* p) {
    CoordPoolNode* node = pool->NodeOf(p);
    node->m_next = pool->m_freeHead;
    pool->m_freeHead = node;
}

#endif // GRUNTZ_FREENODEPOOLINLINE_H
