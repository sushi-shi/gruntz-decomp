// CFixedPtrArray32.cpp - a 32-slot fixed-capacity pointer array (trace
// placeholder ClassUnknown_1, RVAs 0x134be0 / 0x134c60 / 0x134c80). A small
// value-embedded collection: m_00 flag at +0x00, m_count at +0x04, a 32-entry
// pointer table m_items[32] at +0x08. Add() appends until the 32-slot cap;
// FillFrom() resets the object and bulk-appends a source list (skipping nulls).
// Self-contained (no externs, no EH frame); names are placeholders, offsets +
// code bytes are load-bearing.
#include <Gruntz/CFixedPtrArray32.h>

#include <rva.h>

// ===========================================================================
// 0x134be0 - FillFrom(src, n): reset then bulk-append. Rejects a null src or
// n >= 32; zeroes the whole table (rep stos of 32 dwords), then Add()s each
// non-null source entry, bailing (return 0) the first time Add fails.
// ===========================================================================
// @early-stop
// regalloc wall (docs/patterns/zero-register-pinning.md + pin-local-for-callee-
// saved-reg.md): logic + structure byte-exact, but retail keeps `src` in volatile
// edx and the loop counter/limit in callee-saved esi/ebx (no spill), while cl
// pins the src-walker in a callee-saved reg and spills `n` to a stack slot
// (reloaded each iter). Not flipped by pointer-walk / src[i] indexing / cnt-pin /
// do-while variants. ~85%; Clear + Add are 100%.
RVA(0x00134be0, 0x7e)
i32 CFixedPtrArray32::FillFrom(void** src, i32 n, i32 unused) {
    i32 i = 0;
    if (!src) {
        return 0;
    }
    if (n >= 32) {
        return 0;
    }
    m_00 = 0;
    m_count = 0;
    for (i32 j = 0; j < 32; j++) {
        m_items[j] = 0;
    }
    void** p = src;
    for (; i < n; i++, p++) {
        if (*p) {
            if (!Add(*p)) {
                return 0;
            }
        }
    }
    return 1;
}

// ===========================================================================
// 0x134c60 - Clear(): zero the 32-slot table (rep stos) and clear m_count. m_00
// is left untouched (set by the owner). Returns void (no return-this epilogue),
// so this is a plain method, not the C++ constructor.
// ===========================================================================
RVA(0x00134c60, 0x14)
void CFixedPtrArray32::Clear() {
    for (i32 j = 0; j < 32; j++) {
        m_items[j] = 0;
    }
    m_count = 0;
}

// ===========================================================================
// 0x134c80 - Add(item): append to the table if a slot remains (count < 32),
// returning 1; otherwise return 0 without storing.
// ===========================================================================
RVA(0x00134c80, 0x24)
i32 CFixedPtrArray32::Add(void* item) {
    if (m_count >= 32) {
        return 0;
    }
    m_items[m_count] = item;
    m_count++;
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CFixedPtrArray32);
