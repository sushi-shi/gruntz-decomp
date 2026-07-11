// BitArray.cpp - zBitVec::SetBit (0x193640). A small-buffer-optimized bit set:
// the bit storage is inline (&m_words) while capacity <= 32, else on the heap
// (m_words is the heap pointer). EnsureSize (0x1936e0, external) grows it; the
// call is reloc-masked. This is the devs' real zBitVec (<Wap32/zBitVec.h>); the
// former CBitArray was a per-TU view of it (its m_capacity/m_storage == zBitVec's
// m_capacity/m_words, and Reserve IS zBitVec::EnsureSize). Field names are
// placeholders; only offsets + code bytes matter.
#include <Wap32/zBitVec.h>
#include <rva.h>

RVA(0x00193640, 0x40)
zBitVec* zBitVec::SetBit(u32 idx) {
    if (EnsureSize(idx + 1)) {
        i32* p;
        if ((u32)m_capacity > 0x20) {
            p = (i32*)m_words;
        } else {
            p = (i32*)&m_words;
        }
        p[idx >> 5] |= 1 << (idx & 0x1f);
    }
    return this;
}
