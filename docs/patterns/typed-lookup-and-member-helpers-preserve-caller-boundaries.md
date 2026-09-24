# Typed lookup templates and real member helpers preserve caller boundaries

- **confidence** c9
- **tags** `cpp:inline` `cpp:template` `cpp:member` `cpp:macro` | `asm:call` `asm:mov` | `topic:source-shape` `topic:negative-control`

## Detection and ownership

A header move fixes visibility without necessarily fixing ownership. Review the
complete helper body against existing class APIs and other typed instantiations:

* A free function whose operation belongs to its receiver can be a member.
  `HasPalette` and `ModeSize` duplicated existing member accessors. Reuse those
  accessors, including the by-value `tagSIZE` result, rather than adding new ones.
* A map lookup with a helper-local null sink and a typed pointer result can be
  shared as a template. The sink must stay inside the returning helper.
* A cursor advancing through an array is not a container pop. The animation
  cursor increments its index, wraps to zero and shifts pending/current draw
  values; it neither erases nor returns ownership of a record.
* Repeated arithmetic does not prove equal parameter widths, evaluation order,
  or source lineage. Test complete callers and retained retail entry points.

The controlled starting tree is `e17fa3f05`. Tests use the pinned VC5 `/O2 /MT`
flags of each real consumer TU. Compiler-generated label identities are resolved
by the existing COFF normalizer; raw-name churn is not an instruction change.

## Retained structure

`MapFind<T>` has overloads for `CMapStringToOb` and `CMapStringToPtr`. They replace
the worker, palette, logic-record, animation and sound-cue copies. The object-map
form retains a `CObject*` sink and the typed cast at the return. The pointer-map
form retains its typed sink and existing `MapLookup` adapter. The owner-taking
worker lookup becomes `CDDrawSurfaceMgr::FindWorker`, forwarding inside the
member to `MapFind<CDDrawWorker>`. Map-taking callers still evaluate their map
receiver before entering the helper. Both established receiver boundaries survive.

In the first 33-unit object/animation lookup control, 1,547 of 1,557 emitted
functions keep identical normalized bytes and typed relocation streams. Ten
change instruction allocation/scheduling with unchanged call-target multisets
and call/branch/return counts. No helper body is emitted. Expanding the same
pointer-map template to sound cues also preserves all call sets and emissions.
This supersedes the earlier inference that the owner-taking worker adapter must
have its own flat lookup body: nested composition preserves the exact lookup
callers, including both menu cursor configuration methods. The original evidence
for keeping the owner-chain evaluation inside that adapter still applies.

Members now own surface pixel offsets, checked/typed animation record access,
frame advancement and completion, animation registration, child-list traversal,
minimap grid access, play-state setters, Grunt queries/movement helpers, draw-fill
setters and sound-stream volume-ramp forwarding. Class layout, data ownership,
scalar widths, store order and existing retail out-of-line entry points stay the
same. The record, cursor, registration and mode-size isolated controls are
byte/relocation-identical across their compared consumers. Moving `PixelOffset`
to its surface preserves `DrawBorderRaw`; declaration visibility changes some
other functions in its consumer units.

`AnimationRegistry::AddAnimation` cannot simply become a header inline in this
source context: that trial removes its retail 0x16-byte body at 0x152ab0. The
private `RegisterAnimation` member replaces the macro while the existing public
entry remains out of line. Both loaders and the public entry use that member;
the complete isolated registry unit is byte/relocation-identical.

## Pixel and copy negative controls

| Candidate | Controlled result |
|---|---|
| Parameterized pixel macro, RGB555 supplied as constant shifts | Runtime blitters and `ResolveColorKey` stay unchanged; exact `CDib::Init` falls to 78.54%. |
| Same parameterized expression behind a byte-parameter inline | Constants fold, but `CDib::Init` still has the same wrong 78.54% shape. |
| Fixed RGB555 replaced directly by runtime `PACK_PIXEL16` | Five global shift references appear and `CDib::Init` falls to 66.35%. |
| `PackRgb16(i32,i32,i32)` forwards through the sequential byte helper | Palette callers lose their proven shape; the exact space palette falls to 61.76%. A large trigger caller gains a declined expansion. |
| Sequential `PackPixel16` forwards through full-width packing or the flat macro | `DrawGlyphRun` changes from 92.80% to 83.95%, with the same call set and CFG. |
| Guarded `memcpy` replaces `CopyBytes` | Scalar loops become `rep movs`; exact `CFaderShape::RenderTile` falls to 71.61%. |

The fixed RGB555 source decision remains in lineage ledger ID
`dib-rgb-to-16-macro`. Its authored spelling is preserved. Byte parameters,
full-width integer parameters, sequential word updates and a flat expression are
separate boundaries, even where current arguments lie in the byte range. These
controls reject the tested substitutions, not every possible shared abstraction.

`CopyBytes` is not `memset`: it reads a source buffer. It also has signed-count
and forward-copy behavior that a naked `memcpy` does not preserve for all inputs.
The trial retains the positive-count guard, so the retail rejection comes from
its scalar-versus-intrinsic instruction shape, not a changed zero-count guard.
See [the scalar-copy control](scalar-byte-copy-is-an-inline-helper.md).

## Large inline bodies can contain macros

`CFaderLight::Render` has a retail standalone body and one expansion in
`RenderFrame`. Removing inline visibility reduces the caller's compiled aligned
extent from 2400 to 1120 bytes and replaces six `__ftol` references with another
`Render` call. The standalone function alone is not enough to judge visibility.

The repeated shading and distance calculations admit smaller source units, but
ordinary nested inline methods fail at this caller composition:

* `ShadePixel` emits a new standalone helper and eight calls absent from retail.
* `DistanceAt` emits a helper and replaces an expanded `__ftol` with a call to it.
* Composing both adds twelve new helper calls. The nested candidate does not
  become correct merely because the standalone `Render` still agrees.

`FADER_SHADE_PIXEL` and `FADER_DISTANCE` share those repeated operations without
new function boundaries. The retained macro trial preserves the complete
standalone `Render` body, the caller's extent, call-target multiset and
call/branch/return counts. `RenderFrame` moves from 91.1565% to 90.8721% in the
isolated state; this is not an exact closure of its remaining wall. Its historical
MAX remains the objective. No guessed inliner cost or forced-inline directive is
needed for this conclusion: the actual emitted call targets distinguish the
controls directly.

## Review boundary and reverse use

The remaining free helpers were inspected for existing owners and semantic
counterparts. Geometry over scalar/`Coord`/`RECT` values, SDK adaptation, packed
reads/writes, pointer views, explicit allocator/free-list seams, RNG-source
adapters and operations coordinating multiple owners can remain free functions.
`Span16` returns signed words while `Pix16` returns unsigned words; they are not
identical typed views. Saved-position and coordinate-recycling helpers coexist
with pinned retail methods; merging their names or ignoring nested call
visibility would erase an evidenced call/expansion distinction. Tile lookups
retain pixel-versus-cell coordinates, direct-versus-called indexing and distinct
failure values. Draw-fill ordering variants retain their different store order.

For another audit, first search the actual class API, then compare templates,
members, nested wrappers and macros separately. Verify every consumer and every
retail out-of-line body, not only the prettiest helper or its best-scoring caller.
A successful ownership refactor does not prove its original identifier or header
filename, and this review does not classify the surrounding walls as bounded.

## Full-build evidence

The final full `gruntz build` passes after the approved baseline refresh,
including the MAX gate and every structural gate: label, class,
referent, data and undefined-closure audits. All 32 changed headers compile
independently; seven existing compiler-artifact/include-order controls pass.
The real link uses 293 objects and 19 libraries with zero unresolved or duplicate
symbols and no `/FORCE`.

Across all 8,339 common emitted functions, 8,269 retain identical normalized
bytes and ordered relocation offsets, identities, types and embedded addends.
Sixty-eight more preserve call/branch/return counts. `StepArrivalDrop` changes
132 to 130 branches and `StepCompassMove` changes 129 to 131; both retain their
call sets and returns. Every call-target multiset is preserved. No function
emission appears or disappears. These two existing caller CFG residues remain
open; their helper operations and arguments retain the same source semantics.

The approved baseline refresh records 23 fresh current-score dips and 162 edited
fingerprints; 12 fingerprints start below their previous-source best. Six of the
12 already had essentially the same lower current score before this edit.
`Refresh` improves its current state while remaining below its historical best.
The largest new source-best decrease is `HandleActionOptionsPointer`, 99.9510 to
95.1667, with unchanged calls, CFG counts and external referents. The other newly
lower edited states are `UseEquippedToolAt`, `RollingBall::Update`,
`StepCompassMove`, and `ComputeChecksum`. Six existing banks rise, including
`CTriggerMgr::ResetAll` to exact. The refresh preserves all 4,439 historical
maxima and every unchanged-source MAX and removes no baseline row. The source
retains its natural owners rather than adding inert declarations to recover a
transient allocation state.
