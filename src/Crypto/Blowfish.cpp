#include <Ints.h>
#include <rva.h>

#include <Crypto/Blowfish.h>

#include <Crypto/BlowfishPi.h>

#include <string.h> // memcpy - retail's S-box reload is one inline `rep movsd` of 0x400 dwords

DATA(0x0021aeb0)
u32 g_bfP[18] = BF_PI_P_INIT;
DATA(0x0021aef8)
u32 g_bfS[4][256] = BF_PI_S_INIT;
DATA(0x0021bef8)
u32 g_bfInitP[18] = BF_PI_P_INIT;
DATA(0x0021bf40)
u32 g_bfInitS[4][256] = BF_PI_S_INIT;

#define BF_ENC(LL, R, P)                                                                           \
    (LL ^= (P),                                                                                    \
     LL ^=                                                                                         \
     (((g_bfS[0][(R) >> 24] + g_bfS[1][((R) >> 16) & 0xff]) ^ g_bfS[2][((R) >> 8) & 0xff])         \
      + g_bfS[3][(R) & 0xff]))

// @early-stop
// the other arm of the decipher mirror wall below (see its note): with the
// memcpy S-box reload retail's decipher schedule is the one cl emits, so this
// twin sits at 60.41. Its own MAX (100.00) is banked from the loop spelling.
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
// 99.875 - mirror-function scheduling wall, and it is a ONE-OR-THE-OTHER wall:
// encipher and decipher share this macro verbatim, but retail's two bodies chose
// DIFFERENT round-1 byte-extract regallocs (which of eax/ecx is zeroed on entry
// and which one takes the `shr ..,0x18`), and cl emits one schedule for both.
// MEASURED 2026-07-29, four builds, all cast-free variants:
//   S-box declared u32[1024] + the flat 1024-iteration copy loop
//        -> encipher 100.00, decipher 61.51, InitializeBlowfish 99.89
//   S-box declared u32[4][256] + memcpy (either subscript spelling)
//        -> encipher  60.41, decipher 99.875, InitializeBlowfish 100.00
// The subscript form (`g_bfS[1][x]` vs `g_bfS[0][0x100+x]`) is NOT the steer -
// both give byte-identical output; the steer is the copy spelling above, i.e. a
// TU-cumulative optimizer-state effect from a sibling function. The memcpy arm is
// kept: it banks the higher decipher MAX, takes InitializeBlowfish to EXACT, and
// is the shape retail's own `mov ecx,0x400; rep movsd` proves.
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
i16 InitializeBlowfish(const char* key, i16 keybytes) {
    i16 i, j;
    u32 data, datal, datar;

    for (i = 0; i < 18; i++) {
        g_bfP[i] = g_bfInitP[i];
    }
    memcpy(g_bfS, g_bfInitS, sizeof(g_bfS));

    j = 0;
    for (i = 0; i < 18; i++) {
        // the schedule reads the key as UNSIGNED bytes (retail zero-extends each one:
        // `xor eax,eax; mov al,BYTE PTR [...]`), so each byte is widened here.
        data = (static_cast<u32>(static_cast<u8>(key[j])) << 24)
               | (static_cast<u32>(static_cast<u8>(key[(j + 1) % keybytes])) << 16)
               | (static_cast<u32>(static_cast<u8>(key[(j + 2) % keybytes])) << 8)
               | static_cast<u32>(static_cast<u8>(key[(j + 3) % keybytes]));
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
void __stdcall Blowfish_InitKey(const char* key) {
    InitializeBlowfish(key, 4);
}
