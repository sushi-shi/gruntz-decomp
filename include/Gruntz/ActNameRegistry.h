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

// The find-or-create every activation registrar opens with.  Retail expands it at
// ~60 sites across 34 units; the register allocation is load-bearing, so the counter
// is read from the GLOBAL for both the insert argument and the slot lookup
// (docs/patterns/act-registrar-counter-cse-and-freeloop.md).
//
// That the shipped source had ONE of these, not sixty, is visible in the binary: the
// nineteen act-key strings ("A".."S") each occupy exactly ONE address in the whole
// image, referenced from every registrar.  A per-TU literal cannot fold under /Gf -
// a COMDAT string emitted from a shared header can, and does.
//
// A MACRO, not an inline function.  Measured 2026-08-08: written as an inline taking
// the key by parameter, every single-block registrar stayed byte-exact but all nine
// registrars that expand it TWICE OR MORE dropped 100 -> 71.8%, cl spending a fourth
// callee-saved register (`push ebp`) it does not spend on two hand-written blocks.
// MSVC 5 has no __forceinline; a textual macro is the period device for a block that
// must expand independently at every site, and it is what CGrunt's 19-block registrar
// already uses.
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

// Same, for the registrars whose out-of-memory path calls zDArray::Report instead of
// poking the error sink directly.  Both shapes are present in retail.
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
