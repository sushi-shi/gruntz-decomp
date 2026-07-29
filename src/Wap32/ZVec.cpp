#include <Wap32/ZVec.h>
#include <Wap32/zBitVec.h> // GetRetAddr / g_projActCache / g_retAddrBreadcrumb (grow-fail breadcrumb)
#include <rva.h>
#include <Mfc.h> // CString (0x1b9b93 default ctor)
#include <new>   // placement CString ctor
#include <Bute/ButeTree.h>

#include <stdlib.h> // realloc (0x125180), free (0x120c30)
#include <string.h> // memcpy (0x121960), memset (rep stos)

RVA(0x000310f0, 0x8d)
char* _zdvec::IndexToPtr(i32 i) {
    char* r;
    m_grown = 0;
    if (i >= m_lo && i <= m_hi) {
        r = m_base + (i - m_lo) * m_stride;
    } else if (GrowTo(i, 0)) {
        r = m_base + (i - m_lo) * m_stride;
    } else {
        void* sentinel = g_projActCache;
        g_retAddrBreadcrumb = GetRetAddr();
        m_errSink->Set(this, sentinel, 0xc);
        r = m_spare;
    }
    char* slot = m_alloc;
    i32 n = m_grown;
    while (n-- != 0) {
        if (slot) {
            new (slot) CString();
        }
        slot += 4;
    }
    return r;
}

// _zvec::IndexToPtr(idx) - the plain accessor; grows on a bounds miss. 0x312a0.
// Byte-identical body to the _zdvec override above minus the construction loop:
// one result variable assigned in each arm and returned once (cl duplicates the
// two-pop epilogue into all three arms). That single-return shape is what pins
// idx in esi / this in edi the way retail does; the earlier multiple-return
// spelling reversed the pair and capped the fn at ~83%.
// @interleaver _zvec::IndexToPtr emitted-in <boundary: QueueDrainHost.cpp Drain
// @0x31250 (before) + BattlezMapConfig.cpp Step @0x31610 (after)>. A template-accessor
// COMDAT the /Gy linker placed by first-use between two OTHER units, not this TU block.
RVA(0x000312a0, 0x74)
char* _zvec::IndexToPtr(i32 idx) {
    char* r;
    m_grown = 0;
    if (idx >= m_lo && idx <= m_hi) {
        r = m_base + (idx - m_lo) * m_stride;
    } else if (GrowTo(idx, 0)) {
        r = m_base + (idx - m_lo) * m_stride;
    } else {
        void* sentinel = g_projActCache;
        g_retAddrBreadcrumb = GetRetAddr();
        m_errSink->Set(this, sentinel, 0xc);
        r = m_spare;
    }
    return r;
}
