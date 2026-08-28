# A typed element index can restore a strength-reduced scale-1 SIB

tags: cpp:pointer cpp:loop cpp:index cpp:type | asm:mov asm:sib | topic:source-oracle topic:codegen-idiom topic:instruction-selection
symptoms: the loop's CFG, addresses, induction step, mnemonic sequence, and relocations
agree, but one scale-1 memory operand has its base and index registers commuted
confidence: 10/10 (two exact closures from one surviving source family plus a negative
control in the same container library)
variants: surviving-source-lineage-restores-typed-layers-and-order.md

## Evidence

The reconstructed mono text buffer was a `char*`. Its scroll and clear loops manually
advanced byte offsets by two and recovered 16-bit cells through a union view. That source
was semantically correct and compiled to the right byte induction, branches, stores, and
addresses, but each function retained a commuted scale-1 SIB:

| function | byte-view source | typed-index source |
|---|---:|---:|
| `MonoNewline` 0x184d50 | 98.5714 | **100.000** |
| `MonoClear` 0x184db0 | 99.0000 | **100.000** |

The surviving 1995-1997 LithTech `libs/lith/dprintf.cpp` supplies the missing source
fact. Its mono buffer is an `unsigned short*`, and both operations are ordinary indexed
`for` loops. Restoring `u16*` and element indices lets VC5 perform its own strength
reduction. The final machine loop still advances by bytes and still uses a scale-1 SIB,
but C2 now assigns retail's register as the base and the other as the index.

This is a source-type effect, not an argument for spelling the retail assembly in C++.
The element scale can disappear during optimization while the front-end expression that
carried it continues to affect instruction selection.

## Detection signature

Use this hypothesis only when all of the following hold:

1. calls, branches, returns, relocations, induction step, and effective addresses agree;
2. the first substantive residue is a commuted base/index pair in a scale-1 SIB;
3. the current source views wider elements as bytes and manually multiplies offsets;
4. consumers, storage extent, or a surviving source family independently proves the
   wider element type and its natural index domain.

Restore `T*` plus element indexing as one A/B. Compare the complete normalized function,
because the compiler may strength-reduce the loop into the same byte induction while
changing only the operand encoding.

## Boundary and negative control

A commuted scale-1 SIB is often not source-controllable. `CHashElement::Prev` remains at
99.6552: importing its surviving loop spelling lowers it to 99.1379 and does not reveal a
missing element type. Do not manufacture a typed view solely to recolor a SIB. The lever
is licensed by independent element-type evidence, not by the encoding residue alone.
