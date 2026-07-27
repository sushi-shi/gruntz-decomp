#include <rva.h>

#include <zlib.h>

// ===========================================================================
// 0x1853b0 - deflate the source block into dest at Z_DEFAULT_COMPRESSION (-1); on
// success store the produced length back through pDestLen and return deflateEnd's
// result. Returns the init error if deflateInit_ fails, Z_BUF_ERROR (-5) if deflate
// stops at Z_OK without reaching Z_STREAM_END, else the deflate error.
// ===========================================================================
// @early-stop
// regalloc register-choice wall (measured 2026-07-27, 85.93 -> 87.59 after the exit
// fix): base had 3 rets against retail's 2 - the Z_BUF_ERROR return carried its own
// epilogue where retail preloads eax=-5 and `je`s into the shared exit that the
// init-error and the deflateEnd result also use. Nesting the body under
// `if (err == 0) { ... }` over one trailing `return err` reproduces that.
// Residual: MSVC pins the long-lived pDestLen in ebx where retail uses edi (the
// allocator's 2nd-callee-saved pick {esi,ebx} vs {esi,edi}), which also flips the
// coupled avail_in/next_in store order at the top. Not source-steerable.
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
    if (err == 0) {
        err = deflate(&s, 4);
        if (err != 1) {
            deflateEnd(&s);
            if (err != 0) {
                return err;
            }
            err = -5;
        } else {
            *pDestLen = s.total_out;
            err = deflateEnd(&s);
        }
    }
    return err;
}
