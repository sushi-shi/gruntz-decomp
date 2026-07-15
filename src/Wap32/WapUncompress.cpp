#include <rva.h>
// WapUncompress.cpp - the game's zlib `compress` helper (0x1853b0). Despite the
// leaked "WapUncompress" name this is the COMPRESS side: its body is verbatim
// zlib-1.0.4 compress.c `compress` shape (deflateInit at Z_DEFAULT_COMPRESSION,
// deflate(Z_FINISH), deflateEnd) and it calls the DEFLATE entry points of the
// single statically-linked zlib - deflateInit_ 0x186180, deflate 0x186620,
// deflateEnd 0x186990 (all three now library-anchored; the earlier "second zlib
// copy / inflate" note was a misread - there is one zlib and these are its
// compressors). The "1.0.4" version string is the zlib $SG constant; the three
// zlib entry points are reloc-masked rel32 externs.

// The zlib z_stream (1.0.4 layout, sizeof 0x38) + the deflate entry points come from
// the REAL vendored <zlib.h> (extern "C", EXPORT==empty==__cdecl since ZLIB_DLL is
// unset) - the struct is byte-identical to the retail layout, so the field stores land
// at the retail offsets. (Was a hand-rolled GzStream view + local extern "C" decls;
// deflateInit_ 0x186180, deflate 0x186620, deflateEnd 0x186990 are the reloc-masked
// rel32 externs the vendored zlib TUs anchor.)
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
    s.avail_in = (unsigned int)srcLen;
    s.next_out = dest;
    s.avail_out = (unsigned int)*pDestLen;
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
