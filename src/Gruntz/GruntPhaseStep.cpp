// GruntPhaseStep.cpp - CGrunt::PhaseStep (0x000f60f0, __thiscall ret 4, /GX): the
// arrival/relocation phase state machine. Gated on the grunt's resolved type name
// (g_typeColl.Lookup(m_14->m_1c) vs "F"): for a non-"F" grunt it drives a small
// state machine on m_defenderState (states 0/2/4/0x19/0x1a) that recomputes the
// grunt's target tile, builds the 16 border cells of the 5x5 block around it into a
// point accumulator, picks a random still-free cell to relocate/arrive on (marking
// tiles via the tile-manager), and recycles the visited-cell CObList nodes back onto
// the shared free list. /GX for the accumulator + rect temporaries.
//
// FOLDED onto the canonical CGrunt (<Gruntz/Grunt.h>): the C1189 wall doc that kept
// this a pure-Win32 partial is disproven (<Mfc.h> is a superset of <Win32.h>, and
// ScanGrid.h/ScanRectInit.h coexist after it - see GruntPathScan.cpp). The local
// `struct CGrunt` view + the local TileMgr/MapObj/NeighborNode/ObList views are gone:
// this is a real CGrunt method; the grid/coord views are the shared CScanGrid family;
// the manager singleton is g_pGameRegistry; the cue is m_cueSink->CueA; the visited
// nodes are the canonical m_320 GruntCoordNode chain. Placeholder field names; only
// offsets + code bytes are load-bearing.
#include <Gruntz/Grunt.h> // canonical CGrunt (pulls <Mfc.h> FIRST: RECT + IntersectRect)

#include <rva.h>
#include <Ints.h>
#include <string.h>
#include <Gruntz/ScanGrid.h>     // CScanGrid tile-grid + CScanCell (dirty-rect view)
#include <Gruntz/ScanRectInit.h> // Set34a4 rect-init helper
#include <Gruntz/TypeColl.h>     // g_typeColl type-name lookup

#pragma intrinsic(strcmp)

// The shared game-manager singleton (*0x64556c): the +0x60 cue sink, +0x30 world,
// +0x68 reused command grid, +0x70 tile grid. (The WwdGameReg g_gameReg name is a
// dual-view of the same address; the CGameRegistry facet is what this TU reaches.)
extern CGameRegistry* g_pGameRegistry; // ?g_gameReg@@3PAUWwdGameReg@@A (0x64556c)

// The type-name collection singleton (0x6bf650, aliased g_animNameResolver in
// Grunt.h): Lookup(key)->node, node->m_0 = the type-name string. DATA-bound in
// GruntUpdateStep.cpp; referenced here as a reloc-masked extern.
extern CTypeColl g_typeColl; // 0x6bf650

// The shared free-list of recycled CObList coord nodes (head @0x645544, bias @0x64554c;
// the same g_freePoolHead/Base pool the movement machines front).
extern void* g_freeList;       // 0x645544
extern i32 g_freeListNodeBias; // 0x64554c

// The 16-border-cell point accumulator: a /GX-forcing stack CDWordArray temporary
// unique to PhaseStep (kept a local view like GruntPathScan.cpp's CScanList). m_4 is
// the packed-point array, m_8 the live count. Its four MFC-collection methods are
// external/no-body (reloc-masked).
struct CGruntPtAcc {
    i32 m_0;
    i32* m_4;                    // +0x04  packed-point array
    i32 m_8;                     // +0x08  live count
    void Ctor();                 // 0x1b4b43
    void Add(i32 count, i32 pt); // 0x1b4d7c
    void RemoveAt(i32 i, i32 f); // 0x1b4e38
    void Dtor();                 // 0x1b4b76
};
SIZE_UNKNOWN(CGruntPtAcc);

// Recompute the grid dirty rect (m_60) as {0,0,w,h} intersected with a copy, then
// m_70/m_74 = the resulting size (the shared GruntTileScan dirty-rect idiom).
#define GRID_BOUNDS(grid)                                                                          \
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

// Recycle the visited-coord CObList nodes (head) back onto the shared free list.
#define RECYCLE_COORDS(head)                                                                       \
    {                                                                                              \
        GruntCoordNode* n = (head);                                                                \
        while (n != 0) {                                                                           \
            GruntCoordNode* next = n->m_next;                                                      \
            void* pay = n->m_coord;                                                                \
            if (pay != 0) {                                                                        \
                void** slot = (void**)((char*)pay - g_freeListNodeBias);                           \
                *slot = g_freeList;                                                                \
                g_freeList = slot;                                                                 \
            }                                                                                      \
            n = next;                                                                              \
        }                                                                                          \
    }

// @early-stop
// regalloc + region-build wall. Complete reconstruction folded onto the canonical
// CGrunt: the type-name gate (inline strcmp of g_typeColl.Lookup(m_14->m_1c) vs "F"),
// the m_defenderState state dispatch (0x19/0x1a re-mark, 0/2/4), the 5x5-border
// 16-point accumulator build + random-free-cell relocation with tile marking
// (TileSwitch6), the state-0 neighbour resolve (GetOccupant + RectContains/CommitNeighbor/
// GruntInRadius + m_cueSink->CueA on-screen cue), and the common tail's coord recycle +
// CommitTileSlot2 arrival commit all align by shape (llvm-objdump -dr). Residual: MSVC5
// pins the tile coords/loop indices across esi/edi/ebp/ebx and schedules the 16 unrolled
// packed-point stores + IntersectRect rect temporaries at [esp+N] slots a source
// transcription can't reproduce exactly.
RVA(0x000f60f0, 0xb30)
i32 CGrunt::PhaseStep() {
    CGruntPtAcc acc;
    GruntTilePos pa;
    GruntTilePos pb;

    m_358 = 0;
    if (strcmp(g_typeColl.Lookup((i32)m_14->m_1c)->m_0, g_codeF) == 0) {
        return 1;
    }
    m_defenderX = m_lastTilePxX;
    m_defenderY = m_lastTilePxY;

    if (m_defenderState == 0x19) {
        GetScreenPos(&pa);
        i32 ax = pa.m_x >> 5;
        GetScreenPos(&pb);
        i32 gx = (pb.m_x >> 5) - m_arrivalCol + ax;
        GetScreenPos(&pa);
        i32 ay = pa.m_y >> 5;
        GetScreenPos(&pb);
        i32 gy = (pb.m_y >> 5) - m_arrivalRow + ay;
        TileSwitch6(gx, gy, 0, m_arrivalFlags, 1, 0);
        m_dwell = 0;
        m_defenderState = 4;
    }
    if (m_defenderState == 0x1a) {
        GetScreenPos(&pa);
        i32 ax = pa.m_x >> 5;
        GetScreenPos(&pb);
        GetScreenPos(&pa);
        i32 gx = (pb.m_x >> 5) - m_arrivalCol + ax;
        i32 ay = pa.m_x >> 5;
        GetScreenPos(&pb);
        i32 gy = (pb.m_y >> 5) - m_arrivalRow + ay;
        TileSwitch6(gx, gy, 0, m_arrivalFlags, 1, 0);
        m_defenderState = 0;
        return 1;
    }

    if (m_defenderState == 0) {
        goto state0;
    }
    if (m_defenderState == 2) {
        goto state2;
    }
    if (m_defenderState != 4) {
        goto common;
    }
    if (m_dwell <= 0x1f40) {
        return 1;
    }
    m_defenderState = 0;
    return 1;

state2: {
    if (strcmp(g_typeColl.Lookup((i32)m_14->m_1c)->m_0, g_codeF) == 0) {
        goto common;
    }
    i32 x = m_arrivalCol;
    i32 y = m_arrivalRow;
    CScanGrid* grid = (CScanGrid*)g_pGameRegistry->m_tileGrid;
    {
        RECT box;
        box.left = x - 4;
        box.top = y - 4;
        box.right = x + 5;
        box.bottom = y + 5;
        RECT gb;
        gb.left = 0;
        gb.top = 0;
        gb.right = grid->m_c;
        gb.bottom = grid->m_10;
        if (!IntersectRect(&grid->m_60, &box, &gb)) {
            grid->m_60 = box;
        }
        grid->m_70 = grid->m_60.right - grid->m_60.left;
        grid->m_74 = grid->m_60.bottom - grid->m_60.top;
    }
    acc.Ctor();
    acc.Add(acc.m_8, ((x - 2) << 16) | ((y - 2) & 0xffff));
    acc.Add(acc.m_8, ((x - 1) << 16) | ((y - 2) & 0xffff));
    acc.Add(acc.m_8, (x << 16) | ((y - 2) & 0xffff));
    acc.Add(acc.m_8, ((x + 1) << 16) | ((y - 2) & 0xffff));
    acc.Add(acc.m_8, ((x + 2) << 16) | ((y - 2) & 0xffff));
    acc.Add(acc.m_8, ((x - 2) << 16) | ((y + 2) & 0xffff));
    acc.Add(acc.m_8, ((x - 1) << 16) | ((y + 2) & 0xffff));
    acc.Add(acc.m_8, (x << 16) | ((y + 2) & 0xffff));
    acc.Add(acc.m_8, ((x + 1) << 16) | ((y + 2) & 0xffff));
    acc.Add(acc.m_8, ((x + 2) << 16) | ((y + 2) & 0xffff));
    acc.Add(acc.m_8, ((x - 2) << 16) | ((y - 1) & 0xffff));
    acc.Add(acc.m_8, ((x - 2) << 16) | (y & 0xffff));
    acc.Add(acc.m_8, ((x - 2) << 16) | ((y + 1) & 0xffff));
    acc.Add(acc.m_8, ((x + 2) << 16) | ((y - 1) & 0xffff));
    acc.Add(acc.m_8, ((x + 2) << 16) | (y & 0xffff));
    acc.Add(acc.m_8, ((x + 2) << 16) | ((y + 1) & 0xffff));
    while (acc.m_8 != 0) {
        i32 sel = GruntRand() % acc.m_8;
        i32 pt = acc.m_4[sel];
        i32 px = (u32)pt >> 0x10;
        i32 py = pt & 0xffff;
        CScanGrid* pl = (CScanGrid*)g_pGameRegistry->m_tileGrid;
        i32 flag;
        if ((u32)px < (u32)pl->m_c && (u32)py < (u32)pl->m_10 && px < pl->m_c && py < pl->m_10) {
            flag = ((i32*)pl->m_8[py])[px * 8 - px];
        } else {
            flag = 1;
        }
        if ((flag & 0x939) == 0) {
            if (TileSwitch6(px, py, 0, m_arrivalFlags, 1, 0) != 0) {
                m_defenderState = 4;
                m_dwell = 0;
                goto build_tail;
            }
        }
        acc.RemoveAt(sel, 1);
    }
build_tail: {
    CScanGrid* pl2 = (CScanGrid*)g_pGameRegistry->m_tileGrid;
    GRID_BOUNDS(pl2);
    acc.Dtor();
    goto common;
}
}

state0: {
    CGrunt* nb = m_tileMgr->GetOccupant(this);
    if (nb == 0) {
        goto common;
    }
    if (nb->m_entranceCommitted == 0) {
        goto common;
    }
    if (m_poweredUp == 0 && m_stamina >= 0x64 && nb->m_10->m_5c == nb->m_lastTilePxX
        && nb->m_10->m_60 == nb->m_lastTilePxY
        && RectContains(nb->m_10->m_5c, nb->m_10->m_60) != 0) {
        CommitNeighbor(nb->m_tileOwnerHi, nb->m_tileOwnerLo, nb->m_lastTilePxX, nb->m_lastTilePxY);
        m_arrivalCol = nb->m_10->m_5c >> 5;
        m_arrivalRow = nb->m_10->m_60 >> 5;
        m_defenderState = 2;
        goto common;
    }
    if (m_dwell <= 0x1f4) {
        goto common;
    }
    if (GruntInRadius(nb->m_tileOwnerHi, nb->m_tileOwnerLo) == 0) {
        goto s0_reset;
    }
    if (TileSwitch6(nb->m_10->m_5c >> 5, nb->m_10->m_60 >> 5, 0, m_arrivalFlags, 1, 0) == 0) {
        m_24c |= 0x4020;
        TileSwitch6(nb->m_10->m_5c >> 5, nb->m_10->m_60 >> 5, 0, m_arrivalFlags, 1, 0);
        m_24c &= 0xffffbfdf;
    }
    m_dwell = 0;
    if (m_390 == 0) {
        goto common;
    }
    if (GruntPointVisible(g_pGameRegistry->m_world->m_24->m_5c + 0x40, m_10->m_5c, m_10->m_60)
        == 0) {
        goto s0_reset;
    }
    g_pGameRegistry->m_cueSink->CueA(this, 0x366, -1, 0, -1, -1);
s0_reset:
    m_390 = 0;
    goto common;
}

common: {
    i32 st = m_defenderState;
    if (st != 4 && st != 0x19 && m_coordCount >= 2) {
        GruntCoordNode* head = m_320;
        i32 bx = head->m_coord->m_x;
        i32 by = head->m_coord->m_y;
        GruntCoord* nc = head->m_next->m_coord;
        i32 fx = nc->m_x;
        i32 fy = nc->m_y;
        CScanGrid* pl = (CScanGrid*)g_pGameRegistry->m_tileGrid;
        i32 flag;
        if ((u32)fx < (u32)pl->m_c && (u32)fy < (u32)pl->m_10) {
            flag = ((i32*)pl->m_8[fy])[fx * 8 - fx];
        } else {
            flag = 1;
        }
        if ((flag & 0x20) != 0) {
            if (m_coordCount != 0) {
                RECYCLE_COORDS(m_320);
                m_31c.RemoveAll();
            }
            ((CGruntTileMgr*)g_pGameRegistry->m_68)
                ->CommitTileSlot2(m_tileOwnerHi, m_tileOwnerLo, bx * 32 + 16, by * 32 + 16);
            m_arrivalCol = bx;
            m_arrivalRow = by;
            m_defenderState = 0x19;
            return 1;
        }
    }
    if (m_coordCount == 0) {
        return 1;
    }
    GruntCoord* p1 = m_320->m_coord;
    CScanGrid* pl2 = (CScanGrid*)g_pGameRegistry->m_tileGrid;
    i32 gx = p1->m_x;
    i32 gy = p1->m_y;
    i32 flag2;
    if ((u32)gx < (u32)pl2->m_c && (u32)gy < (u32)pl2->m_10) {
        flag2 = ((i32*)pl2->m_8[gy])[gx * 8 - gx];
    } else {
        flag2 = 1;
    }
    if ((flag2 & 0x20) == 0) {
        return 1;
    }
    m_arrivalCol = gx;
    m_arrivalRow = gy;
    if (m_coordCount != 0) {
        RECYCLE_COORDS(m_320);
        m_31c.RemoveAll();
    }
    m_defenderState = 0x1a;
    return 1;
}
}
