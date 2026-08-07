#ifndef GRUNTZ_ACTNAMEREGISTRY_H
#define GRUNTZ_ACTNAMEREGISTRY_H

#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/TypeKeyColl.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>

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
    } else if (g_typeColl._zvec::GrowTo(id, 0) != NULL) {
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
    } else if (g_typeColl._zvec::GrowTo(id, 0) != NULL) {
        slot = g_typeColl.Elem(id);
    } else {
        g_typeColl.Report(g_errOutOfMem, 0xc);
        slot = g_typeColl.Scratch();
    }
    return slot;
}

// zDArray::GrowTo hands back m_grown raw slots, so whoever grew the name table
// default-constructs them.  Retail runs it as a POST-decrement loop; the shape is
// load-bearing (docs/patterns/act-registrar-counter-cse-and-freeloop.md).
static inline void ActNameConstructGrownSlots() {
    CString* slot = ActNameSlots();
    i32 cnt = g_typeColl.m_grown;
    while (cnt-- != 0) {
        if (slot != NULL) {
            slot->CString::CString();
        }
        slot++;
    }
}

#endif // GRUNTZ_ACTNAMEREGISTRY_H
