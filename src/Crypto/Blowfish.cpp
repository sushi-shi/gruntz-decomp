#include <Ints.h>
#include <rva.h>

#include <Crypto/Blowfish.h>

#include <Crypto/BlowfishPi.h>

DATA(0x0021aeb0)
u32 g_bfP[18] = BF_PI_P_INIT;
DATA(0x0021aef8)
u32 g_bfS[4][256] = BF_PI_S_INIT;
DATA(0x0021bef8)
u32 g_bfInitP[18] = BF_PI_P_INIT;
DATA(0x0021bf40)
u32 g_bfInitS[4][256] = BF_PI_S_INIT;

#define BF_S (reinterpret_cast<u32*>(g_bfS))

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

RVA(0x00170100, 0x104)
i16 InitializeBlowfish(u8* key, i16 keybytes) {
    i16 i, j;
    u32 data, datal, datar;

    for (i = 0; i < 18; i++) {
        g_bfP[i] = g_bfInitP[i];
    }
    for (i = 0; i < 1024; i++) {
        BF_S[i] = (reinterpret_cast<const u32*>(g_bfInitS))[i];
    }

    j = 0;
    for (i = 0; i < 18; i++) {
        data = (static_cast<u32>(key[j]) << 24) | (static_cast<u32>(key[(j + 1) % keybytes]) << 16)
               | (static_cast<u32>(key[(j + 2) % keybytes]) << 8) | static_cast<u32>(key[(j + 3) % keybytes]);
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

RVA(0x0016f6c0, 0x12)
void __stdcall Blowfish_InitKey(u8* key) {
    InitializeBlowfish(key, 4);
}
