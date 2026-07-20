#include <Wap32/zBitVec.h>
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
