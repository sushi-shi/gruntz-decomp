#include <rva.h>

#include <zlib.h>

// ===========================================================================
// 0x1853b0 - deflate the source block into dest at Z_DEFAULT_COMPRESSION (-1); on
// success store the produced length back through pDestLen and return deflateEnd's
// result. Returns the init error if deflateInit_ fails, Z_BUF_ERROR (-5) if deflate
// stops at Z_OK without reaching Z_STREAM_END, else the deflate error.
// ===========================================================================
// @early-stop
// ~86% regalloc register-choice wall: control flow and every instruction's shape
// are identical to retail. MSVC pins the long-lived pDestLen in ebx where retail
// uses edi (the allocator's 2nd-callee-saved pick {esi,ebx} vs {esi,edi}), which
// also flips the coupled avail_in/next_in store order at the top. The three zlib
// entry-point rel32 calls are now correctly named (deflateInit_/deflate/deflateEnd
// are library-anchored), so the earlier reloc-name artifact is resolved; only the
// register-coloring residual remains. Not source-steerable.
RVA(0x001853b0, 0xa6)
int WapUncompress(
    unsigned char* dest,
    unsigned long* pDestLen,
    unsigned char* src,
    unsigned long srcLen
) {
    z_stream s;
    s.next_in = src;
    s.avail_in = static_cast<unsigned int>(srcLen);
    s.next_out = dest;
    s.avail_out = static_cast<unsigned int>(*pDestLen);
    s.zalloc = 0;
    s.zfree = 0;
    s.opaque = 0;
    int err = deflateInit_(&s, -1, "1.0.4", sizeof(z_stream));
    if (err != 0) {
        return err;
    }
    err = deflate(&s, 4);
    if (err != 1) {
        deflateEnd(&s);
        if (err == 0) {
            return -5;
        }
        return err;
    }
    *pDestLen = s.total_out;
    return deflateEnd(&s);
}
