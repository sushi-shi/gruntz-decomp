#include <rva.h>
#include <iostream.h>

#include <Crypto/BitStreamBlowfish.h>
#include <Crypto/Blowfish.h>

RVA(0x0016f760, 0x82)
void __stdcall BitStreamBlowfishDecode(istream* in, ostream* out) {
    unsigned int blk[4];
    bool first = true;
    while (!in->eof()) {
        in->read(reinterpret_cast<char*>(&blk[0]), 8);
        int sample = in->gcount();
        if (sample == 1) {
            sample = *reinterpret_cast<signed char*>(&blk[0]);
        }
        if (!first) {
            out->write(reinterpret_cast<const char*>(&blk[3]), sample);
        } else {
            first = false;
        }
        Blowfish_decipher(&blk[0], &blk[1]);
        blk[2] = blk[0];
        blk[3] = blk[1];
    }
}
