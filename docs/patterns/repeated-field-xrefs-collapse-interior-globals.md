# Repeated field xrefs collapse interior globals into one object
tags: cpp:class cpp:inline | data:bss data:layout | asm:mov asm:call | topic:identity topic:mis-model
symptoms: several declared-only globals occupy consecutive word offsets; callers access them in the same order as the fields of a known class and reproduce one of that class's inline methods
confidence: 10/10

An address label at every referenced word can shred one retail object into fake
globals. Contiguous addresses alone do not prove the correction, but a complete
field-offset map plus a repeated method body does.

The former GruntVoice activation declarations covered:

```text
0x006514d8  collection/base object
0x006514dc  error sink
0x006514e0  low bound
0x006514e4  high bound
0x006514e8  allocation base
0x006514ec  spare/current entry
0x006514f0  element stride
0x006514f8  scratch/grown count
```

Those offsets are exactly the inherited `_zvec`/`_zdvec`/`zDArray` layout of
the real `CActReg g_actReg_6514d8`. Retail
`CGruntVoice::FireActivation` reads and writes the same fields, calls
`_zvec::GrowTo` with `ECX = 0x006514d8`, and invokes
`CVariantSlot::Set` through the field at `+4`. Replacing the eight field names
with `g_actReg_6514d8.ResolveEntry(coord)` makes the function instruction
identical.

The safe recovery procedure is:

1. Map every apparent global address to a candidate object's field offset.
2. Check the class hierarchy and full inherited layout, not only the leaf
   declaration.
3. Verify receiver setup at calls and field access widths in retail
   disassembly.
4. Compare the whole access sequence with a known or reconstructed method.
5. Replace consumers with the real object/member operation and remove the
   interior declarations.

Do not satisfy the declared-only ratchet by defining storage at each interior
address. That would create overlapping objects and conceal the actual class
model. If the outer object's construction or storage class is still unclear,
keep that one boundary symbol unresolved and record the remaining evidence;
the proven interior aliases can still be removed.

Use this pattern in reverse when auditing other dense data bands. A cluster of
word-spaced declared-only names is a strong candidate when multiple consumers
repeat the same bounds check, stride calculation, grow call, error path, and
spare-entry return. `g_typeColl` is another registry-shaped example: its
members should be modeled through the shared object rather than reintroduced
as globals at their individual addresses.
