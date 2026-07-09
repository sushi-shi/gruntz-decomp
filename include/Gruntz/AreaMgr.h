// AreaMgr.h - Gruntz CAreaMgr (C:\Proj\Gruntz).
//
// The area/zone + spawn-entry manager: a current-index word at +0x00 and an
// embedded CSpawnList (<Gruntz/SpawnList.h>) at +0x04 - the named spawn-entry
// list the level loader, the OBJECTZ_ resource reconcilers and the multiplayer
// roster all walk. One instance is a file-scope singleton (DAT_006459b0);
// another is reached via the level-loader's pointer at DAT_0061139c. The class
// is non-polymorphic (the ctor stamps no vtable); only the embedded CObList
// carries a vptr.
//
// Reset() clears the current-index word; Dispatch(index) records the 1..40 index
// then calls the matching per-area handler (40 sibling methods, each a no-op that
// returns 1); SameGroup(a) reports whether `a` and the current index fall in the
// same group-of-four within mod-36 index space (the level-loader's adjacency
// test). The /GX destructor clears the index then inline-folds the member
// ~CSpawnList (DeleteAllEntries + ~CObList, EH state 1).
//
// LoadObject{Image,Sound,Anim}Resources (0x9a510/0x9a910/0x9ac20, defined in
// src/Gruntz/LoadObjectResources.cpp) are CAreaMgr methods: their `this` reaches
// the entry list at +0x04 exactly like the methods here (the old CObjResTree
// view), and they sit inside the 0x99ba0..0x9b430 areamgr retail TU band.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#ifndef SRC_GRUNTZ_AREAMGR_H
#define SRC_GRUNTZ_AREAMGR_H

#include <Ints.h>
#include <Gruntz/SpawnList.h> // CSpawnList (embedded at +0x04) + CSpawnEntry

// The per-spawn registry-holder + resolution-source args of the LoadObject*
// reconcilers (full defs local to src/Gruntz/LoadObjectResources.cpp).
struct ObjSpawnEntry;
class CSymTab; // Bute/SymTab.h (ResolvePath)

class CAreaMgr {
public:
    // 0x099ba0 - the ctor: the member CSpawnList's inline ctor folds in
    // (CObList(10), cursor = 0, last-picked = -1), then the index word clears.
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
    i32 LoadObjectResources(ObjSpawnEntry* entry, CSymTab* src); // 0x09a4c0 (drives the 3 below)
    i32 LoadObjectImageResources(ObjSpawnEntry* entry, CSymTab* src); // 0x09a510
    i32 LoadObjectSoundResources(ObjSpawnEntry* entry, CSymTab* src); // 0x09a910
    i32 LoadObjectAnimResources(ObjSpawnEntry* entry, CSymTab* src);  // 0x09ac20

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
SIZE(CAreaMgr, 0x28);

#endif // SRC_GRUNTZ_AREAMGR_H
