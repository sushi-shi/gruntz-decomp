#include <rva.h>
// WapUncompress.cpp - the game's raw-deflate `uncompress` helper (0x1853b0). A
// classic zlib uncompress shape over the game's statically-linked second zlib copy
// (inflateInit2_ 0x186180 / inflate 0x186620 / inflateEnd 0x186990), but with
// windowBits = -1 (raw deflate). The "1.0.4" version string is the zlib $SG
// constant; the three zlib entry points are reloc-masked rel32 externs.

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
    int
    inflateInit2_(GzStream* strm, int windowBits, const char* version, int stream_size); // 0x186180
    int inflate(GzStream* strm, int flush);                                              // 0x186620
    int inflateEnd(GzStream* strm);                                                      // 0x186990
}

// ===========================================================================
// 0x1853b0 - raw-inflate the source block into dest; on success store the produced
// length back through pDestLen and return inflateEnd's result. Returns the init
// error if inflateInit2_ fails, Z_BUF_ERROR (-5) if inflate stops at Z_OK without
// reaching Z_STREAM_END, else the inflate error.
// ===========================================================================
// @early-stop
// ~86% (regalloc register-choice wall + reloc-name artifact): the control flow and
// every instruction's shape are identical to retail. Two residuals, neither
// source-steerable: (1) MSVC pins the long-lived pDestLen in ebx where retail uses
// edi (the allocator's 2nd-callee-saved pick {esi,ebx} vs {esi,edi}), which also
// flips the coupled avail_in/next_in store order at the top; (2) the three zlib
// entry-point rel32 calls resolve to the game's still-unreconstructed second zlib
// copy (?Unmatched_186180/186620/186990@@YAXXZ) so their masked relocs name
// differently than our inflateInit2_/inflate/inflateEnd - they go exact for free
// once that copy is named. Logic complete; deferred to the final sweep.
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
    int err = inflateInit2_(&s, -1, "1.0.4", sizeof(GzStream));
    if (err != 0) {
        return err;
    }
    err = inflate(&s, 4);
    if (err != 1) {
        inflateEnd(&s);
        if (err == 0) {
            return -5;
        }
        return err;
    }
    *pDestLen = s.total_out;
    return inflateEnd(&s);
}
