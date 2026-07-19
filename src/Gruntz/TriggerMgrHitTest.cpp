// TriggerMgrHitTest.cpp - the CTriggerMgr hit-test/probe obj: retail .text
// [0x759e0..0x75e1a], the SECOND of the three objs our old `triggermgr` unit
// conflated (SPLIT verdict, docs/exe-map/TU_MIGRATION.md). Original TU:
// filename unknown (@identity-TODO).
//
// Oracle evidence for the carve (own obj, distinct from 0x6b640 and 0x77f80):
//   * the block directly continues the UNRECONSTRUCTED ~21 KB megafunction
//     FUN_6f2f0 (0x6f2f0..~0x7450b, `ret 0x24`, + its switch tables filling
//     0x74540..0x759e0 flush to GetOriginXY) - that megafunction is this TU's
//     main body and the ONLY caller of GetOriginXY, TmFlagsAllow, CPairXY::Set
//     and CGridLookup::Lookup (gruntz sema xref, exclusive);
//   * no CRT init-frag run of its own (both triggermgr runs bracket the OTHER
//     two objs: 10@0x6b370 and 7@0x7d8f0);
//   * bounded by CC pads / foreign objs (goowellmgr end 0x6f16f + 0x180 CC
//     before the megafn; terraintileloader::Load @0x75e90 after).
// Reconstructing FUN_6f2f0 (and the 4-byte FUN_00475ad0 @0x75ad0) here is the
// TU's remaining worklist.
//
// Functions in retail-RVA order; shared views/externs in
// <Gruntz/TriggerMgrViews.h>. /GX profile kept from the parent unit (no EH
// temps in these leaves; byte-neutral).
#include <Gruntz/Grunt.h> // CTmCell IS CGrunt (folded) - the cells are dereferenced here
#include <Gruntz/GameRegPtr.h>
#include <Gruntz/TriggerMgr.h>

#include <Gruntz/TileGrid.h> // canonical CTileGrid (FindGruntAt's packed owner grid)
#include <Globals.h>

#include <Gruntz/TriggerMgrViews.h>  // the shared CTm* views + singleton externs

// 0x759e0: GetOriginXY - copy the cached origin pair (m_cellFlag[0x16],[0x17] ==
// +0x174,+0x178) into the caller's out-slot and return it.
RVA(0x000759e0, 0x18)
CTrigPoint* CTriggerMgr::GetOriginXY(CTrigPoint* out) {
    out->x = m_cellFlag[0x16];
    out->y = m_cellFlag[0x17];
    return out;
}

// ---------------------------------------------------------------------------
// CPairXY / CGridCell / CGridLookup (the megafn FUN_6f2f0 leaf helpers, @identity-TODO)
// are declared in <Gruntz/TriggerMgrViews.h> (included above) - their shapes belong in
// the family scaffolding header; only the method bodies live here.

// CPairXY::Set (0x75a10) - fill m_0/m_4 and return this. @identity-TODO: owner unrecovered
// (called ONLY by this TU's megafn FUN_6f2f0).
RVA(0x00075a10, 0x12)
CPairXY* CPairXY::Set(i32 a, i32 b) {
    m_0 = a;
    m_4 = b;
    return this;
}

// ---------------------------------------------------------------------------
// 0x75a40: a 2D grid lookup - bounds-check (x, y) against the width/height, then
// return the first dword of the (28-byte-stride) cell at rows[y][x]; out of bounds
// returns 1.
RVA(0x00075a40, 0x34)
i32 CGridLookup::Lookup(i32 x, i32 y) {
    if (static_cast<u32>(x) < static_cast<u32>(m_c) && static_cast<u32>(y) < static_cast<u32>(m_10)) {
        return m_8[y][x].m_0;
    }
    return 1;
}
// (^ @identity-TODO: called ONLY by this TU's megafn FUN_6f2f0; moved from
// OrphanMethods.cpp per the interval verdict. The m_8/m_c/m_10 trio + the
// 0x1c cell stride are the SAME shape as canonical CTileGrid - a likely
// CTileGrid method; kept as the view pending the megafn's reconstruction,
// because respelling the [y][x] walk onto CTileGrid's i32* rows changes the
// scaled-index instruction selection.)

// 0x75a90: TmFlagsAllow(a, b, c) - a __cdecl trigger-flag compatibility test on the
// shared bits m = a & b: bit 0x20000000 vetoes outright; with no shared bits, allow;
// otherwise allow only when (a & c) is also set.
RVA(0x00075a90, 0x27)
i32 TmFlagsAllow(i32 a, i32 b, i32 c) {
    i32 m = b & a;
    if (m & 0x20000000) {
        return 0;
    }
    if (m != 0 && (c & a) == 0) {
        return 0;
    }
    return 1;
}

// 0x75af0: HitTestCell(x, y, outRow, outCol, exact) - sample the plane tile-attr at
// (x>>5, y>>5); its high byte is the row, low byte the col. Look up grid[row*15+col]; if
// live+clickable, either exact-match its world pos (exact) or its 30x30 bounds, then write
// (row,col) through the out-ptrs. ret 0 on any miss. (__stdcall: ret 0x14.)
// @early-stop
// regalloc + bounds-arith wall: the row/col high/low-byte split pins edi/ebp and the
// ±7 box arithmetic spills to different slots than retail. Logic + offsets byte-exact.
RVA(0x00075af0, 0x111)
i32 CTriggerMgr::HitTestCell(i32 x, i32 y, i32* outRow, i32* outCol, i32 exact) {
    CTileGrid* plane = g_gameReg->m_tileGrid;
    i32 ix = x >> 5;
    i32 iy = y >> 5;
    i32 attr;
    if (ix >= plane->m_c || iy >= plane->m_10) {
        attr = -1;
    } else {
        attr = plane->m_8[iy][ix * 7 + 1];
    }
    if (attr == -1) {
        return 0;
    }
    i32 row = (attr >> 8) & 0xff;
    i32 col = attr & 0xff;
    CTmCell* cell = m_grid[col + row * TM_GRID_COLS];
    if (cell == 0 || cell->m_entranceCommitted == 0) {
        return 0;
    }
    if (exact != 0) {
        CGameObject* o = cell->m_10;
        if (o->m_screenX != x || o->m_screenY != y) {
            return 0;
        }
        if (outRow != 0) {
            *outRow = row;
        }
        if (outCol != 0) {
            *outCol = col;
        }
        return 1;
    }
    CGameObject* o = cell->m_10;
    i32 ox = o->m_screenX;
    i32 oy = o->m_screenY;
    if (x + 7 > ox + 14 || x - 7 < ox - 7 || y + 7 > oy + 14 || y - 7 < oy - 7) {
        return 0;
    }
    if (outRow != 0) {
        *outRow = row;
    }
    if (outCol != 0) {
        *outCol = col;
    }
    return 1;
}

// 0x75c60: CTriggerMgr::FindGruntAt - given a pixel point + tile-span rect (or an
// explicit source rect), scan the surrounding tile cells: read each cell's packed
// owner word out of the tile grid, index the placed-object grid (m_grid[col+row*15]),
// and return the first live cell (m_1fc) whose 15x15 display box hits the rect, with
// its (col,row) reported via out-params. The sibling of CellHitTest (same m_grid /
// m_10->m_5c scan). (Re-homed from ApiWrappers; the GruntHitMgr/HitGrunt/HitTileRect/
// HitGrid views folded onto CTmCell / CTmDisplay / CTileGrid.)
//
// @early-stop
// 75% - nested-loop regalloc wall: identical instruction selection/logic, but MSVC5
// assigns the point/rect args to a permuted register set (a0->edi, a1->esi vs retail
// a0->ebx/a1->edi) and spills one fewer local, so the frame is 0x1c vs retail 0x20 and
// every [esp+N] stack offset shifts. Not steerable from source (llvm-objdump -dr: same
// mnemonics, shifted operands).
RVA(0x00075c60, 0x1ba)
CTmCell* CTriggerMgr::FindGruntAt(i32 px, i32 py, RECT* span, i32* outCol, i32* outRow, RECT* src) {
    i32 tcol = px >> 5;
    i32 trow = py >> 5;
    RECT rc;
    if (src) {
        CopyRect(&rc, src);
    } else {
        SetRect(
            &rc,
            px - span->left * 32 - 7,
            py - span->top * 32 - 7,
            span->right * 32 + px + 7,
            span->bottom * 32 + py + 7
        );
    }
    i32 xEnd = span->right + tcol + 1;
    for (i32 x = tcol - span->left - 1; static_cast<u32>(x) <= static_cast<u32>(xEnd); x++) {
        i32 yEnd = span->bottom + trow + 1;
        for (i32 y = trow - span->top - 1; static_cast<u32>(y) <= static_cast<u32>(yEnd); y++) {
            if (static_cast<u32>(x) >= static_cast<u32>(g_gameReg->m_tileGrid->m_c)) {
                continue;
            }
            CTileGrid* grid = g_gameReg->m_tileGrid;
            if (static_cast<u32>(y) >= static_cast<u32>(grid->m_10)) {
                continue;
            }
            i32 val;
            if (static_cast<u32>(x) < static_cast<u32>(grid->m_c) && static_cast<u32>(y) < static_cast<u32>(grid->m_10)) {
                val = *(i32*)(reinterpret_cast<char*>(grid->m_8[y]) + x * 0x1c + 4);
            } else {
                val = -1;
            }
            if (val == -1) {
                continue;
            }
            i32 col = val & 0xff;
            i32 row = (val >> 8) & 0xff;
            CTmCell* g = m_grid[col + row * TM_GRID_COLS];
            if (!g) {
                continue;
            }
            if (!g->m_entranceCommitted) {
                continue;
            }
            i32 sx = g->m_10->m_screenX - 7;
            i32 sy = g->m_10->m_screenY - 7;
            if (rc.left <= sx + 0xe && rc.right >= sx && rc.top <= sy + 0xe && rc.bottom >= sy) {
                *outCol = row;
                *outRow = col;
                return g;
            }
        }
    }
    return 0;
}
