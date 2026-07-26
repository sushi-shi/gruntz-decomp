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
struct CTypeNameEntry; // canonical g_typeColl.m_spare slot record (<Gruntz/TypeNameEntry.h>)




// The grow-scratch CString array base (the dtor sweeps walk it).
static inline CString* ActNameSlots() {
    return reinterpret_cast<CString*>(g_typeColl.m_alloc);
}

static inline CString* ActNameLookup(i32 id) {
    g_typeColl.m_grown = 0;
    CString* slot;
    if (id >= g_typeColl.m_lo && id <= g_typeColl.m_hi) {
        slot = reinterpret_cast<CString*>(
            g_typeColl.m_base + (id - g_typeColl.m_lo) * g_typeColl.m_stride
        );
    } else if (reinterpret_cast<i32>((static_cast<_zvec*>(&g_typeColl))->GrowTo(id, 0))) {
        slot = reinterpret_cast<CString*>(
            g_typeColl.m_base + (id - g_typeColl.m_lo) * g_typeColl.m_stride
        );
    } else {
        void* item = g_projActCache;
        g_retAddrBreadcrumb = GetRetAddr();
        g_typeColl.m_errSink->Set(&g_typeColl, reinterpret_cast<i32>(item), 0xc);
        slot = reinterpret_cast<CString*>(g_typeColl.m_spare);
    }
    return slot;
}

#endif // GRUNTZ_ACTNAMEREGISTRY_H
