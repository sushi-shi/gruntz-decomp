// AreaMgr.h - Gruntz CAreaMgr (C:\Proj\Gruntz).
//
// A small global area/zone state object: a current-index word at +0x00, an MFC
// CPtrList (block size 10) at +0x04, and two trailing ints at +0x20 / +0x24.
// One instance is a file-scope singleton (DAT_006459b0); another is reached via
// the level-loader's pointer at DAT_0061139c.  The class is non-polymorphic (the
// ctor stamps no vtable into +0x00); only the embedded CPtrList carries a vptr.
//
// Reset() clears the current-index word; Dispatch(index) records the 1..40 index
// then calls the matching per-area handler (40 sibling methods, each a no-op that
// returns 1); SameGroup(a) reports whether `a` and the current index fall in the
// same group-of-four within mod-36 index space (the level-loader's adjacency
// test).  The /GX destructor tears the embedded CPtrList down.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#ifndef SRC_GRUNTZ_AREAMGR_H
#define SRC_GRUNTZ_AREAMGR_H

#include <Ints.h>

// The MFC CPtrList base sub-object (0x1c bytes): vptr@+0x00, m_pNodeHead@+0x04,
// the remaining count/free/blocks fields up to +0x18.  Its destructor (0x1b48c6,
// ~CPtrList) is the trailing base-dtor call (trylevel -1) of the embedded list's
// teardown.  Reloc-masked rel32 callee.
class AreaPtrListBase {
public:
    void Dtor(); // 0x1b48c6  ~CPtrList
    ~AreaPtrListBase() {
        Dtor();
    }
    void* m_vptr;             // +0x00
    void* m_pNodeHead;        // +0x04
    char _pad08[0x1c - 0x08]; // +0x08..0x1b
};

// The embedded list member: a CPtrList-derived list whose destructor body frees
// every node (RemoveAllNodes, 0x09a450, trylevel 1) before the base ~CPtrList runs
// (trylevel -1).  The value-member dtor drives the container's /GX frame (see
// eh-dtor-model-members-as-destructible).
class AreaPtrList : public AreaPtrListBase {
public:
    void RemoveAllNodes(); // 0x09a450  frees every node + base teardown
    ~AreaPtrList() {
        RemoveAllNodes();
    }
};

class CAreaMgr {
public:
    // 0x09a0b0 - clear the current-index word.  Also the +0x00 member-teardown
    // call in the /GX destructor (trylevel 0).
    void Reset(); // 0x09a0b0

    // 0x099d40 - record the 1..40 area index then call the matching handler.
    i32 Dispatch(i32 index); // 0x099d40

    // 0x09b430 - same group-of-four (mod 36) as the current index?
    i32 SameGroup(i32 a); // 0x09b430

    // 0x099c20 - the /GX destructor: clears +0x00 then tears down the CPtrList.
    ~CAreaMgr(); // 0x099c20

    // The 40 per-area handlers (0x09af30..0x09b410, each `mov eax,1; ret`).  Each
    // is an external sibling method; reloc-masked rel32 callees of Dispatch.
    i32 H01();
    i32 H02();
    i32 H03();
    i32 H04();
    i32 H05();
    i32 H06();
    i32 H07();
    i32 H08();
    i32 H09();
    i32 H10();
    i32 H11();
    i32 H12();
    i32 H13();
    i32 H14();
    i32 H15();
    i32 H16();
    i32 H17();
    i32 H18();
    i32 H19();
    i32 H20();
    i32 H21();
    i32 H22();
    i32 H23();
    i32 H24();
    i32 H25();
    i32 H26();
    i32 H27();
    i32 H28();
    i32 H29();
    i32 H30();
    i32 H31();
    i32 H32();
    i32 H33();
    i32 H34();
    i32 H35();
    i32 H36();
    i32 H37();
    i32 H38();
    i32 H39();
    i32 H40();

    i32 m_currentAreaIndex;       // +0x00
    AreaPtrList m_spawnEntryList; // +0x04  CPtrList (block size 10)
    i32 m_20;                     // +0x20
    i32 m_24;                     // +0x24
};

#endif // SRC_GRUNTZ_AREAMGR_H
