// Brickz.cpp - reconstruction of the trace-discovered "CBrickzGrid" cluster.
//
// Placeholder class name (see <Gruntz/Brickz.h>): these are __thiscall pointer-
// shuffle ops over a self-contained graph/grid container's intrusive node lists.
// They match by shape; field names are placeholders, offsets are load-bearing.
#include <rva.h>
#include <Gruntz/MapMgr.h>
#include <stdlib.h> // abs (/Oi intrinsic: |goal-cur| lowers to cdq/xor/sub, not jns)
#include <string.h> // memset (/Oi intrinsic: shr/rep stosd/and/rep stosb)

#include <Gruntz/Brickz.h>
#include <Win32.h> // RECT + IntersectRect (AllocGrid seeds the grid bounding rect)

// The pool allocator the container new's its cell pool + column table off
// (0x1b9b46, __cdecl). Modeled no-body so the call reloc-masks.
extern "C" void* __cdecl RezAlloc(u32 n); // 0x1b9b46

// The two intrusive node-pool sub-objects embedded at +0x30 and +0x3c: AllocGrid
// seeds each with count*5 nodes through its __thiscall init (reloc-masked thunks).
struct BrickzNodePoolA {
    i32 Init(i32 count); // 0x408710 (via the 0x42d7 thunk)
};

// A recycled result record off the shared free-list: m_0 = next-free link,
// m_4/m_8 = the path cell (col,row) handed to the result list.
struct BrickzFreeRec {
    i32 m_0; // +0x00  next-free link
    i32 m_4; // +0x04  path col
    i32 m_8; // +0x08  path row
};

// The intrusive free-list the search nodes recycle into when a goal node is
// reached (shared global pool; modeled extern + DATA() so the load reloc-masks).
// ?g_freeList@@3PAXA at VA 0x645544.
DATA(0x00245544)
extern BrickzFreeRec* g_brickzFreeList;

// CPtrList::AddHead (0x1b4967, __thiscall) - the list the matched search records
// are handed off to as the result path; modeled no-body so the call reloc-masks.
struct BrickzResultList {
    void* AddHead(void* node); // 0x1b4967
};

// MapSerializeCurve (0x0ec230) - declared 4-arg __cdecl here: the Serialize
// wrapper forwards all four of its args unchanged (the callee only reads the first
// two). Modeled no-body so the call reloc-masks. Same symbol as the 2-arg form in
// MapLogic.cpp; the extra params don't change the @0xec230 displacement.
extern "C" i32 __cdecl MapSerializeCurve(i32 a0, i32 a1, i32 a2, i32 a3);

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
    return ((BrickzSerObj*)a3)->Serialize(a0, a1, a2, a3) != 0;
}

// ---------------------------------------------------------------------------
// CBrickzGrid::AllocGrid (0x09ea60) - allocate + initialize the width x height grid:
// new the flat cell pool (0x1c bytes/cell) + the per-row column table, zero the
// pool, thread each row pointer, seed the two intrusive node pools (count*5
// nodes each), record the per-step callback, and compute the grid bounding rect
// (m_60) via the Win32 IntersectRect (of the {0,0,width,height} box with itself),
// from which m_70/m_74 = the rect width/height. Returns 1, or 0 on any alloc fail.
// @early-stop
// alloc/loop spill wall: logic byte-correct (the two new's + null gates, the
// inline memset, the row-pointer accumulate loop, the two pool inits, the
// IntersectRect rect build + the m_70/m_74 size compute), but the count*0x1c temp
// + the rect stack slots spill against retail's slot schedule. Parked for sweep.
RVA(0x0009ea60, 0x168)
i32 CBrickzGrid::AllocGrid(i32 width, i32 height, i32 callback) {
    i32 count = height * width;
    m_width = width;
    m_height = height;
    m_cellCount = count;
    m_cellPool = (BrickzCell*)RezAlloc(count * 0x1c);
    if (m_cellPool == 0) {
        return 0;
    }
    m_rows = (BrickzCell**)RezAlloc(height * 4);
    if (m_rows == 0) {
        return 0;
    }
    memset(m_cellPool, 0, count * 0x1c);
    i32 stride = width * 0x1c;
    i32 off = 0;
    for (i32 i = 0; i < height; i++) {
        m_rows[i] = (BrickzCell*)((char*)m_cellPool + off);
        off += stride;
    }
    if (((BrickzNodePoolA*)&m_nodePool)->Init(count * 5) == 0) {
        return 0;
    }
    if (((CMapArrayB*)((char*)this + 0x3c))->Allocate(count * 5) == 0) {
        return 0;
    }
    m_stepCb = (void (*)())callback;
    // Build the grid bounding rect: intersect the {0,0,width,height} box with
    // itself into m_60 (the {left,top,right,bottom} at +0x60); on an empty result
    // fall back to the box. m_70/m_74 = the resulting width/height.
    RECT a;
    RECT b;
    a.left = 0;
    a.top = 0;
    a.right = width;
    a.bottom = height;
    b.left = 0;
    b.top = 0;
    b.right = width;
    b.bottom = height;
    RECT* out = (RECT*)&m_originX;
    if (!IntersectRect(out, &a, &b)) {
        *out = a;
    }
    m_gridW = out->right - out->left;
    m_gridH = out->bottom - out->top;
    return 1;
}

// ---------------------------------------------------------------------------
// CBrickzGrid::Search (0x09eca0) - the A*-style grid search driver. Bounds-check the
// start (x1,y1) and goal (x2,y2) against the grid origin (m_60,m_64) / size
// (m_70,m_74); reject if the start cell fails the passability masks; reset the
// per-cell open counts; seed the open list with the start record and expand
// neighbours via Expand until the open list empties or the goal is popped. On
// success the result path is recycled to g_brickzFreeList + handed to `list`.
// @early-stop
// regalloc wall, ~91%: logic byte-correct (BFS loop + path-walk + free-list pop
// all match). Residual is the opening bounds-gate's callee-saved assignment
// (retail pins x1 in ebx for the whole fn, reused at m_20=x1) and the cell-clear
// loop's reg/zero choice; no source spelling flips MSVC5's allocator here.
RVA(0x0009eca0, 0x2bd)
i32 CBrickzGrid::Search(
    i32 x1,
    i32 y1,
    i32 x2,
    i32 y2,
    void* list,
    i32 maskA,
    i32 maskB,
    i32 maskC
) {
    i32 ox = m_originX;
    if ((u32)(x1 - ox) >= (u32)m_gridW) {
        return 0;
    }
    i32 oy = m_originY;
    i32 hgt = m_gridH;
    if ((u32)(y1 - oy) >= (u32)hgt) {
        return 0;
    }
    if ((u32)(x2 - ox) >= (u32)m_gridW) {
        return 0;
    }
    if ((u32)(y2 - oy) >= (u32)hgt) {
        return 0;
    }
    m_maskC = maskC;
    m_maskB = maskB;
    m_maskA = maskA;
    i32 flags = *(i32*)&m_rows[y2][x2];
    if ((maskA & flags) != 0 && (maskC & flags) != 0) {
        return 0;
    }
    // Reset the per-cell open counts across all m_14 cells.
    if (m_cellCount != 0) {
        u32 i = 0;
        i32 off = 0;
        do {
            *(i32*)((char*)m_cellPool + off + 0x14) = 0;
            i++;
            off += 0x1c;
        } while (i < m_cellCount);
    }
    if (x1 == x2 && y1 == y2) {
        return 1;
    }
    m_goalX = x2;
    m_startX = x1;
    m_goalY = y2;
    m_startY = y1;
    // Pop the seed record off the closed (m_30) list head.
    BrickzNode* seed = m_nodePool;
    BrickzNode* slot = seed->m_14;
    if (slot == 0) {
        m_nodePool = 0;
    } else {
        m_nodePool = slot;
        slot->m_18 = 0;
    }
    if (seed == 0) {
        return 0;
    }
    seed->m_0 = x1;
    seed->m_4 = y1;
    seed->m_8 = 0;
    i32 dx = abs(m_goalY - y1);
    i32 dxx = abs(m_goalX - x1);
    i32 h = (dx + dxx) * 2;
    seed->m_c = h;
    seed->m_10 = h;
    seed->m_14 = 0;
    seed->m_18 = 0;
    seed->m_1c = 0;
    Insert(seed);
    (&m_rows[y1][x1])->m_count++;
    BrickzNode* node = 0;
    while (m_openList != 0) {
        node = PopFront();
        (&m_rows[node->m_4][node->m_0])->m_count--;
        if (node->m_0 == m_goalX && node->m_4 == m_goalY) {
            goto reached;
        }
        Expand(node, 0, 1, 2, 0);
        Expand(node, 1, 0, 2, 0);
        Expand(node, 0, -1, 2, 0);
        Expand(node, -1, 0, 2, 0);
        Expand(node, 1, 1, 3, 1);
        Expand(node, 1, -1, 3, 1);
        Expand(node, -1, -1, 3, 1);
        Expand(node, -1, 1, 3, 1);
        CellPush(node);
    }
    node = 0;
    Drain();
    Reset();
    if (m_stepCb != 0) {
        m_stepCb();
    }
    return 0;

reached:
    BrickzNode* p = node;
    do {
        BrickzFreeRec* rec = (BrickzFreeRec*)g_brickzFreeList;
        i32* slot = 0;
        if (rec->m_0 != 0) {
            slot = &rec->m_4;
            rec->m_4 = p->m_0;
            rec->m_8 = p->m_4;
            g_brickzFreeList = (BrickzFreeRec*)rec->m_0;
        }
        ((BrickzResultList*)list)->AddHead(slot);
        p = (BrickzNode*)p->m_1c;
    } while (p != 0);
    if (m_stepCb != 0) {
        m_stepCb();
    }
    node->m_18 = 0;
    node->m_14 = m_nodePool;
    m_nodePool->m_18 = node;
    m_nodePool = node;
    Drain();
    Reset();
    return 1;
}

// ---------------------------------------------------------------------------
// CBrickzGrid::Expand (0x09f010) - relax one neighbour of `node` in the (dx,dy)
// direction. Bounds-check the target cell, reject it on the passability masks
// (m_4c / m_50&m_54), and for a diagonal step (diag) reject corner-cutting via
// the two orthogonal cells (m_58). If the cell is unvisited / improvable, take a
// search record (reusing an open Find()ed record, a closed CellPop'd record, or
// a fresh one off the m_30 pool), set its g = node->g + cost and f = g + h
// (h = 2*(|gx-ncol| + |gy-nrow|)), parent = node, and Insert() it. Returns 1
// (the open-list is unchanged only when out of records => 0).
// @early-stop
// spill-scheduling + sibling-retest wall, ~79%: logic byte-correct (bounds gate,
// 4-quadrant corner check, open/closed-record relax, heuristic abs() all match).
// Residual is the prologue's local-spill count (retail spills ng/ncol/nrow to 4
// stack slots; MSVC5 rematerializes into 2) + the redundant open-node re-tests;
// neither flips with a local-pin here. Parked for the final sweep.
RVA(0x0009f010, 0x2a1)
i32 CBrickzGrid::Expand(BrickzNode* node, i32 dx, i32 dy, i32 cost, i32 diag) {
    i32 ng = (i32)node->m_8 + cost;
    i32 ncol = node->m_0 + dx;
    i32 nrow = node->m_4 + dy;
    BrickzNode* found0 = 0;
    if ((u32)(ncol - m_originX) >= (u32)m_gridW) {
        return 1;
    }
    if ((u32)(nrow - m_originY) >= (u32)m_gridH) {
        return 1;
    }
    i32* ncell = (i32*)&m_rows[nrow][ncol];
    i32 nflags = *ncell;
    i32* cell = (i32*)&m_rows[node->m_4][node->m_0];
    if ((m_edgeMask & nflags) != 0) {
        return 1;
    }
    if ((m_maskA & nflags) != 0 && (m_maskC & nflags) == 0) {
        return 1;
    }
    if (diag != 0 && m_maskB != 0) {
        i32 *cellA, *cellB;
        if (dx > 0 && dy > 0) {
            cellB = cell + (m_width * 7);
            cellA = cell + 7;
        } else if (dx < 0 && dy > 0) {
            cellB = cell + (m_width * 7);
            cellA = cell - 7;
        } else if (dx > 0 && dy < 0) {
            cellB = cell - (m_width * 7);
            cellA = cell + 7;
        } else if (dx < 0 && dy < 0) {
            cellB = cell - (m_width * 7);
            cellA = cell - 7;
        } else {
            goto relax;
        }
        if ((m_maskB & *cellA) != 0 || (m_maskB & *cellB) != 0) {
            return 1;
        }
    }
relax:
    BrickzNode* closed = 0;
    BrickzNode* head = ((BrickzCell*)ncell)->m_head;
    if (head != 0) {
        closed = (BrickzNode*)head->m_0;
    }
    if (closed != 0) {
        if (ng >= (i32)closed->m_8) {
            return 1;
        }
    }
    BrickzNode* open;
    if (((BrickzCell*)ncell)->m_count != 0) {
        open = Find(ncol, nrow);
    } else {
        open = found0;
    }
    if (open != 0) {
        i32 og = (i32)open->m_8;
        if (ng >= og) {
            return 1;
        }
        if (open != 0 && ng < og) {
            if (closed != 0) {
                CellPop(closed, 1);
            }
            Unlink(open);
            open->m_10 = ng + open->m_c;
            open->m_1c = (i32)node;
            open->m_8 = (BrickzNode*)ng;
            Insert(open);
            return 1;
        }
    }
    if (closed != 0) {
        if (ng < (i32)closed->m_8) {
            CellPop(closed, 0);
            closed->m_1c = (i32)node;
            closed->m_8 = (BrickzNode*)ng;
            closed->m_10 = closed->m_c + ng;
            Insert(closed);
            ((BrickzCell*)ncell)->m_count++;
            return 1;
        }
        CellPop(closed, 1);
    }
    if (open != 0) {
        return 1;
    }
    BrickzNode* rec = m_nodePool;
    BrickzNode* nx = rec->m_14;
    if (nx == 0) {
        rec = 0;
    } else {
        m_nodePool = nx;
        nx->m_18 = 0;
    }
    if (rec == 0) {
        return 0;
    }
    rec->m_0 = ncol;
    rec->m_4 = nrow;
    rec->m_8 = (BrickzNode*)ng;
    i32 hy = abs(m_goalY - nrow);
    i32 hx = abs(m_goalX - ncol);
    i32 h = (hy + hx) * 2;
    rec->m_1c = (i32)node;
    rec->m_c = h;
    rec->m_10 = ng + h;
    rec->m_14 = 0;
    rec->m_18 = 0;
    rec->m_20 = 0;
    Insert(rec);
    ((BrickzCell*)ncell)->m_count++;
    return 1;
}

// ---------------------------------------------------------------------------
// CBrickzGrid::Insert - insert node into the m_18-headed list, kept ascending by
// m_10. Links: m_14 = forward (next), m_18 = backward (prev). Always returns 1.
RVA(0x0009f370, 0x8a)
i32 CBrickzGrid::Insert(BrickzNode* node) {
    BrickzNode* cur = m_openList;
    node->m_18 = 0;
    node->m_14 = 0;
    if (cur == 0) {
        m_openList = node;
        return 1;
    }
    i32 key = node->m_10;
    while (cur != 0) {
        if (key < cur->m_10) {
            if (cur->m_18 != 0) {
                node->m_18 = cur->m_18;
                node->m_14 = cur;
                cur->m_18->m_14 = node;
                cur->m_18 = node;
            } else {
                m_openList = node;
                node->m_14 = cur;
                cur->m_18 = node;
            }
            return 1;
        }
        if (cur->m_14 == 0) {
            cur->m_14 = node;
            node->m_18 = cur;
            return 1;
        }
        cur = cur->m_14;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CBrickzGrid::PopFront - detach the head of the m_18 list; promote its m_14
// successor (clearing the successor's back-link) and clear the popped links.
// Returns the popped head (eax, consumed by Search). The return type does not
// affect the callee's own bytes - head is already materialized in eax.
// @early-stop
// regalloc wall: only residual is a head<->next register swap (retail pins head
// in eax, recompile lands it in edx); logic byte-correct, 97% (no source steer).
RVA(0x0009f430, 0x2a)
BrickzNode* CBrickzGrid::PopFront() {
    BrickzNode* head = m_openList;
    if (head != 0) {
        BrickzNode* next = head->m_14;
        if (next != 0) {
            m_openList = next;
            next->m_18 = 0;
        } else {
            m_openList = 0;
        }
        head->m_14 = 0;
        head->m_18 = 0;
    }
    return head;
}

// ---------------------------------------------------------------------------
// CBrickzGrid::Unlink - remove node from the m_18-headed doubly-linked list
// (m_14 = next, m_18 = prev), repairing the neighbours and the head, then
// clearing the node's links.
// @early-stop
// sibling-guard-retest + regalloc wall: retail keeps redundant `cmp prev,0`
// re-tests (4-way dispatch from a sequential-if source) and uses 2 callee-saved
// regs; with no calls to pin the flag MSVC5 folds the re-tests + uses 1 reg.
// Logic byte-correct, ~75%.
RVA(0x0009f690, 0x5d)
void CBrickzGrid::Unlink(BrickzNode* node) {
    if (node->m_18 != 0) {
        if (node->m_14 != 0) {
            node->m_18->m_14 = node->m_14;
            node->m_14->m_18 = node->m_18;
        }
    } else if (node->m_14 == 0) {
        m_openList = 0;
    } else if (node->m_18 == 0) {
        BrickzNode* next = node->m_14;
        if (next != 0) {
            m_openList = next;
            next->m_18 = 0;
        }
    }
    if (node->m_18 != 0 && node->m_14 == 0) {
        node->m_18->m_14 = 0;
    }
    node->m_18 = 0;
    node->m_14 = 0;
}

// ---------------------------------------------------------------------------
// CBrickzGrid::CellPush - allocate a bucket node from the m_40 free list and link it
// into the grid cell m_8[node->m_4][node->m_0]; record the slot in node->m_20.
// @early-stop
// regalloc/scheduling wall: branch shape + free-list pop byte-match; only the
// arg-pointer register (retail defers the `node` load past the 3 pushes -> edi;
// recompile loads it pre-push -> edx) and the dependent reg chain differ, ~86%.
RVA(0x0009f470, 0x62)
void CBrickzGrid::CellPush(BrickzNode* node) {
    BrickzNode** head = &m_rows[node->m_4][node->m_0].m_head;
    BrickzNode* slot = m_freeList;
    BrickzNode* nx = slot->m_8;
    if (nx == 0) {
        slot = 0;
    } else {
        m_freeList = nx;
        nx->m_4 = 0;
    }
    BrickzNode* old = *head;
    if (old == 0) {
        *head = slot;
        slot->m_4 = 0;
        slot->m_8 = 0;
        slot->m_0 = (i32)node;
        node->m_20 = slot;
    } else {
        slot->m_4 = (i32)old;
        slot->m_8 = (*head)->m_8;
        *head = slot;
        node->m_20 = slot;
    }
}

// ---------------------------------------------------------------------------
// CBrickzGrid::Reset - empty every grid cell: each bucket node's child (m_0) is
// pushed onto the m_30 active list and the bucket node itself onto the m_40
// free list; then the cell head is cleared.
// @early-stop
// regalloc/addressing wall (same family as Drain): retail materializes
// &node->m_8 as a callee-saved base ptr (lea + extra reg) and commutes the
// m_10*m_c imul operand order; recompile uses direct offsets. Logic
// byte-correct (loop structure + unsigned counter match), ~65%.
RVA(0x0009f5d0, 0x81)
void CBrickzGrid::Reset() {
    BrickzCell* cell = m_cellPool;
    for (u32 i = 0; i < m_height * m_width; i++) {
        BrickzNode* node = cell->m_head;
        while (node != 0) {
            BrickzNode** link = &node->m_8;
            BrickzNode* next = *link;
            BrickzNode* child = (BrickzNode*)node->m_0;
            child->m_14 = m_nodePool;
            child->m_18 = 0;
            m_nodePool->m_18 = child;
            m_nodePool = child;
            node->m_4 = 0;
            *link = m_freeList;
            m_freeList->m_4 = (i32)node;
            m_freeList = node;
            node = next;
        }
        cell->m_head = 0;
        cell++;
    }
}

// ---------------------------------------------------------------------------
// CBrickzGrid::CellPop - remove node's bucket slot (node->m_20) from its grid cell's
// doubly-linked bucket list (m_4 = prev, m_8 = next), clear node's links, return
// the slot to the m_40 free list, and (if flag) push node onto the m_30 list.
// @early-stop
// sibling-guard-retest wall (same as Unlink): the 4-way prev/next dispatch keeps
// redundant `cmp prev,0` re-tests in retail that MSVC5 folds with no call to pin
// the flag. Logic byte-correct, container shape proven; parked for the sweep.
RVA(0x0009f710, 0xa7)
void CBrickzGrid::CellPop(BrickzNode* node, i32 flag) {
    BrickzNode** head = &m_rows[node->m_4][node->m_0].m_head;
    BrickzNode* slot = node->m_20;
    if ((BrickzNode*)slot->m_4 != 0) {
        if (slot->m_8 != 0) {
            ((BrickzNode*)slot->m_4)->m_8 = slot->m_8;
            slot->m_8->m_4 = slot->m_4;
        }
    } else if (slot->m_8 == 0) {
        *head = 0;
    } else if ((BrickzNode*)slot->m_4 == 0) {
        BrickzNode* next = slot->m_8;
        if (next != 0) {
            *head = next;
            next->m_4 = 0;
        }
    }
    if ((BrickzNode*)slot->m_4 != 0 && slot->m_8 == 0) {
        ((BrickzNode*)slot->m_4)->m_8 = 0;
    }
    node->m_18 = 0;
    node->m_14 = 0;
    node->m_20 = 0;
    slot->m_8 = m_freeList;
    slot->m_4 = 0;
    m_freeList->m_4 = (i32)slot;
    m_freeList = slot;
    if (flag != 0) {
        node->m_18 = 0;
        node->m_14 = m_nodePool;
        m_nodePool->m_18 = node;
        m_nodePool = node;
    }
}

// ---------------------------------------------------------------------------
// CBrickzGrid::Find - walk the m_18 lookup list (linked via m_14), return the node
// whose (m_0,m_4) pair equals (key1,key2); 0 if absent.
RVA(0x0009f500, 0x24)
BrickzNode* CBrickzGrid::Find(i32 key1, i32 key2) {
    BrickzNode* p = m_openList;
    if (p == 0) {
        return 0;
    }
    do {
        if (p->m_0 == key1 && p->m_4 == key2) {
            return p;
        }
        p = p->m_14;
    } while (p != 0);
    return 0;
}

// ---------------------------------------------------------------------------
// CBrickzGrid::Drain - move every node off the m_18 list onto the front of the m_30
// list (re-threaded via m_14/m_18), then clear the m_18 head.
// @early-stop
// regalloc wall: retail materializes &node->m_14 as a base ptr in a callee-saved
// reg (lea + 3 pushes); recompile uses a direct offset + 2 pushes. Logic
// byte-correct, ~67% (no source spelling forces the 3rd reg / lea base).
RVA(0x0009f590, 0x2f)
void CBrickzGrid::Drain() {
    BrickzNode* p = m_openList;
    if (p != 0) {
        do {
            BrickzNode* next = p->m_14;
            p->m_14 = m_nodePool;
            p->m_18 = 0;
            m_nodePool->m_18 = p;
            m_nodePool = p;
            p = next;
        } while (p != 0);
    }
    m_openList = 0;
}

SIZE_UNKNOWN(BrickzAttrMgr);
SIZE_UNKNOWN(BrickzButeObj);
SIZE_UNKNOWN(BrickzCell);
SIZE_UNKNOWN(BrickzFreeRec);
SIZE_UNKNOWN(BrickzGridDesc);
SIZE_UNKNOWN(BrickzNode);
SIZE_UNKNOWN(BrickzNodePoolA);
SIZE_UNKNOWN(BrickzNodePoolB);
SIZE_UNKNOWN(BrickzResultList);
SIZE_UNKNOWN(BrickzSerObj);
SIZE_UNKNOWN(CBrickzGrid);
