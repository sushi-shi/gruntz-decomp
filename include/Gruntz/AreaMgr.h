#ifndef SRC_GRUNTZ_AREAMGR_H
#define SRC_GRUNTZ_AREAMGR_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SpawnList.h> // CSpawnList (embedded at +0x04) + CSpawnEntry

class CDDrawSurfaceMgr; // DDrawMgr/DDrawSurfaceMgr.h (the per-spawn registry holder)
class CSymTab;          // Bute/SymTab.h (ResolvePath)

class CAreaMgr {
public:
    // 0x099ba0 - the ctor: the member CSpawnList's inline ctor folds in
    // (CPtrList(10), cursor = 0, last-picked = -1), then the index word clears.
    CAreaMgr(); // 0x099ba0

    // 0x09a0b0 - clear the current-index word.  Also the +0x00 member-teardown
    // call in the /GX destructor (trylevel 0).
    void Reset(); // 0x09a0b0

    // 0x099d40 - record the 1..40 area index then call the matching handler.
    i32 Dispatch(i32 index); // 0x099d40

    // 0x09b430 - same group-of-four (mod 36) as the current index?
    i32 SameGroup(i32 a); // 0x09b430

    // 0x099c20 - the /GX destructor: Reset() then the inline-folded ~CSpawnList.
    ~CAreaMgr(); // 0x099c20

    // The OBJECTZ_ namespace asset reconcilers (LoadObjectResources.cpp): walk
    // the entry's registry map, reconcile against m_spawnEntryList, install the
    // still-unwanted assets through the registry.
    i32 LoadObjectResources(CDDrawSurfaceMgr* entry, CSymTab* src); // 0x09a4c0 (drives the 3 below)
    i32 LoadObjectImageResources(CDDrawSurfaceMgr* entry, CSymTab* src); // 0x09a510
    i32 LoadObjectSoundResources(CDDrawSurfaceMgr* entry, CSymTab* src); // 0x09a910
    i32 LoadObjectAnimResources(CDDrawSurfaceMgr* entry, CSymTab* src);  // 0x09ac20

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

    i32 m_currentAreaIndex;      // +0x00
    CSpawnList m_spawnEntryList; // +0x04  the named spawn-entry list (0x24 B)
};
SIZE(0x28);

extern CAreaMgr g_areaMgr;
extern CAreaMgr* g_pAreaMgr;

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" i32 SpawnNameCmp(const char* a, const char* b, i32 n); // 0x120440

#endif // SRC_GRUNTZ_AREAMGR_H
