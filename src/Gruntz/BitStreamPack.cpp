// BitStreamPack.cpp - 0x188860: a 16-bit-accumulator bitstream emitter that
// writes an end-of-block marker (the 3-bit code 010 followed by the global
// Huffman marker {code,len}@0x6c0060), bumps the byte budget, flushes, and -
// when the remaining budget is still short - emits the marker a second time.
// __cdecl over the codec state. Field names are placeholders; only offsets +
// emitted bytes are load-bearing.
#include <Ints.h>
#include <rva.h>

// The shared Huffman marker symbol: code @+0, bit-length @+2 (one .data object).
struct BitMarker {
    u16 code; // +0x00
    u16 len;  // +0x02
};
DATA(0x002c0060)
extern BitMarker g_bitMarker; // 0x6c0060

// The codec/bitstream state.
struct BitState {
    char _0[0x08];
    u8* m_buf; // +0x08 output byte buffer
    char _c[0x10 - 0x0c];
    i32 m_pos; // +0x10 write index
    char _14[0x16a4 - 0x14];
    i32 m_16a4; // +0x16a4 byte budget A
    char _16a8[0x16ac - 0x16a8];
    i32 m_16ac; // +0x16ac byte budget B
    u8 m_accLo; // +0x16b0 accumulator low byte
    u8 m_accHi; // +0x16b1 accumulator high byte
    char _16b2[0x16b4 - 0x16b2];
    i32 m_bits; // +0x16b4 bit position in accumulator
};

// The mid-marker flush (0x18a190, __cdecl over the state); reloc-masked.
extern "C" void BitFlush(BitState* o); // 0x18a190

// Accumulate `nbits` of `code` into the 16-bit accumulator; when it would pass
// bit 16 spill the two accumulator bytes to the output and keep the overflow.
static __inline void PutBits(BitState* o, u32 code, i32 nbits) {
    i32 bitpos = o->m_bits;
    u16* acc = (u16*)&o->m_accLo;
    if (bitpos > 16 - nbits) {
        *acc |= (u16)(code << bitpos);
        o->m_buf[o->m_pos] = o->m_accLo;
        o->m_pos++;
        o->m_buf[o->m_pos] = o->m_accHi;
        o->m_pos++;
        *acc = (u16)(code >> (16 - bitpos));
        o->m_bits = bitpos + nbits - 16;
    } else {
        *acc |= (u16)(code << bitpos);
        o->m_bits = bitpos + nbits;
    }
}

RVA(0x00188860, 0x267)
void BitEmitMarker(BitState* o) {
    PutBits(o, 2, 3);
    PutBits(o, g_bitMarker.code, g_bitMarker.len);
    o->m_16a4 += 10;
    BitFlush(o);
    if (o->m_16ac - o->m_bits + 11 < 9) {
        PutBits(o, 2, 3);
        PutBits(o, g_bitMarker.code, g_bitMarker.len);
        o->m_16a4 += 10;
        BitFlush(o);
    }
    o->m_16ac = 7;
}
