#include <rva.h>

#include <Crypto/BitStreamBlowfish.h>

#include <Bute/ButeMgr.h>
#include <Crypto/Blowfish.h>

#include <iostream.h>

RVA(0x0016f760, 0x82)
void CButeTail::Decode(istream* in, ostream* out) {

    BlowfishBlock blk[2];
    bool first = true;
    while (!in->eof()) {
        in->read(blk[0].m_bytes, 8);
        int sample = in->gcount();
        if (sample == 1) {

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
