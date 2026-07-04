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

// zlib deflate_state (internal_state) - the opaque per-stream deflate state,
// allocated by deflateInit2_ (external). Only the flush_pending fields are pinned
// (this build's layout has pending_out@+0xc, not the standard zlib-1.1.x +0x10);
// its full size is not recoverable from the matched code, so it stays opaque.
struct internal_state {
    char pad0[8];
    char* pending_buf; // +0x08 output buffer base
    char* pending_out; // +0x0c next pending byte to output
    unsigned pending;  // +0x10 bytes pending in pending_buf
};
// The zlib public z_stream (the standard 0x38-byte API struct).
struct z_stream {
    unsigned char* next_in;  // +0x00 next input byte
    unsigned avail_in;       // +0x04 number of bytes available at next_in
    unsigned long total_in;  // +0x08 total number of input bytes read so far
    unsigned char* next_out; // +0x0c next output byte
    unsigned avail_out;      // +0x10 remaining free space at next_out
    unsigned long total_out; // +0x14 total bytes output so far
    char* msg;               // +0x18 last error message, or 0
    internal_state* state;   // +0x1c the deflate/inflate state (opaque)
    void* zalloc;            // +0x20 alloc callback
    void* zfree;             // +0x24 free callback
    void* opaque;            // +0x28 private data for alloc/free
    int data_type;           // +0x2c ascii/binary hint
    unsigned long adler;     // +0x30 adler32 of uncompressed data
    unsigned long reserved;  // +0x34 reserved for future use
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
SIZE(z_stream, 0x38);         // the standard zlib public z_stream
SIZE_UNKNOWN(internal_state); // zlib deflate_state: opaque, allocated by deflateInit2_
