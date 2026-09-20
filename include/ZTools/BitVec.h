#ifndef GRUNTZ_ZTOOLS_BITVEC_H
#define GRUNTZ_ZTOOLS_BITVEC_H

#include <Utils/BitArrayWord.h>
#include <ZTools/Error.h>

class istream;
class ostream;

extern i32 g_defaultProjActSize;

class zBitVec : public zErrHandling {
public:
    zBitVec();
    zBitVec(i32 idx, i32 sizehint);
    zBitVec(const char* tokens, i32 minSize);
    zBitVec& operator=(const zBitVec& o);
    virtual ~zBitVec() OVERRIDE;
    i32 SetSize(i32 nbits);
    i32 EnsureSize(i32 nbits);
    zBitVec* Or(zBitVec* o);
    zBitVec* SetBit(u32 idx);

    i32 m_capacity;

    union {
        u32* m_words;
        u32 m_inline;
    };

    i32 GetBit(u32 idx) const {
        if (idx >= static_cast<u32>(m_capacity)) {
            return 0;
        }
        const u32* words = static_cast<u32>(m_capacity) > BITARRAY_WORD_BITS ? m_words : &m_inline;
        return (words[idx >> BITARRAY_WORD_SHIFT] & (1 << (idx & BITARRAY_BIT_MASK))) != 0;
    }

    zBitVec* Set(u32 idx) {
        if (idx >= static_cast<u32>(m_capacity)) {
            return SetBit(idx);
        }
        u32* words = static_cast<u32>(m_capacity) > BITARRAY_WORD_BITS ? m_words : &m_inline;
        words[idx >> BITARRAY_WORD_SHIFT] |= 1 << (idx & BITARRAY_BIT_MASK);
        return this;
    }
};

ostream& operator<<(ostream& accum, const zBitVec& bits);
istream& operator>>(istream& accum, zBitVec& bits);

#endif // GRUNTZ_ZTOOLS_BITVEC_H
