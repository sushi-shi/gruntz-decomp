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

A second, parallel nine-object band at full-image addresses
`0x006448c8..0x00644948` exposed a subtler false model. The bases are 0x10
bytes apart, so a placeholder `DirDesc[9]` with a fabricated 0x10-byte element
appeared to index them correctly. Each retail initializer instead writes only
three dwords at `base+0`, `base+4`, and `base+8`, and the compiled definitions
are nine 0xc-byte `GruntDirectionCell` objects with per-object alignment gaps.
Thus a regular address stride does not prove an array when separate objects
receive the same alignment.

Initializer-table attribution also splits this apparently contiguous data
band across two translation units. Eight helpers at
`0x00047760..0x00047990` form the GruntSteps init run; the center helper at
`0x000479e0` is the one-entry DirectionClassify run. The latter object's base,
`0x00644938`, lies inside GruntSteps' address band, but its distinct CRT slot
and helper prove it is an evidence-backed scattered singleton rather than a
reason to move the definition. This is a valid `data-tu-order` baseline case.

The parallel GruntCombat set supplies a strong type-and-value check: both bands
use the same nine triples and the same three-store helper shapes. Recovering
the second set as the shared `GruntDirectionCell` type removed both the fake
`DirDesc[9]` overlay and the provisional `CGruntVoiceRec` names. In reverse,
audit any stride-shaped "array" by grouping initializer helpers by relocation
base and CRT/TU run before trusting the apparent element size.

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

A second form is a helper that passes one constructor argument and one object
address:

```asm
push  <label>
mov   ecx,<object>
call  CVariantSlot::CVariantSlot
ret
```

Four 16-byte helpers at `0x0016d700`, `0x0016d9b0`, `0x0016de20`, and
`0x0016dfe0` pass the labels `"zBitSet: "`, `"Global Error: "`,
`"Dynamic Array: "`, and `"zSymTab: "` to the same constructor, with object
bases `0x006bf408`, `0x006bf430`, `0x006bf468`, and `0x006bf480`. The
constructor writes `+0x08`, `+0x0c`, `+0x10`, and `+0x14`, so each base owns a
complete 0x18-byte `CVariantSlot`. Consumers pass those same bases through
`zErrHandling` and invoke `CVariantSlot::Set`/`Add`. This disproves the former
`char[]`, `u8`, and `void*` declarations: the byte at `0x006bf468` is the first
byte of a constructed object, not an independent tag.

Code proximity did not prove storage ownership in this case. The
`0x0016d9b0` initializer sits immediately before a constructor emitted from
GameText.cpp, but the ordinary-data interleave gate places all four objects in
TypeKeyColl.cpp's contiguous `0x006bf400` band. Keep the typed extern in the
consumer and the definition in the data owner's TU.

This pattern is useful in reverse. Search unknown data clusters for repeated
compiler-generated helpers with the same relative store offsets, group their
relocation targets by base, and use the consumers to name the fields. The
initializer establishes boundaries and construction shape; xrefs establish
meaning and ownership.
