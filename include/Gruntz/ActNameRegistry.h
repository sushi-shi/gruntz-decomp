// ActNameRegistry.h - the global activation-name registry shared by every
// CUserLogic leaf's RegisterActs (the trace's 0x18d method). Each leaf registers
// its activation handler under a bute-tree-managed name key:
//
//   id = g_buteTree.Find(key);          // name -> id
//   if (id == 0) {                       // not yet registered:
//       id = g_nextActId;
//       g_buteTree.Insert(key, id);      // map name -> id
//       slot = ActNameLookup(id);        // resolve id -> a name slot
//       <free the slot's old CString list>; slot->name = key;
//       g_nextActId++;
//   }
//   *ClassRegLookup(id) = handler;       // bind id -> the class's handler entry
//
// The id->slot resolve (ActNameLookup) and the id->entry resolve (ClassRegLookup)
// are BOTH the same activation-registry lookup archetype as VActLookup (the fast
// [lo,hi] range path + the slow Find/ActAlloc/Insert rebuild) - here over the
// SHARED name registry @0x6bf650 and the per-class registry respectively. All
// registry globals are DATA-pinned so the loads reloc-mask; the collection
// methods (Find 0x16da80 / Insert 0x16d850 / ActAlloc 0x16d990) are the same
// shared engine functions every registry calls (no body -> reloc-masked).
#ifndef GRUNTZ_ACTNAMEREGISTRY_H
#define GRUNTZ_ACTNAMEREGISTRY_H

#include <rva.h>

#include <Bute/ButeMgr.h>   // CButeTree::Find / Insert
#include <Gruntz/ActColl.h> // CActColl/CActColl2/ActAlloc + g_actCache/g_actAllocResult

// g_buteTree (0x6bf620) doubles as the name->id map here: Find (0x16d190) returns
// the id (0 == absent); Insert (0x16db90) maps a new key->id. Owned by the bute
// TU; redeclared so the calls reloc-mask.
DATA(0x002bf620)
extern CButeTree g_buteTree;

// The running activation-id counter (0x61aea8): the next id handed out, bumped
// after each new name is registered.
DATA(0x0021aea8)
extern i32 g_nextActId;

// The activation key string "A" (0x60a454) every RegisterActs registers under.
DATA(0x0020a454)
extern char s_actKeyA[];

// The shared coordinate-registry collection methods + alloc scratch (CActColl /
// CActColl2 / ActAlloc + g_actCache 0x6bf464 / g_actAllocResult 0x6bf428) come from
// <Gruntz/ActColl.h> - the SAME engine functions/globals every registry reuses.

// ---------------------------------------------------------------------------
// The shared name registry @0x6bf650 (the id->name-slot map RegisterActs writes
// the key string into). Same range/cache shape as VActColl: lo/hi gate the fast
// path, base+(- lo)*stride yields the slot; scratch is zeroed first, cur is the
// slow-path result. The resolved slot holds a CString-list the register frees
// (calling ~CString 0x1b9b93 over [curList .. count]) before assigning the key
// (CString::operator= 0x1b9e74).
// ---------------------------------------------------------------------------
DATA(0x002bf650)
extern CActColl g_nameReg; // 0x6bf650
DATA(0x002bf654)
extern CActColl2* g_nameReg2; // 0x6bf654
DATA(0x002bf658)
extern i32 g_nameRegLo; // 0x6bf658
DATA(0x002bf65c)
extern i32 g_nameRegHi; // 0x6bf65c
DATA(0x002bf660)
extern char* g_nameRegBase; // 0x6bf660
DATA(0x002bf668)
extern i32 g_nameRegStride; // 0x6bf668
DATA(0x002bf664)
extern char* g_nameRegCur; // 0x6bf664 (slow-path result slot)
DATA(0x002bf66c)
extern void** g_nameRegCurList; // 0x6bf66c (the slot's CString list base)
DATA(0x002bf670)
extern i32 g_nameRegScratch; // 0x6bf670 (zeroed first; doubles as the list count)

// A CString in the resolved name slot: ~CString (0x1b9b93) frees the old entries,
// operator= (0x1b9e74) assigns the new key. Modeled so the calls reloc-mask.
struct CActName {
    void Free();                  // 0x1b9b93 (~CString)
    void Assign(const char* key); // 0x1b9e74 (CString::operator=(char const*))
};

// The id->name-slot resolve (the fast range path + the slow Find/ActAlloc/Insert
// rebuild), then free the old name list and assign the key. Folded inline by
// RegisterActs once, in the build-id branch.
static inline char* ActNameLookup(i32 id) {
    g_nameRegScratch = 0;
    char* slot;
    if (id >= g_nameRegLo && id <= g_nameRegHi) {
        slot = g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    } else if (g_nameReg.Find(id, 0)) {
        slot = g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    } else {
        void* item = g_actCache;
        g_actAllocResult = (void*)ActAlloc();
        g_nameReg2->Insert(&g_nameReg, item, 0xc);
        slot = g_nameRegCur;
    }
    return slot;
}

#endif // GRUNTZ_ACTNAMEREGISTRY_H
