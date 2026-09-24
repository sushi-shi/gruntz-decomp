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

## Second sweep: owner headers and open-coded macros

After rebasing onto `37ace99bd`, a tree-wide scan (exact, alpha-renamed and
type-erased body hashes over every `.cpp` and header) found the rest:

| Family | Copies | Shared owner |
|---|---:|---|
| `LookupAnimation` (was `LookupAnim`/`LookupAni`/`FindAnimElement`) | 10 | `Gruntz/AniElement.h` |
| `LookupSoundCue` (was `LookupCue`) | 8 | `Gruntz/SoundCue.h` |
| `ResolveRegisteredAct<Logic>` (seven named wrappers) | 7 | `Gruntz/ActReg.h` |
| `typedef CActHandler` | 5 | `Gruntz/UserLogic.h` |
| `DispatchUnhandledLogicEvent` (two headers, two TUs) | 4 | `Gruntz/LogicEventDispatch.h` |
| `LookupLogicTemplate` | 3 | `DDrawMgr/LogicRecord.h` |
| `CLEAR_TAB_HINT` | 3 | `Gruntz/SoundCueRegistry.h` |
| `LookupPaletteResource` (was `LookupWorker`/`LookupRecord`) | 2 | `DDrawMgr/DDrawPaletteResource.h` |
| `ListGetFirst`, `ListGetNext` | 2 + 2 | `DDrawMgr/DDrawChildGroup.h` |
| `ScreenTile(Coord*)`, `ScreenTile(CGrunt*)` | 2 + 2 | `Gruntz/GruntMovementInline.h` |
| `PackRgb16` | 2 | `DDrawMgr/PixelShift.h` |

TU-local macro copies of header macros (`SCAN_BOUNDS`, `MOVE_RECYCLE`,
`STEP_DRAIN`, `FREELIST_PUSH`, a second `LOGIC_RECORD_DISPATCH`) now use the
header form. Among these, the `if`/`do`-`while` drain compiles byte-identically to the
header's `while`. 64 open-coded sites whose token stream equals an existing
macro's expansion now invoke the macro. Being textual, that swap cannot move
code; the only object changes came from includes added to five functions'
TUs. No shared helper gained an out-of-line body.

Deliberately left alone: open-coded bodies of inline *functions* (a call
boundary can change allocation), near-duplicates whose store order, control
spelling, RNG source or type differs, and identical bodies that are separate
retail functions.

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

## Separate retail copies of one function: an anonymous-namespace include

Retail holds three byte-identical 0x45-byte `FileExists` bodies: the extern one at
0x1189c0 that game TUs call, and private copies at 0xf90f0 (sound-font path) and
0x1fd70 (CD-ROM probe), each called only inside its own TU. `Utils/FileExists.h`
now holds the single definition. `HeapDiag.cpp` includes it at file scope; the two
private users include it inside `namespace { }`, which gives each copy a distinct
symbol and its own address. Unlike an `inline` helper, the definition is an
ordinary out-of-line function, so no caller expands it.

`RVA()` in the header would give all three copies one address, so each TU pins its
copy with `RVA_COMPGEN(rva, 0x45, <symbol>)` instead. For the private copies the
symbol spells the canonical source-file namespace `?A0x<sha256("src/...")[:16]>`
that `msvc_names.anonymous_namespaces` produces. The namespace has to open in the
`.cpp`: VC5 names it after the file that opens it, and only source-file identities
are canonicalized. All three pins stay 100.00 exact.

## Negative control: identical members of two classes are two definitions

`CBattlezDlg` and `CMultiStartDlg` carry five text-identical members
(`GetPlayerTypeControl`, `GetPlayerNameControl`, `GetMaxGruntzControl`,
`GetPlayerColorControl`, `OnDrawItem`), and retail keeps a separate copy of
each. A plain shared base would leave one copy. A CRTP base
`CPlayerSlotDlg<Dlg> : CDialog` would give two, but a VC5 trial on both TUs
emits a vtable `??_7?$CPlayerSlotDlg@...@@6B@`, its `??_R0`-`??_R4` RTTI and an
out-of-line `??1`/`??_G` destructor pair per dialog. Retail has one vtable per
dialog, no locator before it, and no intermediate destructor. Both message maps
also chain straight to `&CDialog::messageMap`, the immediate-base convention. The
members stay as two written definitions. Recognize the refutation by an
intermediate class's vtable, RTTI or destructor appearing in the base obj with no
retail counterpart.

## Third sweep: every TU-local inline and macro is a candidate

Duplicate detection did not finish the ownership audit. Against `15815a1f7`, a
comment-aware scan of every C/C++ source under `src/` found 208 definitions in
73 files: 108 inline functions and 100 macros. Include `__inline`, `_inline`,
and `__forceinline` in this scan: searching only for the token `inline` missed
23 definitions. Inspect object-like statement macros as well as function-like
macros. There are no `.cpp`-local class definitions hiding implicit inline
members in this tree.

192 definitions (106 inline functions and 86 macros) moved to 76 owner or
focused inline/macro headers. This includes single-consumer helpers: a single
current caller does not prove a `.cpp` definition. Complete class definitions
remain in their existing owner headers; focused inline headers include the
complete types they use. No storage definition or data address moved. The
`CFaderLight::Render` RVA annotation travels with its definition.

The two `ScreenPosition` definitions now share the original Brickz spelling,
including its Y-before-X stores to the returned `Coord`. Their callers retain
the same one-call topology. The two different `TileNeighborhood` operations
remain separate functions, named `AttackTileNeighborhood` (reach-dependent)
and `AdjacentTileNeighborhood` (the neighboring 3-by-3 cells). Similar names
were not evidence of interchangeable behavior. Likewise, the clock serializers,
tile lookup variants, sound lookup layers, and RNG adapters retain their distinct
source bodies and boundaries.

Only these 16 definitions remain in implementation files:

| Definitions | Evidence for retaining the location |
|---|---|
| `bf_N`, `S`, `bf_F`, `ROUND` | Surviving source; see lineage ledger ID `blowfish-authentic-source-layer`. |
| `CRezDir::IsGoodChar` | Surviving source; see lineage ledger ID `rezdir-isgoodchar-helper`. |
| `CGrunt::SelectCombatHitCue`, `LK` | The helper selects the owning TU's unique, address-pinned `static` sound-name arrays. Moving those arrays into a header would create different storage; changing their linkage would expose private data. Keep this inline implementation with its data. |
| `GRUNTZ_MENUITEM_TU` | A pre-include configuration switch for the separately emitted constructor, not an operation helper. |
| `INPUTDEVICE_FILE`, `DINMGR2_FILE`, `DSNDMGSR_FILE`, `DSNDMGR_FILE`, `DIRPAL_FILE`, `DIRSURF_FILE`, `DDRAWMGR_FILE`, `DDRAWMGR_H_FILE` | Diagnostic source-path literals, not operation helpers. |

Reproduce the remaining lexical census with:

```sh
rg -n '\b(inline|__inline|_inline|__forceinline)\b|^\s*#\s*define\b' src
```

Read the hits rather than treating comments, configuration, or literals as
helpers. The source-lineage decisions remain in `config/lithtech_lineage.tsv`;
this census is not a wall worklist or proof that open-coded inline expansions
have all been recovered.

All 76 changed headers compile independently with VC5 `/O2 /MT`. The full
build's source/data gates pass, including label extraction with zero violations.
The two existing lifetime-operation audit entries merely follow their source
definitions to the new headers; their allowed types and counts are unchanged.
Seven existing compiler-artifact and include-order controls also pass.

Across all normalized before/after objects, 8,270 of 8,338 common function
bodies retain identical raw bytes and relocation offsets, targets, types, and
embedded addends. Another 64 retain their call-target multisets and
call/branch/return counts. Four unchanged caller bodies show compiler movement:
`StepArrivalDrop` declines one `CPtrList::RemoveHead` expansion;
`HandleTargetSelection` declines one `CLightFx::Activate` expansion;
`StepCompassMove` and `LoadTileArrivalFx` change branch counts with unchanged
call sets. These are observed consequences of the new TU declaration/definition
environment, not newly reconstructed caller logic. Keep the owner-header source
and preserve its banked MAX rather than adding inert declarations to restore a
current score. The baseline refresh records this adjudicated current state;
historical maxima remain intact.

The only additional function emission is `CSpawnList::~CSpawnList` in the voice
manager TU, where the shared class is used. Its bytes and typed relocations are
identical to the existing AreaMgr copy; both are ordinary selection-2 COMDATs.
This is a natural duplicate emission of the same inline definition, unlike the
additional class artifacts in the rejected dialog-base experiment.

For future audits, first establish the complete lexical census, then assign each
definition a semantic owner or an evidence-backed reason to remain private.
Compile headers on their own and their complete consumer family. Check emitted
symbols and ordered relocations as well as scores: header visibility can change
which body is emitted or expanded even when the moved text is unchanged.
