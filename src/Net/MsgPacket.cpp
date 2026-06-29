// MsgPacket.cpp - two __cdecl helpers over a serialization packet whose header is
// 0x38 bytes (data pointer at +0x8, write cursor at +0x10, a "valid" flag at +0x1c).
// Best-guess owner; self-contained (no external relocs), so matching is purely
// offset/code-byte based. Field names are placeholders.
#include <Ints.h>
#include <rva.h>

struct CMsgPacket {
    char m_pad0[8];
    u8* m_8; // +0x8   byte buffer
    char m_padc[0x10 - 0xc];
    i32 m_10; // +0x10  write cursor
    char m_pad14[0x1c - 0x14];
    i32 m_1c; // +0x1c  populated flag
    char m_pad20[0x38 - 0x20];
};

// 0x1868e0 - append a 16-bit value big-endian, advancing the cursor by 2.
RVA(0x001868e0, 0x2f)
void WritePacketU16(CMsgPacket* p, u32 val) {
    p->m_8[p->m_10] = (u8)(val >> 8);
    p->m_10++;
    p->m_8[p->m_10] = (u8)val;
    p->m_10++;
}

// 0x186a20 - copy a populated source header over a destination header; returns -2.
RVA(0x00186a20, 0x28)
i32 CopyPacketHeader(CMsgPacket* dst, CMsgPacket* src) {
    if (src != 0 && dst != 0 && src->m_1c != 0) {
        *dst = *src;
    }
    return -2;
}
