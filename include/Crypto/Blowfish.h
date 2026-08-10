#ifndef GRUNTZ_UTILS_BLOWFISH_H
#define GRUNTZ_UTILS_BLOWFISH_H

#include <Ints.h>

extern u32 g_bfP[18];
extern u32 g_bfS[4][256];

extern u32 g_bfInitP[18];
extern u32 g_bfInitS[4][256];

union BlowfishBlock {
    u32 m_w[2];
    char m_bytes[8];
    signed char m_lenByte;
};

void Blowfish_encipher(u32* xl, u32* xr);
void Blowfish_decipher(u32* xl, u32* xr);
i16 InitializeBlowfish(const char* key, i16 keybytes);

#endif // GRUNTZ_UTILS_BLOWFISH_H
