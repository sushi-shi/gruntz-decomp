// CRangeSet.cpp - a small fixed-capacity inclusive-range set (capacity 15) that
// lives in the 0x1845b0..0x185700 menu cluster. The object is { u32 m_count; then
// up to 16 { u32 lo; u32 hi } pairs starting at +0x04 }. AddRange appends a pair
// while count+1 < 16; Contains scans for the first pair whose [lo,hi] (unsigned)
// brackets the probe value (both __thiscall leaves with no externals). AddFromString
// parses a marker-delimited number/range string and AddRange's each token (its
// strstr/strpbrk/atol callees are reloc-masked).
//
// RTTI name does not survive; the class name is a placeholder (campaign doctrine -
// matching-neutral). Only offsets + code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>
#include <stdlib.h> // atol
#include <string.h> // inline strcpy (rep movs / repne scasb), strpbrk

// The strstr-class helper at 0x120090 ("SubstringMatch" - the campaign name for
// this engine strstr). Returns the position of the marker in the string (used as
// a char* cursor here). __cdecl, haystack first. Reloc-masked.
extern "C" char* SubstringMatch(const char* haystack, const char* needle);

struct CRange {
    u32 lo; // +0x00
    u32 hi; // +0x04
};

class CRangeSet {
public:
    bool Contains(u32 value);      // 0x184ba0
    void AddRange(u32 lo, u32 hi); // 0x184be0
    void AddFromString(char* str); // 0x184c10

    u32 m_count;        // +0x00
    CRange m_pairs[16]; // +0x04
};

// ===========================================================================
// 0x184ba0 - Contains(value): true if any stored [lo,hi] range (unsigned)
// includes value. Walks m_pairs[0..m_count).
// ===========================================================================
RVA(0x00184ba0, 0x33)
bool CRangeSet::Contains(u32 value) {
    for (u32 i = 0; i < m_count; i++) {
        if (value >= m_pairs[i].lo && value <= m_pairs[i].hi) {
            return true;
        }
    }
    return false;
}

// ===========================================================================
// 0x184be0 - AddRange(lo, hi): append a pair if there is room (count+1 < 16).
// m_count is re-read after each store (the member array may alias it).
// ===========================================================================
RVA(0x00184be0, 0x24)
void CRangeSet::AddRange(u32 lo, u32 hi) {
    if (m_count + 1 < 16) {
        m_pairs[m_count].lo = lo;
        m_pairs[m_count].hi = hi;
        m_count = m_count + 1;
    }
}

// ===========================================================================
// 0x184c10 - AddFromString(str): parse a marker-delimited number/range list
// ("X<n>" or "X<lo>-<hi>" tokens) and AddRange each. For every 'X' marker found,
// skip to the first digit, extract the leading digit run into a scratch buffer
// (truncating at the first non-digit while advancing the cursor in parallel),
// atol it, and - if a '-' follows - parse the upper bound the same way; otherwise
// the range is a single value. Stops at end-of-string or a missing marker/digit.
// ===========================================================================
// @early-stop
// ~92.15% loop-carried-cursor regalloc wall (docs/patterns/shrink-wrapped-callee-
// save-push.md / this-spilled-to-local-for-loop-seed.md family): the ENTIRE loop
// body (inline strcpy, digit-scan, atol, the '-' range branch, AddRange) is
// byte-identical. The only residual is the prologue - retail pins the cursor in
// ebx from entry (`push ebx; mov ebx,[esp+0x10c]`) and keeps it there across every
// iteration, while cl peels the first iteration's cursor into eax (the param is
// dead after the first SubstringMatch). Both `char* s = str;` and reusing the
// `str` param produce the identical eax split; not source-steerable. The lone
// `atol` (0x11ff10) rel32 is also reloc-masked (Ghidra names it `atol`, cl emits
// `_atol`). Logic complete; parked for the final sweep.
RVA(0x00184c10, 0x136)
void CRangeSet::AddFromString(char* str) {
    char buf[0x100];
    if (*str == 0) {
        return;
    }
    do {
        char* x = SubstringMatch(str, "X");
        if (x == 0) {
            return;
        }
        str = strpbrk(x, "0123456789");
        if (str == 0) {
            return;
        }
        strcpy(buf, str);
        char* q = buf;
        while (*q != 0) {
            char c = *q;
            if (c >= '0' && c <= '9') {
                q++;
                str++;
            } else {
                *q = 0;
            }
        }
        i32 lo = atol(buf);
        i32 hi;
        if (*str == '-') {
            str = strpbrk(str, "0123456789");
            if (str == 0) {
                return;
            }
            strcpy(buf, str);
            q = buf;
            while (*q != 0) {
                char c = *q;
                if (c >= '0' && c <= '9') {
                    q++;
                    str++;
                } else {
                    *q = 0;
                }
            }
            hi = atol(buf);
        } else {
            hi = lo;
        }
        AddRange(lo, hi);
    } while (*str != 0);
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CRange);
SIZE_UNKNOWN(CRangeSet);
