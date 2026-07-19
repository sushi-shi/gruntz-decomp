// ActNameRegistry.h - the global activation-name registry shared by every
// CUserLogic leaf's RegisterActs (the trace's 0x18d method). Each leaf registers
// its activation handler under a bute-tree-managed name key:
//
//   id = g_buteTree.Find(key);          // name -> id
//   if (id == 0) {                       // not yet registered:
//       id = g_typeCounter;
//       g_buteTree.Insert(key, id);      // map name -> id
//       slot = ActNameLookup(id);        // resolve id -> a name slot
//       <free the slot's old CString list>; slot->name = key;
//       g_typeCounter++;
//   }
//   *ClassRegLookup(id) = handler;       // bind id -> the class's handler entry
//
// The id->slot resolve (ActNameLookup) and the id->entry resolve (ClassRegLookup)
// are BOTH the same activation-registry lookup archetype as VActLookup (the fast
// [lo,hi] range path + the slow Find/GetRetAddr/Insert rebuild) - here over the
// SHARED name registry @0x6bf650 and the per-class registry respectively. All
// registry globals are DATA-pinned so the loads reloc-mask; the collection
// methods (Find 0x16da80 / Insert 0x16d850 / GetRetAddr 0x16d990) are the same
// shared engine functions every registry calls (no body -> reloc-masked).
#ifndef GRUNTZ_ACTNAMEREGISTRY_H
#define GRUNTZ_ACTNAMEREGISTRY_H

#include <Bute/ButeTree.h>
#include <Wap32/ZVec.h>

class CVariantSlot; // folded CActColl2
// CTypeKeyColl is the REAL class of the registry at 0x6bf650 - and its fields ARE the eight
// scalars this header used to declare as separate globals. Needs the full definition now.
#include <Gruntz/TypeKeyColl.h>
struct CTypeNameEntry; // canonical g_typeColl.m_spare slot record (<Gruntz/TypeNameEntry.h>)

#include <rva.h>

#include <Bute/ButeMgr.h>   // CButeTree::Find / Insert
#include <Gruntz/ActColl.h> // CActColl/CVariantSlot/GetRetAddr + g_projActCache/g_retAddrBreadcrumb

// g_buteTree (0x6bf620) doubles as the name->id map here: Find (0x16d190) returns
// the id (0 == absent); Insert (0x16db90) maps a new key->id. g_buteTree is declared
// canonically in <Bute/ButeTree.h> (reached via the <Bute/ButeMgr.h>/<Bute/ButeTree.h>
// includes above), and g_typeCounter in <Gruntz/TypeKeyColl.h> (included above).

// s_codeA (the "A" key string, 0x60a454) is declared canonically in
// <Gruntz/TypeKeyColl.h>, included above beside g_typeCounter.

// The shared coordinate-registry collection methods + alloc scratch (CActColl /
// CVariantSlot / GetRetAddr + g_projActCache 0x6bf464 / g_retAddrBreadcrumb 0x6bf428) come from
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
extern CTypeKeyColl g_typeColl; // 0x6bf650

// A CString in the resolved name slot: ~CString (0x1b9b93) frees the old entries,
// operator= (0x1b9e74) assigns the new key. Modeled so the calls reloc-mask.
#include <Mfc.h> // real CString (CActName was a fake view over it)

// The id->name-slot resolve (the fast range path + the slow Find/GetRetAddr/Insert
// rebuild), then free the old name list and assign the key. Folded inline by
// RegisterActs once, in the build-id branch.
static inline char* ActNameLookup(i32 id) {
    g_typeColl.m_grown = 0;
    char* slot;
    if (id >= g_typeColl.m_lo && id <= g_typeColl.m_hi) {
        slot = reinterpret_cast<char*>((g_typeColl.m_base + (id - g_typeColl.m_lo) * g_typeColl.m_stride));
    } else if (reinterpret_cast<i32>(((_zvec*)&g_typeColl)->GrowTo(id, 0))) {
        slot = reinterpret_cast<char*>((g_typeColl.m_base + (id - g_typeColl.m_lo) * g_typeColl.m_stride));
    } else {
        void* item = g_projActCache;
        g_retAddrBreadcrumb = GetRetAddr();
        g_typeColl.m_errSink->Set(&g_typeColl, reinterpret_cast<i32>(item), 0xc);
        slot = reinterpret_cast<char*>(g_typeColl.m_spare);
    }
    return slot;
}

#endif // GRUNTZ_ACTNAMEREGISTRY_H
