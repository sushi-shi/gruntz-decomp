#include <rva.h>

#include <Bute/ButeTree.h>
#include <Utils/BitArrayWord.h>
#include <Wap32/zBitVec.h>

#include <stdlib.h>
#include <string.h>

RVA(0x00193640, 0x40)
zBitVec* zBitVec::SetBit(u32 idx) {
    if (EnsureSize(idx + 1)) {
        u32* p;
        if (static_cast<u32>(m_capacity) > 0x20) {
            p = m_words;
        } else {
            p = &m_inline;
        }
        p[idx >> BITARRAY_WORD_SHIFT] |= 1 << (idx & BITARRAY_BIT_MASK);
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
    i32 nwords = static_cast<i32>((static_cast<u32>((o->m_capacity + 1)) >> BITARRAY_WORD_SHIFT));
    u32* obuf = static_cast<u32>(o->m_capacity) > 0x20 ? o->m_words : &o->m_inline;
    u32* tbuf = static_cast<u32>(m_capacity) > 0x20 ? m_words : &m_inline;
    for (i32 i = 0; i < nwords; i++) {
        tbuf[i] |= obuf[i];
    }
    return this;
}

#pragma function(memcpy)
RVA(0x001936e0, 0xd3)
i32 zBitVec::EnsureSize(i32 nbits) {
    u32 ndwords = ((nbits & BITARRAY_BIT_MASK) != 0 ? 1 : 0)
                  + (static_cast<u32>(nbits) >> BITARRAY_WORD_SHIFT);
    void* nbuf;
    if (static_cast<u32>(m_capacity) > 0x20) {
        nbuf = realloc(m_words, ndwords * 4);
        if (!nbuf) {
            goto fail;
        }
        u32 oldn = static_cast<u32>(m_capacity) >> BITARRAY_WORD_SHIFT;
        memset(static_cast<u32*>(nbuf) + oldn, 0, (ndwords - oldn) * 4);
    } else {
        nbuf = malloc(ndwords * 4);
        if (!nbuf) {
            goto fail;
        }
        memset(nbuf, 0, ndwords * 4);
        memcpy(nbuf, &m_words, sizeof(m_words));
    }
    m_words = static_cast<u32*>(nbuf);
    m_capacity = ndwords * BITARRAY_WORD_BITS;
    return 1;
fail:
    char* msg = g_errOutOfMem;
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errSink->Set(this, msg, 0xc);
    return 0;
}
