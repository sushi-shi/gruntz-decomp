#ifndef GRUNTZ_BUTE_BUTETOKEN_H
#define GRUNTZ_BUTE_BUTETOKEN_H

#include <Enums.h>

// The Bute lexer's token type (CButeMgr::m_tokType, and ScanToken's expected
// type). Every name below is read off what the parser DOES with the token:
// the value-producing arms of ParseAttributeLine each build one CButeValue
// kind, and the structural ones are what ScanToken demands at a known position.
//
//   0  the lexer's failure state - every caller treats it as "no token"
//   1  ends the scan: `if (m_tokType == 1) return true;` after SkipToTag
//   2  opens a tag line: ParseGroup enters (and re-enters) its tag loop on it,
//      and SkipToTag scans forward until it sees one
//   3  closes a tag line: the last thing ParseTagLine requires
//   4  the tag/key name: ParseTagLine takes m_token as m_tagName right after it
//   5  the separator before a value
//   6/7 -> atoi()  -> BUTE_INT     8 -> atof() -> BUTE_DOUBLE
//   9  -> BUTE_STRING   10 -> BUTE_RECT   11 -> BUTE_POINT
//   12 -> BUTE_VECTOR   13 -> BUTE_RANGE
//   14 the `Dword` keyword (then demands 6)   15 the `Float` keyword (then 8)
//
// INFERRED, not proven: 6 vs 7. Both feed atoi, but only 6 is accepted after the
// `Dword` keyword, so 7 is the integer form `Dword` rejects - a signed literal.
// Rename it if the transition table says otherwise.
GZ_ENUM_BEGIN_SPLIT(ButeToken, i16)
    BUTETOK_NONE = 0,
    BUTETOK_END = 1,
    BUTETOK_TAG_OPEN = 2,
    BUTETOK_TAG_CLOSE = 3,
    BUTETOK_NAME = 4,
    BUTETOK_ASSIGN = 5,
    BUTETOK_INT = 6,
    BUTETOK_INT_SIGNED = 7,
    BUTETOK_DOUBLE = 8,
    BUTETOK_STRING = 9,
    BUTETOK_RECT = 10,
    BUTETOK_POINT = 11,
    BUTETOK_VECTOR = 12,
    BUTETOK_RANGE = 13,
    BUTETOK_KEYWORD_DWORD = 14,
    BUTETOK_KEYWORD_FLOAT = 15
GZ_ENUM_END_SPLIT(ButeToken, i16)

#endif // GRUNTZ_BUTE_BUTETOKEN_H
