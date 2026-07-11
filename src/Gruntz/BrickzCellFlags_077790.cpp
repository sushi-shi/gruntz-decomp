// BrickzCellFlags_077790.cpp - the 0x077790 obj carved out of src/Gruntz/Brickz.cpp
// (holding-TU drain, 2026-07-11): CBrickzGrid::ComputeCellFlags (0x077790),
// BrickzGridDesc::SetCell (0x077dc0), and Grid_77df0::FindNearest (0x077df0) - one
// contiguous retail .text block, distinct from the main CBrickzGrid pathfinding obj
// (0x081e10+ in Brickz.cpp). ComputeCellFlags/SetCell are real CBrickzGrid/
// BrickzGridDesc methods; Grid_77df0 stays an @identity-TODO RVA-named view.
//
// @identity-TODO Grid_77df0 (FindNearest 0x077df0): xref (2026-07-11) - the only
// direct caller is the ILT thunk 0x253b; the caller tree resolves to ~15 CGrunt
// arrival/scan methods (ResolveArrivalReposition/ArrivalScanA-C/WanderStep/
// ChargeStep/UpdateArrival/SeekTarget/PhaseStep/StepArrivalDefense*/...). So `this`
// is a per-Grunt 4x15 spatial-neighbour grid (cells' spatial obj @+0x10 with x/y at
// +0x5c/+0x60; the GridWorld arg carries reference x/y @+0x17c/+0x180, a skip-row
// @+0x1ec and radius parts @+0x298/+0x2dc) - a distinct object, NOT CGrunt itself.
// The exact class does not crack from the caller set alone (per-call-site `this`
// typing needed); left RVA-named rather than fabricating a placeholder.
#include <rva.h>
#include <Gruntz/Brickz.h>
#include <Win32.h> // RECT/POINT + PtInRect (FindNearest)

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

SIZE_UNKNOWN(BrickzAttrMgr);
SIZE_UNKNOWN(BrickzButeObj);
SIZE_UNKNOWN(BrickzGridDesc);
