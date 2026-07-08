// SpawnList.h - the named spawn/voice record (CSpawnEntry) + its owning list
// (CSpawnList), C:\Proj\Gruntz. ONE record class + ONE list class, unified from
// SIX per-TU views (matcher-5, evidence below):
//
//   record views: Obj09a260 (GetStr @0x9a260), CSpawnEntryN (AreaMgr.cpp),
//                 CObjResNode (LoadObjectResources.cpp), CVoiceSound
//                 (GruntSpawnConfig.h), AreaMgr.cpp's old local CSpawnEntry
//   list views:   CSpawnList (AreaMgr.cpp), CObjResBuilder (LoadObjectResources
//                 .cpp), AreaPtrList/+Base (AreaMgr.h), GruntSpawnConfig.h's old
//                 "CSpawnEntry" (a misnomer - it held the CObList, i.e. the LIST),
//                 VoiceList (VoiceSoundList.cpp), C99ba0's m_sub / C9a420
//                 (BoundaryLowerMethodsViews.h)
//
// UNIFICATION PROOF: MSVC 5.10's linker has no /OPT:ICF, so one retail body =
// one class method. GetName's body @0x9a260 is called on spawn-list entries
// (CSpawnList::FindEntry/FindByName), res-tree children (CAreaMgr::LoadObject*
// Resources) and voice-list elements (CGruntSpawnConfig::PickWeighted @0x11c0c3)
// -> all three "element" kinds are ONE class. Same for the list: FindByName
// @0x9a290 is both areamgr's Extract and loadobjectresources' FindAdd;
// DeleteAllEntries @0x9a450 is both AreaPtrList::RemoveAllNodes and
// CSpawnEntry::EmptyVoiceList; the {CObList ctor(10), +0x1c=0, +0x20=-1} init is
// both CAreaMgr's ctor fold (0x99ba0) and VoiceList's inline ctor.
//
// Field names are placeholders where unproven; offsets + code bytes are
// load-bearing (campaign doctrine).
#ifndef GRUNTZ_GRUNTZ_SPAWNLIST_H
#define GRUNTZ_GRUNTZ_SPAWNLIST_H

#include <rva.h>

#include <Ints.h>
#include <Mfc.h> // CObList (embedded) + CString (name member / by-value returns)

// ---------------------------------------------------------------------------
// CSpawnEntry - the 12-byte named record: a name CString, the "wanted" mark the
// resource reconcilers flip, and the ctor's data word (the voice-add flag).
// Non-polymorphic; teardown is the implicit ~CString + operator delete (RezFree).
// ---------------------------------------------------------------------------
class CSpawnEntry {
public:
    CSpawnEntry(CString name, i32 data); // 0x11c630 (__thiscall ret 8; /GX by-value temp)
    RVA(0x0009a260, 0x1d)
    CString GetName() {
        return m_name;
    }
    CString GetTail();                   // 0x9a830  the name past its 8-char group prefix

    CString m_name; // +0x00  the record name
    i32 m_flag;     // +0x04  = 0 at ctor; "wanted" mark (LoadObject*Resources set 1)
    i32 m_data;     // +0x08  = the ctor's 2nd arg (0 from the voice builder)
};
SIZE(CSpawnEntry, 0xc);

// The CObList node the walkers traverse (the MFC CNode shape: next/prev/data).
// POSITION <-> node view is MFC's own idiom (GetHeadPosition returns the node).
struct CSpawnNode {
    CSpawnNode* m_next;   // +0x00
    CSpawnNode* m_prev;   // +0x04
    CSpawnEntry* m_entry; // +0x08
};
SIZE(CSpawnNode, 0xc);

// ---------------------------------------------------------------------------
// CSpawnList - a CObList of CSpawnEntry* plus a scan cursor and the last-picked
// index (the weighted random picker's memory). The CObList is EMBEDDED, not a
// base: the dtor @0x99ca0 stamps no vtable before the member ~CObList call.
//
// The ctor is inline (folds into CAreaMgr's ctor @0x99ba0 and the voice
// builder's `new` site). The dtor's out-of-line copy is @0x99ca0; it is DEFINED
// (inline) in AreaMgr.cpp - its retail TU - so it inline-folds into ~CAreaMgr
// (@0x99c20, the EH state-1 member phase) exactly as retail; every OTHER TU
// (e.g. CGruntSpawnConfig::Clear's explicit `l->~CSpawnList()`) sees only this
// declaration and emits the retail extern call.
// ---------------------------------------------------------------------------
class CSpawnList {
public:
    CSpawnList() {
        m_cursor = 0;
        m_lastPicked = -1;
    }
    ~CSpawnList();           // 0x99ca0  DeleteAllEntries + member ~CObList (def: AreaMgr.cpp)
    void ClearFlags();       // 0x9a420  zero every entry's m_flag
    void DeleteAllEntries(); // 0x9a450  delete every entry, then m_list.RemoveAll()
    CSpawnEntry* FindEntry(CString name, i32 useHash); // 0x9a0d0  (hash / strcmp match)
    CSpawnEntry* FindByName(const CString& name);      // 0x9a290  (was Extract/FindAdd)
    void AddVoiceSound(CString s, i32 flag);           // 0x11c560 (def: GruntSpawnConfig.cpp)

    CObList m_list;       // +0x00  the entry list (0x1c B; block size 10)
    CSpawnNode* m_cursor; // +0x1c  scan cursor (LoadObject*Resources' re-scan)
    i32 m_lastPicked;     // +0x20  last-picked index (-1; the weighted picker's memory)
};
SIZE(CSpawnList, 0x24);

#endif // GRUNTZ_GRUNTZ_SPAWNLIST_H
