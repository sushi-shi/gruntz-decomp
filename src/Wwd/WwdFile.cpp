// WwdFile.cpp - Gruntz WWD level-file loader (Monolith-faithful reconstruction).
//
// WIP near-match: WwdFile::InflateMainBlock @ RVA 0x160790 (objdiff ~88.7% fuzzy).
// Validates the WWD header, copies the 1524-byte header prefix into the caller
// buffer, then zlib-uncompresses the deflated main block (planes/tiles) into the
// buffer right after the header. Returns the dest buffer on success, else 0.
//
// __stdcall (callee cleans 12 bytes; ret 0xc in the binary). The function is
// `WwdFile::InflateMainBlock` in the engine but takes no `this` and uses
// callee-cleanup, so it reconstructs as a __stdcall free function.
//
// STATUS: algorithm/structure are byte-faithful (every conditional, the
// branch polarities, the field re-reads, the inline memcpy, the uncompress call
// and the `outLen == mainBlockLength ? dest : 0` return all reproduce). The
// residual ~11% is a single MSVC5 register-allocation choice we could not steer
// from C: the original assigns the one spare callee-saved register (ebp) to
// `destLen` and keeps `dest` in volatile edx (spilled across the call), whereas
// `cl` here gives ebp to `dest`. That cascades into add-vs-lea pointer arithmetic
// and the spill schedule. This is the documented allocation/entropy tail
// (docs/matching-patterns.md). Reading the header fields DIRECTLY (not via a
// cached `sig` local) was the key step that pinned wwdSignature->ecx /
// mainBlockLength->eax and lifted the match from ~84% to ~88.7%.
//
// NOTE: an earlier reconstruction inverted the size check; the binary CONTINUES
// (not returns) when mainBlockLength <= destLen+sig, i.e. it returns 0 only when
// mainBlockLength > destLen + sig. That polarity is now correct below.
#include "WwdFile.h"

#include <string.h>  // memcpy

int __stdcall WwdFile_InflateMainBlock(WwdHeader* src, Bytef* dest, unsigned int destLen)
{
    uLongf outLen;

    if (src == 0)
        return 0;
    if (dest == 0)
        return 0;

    if (src->wwdSignature > 0x5f4)           // header size (== 1524)
        return 0;
    if ((src->flags & 0x2) == 0)             // require COMPRESS
        return 0;
    if (src->mainBlockLength == 0)
        return 0;
    if (src->mainBlockLength > destLen + src->wwdSignature)
        return 0;

    memcpy(dest, src, src->wwdSignature);     // copy the 1524-byte header prefix
    outLen = (uLongf)(destLen - src->wwdSignature);
    if (uncompress(dest + src->wwdSignature, &outLen,
                   (Bytef*)src + src->wwdSignature, src->mainBlockLength) != 0)
        return 0;

    return outLen == src->mainBlockLength ? (int)dest : 0;
}
