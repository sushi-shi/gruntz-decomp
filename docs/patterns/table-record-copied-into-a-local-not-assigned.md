# A load-all-then-store-all block is a LOCAL struct copy, not `*this = tbl[i]`

- **confidence** c9
- **tags** `cpp:struct` `cpp:local` `cpp:array` `cpp:const` | `asm:mov` `asm:lea` | `topic:codegen-idiom`
- **first seen** `GruntDirectionCell::RotateClockwise` / `RotateCounterclockwise`
  @0x0003c850 / 0x0003c8a0, 67.74 -> **100.00 EXACT** each

## Symptom

Retail reads every field of a record out of a static table and only then writes
them, through a register holding the element's ADDRESS, with one apparently
redundant register-to-register move in front:

```
lea    eax,[eax+eax*2]          ; idx * 3
lea    edx,[eax*4+0x60d008]     ; &tbl[idx]        <- table base folded into the lea
mov    esi,edx                  ; the "redundant" copy
mov    eax,DWORD PTR [esi]
mov    edx,DWORD PTR [esi+0x4]
mov    esi,DWORD PTR [esi+0x8]  ; ALL loads first
mov    DWORD PTR [ecx],eax
mov    DWORD PTR [ecx+0x4],edx
mov    DWORD PTR [ecx+0x8],esi  ; ...then all stores
```

Three spellings that do NOT produce it:

| source | what cl5 emits |
|---|---|
| `row = e[0]; column = e[1]; direction = e[2];` (over a flat `const i32[]`) | interleaved load/store pairs, table base folded into each load's displacement |
| `*this = tbl[i];` (the implicit memberwise `operator=`) | ONE scratch register, interleaved: `mov edi,[eax]; mov [esi],edi; mov edi,[eax+4]; ...`, plus a `mov esi,ecx` copy of `this` |
| `CTriRecord& cell = *this; cell = tbl[i];` | identical to the above |

## The source

Copy the element into a LOCAL and assign the members from it:

```cpp
CTriRecord next = g_directionClockwiseTable[row * 3 + column];
row = next.row;
column = next.column;
direction = next.direction;
```

cl5 copy-propagates the dead local away, but the *initialisation* of the local is
a single point that must read the whole record before any member store can run —
so the three loads bunch at the top and the three stores at the bottom. It also
gives the element address its own register (`lea edx,...; mov esi,edx`), where the
`operator=` expansion addresses the source off the table base.

The extra register pressure is visible in the prologue: retail loads the loop
counter into a callee-saved `edi` **before** the zero test (`push edi; mov
edi,[esp+8]; test edi,edi`), because the copy claims eax/edx/esi. A recompile
that keeps the counter in edx is a reliable tell that the copy has not been
modelled.

## The table has to be typed — which forces the class to be an aggregate

The whole thing only works if the table is `const Record[N]` rather than a flat
`const i32[3*N]`. MSVC 5.0 has no `constexpr`, so an array of a class with a
user-declared constructor gets a **dynamic** `$E` initialiser and lands zero-filled
in the object — which breaks the `DATA()` pin. Make the record type an aggregate
(no user-declared ctors, no base) and brace-initialise the table:

```cpp
struct CTriRecord {
    i32 row;
    i32 column;
    GruntDirection direction;
};
DATA(0x0020d008)
const CTriRecord g_directionClockwiseTable[9] = {
    {0, 1, DIR_NORTH}, {0, 2, DIR_NORTHEAST}, ...
};
```

A derived class may keep its constructors (here `GruntDirectionCell` has to: retail
emits `$E` initialisers for the nine per-TU direction-cell statics); move the base's
member initialisation into the derived ctor's body.

## Caveats

- Namespace-scope `const` has internal linkage in C++. Do NOT paper over that with
  `extern const ... = {...}` in the `.cpp` — the `cpp extern decls` ratchet counts
  it. Other DATA-pinned `const` definitions in the tree bind fine as-is.
- If the derived class is the one being assigned, the assignment must name the
  base sub-object; a reference binding (`CTriRecord& cell = *this;`) is cast-free
  but selects the wrong expansion. Use the local-copy form instead.
