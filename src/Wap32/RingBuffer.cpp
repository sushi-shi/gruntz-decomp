// RingBuffer.cpp - two zlib deflate glue routines (part of the statically-linked
// zlib; adjacent to _deflate/_deflate_fast/_deflate_slow, which call flush_pending).
//
//  0x186910 - flush_pending(z_streamp): copy min(state->pending, avail_out) bytes
//             from state->pending_out to next_out, advance both pointers + total_out
//             / avail_out / pending, and rewind pending_out to pending_buf on drain.
//  0x186180 - deflateInit_(strm, level, ...): forward to deflateInit2_ with the zlib
//             defaults (method=Z_DEFLATED 8, windowBits=MAX_WBITS 15, memLevel 8,
//             strategy=Z_DEFAULT_STRATEGY 0).
#include <rva.h>
#include <string.h>

// zlib deflate_state (internal_state) - only the flush_pending fields are pinned.
struct internal_state {
    char pad0[8];
    char* pending_buf; // +0x08 output buffer base
    char* pending_out; // +0x0c next pending byte to output
    unsigned pending;  // +0x10 bytes pending in pending_buf
};
// zlib z_stream - only the flush_pending fields are pinned.
struct z_stream {
    char pad0[0xc];
    char* next_out;          // +0x0c next output byte
    unsigned avail_out;      // +0x10 remaining free space at next_out
    unsigned total_out;      // +0x14 total bytes output so far
    char pad18[0x1c - 0x18]; // +0x18 msg
    internal_state* state;   // +0x1c the deflate/inflate state
};

// 0x186910 - flush_pending
RVA(0x00186910, 0x72)
void flush_pending(z_stream* strm) {
    unsigned len = strm->state->pending;
    if (len > strm->avail_out) {
        len = strm->avail_out;
    }
    if (len) {
        memcpy(strm->next_out, strm->state->pending_out, len);
        strm->next_out += len;
        strm->state->pending_out += len;
        strm->total_out += len;
        strm->avail_out -= len;
        strm->state->pending -= len;
        if (strm->state->pending == 0) {
            strm->state->pending_out = strm->state->pending_buf;
        }
    }
}

// 0x186180 - deflateInit_(strm, level, version, stream_size) forwards to deflateInit2_.
extern "C" int deflateInit2_(int, int, int, int, int, int, int, int); // 0x1861b0
RVA(0x00186180, 0x25)
int deflateInit_(int strm, int level, int version, int stream_size) {
    return deflateInit2_(strm, level, 8, 0xf, 8, 0, version, stream_size);
}

// Class metadata (hosted at .cpp EOF).
SIZE_UNKNOWN(z_stream);       // zlib z_stream partial view
SIZE_UNKNOWN(internal_state); // zlib deflate_state partial view
