// BitArray.cpp - CBitArray::SetBit (0x193640). A small-buffer-optimized bit set:
// the bit storage is inline (&m_storage) while capacity <= 32, else on the heap
// (m_storage is the heap pointer). Reserve (0x1936e0, external) grows it; the call is
// reloc-masked. Field names are placeholders; only offsets + code bytes matter.
#include <Ints.h>
#include <rva.h>

SIZE_UNKNOWN(CBitArray);
struct CBitArray {
    char m_pad0[8];
    u32 m_capacity; // +0x8  capacity in bits (unsigned: cmp is jbe)
    i32 m_storage;  // +0xc  inline bit storage / heap pointer (union by capacity)

    i32 Reserve(i32 nbits);     // 0x1936e0 (external, reloc-masked)
    CBitArray* SetBit(u32 idx); // 0x193640
};

RVA(0x00193640, 0x40)
CBitArray* CBitArray::SetBit(u32 idx) {
    if (Reserve(idx + 1)) {
        i32* p;
        if (m_capacity > 0x20) {
            p = (i32*)m_storage;
        } else {
            p = &m_storage;
        }
        p[idx >> 5] |= 1 << (idx & 0x1f);
    }
    return this;
}
