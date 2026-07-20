#ifndef SRC_GRUNTZ_MAPMGR_H
#define SRC_GRUNTZ_MAPMGR_H

#include <rva.h>
#include <Ints.h>

struct BrickzCell;
struct BrickzNode;
class CDDrawSurfaceMgr; // the +0x78 world/asset holder (ex the BrickzAttrMgr view)
struct tagRECT;

class CFileMemBase;
typedef CFileMemBase CSerialArchive;

SIZE_UNKNOWN(CMapArrayA);
class CMapArrayA {
public:
    CMapArrayA();
    ~CMapArrayA();
    i32 Allocate(u32 count);

    BrickzNode* m_block; // +0x00  the 0x24-byte search-record pool (was void*)
    BrickzNode* m_0;     // +0x04  the owned element block (the dtor frees it; == m_block)
    u32 m_count;         // +0x08
};

SIZE_UNKNOWN(CMapArrayB);
class CMapArrayB {
public:
    CMapArrayB();
    ~CMapArrayB();
    i32 Allocate(u32 count);

    struct MapElemB* m_0; // +0x00  the owned element block (the dtor frees it)
    BrickzNode* m_block; // +0x04  the 0x0c-byte bucket-node pool (was void*)
    u32 m_count;         // +0x08
};

SIZE_UNKNOWN(MapCell);
struct MapCell {
    char m_pad0[0x18];
    i32 m_18; // +0x18  runtime field (zeroed after Load) == BrickzCell::m_head
};

SIZE_UNKNOWN(CMapMgr);
VTBL(CMapMgr, 0x001ea3b4); // vtable_names -> code (RTTI game class)
class CMapMgr {
public:
    CMapMgr();
    ~CMapMgr();

    // The six virtual slots, ALL REAL - read off the retail vtable @0x1ea3b4 (RTTI
    // .?AVCMapMgr@@). Slots 1/4/5 were modelled as `Vfunc1/Vfunc4/Vfunc5` EMPTY STUBS
    // "to anchor the vftable"; that was a fabrication - each slot holds a real function
    // that this tree already reconstructs under a FAKE class name:
    //   [1] 0x09f7f0 = ?Visit@CMapVisitTarget@@       (MapMgr.cpp - same TU!)
    //   [4] 0x09eca0 = ?Search@CBrickzGrid@@          (MapMgr.cpp - same TU!)
    //   [5] 0x0853f0 = ?IsCellClear@CBrickzGrid@@     (Brickz.cpp - other TU)
    // Slots 2/3 (Save/Load) are bit-identical in CGruntzMapMgr's vtable @0x1e9bb4, and
    // slot 5 there is the SAME 0x853f0 body - so CMapMgr / CBrickzGrid / CMapVisitTarget
    // are ONE class under three names. Slots 1 and 4 are folded here; IsCellClear stays
    // declared-only until CBrickzGrid's whole method set folds (@identity-TODO).
    virtual void Reset();                                            // slot 0  0x09ec30
    virtual i32 Visit(CSerialArchive* ar, i32 mode, i32 a2, i32 a3); // [1] 0x09f7f0
    virtual i32 Save(CSerialArchive*);                               // slot 2  0x09f840
    virtual i32 Load(CSerialArchive*);                               // slot 3  0x09f9a0
    // Slots 4/5 are declared-only HERE: their bodies (0x09eca0 / 0x0853f0) are written
    // against CBrickzGrid's member set (m_rows / Insert / PopFront / Expand / CellPush),
    // so they keep that name until the whole CBrickzGrid method set folds onto this class.
    // @identity-TODO: CBrickzGrid == CMapMgr (its Search/IsCellClear ARE these two slots).
    virtual i32
    Search(i32 x1, i32 y1, i32 x2, i32 y2, void* list, i32 maskA, i32 maskB, i32 maskC); // [4]
    virtual i32 IsCellClear(i32 x, i32 y);                                               // [5]

    // ---- the CBrickzGrid method set, FOLDED (one class, three names: CMapMgr /
    // CBrickzGrid / the ex-CMapVisitTarget). Slots 4/5 above are these Search and
    // IsCellClear, so the vtable binds to real bodies now. Definitions stay where they
    // are (MapMgr.cpp / Brickz.cpp / the Brickz* leaf TUs) - MSVC5 mangles a member
    // defined through the `CBrickzGrid` typedef as CMapMgr::, so no body moved.
    void Clip(const tagRECT* r);                        // 0x02b340 board dirty-rect clip
    void ComputeCellFlags(i32 x, i32 y, i32 id3);       // 0x077790 terrain cell-flag compute
    i32 AllocGrid(i32 width, i32 height, void (*callback)()); // 0x09ea60 grid allocator/initializer
    i32 SearchEdge(
        i32 xA,
        i32 yA,
        i32 xB,
        i32 yB,
        void* list,
        i32 clearFlag,
        i32 maskA,
        i32 maskC
    );                                               // 0x081e10
    i32 UpdateDiagonals(i32 unused);                 // 0x082030 diagonal-passability walk
    i32 LineIsClear(i32 x0, i32 y0, i32 x1, i32 y1); // 0x082250 straight-line probe
    // (`Serialize` @0x09356c is GONE - it was never this class's, and never a function:
    //  0x9356c is the TAIL of CGruntzMgr::BroadcastCmd @0x093460, which already claims
    //  0x93460..0x935bc. Its stack proves it (4 pushes + `add esp,0x10` vs FIVE pops
    //  before `ret 0x10` = five dwords short of a valid entry). The note that the +0x7c
    //  object "comes from a3, NOT from `this`" was the tell, read backwards: under
    //  BroadcastCmd's real 5-push frame, [esp+0x10] is the saved ecx - it IS `this`.
    //  See docs/patterns/unbalanced-stack-means-wrong-boundary.md.)
    i32 Expand(BrickzNode* node, i32 dx, i32 dy, i32 cost, i32 diag); // 0x09f010
    i32 Insert(BrickzNode* node);                                     // 0x09f370
    BrickzNode* PopFront();                                           // 0x09f430
    void CellPush(BrickzNode* node);                                  // 0x09f470
    BrickzNode* Find(i32 key1, i32 key2);                             // 0x09f500
    BrickzNode* FindCellNode(i32 col, i32 row);                       // 0x09f540
    void Drain();                                                     // 0x09f590
    void Unlink(BrickzNode* node);                                    // 0x09f690
    void CellPop(BrickzNode* node, i32 flag);                         // 0x09f710
    // 0x09f5d0 - empty every grid cell back into the node pools. NOT the slot-0 virtual
    // Reset (0x09ec30): two DIFFERENT functions that both carried the name `Reset` while
    // the class wore two names. Renamed on the fold (nothing called it by name).
    void ResetCells(); // 0x09f5d0

    // The tile-system notify the +0x70 consumers drive (was the fake CTileGrid::Notify -
    // a method of a class that owns no retail address, i.e. an unlinkable phantom).
    void Notify(i32 x, i32 y, i32 state);

    // 0x75a40 - bounds-checked cell-flags probe: the first dword (packed terrain
    // flags) of the 0x1c-byte cell at m_rows[y][x]; out of bounds returns 1. The
    // ex-`CGridLookup::Lookup` view (same m_rows/m_width/m_height trio at the same
    // offsets + the 0x1c cell stride). Out-of-line in TriggerMgrHitTest.cpp (the
    // hit-test unit interleaves it; its sole caller is that TU's megafn FUN_6f2f0).
    i32 CellFlagsAt(i32 x, i32 y);

    // FIELDS - the union of the CMapMgr and CBrickzGrid models of this ONE object.
    // The first four offsets carry BOTH spellings (a name alias at the same offset, zero
    // layout change): the semantic names come from the CBrickzGrid model, the m_<hex>
    // placeholders are what ~20 TUs already read through the CTileGrid spelling.
    // @fold-TODO: converge those consumers onto the semantic names (needs a TYPED sweep -
    // a blind rename would hit the identically-named members of other classes), then drop
    // the placeholder halves.
    union {
        BrickzCell* m_cellPool; // +0x04  flat cell pool (width*height cells)
        MapCell* m_4;           //        (same pointer; Reset frees it)
    };
    union {
        BrickzCell** m_rows; // +0x08  row table (m_rows[row] -> that row's cells)
        i32** m_8;           //        (same table; cell = (i32*)m_8[y] + x*7)
        char** m_rowBytes;   //        (same table, byte-addressed: the grunt step/arrival
                             //         machines walk rows[y] + x*0x1c - the ex GruntBoard view)
    };
    union {
        u32 m_width; // +0x0c  grid width (columns)
        u32 m_c;
    };
    union {
        u32 m_height; // +0x10  grid height (rows)
        u32 m_10;
    };
    u32 m_cellCount;        // +0x14  total cell count (width*height)
    BrickzNode* m_openList; // +0x18  sorted open/lookup list head
    i32 m_1c;               // +0x1c
    i32 m_startX;           // +0x20  search start x   (the "serialized 8-byte block")
    i32 m_startY;           // +0x24  search start y
    i32 m_goalX;            // +0x28  search goal x
    i32 m_goalY;            // +0x2c  search goal y
    // The two embedded pools. Their BLOCK pointers are the same two words the CBrickzGrid
    // model called m_nodePool (+0x30 = m_colA.m_block, the 0x24-byte search records) and
    // m_freeList (+0x40 = m_colB.m_block, the 0x0c-byte bucket nodes) - one object read
    // from two ends. (No union: a union member may not have a constructor, and these two
    // sub-objects have real ctors/dtors at their own RVAs.)
    CMapArrayA m_colA;  // +0x30
    CMapArrayB m_colB;  // +0x3c
    void (*m_stepCb)(); // +0x48  per-step callback (engine hook)
    i32 m_edgeMask;     // +0x4c  edge-punch reject mask (SearchEdge; Expand pre-filter)
    i32 m_maskA;        // +0x50  Search maskA: cell blocked when set (= -1 in the ctor)
    i32 m_maskC;        // +0x54  Search maskC: allow-override for m_maskA
    i32 m_maskB;        // +0x58  Search maskB: diagonal corner-cut reject mask
    i32 m_dirty;        // +0x5c  dirty flag (UpdateDiagonals re-walk gate; = 1 in the ctor)
    // +0x60..+0x6c IS a RECT - the board bound-rect (Clip/AllocGrid take its address and
    // hand it straight to Win32 IntersectRect). The "serialized 0x10 block" the CMapMgr
    // model saw and the bound rect the CBrickzGrid model saw are the same 16 bytes.
    // Spelled as the REAL RECT member it is (the ex four-edge + reinterpret spelling
    // was include-diet friction; includers are afx/windows-first, build-measured).
    RECT m_bounds;               // +0x60  board bound rect (grid origin at .left/.top)
    i32 m_gridW;                 // +0x70  clipped grid width extent (cells)
    i32 m_gridH;                 // +0x74  clipped grid height extent (cells)
    CDDrawSurfaceMgr* m_attrMgr; // +0x78  the world/asset holder (its m_24 is the
                                 //        CGameLevel; ex the BrickzAttrMgr view)
};

typedef CMapMgr CBrickzGrid;

struct MapElemB {
    void* m_0;        // +0x00
    MapElemB* m_prev; // +0x04
    MapElemB* m_next; // +0x08
};

struct BrickzFreeRec {
    i32 m_0; // +0x00  next-free link
    i32 m_4; // +0x04  path col
    i32 m_8; // +0x08  path row
};

#endif // SRC_GRUNTZ_MAPMGR_H
