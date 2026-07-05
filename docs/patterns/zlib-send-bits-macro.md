# zlib `send_bits` must be a MACRO (per-branch value re-substitution + 16-bit `ush` accumulator)
tags: cpp:macro cpp:inline cpp:int | asm:shr asm:shl asm:and | topic:codegen-idiom topic:wall topic:tail-merge
symptoms: deflate bit emitter, `shr dx,cl` 16-bit shift, `and eax,0xffff` deferred into a branch, extra `push ebp`, `mov edx,ecx` copies
confidence: 8/10

zlib 1.0.4 `trees.c` emits bits through the `send_bits(s, value, length)` **macro**
(and `put_short`/`put_byte`), over a **16-bit `ush bi_buf`** accumulator. Reconstructing
those functions (`_tr_align` = 0x188860 `BitEmitMarker`, `compress_block`, `_tr_flush_block`,
`send_tree`, …) as a hand-rolled `__inline void PutBits(BitState*, u32 code, i32 nbits)` does
NOT match — two independent reasons:

1. **16-bit accumulator.** `bi_buf` is `ush` (unsigned short). `s->bi_buf = (ush)val >> (16 - s->bi_valid)`
   assigns a ush, so MSVC5 does a **16-bit** `mov dx,imm; shr dx,cl` (and computes the
   count 16-bit: `mov cx,0x10; sub cx,ax`). A `u32` accumulator gives 32-bit `shr edx,cl`.
   put_short reads the two accumulator bytes directly: `mov al,[bi_buf]` / `mov cl,[bi_buf+1]`.
   Model `bi_buf` as `u16` at its offset and write the two `put_byte`s as
   `buf[pending++] = (u8)(bi_buf & 0xff); buf[pending++] = (u8)((u16)bi_buf >> 8);`.

2. **Macro, not inline fn.** The macro re-substitutes `value` textually in each branch
   (`int val = value;` in the taken branch, bare `value` in the else). With a `u16`/`int`
   parameter the value is collapsed to ONE pre-extended int: MSVC loads+masks `g_code`
   (`mov eax,[code]; and eax,0xffff`) once at the TOP and hoists it above the branch. The
   macro form lets MSVC load `g_code` **once but after the `cmp`**, mask it (`and eax,0xffff`)
   **inside** the taken branch, and use the raw dword in the else (only `ax` matters for the
   `or [bi_buf],ax`). Writing it as `#define send_bits(s,value,length) { int len=length; if(...){int val=value; ...} else {... value ...} }` (verbatim zlib) reproduces this.

Progress on `BitEmitMarker`: hand `PutBits(u32,i32)` inline 59% → `u16` acc + exact
byte-reads 87% → **macro** form 96%.

## Residual (WALL, ~4% here): variable-length `send_code` tail-merge
The constant-length `send_bits(s, STATIC_TREES<<1, 3)` blocks go byte-exact. The
variable-length `send_code(s, END_BLOCK, static_ltree)` (length = `static_ltree[256].Len`,
a loaded value in `edx`) does not: retail keeps `len` in `edx` and accumulates the new
`bi_valid` INTO edx in BOTH the `if` (`lea edx,[edx+edi-0x10]`) and `else`
(`add edx,ecx`) arms, so the `mov [bi_valid],edx` store **tail-merges** to one shared
site. Our cl accumulates into `bi_valid`'s own register instead (`lea edx,[edi+edx-0x10]`
with the SIB base/index swapped + `else: add ecx,edx`), leaving the result in a different
register per arm and DUPLICATING the store. Pure commutative-operand destination
selection (`a += b` → dest `a` vs dest `b`) — non-steerable from canonical zlib source.
WALL. Evidence: BitEmitMarker 0x188860 96% (both send_code blocks affected).
