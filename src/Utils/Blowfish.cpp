// Blowfish.cpp - the Blowfish block cipher (Bruce Schneier reference design),
// statically linked into GRUNTZ.EXE. A single global key (P-array @ 0x61aeb0 +
// four contiguous S-boxes @ 0x61aef8) is seeded by InitializeBlowfish from the
// const digits-of-pi init tables (0x61bef8 / 0x61bf40) and then used by the
// two-pointer encipher/decipher routines.
//
// Recovered from src/Stub/Discovered.cpp's ClassUnknown_18 (encipher, 0x16f7f0)
// / ClassUnknown_19 (decipher, 0x16fc70) / ClassUnknown_17 (init, 0x170100) -
// these are __cdecl FREE functions over the global key, NOT class methods (the
// trace-new this-pointer membership was a false positive: the args are two
// data pointers, the P/S are absolute globals).
//
// The Feistel rounds are fully unrolled with the classic OpenSSL F macro
//   F(x) = ((S0[x>>24] + S1[(x>>16)&0xff]) ^ S2[(x>>8)&0xff]) + S3[x&0xff]
// MSVC5 /O2 spills the running half to the stack and pulls the middle bytes via
// BYTE PTR [esp+n] / dh,bh - the documented byte-extraction codegen.
#include <Ints.h>
#include <rva.h>

#include <Utils/Blowfish.h>

// The global key state (zero-init -> .bss). The four 256-entry S-boxes are
// contiguous immediately after S[0], so a flat 1024-entry view (0x000/0x100/
// 0x200/0x300 quadrants) drives the F macro exactly as retail addresses them.
u32 g_bfP[18];
u32 g_bfS[4][256];

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
