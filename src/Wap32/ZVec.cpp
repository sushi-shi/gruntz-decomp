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
        i32 sentinel = reinterpret_cast<i32>(g_projActCache);
        g_retAddrBreadcrumb = GetRetAddr();
        m_errSink->Set(static_cast<void*>(this), sentinel, 0xc);
        r = m_spare;
    }
    char* slot = m_alloc;
    i32 n = m_grown;
    while (n-- != 0) {
        if (slot) {
            new (static_cast<void*>(slot)) CString();
        }
        slot += 4;
    }
    return r;
}

// _zvec::IndexToPtr(idx) - the plain accessor; grows on a bounds miss. 0x312a0.
// @interleaver _zvec::IndexToPtr emitted-in <boundary: QueueDrainHost.cpp Drain
// @0x31250 (before) + BattlezMapConfig.cpp Step @0x31610 (after)>. A template-accessor
// COMDAT the /Gy linker placed by first-use between two OTHER units, not this TU block.
// @early-stop
// regalloc wall: retail pins idx in esi / this in edi (arg-before-this) and
// merges the in-range + grow-success offset tails; our recompile mirrors the
// esi/edi assignment and duplicates the tail, ~83%. Logic exact. The derived
// override (0x310f0) matches because its trailing fixup loop shifts the reg
// pressure; the loop-less base accessor does not flip. Not source-steerable.
RVA(0x000312a0, 0x74)
char* _zvec::IndexToPtr(i32 idx) {
    i32 lo = m_lo;
    m_grown = 0;
    if (idx >= lo && idx <= m_hi) {
        idx -= lo;
        idx *= m_stride;
        return m_base + idx;
    }
    if (GrowTo(idx, 0)) {
        char* base = m_base;
        idx -= m_lo;
        idx *= m_stride;
        return base + idx;
    }
    i32 sentinel = reinterpret_cast<i32>(g_projActCache);
    g_retAddrBreadcrumb = GetRetAddr();
    m_errSink->Set(static_cast<void*>(this), sentinel, 0xc);
    return m_spare;
}
