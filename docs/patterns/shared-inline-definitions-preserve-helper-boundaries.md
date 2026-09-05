# Shared inline definitions preserve helper boundaries

tags: cpp:inline cpp:header cpp:call cpp:local | asm:call asm:mov | topic:codegen-idiom
symptoms: identical free inline helpers recur in several reconstructed TUs; their
boundaries are supported by caller code, but their separate source definitions are not
confidence: 9/10

A helper's effect on VC5 does not establish that its body was written separately
in every `.cpp`. Keep the proven inline boundary and test one definition in a
shared header. Retain parameter widths, the helper's local census, nested calls,
and distinct overload boundaries while making that test.

## Controlled consolidation

Against `606f41493`, five families occupied 30 definitions in 27 TUs:

| Family | Previous definitions | Shared owner |
|---|---:|---|
| `LookupWorker` | 16 map-taking, 4 host-taking | `DDrawMgr/WorkerLookup.h` |
| `LookupSerialRef` | 2 | `Gruntz/SerialRefLookup.h` |
| `RandRange(CGruntzMgr*, i32, i32)` | 2 | `Gruntz/RandomRange.h` |
| `PackPixel16` | 2 | `DDrawMgr/PixelShift.h` |
| `SquaredDistance` | 4 | `Wap32/TileGeometry.h` |

The retained result has six ordinary header-inline definitions. No claimed
caller body, class layout, parameter type, data owner, or RNG implementation
changed. The four `SQR(dx) + SQR(dy)` bodies use the identical `dx * dx + dy * dy`
arithmetic without acquiring a macro dependency. All five headers compile
independently with VC5 `/O2 /MT`.

The real before/after objects preserve all 3,808 recorded function source
fingerprints. After the existing fail-closed COFF normalizer resolves compiler
labels by their data and ownership, 4,399 of 4,427 compared function bodies retain
both masked instructions and ordered relocation targets/types/addends exactly.
The other 28 retain their call-target multisets and call/branch/return counts.
Their external referent multisets also agree: three functions reorder external
references, and five move owner-relative jump-table destinations with the code.
No selected helper gains or loses an out-of-line emission.

This distinguishes actual instruction movement from label-counter churn. A raw
name comparison initially reports 670 changed rows; 642 disappear when `$L` and
`$T` identities are resolved by the existing object-evidence normalizer. Do not
strip those names blindly or ignore DIR32 addends: compare the canonical data
and owning branch destinations.

## Boundaries retained

`LookupWorker` still initializes its `CObject*` inside the helper. The map-taking
and host-taking overloads each keep their own flat body: prior controlled tests
proved that placing the owner chain inside the latter changes argument setup.
See [the out-parameter reset pattern](out-param-reset-between-arg-setup-and-call-is-in-the-helper.md).
Sharing definitions does not license flattening those distinct call boundaries.

`PackPixel16` retains its by-value byte parameters, word intermediate/result,
shift globals, and nesting inside `BlendPixel16`. `LookupSerialRef` retains the
failed-lookup, null, and virtual class-discriminator checks. The manager-based
range helper retains its zero-span arm and call to `CGruntzMgr::Rand`.

The palette-returning `LookupWorker` spelling is a different typed operation.
Likewise, a range helper calling `GetRandomNumber` or CRT `rand` is not identical
to one calling the receiver-bearing manager method. Equal names or arithmetic
alone do not merge these identities.

## Reverse use

Search for duplicate complete helper bodies after recovering an inline layer.
Unify identical signatures in a semantic shared header and compile the complete
consumer family. Preserve MAX when header visibility moves unrelated compiler
state; do not restore `.cpp` copies solely to recover current scores.

The experiment proves a working shared-definition structure. It does not recover
the original header filename, original linkage spelling, or whether a developer
ever copied a helper between libraries.
