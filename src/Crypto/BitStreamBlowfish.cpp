#include <rva.h>
#include <iostream.h>

#include <Crypto/BitStreamBlowfish.h>
#include <Crypto/Blowfish.h>

RVA(0x0016f760, 0x82)
void __stdcall BitStreamBlowfishDecode(istream* in, ostream* out) {
    // Two 8-byte blocks: [0] is the block being read/deciphered, [1] the PREVIOUS
    // pass's plaintext (retail's `lea eax,[esp+0x18]` resolves to its base).
    BlowfishBlock blk[2];
    bool first = true;
    while (!in->eof()) {
        in->read(blk[0].m_bytes, 8);
        int sample = in->gcount();
        if (sample == 1) {
            // retail 0x16f760 sign-extends ONE byte out of the block (`movsx`), so the
            // 1-byte tail block's length is the first byte read signed
            sample = blk[0].m_lenByte;
        }
        if (!first) {
            out->write(blk[1].m_bytes, sample);
        } else {
            first = false;
        }
        Blowfish_decipher(&blk[0].m_w[0], &blk[0].m_w[1]);
        blk[1].m_w[0] = blk[0].m_w[0];
        blk[1].m_w[1] = blk[0].m_w[1];
    }
}
