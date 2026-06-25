// Blowfish.h - the Blowfish block cipher (Bruce Schneier's reference design),
// statically linked into GRUNTZ.EXE and operating on a single global key
// (P-array + four S-boxes). Used by the engine to obfuscate on-disk data.
//
// The implementation is the classic two-pointer reference form
// `Blowfish_encipher(u32* xl, u32* xr)` / `Blowfish_decipher(...)`, fully
// unrolled (16 Feistel rounds) with the OpenSSL-style F macro, plus
// `InitializeBlowfish(key, keybytes)` which seeds the global P/S from the
// digits-of-pi init tables and key-mixes.
#ifndef GRUNTZ_UTILS_BLOWFISH_H
#define GRUNTZ_UTILS_BLOWFISH_H

#include <Ints.h>

// The single global key state: P[18] subkeys followed by S[4][256] boxes,
// contiguous (P @ 0x61aeb0, S @ 0x61aef8 in retail). Filled at runtime by
// InitializeBlowfish; zero-initialized in the image (.bss).
extern u32 g_bfP[18];
extern u32 g_bfS[4][256];

// The const digits-of-pi init tables (P @ 0x61bef8, S @ 0x61bf40 in retail) that
// InitializeBlowfish copies into the runtime key state.
extern const u32 g_bfInitP[18];
extern const u32 g_bfInitS[4][256];

void Blowfish_encipher(u32* xl, u32* xr);
void Blowfish_decipher(u32* xl, u32* xr);
i16 InitializeBlowfish(u8* key, i16 keybytes);
void __stdcall Blowfish_InitKey(u8* key);

#endif // GRUNTZ_UTILS_BLOWFISH_H
