// Blowfish.cpp - the Blowfish block cipher (Bruce Schneier reference design),
// statically linked into GRUNTZ.EXE. A single global key (P-array @ 0x61aeb0 +
// four contiguous S-boxes @ 0x61aef8) is seeded by InitializeBlowfish from the
// const digits-of-pi init tables (0x61bef8 / 0x61bf40) and then used by the
// two-pointer encipher/decipher routines.
//
// Recovered from src/Stub/Discovered.cpp's ClassUnknown_18 (encipher, 0x16f7f0)
// / ClassUnknown_19 (decipher, 0x16fc70) / ClassUnknown_17 (init 0x170100 +
// the 4-byte-key wrapper 0x16f6c0) - these are __cdecl/__stdcall FREE functions
// over the global key, NOT class methods (the trace-new this-pointer membership
// was a false positive: the args are data pointers, the P/S are absolute globals).
// (ClassUnknown_17's third member 0x16f760 is a separate istream/ostream stream
// decrypt loop that calls Blowfish_decipher; it stays stubbed - see Discovered.cpp.)
//
// The Feistel rounds are fully unrolled with the classic OpenSSL F macro
//   F(x) = ((S0[x>>24] + S1[(x>>16)&0xff]) ^ S2[(x>>8)&0xff]) + S3[x&0xff]
// MSVC5 /O2 spills the running half to the stack and pulls the middle bytes via
// BYTE PTR [esp+n] / dh,bh - the documented byte-extraction codegen.
#include <Ints.h>
#include <rva.h>

#include <Crypto/Blowfish.h>

// The global key state (zero-init -> .bss). The four 256-entry S-boxes are
// contiguous immediately after S[0], so a flat 1024-entry view (0x000/0x100/
// 0x200/0x300 quadrants) drives the F macro exactly as retail addresses them.
// P-array @ 0x21aeb0, S-boxes @ 0x21aef8 (RVA); DATA-pinned so the reloc targets
// bind to their retail rvas (clang & cl agree on the non-const array mangling).
DATA(0x0021aeb0)
u32 g_bfP[18];
DATA(0x0021aef8)
u32 g_bfS[4][256];

// The const digits-of-pi init tables InitializeBlowfish copies into the runtime
// key state (rep movs sources). P-init @ 0x21bef8 (18 dwords), S-init @ 0x21bf40
// (1024 dwords); reloc-masked so the `mov esi,addr` immediates match retail.
// Bound via @data-symbol (not DATA) because clang mangles the const-extern array
// with a `Q` storage class while cl 5.0 emits `P` (?g_bfInitP@@3PBIB), so a DATA()
// label's clang mangledName would miss the base obj's undefined external. The
// @data-symbol name is the exact cl mangling and is authority-checked against it.
// @data-symbol: ?g_bfInitP@@3PBIB 0x0021bef8
extern const u32 g_bfInitP[18];
// @data-symbol: ?g_bfInitS@@3PAY0BAA@$$CBIA 0x0021bf40
extern const u32 g_bfInitS[4][256];

#define BF_S ((u32*)g_bfS)

// One Feistel round: LL ^= P; LL ^= F(R). Computed as retail does it -
// (S[0x100+b] + S[0x000+a]) is a commutative add the optimizer reorders.
#define BF_ENC(LL, R, P)                                                                           \
    (LL ^= (P),                                                                                    \
     LL ^=                                                                                         \
     (((BF_S[0x000 + ((R) >> 24)] + BF_S[0x100 + (((R) >> 16) & 0xff)])                            \
       ^ BF_S[0x200 + (((R) >> 8) & 0xff)])                                                        \
      + BF_S[0x300 + ((R) & 0xff)]))

RVA(0x0016f7f0, 0x47b)
void Blowfish_encipher(u32* xl, u32* xr) {
    u32 l = *xl;
    u32 r = *xr;

    l ^= g_bfP[0];
    BF_ENC(r, l, g_bfP[1]);
    BF_ENC(l, r, g_bfP[2]);
    BF_ENC(r, l, g_bfP[3]);
    BF_ENC(l, r, g_bfP[4]);
    BF_ENC(r, l, g_bfP[5]);
    BF_ENC(l, r, g_bfP[6]);
    BF_ENC(r, l, g_bfP[7]);
    BF_ENC(l, r, g_bfP[8]);
    BF_ENC(r, l, g_bfP[9]);
    BF_ENC(l, r, g_bfP[10]);
    BF_ENC(r, l, g_bfP[11]);
    BF_ENC(l, r, g_bfP[12]);
    BF_ENC(r, l, g_bfP[13]);
    BF_ENC(l, r, g_bfP[14]);
    BF_ENC(r, l, g_bfP[15]);
    BF_ENC(l, r, g_bfP[16]);
    r ^= g_bfP[17];

    *xr = l;
    *xl = r;
}

// @early-stop
// 99.84% - mirror-function scheduling wall: identical macro to the (100%-exact)
// encipher, but MSVC5 chose a different round-1 byte-extract regalloc here
// (extra entry `xor ecx,ecx`, shr via eax not ecx). All 16 rounds' logic is
// byte-exact; only the round-1 prologue schedule diverges. Not source-steerable.
// See docs/patterns/mirror-function-divergent-schedule.md.
RVA(0x0016fc70, 0x48e)
void Blowfish_decipher(u32* xl, u32* xr) {
    u32 l = *xl;
    u32 r = *xr;

    l ^= g_bfP[17];
    BF_ENC(r, l, g_bfP[16]);
    BF_ENC(l, r, g_bfP[15]);
    BF_ENC(r, l, g_bfP[14]);
    BF_ENC(l, r, g_bfP[13]);
    BF_ENC(r, l, g_bfP[12]);
    BF_ENC(l, r, g_bfP[11]);
    BF_ENC(r, l, g_bfP[10]);
    BF_ENC(l, r, g_bfP[9]);
    BF_ENC(r, l, g_bfP[8]);
    BF_ENC(l, r, g_bfP[7]);
    BF_ENC(r, l, g_bfP[6]);
    BF_ENC(l, r, g_bfP[5]);
    BF_ENC(r, l, g_bfP[4]);
    BF_ENC(l, r, g_bfP[3]);
    BF_ENC(r, l, g_bfP[2]);
    BF_ENC(l, r, g_bfP[1]);
    r ^= g_bfP[0];

    *xr = l;
    *xl = r;
}

// InitializeBlowfish(key, keybytes) - seed the global P/S from the digits-of-pi
// init tables, fold the key into P[] (4 bytes at a time, index wrapping `% keybytes`
// via idiv), then re-encipher an all-zero block forward through P[0..17] and the
// four S-boxes. Classic Schneier reference key schedule.
RVA(0x00170100, 0x104)
i16 InitializeBlowfish(u8* key, i16 keybytes) {
    i16 i, j;
    u32 data, datal, datar;

    for (i = 0; i < 18; i++) {
        g_bfP[i] = g_bfInitP[i];
    }
    for (i = 0; i < 1024; i++) {
        BF_S[i] = ((const u32*)g_bfInitS)[i];
    }

    j = 0;
    for (i = 0; i < 18; i++) {
        data = ((u32)key[j] << 24) | ((u32)key[(j + 1) % keybytes] << 16)
               | ((u32)key[(j + 2) % keybytes] << 8) | (u32)key[(j + 3) % keybytes];
        g_bfP[i] ^= data;
        j = (j + 4) % keybytes;
    }

    datal = 0;
    datar = 0;
    for (i = 0; i < 18; i += 2) {
        Blowfish_encipher(&datal, &datar);
        g_bfP[i] = datal;
        g_bfP[i + 1] = datar;
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 256; j += 2) {
            Blowfish_encipher(&datal, &datar);
            g_bfS[i][j] = datal;
            g_bfS[i][j + 1] = datar;
        }
    }
    return 0;
}

// A thin key wrapper: InitializeBlowfish(key, 4) - seeds the global key from a
// 4-byte key. __stdcall (callee-cleanup ret 4) over the single key pointer.
RVA(0x0016f6c0, 0x12)
void __stdcall Blowfish_InitKey(u8* key) {
    InitializeBlowfish(key, 4);
}

// Copy_16f6e0 (0x16f6e0) lives in its own TU (BlowfishCopy.cpp) so its <iostream.h>
// dependency does not perturb Blowfish_decipher's fragile mirror schedule in this unit.
