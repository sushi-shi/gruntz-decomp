#include <Mfc.h>
#include <Gruntz/Grunt.h>
// GruntTileScan.cpp - the per-tick grunt region-scan (0x032ce0, __thiscall ret 4).
// `this` (edi) is a grunt manager owning the move grid (m_c, a CBrickz-shape grid)
// plus a goal-point table (m_f4/m_f8); the argument (ebp) is the CGrunt being
// serviced. When the grunt's "stuck" counter (m_3f0) crosses 0x64 the method:
//   (A) if a pending coord is latched (m_328) and the grunt's current tile is a
//       0x4000/type-0x99 cell, drains the pending-coord list (g_coordPool recycle
//       + m_31c RemoveAll) and returns; else
//   (B) if the idle counter (m_2ec) exceeds the manager threshold (m_cc) and no
//       coord is pending, scans a 10x10 tile box (clamped to the grid) around the
//       grunt, firing the trigger (DoTrigger1fb9, msg 0xd87) on up to five
//       flagged cells, recomputes the grid dirty rect (m_60/m_70/m_74), picks a
//       random goal from the table and re-targets the grunt (Trigger1640, msg
//       0x983), then clears the idle counter.
// Shares the CGrunt coord-pool / grid family with GruntMoveStep.cpp + Brickz.cpp.
#include <rva.h>

#include <Ints.h>
#include <Win32.h> // RECT + IntersectRect
#include <Gruntz/ScanRectInit.h>
#include <Gruntz/ScanGrid.h>
#include <Gruntz/FreeNodePool.h>
#include <stdlib.h> // engine rand (0x11fee0)

// --- offset-faithful views (offsets + called methods load-bearing; reloc-masked) ---
// GruntTilePos/GruntCoordNode (grunt->m_324), GruntCoordNode (grunt->m_320 pending-coord
// node) and CGruntSub10 (grunt->m_10) are the shared def in <Gruntz/ScanGrid.h>.
// CScanCell (the grid's 0x1c-B cell) is the shared def in <Gruntz/ScanGrid.h>.
struct CScanGoal { // this->m_f4[] element
    i32 m_0, m_4;
};
struct CScanMgr {                                                      // this (edi)
    i32 DoTrigger1fb9(CGrunt* g, i32 x, i32 y, i32 msg, i32 c, i32 d); // 0x1fb9
    char _00[0xc];
    CScanGrid* m_c; // +0x0c
    char _10[0xcc - 0x10];
    u32 m_cc; // +0xcc idle threshold
    char _d0[0xf4 - 0xd0];
    CScanGoal** m_f4; // +0xf4 goal table
    i32 m_f8;         // +0xf8 goal count
    i32 ScanRegion32ce0(CGrunt* g);
};


// Recompute the grid dirty rect (m_60) as the {0,0,w,h} box intersected with a
// copy of itself; m_70/m_74 = the resulting size. Inlined at each exit.
#define SCAN_RECT_BOUNDS(grid)                                                                     \
    {                                                                                              \
        RECT ra;                                                                                   \
        RECT rb;                                                                                   \
        ((CScanRectInit*)&ra)->Set34a4(0, 0, (grid)->m_c, (grid)->m_10);                           \
        RECT* pb = ((CScanRectInit*)&rb)->Set34a4(0, 0, (grid)->m_c, (grid)->m_10);                \
        ra.left = pb->left;                                                                        \
        ra.top = pb->top;                                                                          \
        ra.right = pb->right;                                                                      \
        ra.bottom = pb->bottom;                                                                    \
        if (!IntersectRect(&(grid)->m_60, &ra, &rb)) {                                             \
            (grid)->m_60 = ra;                                                                     \
        }                                                                                          \
        (grid)->m_70 = (grid)->m_60.right - (grid)->m_60.left;                                     \
        (grid)->m_74 = (grid)->m_60.bottom - (grid)->m_60.top;                                     \
    }

// @early-stop  ~72% fuzzy (was 0.53% stub)
// large tile-scan reconstruction (final-sweep candidate): the m_3f0 gate, the
// pending-coord tile check + g_coordPool recycle drain, the 10x10 box build /
// IntersectRect clamp, the capped-at-five double tile loop firing DoTrigger1fb9,
// the triple grid-dirty-rect recompute and the random-goal retarget are byte-
// shaped. Residual walls:
//  (1) this/g frame-register swap: retail keeps `this` in edi (re-reads
//      [edi+0xc] fresh inside the row loop, weighting `this` high) and `g` in
//      ebp; caching `grid` here (which scores HIGHER overall, 72 vs 65) drops
//      `this`'s loop weight so MSVC assigns g->edi, this->ebp - every this/g
//      member ref then mismatches the reg. Re-reading m_c inline flips the reg
//      but adds more divergence than it fixes; no source spelling wins both.
//  (2) retail emits GetTilePos36c0 three times into three stack points feeding
//      the box (modeled with one call; the optimizer won't re-emit the dead
//      redundant calls), and frame is 0x68 vs 0x60 from the extra slots.
//  (3) the two hit-exit rect recomputes use one Set34a4 vs the normal exit's
//      two, and the incoming-arg slot is recycled as the hit counter - slot
//      schedule diverges. Logic/offsets correct; re-attack leaf-first in sweep.
RVA(0x00032ce0, 0x448)
i32 CScanMgr::ScanRegion32ce0(CGrunt* g) {
    if (g->m_stamina >= 0x64) {
        if (g->CoordCount() != 0) {
            GruntTilePos* c = (GruntTilePos*)g->CoordTail()->m_coord;
            i32 col = c->m_x;
            i32 row = c->m_y;
            CScanGrid* grid = m_c;
            i32 flags;
            if ((u32)col < (u32)grid->m_c && (u32)row < (u32)grid->m_10) {
                flags = grid->m_8[row][col].m_flags;
            } else {
                flags = 1;
            }
            if ((flags & 0x4000) && grid->m_8[row][col].m_type == 0x99) {
                GruntCoordNode* n = g->CoordHead();
                while (n != 0) {
                    GruntCoordNode* cur = n;
                    n = n->m_next;
                    if (cur->m_coord != 0) {
                        g_coordPool.Push((void*)(cur->m_coord));
                    }
                }
                g->m_31c.RemoveAll();
                return 1;
            }
        }
        if (g->m_dwell > m_cc && g->CoordCount() == 0) {
            CScanGrid* grid = m_c;
            GruntTilePos tp;
            g->GetScreenPos((GruntTilePos*)&tp);
            i32 cx = tp.m_x >> 5;
            i32 cy = tp.m_y >> 5;
            RECT box;
            box.left = cx - 5;
            box.top = cy - 5;
            box.right = cx + 5;
            box.bottom = cy + 5;
            RECT gb;
            gb.left = 0;
            gb.top = 0;
            gb.right = grid->m_c;
            gb.bottom = grid->m_10;
            RECT isect;
            if (IntersectRect(&isect, &box, &gb)) {
                i32 hits = 0;
                for (i32 row = isect.top; row < isect.bottom; row++) {
                    if (hits > 4) {
                        break;
                    }
                    CScanCell* cell = &grid->m_8[row][isect.left];
                    for (i32 col = isect.left; col < isect.right; col++) {
                        if (hits < 5) {
                            i32 flags = cell->m_flags;
                            if (flags & 0x8000) {
                                if (DoTrigger1fb9(g, col, row, 0xd87, 0, 0)) {
                                    SCAN_RECT_BOUNDS(grid);
                                    return 1;
                                }
                                hits++;
                            } else if ((flags & 0x4000) && cell->m_type != 0x99) {
                                if (DoTrigger1fb9(g, col, row, 0xd87, 0, 0)) {
                                    SCAN_RECT_BOUNDS(grid);
                                    return 1;
                                }
                                hits++;
                            }
                        }
                        cell = (CScanCell*)((char*)cell + 0x1c);
                    }
                }
            }
            SCAN_RECT_BOUNDS(grid);
            if (m_f8 != 0) {
                CScanGoal* e = m_f4[rand() % m_f8];
                CGrunt_TileSwitch(e->m_0, e->m_4, 0, 0x983, 0, 0);
            }
            g->m_dwell = 0;
        }
    }
    return 1;
}

SIZE_UNKNOWN(CScanGoal);
SIZE_UNKNOWN(CScanMgr);
