# A square macro preserves the integer expression origin that expanded locals erase

tags: cpp:macro cpp:expr cpp:local | asm:imul asm:mov | topic:codegen-idiom topic:regalloc topic:scheduling
symptoms: a pure integer function has exact extent, calls, CFG, constants, and mnemonic multiset, but an expanded set of delta locals rotates every long-lived register and stack home; mixed TU-state forests are flat
confidence: 10/10

MSVC 5.0 does not canonicalize a repeated integer square macro, an inline square
function, and hand-expanded delta locals to the same optimizer graph. A macro
can therefore be the missing source abstraction even when all three spellings
express the same arithmetic and ultimately emit the same instruction multiset.

## Exact witness

`CShadeTableCache::FindNearestColor` 0x14fbf0 computes squared RGB distance for
palette entry zero and again inside the remaining 255-entry search. The expanded
reconstruction named widened query channels and six delta locals. Base and retail
were both exactly 0xcb bytes with no calls or relocations, three branches, one
return, and the same mnemonic, immediate, displacement, and store multisets, but
the register and three reusable stack homes were rotated throughout. It scored
77.5600%.

Two independent 64-island mixed-kind declaration forests, inserted beside the
target and at the top of the TU, emitted one identical 77.5600% object. That
rules out the known C1-handle and `/Og` phase for this source graph and routes the
wall to structure.

The structural A/B reduced the repeated operation to its natural source unit:

```cpp
#define SQUARE(value) ((value) * (value))

i32 bestDist =
    SQUARE(r - pal->peRed) + SQUARE(g - pal->peGreen) + SQUARE(b - pal->peBlue);

for (i32 i = 1; i < PALETTE_ENTRY_COUNT; i++) {
    i32 distance = SQUARE(r - pal[i].peRed) + SQUARE(g - pal[i].peGreen)
                   + SQUARE(b - pal[i].peBlue);
    // ... retain the strictly-lower winner
}
```

The macro form emits retail byte-for-byte: 100.0000%, exact 0xcb extent, complete
decode, and the zero-length relocation streams agree. It also restores the
authored RGB expression instead of transcribing retail's scheduled green/blue/red
loads into source locals.

The nearby controls separate macro origin from spelling and arithmetic:

| source form | score | extent | result |
|---|---:|---:|---|
| expanded widened channels and named deltas | 77.5600% | 203 | exact topology, rotated allocation |
| `inline i32 Square(i32)` over direct RGB expressions | 77.3600% | 203 | a different exact-topology island |
| the inline helper plus widened channel carriers | 73.0933% | 200 | one instruction short |
| square macro over direct RGB expressions | **100.0000%** | 203 | byte-exact |

Five macro controls—local `SQR`, local semantic name, top-level `SQR`, top-level
`SQUARE`, and top-level semantic name—were one byte-exact compiler island. The
identifier and definition position are therefore irrelevant here; the
preprocessor expansion boundary is the measured cause.

## Cross-function validation

The same transcription defect recurred in both palette-conversion loops in
`CDDSurface::Blit824` 0x140110 and `CDDSurface::Blit816` 0x140420. Replacing
their named per-channel delta carriers with the already-proven `SQUARE` macro
gave two independent positive controls:

| function | expanded carriers | direct `SQUARE` expressions | structural result |
|---|---:|---:|---|
| `Blit824` | 69.8301% | 70.7683% | 271 -> retail 265 instructions became exact 265/265; mnemonic multisets became exact |
| `Blit816` | 72.91% | 91.80% | the residual collapsed to two excess `mov`s, 278 vs 276 instructions |

Both rows retain exact calls, branch/return skeletons, immediates,
displacements, stores, and ordered referents. Reordering the equal-semantic
winner assignments to match the exact `FindNearestColor` source family
(`bestDist = d; best = i`) then moved them to 71.06% and 92.36% without changing
those structural counts.

A 64-island mixed declaration forest on the structurally corrected `Blit824`
emitted one identical 70.7683% object. A full nearest-color inline helper and a
source-consuming `u8*&` inline helper were also byte-flat for that function.
Those controls leave the larger search boundary undecided, but they strengthen
the narrow conclusion: the squared operation itself came from macro-expanded
expressions, and the remaining residue is a separate allocation/lifetime wall.

## Reverse-use rule

Use this only after `walls diagnose` proves an exact call set and CFG and a mixed
state campaign is one island. When the source has named arithmetic temporaries
that merely spell out a small repeated operation, test that operation as a
general macro and as an inline function in the same matrix. A macro exact result
with the inline control on another island is evidence of authentic macro origin,
not permission to wrap arbitrary expressions for score. Retain it only when the
macro names a reusable operation and simplifies the reconstructed source.

## Related

- [`macro-origin-changes-vc5-x87-cse.md`](macro-origin-changes-vc5-x87-cse.md)
- [`scalar-byte-copy-is-an-inline-helper.md`](scalar-byte-copy-is-an-inline-helper.md)
- [`tu-state-probe-family-decides-reachability.md`](tu-state-probe-family-decides-reachability.md)
