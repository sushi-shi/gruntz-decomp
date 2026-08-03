#ifndef UTILS_BITARRAYWORD_H
#define UTILS_BITARRAYWORD_H

#include <Enums.h>

// The word a bit array is packed into: 32 bits, indexed by shifting 5 and
// masked by 0x1f.
//
// Deliberately its OWN domain rather than TileGeometry's, even though the three
// numbers are identical. Nothing here is a coordinate; the shift divides a BIT
// index into a word index, and the mask picks the bit within that word:
//
//   p[idx >> BITARRAY_WORD_SHIFT] |= 1 << (idx & BITARRAY_BIT_MASK)
//
// CBitArray::Resize proves the width from the other side - it converts a word
// count back with `m_capacity = ndwords * BITARRAY_WORD_BITS`, and rounds up
// with `((nbits & BITARRAY_BIT_MASK) != 0) + (nbits >> BITARRAY_WORD_SHIFT)`.
GZ_ENUM_CONST_BEGIN(BitArrayWord)
    BITARRAY_WORD_BITS = 32,
    BITARRAY_WORD_SHIFT = 5,
    BITARRAY_BIT_MASK = 0x1f
GZ_ENUM_CONST_END(BitArrayWord)

#endif // UTILS_BITARRAYWORD_H
