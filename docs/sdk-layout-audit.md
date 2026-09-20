# SDK layout audit

This pass replaces project-owned copies of SDK objects with the declarations
shipped in the pinned VC5/DirectX toolchain. Layout agreement proposes a
candidate; the producer, consumer, object extent, and retail call family decide
whether it is the same object.

Run the census with:

```
nix develop -c python3 scripts/audit-sdk-layouts.py
```

The script reads the real compilation database with pylibclang, visits template
bodies, and also parses source/header files not reached by those translation
units. It emits records, candidate pairs, diagnostics, coverage, and a compact
TSV under `build/audits/sdk-layouts/`. Its comparison includes complete size,
alignment, field offsets, signedness, widths, and array lengths. Pointer targets
and nested record semantics deliberately remain review questions. A candidate
is not an automatic rewrite or a cleanliness failure.

## Replacements and evidence

| Previous representation | SDK representation | Deciding evidence |
| --- | --- | --- |
| `DIMouseStateZ`, `DIJoyState2Z`, `DeviceState` | `DIMOUSESTATE`, `DIJOYSTATE2`, SDK `void*` buffer boundary | Producers select `c_dfDIMouse` and `c_dfDIJoystick2`; retail allocates 0x10 and 0x110 bytes, respectively. The joystick button array begins at 0x30. |
| `WaveFormatX`, `WaveFormatSdk` | `WAVEFORMATEX` | RIFF format fields feed DirectSound's `lpwfxFormat` and `SetFormat` directly; all seven fields and the packed 18-byte extent agree. |
| `CDDSurface` anonymous descriptor and word overlay | `DDSURFACEDESC` | `GetSurfaceDesc`, lock, and creation use the complete 0x6c-byte SDK object; pixel format and caps are its actual nested members. |
| `BltFxWords` | `DDBLTFX` | SDK `Blt` receives the 0x64-byte object; the old word 0x14 is `dwFillColor`. Zeroing now uses `memset` on the SDK object. |
| `CDDrawRect` | `RECT` | Rectangle overlap consumes the same four edges copied from existing `RECT` members in the object-placement family. |
| `NetGuid` | `GUID` | DirectPlay creation/session descriptors consume the same application GUID; the unused four-word arm had no consumers. The initializer's sixteen bytes are preserved. |
| `CShadeTableArray`, `CFaderArray` | Real `CArray` instantiations | Retail serializers, destructors, and vtables agree with the shipped MFC template family; callers now use `Add`, `RemoveAt`, `RemoveAll`, and indexed access. |
| `CRezBufferObject`, `InitRezElem`, `ZeroRecords` | `CArray<RezElem40, const RezElem40&>`, compiler-generated constructor, `ConstructElements` | The element's two `CRect` members explain its implicit constructor. The real template emits the exact 0x1ce-byte serializer and all ten ordered relocations. See the controlled experiment below. |
| `CoordNode`, `CGruntCoordList`, MFC pointer adapters | `POSITION`, `CPtrList`, `CPtrArray` public API | Node addresses originate in `GetHeadPosition`/`GetTailPosition` and are passed back to `RemoveAt`; the fake fields copied MFC's private `CNode`. The wrapper at 0x29a30 is the real non-const `CPtrList::GetNext`, attributed through the established MFC header-inline label channel. |
| `CString` exposed through `char**` | `CString*` and its public text conversion | The collection constructs and destroys real `CString` elements. Text readers no longer assume its private first-member layout. |

Unused `BmpInfoHeaderStamp`, `ClipRect16`, and `CSpawnNode` were removed. The last
was another unused spelling of a private MFC list node. No substitute shells or
layout-compatible aliases remain for these findings. Aliases for the real MFC
template instantiations introduce no new record definitions.

## Controls and limits

The census is complemented by inspection of SDK-facing input, sound, display,
image, networking, and MFC collection code, the existing padding inventory, and
union/pointer adapters. This matters because a padded partial SDK copy need not
match the SDK's complete field fingerprint, and a one-pointer `CString` view
need not declare a record at all.

* `PidHeader` shares a header and width/height spellings with `CDDSurface`, but
  is a PID file record. Its fields remain separate. AST edits must check the
  field's semantic owner, not merely its filename and spelling.
* `PcxRgb` and `RGBTRIPLE` have identical three-byte storage but opposite channel
  order (RGB versus BGR). The PCX file type remains.
* `Bmp256Info`, `DIB_BMI256`, `DIB_LOGPAL256`, and `LogPal256` supply 256 palette
  entries. The SDK's one-entry trailing-array declarations cannot replace
  these complete storage objects. Their SDK headers/entries remain SDK types.
* `BmpFileHeaderStamp` contains the actual `BITMAPFILEHEADER` plus a byte view
  used for file I/O; it does not redeclare the bitmap header fields.
* `WwdRect::Contains` includes its maximum edges and accepts an engine grid node.
  Replacing that domain/API with Win32 rectangle containment would change it.
* `Coord`, waypoints, dimensions, timers, identity pairs, parser transitions,
  intrusive engine nodes, and HSV triples can share a primitive layout with SDK
  records without representing those records. Their complete use family is the
  negative control, not the similarity of field counts.
* Bute point/rectangle ownership is governed by lineage ledger ID
  `bute-string-point-rect-api`; this audit does not override that decision from
  layout similarity alone.
* Unexplained class gaps are not proven SDK objects merely because their sizes
  equal `DSCAPS` or `DDCAPS`. No SDK producer was found for the questioned sound
  or display-manager gaps; their identity remains part of data-model recovery.

## Why the old cast checks missed this

DirectInput writes through `void*`, so a fake consumer type does not inherently
need a cast. Commit `d516fc37c` introduced `DIJoyState2Z` while explicitly naming
`DIJOYSTATE2`; `d9e3104e6` introduced `RecordBytes` pointer unions during cast
cleanup. Similar unions exposed MFC private nodes and `CString` internals.
Removing the spelling of a cast did not establish correct type identity.
The audit follows producers, consumers, and actual SDK declarations instead.

## Verification

The isolated SDK branch excludes unrelated in-progress Grunt source edits.
The final census covers 282 translation units, 1,571 records, and 359 owned
records. All 68 layout candidates have a disposition in
[`sdk-layout-review.tsv`](sdk-layout-review.tsv); there are zero parse errors,
uncovered files, or supplemental headers. The pinned VC5 compiler passes 73
SDK/owner size and member-offset assertions. The data-access consumer passes
21 tests, including seven complete-copy positive/negative controls and the
existing whole-tree injected-defect controls.

The final pinned `gruntz build` passes the MAX check and every fast/normal gate.
Comparing baseline rows by retail RVA confirms no historical-MAX decrease for
the 4,429 retained functions (ten increase). The sole removed score row is the
MFC header-inline function at 0x29a30, whose identity is recorded in
`config/retail/functions_static_libs.tsv`.

SDK `DWORD` dimensions require a signed conversion when used for engine
coordinate comparisons, division, or floating-point geometry. The first
descriptor replacement exposed two unsigned `div` instructions in mesh
initialization where retail has `idiv`. Reviewed value conversions restore both
signed divisions and the signed half-dimension operations while retaining the
SDK object. Plain copies, byte counts, and bit masks need no blanket conversion.

The source-family corrections reopen the previous matching certifications for
`AdvanceToEnemyBase`, `StepDefenderUnit`, and mesh `ApplyInit`. Their inline
boundaries remain matching work; the old copied SDK classes are not valid
fallbacks. The function at 0x29a30 is now attributed to the MFC header-inline
family, so it leaves the reconstructed game-function score set. Current score
changes are adjudicated as consequences of these SDK ownership/API fixes;
historical MAX is retained by the baseline banking tool. Generated audit output
is evidence, not a replacement for a full build or a hand-maintained matching
ledger.

The real GUID additionally exposed a width-verifier false positive. Its fix
proves complete decoded copies and tests the actual width consumer; see
[SDK record copies](patterns/sdk-record-copies-cross-member-widths.md).
