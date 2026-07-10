// BitStreamBlowfish.cpp (0x16f760): a __stdcall free function that decodes a
// Blowfish-ciphered sample stream. It pulls 8-byte blocks from a CRT <iostream.h>
// `istream`, emits the PREVIOUS decrypted right-half through a CRT `ostream` (skipping
// the priming block), Blowfish_decipher's the (xl,xr) pair, then chains. It loops until
// the reader hits EOF (`istream::eof()`, the ios virtual-base state & eofbit).
//
// The stream classes are the statically-linked CRT iostreams (recovered identity):
// istream::read @0x16a510, ostream::write @0x16ab20, and `x_gcount` (istream+0x8, read
// via gcount()) is the count of bytes the last read() consumed. Blowfish_decipher
// resolves to the matched `blowfish` unit (reloc-masked).
#include <rva.h>
#include <iostream.h>

// matched in the `blowfish` unit (?Blowfish_decipher@@YAXPAI0@Z)
void Blowfish_decipher(unsigned int* xl, unsigned int* xr);

RVA(0x0016f760, 0x82)
void __stdcall BitStreamBlowfishDecode(istream* in, ostream* out) {
    unsigned int blk[4];
    bool first = true;
    while (!in->eof()) {
        in->read((char*)&blk[0], 8);
        int sample = in->gcount();
        if (sample == 1) {
            sample = *(signed char*)&blk[0];
        }
        if (!first) {
            out->write((const char*)&blk[3], sample);
        } else {
            first = false;
        }
        Blowfish_decipher(&blk[0], &blk[1]);
        blk[2] = blk[0];
        blk[3] = blk[1];
    }
}
