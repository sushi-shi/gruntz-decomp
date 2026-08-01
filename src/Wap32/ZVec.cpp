#include <Wap32/ZVec.h>
#include <Wap32/zBitVec.h>
#include <rva.h>
#include <Mfc.h>
#include <new>
#include <Bute/ButeTree.h>

#include <stdlib.h>
#include <string.h>

RVA(0x000310f0, 0x8d)
char* _zdvec::IndexToPtr(i32 i) {
    char* r;
    m_grown = 0;
    if (i >= m_lo && i <= m_hi) {
        r = m_base + (i - m_lo) * m_stride;
    } else if (GrowTo(i, 0)) {
        r = m_base + (i - m_lo) * m_stride;
    } else {
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetRetAddr();
        m_errSink->Set(this, msg, 0xc);
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

// @interleaver _zvec::IndexToPtr emitted between QueueDrainHost::Drain and
// BattlezMapConfig::Step as a first-use COMDAT.
RVA(0x000312a0, 0x74)
char* _zvec::IndexToPtr(i32 idx) {
    char* r;
    m_grown = 0;
    if (idx >= m_lo && idx <= m_hi) {
        r = m_base + (idx - m_lo) * m_stride;
    } else if (GrowTo(idx, 0)) {
        r = m_base + (idx - m_lo) * m_stride;
    } else {
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetRetAddr();
        m_errSink->Set(this, msg, 0xc);
        r = m_spare;
    }
    return r;
}
