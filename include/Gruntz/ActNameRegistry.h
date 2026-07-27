#ifndef GRUNTZ_ACTNAMEREGISTRY_H
#define GRUNTZ_ACTNAMEREGISTRY_H

#include <Bute/ButeTree.h>
#include <Wap32/ZVec.h>
#include <Wap32/zBitVec.h> // ex Globals.h
#include <Gruntz/TypeKeyColl.h>
#include <rva.h>
#include <Bute/ButeMgr.h> // CButeTree::Find / Insert
#include <Gruntz/ActReg.h>
#include <Mfc.h> // real CString (CActName was a fake view over it)

class CVariantSlot; // folded CActColl2
struct CString; // canonical g_typeColl.m_spare slot record (<Gruntz/TypeNameEntry.h>)




// The grow-scratch CString array base (the dtor sweeps walk it).
static inline CString* ActNameSlots() {
    return g_typeColl.Slots();
}

static inline CString* ActNameLookup(i32 id) {
    g_typeColl.m_grown = 0;
    CString* slot;
    if (id >= g_typeColl.m_lo && id <= g_typeColl.m_hi) {
        slot = g_typeColl.Elem(id);
    } else if (g_typeColl._zvec::GrowTo(id, 0) != 0) {
        slot = g_typeColl.Elem(id);
    } else {
        void* item = g_projActCache;
        g_retAddrBreadcrumb = GetRetAddr();
        g_typeColl.m_errSink->Set(&g_typeColl, item, 0xc);
        slot = g_typeColl.Scratch();
    }
    return slot;
}

// The same lookup with the grow-fail tail left OUTLINED (`zErrHandling::Report`,
// 0x34960) instead of expanded. Retail's shape at the FIRST key's name lookup in a
// two-key registrar; the second key's still expands. See the ResolveEntryCallReport
// note in <Wap32/ZVec.h>.
static inline CString* ActNameLookupCallReport(i32 id) {
    g_typeColl.m_grown = 0;
    CString* slot;
    if (id >= g_typeColl.m_lo && id <= g_typeColl.m_hi) {
        slot = g_typeColl.Elem(id);
    } else if (g_typeColl._zvec::GrowTo(id, 0) != 0) {
        slot = g_typeColl.Elem(id);
    } else {
        g_typeColl.Report(g_projActCache, 0xc);
        slot = g_typeColl.Scratch();
    }
    return slot;
}

#endif // GRUNTZ_ACTNAMEREGISTRY_H
