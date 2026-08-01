#ifndef GRUNTZ_ACTNAMEREGISTRY_H
#define GRUNTZ_ACTNAMEREGISTRY_H

#include <Bute/ButeTree.h>
#include <Wap32/ZVec.h>
#include <Wap32/zBitVec.h>
#include <Gruntz/TypeKeyColl.h>
#include <rva.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/ActReg.h>
#include <Mfc.h>

class CVariantSlot;
struct CString;

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
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetRetAddr();
        g_typeColl.m_errSink->Set(&g_typeColl, msg, 0xc);
        slot = g_typeColl.Scratch();
    }
    return slot;
}

static inline CString* ActNameLookupCallReport(i32 id) {
    g_typeColl.m_grown = 0;
    CString* slot;
    if (id >= g_typeColl.m_lo && id <= g_typeColl.m_hi) {
        slot = g_typeColl.Elem(id);
    } else if (g_typeColl._zvec::GrowTo(id, 0) != 0) {
        slot = g_typeColl.Elem(id);
    } else {
        g_typeColl.Report(g_errOutOfMem, 0xc);
        slot = g_typeColl.Scratch();
    }
    return slot;
}

#endif // GRUNTZ_ACTNAMEREGISTRY_H
