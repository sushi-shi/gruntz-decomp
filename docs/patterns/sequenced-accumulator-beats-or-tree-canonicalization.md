# A flat a|b|c (or shift-sum) tree evaluates the WRONG operand first: sequence it

tags: cpp:expr cpp:assign | asm:or asm:shl asm:movzx | topic:codegen-idiom
symptoms: first channel load swapped ([eax-0x1] vs [eax-0x2]), or-chain built
from the second operand, source operand swap changes nothing, per-arm epilogue
vs shared ret tail
confidence: 8/10
variants: pixel-pack-is-a-byvalue-u8-helper.md

cl5 canonicalizes commutative `|`/`+` trees, so `red | green | blue` and
`green | red | blue` compile identically — and often with the WRONG operand
first vs retail. Textual swaps, nesting alone, and by-value helper args are
frequently inert. What DOES pin the order is a sequenced accumulator: give the
first term its own statement, fold the rest through the named value. cl still
reassociates `v | (x | y)` into `((v|x)|y)` but preserves leftmost-first
evaluation:

```cpp
u16 v = static_cast<u16>((static_cast<u8>(p->peRed >> (u8)g_rDown)) << g_rUp);
v = static_cast<u16>(v | ((u8)(p->peGreen >> (u8)g_gDown) << g_gUp
                          | (u8)(p->peBlue >> (u8)g_bDown)));
*out++ = v;                       // AlphaTable: red chain first, 2 ORs
```

For a shift-sum pack the same trick works with `acc`:

```cpp
i32 acc = static_cast<u8>(v >> 0xb) << 4;      // GreyTable: high nibble first
acc = (acc + static_cast<u8>((v >> 6) & 0xf)) << 4;
*out++ = static_cast<u16>(acc + static_cast<u8>((v >> 1) & 0xf));
```

Two traps, both measured: (1) `v |= x; v |= y;` statements reassociate freely
again — use the ONE nested fold, not three flat ORs, and match retail's OR
count; (2) the lever sometimes flips the pointer-bias or loop rotation
(SubTable, ConvertRowFlip regressed) — it is a per-site coin, verify against
the target bytes. When a loop-INVARIANT operand keeps winning the first slot
regardless (SubTable), the coin is scheduler state, not spelling.

STEERABLE (per-site). AlphaTable 0x14f5b0 99.98 -> 100.00 EXACT; GreyTable
0x14eef0 82.23 -> 92.86 (also recovered the per-arm epilogues); Blit1624
0x13fce0 79.65 -> 95.15 (with the arms' TRUE per-arm term orders — retail's
forward arm reads RGB where its bottom-up arm reads BGR); Blit168 0x13fbb0
92.04 -> 100.00 EXACT via the indexed-subscript variant
(`pal[i * 4 + k]`, no pointer walk) feeding the by-value helper.
