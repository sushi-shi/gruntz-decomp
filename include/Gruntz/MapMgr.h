// MapMgr.h - the engine's CMapMgr (the level/map manager), self-located via its
// RTTI vftable. Field names are placeholders
// (m_<hexoffset>); ONLY the OFFSETS + code bytes are load-bearing (campaign
// doctrine). The layout below is CONFIRMED from the ctor, the dtor
// and the slot-0 cleanup/reset method.
//
// ---------------------------------------------------------------------------
// CMapMgr (>= 0x60 bytes). vftable (6 virtual slots; see below). The
// ctor runs two embedded sub-object ctors (the two small growable arrays at
// +0x30 and +0x3c), then zeroes/seeds the scalar members:
//   +0x00  m_vtbl   : vftable pointer.
//   +0x04  m_4      : 0   (a heap pointer; slot-0 Reset `operator delete`s it).
//   +0x08  m_8      : 0   (a heap pointer; slot-0 Reset `operator delete`s it).
//   +0x0c  m_c      : 0
//   +0x10  m_10     : 0
//   +0x14  m_14     : (not written by the ctor)
//   +0x18  m_18     : 0
//   +0x1c  m_1c     : 0
//   +0x30  m_colA   : CMapArrayA  (0x0c bytes; element stride 0x24).
//   +0x3c  m_colB   : CMapArrayB  (0x0c bytes; element stride 0x0c).
//   +0x4c  m_4c     : 0
//   +0x50  m_50     : -1
//   +0x58  m_58     : 0
//   +0x5c  m_5c     : 1
//
// CMapMgr vftable (6 slots): slot0=Reset (the cleanup the dtor calls inline),
// slots 1..4 are local methods, slot5 is a shared/base method. Slots 1..5
// are out-of-line empty stubs here, present only to anchor the vftable
// relocation that the ctor stores (the CGameWnd vftable-in-TU idiom).
#ifndef SRC_GRUNTZ_MAPMGR_H
#define SRC_GRUNTZ_MAPMGR_H

#include <rva.h>
#include <Ints.h>

struct CSerialArchive; // Save/Load stream through it (Read @+0x2c / Write @+0x30)

// The arrays link their blocks in with the global ::operator new (??2@YAPAXI@Z
// @0x1b9b46) / ::operator delete (??3@YAXPAX@Z @0x1b9b82) - the real NAFXCW CRT
// allocators (called cast-free via <Mfc.h>); the former MapAlloc/MapFree fake
// externs (which only reloc-masked) are dissolved.

// ---------------------------------------------------------------------------
// CMapArrayA - a small growable array embedded in CMapMgr at +0x30 (0x0c bytes).
//   +0x00  m_block : the allocated element block (set by Allocate, freed by dtor)
//   +0x04  m_0     : a second block pointer (the array body the ctor/dtor own)
//   +0x08  m_count : element count.
// Element stride = 0x24 (36 bytes). Allocate(count) carves the block into a
// doubly-linked free list (each element's +0x14/+0x10 links). Ctor zeroes the
// three slots; dtor frees +0x04 then zeroes.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CMapArrayA);
class CMapArrayA {
public:
    CMapArrayA();
    ~CMapArrayA();
    i32 Allocate(u32 count);

    void* m_block; // +0x00
    void* m_0;     // +0x04  (the heap block the dtor frees)
    u32 m_count;   // +0x08
};

// ---------------------------------------------------------------------------
// CMapArrayB - a small growable array embedded in CMapMgr at +0x3c (0x0c bytes).
//   +0x00  m_0     : the heap block (freed by the dtor)
//   +0x04  m_block : the allocated element block (set by Allocate)
//   +0x08  m_count : element count.
// Element stride = 0x0c (12 bytes). Same free-list build as CMapArrayA but a
// 0x0c stride and the heap pointer at +0x00 (vs +0x04). Ctor zeroes the three
// slots; dtor frees +0x00 then zeroes.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CMapArrayB);
class CMapArrayB {
public:
    CMapArrayB();
    ~CMapArrayB();
    i32 Allocate(u32 count);

    void* m_0;     // +0x00  (the heap block the dtor frees)
    void* m_block; // +0x04
    u32 m_count;   // +0x08
};

// ---------------------------------------------------------------------------
// CMapMgr - the level/map manager (vftable, 6 slots). Polymorphic so
// the vptr lands at +0x00 and the two-phase vtable store falls out.
// ---------------------------------------------------------------------------
// A serialized map cell: 0x1c bytes. Save/Load stream the whole cell; Load then
// zeroes the +0x18 runtime field (not a persisted value).
SIZE_UNKNOWN(MapCell);
struct MapCell {
    char m_pad0[0x18];
    i32 m_18; // +0x18  runtime field (zeroed after Load)
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
    virtual void Reset();                                    // slot 0  0x09ec30
    virtual i32 Visit(CSerialArchive* ar, i32 mode, i32 a2, i32 a3); // [1] 0x09f7f0
    virtual i32 Save(CSerialArchive*);                       // slot 2  0x09f840
    virtual i32 Load(CSerialArchive*);                       // slot 3  0x09f9a0
    // Slots 4/5 are declared-only HERE: their bodies (0x09eca0 / 0x0853f0) are written
    // against CBrickzGrid's member set (m_rows / Insert / PopFront / Expand / CellPush),
    // so they keep that name until the whole CBrickzGrid method set folds onto this class.
    // @identity-TODO: CBrickzGrid == CMapMgr (its Search/IsCellClear ARE these two slots).
    virtual i32
    Search(i32 x1, i32 y1, i32 x2, i32 y2, void* list, i32 maskA, i32 maskB, i32 maskC); // [4]
    virtual i32 IsCellClear(i32 x, i32 y);                                               // [5]

    // The tile-system notify the +0x70 consumers drive (was the fake CTileGrid::Notify -
    // a method of a class that owns no retail address, i.e. an unlinkable phantom).
    void Notify(i32 x, i32 y, i32 state);

    MapCell* m_4;      // +0x04  cell grid (heap ptr; Reset frees it)
    i32** m_8;         // +0x08  row-pointer table (cell = (i32*)m_8[tileY] + tileX*7;
                       //        the type the CTileGrid view proved; Reset frees it)
    u32 m_c;           // +0x0c  grid dim 1 (outer count)
    u32 m_10;          // +0x10  grid dim 2 (inner count)
    i32 m_14;          // +0x14  (not ctor-written)
    i32 m_18;          // +0x18
    i32 m_1c;          // +0x1c
    i32 m_20;          // +0x20  (serialized 8-byte block m_20/m_24)
    i32 m_24;          // +0x24
    i32 m_28;          // +0x28  (serialized 8-byte block m_28/m_2c)
    i32 m_2c;          // +0x2c
    CMapArrayA m_colA; // +0x30  (0x0c bytes)
    CMapArrayB m_colB; // +0x3c  (0x0c bytes)
    i32 m_48;          // +0x48  (not ctor-written)
    i32 m_4c;          // +0x4c  (= 0)
    i32 m_50;          // +0x50  (= -1)
    i32 m_54;          // +0x54  (not ctor-written)
    i32 m_58;          // +0x58  (= 0)
    i32 m_5c;          // +0x5c  (= 1)
    i32 m_60;          // +0x60  (serialized 0x10 block m_60..m_6c)
    i32 m_64;          // +0x64
    i32 m_68;          // +0x68
    i32 m_6c;          // +0x6c
    i32 m_70;          // +0x70
    i32 m_74;          // +0x74
};

#endif // SRC_GRUNTZ_MAPMGR_H
