// Brickz.h - trace-discovered "CBrickz" cluster (placeholder name).
//
// The trace tool (gruntz.analysis.this_cluster) grouped these __thiscall methods
// by a shared `this`/ecx pointer and labeled them CBrickz. The RTTI class actually
// reached through the 0x113c0 ctor's vftable (0x5e705c) is CUserLogic, and the
// 0x9f* run operates on a SELF-CONTAINED graph/grid container sub-object with a
// different `this` shape (links via +0x14/+0x18, free/active lists at +0x30/+0x40,
// a column table at +0x08 and a width/height at +0x0c/+0x10). The true class name
// is undetermined; only the OFFSETS + code bytes are load-bearing here.
//
// These methods are pure pointer-shuffle over the container's node graph, so they
// match by shape independent of the (placeholder) class name. We model the node
// and container faithfully from the field stores; field names are placeholders.
#ifndef GRUNTZ_BRICKZ_H
#define GRUNTZ_BRICKZ_H

#include <Ints.h>

// A graph/list node. The container threads nodes through several intrusive links;
// the same physical node is reached as different "views" depending on the list.
// In the A*-style search (Search/Expand) it is also a search record: m_0/m_4 are
// the cell (col,row), m_8 the accumulated cost g, m_c the priority key, m_10 the
// total f, m_14/m_18 the open-list links, m_1c the back-pointer payload, m_20 the
// parent-cell back-pointer.
struct BrickzNode {
    // m_0/m_4 are a (key1,key2) pair when the node is in the lookup list, and a
    // (child-ptr, free-list back-ptr) pair when the node is a cell-bucket node;
    // typed int here (Find compares them as ints), reinterpreted in Reset.
    i32 m_0;          // +0x00  key1 / col / child ptr
    i32 m_4;          // +0x04  key2 / row / back-link (free list)
    BrickzNode* m_8;  // +0x08  fwd-link (free/active list) / bucket next / g cost
    i32 m_c;          // +0x0c  priority key
    i32 m_10;         // +0x10  sort key / total f
    BrickzNode* m_14; // +0x14  list link A (prev / next)
    BrickzNode* m_18; // +0x18  list link B (next / prev)
    i32 m_1c;         // +0x1c  payload (search data)
    BrickzNode* m_20; // +0x20  owning bucket-node / parent back-pointer
};

// A grid cell occupies 0x1c bytes. +0x00 holds the packed terrain flags (the
// 0x939 passability mask, the 0x1000 diagonal-pass bit, the 0x20000000 edge bit),
// +0x04 a per-cell edge/id payload; the per-cell open-count sits at +0x14 and the
// bucket-list head at +0x18.
struct BrickzCell {
    i32 m_0; // +0x00  packed terrain flags
    i32 m_4; // +0x04  per-cell edge/id payload
    char m_pad8[0x0c - 0x08];
    i32 m_c;            // +0x0c  id3 payload (ComputeCellFlags)
    i32 m_10;           // +0x10  bute type code (ComputeCellFlags)
    i32 m_count;        // +0x14  per-cell open-list reference count
    BrickzNode* m_head; // +0x18  bucket-list head
};

// A bute terrain object: ComputeCellFlags reads its type code through its own
// vtable (slot +0x20, __thiscall(i32,i32)). Modeled polymorphic (the touched slot
// only) so `mov edx,[obj]; call [edx+0x20]` falls out, no manual vtable cast.
struct BrickzButeObj {
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual i32 GetTypeCode(i32 a, i32 b); // +0x20  returns the 1-based type id
};

// The terrain grid descriptor reached as attr->m_5c: a flat cell-id table (m_20)
// indexed by a per-row offset table (m_24), with width m_28 / height m_2c.
struct BrickzGridDesc {
    char m_pad0[0x20];
    i32* m_20; // +0x20  flat id table (id = m_20[ rowTab[y] + x ])
    i32* m_24; // +0x24  per-row base-offset table
    i32 m_28;  // +0x28  width  (x clamp)
    i32 m_2c;  // +0x2c  height (y clamp)
};

// The attribute/bute-type manager reached as this->m_78->m_24. Holds the grid
// descriptor at +0x5c and the bute-object pointer array at +0x4c.
struct BrickzAttrMgr {
    char m_pad0[0x24];
    BrickzAttrMgr* m_24; // +0x24  self/inner (chained: m_78->m_24)
    char m_pad28[0x4c - 0x28];
    BrickzButeObj** m_4c; // +0x4c  bute-object pointer array (indexed by id&0xffff)
    char m_pad50[0x5c - 0x50];
    BrickzGridDesc* m_5c; // +0x5c  the terrain grid descriptor
};

// The +0x7c serialize sub-object the Serialize wrapper dispatches to. Modeled with
// a no-body Serialize so the `mov ecx,[arg3+0x7c]; call` reloc-masks.
struct BrickzSerObj {
    i32 Serialize(i32 a0, i32 a1, i32 a2, i32 a3); // 0x0fd3f0 (__thiscall, ret 0x10)
};

class CBrickz {
public:
    // ---- reconstructed in src/Gruntz/Brickz.cpp ----
    // The terrain-grid cell-flag compute (0x077790): look up the bute-type id for
    // cell (x,y) via the m_78 attribute manager, switch it to a packed flag value,
    // OR in the preserved high bits, then run the 8-neighbour edge-update walk over
    // the 3x3 block. __thiscall(x, y, id3) ; ret 0xc.
    void ComputeCellFlags(i32 x, i32 y, i32 id3); // 0x077790

    // The grid allocator/initializer (0x09ea60): allocate the width*height cell
    // pool + the per-row column table, zero the pool, thread the two node pools,
    // record the per-step callback, and seed the grid bounding rect (width x
    // height) via the Win32 IntersectRect. __thiscall(width, height, cb) ; ret 0xc.
    // (Ghidra labeled it "winapi_09ea60_IntersectRect" off that one import.)
    i32 AllocGrid(i32 width, i32 height, i32 callback); // 0x09ea60

    // The edge-search op (0x081e10): temporarily punch a passage between the two
    // cells (xA,yA)/(xB,yB), run Search() between them, then restore the cells.
    // __thiscall(8 args) ; ret 0x20.
    i32 SearchEdge(
        i32 xA,
        i32 yA,
        i32 xB,
        i32 yB,
        void* list,
        i32 clearFlag,
        i32 maskA,
        i32 maskC
    ); // 0x081e10

    // The diagonal-passability flag walk (0x082030): when m_5c (dirty) is set, walk
    // the whole flat cell pool and, for each cell with bit 0x100 set, set bit 0x1000
    // if any opposite neighbour pair is both passable (no 0x939 bit). __thiscall(1
    // unused arg) ; ret 0x4. Returns 1.
    i32 UpdateDiagonals(i32 unused); // 0x082030

    // The thin Serialize wrapper (0x09356c): MapSerializeCurve(...) then, on success,
    // the +0x7c sub-object's Serialize(...). __thiscall(4 args) ; ret 0x10.
    i32 Serialize(i32 a0, i32 a1, i32 a2, i32 a3); // 0x09356c

    i32 Search(
        i32 x1,
        i32 y1,
        i32 x2,
        i32 y2,
        void* list,
        i32 maskA,
        i32 maskB,
        i32 maskC
    );                                                                // 0x09eca0
    i32 Expand(BrickzNode* node, i32 dx, i32 dy, i32 cost, i32 diag); // 0x09f010
    i32 Insert(BrickzNode* node);                                     // 0x09f370
    BrickzNode* PopFront();                                           // 0x09f430
    void CellPush(BrickzNode* node);                                  // 0x09f470
    BrickzNode* Find(i32 key1, i32 key2);                             // 0x09f500
    void Drain();                                                     // 0x09f590
    void Unlink(BrickzNode* node);                                    // 0x09f690
    void CellPop(BrickzNode* node, i32 flag);                         // 0x09f710
    void Reset();                                                     // 0x09f5d0

    // container fields (offsets recovered from the field stores above)
    char m_pad0[0x04];
    BrickzCell* m_cellPool; // +0x04  flat cell pool (width*height cells)
    BrickzCell** m_rows;    // +0x08  row table (m_rows[row] -> cell row base)
    u32 m_width;            // +0x0c  grid width (columns; AllocGrid: m_c = width)
    u32 m_height;           // +0x10  grid height (rows; AllocGrid: m_10 = height)
    u32 m_cellCount;        // +0x14  total cell count (width*height)
    BrickzNode* m_openList; // +0x18  sorted open/lookup list head (ascending by BrickzNode::m_10 f)
    i32 m_1c;               // +0x1c  (container scratch; unused in this TU)
    i32 m_startX;           // +0x20  search start x1
    i32 m_startY;           // +0x24  search start y1
    i32 m_goalX;            // +0x28  search goal x2
    i32 m_goalY;            // +0x2c  search goal y2
    BrickzNode* m_nodePool; // +0x30  search-record recycling pool (linked via BrickzNode::m_14)
    char m_pad34[0x40 - 0x34];
    BrickzNode* m_freeList; // +0x40  bucket-node free list (linked via BrickzNode::m_8)
    char m_pad44[0x48 - 0x44];
    void (*m_stepCb)(); // +0x48  per-step callback (engine hook)
    i32 m_edgeMask;     // +0x4c  edge-punch reject mask (SearchEdge; Expand pre-filter)
    i32 m_maskA;        // +0x50  Search maskA: cell blocked when set (unless m_maskC bit)
    i32 m_maskC;        // +0x54  Search maskC: allow-override for m_maskA
    i32 m_maskB;        // +0x58  Search maskB: diagonal corner-cut reject mask
    i32 m_dirty;        // +0x5c  dirty flag (UpdateDiagonals re-walk gate)
    i32 m_originX;      // +0x60  grid origin x (also RECT.left of the +0x60 bound rect)
    i32 m_originY;      // +0x64  grid origin y (RECT.top)
    char m_pad68[0x70 - 0x68];
    i32 m_gridW;              // +0x70  grid width extent (cells)
    i32 m_gridH;              // +0x74  grid height extent (cells)
    BrickzAttrMgr* m_attrMgr; // +0x78  attribute/bute-type manager (ComputeCellFlags)
    BrickzSerObj* m_7c;       // +0x7c  serialize sub-object (unused in this TU)
};

#endif // GRUNTZ_BRICKZ_H
