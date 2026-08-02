#include <rva.h>

#include <Wap32/zBitVec.h>

#include <ctype.h>
#include <iostream.h>

RVA(0x00193080, 0xb5)
ostream& operator<<(ostream& accum, const zBitVec& bits) {
    if (accum.opfx()) {
        long saved = accum.flags();
        accum << '[';
        i32 first = 1;
        for (u32 i = 0; i < static_cast<u32>(bits.m_capacity); i++) {
            if (bits.GetBit(i)) {
                if (!first) {
                    accum << ' ';
                }
                accum << static_cast<i32>(i);
                first = 0;
            }
        }
        accum << ']';
        accum.flags(saved);
        accum.osfx();
    }
    return accum;
}

RVA(0x00193140, 0x1fa)
istream& operator>>(istream& accum, zBitVec& bits) {
    if (accum.ipfx(0)) {
        i32 bit;
        char ch;
        accum.get(ch);
        if (!accum) {
            return accum;
        }
        if (ch != '[') {
            accum.putback(ch);
            if (isdigit(ch)) {
                accum >> bit;
                bits.Set(bit);
            } else {
                accum.clear(accum.rdstate() | ios::failbit);
            }
            return accum;
        }

        for (;;) {
            accum >> ws;
            if (!accum) {
                return accum;
            }
            bit = -1;
            accum >> bit;
            if (!accum) {
                return accum;
            }
            if (bit == -1) {
                return accum;
            }
            bits.Set(bit);

            accum.get(ch);
            if (!accum) {
                return accum;
            }
            if (ch == ',') {
                continue;
            }
            if (ch == ']') {
                return accum;
            }
            accum.putback(ch);
            if (isdigit(ch)) {
                continue;
            }
            accum.clear(accum.rdstate() | ios::failbit);
            return accum;
        }
    }
    return accum;
}
