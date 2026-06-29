// CDDPageMgr.cpp - the record-cache manager that sits between the (unconfirmed)
// CDDPageMgr cluster and CFaderMgr (proximity candidate; RTTI name does not
// survive, so the class name is a placeholder - campaign doctrine). The object is
// large (~0x8698 bytes): a "loaded" guard pointer at +0x04 and, far down, a 1-based
// growable array of record pointers { void* m_data@+0x8690; i32 m_count@+0x8694;
// i32 m_8698 }. Each record owns three heap buffers (+0x00, +0x10, +0x14).
//
// RemoveAt frees the idx-th record's three buffers, memmoves the tail down one
// slot, drops the count, and frees the record. FreeAll repeatedly RemoveAt(1)s
// every record, then frees the array buffer and clears the bookkeeping. The Rez
// heap free (0x1b9b82) is external/reloc-masked; memmove inlines to `rep movsd`.
//
// Only offsets + code bytes are load-bearing; field names are placeholders.
#include <Ints.h>
#include <rva.h>
#include <string.h> // memcpy (inlined as rep movsd at /O2 /Oi)

// The Rez heap free (0x1b9b82, __cdecl); reloc-masked.
extern "C" void RezFree(void* p);

// A cached record: three independently-owned heap buffers.
struct CPageRec {
    void* m_00; // +0x00 owned buffer
    char m_pad04[0x10 - 4];
    void* m_10; // +0x10 owned buffer
    void* m_14; // +0x14 owned buffer
};

class CDDPageMgr {
public:
    i32 RemoveAt(i32 idx); // 0x17d600
    i32 FreeAll();         // 0x17d6b0

    i32 m_00;   // +0x00
    void* m_04; // +0x04 "loaded" guard
    char m_pad08[0x8690 - 8];
    CPageRec** m_data; // +0x8690
    i32 m_count;       // +0x8694
    i32 m_8698;        // +0x8698
};

// ===========================================================================
// 0x17d600 - RemoveAt(idx): drop the 1-based idx-th record. Free its three owned
// buffers, shift the tail down one slot, decrement the count, free the record.
// ===========================================================================
// @early-stop
// constant-materialization wall: logic + layout byte-correct, but retail hoists
// the null `0` into edi (callee-saved) and reuses it for all 7 pointer
// null-checks/stores (`cmp [..],edi` / `mov [..],edi`), which forces idx into esi;
// MSVC5 here emits `test`/immediate-0 and keeps idx in edi instead. The whole
// register allocation cascades from that one materialization choice; not
// source-steerable (an explicit `void* z=0` folds back). ~86%.
RVA(0x0017d600, 0xad)
i32 CDDPageMgr::RemoveAt(i32 idx) {
    if (!m_04) {
        return 0;
    }
    if (m_count < idx) {
        return 0;
    }
    CPageRec* rec = m_data[idx - 1];
    if (rec->m_00) {
        RezFree(rec->m_00);
        rec->m_00 = 0;
    }
    if (rec->m_10) {
        RezFree(rec->m_10);
        rec->m_10 = 0;
    }
    if (rec->m_14) {
        RezFree(rec->m_14);
        rec->m_14 = 0;
    }
    i32 n = m_count - idx;
    CPageRec** dst = &m_data[idx - 1];
    if (n) {
        memcpy(dst, dst + 1, n * sizeof(CPageRec*));
    }
    m_count--;
    RezFree(rec);
    return 1;
}

// ===========================================================================
// 0x17d6b0 - FreeAll: RemoveAt(1) every record in turn (bailing if one fails),
// then free the array buffer and clear the bookkeeping fields.
// ===========================================================================
RVA(0x0017d6b0, 0x70)
i32 CDDPageMgr::FreeAll() {
    if (!m_04) {
        return 0;
    }
    i32 count = m_count;
    for (i32 i = 0; i < count; i++) {
        if (!RemoveAt(1)) {
            return 0;
        }
    }
    if (m_data) {
        RezFree(m_data);
        m_data = 0;
    }
    m_8698 = 0;
    m_count = 0;
    return 1;
}
