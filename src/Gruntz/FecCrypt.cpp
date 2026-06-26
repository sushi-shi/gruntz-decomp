// FecCrypt.cpp - the two leaf string-obfuscation routines that sit between the
// Font cluster and the "FEC File" reader (0x17b5f0 references the "Opened FEC
// File %s" diagnostics). A simple alternating-byte cipher: even-index bytes are
// biased by 0x4f, odd-index bytes by 0x53. Encode/Decode are pure leaf functions
// (no relocs; inline strlen via the /Oi `repnz scasb` intrinsic), __stdcall.
//
// The loop index is a `unsigned short` kept in a full register and masked to
// 16-bit on use (the `and reg,0xffff`); `i % 2` promotes it to int, so the
// even/odd test lowers to MSVC's signed `% 2` idiom (cdq/xor/sub) even though the
// value is always >= 0. Names are placeholders; offsets + code bytes are the
// load-bearing fact.
#include <rva.h>
#include <string.h> // strlen (inlined as repnz scasb at /O2 /Oi)

// ===========================================================================
// 0x17bf70 - Encode(src, dst): dst[i] = src[i] + (i odd ? 0x53 : 0x4f), for the
// whole NUL-terminated src (strlen recomputed each iteration). __stdcall.
// ===========================================================================
RVA(0x0017bf70, 0x65)
void __stdcall FecEncode(const char* src, char* dst) {
    for (unsigned short i = 0; i < strlen(src); i++) {
        if (i % 2 == 0) {
            dst[i] = src[i] + 0x4f;
        } else {
            dst[i] = src[i] + 0x53;
        }
    }
}

// ===========================================================================
// 0x17bfe0 - Decode(src, dst, len): dst[i] = src[i] - (i odd ? 0x53 : 0x4f) for
// `len` bytes, then NUL-terminate dst[len]. `len` is a WORD. __stdcall.
// ===========================================================================
// @early-stop
// entropy tail (~99.75%): every instruction is byte-identical except the final
// `dst[len]=0` store, where retail bases the SIB on len (ebp) and indexes by dst,
// while cl bases on dst and indexes by len (same address, 1-byte-different
// encoding). MSVC canonicalizes `len[dst]` back to `dst+len`, so the base/index
// pick is not source-steerable.
RVA(0x0017bfe0, 0x5d)
void __stdcall FecDecode(const char* src, char* dst, unsigned short len) {
    for (unsigned short i = 0; i < len; i++) {
        if (i % 2 == 0) {
            dst[i] = src[i] - 0x4f;
        } else {
            dst[i] = src[i] - 0x53;
        }
    }
    dst[len] = 0;
}
