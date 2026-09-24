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

## Nested scalar-wrapper control

The shared `SquaredDistance(i32, i32)` in `Wap32/TileGeometry.h` provides a
different measured signature. Its four earlier TU-local implementations used
`SQR`; shared-owner consolidation in `e17fa3f05` flattened the inner products.
At `8a4002bca`, restoring the existing `BDefs.h` dependency and then restoring
`SQR(dx) + SQR(dy)` preserved the outer by-value inline boundary. The two
authored macro uses serve eleven calls in eight owners across four arithmetic
TUs. Original low-32-bit products, sums, signed guards and the one signed
integer-to-square-root conversion were inspected at all eleven sites.

The real production graph was compiled separately for the include-only and
complete nested-macro states. Macro-only versus include-only yields **290/290
identical normalized objects**, with all **7,476 scored records** unchanged.
All eight complete consumer bodies, extents and ordered fixups are identical
to the original baseline too. This restores the omitted inner abstraction; it
does not repair or certify their larger array/pool-call and control-flow gaps.

The required include alone changes ten scored bodies elsewhere; 7,466 remain
identical. It recovers historical exactness for `ActivateVisibleObjects` and
`CSBI_GruntMachine::Render`, and moves `CSpotLight::Update`'s historical best
from 80.2584% to 81.3820%. These are dependency-context effects, not effects of
the square expressions. Three current-score dips retain their old maxima;
all 4,429 historical and same-fingerprint maxima survive banking. The header
body is outside the current TU-only fingerprint census, so direct complete
object equality is recorded rather than inferred from unchanged fingerprints.
No unused include is retained as a compiler-state device.

Raw operand comparisons cover the eight references in the two recovered exact
owners, Spotlight Update's four named references plus the original pooled
string payload, and the single RouteUnitTo/PtInRect references in Repath and
FindNearestEnemy. Pooled-string content agreement does not prove final linked
placement. Tests and the default gated build remain deferred to squash merge;
these results come from production compilation and direct object/image reads.

Reverse-use rule: distinguish the inner macro, the enclosing value wrapper,
and required header visibility. Macro-versus-inline differences in the exact
palette witness above are not a universal law. Compile each source layer and
record a flat result as scoped evidence; do not discard authentic helpers or
flatten their caller argument/lifetime boundary merely because a score stays
unchanged. Canonical options and reopening conditions live under
`reassess-sqr-tile-geometry` and `tile-sqr-*` in the lineage ledger.

## A nested palette boundary can change a different table builder

A fresh seven-owner reassessment at `5fe33a945` compiled the actual DDSurface
and ShadeTableCache TUs, separating declaration visibility, square form and
an enclosing RGB-distance boundary. The latter takes the existing
`const PALETTEENTRY&` and three `u8` values; no new color layout is introduced.

| Compiler control | Blit824 | Blit816 | Member nearest color | GammaTable |
|---|---:|---:|---:|---:|
| Existing direct `SQR` expressions | 71.0618 | 92.5259 | 100 | 95.0571 |
| Include `Utils/Square.h`, no calls changed | 71.0618 | 92.5259 | 100 | 95.0571 |
| Global helper's six expressions use `Sqr` | 65.1081 | 69.3111 | 100 | **100** |
| Both helpers' twelve expressions use `Sqr` | 65.1081 | 69.3111 | 77.3600 | **100** |
| Compose shared RGB-distance boundary around `Sqr` | 65.1081 | 69.3111 | 77.3600 | **100** |
| Same boundary, inner `SQR` | 74.5019 | 85.9185 | 78.9733 | 95.0571 |
| Final global boundary, direct member macro, unused includes removed | 74.5019 | 85.9185 | 100 | 95.0571 |

The template dip was inspected before composition: Blit824 acquires retail's
`this=ESI`, absent from the original `this=EDI` baseline, but not retail's
palette register. The nested template is byte-flat in all three search owners;
the nested macro changes their expression/lifetime graph again. Final Blits
still have four and six excess `mov`s respectively. All four zero-distance
exits remain, and the complete twelve ordered Blit references retain their
raw targets and addends. These are partial reconstructions, not bounded walls.
Candidate selection and reopening conditions are recorded only in the
`palette-sqr-*` lineage rows.

GammaTable is the independent control. Its body, weighted-channel helper,
callers, and fingerprint `742beec58eed` do not change. It calls the **member**
nearest-color function, never the edited global helper. Nevertheless, the
global template body makes Gamma's 518-byte/176-instruction output become
retail's complete 520-byte/177-instruction output. Include-only does not.
The complete-template and nested-template controls repeat that result.
This establishes a TU-context dependency, not which internal C1/C2 structure
causes it; no IL-tap experiment was performed here.

Raw comparison covers all 520 bytes outside the six relocation operands,
the five resolved calls (allocation, table constructor, table allocation,
array growth, member palette search), and the EH registration operand.
The retail registration at `0x1e12fb` loads FuncInfo `0x203138`; its one-state
map at `0x203158` targets cleanup `0x1e12f0`. The ten-byte registration and
eleven-byte cleanup also agree with their normalized emitted counterparts.
This is evidence for this owner, not certification of every EH family.

The exact unchanged fingerprint was banked while the real compiler output
was present. Removing the experiment and rebuilding restores Gamma's original
object and current score while retaining MAX 100. No unused template include
or inferior search implementation is retained merely to hold that state.
The older same-hash bounded scheduling verdict is superseded by this exact
proof; prior local spelling trials did not cover this TU context.

Reverse-use rule: when a genuine helper restoration moves a source-identical
neighbor, separate include visibility from helper-body and consumer effects.
Bank an audited exact unchanged-source state before removing disposable
experiments. Do not infer a source relationship from correlated movement or
flatten either real helper to preserve a current percentage. Production
compilation and raw reads produced these results; suites remain deferred to
authorized squash merge.

## Related patterns

- [`macro-origin-changes-vc5-x87-cse.md`](macro-origin-changes-vc5-x87-cse.md)
- [`scalar-byte-copy-is-an-inline-helper.md`](scalar-byte-copy-is-an-inline-helper.md)
- [`tu-state-probe-family-decides-reachability.md`](tu-state-probe-family-decides-reachability.md)
