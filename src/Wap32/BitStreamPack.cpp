// BitStreamPack.cpp - 0x188860: zlib 1.0.4 deflate's _tr_align (trees.c), the
// static-tree end-of-block aligner. Emits the 3-bit STATIC_TREES<<1 block header
// then the static-Huffman END_BLOCK code {Code,Len}@static_ltree[256]@0x6c0060,
// bumps compressed_len, bi_flush()es, and - when the remaining budget is short -
// emits the pair a second time, then sets last_eob_len = 7. __cdecl over the
// deflate_state. Modeled with the file-local BitState view of the fields _tr_align
// touches (real class = zlib deflate_state); the bit ops mirror the send_bits /
// put_short macros exactly (16-bit ush accumulator) so the codegen falls out.
#include <Ints.h>
#include <rva.h>

// static_ltree[END_BLOCK] (END_BLOCK == 256): the static-Huffman EOB code, a
// ct_data { ush Code@+0; ush Len@+2 } (one .data object). Reloc-masked DATA.
struct BitMarker {
    u16 code; // +0x00  ct_data.fc.code
    u16 len;  // +0x02  ct_data.dl.len
};
DATA(0x002c0060)
extern BitMarker g_bitMarker; // static_ltree[256] @ 0x6c0060

// zlib deflate_state (file-local view of the fields _tr_align + send_bits touch).
struct BitState {
    char _0[0x08];
    u8* m_pendingBuf; // +0x08  pending_buf (put_byte output)
    char _c[0x10 - 0x0c];
    u32 m_pending; // +0x10  pending (write index into pending_buf)
    char _14[0x16a4 - 0x14];
    u32 m_compressedLen; // +0x16a4  compressed_len (ulg)
    char _16a8[0x16ac - 0x16a8];
    i32 m_lastEobLen; // +0x16ac  last_eob_len
    u16 m_biBuf;      // +0x16b0  bi_buf (16-bit bit accumulator)
    char _16b2[0x16b4 - 0x16b2];
    i32 m_biValid; // +0x16b4  bi_valid (bit position in bi_buf)
};

// bi_flush (0x18a190, local void bi_flush(deflate_state*)): flush the accumulator
// out to whole bytes. External, reloc-masked (no body).
extern "C" void BitFlush(BitState* s); // 0x18a190

// zlib trees.c send_bits macro (Buf_size = 16), verbatim over the BitState view.
// It MUST be a macro, not an inline fn: `value` is textually re-substituted per
// branch (raw ush member load + per-use extension) - collapsing it to one `int`
// param pre-masks the load and hoists the extend out of the taken branch. put_short
// is inlined as its two put_byte low/high reads of the ush accumulator.
#define send_bits(s, value, length)                                                                \
    {                                                                                              \
        int len = length;                                                                          \
        if ((s)->m_biValid > 16 - len) {                                                           \
            int val = value;                                                                       \
            (s)->m_biBuf |= (u16)(val << (s)->m_biValid);                                          \
            (s)->m_pendingBuf[(s)->m_pending++] = (u8)((s)->m_biBuf & 0xff);                       \
            (s)->m_pendingBuf[(s)->m_pending++] = (u8)((u16)(s)->m_biBuf >> 8);                    \
            (s)->m_biBuf = (u16)val >> (16 - (s)->m_biValid);                                      \
            (s)->m_biValid += len - 16;                                                            \
        } else {                                                                                   \
            (s)->m_biBuf |= (u16)(value << (s)->m_biValid);                                        \
            (s)->m_biValid += len;                                                                 \
        }                                                                                          \
    }

// send_code(s, END_BLOCK, static_ltree) == send_bits(s, static_ltree[256].Code,
// static_ltree[256].Len).
#define send_code(s, marker) send_bits(s, (marker).code, (marker).len)

// @early-stop
// 96% - the two send_bits(STATIC_TREES<<1, 3) constant-length blocks are byte-exact;
// the residual is confined to the two variable-length send_code (g_bitMarker.len)
// blocks. Retail keeps `len` in edx and accumulates the new bi_valid INTO edx in BOTH
// branches (`lea edx,[edx+edi-0x10]` if, `add edx,ecx` else), so the `mov [bi_valid],edx`
// store TAIL-MERGES to one shared site; our cl accumulates into bi_valid's own register
// (`lea edx,[edi+edx-0x10]` SIB-swapped + `add ecx,edx`) and duplicates the store per
// branch. Pure commutative-operand destination selection (a += b -> dest a vs dest b),
// non-steerable from canonical zlib source. See docs/patterns/zlib-send-bits-macro.md.
RVA(0x00188860, 0x267)
void BitEmitMarker(BitState* s) {
    send_bits(s, 1 << 1, 3); // STATIC_TREES<<1, block type
    send_code(s, g_bitMarker);
    s->m_compressedLen += 10L;
    BitFlush(s);
    if (1 + s->m_lastEobLen + 10 - s->m_biValid < 9) {
        send_bits(s, 1 << 1, 3);
        send_code(s, g_bitMarker);
        s->m_compressedLen += 10L;
        BitFlush(s);
    }
    s->m_lastEobLen = 7;
}

SIZE_UNKNOWN(BitMarker);
SIZE_UNKNOWN(BitState);
