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

// Raw heap alloc/free the arrays link in (engine NAFXCW: alloc(size) returns a
// pointer; free(ptr)). __cdecl, args on the stack. Modeled external/no-body so
// the `call rel32` displacements are reloc-masked.
extern "C" void *MapAlloc(unsigned int size);
extern "C" void  MapFree(void *p);

// ---------------------------------------------------------------------------
// CMapArrayA - a small growable array embedded in CMapMgr at +0x30 (0x0c bytes).
//   +0x00  m_block : the allocated element block (set by Allocate, freed by dtor)
//   +0x04  m_0     : a second block pointer (the array body the ctor/dtor own)
//   +0x08  m_count : element count.
// Element stride = 0x24 (36 bytes). Allocate(count) carves the block into a
// doubly-linked free list (each element's +0x14/+0x10 links). Ctor zeroes the
// three slots; dtor frees +0x04 then zeroes.
// ---------------------------------------------------------------------------
class CMapArrayA {
public:
    CMapArrayA();
    ~CMapArrayA();
    int Allocate(unsigned int count);

    void    *m_block;   // +0x00
    void    *m_0;       // +0x04  (the heap block the dtor frees)
    unsigned m_count;   // +0x08
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
class CMapArrayB {
public:
    CMapArrayB();
    ~CMapArrayB();
    int Allocate(unsigned int count);

    void    *m_0;       // +0x00  (the heap block the dtor frees)
    void    *m_block;   // +0x04
    unsigned m_count;   // +0x08
};

// ---------------------------------------------------------------------------
// CMapMgr - the level/map manager (vftable, 6 slots). Polymorphic so
// the vptr lands at +0x00 and the two-phase vtable store falls out.
// ---------------------------------------------------------------------------
class CMapMgr {
public:
    CMapMgr();
    ~CMapMgr();

    // The six virtual slots. Slot 0 (Reset) is matched; the dtor
    // calls it inline. Slots 1..5 are out-of-line stubs that anchor the vftable.
    virtual void Reset();           // slot 0
    virtual void Vfunc1();          // slot 1
    virtual void Vfunc2();          // slot 2
    virtual void Vfunc3();          // slot 3
    virtual void Vfunc4();          // slot 4
    virtual void Vfunc5();          // slot 5

    void    *m_4;       // +0x04  (heap ptr; Reset frees it)
    void    *m_8;       // +0x08  (heap ptr; Reset frees it)
    int      m_c;       // +0x0c
    int      m_10;      // +0x10
    int      m_14;      // +0x14  (not ctor-written)
    int      m_18;      // +0x18
    int      m_1c;      // +0x1c
    char     m_pad20[0x30 - 0x20];  // +0x20..+0x2f (not ctor-written)
    CMapArrayA m_colA;  // +0x30  (0x0c bytes)
    CMapArrayB m_colB;  // +0x3c  (0x0c bytes)
    int      m_48;      // +0x48  (not ctor-written)
    int      m_4c;      // +0x4c  (= 0)
    int      m_50;      // +0x50  (= -1)
    int      m_54;      // +0x54  (not ctor-written)
    int      m_58;      // +0x58  (= 0)
    int      m_5c;      // +0x5c  (= 1)
};

#endif // SRC_GRUNTZ_MAPMGR_H
