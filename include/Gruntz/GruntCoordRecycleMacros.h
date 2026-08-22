#ifndef GRUNTZ_GRUNTCOORDRECYCLEMACROS_H
#define GRUNTZ_GRUNTCOORDRECYCLEMACROS_H

// Release every Coord queued on a grunt's path list back to g_coordPool and
// empty the list.  Three spellings, because retail commits each SITE to a
// different callee set; audited 2026-08-22 by folding each pair and reading a
// full build, and every fold that is not listed here was BYTE-NEUTRAL and its
// macro deleted:
//   * the walk - `m_coordList.GetHeadPosition()`/`GetNext` against
//     `CoordHead()`/`m_next` - is the same code at all 6 sites that used the
//     POSITION spelling (0 rows moved), because <MfcNoInline.h> cannot reach
//     CPtrList (afxcoll.inl is parsed inside <Mfc.h>, before the #undef);
//   * `if (CoordCount() != 0)` was a fourth axis (`*_IF_ANY`) that hid a real
//     conditional inside a macro name; hoisting it to all 21 call sites moved
//     0 rows, and several sites already wrote it by hand;
//   * the expanded push spelled longhand here against `PushFreeNode` from
//     <Gruntz/FreeNodePoolInline.h> is the same code at all 24 sites, but the
//     named form costs CBoomerang::AdvanceMotion 86.25 -> 84.58 - a fresh MAX
//     regression - because losing the `slot` local rotates cl 5.0's allocation
//     cursor for the next function in Projectile.cpp.  REMOVAL CONDITION for
//     the duplicated body: break AdvanceMotion's regalloc wall (diagnose says
//     REGALLOC/SCHEDULING, differs from +0x2), then respell this arm as
//     `PushFreeNode(&g_coordPool, current->m_coord)` and delete the note.
// Ledger: docs/patterns/comdat-home-adjudicates-inline-spelling.md.

// Calls FreeNodePool::Push (0x311b0).  Collapsing this into the expansion below
// costs -26.85 over 9 functions (CheckQueuedSpawnTile -6.61, RouteUnitTo -4.51,
// PathToNearestCandidate -3.94), so the sites really do call it.
#define RECYCLE_GRUNT_COORDS(grunt)                                                                \
    {                                                                                              \
        CoordNode* node = (grunt)->CoordHead();                                                    \
        while (node != NULL) {                                                                     \
            CoordNode* current = node;                                                             \
            node = node->m_next;                                                                   \
            if (current->m_coord != NULL) {                                                        \
                g_coordPool.Push(current->m_coord);                                                \
            }                                                                                      \
        }                                                                                          \
        (grunt)->m_coordList.RemoveAll();                                                          \
    }

// FreeNodePool::Push expanded at the site - retail's split is per SITE, not per
// TU (BattlezUnitStep.cpp calls 0x311b0 and expands it a few lines apart), which
// is why a visibility header cannot express it and two spellings can.
#define RECYCLE_GRUNT_COORDS_EXPANDED(grunt)                                                       \
    {                                                                                              \
        CoordNode* node = (grunt)->CoordHead();                                                    \
        while (node != NULL) {                                                                     \
            CoordNode* current = node;                                                             \
            node = node->m_next;                                                                   \
            if (current->m_coord != NULL) {                                                        \
                CoordPoolNode* slot = g_coordPool.NodeOf(current->m_coord);                        \
                slot->m_next = g_coordPool.m_freeHead;                                             \
                g_coordPool.m_freeHead = slot;                                                     \
            }                                                                                      \
        }                                                                                          \
        (grunt)->m_coordList.RemoveAll();                                                          \
    }

// Walks through CGruntCoordList::NextData (0x29a30) instead of inlining the
// step.  Collapsing this into the inline walk costs -16.37 over 4 functions
// (TrackAssignedEnemy -9.43, Step -4.10), and retail's call counts agree:
// StepRowUnits 6, TrackAssignedEnemy 2, Step 1, AdvanceToEnemyBase 1.
#define RECYCLE_GRUNT_COORDS_VIA_NEXTDATA(grunt)                                                   \
    {                                                                                              \
        POSITION position = (grunt)->m_coordList.GetHeadPosition();                                \
        while (position != NULL) {                                                                 \
            Coord* coord = static_cast<Coord*>((grunt)->CoordListOps()->NextData(position));       \
            if (coord != NULL) {                                                                   \
                g_coordPool.Push(coord);                                                           \
            }                                                                                      \
        }                                                                                          \
        (grunt)->m_coordList.RemoveAll();                                                          \
    }

#endif // GRUNTZ_GRUNTCOORDRECYCLEMACROS_H
