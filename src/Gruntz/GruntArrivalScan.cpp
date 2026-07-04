// GruntArrivalScan.cpp - three per-grunt-type arrival/scan update steps that sit just
// after CGrunt_0ef6b0 in retail (0xecc90 + 0xf0e20 + 0xf36a0). All __thiscall, no
// explicit args, return 1. Direct siblings of GruntUpdateStep.cpp's UpdateArrival
// (0xf0130) / SeekTarget (0xf71c0) and GruntArrivalStep.cpp's StepArrivalDefenseAlt
// (0xf1c70): the head gates on the grunt's type name (g_typeColl.Lookup(...)->m_0) -
// each returns 1 immediately when the grunt IS an "I" (0x60cca0) - and the body drives
// the tile-to-tile move: resolve the grunt under the HUD center (FindGrunt), gate on
// the powered-up / arrival state words, recompute the board dirty rect (m_60/m_70/m_74,
// the GruntTileScan.cpp idiom), scan a box of grid cells for the nearest live target
// and Scatter / ProbeMove to it, and on commit fire the grunt cue (0x39f4). All engine
// helpers + the manager/grid globals are external (reloc-masked); the CGrunt field bag
// is addressed by raw offset exactly as retail does. Data globals are named to the
// current symbol_names.csv bindings: g_gameReg (canonical CGameRegistry* @0x24556c),
// g_typeColl (?g_typeColl@@3UCTypeColl@@A), g_dropList (?g_dropList@@3UCStepList2@@A).
#include <Ints.h>
#include <string.h> // inline strcmp of the grunt type name

#include <Win32.h> // RECT + IntersectRect / PtInRect
#include <rva.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/StepList2.h>
#include <Gruntz/ScanRectInit.h>
#include <Gruntz/ScanGrid.h>
#include <Gruntz/TypeColl.h>
#include <Gruntz/StepList.h>

#pragma intrinsic(strcmp)

#define F(base, o) (*(i32*)((char*)(base) + (o)))
#define P(base, o) (*(char**)((char*)(base) + (o)))
#define IABS(v) ((v) = ((v) ^ ((v) >> 31)) - ((v) >> 31)) // MSVC cdq/xor/sub abs

struct CGruntStep;

// Type-name collection (g_typeColl @0x6bf650): Lookup(key)->node, node->m_0 = name.
extern CTypeColl g_typeColl; // ?g_typeColl@@3UCTypeColl@@A (bound in GruntUpdateStep.cpp)

// A grunt in the tileMgr's live-grunt list (node->m_8): m_54/m_58 = tile col/row,
// m_5c = a busy flag.
struct CScanGruntNode;

// Per-grunt move/path sub-manager (CGrunt+0x260): FindGrunt(this)->grunt-under-HUD;
// also the grid the Scatter helper (0x14bf) is a __thiscall on; m_4 = live-grunt list.
struct CGruntTileMgr {
    CGruntStep* FindGrunt(CGruntStep* g);    // 0x40253b
    i32 Scatter(i32 a, i32 b, i32 c, i32 d); // 0x4014bf
    char _00[4];
    CScanGruntNode* m_4; // +0x04 live-grunt list head
};
struct CScanGruntNode {
    CScanGruntNode* m_next; // +0x00
    char _04[8 - 4];
    char* m_8; // +0x08 -> grunt (m_54 col, m_58 row, m_5c busy)
};

// __cdecl board/cell predicates (reloc-masked). BoardTest: point-in-board-rect;
// CellTargetable: the active-move cell gate.
extern "C" i32 BoardTest(char* board, i32 x, i32 y); // 0x401127
extern "C" i32 CellTargetable(i32 col, i32 row);     // 0x40107d

// The owned pending-coord collection at CGrunt+0x31c that gets RemoveAll'd on commit.

extern CStepList2 g_dropList; // ?g_dropList@@3UCStepList2@@A (bound in GruntUpdateStep.cpp)

// The board grid (g_gameReg->m_tileGrid, CScanGrid-shape) and its 0x1c-B CScanCell,
// plus the shared CScanCoord/CScanNode324 (CGrunt+0x324) and CScanListNode
// (CGrunt+0x320) path-node types, are the shared def in <Gruntz/ScanGrid.h>.

// The +0x60 on-screen cue receiver (CGruntCueSink, forward-declared by
// CGameRegistry.h): its 6-arg grunt entrance cue is at 0x4039f4 (__thiscall).
// Completed here with just that method (data-less class, layout-neutral).
class CGruntCueSink {
public:
    void PlayCue(CGruntStep* g, i32 code, i32 a, i32 b, i32 c, i32 d); // 0x4039f4
};

// The game-manager singleton is the canonical CGameRegistry (*0x64556c): the board
// base via m_world->m_24->+0x5c (CSpriteFactoryHolder -> CGameViewport), the +0x60
// cue receiver (m_cueSink), and the +0x70 tile grid downcast to the richer CScanGrid
// view (dirty rect + row table).
DATA(0x0024556c)
extern CGameRegistry* g_gameReg; // 0x64556c

struct CGruntStep {
    i32 ArrivalScanA(); // 0xecc90
    i32 ArrivalScanB(); // 0xf0e20
    i32 ArrivalScanC(); // 0xf36a0

    // reloc-masked CGrunt __thiscall helpers (called on this and on other grunts):
    i32 TileProbe(i32 x, i32 y);                             // 0x403c4c
    i32 RunGate(i32 a);                                      // 0x403d5a
    void ResetEntrance(i32 a, i32 b, i32 c);                 // 0x40136b
    i32 OwnsTile(i32 a, i32 b);                              // 0x401014
    void CommitMove(i32 a, i32 b, i32 c, i32 d);             // 0x40302b
    i32 ProbeMove(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // 0x401640
    void StampMove(i32 a, i32 b);                            // 0x401401
    void ReadCenter(void* out);                              // 0x4036c0
    void SelectIcon(i32 a, i32 b, i32 c, i32 d);             // 0x403bd9
};

// Recompute the board dirty rect (m_60) as {0,0,w,h} intersected with a copy of
// itself; m_70/m_74 = the resulting size. (GruntTileScan.cpp SCAN_RECT_BOUNDS.)
#define GRID_RECT_BOUNDS(grid)                                                                     \
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

// Same, but built with inline rect field stores (no Set34a4) - the box-scan exit form.
#define GRID_RECT_INLINE(grid)                                                                     \
    {                                                                                              \
        RECT ra;                                                                                   \
        ra.left = 0;                                                                               \
        ra.top = 0;                                                                                \
        ra.right = (grid)->m_c;                                                                    \
        ra.bottom = (grid)->m_10;                                                                  \
        RECT rb;                                                                                   \
        rb.left = 0;                                                                               \
        rb.top = 0;                                                                                \
        rb.right = (grid)->m_c;                                                                    \
        rb.bottom = (grid)->m_10;                                                                  \
        if (!IntersectRect(&(grid)->m_60, &ra, &rb)) {                                             \
            (grid)->m_60 = ra;                                                                     \
        }                                                                                          \
        (grid)->m_70 = (grid)->m_60.right - (grid)->m_60.left;                                     \
        (grid)->m_74 = (grid)->m_60.bottom - (grid)->m_60.top;                                     \
    }

// Drain the pending-coord list (recycle each node's link, then RemoveAll).
#define DRAIN_COORDS()                                                                             \
    if (F(this, 0x328) != 0) {                                                                     \
        CScanListNode* n = (CScanListNode*)P(this, 0x320);                                         \
        while (n != 0) {                                                                           \
            CScanListNode* cur = n;                                                                \
            n = cur->m_next;                                                                       \
            if (cur->m_8 != 0) {                                                                   \
                g_dropList.Drop(cur->m_8);                                                         \
            }                                                                                      \
        }                                                                                          \
        ((CStepList*)((char*)this + 0x31c))->RemoveAll();                                          \
    }

// ===========================================================================
// @early-stop
// Deep per-tick arrival/scan step (non-"I" grunt types). Logic reconstructed fully;
// same deep-regalloc + slot-recycle + cold-block wall family as GruntUpdateStep.cpp's
// UpdateArrival/SeekTarget and GruntArrivalStep.cpp's StepArrivalDefenseAlt.
// Final-sweep candidate.
RVA(0x000ecc90, 0x86a)
i32 CGruntStep::ArrivalScanA() {
    if (strcmp(g_typeColl.Lookup(F(P(this, 0x14), 0x1c))->m_0, "I") == 0) {
        return 1;
    }
    F(this, 0x300) = F(this, 0x17c);
    F(this, 0x304) = F(this, 0x180);
    CScanGrid* grid = (CScanGrid*)g_gameReg->m_tileGrid;
    GRID_RECT_BOUNDS(grid);

    i32 c1[4];
    ReadCenter(c1);
    i32 cx = c1[0] >> 5;
    i32 c2[4];
    ReadCenter(c2);
    i32 cy = c2[1] >> 5;

    CGruntStep* g = ((CGruntTileMgr*)P(this, 0x260))->FindGrunt(this);
    i32 atTarget = 0;
    if (g != 0) {
        i32 x = F(P(g, 0x10), 0x5c);
        if (x == F(g, 0x17c) && F(P(g, 0x10), 0x60) == F(g, 0x180)
            && this->TileProbe(x, F(P(g, 0x10), 0x60)) != 0) {
            atTarget = 1;
        }
    }

    if (F(this, 0x220) != 0) {
        if (F(this, 0x21c) != 0) {
            F(this, 0x21c) = 0;
            return 1;
        }
        if (F(this, 0x218) != 0) {
            return 1;
        }
        if (F(this, 0x3f0) >= 100) {
            if (RunGate(1) != 0) {
                return 1;
            }
            if (atTarget && g == 0) {
                return 1;
            }
            if (F(this, 0x220) == 0) {
                return 1;
            }
        } else {
            if (atTarget) {
                return 1;
            }
            if (F(this, 0x220) == 0) {
                return 1;
            }
        }
        if (F(this, 0x21c) != 0) {
            return 1;
        }
        F(this, 0x1e4) = 0;
        F(this, 0x218) = 0;
        F(this, 0x21c) = 0;
        F(this, 0x220) = 0;
        ResetEntrance(1, 0, 0);
        return 1;
    }

    if (g == 0) {
        F(this, 0x390) = 0;
        goto L_ed006;
    }
    if (F(this, 0x21c) != 0) {
        return 1;
    }
    if (F(this, 0x218) == 0 && F(this, 0x3f0) >= 100) {
        if (atTarget) {
            CommitMove(F(g, 0x1ec), F(g, 0x1f0), F(g, 0x17c), F(g, 0x180));
            DRAIN_COORDS();
            return 1;
        }
    } else {
        if (atTarget) {
            DRAIN_COORDS();
            return 1;
        }
    }

L_ed006:
    if (g == 0 || (u32)F(this, 0x2ec) <= 0x1f4 || this->OwnsTile(F(g, 0x1ec), F(g, 0x1f0)) == 0) {
        F(this, 0x390) = 0;
        goto L_ed153;
    }
    if (F(this, 0x220) != 0) {
        goto L_ed153;
    }
    if (F(this, 0x3f0) >= 100 && F(P(g, 0x10), 0x5c) == F(g, 0x17c)
        && F(P(g, 0x10), 0x60) == F(g, 0x180)
        && this->TileProbe(F(P(g, 0x10), 0x5c), F(P(g, 0x10), 0x60)) != 0) {
        CommitMove(F(g, 0x1ec), F(g, 0x1f0), F(g, 0x17c), F(g, 0x180));
        F(this, 0x2ec) = 0;
        return 1;
    }
    if (F(this, 0x220) != 0) {
        goto L_ed153;
    }
    if (this->ProbeMove(F(P(g, 0x10), 0x5c) >> 5, F(P(g, 0x10), 0x60) >> 5, 0, F(this, 0x248), 1, 0)
        == 0) {
        goto L_ed153;
    }
    if (F(this, 0x390) != 0) {
        char* board = P(g_gameReg->m_world->m_24, 0x5c) + 0x40;
        i32 x = F(P(this, 0x10), 0x5c);
        i32 y = F(P(this, 0x10), 0x60);
        if (x < F(board, 8) && F(board, 0) <= x && y < F(board, 0xc) && F(board, 4) <= y) {
            g_gameReg->m_cueSink->PlayCue(this, 0x366, -1, 0, -1, -1);
        }
        F(this, 0x390) = 0;
    }
    F(this, 0x2ec) = 0;

L_ed153:
    if (F(this, 0x328) != 0) {
        CScanCoord* coord = ((CScanNode324*)P(this, 0x320))->m_8;
        i32 col = coord->x;
        i32 row = coord->y;
        CScanCell* cell = &grid->m_8[row][col];
        if ((cell->m_flags & 0x8000) != 0 || cell->m_type == 0x97 || cell->m_type == 0x98) {
            ((CGruntTileMgr*)P(this, 0x260))
                ->Scatter(F(this, 0x1ec), F(this, 0x1f0), (col << 5) + 0x10, (row << 5) + 0x10);
            StampMove(1, 1);
            F(this, 0x2ec) = 0;
        }
        return 1;
    }
    if ((u32)F(this, 0x2ec) <= 0x3e8) {
        return 1;
    }

    i32 r = F(this, 0x2dc);
    RECT box;
    box.left = cx - r;
    box.right = cx + r;
    box.top = cy - r;
    box.bottom = cy + r;
    RECT gb;
    gb.left = 0;
    gb.top = 0;
    gb.right = grid->m_c;
    gb.bottom = grid->m_10;
    RECT isect;
    if (!IntersectRect(&isect, &box, &gb)) {
        isect = box;
    }

    i32 best = 0x7fffffff;
    i32 bestCol = -1;
    i32 bestRow = -1;
    {
        RECT lb;
        lb.left = isect.left;
        lb.top = isect.top;
        lb.right = isect.right + 1;
        lb.bottom = isect.bottom + 1;
        RECT gb2;
        gb2.left = 0;
        gb2.top = 0;
        gb2.right = grid->m_c;
        gb2.bottom = grid->m_10;
        if (!IntersectRect(&grid->m_60, &lb, &gb2)) {
            grid->m_60 = lb;
        }
        grid->m_70 = grid->m_60.right - grid->m_60.left;
        grid->m_74 = grid->m_60.bottom - grid->m_60.top;
    }
    for (i32 row = isect.top; row < isect.bottom; row++) {
        CScanCell* cell = &grid->m_8[row][isect.left];
        for (i32 col = isect.left; col < isect.right; col++) {
            if ((cell->m_flags & 0x8000) != 0 || cell->m_type == 0x97 || cell->m_type == 0x98) {
                i32 dr = row - cy;
                IABS(dr);
                i32 dc = col - cx;
                IABS(dc);
                i32 dist = dr + dc;
                if (dist < best) {
                    best = dist;
                    bestCol = col;
                    bestRow = row;
                }
            }
            cell = (CScanCell*)((char*)cell + 0x1c);
        }
    }
    if (best != 0x7fffffff) {
        i32 dc = bestCol - cx;
        IABS(dc);
        i32 dr = bestRow - cy;
        IABS(dr);
        if (dc <= 1 && dr <= 1) {
            ((CGruntTileMgr*)P(this, 0x260))
                ->Scatter(
                    F(this, 0x1ec),
                    F(this, 0x1f0),
                    (bestCol << 5) + 0x10,
                    (bestRow << 5) + 0x10
                );
            StampMove(1, 1);
        } else {
            this->ProbeMove(bestCol, bestRow, 0, F(this, 0x248), 1, 0);
        }
    }
    GRID_RECT_INLINE(grid);
    F(this, 0x2ec) = 0;
    return 1;
}

// ===========================================================================
// @early-stop
// Sibling of ArrivalScanA with a __cdecl board test (BoardTest 0x1127) instead of the
// inlined point-in-rect, and a live-grunt-LIST scan (tileMgr->m_4, PtInRect membership)
// instead of the grid-cell box scan. Logic reconstructed fully; same deep-regalloc +
// slot-recycle wall family. Final-sweep candidate.
RVA(0x000f0e20, 0x928)
i32 CGruntStep::ArrivalScanB() {
    if (strcmp(g_typeColl.Lookup(F(P(this, 0x14), 0x1c))->m_0, "I") == 0) {
        return 1;
    }
    F(this, 0x300) = F(this, 0x17c);
    F(this, 0x304) = F(this, 0x180);
    CScanGrid* grid = (CScanGrid*)g_gameReg->m_tileGrid;
    GRID_RECT_BOUNDS(grid);

    i32 c1[4];
    ReadCenter(c1);
    i32 cx = c1[0] >> 5;
    i32 c2[4];
    ReadCenter(c2);
    i32 cy = c2[1] >> 5;

    CGruntStep* g = ((CGruntTileMgr*)P(this, 0x260))->FindGrunt(this);
    i32 atTarget = 0;
    if (g != 0) {
        i32 x = F(P(g, 0x10), 0x5c);
        if (x == F(g, 0x17c) && F(P(g, 0x10), 0x60) == F(g, 0x180)
            && this->TileProbe(x, F(P(g, 0x10), 0x60)) != 0) {
            atTarget = 1;
        }
    }

    if (F(this, 0x220) != 0) {
        if (F(this, 0x21c) != 0) {
            F(this, 0x21c) = 0;
            return 1;
        }
        if (F(this, 0x218) != 0) {
            return 1;
        }
        if (F(this, 0x3f0) >= 100) {
            if (RunGate(1) != 0) {
                return 1;
            }
            if (atTarget && g == 0) {
                return 1;
            }
            if (F(this, 0x220) == 0) {
                return 1;
            }
        } else {
            if (atTarget) {
                return 1;
            }
            if (F(this, 0x220) == 0) {
                return 1;
            }
        }
        if (F(this, 0x21c) != 0) {
            return 1;
        }
        F(this, 0x1e4) = 0;
        F(this, 0x218) = 0;
        F(this, 0x21c) = 0;
        F(this, 0x220) = 0;
        ResetEntrance(1, 0, 0);
        return 1;
    }

    if (g == 0) {
        F(this, 0x390) = 0;
        goto L_ed006b;
    }
    if (F(this, 0x21c) != 0) {
        return 1;
    }
    if (F(this, 0x218) == 0 && F(this, 0x3f0) >= 100) {
        if (atTarget) {
            CommitMove(F(g, 0x1ec), F(g, 0x1f0), F(g, 0x17c), F(g, 0x180));
            DRAIN_COORDS();
            return 1;
        }
    } else {
        if (atTarget) {
            DRAIN_COORDS();
            return 1;
        }
    }

L_ed006b:
    if (g == 0 || this->OwnsTile(F(g, 0x1ec), F(g, 0x1f0)) == 0) {
        F(this, 0x390) = 0;
        goto L_scanb;
    }
    if (F(this, 0x220) != 0) {
        goto L_scanb;
    }
    if (F(this, 0x3f0) >= 100 && F(P(g, 0x10), 0x5c) == F(g, 0x17c)
        && F(P(g, 0x10), 0x60) == F(g, 0x180)
        && this->TileProbe(F(P(g, 0x10), 0x5c), F(P(g, 0x10), 0x60)) != 0) {
        CommitMove(F(g, 0x1ec), F(g, 0x1f0), F(g, 0x17c), F(g, 0x180));
    }
    if (F(this, 0x220) != 0) {
        goto L_scanb;
    }
    if ((u32)F(this, 0x2ec) <= 0x1f4) {
        goto L_scanb;
    }
    {
        i32 cc[4];
        g->ReadCenter(cc);
        if (this->ProbeMove(cc[0] >> 5, cc[1] >> 5, 0, F(this, 0x248), 1, 0) != 0) {
            if (F(this, 0x390) != 0) {
                i32 x = F(P(this, 0x10), 0x5c);
                i32 y = F(P(this, 0x10), 0x60);
                if (BoardTest(P(g_gameReg->m_world->m_24, 0x5c) + 0x40, x, y) != 0) {
                    g_gameReg->m_cueSink->PlayCue(this, 0x366, -1, 0, -1, -1);
                }
                F(this, 0x390) = 0;
            }
            F(this, 0x2ec) = 0;
        }
    }

L_scanb:
    if (F(this, 0x328) != 0) {
        CScanCoord* coord = ((CScanNode324*)P(this, 0x320))->m_8;
        i32 col = coord->x;
        i32 row = coord->y;
        if (CellTargetable(col, row) != 0) {
            ((CGruntTileMgr*)P(this, 0x260))
                ->Scatter(F(this, 0x1ec), F(this, 0x1f0), (col << 5) + 0x10, (row << 5) + 0x10);
            StampMove(1, 1);
            F(this, 0x2ec) = 0;
        }
        return 1;
    }
    if ((u32)F(this, 0x2ec) <= 0x3e8) {
        return 1;
    }

    i32 r = F(this, 0x2dc);
    RECT box;
    box.left = cx - r;
    box.right = cx + r;
    box.top = cy - r;
    box.bottom = cy + r;
    RECT gb;
    gb.left = 0;
    gb.top = 0;
    gb.right = grid->m_c;
    gb.bottom = grid->m_10;
    RECT isect;
    if (!IntersectRect(&isect, &box, &gb)) {
        isect = box;
    }
    {
        RECT lb;
        lb.left = isect.left;
        lb.top = isect.top;
        lb.right = isect.right + 1;
        lb.bottom = isect.bottom + 1;
        RECT gb2;
        gb2.left = 0;
        gb2.top = 0;
        gb2.right = grid->m_c;
        gb2.bottom = grid->m_10;
        if (!IntersectRect(&grid->m_60, &lb, &gb2)) {
            grid->m_60 = lb;
        }
        grid->m_70 = grid->m_60.right - grid->m_60.left;
        grid->m_74 = grid->m_60.bottom - grid->m_60.top;
    }

    i32 best = 0x7fffffff;
    i32 bestX = 0;
    i32 bestY = 0;
    CScanGruntNode* node = ((CGruntTileMgr*)P(this, 0x260))->m_4;
    while (node != 0) {
        char* gg = node->m_8;
        node = node->m_next;
        if (F(gg, 0x5c) == 0) {
            i32 gx = F(gg, 0x54);
            i32 gy = F(gg, 0x58);
            if (this->TileProbe((gx << 5) + 0x10, (gy << 5) + 0x10) != 0) {
                ((CGruntTileMgr*)P(this, 0x260))
                    ->Scatter(F(this, 0x1ec), F(this, 0x1f0), (gx << 5) + 0x10, (gy << 5) + 0x10);
                GRID_RECT_BOUNDS(grid);
                return 1;
            }
            i32 dx = gx - (F(P(this, 0x10), 0x5c) >> 5);
            IABS(dx);
            i32 dy = gy - (F(P(this, 0x10), 0x60) >> 5);
            i32 dist = ((dy ^ (dy >> 31)) - (dy >> 31)) + dx;
            if (dist < best) {
                POINT pt;
                pt.x = gx;
                pt.y = gy;
                if (PtInRect(&isect, pt)) {
                    best = dist;
                    bestX = gx;
                    bestY = gy;
                }
            }
        }
    }
    if (best != 0x7fffffff) {
        i32 dx = bestX - cx;
        IABS(dx);
        i32 dy = bestY - cy;
        IABS(dy);
        if (dx <= 1 && dy <= 1) {
            ((CGruntTileMgr*)P(this, 0x260))
                ->Scatter(F(this, 0x1ec), F(this, 0x1f0), (bestX << 5) + 0x10, (bestY << 5) + 0x10);
            StampMove(1, 1);
        } else {
            this->ProbeMove(bestX, bestY, 0, F(this, 0x248), 1, 0);
        }
    }
    GRID_RECT_INLINE(grid);
    F(this, 0x2ec) = 0;
    return 1;
}

// ===========================================================================
// @early-stop
// Sibling of ArrivalScanA: stamps m_300/m_304 AFTER the atTarget probe, duplicates
// the powered-up reset per branch, gates the box scan on m_220==0 too, and matches
// grid cells by flag 0x10000 (active-move by 0x40|0x10000). Logic reconstructed fully;
// same deep-regalloc + slot-recycle wall family. Final-sweep candidate.
RVA(0x000f36a0, 0x78e)
i32 CGruntStep::ArrivalScanC() {
    if (strcmp(g_typeColl.Lookup(F(P(this, 0x14), 0x1c))->m_0, "I") == 0) {
        return 1;
    }
    CScanGrid* grid = (CScanGrid*)g_gameReg->m_tileGrid;
    GRID_RECT_BOUNDS(grid);

    i32 c1[4];
    ReadCenter(c1);
    i32 cx = c1[0] >> 5;
    i32 c2[4];
    ReadCenter(c2);
    i32 cy = c2[1] >> 5;

    CGruntStep* g = ((CGruntTileMgr*)P(this, 0x260))->FindGrunt(this);
    i32 atTarget = 0;
    if (g != 0) {
        i32 x = F(P(g, 0x10), 0x5c);
        if (x == F(g, 0x17c) && F(P(g, 0x10), 0x60) == F(g, 0x180)
            && this->TileProbe(x, F(P(g, 0x10), 0x60)) != 0) {
            atTarget = 1;
        }
    }

    F(this, 0x300) = F(this, 0x17c);
    F(this, 0x304) = F(this, 0x180);

    if (F(this, 0x220) != 0) {
        if (F(this, 0x21c) != 0) {
            F(this, 0x21c) = 0;
            return 1;
        }
        if (F(this, 0x218) != 0) {
            return 1;
        }
        if (F(this, 0x3f0) >= 100) {
            if (RunGate(1) != 0) {
                return 1;
            }
            if (atTarget && g == 0) {
                return 1;
            }
            if (F(this, 0x220) == 0) {
                return 1;
            }
            if (F(this, 0x21c) != 0) {
                return 1;
            }
            F(this, 0x1e4) = 0;
            F(this, 0x218) = 0;
            F(this, 0x21c) = 0;
            F(this, 0x220) = 0;
            ResetEntrance(1, 0, 0);
            return 1;
        }
        if (atTarget) {
            return 1;
        }
        if (F(this, 0x220) == 0) {
            return 1;
        }
        if (F(this, 0x21c) != 0) {
            return 1;
        }
        F(this, 0x1e4) = 0;
        F(this, 0x218) = 0;
        F(this, 0x21c) = 0;
        F(this, 0x220) = 0;
        ResetEntrance(1, 0, 0);
        return 1;
    }

    if (g == 0 || this->OwnsTile(F(g, 0x1ec), F(g, 0x1f0)) == 0) {
        F(this, 0x390) = 0;
        goto L_tailc;
    }
    if (F(this, 0x220) != 0) {
        goto L_tailc;
    }
    if (F(this, 0x3f0) >= 100 && F(P(g, 0x10), 0x5c) == F(g, 0x17c)
        && F(P(g, 0x10), 0x60) == F(g, 0x180)
        && this->TileProbe(F(P(g, 0x10), 0x5c), F(P(g, 0x10), 0x60)) != 0) {
        CommitMove(F(g, 0x1ec), F(g, 0x1f0), F(g, 0x17c), F(g, 0x180));
        F(this, 0x2ec) = 0;
        return 1;
    }
    if (F(this, 0x220) != 0) {
        goto L_tailc;
    }
    if ((u32)F(this, 0x2ec) <= 0x1f4) {
        goto L_tailc;
    }
    if (this->ProbeMove(F(P(g, 0x10), 0x5c) >> 5, F(P(g, 0x10), 0x60) >> 5, 0, F(this, 0x248), 1, 0)
        != 0) {
        if (F(this, 0x390) != 0) {
            char* board = P(g_gameReg->m_world->m_24, 0x5c) + 0x40;
            i32 x = F(P(this, 0x10), 0x5c);
            i32 y = F(P(this, 0x10), 0x60);
            if (x < F(board, 8) && F(board, 0) <= x && y < F(board, 0xc) && F(board, 4) <= y) {
                g_gameReg->m_cueSink->PlayCue(this, 0x366, -1, 0, -1, -1);
            }
            F(this, 0x390) = 0;
        }
        F(this, 0x2ec) = 0;
    }

L_tailc:
    if (F(this, 0x328) != 0) {
        CScanCoord* coord = ((CScanNode324*)P(this, 0x320))->m_8;
        i32 col = coord->x;
        i32 row = coord->y;
        CScanCell* cell = &grid->m_8[row][col];
        if ((cell->m_flags & 0x40) != 0 || (cell->m_flags & 0x10000) != 0) {
            ((CGruntTileMgr*)P(this, 0x260))
                ->Scatter(F(this, 0x1ec), F(this, 0x1f0), (col << 5) + 0x10, (row << 5) + 0x10);
            StampMove(1, 1);
            F(this, 0x2ec) = 0;
        }
        return 1;
    }
    if (F(this, 0x220) == 0 && (u32)F(this, 0x2ec) > 0x3e8) {
        i32 r = F(this, 0x2dc);
        RECT box;
        box.left = cx - r;
        box.right = cx + r;
        box.top = cy - r;
        box.bottom = cy + r;
        RECT gb;
        gb.left = 0;
        gb.top = 0;
        gb.right = grid->m_c;
        gb.bottom = grid->m_10;
        RECT isect;
        if (!IntersectRect(&isect, &box, &gb)) {
            isect = box;
        }
        {
            RECT lb;
            lb.left = isect.left;
            lb.top = isect.top;
            lb.right = isect.right + 1;
            lb.bottom = isect.bottom + 1;
            RECT gb2;
            gb2.left = 0;
            gb2.top = 0;
            gb2.right = grid->m_c;
            gb2.bottom = grid->m_10;
            if (!IntersectRect(&grid->m_60, &lb, &gb2)) {
                grid->m_60 = lb;
            }
            grid->m_70 = grid->m_60.right - grid->m_60.left;
            grid->m_74 = grid->m_60.bottom - grid->m_60.top;
        }
        i32 best = 0x7fffffff;
        i32 bestCol = -1;
        i32 bestRow = -1;
        for (i32 row = isect.top; row < isect.bottom; row++) {
            CScanCell* cell = &grid->m_8[row][isect.left];
            for (i32 col = isect.left; col < isect.right; col++) {
                if ((cell->m_flags & 0x10000) != 0) {
                    i32 dr = row - cy;
                    IABS(dr);
                    i32 dc = col - cx;
                    IABS(dc);
                    i32 dist = dr + dc;
                    if (dist < best) {
                        best = dist;
                        bestCol = col;
                        bestRow = row;
                    }
                }
                cell = (CScanCell*)((char*)cell + 0x1c);
            }
        }
        if (best != 0x7fffffff) {
            i32 dc = bestCol - cx;
            IABS(dc);
            i32 dr = bestRow - cy;
            IABS(dr);
            if (dc <= 1 && dr <= 1) {
                ((CGruntTileMgr*)P(this, 0x260))
                    ->Scatter(
                        F(this, 0x1ec),
                        F(this, 0x1f0),
                        (bestCol << 5) + 0x10,
                        (bestRow << 5) + 0x10
                    );
                StampMove(1, 1);
            } else {
                this->ProbeMove(bestCol, bestRow, 0, F(this, 0x248), 1, 0);
            }
        }
        GRID_RECT_INLINE(grid);
        F(this, 0x2ec) = 0;
    }
    return 1;
}

SIZE_UNKNOWN(CGruntCueSink);
SIZE_UNKNOWN(CGruntStep);
SIZE_UNKNOWN(CScanCell);
SIZE_UNKNOWN(CScanCoord);
SIZE_UNKNOWN(CScanGrid);
SIZE_UNKNOWN(CScanGruntNode);
SIZE_UNKNOWN(CScanListNode);
SIZE_UNKNOWN(CScanNode324);
SIZE_UNKNOWN(CScanRectInit);
SIZE_UNKNOWN(CStepList);
SIZE_UNKNOWN(CStepList2);
SIZE_UNKNOWN(CTypeColl);
SIZE_UNKNOWN(CTypeNode);
