#ifndef GRUNTZ_UTILS_BLOWFISH_H
#define GRUNTZ_UTILS_BLOWFISH_H

#include <Ints.h>

extern u32 g_bfP[18];
extern u32 g_bfS[4][256];

extern u32 g_bfInitP[18];
extern u32 g_bfInitS[4][256];

// One 8-byte Blowfish block. The cipher works on it as the two dwords {xl, xr};
// the istream/ostream interface reads and writes the very same bytes as a char
// buffer, and the 1-byte tail block's length is the first byte SIGN-extended
// (retail `movsx`). All three readings are real, so they are named here instead of
// punned at the stream boundary.
union BlowfishBlock {
    u32 m_w[2];            // the cipher pair {xl, xr}
    char m_bytes[8];       // the istream::read / ostream::write buffer
    signed char m_lenByte; // the 1-byte tail block's length
};

void Blowfish_encipher(u32* xl, u32* xr);
void Blowfish_decipher(u32* xl, u32* xr);
i16 InitializeBlowfish(const char* key, i16 keybytes);
void __stdcall Blowfish_InitKey(const char* key);

#endif // GRUNTZ_UTILS_BLOWFISH_H
