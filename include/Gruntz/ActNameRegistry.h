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

#define ACT_NAME_ID(idvar, key)                                                                    \
    i32 idvar = ActFindId(key);                                                                    \
    if (idvar == 0) {                                                                              \
        ActInsertId((key), g_typeCounter);                                                         \
        idvar = g_typeCounter;                                                                     \
        CString* slot = ActNameLookup(g_typeCounter);                                              \
        i32 grown = g_typeColl.m_grown;                                                            \
        CString* raw = ActNameSlots();                                                             \
        while (grown-- != 0) {                                                                     \
            if (raw != NULL) {                                                                     \
                raw->CString::CString();                                                           \
            }                                                                                      \
            raw++;                                                                                 \
        }                                                                                          \
        *slot = (key);                                                                             \
        g_typeCounter++;                                                                           \
    }

#define ACT_NAME_ID_CALL_REPORT(idvar, key)                                                        \
    i32 idvar = ActFindId(key);                                                                    \
    if (idvar == 0) {                                                                              \
        ActInsertId((key), g_typeCounter);                                                         \
        idvar = g_typeCounter;                                                                     \
        CString* slot = ActNameLookupCallReport(g_typeCounter);                                    \
        i32 grown = g_typeColl.m_grown;                                                            \
        CString* raw = ActNameSlots();                                                             \
        while (grown-- != 0) {                                                                     \
            if (raw != NULL) {                                                                     \
                raw->CString::CString();                                                           \
            }                                                                                      \
            raw++;                                                                                 \
        }                                                                                          \
        *slot = (key);                                                                             \
        g_typeCounter++;                                                                           \
    }

#endif // GRUNTZ_ACTNAMEREGISTRY_H
