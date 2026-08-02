#include <Crypto/BlowfishCopy.h>
#include <Bute/ButeMgr.h>
#include <Crypto/Blowfish.h>
#include <Ints.h>
#include <rva.h>
#include <iostream.h>
#include <string.h>

RVA(0x0016f6e0, 0x76)
void CButeTail::Encode(istream* src, ostream* dst) {
    i32 last = 0;
    while (!src->eof()) {
        BlowfishBlock rec;

        memset(rec.m_bytes, 0, 8);
        src->read(rec.m_bytes, 8);
        last = src->gcount();
        Blowfish_encipher(&rec.m_w[0], &rec.m_w[1]);
        dst->write(rec.m_bytes, 8);
    }
    dst->put(static_cast<unsigned char>(last));
}
