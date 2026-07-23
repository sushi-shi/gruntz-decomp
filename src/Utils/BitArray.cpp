#include <Bute/ButeTree.h> // CVariantSlot::Set (EnsureSize failure reporting)
#include <Wap32/zBitVec.h>

#include <stdlib.h> // realloc, malloc
#include <string.h> // memset, memcpy
#include <rva.h>

RVA(0x00193640, 0x40)
zBitVec* zBitVec::SetBit(u32 idx) {
    if (EnsureSize(idx + 1)) {
        i32* p;
        if (static_cast<u32>(m_capacity) > 0x20) {
            p = reinterpret_cast<i32*>(m_words);
        } else {
            p = reinterpret_cast<i32*>(&m_words);
        }
        p[idx >> 5] |= 1 << (idx & 0x1f);
    }
    return this;
}

RVA(0x00193680, 0x5e)
zBitVec* zBitVec::Or(zBitVec* o) {
    if (static_cast<u32>(o->m_capacity) > static_cast<u32>(m_capacity)) {
        if (!EnsureSize(o->m_capacity)) {
            return this;
        }
    }
    i32 nwords = static_cast<i32>((static_cast<u32>((o->m_capacity + 1)) >> 5));
    u32* obuf =
        static_cast<u32>(o->m_capacity) > 0x20 ? o->m_words : reinterpret_cast<u32*>(&o->m_words);
    u32* tbuf = static_cast<u32>(m_capacity) > 0x20 ? m_words : reinterpret_cast<u32*>(&m_words);
    for (i32 i = 0; i < nwords; i++) {
        tbuf[i] |= obuf[i];
    }
    return this;
}

#pragma function(memcpy)
RVA(0x001936e0, 0xd3)
i32 zBitVec::EnsureSize(i32 nbits) {
    u32 ndwords = ((nbits & 0x1f) != 0 ? 1 : 0) + (static_cast<u32>(nbits) >> 5);
    void* nbuf;
    if (static_cast<u32>(m_capacity) > 0x20) {
        nbuf = realloc(m_words, ndwords * 4);
        if (!nbuf) {
            goto fail;
        }
        u32 oldn = static_cast<u32>(m_capacity) >> 5;
        memset(reinterpret_cast<u32*>(nbuf) + oldn, 0, (ndwords - oldn) * 4);
    } else {
        nbuf = malloc(ndwords * 4);
        if (!nbuf) {
            goto fail;
        }
        memset(nbuf, 0, ndwords * 4);
        memcpy(nbuf, &m_words, 4);
    }
    m_words = static_cast<u32*>(nbuf);
    m_capacity = ndwords * 32;
    return 1;
fail:
    void* cache = g_projActCache;
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errSink->Set(this, reinterpret_cast<i32>(cache), 0xc);
    return 0;
}
