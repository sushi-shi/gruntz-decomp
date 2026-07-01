// GroupOps.cpp - two selection/group operations that hang off the g_gameReg
// singleton (?g_gameReg @0x64556c):
//
//   * CenterOnGroup (0x7cf40) - walk a selection list (+0x244), accumulate the
//     bounding box of the selected gruntz over the map grid (+0x1c), centre the
//     view on the box midpoint, and (single selection) latch the picked cell.
//   * Broadcast (0x112080) - iterate a 0-terminated 24-entry key array (+0x2c),
//     look each key up in the +0x24 map, and for each matching member of the
//     resolved node's inner list run its destroy slot.
//
// Field names are placeholders; only the OFFSETS, the grid-hash (x*15 + y), the
// vtable SLOT offsets and the diagnostic ids are load-bearing.  The deeper engine
// leaves (map lookups, the centre helper, the diagnostics sink) are external /
// reloc-masked.
#include <rva.h>
#include <Gruntz/CGameRegistry.h>

// ===========================================================================
// CenterOnGroup (0x7cf40)
// ===========================================================================
// The view-centre helper reached as g_gameReg->m_2c->Center(x, y) (0x2e28 thunk).
struct CCenterTarget {
    i32 Center(i32 x, i32 y); // 0x2e28
};
// The map dimensions: gameReg->m_30->m_24->m_5c -> {width @0x30, height @0x34}.
struct CMapDims {
    char m_pad00[0x30];
    i32 m_30; // +0x30 width
    i32 m_34; // +0x34 height
};
struct CMapHolderB {
    char m_pad00[0x5c];
    CMapDims* m_5c; // +0x5c
};
struct CMapHolderA {
    char m_pad00[0x24];
    CMapHolderB* m_24; // +0x24
};
DATA(0x0024556c)
extern CGameRegistry* g_gameRegSel; // 0x64556c

// A selected cell's grunt (cell->m_10) carries its tile position at +0x5c/+0x60.
struct CSelGrunt {
    char m_pad00[0x5c];
    i32 m_5c; // +0x5c x
    i32 m_60; // +0x60 y
};
// A grid cell: +0x10 the grunt, +0x1ec/+0x1f0 the latch values.
struct CSelGridCell {
    char m_pad00[0x10];
    CSelGrunt* m_10; // +0x10
    char m_pad14[0x1ec - 0x14];
    i32 m_1ec; // +0x1ec
    i32 m_1f0; // +0x1f0
};
// A selection list node: next @0, the cell-key object @0x08.
struct CSelKey {
    i32 m_0; // +0x00 grid x-ish
    i32 m_4; // +0x04 grid y-ish
};
struct CSelNode {
    CSelNode* m_next; // +0x00
    void* m_pad04;
    CSelKey* m_8; // +0x08
};

struct CGroupSel {
    char m_pad00[0x1c];
    CSelGridCell* m_grid[1]; // +0x1c  grid cell pointer table (indexed by x*15 + y)
    // ... the latch / state fields below live well past the grid; reached by
    // offset so the modeled array stays size-1.
    i32 CenterOnGroup(i32 doSelect); // 0x7cf40
    i32 TrySelect(i32 a, i32 b);     // 0x33aa
    void Commit();                   // 0x3d1e
};

// @early-stop
// 83% - regalloc wall: the list walk, grid-hash (x*15 + y), bounding-box min/max
// fold, midpoint centre call and single-selection latch are byte-faithful; the
// residual is min/max register colouring + the doubled grid-lookup spill.  No EH.
RVA(0x0007cf40, 0x12e)
i32 CGroupSel::CenterOnGroup(i32 doSelect) {
    CSelNode* n = *(CSelNode**)((char*)this + 0x244);
    if (n == 0) {
        return 0;
    }
    CMapDims* dims = (CMapDims*)g_gameRegSel->m_30->m_24->m_5c;
    i32 minX = dims->m_30 - 1;
    i32 minY = dims->m_34 - 1;
    i32 maxX = 0;
    i32 maxY = 0;
    i32 count = 0;
    do {
        CSelKey* k = n->m_8;
        n = n->m_next;
        CSelGridCell* cell = m_grid[k->m_0 * 15 + k->m_4];
        if (cell != 0) {
            count++;
            CSelGrunt* g = cell->m_10;
            i32 gx = g->m_5c;
            i32 gy = g->m_60;
            if (gx < minX) {
                minX = gx;
            }
            if (gx > maxX) {
                maxX = gx;
            }
            if (gy < minY) {
                minY = gy;
            }
            if (gy > maxY) {
                maxY = gy;
            }
        }
    } while (n != 0);
    i32 cy = minY + (maxY - minY) / 2;
    i32 cx = minX + (maxX - minX) / 2;
    i32 r = ((CCenterTarget*)g_gameRegSel->m_2c)->Center(cx, cy);
    if (r != 0 && count == 1 && *(i32*)((char*)this + 0x24c) == 1) {
        CSelKey* head = (*(CSelNode**)((char*)this + 0x244))->m_8;
        CSelGridCell* cell2 = m_grid[head->m_0 * 15 + head->m_4];
        if (cell2 != 0) {
            i32 v1f0 = cell2->m_1f0;
            i32 v1ec = cell2->m_1ec;
            if (TrySelect(v1ec, v1f0)) {
                *(i32*)((char*)this + 0x234) = v1ec;
                *(i32*)((char*)this + 0x238) = v1f0;
                *(i32*)((char*)this + 0x230) = 1;
                Commit();
            }
        }
    }
    return 1;
}

// ===========================================================================
// Broadcast (0x112080)
// ===========================================================================
// The diagnostics sink reached as g_gameReg->Report(id, line) (0x346d thunk).
DATA(0x0024556c)
extern CGameRegistry* g_gameRegDiag; // 0x64556c

// A resolved map node: virtual prepare at slot +0x0c, +0x10 the key, +0x14 a flag.
struct CFindNode {
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c(); // +0x0c prepare
    char m_pad10[0x10 - 0x04];
    i32 m_10; // +0x10 key
    i32 m_14; // +0x14 flag
};
// An inner-list member: virtual destroy at slot 0, non-virtual Match (0x1fa5).
struct CBcastMember {
    virtual void v00(); // +0x00 destroy
    i32 Match(i32 key); // 0x1fa5
};
struct CBcastListNode {
    CBcastListNode* m_next; // +0x00
    void* m_pad04;
    CBcastMember* m_8; // +0x08
};
struct CBcastMap {
    char m_pad00[0x20];
    CBcastListNode* m_20;            // +0x20 inner list head
    CFindNode* Find(i32 key, i32 n); // 0x1c21
};

struct CGroupBroadcast {
    char m_pad00[0x10];
    i32 m_10; // +0x10  compared with each node's key
    char m_pad14[0x24 - 0x14];
    CBcastMap* m_24; // +0x24
    char m_pad28[0x2c - 0x28];
    i32 m_2c[0x18];  // +0x2c  0-terminated key array
    i32 Broadcast(); // 0x112080
    void Init();     // 0x2e0f
};

// @early-stop
// 84% - regalloc wall: the 0-terminated key-array walk, per-key map Find, the
// inner match/destroy list loop and both diagnostic exits are byte-faithful; the
// residual is loop-induction / counter register colouring.  No EH frame.
RVA(0x00112080, 0x138)
i32 CGroupBroadcast::Broadcast() {
    Init();
    i32 counter = 0;
    i32* p = &m_2c[0];
    i32 i = 0;
    i32 done = 0;
    do {
        if (i >= 0x18) {
            return 1;
        }
        CFindNode* node = m_24->Find(*p, 4);
        if (node == 0) {
            g_gameRegDiag->Report(0x80dd, 0x44f);
            return 0;
        }
        if (node->m_10 != m_10 && node->m_14 != 0) {
            node->v0c();
            i32 any = 0;
            for (CBcastListNode* it = m_24->m_20; it != 0; it = it->m_next) {
                CBcastMember* o = it->m_8;
                if (o != 0 && o->Match(node->m_10)) {
                    o->v00();
                    counter++;
                    any = 1;
                }
            }
            if (any == 0) {
                g_gameRegDiag->Report(0x80de, 0x450);
                return 0;
            }
        }
        i32 next = p[1];
        p++;
        i++;
        if (next == 0) {
            done = 1;
        }
    } while (!done);
    return 1;
}

SIZE_UNKNOWN(CCenterTarget);
SIZE_UNKNOWN(CMapHolderB);
SIZE_UNKNOWN(CMapHolderA);
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(CSelGrunt);
SIZE_UNKNOWN(CSelGridCell);
SIZE_UNKNOWN(CSelKey);
SIZE_UNKNOWN(CSelNode);
SIZE_UNKNOWN(CGroupSel);
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(CFindNode);
SIZE_UNKNOWN(CBcastMember);
SIZE_UNKNOWN(CBcastListNode);
SIZE_UNKNOWN(CBcastMap);
SIZE_UNKNOWN(CGroupBroadcast);
