// Brickz.cpp - the scattered OUT-OF-LINE CBrickzGrid singletons (Tier B of the
// de-fragmentation assessment below). The Tier-A out-of-line pathfinding core
// (block F, 0x9ea60..0x9f7b7: AllocGrid/Search/Expand/Insert/PopFront/CellPush/
// Find/FindCellNode/Drain/Reset/Unlink/CellPop) is homed in src/Gruntz/MapMgr.cpp
// per docs/exe-map/interval-dossiers.md #10a (mapmgr + brickz-interval = ONE
// original TU; A-B-A-B weave + one bracketing init-frag run).
//
// Placeholder class name (see <Gruntz/Brickz.h>): these are __thiscall pointer-
// shuffle ops over a self-contained graph/grid container's intrusive node lists.
// They match by shape; field names are placeholders, offsets are load-bearing.
//
// ---------------------------------------------------------------------------
// DE-FRAGMENTATION ASSESSMENT (matcher-2, 2026-07-10; Tier A since re-homed):
// One dominant class, CBrickzGrid (non-polymorphic grid/pathfinding container,
// all QAE), + two tiny homed helpers (BrickzGridDesc::SetCell @0x77dc0,
// Grid_77df0::FindNearest @0x77df0). Game object CBrickz is its own TU
// (src/Gruntz/CBrickz.cpp).
//
// TIER B - the scattered singletons here (Clip 0x2b340 | ComputeCellFlags 0x77790 |
//   SetCell 0x77dc0 | FindNearest 0x77df0 | SearchEdge/UpdDiag/Line 0x81e10.. |
//   IsCellClear 0x853f0 | Serialize 0x9356c) are OUT-OF-LINE functions that retail
//   CALLS: each has its own rva, reached from other TUs via incremental-link
//   thunks, and the delinked target references them as an `U <name>` extern (= a
//   linked CALL). They STAY OUT-OF-LINE here. DO NOT move them to Brickz.h.
//   PROVEN (2026-07-10, measured end-to-end): making ComputeCellFlags an `inline`
//   header member makes MSVC5 /O2 INLINE it into its callers (BuildRockBreakParticles
//   doubled 1008->2096B, the switch body folded in, 0 calls left) ->
//   RockBreakParticles 81->0, ApplyMove 70->0. The DELINKER IS NOT THE CAUSE -
//   re-attributing the rva alone (no recompile) craters nothing. Retail inlines
//   these SELECTIVELY: ComputeCellFlags is inlined ONLY into CBrickz::LoadAttributes
//   @0x810f0 (the switch consts appear at exactly 2 rvas: 0x77790 + 0x8150c) and
//   CALLED from the other 3 sites. We mirror that by hand: out-of-line member +
//   hand-written inlined body where retail inlined it (see BrickzLoad.cpp), NEVER
//   the `inline` keyword. IsCellClear is data-ref'd ONLY from vtable slots
//   ??_7CMapMgr@@6B@+0x14 & ??_7CGruntzMapMgr@@6B@+0x14 => an inline VIRTUAL of
//   CMapMgr (slot 5), mislabeled CBrickzGrid::IsCellClear (@owner-TODO). Virtuals
//   ARE safe as header-inline (vtable dispatch = `call *(%eax)`, never inlined at
//   the call site) - that is the ONLY safe header-inline case. See docs/patterns/
//   nonvirtual-inline-header-craters-delinker-packing.md (corrected: root cause is
//   MSVC inlining, not the delinker).
//
// Remaining work: per-method @early-stop residue (final sweep); Grid_77df0 identity
// via xref (@identity-TODO); IsCellClear -> CMapMgr slot 5 (real virtual,
// header-inline-safe); the CBrickzGrid<->CMapMgr identity reconcile (see MapMgr.cpp).
// ---------------------------------------------------------------------------
#include <rva.h>
#include <Gruntz/BattlezData.h>
#include <stdlib.h> // abs (/Oi intrinsic: |goal-cur| lowers to cdq/xor/sub, not jns)

#include <Gruntz/Brickz.h>
#include <Win32.h> // RECT + IntersectRect (Clip) + PtInRect (FindNearest)

// MapSerializeCurve (0x0ec230) - declared 4-arg __cdecl here: the Serialize
// wrapper forwards all four of its args unchanged (the callee only reads the first
// two). Modeled no-body so the call reloc-masks. Same symbol as the 2-arg form in
// MapLogic.cpp; the extra params don't change the @0xec230 displacement.
extern "C" i32 __cdecl MapSerializeCurve(i32 a0, i32 a1, i32 a2, i32 a3);

// ---------------------------------------------------------------------------
// CBrickzGrid::Clip (0x02b340) - board dirty-rect clip finaliser. Clip the
// (0,0,m_width,m_height) box against the optional src rect (right/bottom inclusive
// -> +1), store the clipped rect into the +0x60 bound-rect (m_originX..m_boundBottom),
// then derive the +0x70/+0x74 extents. __thiscall(const RECT*) ; ret 0x4.
// (Homed from BattlezMapConfig.cpp; was the placeholder ClipHost_02b340 view -
//  this->m_board is a CBrickzGrid, so Clip is a real CBrickzGrid method.)
// @early-stop
// regalloc-rotation + scheduling wall (~81%): the clip logic is faithful, but retail
// keeps `src` in eax and interleaves the rect field loads/stores differently, while cl
// pins `src` in edx - a whole-function register rotation + a scheduling shift. Not
// source-steerable.
RVA(0x0002b340, 0xaa)
void CBrickzGrid::Clip(const RECT* src) {
    RECT a, b;
    b.left = 0;
    b.top = 0;
    b.right = m_width;
    b.bottom = m_height;
    if (src) {
        a.left = src->left;
        a.top = src->top;
        a.right = src->right + 1;
        a.bottom = src->bottom + 1;
    } else {
        a.left = 0;
        a.top = 0;
        a.right = m_width;
        a.bottom = m_height;
    }
    if (!IntersectRect((RECT*)&m_originX, &a, &b)) {
        *(RECT*)&m_originX = a;
    }
    m_gridW = m_boundRight - m_originX;
    m_gridH = m_boundBottom - m_originY;
}

// ---------------------------------------------------------------------------
// CBrickzGrid::ComputeCellFlags (0x077790) - terrain-grid cell-flag compute.
// Look up the 1-based bute-type id for cell (x,y) through the m_78 attribute
// manager (clamp x/y into the grid, index the flat id table; id 0xeeeeeeee /
// 0xffffffff = empty), switch it to a packed flag value, OR in the preserved high
// bits (0x1bf40000 / 0x20000000) of the old cell flags, stash id3 at +0xc and the
// type id at +0x10, then run the 8-neighbour edge-update walk over the 3x3 block
// around (x,y): for each in-bounds neighbour cell whose own flag bit 0x100 is set,
// clear its 0x1000 bit and re-set it when one of the four opposite neighbour pairs
// is both passable (no 0x939 bit).
// @early-stop
// dense-switch + 8-neighbour spill wall (objdiff 0% scoring artifact): the bute-id
// lookup + the 0x99-case jump table (cases written in retail .text body order, so
// the byte index LUT + jump table + the per-case `mov [cell],flagval` bodies all
// align) are byte-correct, but the deep 8-neighbour walk's pointer/spill schedule
// diverges from retail's stack-slot-heavy neighbour layout, and the big jump-table
// data region (REL32 vs cl's $L self-relocs) drags the whole-symbol % to 0 (the
// jumptable-data-overlap scoring artifact). Logic complete; parked for the sweep.
RVA(0x00077790, 0x37d)
void CBrickzGrid::ComputeCellFlags(i32 x, i32 y, i32 id3) {
    // The target cell pointer is computed first and held in a callee-saved register
    // across the whole body (retail's esi).
    BrickzCell* cell = &m_rows[y][x];
    BrickzAttrMgr* attr = m_attrMgr->m_24;
    // Clamp the lookup coordinate into the grid descriptor's extent.
    i32 cx = x;
    if (x < 0) {
        cx = 0;
    } else if (x >= attr->m_5c->m_28) {
        cx = attr->m_5c->m_28 - 1;
    }
    i32 cy = y;
    if (y < 0) {
        cy = 0;
    } else if (y >= attr->m_5c->m_2c) {
        cy = attr->m_5c->m_2c - 1;
    }
    i32 id = attr->m_5c->m_20[attr->m_5c->m_24[cy] + cx];
    i32 typeCode;
    if (id == (i32)0xeeeeeeee || id == -1) {
        typeCode = 0;
    } else {
        typeCode = attr->m_4c[id & 0xffff]->GetTypeCode(0, 0);
    }
    i32 oldFlags = cell->m_0;
    i32 keep = oldFlags & 0x1bf40000;
    i32 edgeBit = oldFlags & 0x20000000;
    // Each case writes the packed flag value straight into cell->m_0 (the retail
    // `mov [esi],imm` per body). The case GROUPS are written in retail's .text
    // body-layout order (so the jump table + byte index LUT match); the preserved
    // high bits are merged after.
    switch (typeCode - 1) {
        case 0:
            cell->m_0 = 0x1;
            break;
        case 9:
            cell->m_0 = 0x100;
            break;
        case 113:
            cell->m_0 = 0x300;
            break;
        case 35:
            cell->m_0 = 0x800;
            break;
        case 92:
        case 94:
        case 96:
        case 98:
        case 100:
        case 102:
        case 104:
            cell->m_0 = 0x4002008;
            break;
        case 29:
        case 30:
        case 32:
            cell->m_0 = 0x2021;
            break;
        case 150:
        case 151:
        case 152:
            cell->m_0 = 0x6021;
            break;
        case 149:
            cell->m_0 = 0x8000;
            break;
        case 153:
            cell->m_0 = 0x2001;
            break;
        case 107:
            cell->m_0 = 0x108;
            break;
        case 109:
            cell->m_0 = 0xa;
            break;
        case 3:
            cell->m_0 = 0x2;
            break;
        case 34:
            cell->m_0 = 0x42;
            break;
        case 33:
            cell->m_0 = 0x10000;
            break;
        case 115:
            cell->m_0 = 0x202;
            break;
        case 50:
        case 51:
        case 52:
        case 53:
        case 54:
        case 55:
        case 56:
        case 57:
        case 58:
        case 59:
        case 60:
        case 61:
        case 62:
        case 63:
        case 64:
        case 65:
            cell->m_0 = 0x4;
            break;
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
            cell->m_0 = 0x80;
            break;
        case 31:
            cell->m_0 = 0x400;
            break;
        default:
            cell->m_0 = (id3 == -1) ? 2 : 0;
            break;
    }
    if (edgeBit != 0) {
        cell->m_0 |= 0x20000000;
    }
    cell->m_0 |= keep;
    cell->m_c = id3;
    cell->m_10 = typeCode;
    // 8-neighbour edge-update walk over the 3x3 block around (x,y).
    i32 colCount = (i32)m_width;
    for (i32 r = y - 1; r <= y + 1; r++) {
        if (r < 0 || (u32)r >= (u32)m_height) {
            continue;
        }
        for (i32 c = x - 1; c <= x + 1; c++) {
            if (c < 0 || (u32)c >= (u32)m_width) {
                continue;
            }
            BrickzCell* nc = &m_rows[r][c];
            i32 nf = nc->m_0;
            if ((nf & 0x100) == 0) {
                continue;
            }
            BrickzCell* up = (r != 0) ? nc - colCount : 0;
            BrickzCell* down = (r < (i32)m_height - 1) ? nc + colCount : 0;
            BrickzCell* right = (c < colCount - 1) ? nc + 1 : 0;
            BrickzCell* left = (c != 0) ? nc - 1 : 0;
            BrickzCell* ur = (up && right) ? up + 1 : 0;
            BrickzCell* dl = (down && left) ? down - 1 : 0;
            BrickzCell* ul = (up && left) ? up - 1 : 0;
            BrickzCell* dr = (down && right) ? down + 1 : 0;
            nf &= ~0x1000;
            nc->m_0 = nf;
            if (up && down && !(up->m_0 & 0x939) && !(down->m_0 & 0x939)) {
                goto setbit;
            }
            if (right && left && !(right->m_0 & 0x939) && !(left->m_0 & 0x939)) {
                goto setbit;
            }
            if (ur && dl && !(ur->m_0 & 0x939) && !(dl->m_0 & 0x939)) {
                goto setbit;
            }
            if (ul && dr && !(ul->m_0 & 0x939) && !(dr->m_0 & 0x939)) {
            setbit:
                nc->m_0 = nf | 0x1000;
            }
        }
    }
}

// 0x077dc0 - flat cell setter: m_20[ m_24[y] + x ] = id (re-homed from
// src/Stub/BoundaryLowerMethods.cpp; CTerrainTileLoader::Load reaches this via
// loader->m_24 (BrickzAttrMgr) -> m_5c). __thiscall(x, y, id).
RVA(0x00077dc0, 0x1d)
void BrickzGridDesc::SetCell(i32 x, i32 y, i32 id) {
    m_20[m_24[y] + x] = id;
}

// ---------------------------------------------------------------------------
// 0x77df0 (RVA-homed from src/Stub/ApiCallers.cpp) - __thiscall(w): of the live,
// non-kind-0x36 cells in the 4x15 grid (skipping row w->m_1ec), pick the one nearest
// the reference tile; null it unless it lands inside the reference object's
// +/-(m_298+m_2dc+1) tile box. Tile coords are 1/32-pixel units (>>5).
// @early-stop
// regalloc wall: logic + the distance/rect math are byte-exact, but MSVC spills
// colPtr/rowPtr to the stack where retail keeps them in edi/ecx (it instead reloads
// `w` per outer iter). A spill-weight choice; the loop body matches.
struct GridSpatial_77df0 {
    char m_pad0[0x5c];
    i32 m_5c; // +0x5c x
    i32 m_60; // +0x60 y
};
struct GridCell_77df0 {
    char m_pad0[0x10];
    GridSpatial_77df0* m_10; // +0x10
    char m_pad14[0x1fc - 0x14];
    i32 m_1fc; // +0x1fc live flag
    char m_pad200[0x258 - 0x200];
    i32 m_258; // +0x258 kind
};
struct GridWorld_77df0 {
    char m_pad0[0x10];
    GridSpatial_77df0* m_10; // +0x10 reference object
    char m_pad14[0x17c - 0x14];
    i32 m_17c; // +0x17c reference x
    i32 m_180; // +0x180 reference y
    char m_pad184[0x1ec - 0x184];
    i32 m_1ec; // +0x1ec row to skip
    char m_pad1f0[0x298 - 0x1f0];
    i32 m_298; // +0x298 radius part
    char m_pad29c[0x2dc - 0x29c];
    i32 m_2dc; // +0x2dc radius part
};
struct Grid_77df0 {
    char m_pad0[0x1c];
    GridCell_77df0* m_cells[4][15]; // +0x1c (row stride 0x3c)
    GridCell_77df0* FindNearest(GridWorld_77df0* w);
};
SIZE_UNKNOWN(GridSpatial_77df0);
SIZE_UNKNOWN(GridCell_77df0);
SIZE_UNKNOWN(GridWorld_77df0);
SIZE_UNKNOWN(Grid_77df0);
RVA(0x00077df0, 0x13d)
GridCell_77df0* Grid_77df0::FindNearest(GridWorld_77df0* w) {
    GridCell_77df0* best = 0;
    i32 bestDist = 0x7fffffff;
    i32 tileX = w->m_17c >> 5;
    i32 tileY = w->m_180 >> 5;
    GridCell_77df0** rowPtr = &m_cells[0][0];
    for (i32 i = 0; i < 4; i++) {
        if (i != w->m_1ec) {
            GridCell_77df0** colPtr = rowPtr;
            i32 j = 15;
            do {
                GridCell_77df0* cell = *colPtr;
                if (cell && cell->m_1fc != 0 && cell->m_258 != 0x36) {
                    i32 dx = (cell->m_10->m_5c >> 5) - tileX;
                    i32 dy = (cell->m_10->m_60 >> 5) - tileY;
                    i32 dist = dx * dx + dy * dy;
                    if (dist < bestDist) {
                        best = cell;
                        bestDist = dist;
                    }
                }
                colPtr++;
            } while (--j != 0);
        }
        rowPtr += 15;
    }
    i32 k = w->m_298 + w->m_2dc + 1;
    i32 px = w->m_10->m_5c >> 5;
    i32 py = w->m_10->m_60 >> 5;
    RECT rc;
    rc.left = px - k;
    rc.top = py - k;
    rc.right = px + k + 1;
    rc.bottom = py + k + 1;
    if (best) {
        POINT pt;
        pt.x = best->m_10->m_5c >> 5;
        pt.y = best->m_10->m_60 >> 5;
        if (!PtInRect(&rc, pt)) {
            best = 0;
        }
    }
    return best;
}

// ---------------------------------------------------------------------------
// CBrickzGrid::SearchEdge (0x081e10) - run a Search between two adjacent cells with
// their edge state temporarily punched open, then restore. Bounds-check both
// cells (cols < m_c, rows < m_10); save cellA.m_0/m_4 + cellB.m_0/m_4 + cellB's
// 0x20000000 edge bit; clear that bit, set both cells' m_4 = -1 and (when
// clearFlag) m_0 = 0; set m_4c = maskA & 0x20000000; run Search(...,0x2000,...);
// then m_4c = 0 and restore every saved field. Returns Search's result.
// @early-stop
// regalloc wall (~82%): the body is byte-correct end to end - the combined bounds
// gate shares one return-0 tail, the cell save/punch/restore re-indexes m_8[row][col]
// per write (matching retail's factored col byte-offset), and the 8-arg Search call's
// interleaved push/reload schedule is byte-identical. The residual is that retail pins
// `this` in ebx (xA in edi) where MSVC5 here pins `this` in edi, plus a 1-slot frame
// delta (0x1c vs 0x18); the zero-pin/this-reg choice is not source-steerable.
RVA(0x00081e10, 0x1a7)
i32 CBrickzGrid::SearchEdge(
    i32 xA,
    i32 yA,
    i32 xB,
    i32 yB,
    void* list,
    i32 clearFlag,
    i32 maskA,
    i32 maskC
) {
    if ((u32)xA >= m_width || (u32)yA >= m_height || (u32)xB >= m_width || (u32)yB >= m_height) {
        return 0;
    }
    BrickzCell* cellB = &m_rows[yB][xB];
    BrickzCell* cellA = &m_rows[yA][xA];
    i32 savedB0 = cellB->m_0;
    i32 savedA4 = cellA->m_4;
    i32 savedA0 = cellA->m_0;
    i32 bBit29 = ((u32)savedB0 >> 29) & 1;
    i32 savedB4 = cellB->m_4;
    if (bBit29 != 0) {
        cellB->m_0 = savedB0 & 0xdfffffff;
    }
    // Punch the edge open: re-index m_8[row][col] for each write (retail keeps the
    // col byte-offset factored and re-reads the row base rather than caching cell).
    m_rows[yA][xA].m_4 = -1;
    m_rows[yB][xB].m_4 = -1;
    m_edgeMask = maskA & 0x20000000;
    if (clearFlag != 0) {
        m_rows[yA][xA].m_0 = 0;
        m_rows[yB][xB].m_0 = 0;
    }
    i32 ret = Search(xA, yA, xB, yB, list, maskA, 0x2000, maskC);
    m_edgeMask = 0;
    m_rows[yA][xA].m_4 = savedA4;
    m_rows[yB][xB].m_4 = savedB4;
    if (clearFlag != 0) {
        m_rows[yA][xA].m_0 = savedA0;
        m_rows[yB][xB].m_0 = savedB0;
    }
    if (bBit29 != 0) {
        m_rows[yB][xB].m_0 |= 0x20000000;
    }
    return ret;
}

// ---------------------------------------------------------------------------
// CBrickzGrid::UpdateDiagonals (0x082030) - when m_5c (dirty) is set, walk the whole
// flat cell pool and, for each cell with flag bit 0x100 set, clear its 0x1000 bit
// and re-set it if any opposite neighbour pair (UP/DOWN, RIGHT/LEFT, UR/DL, UL/DR)
// is both passable (no 0x939 bit). Clears m_5c and returns 1.
// @early-stop
// 8-neighbour spill-walk regalloc wall (~54%): logic byte-correct - the dirty gate,
// the per-cell `test ah,1` / `and ah,0xef` / `or ah,0x10` byte-flag ops, and the
// goto-cascade that sets the 0x1000 bit at one shared site all match retail. The
// residual is the shrink-wrapped callee-save push order (retail pins the cell walker
// in ebx by pushing ebx first; MSVC5 here picks ebp) and the 4 stack-slot diagonal
// neighbour layout; neither is source-steerable. Parked for the final sweep.
RVA(0x00082030, 0x1a1)
i32 CBrickzGrid::UpdateDiagonals(i32 unused) {
    BrickzCell* cell = m_cellPool;
    if (m_dirty == 0) {
        return 1;
    }
    for (u32 r = 0; r < m_height; r++) {
        for (u32 c = 0; c < m_width; c++) {
            i32 nf = cell->m_0;
            if ((nf & 0x100) != 0) {
                BrickzCell* up = (r != 0) ? cell - m_width : 0;
                BrickzCell* down = (r < m_height - 1) ? cell + m_width : 0;
                BrickzCell* right = (c < m_width - 1) ? cell + 1 : 0;
                BrickzCell* left = (c != 0) ? cell - 1 : 0;
                BrickzCell* ur = (up && right) ? up + 1 : 0;
                BrickzCell* dl = (down && left) ? down - 1 : 0;
                BrickzCell* ul = (up && left) ? up - 1 : 0;
                BrickzCell* dr = (down && right) ? down + 1 : 0;
                nf &= ~0x1000;
                cell->m_0 = nf;
                if (up && down && !(up->m_0 & 0x939) && !(down->m_0 & 0x939)) {
                    goto setbit;
                }
                if (right && left && !(right->m_0 & 0x939) && !(left->m_0 & 0x939)) {
                    goto setbit;
                }
                if (ur && dl && !(ur->m_0 & 0x939) && !(dl->m_0 & 0x939)) {
                    goto setbit;
                }
                if (ul && dr && !(ul->m_0 & 0x939) && !(dr->m_0 & 0x939)) {
                setbit:
                    cell->m_0 = nf | 0x1000;
                }
            }
            cell++;
        }
    }
    m_dirty = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CBrickzGrid::LineIsClear (0x082250) - DDA line-of-sight probe between (x0,y0)
// and (x1,y1). Pick the major axis (larger absolute delta), step the minor axis
// with a 16.16 fixed-point slope, and bail out with 0 the moment any traversed
// cell's terrain flags are non-zero; a clear line (or a zero-length segment)
// returns 1.
RVA(0x00082250, 0x17c)
i32 CBrickzGrid::LineIsClear(i32 x0, i32 y0, i32 x1, i32 y1) {
    if (x0 == x1 && y0 == y1) {
        return 1;
    }
    i32 dx = x1 - x0;
    i32 dy = y1 - y0;
    if (abs(dx) > abs(dy)) {
        i32 slope = (dy << 16) / dx;
        i32 yacc = y0 << 16;
        if (dx > 0) {
            for (i32 x = x0; x < x1; x++) {
                if (m_rows[yacc >> 16][x].m_0 != 0) {
                    return 0;
                }
                yacc += slope;
            }
        } else {
            for (i32 x = x0; x > x1; x--) {
                if (m_rows[yacc >> 16][x].m_0 != 0) {
                    return 0;
                }
                yacc += slope;
            }
        }
    } else {
        i32 slope = (dx << 16) / dy;
        i32 xacc = x0 << 16;
        if (dy > 0) {
            for (i32 y = y0; y < y1; y++) {
                if (m_rows[y][xacc >> 16].m_0 != 0) {
                    return 0;
                }
                xacc += slope;
            }
        } else {
            for (i32 y = y0; y > y1; y--) {
                if (m_rows[y][xacc >> 16].m_0 != 0) {
                    return 0;
                }
                xacc += slope;
            }
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CBrickzGrid::IsCellClear (0x0853f0) - in-bounds cell probe: return whether the
// cell at (x,y) is inside the grid AND has zero terrain flags. Out-of-bounds is
// treated as occupied (occ=1), so the shared `return occ == 0` tail is duplicated.
RVA(0x000853f0, 0x46)
i32 CBrickzGrid::IsCellClear(i32 x, i32 y) {
    i32 occ;
    if ((u32)x >= m_width || (u32)y >= m_height) {
        occ = 1;
    } else {
        occ = m_rows[y][x].m_0;
    }
    return occ == 0;
}

// ---------------------------------------------------------------------------
// CBrickzGrid::Serialize (0x09356c) - thin wrapper: MapSerializeCurve(a0,a1,a2,a3) and,
// on success, the +0x7c sub-object's Serialize(a0,a1,a2,a3); returns the latter as
// a bool. The +0x7c object is reached through the LAST arg (a3), not `this`.
// @early-stop
// arg-forwarding-via-uninitialised-callee-saved-regs wall (~38%): logic correct
// (MapSerializeCurve gate, then arg3->m_7c->Serialize forwarding the 4 args, neg/
// sbb/neg bool). The wall: retail pushes ebx/ebp/esi/edi straight as the 4 call
// args with NO loads (the forwarded values arrive in those callee-saved registers,
// separate from the stack args - arg3 IS re-read from [esp+0x10] for ->m_7c), so a
// normal __thiscall(i32,i32,i32,i32) reconstruction necessarily emits the 4
// `mov reg,[esp+N]` loads retail omits. Not reproducible from a standard signature;
// see docs/patterns/serialize-wrapper-reg-forward.md. Parked for the final sweep.
RVA(0x0009356c, 0x38)
i32 CBrickzGrid::Serialize(i32 a0, i32 a1, i32 a2, i32 a3) {
    if (MapSerializeCurve(a0, a1, a2, a3) == 0) {
        return 0;
    }
    return ((CBattlezData*)a3)->Serialize((CSerialArchive*)a0, a1, a2, a3) != 0;
}

SIZE_UNKNOWN(BrickzAttrMgr);
SIZE_UNKNOWN(BrickzButeObj);
SIZE_UNKNOWN(BrickzCell);
SIZE_UNKNOWN(BrickzGridDesc);
SIZE_UNKNOWN(BrickzNode);
SIZE_UNKNOWN(BrickzNodePoolB);
SIZE_UNKNOWN(CBrickzGrid);
