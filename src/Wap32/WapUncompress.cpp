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

// The zlib z_stream (1.0.4 layout, sizeof 0x38) - modeled locally so the field
// stores land at the retail offsets without pulling all of zlib.h.
struct GzStream {
    unsigned char* next_in;  // +0x00
    unsigned int avail_in;   // +0x04
    unsigned long total_in;  // +0x08
    unsigned char* next_out; // +0x0c
    unsigned int avail_out;  // +0x10
    unsigned long total_out; // +0x14
    char* msg;               // +0x18
    void* state;             // +0x1c
    void* zalloc;            // +0x20
    void* zfree;             // +0x24
    void* opaque;            // +0x28
    int data_type;           // +0x2c
    unsigned long adler;     // +0x30
    unsigned long reserved;  // +0x34
};
SIZE(GzStream, 0x38);

extern "C" {
    int deflateInit_(GzStream* strm, int level, const char* version, int stream_size); // 0x186180
    int deflate(GzStream* strm, int flush);                                            // 0x186620
    int deflateEnd(GzStream* strm);                                                    // 0x186990
}

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
    GzStream s;
    s.next_in = src;
    s.avail_in = (unsigned int)srcLen;
    s.next_out = dest;
    s.avail_out = (unsigned int)*pDestLen;
    s.zalloc = 0;
    s.zfree = 0;
    s.opaque = 0;
    int err = deflateInit_(&s, -1, "1.0.4", sizeof(GzStream));
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
