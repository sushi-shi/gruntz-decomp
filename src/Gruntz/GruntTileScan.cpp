#include <Mfc.h>
#include <Gruntz/Grunt.h>
#include <rva.h>

#include <Ints.h>
#include <Win32.h> // RECT + IntersectRect
#include <Wap32/Rect.h> // canonical CRect: the 0x29ac0 direct-store ctor (ex the CScanRectInit Set34a4 carrier view)
#include <new> // placement CRect ctor
#include <Gruntz/ScanGrid.h>
#include <Gruntz/FreeNodePool.h>
#include <stdlib.h> // engine rand (0x11fee0)

#define SCAN_RECT_BOUNDS(grid)                                                                     \
    {                                                                                              \
        RECT ra;                                                                                   \
        RECT rb;                                                                                   \
        static_cast<RECT*>(new (&ra) CRect(0, 0, (grid)->m_width, (grid)->m_height));              \
        RECT* pb = static_cast<RECT*>(new (&rb) CRect(0, 0, (grid)->m_width, (grid)->m_height));   \
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
            GruntTilePos* c = reinterpret_cast<GruntTilePos*>(g->CoordTail()->m_coord);
            i32 col = c->m_x;
            i32 row = c->m_y;
            CScanGrid* grid = m_c;
            i32 flags;
            if (static_cast<u32>(col) < static_cast<u32>(grid->m_width)
                && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
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
                        g_coordPool.Push(static_cast<void*>((cur->m_coord)));
                    }
                }
                g->m_31c.RemoveAll();
                return 1;
            }
        }
        if (g->m_dwell > m_cc && g->CoordCount() == 0) {
            CScanGrid* grid = m_c;
            GruntTilePos tp;
            g->GetScreenPos(static_cast<GruntTilePos*>(&tp));
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
            gb.right = grid->m_width;
            gb.bottom = grid->m_height;
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
                        cell++;
                    }
                }
            }
            SCAN_RECT_BOUNDS(grid);
            if (m_f8 != 0) {
                CScanGoal* e = m_f4[rand() % m_f8];
                g->TileSwitch(e->m_0, e->m_4, 0, 0x983, 0, 0);
            }
            g->m_dwell = 0;
        }
    }
    return 1;
}
