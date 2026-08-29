# A sized one-byte `memset` retains VC5's fill preparation

tags: cpp:inline cpp:int cpp:loop | asm:and asm:mov | topic:codegen-idiom topic:regalloc
symptoms: a byte-padding loop has the right calls and CFG, but retail masks the signed
remainder with `0xff`, stores `dl`, and then copies `dl` into `dh`; a direct byte
assignment lacks the mask and high-byte copy
confidence: 9/10

In `CFecFile::AddFile` at `0x17b950`, the archive-name tail is filled with one random
byte per iteration. The direct transcription

```cpp
*p++ = static_cast<char>(Random() % FEC_RANDOM_BYTE_MODULUS);
```

emitted the right loop and call, but VC5 stored the remainder directly. Retail instead
prepared the value like an inlined fill: it masked the remainder through the existing
`0xff` divisor, stored its low byte, and copied that byte into the high half of the
word register.

The source-level boundary that reproduces the complete instruction sequence is:

```cpp
memset(p, static_cast<u8>(Random() % FEC_RANDOM_BYTE_MODULUS), sizeof(*p));
p++;
```

The parts were independently controlled. A one-byte `memset` without the sized cast
introduced the high-byte copy but not the mask. The `u8` cast on a direct assignment
was byte-neutral. Composed at the `memset` boundary, they emitted both instructions and
made the loop body byte-exact, moving AddFile from 98.913666% to 99.84892%.

Pointer advancement is a separate part of the source shape. Writing `p++` in the
`memset` argument kept the mask and high-byte copy but introduced another live pointer
register and fell to 97.960434%. Advancing `p` in the following statement preserves
retail's two-register pointer/count loop and store-before-increment order.

## Reverse use

When retail contains apparently dead byte-replication work after a one-byte store, do
not immediately transcribe the register operations as arithmetic. Test whether the
source used a standard byte-fill boundary. Preserve the fill value's sized alias and
the pointer advancement as separate statements, then verify the entire loop rather
than ranking either instruction in isolation.

This is not a general reason to replace byte assignments with `memset`; the signature
requires the mask, low-byte store, and high-byte replication together.
