# Surviving sibling source restores typed layers and authored order

tags: cpp:inheritance cpp:inline cpp:member cpp:local cpp:loop | asm:lea asm:mov | topic:source-oracle topic:codegen-idiom topic:evidence-discipline
symptoms: a reconstructed subsystem has correct broad behavior but repeated near-exact
register/schedule walls, generic base pointers where consumers always recover one owner
type, payload unions at a common trailing offset, or hand-expanded one-field stores
confidence: 10/10 (twenty clean exact closures, four audited unchanged-source state closures,
exact controls, revision-history audit, and retail negative controls)
variants: inline-expansion-boundary-pins-a-neighbour.md, ctor-body-first-statement-is-an-inline-member.md

## Evidence

The [public LithTech tree](https://github.com/jsj2008/lithtech) contains `libs/rezmgr`,
the `libs/lith` hash/list and debug libraries it uses, and the 1996 `libs/dibmgr` image
decoder family. File headers date these families to 1995-1997. Its public Git history is
also unusually clean for reverse use: the implementations were imported in commit
`845119c`; later commits changed only build files in these libraries. ButeMgr received one
2012 portability patch, so its changed stream-size types and casts are excluded from the
old-MSVC prior.

The pinned corpus is presumptively authentic for matching lineages. Gruntz retail
instructions, relocations, ABI, and ownership still decide a measured conflict. Applied
as a complete source layer with controlled adaptations, it closed these Gruntz functions:

| retail function | surviving source fact | before -> after |
|---|---|---:|
| `CRezFile::Close` 0x13c830 | function-scope `ok`/`check` locals and one shared cleanup tail | 93.0769 -> **100.000** |
| `CRezItm::Read` 0x139af0 | reuse `byteCount` and `m_nCurPos` directly through the three storage arms | 94.6739 -> **100.000** |
| `CRezTyp` ctor 0x139bf0 | typed member setter plus authored body assignment order | 99.3548 -> **100.000** |
| `CRezItm::InitRezItm` 0x139710 | typed one-field `SetRezItm` boundary | 96.2963 -> **100.000** |
| `CRezMgr::AllocateRezItm` 0x13c0c0 | original local census and typed table `Delete` wrapper | 98.2174 -> **100.000** |
| `CBaseHash::GetLast` 0x184b10 | original decrementing `do` loop | 99.4737 -> **100.000** |
| `CBaseHash::Insert` 0x184a70 | real base/derived hash-item layout and implicit intrusive-base conversion | 99.5455 -> **100.000** |
| `CRezMgr::OpenAdditional` 0x13b0c0 | typed hash hierarchy at its consumers | 98.2888 -> **100.000** |
| `CRezMgr::Open` 0x13ad00 | hierarchy first, then original header-to-member statement order | 98.4567 -> 98.7437 -> **100.000** |
| `dprintfmonoincline` 0x184d50 | `unsigned short*` mono buffer and element-indexed scrolling/clear loops | 98.5714 -> **100.000** |
| `dprintfmonoclrscr` 0x184db0 | same typed buffer and ordinary element-indexed `for` loop | 99.0000 -> **100.000** |
| `CRezImage::DecodePcxData` 0x176000 | 1996 DIB decoder's function-scope local census, reverse RLE fill, and direct plane indexing | 97.6772 -> 99.9240 -> **MAX 100.000** |
| `CRezImage::DecodePidData` 0x176440 | unsigned eight-word header census, named transparency value, direct run-byte rereads, and manual literal loop | 81.4772 -> **100.000** |
| `CRezImage::DecodeBlit` 0x175930 | complete `IsStrideless`/size/height/index/width accessor layer and advancing the incoming pixel pointer | 95.3194 -> 99.9722 -> **MAX 100.000** |
| `CRezImage::FlipVertical` 0x176840 | three unsigned offsets, shared forward index, cached dimensions, and ordinary copy loops from `CDib::Invert` | 79.7879 -> **100.000** |
| `CRezImage::SaveBmp` 0x176b30 | two-stage palette fallback, validity/data/palette accessors, symbolic file sizing/open flags, and named row index | 98.9855 -> **100.000** |
| `CDDSurface::CreateFromPcxData` 0x144b30 | complete 128-byte PCX header plus `pStart`, typed header, offset, and packed-byte cursor | 99.9766 -> **100.000** |
| `Blowfish_encipher` 0x16f7f0 | surviving `aword` view and `S`/`bf_F`/`ROUND` macro family, paired with the proven declaration-state boundary | 60.3505 -> 99.9357 -> **100.000** |
| `Blowfish_decipher` 0x16fc70 | the mirror source body retained through its reciprocal first-step dip, then composed with the declaration boundary | 100.000 -> 61.4969 -> **100.000** |
| `InitializeBlowfish` 0x170100 | original short local census, `aword temp`, table copies, symbolic round count, and authored xor assignment | 100.000 -> **100.000** |

The exact controls are strong: all three typed hash lookups stayed exact, both hash-owning
destructors stayed exact, and `rezarchive` reached 115/115 exact functions.

The same audit restored the complete storage hierarchy:
`CVirtBaseListItem`/`CVirtBaseList`, typed `CBaseRezFileList` and
`CRezFileSingleFileList`, then `CBaseRezFile`, `CRezFile`,
`CRezFileDirectoryEmulation`, and `CRezFileSingleFile`. Those are
layout-identical replacements for the inferred flattened list/file classes,
but they restore the real inheritance, typed accessors, method names, const
input boundary, and member ownership. Every non-EH function in the `rezfile`
and `rezlist` units remained exact after the complete replacement. The
surviving ninth virtual `GetFileName` was not imported: Gruntz retail has
eight-slot vtables for all four storage classes, so the later API would be a
real ABI change rather than a harmless declaration cleanup.

Exact code is not permission to keep invented identities. The complete
17-function `libs/lith/dprintf.cpp` family maps one-for-one onto the retail
`debugprintf` TU. Restoring its classes, overloads, globals, output enum, local
census, and statement order left all 17 functions exact and preserved 100% of
the TU's text and data. This is a controlled positive case where the surviving
source materially improves reconstruction fidelity without moving the score.

The useful corpus is broader than `libs/`. Shogo and Blood2 contain byte-identical
`NetStart_FillServiceList` implementations. They independently preserve a cursor local,
a per-iteration `pService` alias, and direct `LPARAM` casts around `LB_ADDSTRING` and
`LB_SETITEMDATA`. Restoring those layers in `CNetMgr::PopulateProviderList` is byte-flat
at 97.2959%, but replaces an invented union-punning temporary with the authored Win32
boundary and gives later searches the right local census.

Small value types require the same complete-family treatment. The surviving `CARange`
and `CAVector` declarations removed the inferred `ButeRefLarge` inheritance layer and
restored their protected members, inline constructors, setters, getters, and authored
accessor use. Applying only their empty default constructors moved the two static-default
getters from 99.9296/99.9275 down to 90.6338/93.2609. Composing the surviving
`static CAVector(0,0,0)` and `static CARange(0,0)` sites returned both to their original
scores, while the same authentic header state banked `CButeMgr::GetFloat` at exact.
This is another direct control against one-step ranking: constructor source and its
initialization sites are one evidence family.

The direct CryptMgr Blowfish source supplies another complete abstraction family. The
earlier reconstruction had flattened the byte view into scalar shifts and expressed each
round through a hand-written comma macro. Restoring the surviving `union aword`, its
little-endian bitfields, the nested `S`/`bf_F`/`ROUND` macros, paired round source-line
groups, and the original initialization locals initially moved the mirror functions in
opposite directions: encipher rose from 60.3505 to 99.9357 while decipher fell from exact
to 61.4969. Their call sets and branch skeletons still agreed with retail. Retaining that
source and composing the previously measured real-header declaration boundary between the
two definitions made both 16-round bodies and `InitializeBlowfish` byte-exact. A reciprocal
mirror-function dip is therefore evidence to inspect TU state, not permission to discard a
surviving algorithm layer.

The Bute scanner shows that the same rule covers semantic data identity and storage
scope. Its earlier reconstruction exposed a 256-entry character map and a
`short[97][49][3]` transition array as generic TU globals, then invented a slot enum to
explain the final dimension. The surviving source instead names `ClassMap`, declares a
`TranType { ActionType, A, B }`, and accesses those fields directly. It also proves that
the token-buffer cursor is `static short Pos` inside `ScanTok`, not a TU-global datum.
Restoring those facts preserves the table bytes and scanner operations while recovering
the entity that owns the local static. Semantic method/member identities and the
overloaded getter family were restored in the same pass, including the inline
`GetChecksum` boundary at its only external consumer. This is a source-model correction
even where the generated instructions were already exact: address and width alone do not
prove scope, record layering, or the developer-facing API.

## The class-model lesson: a trailing union can hide a missing derived layer

The earlier reconstructed `CHashElement` put the owner payloads in a union:

```cpp
union {
    CRezItm* m_archiveEntry;
    CRezTyp* m_archiveType;
    CRezDir* m_archiveDirectory;
};
```

That preserved every observed complete-object size, but it was still the wrong model. The
surviving hierarchy proves that the base item ends after `{ parentHash, currentBin }` and
each typed derived node owns one payload pointer at base+0x14. The complete derived node is
the same 0x18 bytes, so layout checks alone cannot distinguish the two declarations.

Typed node/table wrappers (`GetRezItm`, typed `GetFirst`, `Insert`, `Delete`, and
bucket-local iteration) then express the real abstraction. Their bodies normally fold
away, but the front end sees different typed operations and inline boundaries. That was
enough to close `Insert` and `MergeArchive` and to move `Open` to its final schedule.

This hierarchy also explains the retail `+4/-4` adjustments. A polymorphic hash item has
a vptr at +0 and its intrusive-list base subobject at +4. Developers wrote ordinary
base/derived conversions; VC5 emitted the null-preserving adjustment. Explicit
`reinterpret_cast<char*>(link) - 4` source is therefore a decompiler artifact here.

The same audit falsified a second union at a different semantic level. The reconstructed
crypto stream used one `BlowfishBlock` union to view each eight-byte record as bytes,
words, and a signed length byte. The surviving `CCryptMgr` instead declares ordinary
`char buf[8]`/`char tbuf[8]` locals, converts only at the cipher boundary, and copies the
previous record with `memcpy`. Restoring that source made both `Encrypt` and `Decrypt`
byte-exact without the union. When a union merely explains several emitted access widths,
look for an authentic typed operation at the boundary before treating the overlay as an
object the developers declared.

## Authentic declarations can rotate a commuted SIB without changing the model

Restoring the surviving names and owners all the way from `CHashBase`/`CHashElement` to
`CBaseHash`/`CBaseHashItem` left the complete call sets, CFGs, registers, extents, and
referents intact. It changed only one interchangeable scale-1 SIB byte in each of
`CBaseHashItem::Next`, `CBaseHash::Insert`, `CBaseHash::GetLast`, and
`CBaseHash::GetFirstInBin` (99.6970, 99.5455, 99.4737, and 99.0000 current fuzzy). The
all-time `hist` remains 100 for all four; their new authentic source hashes must be
banked independently.

This controlled A/B is a declaration-state effect: source expression reversal cannot
select a different encoding for the mathematically identical `[base+index]` address.
Keep the authentic hierarchy and names, bank the correct source state, and use a bounded
target-adjacent C1-state trial if current exactness is needed. Do not retain an inferred
class name merely because it happened to select retail's interchangeable encoding.

## Authored order survives optimization indirectly

Do not transcribe C2's store order. In `Open`, retail's scheduled member stores look
interleaved, but the source prior gives the authored sequence:

```cpp
m_nNextWritePos = header.m_nNextWritePos;
m_nRootDirPos = header.m_nRootDirPos;
m_nRootDirSize = header.m_nRootDirSize;
m_nRootDirTime = header.m_nRootDirTime;
m_nLastTimeModified = header.m_nLastTimeModified;
m_nFileFormatVersion = header.m_nFileFormatVersion;
```

Moving only `m_nFileFormatVersion` from the front of the reconstructed block to this position closed
the final 1.2563%. The reusable lever is the source statement order, not the emitted store
order; VC5 uses the former to build the IL and schedules the latter.

## Adoption control

The pinned source is a presumptive source oracle, not merely a bag of optional
clues. Start with the complete source layer and accept it unless Gruntz retail
provides a specific contradiction. A first-step score descent remains a valid
composition base; `CRezMgr::Open` and the DIB family demonstrate why ranking
one sourced fact in isolation traps the search in a local maximum.

[The canonical lineage ledger](../../config/lithtech_lineage.tsv) is the only
place that records candidate status and source-specific exceptions. In
particular, the retained Rez, Bute, DIB, and RegMgr adaptations are identified
there by `rez-read-cursor-guard`, `bute-*-byte`,
`bute-stream-parse-boundary`, `bute-save-adaptation`,
`dib-init-adaptation`, and `reg-*`. This pattern intentionally does not
repeat their rejection rationale. Run `gruntz lineage inventory --todo` for
the dependency/MAX-ordered adoption queue and `gruntz lineage verify --source
<checkout>` to prove pinned blob identity and discovery coverage.

A `take-adapted` decision means the surviving owner and source layer are
retained while a precisely measured retail difference is substituted. It does
not license a hand transcription of the rest of the function. A
`do-not-take` decision rejects only the ledger entity it names; absence or
later behavior elsewhere in the sibling revision is not negative evidence
about Gruntz.

## Reverse-use procedure

1. Audit the external repository's own history first. Separate imported historical source
   from later portability or modernization patches.
2. Map owners and complete layouts in both directions. Prefer facts repeated across a
   class family: base extent, payload offset, typed wrappers, constructor order, and all
   consumers.
3. Import one positive fact or a small composed family as a disposable A/B. Diagnose and
   compare from the first real divergence.
4. Treat added branches, fields, signedness, and parameter-width changes as revision skew
   until Gruntz retail independently supports them.
5. Keep an authentic first-step dip available for a bounded composition. The type-ctor
   setter went down before its surviving assignment order took it to exact. Confirm the
   feature gained by the dip was absent from the higher baseline. Also keep mining the
   repository after one related implementation stalls: the later PCX decoder supplied a
   useful byte type, but the same-era DIB library supplied the authored body that escaped
   the cursor-local maximum and reached an auditable exact state.
6. Run the full build. Shared header declarations are C1/TU-state inputs; preserve correct
   structure and bank MAX rather than deleting a proven layer to recover incidental
   current scores.
