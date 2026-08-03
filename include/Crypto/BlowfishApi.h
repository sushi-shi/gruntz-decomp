#ifndef CRYPTO_BLOWFISHAPI_H
#define CRYPTO_BLOWFISHAPI_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>

void Blowfish_encipher(u32* xl, u32* xr);

class istream;
class ostream;

void __stdcall BitStreamBlowfishEncode(istream* src, ostream* dst);

#endif // CRYPTO_BLOWFISHAPI_H
