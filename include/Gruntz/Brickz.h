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
    i32 m_4;            // +0x04  per-cell edge/id payload
    BrickzCell* m_8;    // +0x08  per-cell link (CGruntzMapMgr::LoadAttributes zeroes it)
    i32 m_c;            // +0x0c  id3 payload (ComputeCellFlags)
    i32 m_10;           // +0x10  bute type code (ComputeCellFlags)
    i32 m_count;        // +0x14  per-cell open-list reference count
    BrickzNode* m_head; // +0x18  bucket-list head
};

// [SETTLED + DISSOLVED (Fable lane, 2026-07-13): the three Brickz* views were a
// second model of real tree classes, offset-for-offset:
//   BrickzButeObj  -> ::CTileImageSet  (<Gruntz/GameLevel.h>; "GetTypeCode @+0x20"
//                     IS GetCollisionAt @+0x20, same (i32,i32) arity)
//   BrickzGridDesc -> ::CLevelPlane    (m_20/m_24/m_28/m_2c/m_30/m_34 ==
//                     m_tileGrid/m_colOffsets/m_width/m_height/m_wrapW/m_wrapH;
//                     SetCell @0x77dc0 is now CLevelPlane::SetCell)
//   BrickzAttrMgr  -> the CSpriteFactoryHolder world holder chained to its
//                     ::CGameLevel (+0x24): the "bute table @+0x4c" is CGameLevel's
//                     m_imageSets CObArray data ptr (+0x48 array -> +0x4c m_pData of
//                     CTileImageSet*), the "grid desc @+0x5c" its m_mainPlane.
// Consumers (BrickzCellFlags_077790.cpp / TriggerMgr.cpp / MapMgr.h) use the real
// classes directly; GameLevel.h is NOT included here to keep this widely-included
// header light.]

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
