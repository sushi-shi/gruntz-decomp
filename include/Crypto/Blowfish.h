// Blowfish.h - Bruce Schneier's reference Blowfish cipher (the engine's shared
// crypto, used for save-game / network payload obfuscation).
//
// The P-array and the four 256-entry S-boxes are GLOBAL tables, statically
// initialized to the standard Blowfish digits-of-pi constants and then mixed
// with the key by Blowfish_Init. The reference signatures are __cdecl free
// functions over `unsigned long *` block halves (u32 here, per the int-alias
// convention - matching-neutral, all 32-bit).
//
//   P-array  @ 0x0061aeb0  (18 u32)
//   S-box 0  @ 0x0061aef8  (256 u32)
//   S-box 1  @ 0x0061b2f8  (256 u32)
//   S-box 2  @ 0x0061b6f8  (256 u32)
//   S-box 3  @ 0x0061baf8  (256 u32)
#ifndef GRUNTZ_CRYPTO_BLOWFISH_H
#define GRUNTZ_CRYPTO_BLOWFISH_H

#include <rva.h>

// The 18-entry P-array (subkeys) and the four S-boxes. Each P[i] / S[j] base is
// referenced by a constant index, so every load reloc-masks to its DATA symbol.
DATA(0x0061aeb0)
extern u32 g_bfP[18];
DATA(0x0061aef8)
extern u32 g_bfS0[256];
DATA(0x0061b2f8)
extern u32 g_bfS1[256];
DATA(0x0061b6f8)
extern u32 g_bfS2[256];
DATA(0x0061baf8)
extern u32 g_bfS3[256];

void Blowfish_encipher(u32* xl, u32* xr);
void Blowfish_decipher(u32* xl, u32* xr);

#endif // GRUNTZ_CRYPTO_BLOWFISH_H
