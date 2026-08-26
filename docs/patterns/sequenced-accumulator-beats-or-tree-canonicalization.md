# Flat and sequenced channel packs select different cl5 evaluation islands

tags: cpp:expr cpp:assign | asm:or asm:shl asm:movzx | topic:codegen-idiom
symptoms: first channel load swapped ([eax-0x1] vs [eax-0x2]), or-chain built
from the wrong operand, source operand swap changes nothing, per-arm epilogue
vs shared ret tail
confidence: 9/10
variants: pixel-pack-is-a-byvalue-u8-helper.md

cl5 canonicalizes commutative `|`/`+` trees, so textual operand swaps are often
inert. A flat expression and a sequenced accumulator are not equivalent inputs,
however: they can select different evaluation islands even when they emit the
same operation multiset. Neither form is universally right. Choose between them
from the retail load order and verify the whole loop.

A sequenced accumulator can pin one term ahead of the remaining fold:

```cpp
u16 v = static_cast<u16>((static_cast<u8>(p->peRed >> (u8)g_rDown)) << g_rUp);
v = static_cast<u16>(v | ((u8)(p->peGreen >> (u8)g_gDown) << g_gUp
                          | (u8)(p->peBlue >> (u8)g_bDown)));
*out++ = v;
```

For a shift-sum pack the same trick works with `acc`:

```cpp
i32 acc = static_cast<u8>(v >> 0xb) << 4;      // GreyTable: high nibble first
acc = (acc + static_cast<u8>((v >> 6) & 0xf)) << 4;
*out++ = static_cast<u16>(acc + static_cast<u8>((v >> 1) & 0xf));
```

Two traps are measured. First, `v |= x; v |= y;` statements can select a third
island rather than behaving like the one nested fold. Second, changing the tree
can also flip pointer bias or loop rotation (SubTable and ConvertRowFlip), so
the lever is per-site. When a loop-invariant operand wins the first slot in
every tree form, the residue is scheduler state rather than spelling.

`CShadeTableCache::AlphaTable` 0x14f5b0 is the negative control that corrected
the former version of this pattern. In a clean build, the sequenced nested form
scored raw 99.655174 and loaded green before red. A flat `red | green | blue`
expression scored raw 99.908040 in the disposable harness and is 100.00 EXACT
in the normal build, loading red first as retail does. Preloading all three
bytes produced the same 99.908040 island; separate `|=` statements, sequential
assignments, and channel locals all fell to 99.183910 and one byte shorter.
A 32-state compiler-state campaign and 223 generic structural variants each
found only the baseline island; the hand expression-tree A/B exposed the
missing axis. Thus the old claim that AlphaTable proved sequencing was false.

The positive sequenced case that remains is GreyTable 0x14eef0, which moved
82.23 -> 92.86 and also recovered its per-arm epilogues. Blit1624 0x13fce0 was
formerly cited here after sequencing moved it 79.65 -> 95.15, but that was an
intermediate local maximum: the general `PACK_PIXEL16(r,g,b)` macro is now
100.00 EXACT. Blit168 0x13fbb0 reached 100.00 EXACT through the related
indexed-subscript/by-value-helper form (`pal[i * 4 + k]`), not by a universal
sequencing rule.
