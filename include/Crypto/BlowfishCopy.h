#ifndef CRYPTO_BLOWFISHCOPY_H
#define CRYPTO_BLOWFISHCOPY_H

#include <Mfc.h>
#include <Ints.h>
#include <rva.h>

void Blowfish_encipher(u32* xl, u32* xr);

class istream;
class ostream;

void __stdcall BitStreamBlowfishEncode(istream* src, ostream* dst);

#endif // CRYPTO_BLOWFISHCOPY_H
