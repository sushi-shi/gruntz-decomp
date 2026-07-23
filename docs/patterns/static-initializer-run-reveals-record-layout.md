# Repeated static initializers reveal records, not independent scalar globals
tags: cpp:static cpp:struct | data:bss data:layout | asm:mov asm:ret | topic:identity topic:tooling
symptoms: several nearby BSS addresses were declared as short scalar arrays; one compiler-generated helper per address writes the same offsets from that address before returning
confidence: 10/10

Global data does not follow the function convention of placing every item on a
16-byte boundary. Its placement follows the object's required alignment,
section allocation, and linker layout. RVA spacing alone therefore cannot tell
whether nearby words are independent constants, fields, array elements, or
separate objects of one record type.

A run of compiler-generated static initializer functions can supply the missing
boundary evidence. When each helper:

1. relocates stores against one base address;
2. writes `base+0`, `base+4`, and `base+8`; and
3. returns before touching the next base,

the source model is a 12-byte constructed record at each base, not an `int[2]`
plus an unexplained neighbouring word. Repeating the same store shape for
several bases proves several objects of the same record type. It does not by
itself prove that those objects are members of one larger structure or one
array.

For the Grunt direction cells, nine helpers at `0x0005b840..0x0005bac0`
initialize these triples:

```text
(row, column, direction)
(0,1,1) (0,2,2) (1,2,3) (2,2,4) (2,1,5)
(2,0,6) (1,0,7) (0,0,8) (1,1,0)
```

The consumers index a 3-by-3 cell table with `3 * row + column`, so the values
support the semantic compass names and the third field's direction identity.
The helpers occur in the GruntCombat contribution and relocate to the same
objects used by `CGrunt::Activate`, which also proves the owning TU.

Model the shared type and define the real objects in that owner:

```cpp
struct GruntDirectionCell {
    GruntDirectionCell(i32 row_, i32 column_, i32 direction_)
        : row(row_), column(column_), direction(direction_) {}
    i32 row;
    i32 column;
    i32 direction;
};
SIZE(0xc);

DATA(0x00......)
GruntDirectionCell g_direction(0, 1, 1);
```

This pattern is useful in reverse. Search unknown data clusters for repeated
compiler-generated helpers with the same relative store offsets, group their
relocation targets by base, and use the consumers to name the fields. The
initializer establishes boundaries and construction shape; xrefs establish
meaning and ownership.
