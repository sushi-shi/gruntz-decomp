// TimeSplit.cpp - millisecond -> H:M:S splitter (0x119210).
//
// Unsigned division-by-constant: hours = n/3600000, minutes = (n%3600000)/60000,
// seconds = (n%60000)/1000, each written through an out-param. MSVC lowers the three
// constant divides to magic-multiply and strength-reduces the `% C` remainders to
// lea/shift multiplies of the quotient. Owner unidentified (free __cdecl helper).
#include <rva.h>

RVA(0x00119210, 0x66)
void SplitMillisToHMS(unsigned n, unsigned* hh, unsigned* mm, unsigned* ss) {
    unsigned q1 = n / 3600000;
    *hh = q1;
    n -= q1 * 3600000;
    unsigned q2 = n / 60000;
    *mm = q2;
    n -= q2 * 60000;
    *ss = n / 1000;
}
