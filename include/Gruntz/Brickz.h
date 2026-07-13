// Brickz.h - trace-discovered move-grid cluster (placeholder name CBrickzGrid).
//
// NOT the game-object CBrickz (include/Gruntz/Brickz.h, a real CUserLogic-derived
// RTTI class): the trace tool mislabeled this self-contained grid container CBrickz,
// which collided its methods' mangled names (e.g. ?Serialize@CBrickz@@QAEHHHHH@Z)
// with the real CBrickz's. Renamed CBrickzGrid to recover the distinct owner.
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
#include <rva.h>

class CBattlezData; // folded BrickzSerObj
struct tagRECT;     // Win32 RECT (CBrickzGrid::Clip arg)

#include <Ints.h>
#include <Gruntz/MapMgr.h> // CBrickzGrid IS CMapMgr (see the fold note below)

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
    // +0x00 is read BOTH ways in retail: as a dword of packed terrain flags, and as a single
    // BYTE at +3 (CGrunt's tracked-coord scan tests `byte [cell+3] & 0x20` - the "stepped"
    // bit - with a byte load, not a dword test of 0x20000000). An anonymous union models both
    // without a cast; it is the same 4 bytes. (This is what the CScanCell view in
    // GruntCombat.cpp existed to express.)
    union {
        i32 m_0;           // +0x00  packed terrain flags
        u8 m_flagBytes[4]; //        byte view; [3] & 0x20 = the stepped/visited bit
    };
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
    // 0x077dc0 - flat cell setter: m_20[ m_24[y] + x ] = id. __thiscall(x, y, id).
    // CTerrainTileLoader::Load reaches it via loader->m_24 (BrickzAttrMgr)->m_5c.
    void SetCell(i32 x, i32 y, i32 id);
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

// CBrickzGrid IS CMapMgr - RTTI .?AVCMapMgr@@, vtable 0x1ea3b4, and the binary says so
// three ways: its Search (0x09eca0) and IsCellClear (0x0853f0) ARE slots 4 and 5 of that
// vtable; its cell record is the same 0x1c-byte struct CMapMgr's MapCell describes; and
// its "+0x30 node pool / +0x40 free list" are exactly the block pointers of CMapMgr's
// embedded CMapArrayA (+0x30, 0x24-byte elements) and CMapArrayB (+0x3c). The trace tool
// that named this class grouped methods by a shared `this` and never had the vtable.
//
// The class definition is DISSOLVED onto <Gruntz/MapMgr.h>'s CMapMgr (which absorbed this
// model's semantic field names and its whole method set). The NAME survives as a typedef:
// MSVC5 mangles `i32 CBrickzGrid::Search(...)` through the typedef as ?Search@CMapMgr@@,
// so every definition and all 17 consumers keep compiling while the symbols become the
// real class's.

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_BRICKZ_H
