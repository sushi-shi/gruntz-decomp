#include <Wap32/zBitVec.h>

#include <ctype.h>
#include <iostream.h>
#include <rva.h>

RVA(0x00193080, 0xb5)
ostream& operator<<(ostream& accum, const zBitVec& bits) {
    accum << '[';
    i32 first = 1;
    for (i32 i = 0; i < bits.m_capacity; i++) {
        const u32* words = static_cast<u32>(bits.m_capacity) > 0x20
                               ? bits.m_words
                               : reinterpret_cast<const u32*>(&bits.m_words);
        if (words[i >> 5] & (1 << (i & 0x1f))) {
            if (!first) {
                accum << ' ';
            }
            accum << i;
            first = 0;
        }
    }
    accum << ']';
    return accum;
}

RVA(0x00193140, 0x1fa)
istream& operator>>(istream& accum, zBitVec& bits) {
    char ch;
    accum.get(ch);
    if (!accum) {
        return accum;
    }
    if (ch != '[') {
        accum.putback(ch);
        if (isdigit(ch)) {
            i32 bit;
            accum >> bit;
            bits.SetBit(bit);
        } else {
            accum.clear(ios::failbit);
        }
        return accum;
    }

    accum >> ws;
    for (;;) {
        i32 bit = -1;
        accum >> bit;
        if (!accum || bit == -1) {
            return accum;
        }
        bits.SetBit(bit);

        accum.get(ch);
        if (!accum || ch == ']') {
            return accum;
        }
        if (ch != ',') {
            accum.putback(ch);
            if (!isdigit(ch)) {
                accum.clear(ios::failbit);
                return accum;
            }
        }
    }
}
