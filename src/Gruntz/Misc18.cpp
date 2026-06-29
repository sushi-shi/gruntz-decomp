// Misc18.cpp - two self-contained helpers in the 0x18 (WWD/compression) range.
//   0x186990  RecFree   - releases a record's sub-allocations via a callback ptr.
//   0x18a190  BitFlush  - flushes the 16-bit pack accumulator to the byte stream.
#include <Ints.h>
#include <rva.h>

// --- 0x186990 : free a record + its owned buffers through self->m_24 ----------
struct FreeNode {
    i32 m_00;
    i32 m_04;   // +0x04 type tag
    void* m_08; // +0x08
    char pad0c[0x2c - 0x0c];
    void* m_2c; // +0x2c
    char pad30[0x34 - 0x30];
    void* m_34; // +0x34
    void* m_38; // +0x38
};

struct FreeOwner {
    char pad00[0x1c];
    FreeNode* m_1c; // +0x1c
    char pad20[0x24 - 0x20];
    void(__cdecl* m_24)(void*, void*); // +0x24 free callback
    void* m_28;                        // +0x28 callback context
};

// @early-stop
// regalloc + bool-tail idiom (~76%): logic correct; retail materializes the
// (tag==0x71)?-3:0 via setne/dec/and and reloads self->m_1c per field; the cl
// reload/return scheduling differs.
// 0x186990
RVA(0x00186990, 0x90)
i32 RecFree(FreeOwner* self) {
    if (!self) {
        return -2;
    }
    if (!self->m_1c) {
        return -2;
    }
    if (self->m_1c->m_08) {
        self->m_24(self->m_28, self->m_1c->m_08);
    }
    if (self->m_1c->m_38) {
        self->m_24(self->m_28, self->m_1c->m_38);
    }
    if (self->m_1c->m_34) {
        self->m_24(self->m_28, self->m_1c->m_34);
    }
    if (self->m_1c->m_2c) {
        self->m_24(self->m_28, self->m_1c->m_2c);
    }
    FreeNode* n = self->m_1c;
    i32 tag = n->m_04;
    self->m_24(self->m_28, n);
    self->m_1c = 0;
    return tag == 0x71 ? -3 : 0;
}

// --- 0x18a190 : flush the bit-pack accumulator --------------------------------
struct BitBuf {
    char pad00[8];
    u8* m_08; // +0x08 output stream
    char pad0c[0x10 - 0x0c];
    i32 m_10; // +0x10 byte write index
    char pad14[0x16b0 - 0x14];
    u8 m_16b0; // +0x16b0 accumulator low byte
    u8 m_16b1; // +0x16b1 accumulator high byte
    char pad16b2[0x16b4 - 0x16b2];
    i32 m_16b4; // +0x16b4 pending bit count
};

// @early-stop
// regalloc tie (~84%): logic byte-exact; retail caches m_08 in esi across the two
// byte stores where this cl reloads it.
// 0x18a190
RVA(0x0018a190, 0x8c)
void BitFlush(BitBuf* b) {
    i32 n = b->m_16b4;
    if (n == 0x10) {
        b->m_08[b->m_10] = b->m_16b0;
        b->m_10++;
        b->m_08[b->m_10] = b->m_16b1;
        b->m_10++;
        *(u16*)&b->m_16b0 = 0;
        b->m_16b4 = 0;
    } else if (n >= 8) {
        b->m_08[b->m_10] = b->m_16b0;
        b->m_10++;
        *(u16*)&b->m_16b0 = b->m_16b1;
        b->m_16b4 -= 8;
    }
}

// --- 0x186a50 : reset a coder's state from the per-mode constant table --------
extern "C" void* memset(void* d, i32 c, unsigned int n);

// Four u16 fields of the current mode's record in a 0xc-stride constant table.
extern u16 g_modeTab_e8; // 0x624fe8
extern u16 g_modeTab_ea; // 0x624fea
extern u16 g_modeTab_ec; // 0x624fec
extern u16 g_modeTab_ee; // 0x624fee

struct Coder {
    char pad00[0x20];
    i32 m_20; // +0x20
    char pad24[0x30 - 0x24];
    i32 m_30; // +0x30
    char pad34[0x38 - 0x34];
    u16* m_38; // +0x38 work buffer
    i32 m_3c;  // +0x3c
    i32 m_40;  // +0x40 length
    char pad44[0x50 - 0x44];
    i32 m_50; // +0x50
    i32 m_54; // +0x54
    char pad58[0x5c - 0x58];
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
    char pad64[0x68 - 0x64];
    i32 m_68; // +0x68
    i32 m_6c; // +0x6c
    i32 m_70; // +0x70
    i32 m_74; // +0x74
    i32 m_78; // +0x78 mode index
    char pad7c[0x80 - 0x7c];
    i32 m_80; // +0x80
    i32 m_84; // +0x84
};

// 0x186a50
RVA(0x00186a50, 0x93)
void CoderReset(Coder* c) {
    c->m_30 = c->m_20 << 1;
    c->m_38[c->m_40 - 1] = 0;
    memset(c->m_38, 0, c->m_40 * 2 - 2);
    i32 i = c->m_78 * 0xc;
    c->m_74 = *(u16*)((char*)&g_modeTab_ea + i);
    c->m_80 = *(u16*)((char*)&g_modeTab_e8 + i);
    c->m_84 = *(u16*)((char*)&g_modeTab_ec + i);
    i32 last = *(u16*)((char*)&g_modeTab_ee + i);
    c->m_60 = 0;
    c->m_50 = 0;
    c->m_68 = 0;
    c->m_5c = 0;
    c->m_3c = 0;
    c->m_70 = last;
    c->m_6c = 2;
    c->m_54 = 2;
}
