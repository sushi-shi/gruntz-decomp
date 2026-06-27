// TimeSplit.cpp - millisecond -> H:M:S splitter (0x119210).
//
// Unsigned division-by-constant: q1 = n/3600000 (hours), q2 = (n%3600000)/60000
// (minutes), q3 = (n%60000)/1000 (seconds), each written through an out-param.
// MSVC lowers the three constant divides to magic-multiply (0x95217cb1>>21,
// 0x45e7b273>>14, 0x10624dd3>>6) and strength-reduces the `% C` remainders to
// lea/shift multiplies of the quotient.  Owner unidentified (free __cdecl helper).
#include <rva.h>

RVA(0x00119210, 0x66)
void Unmatched_119210(unsigned n, unsigned* hh, unsigned* mm, unsigned* ss) {
    unsigned q1 = n / 3600000;
    *hh = q1;
    n -= q1 * 3600000;
    unsigned q2 = n / 60000;
    *mm = q2;
    n -= q2 * 60000;
    *ss = n / 1000;
}
