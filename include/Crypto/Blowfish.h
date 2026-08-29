#ifndef GRUNTZ_UTILS_BLOWFISH_H
#define GRUNTZ_UTILS_BLOWFISH_H

#include <Ints.h>

void Blowfish_encipher(u32* xl, u32* xr);
void Blowfish_decipher(u32* xl, u32* xr);
i16 InitializeBlowfish(const char* key, i16 keybytes);

#endif // GRUNTZ_UTILS_BLOWFISH_H
