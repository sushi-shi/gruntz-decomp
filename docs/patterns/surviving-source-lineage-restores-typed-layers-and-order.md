# Surviving sibling source is a positive-only oracle for typed layers and authored order

tags: cpp:inheritance cpp:inline cpp:member cpp:local cpp:loop | asm:lea asm:mov | topic:source-oracle topic:codegen-idiom topic:evidence-discipline
symptoms: a reconstructed subsystem has correct broad behavior but repeated near-exact
register/schedule walls, generic base pointers where consumers always recover one owner
type, payload unions at a common trailing offset, or hand-expanded one-field stores
confidence: 10/10 (thirteen clean exact closures, two audited unchanged-source state closures,
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

The corpus is not Gruntz ground truth. Names and later behavior differ. It is a sibling
revision whose positive structure must be tested against retail instructions and
relocations. Applied that way, it closed these Gruntz functions:

| retail function | surviving source fact | before -> after |
|---|---|---:|
| `CRezItm::Close` 0x13c830 | function-scope `ok`/`check` locals and one shared cleanup tail | 93.0769 -> **100.000** |
| `CRezArchiveEntry::Read` 0x139af0 | reuse `byteCount` and `m_cursor` directly through the three storage arms | 94.6739 -> **100.000** |
| `CRezArchiveType` ctor 0x139bf0 | typed member setter plus authored body assignment order | 99.3548 -> **100.000** |
| `CRezArchiveEntry::Initialize` 0x139710 | typed one-field `SetArchiveEntry` boundary | 96.2963 -> **100.000** |
| `CRezArchive::AcquireEntry` 0x13c0c0 | original local census and typed table `Delete` wrapper | 98.2174 -> **100.000** |
| `CHashBase::Last` 0x184b10 | original decrementing `do` loop | 99.4737 -> **100.000** |
| `CHashBase::Insert` 0x184a70 | real base/derived hash-item layout and implicit intrusive-base conversion | 99.5455 -> **100.000** |
| `CRezArchive::MergeArchive` 0x13b0c0 | typed hash hierarchy at its consumers | 98.2888 -> **100.000** |
| `CRezArchive::Open` 0x13ad00 | hierarchy first, then original header-to-member statement order | 98.4567 -> 98.7437 -> **100.000** |
| `MonoNewline` 0x184d50 | `unsigned short*` mono buffer and element-indexed scrolling/clear loops | 98.5714 -> **100.000** |
| `MonoClear` 0x184db0 | same typed buffer and ordinary element-indexed `for` loop | 99.0000 -> **100.000** |
| `CRezImage::DecodePcxData` 0x176000 | 1996 DIB decoder's function-scope local census, reverse RLE fill, and direct plane indexing | 97.6772 -> 99.9240 -> **MAX 100.000** |
| `CRezImage::DecodePidData` 0x176440 | unsigned eight-word header census, named transparency value, direct run-byte rereads, and manual literal loop | 81.4772 -> **100.000** |
| `CRezImage::DecodeBlit` 0x175930 | complete `IsStrideless`/size/height/index/width accessor layer and advancing the incoming pixel pointer | 95.3194 -> 99.9722 -> **MAX 100.000** |
| `CRezImage::FlipVertical` 0x176840 | three unsigned offsets, shared forward index, cached dimensions, and ordinary copy loops from `CDib::Invert` | 79.7879 -> **100.000** |

The exact controls are strong: all three typed hash lookups stayed exact, both hash-owning
destructors stayed exact, and `rezarchive` reached 115/115 exact functions.

The useful corpus is broader than `libs/`. Shogo and Blood2 contain byte-identical
`NetStart_FillServiceList` implementations. They independently preserve a cursor local,
a per-iteration `pService` alias, and direct `LPARAM` casts around `LB_ADDSTRING` and
`LB_SETITEMDATA`. Restoring those layers in `CNetMgr::PopulateProviderList` is byte-flat
at 97.2959%, but replaces an invented union-punning temporary with the authored Win32
boundary and gives later searches the right local census.

## The class-model lesson: a trailing union can hide a missing derived layer

The reconstructed `CHashElement` put the owner payloads in a union:

```cpp
union {
    CRezArchiveEntry* m_archiveEntry;
    CRezArchiveType* m_archiveType;
    CRezArchiveDir* m_archiveDirectory;
};
```

That preserved every observed complete-object size, but it was still the wrong model. The
surviving hierarchy proves that the base item ends after `{ parentHash, currentBin }` and
each typed derived node owns one payload pointer at base+0x14. The complete derived node is
the same 0x18 bytes, so layout checks alone cannot distinguish the two declarations.

Typed node/table wrappers (`GetArchiveEntry`, typed `First`, `Insert`, `Delete`, and
bucket-local iteration) then express the real abstraction. Their bodies normally fold
away, but the front end sees different typed operations and inline boundaries. That was
enough to close `Insert` and `MergeArchive` and to move `Open` to its final schedule.

This hierarchy also explains the retail `+4/-4` adjustments. A polymorphic hash item has
a vptr at +0 and its intrusive-list base subobject at +4. Developers wrote ordinary
base/derived conversions; VC5 emitted the null-preserving adjustment. Explicit
`reinterpret_cast<char*>(link) - 4` source is therefore a decompiler artifact here.

## Authored order survives optimization indirectly

Do not transcribe C2's store order. In `Open`, retail's scheduled member stores look
interleaved, but the source prior gives the authored sequence:

```cpp
m_nextWritePos = header.m_nextWritePos;
m_rootDirectoryOffset = header.m_rootDirectoryOffset;
m_rootDirectorySize = header.m_rootDirectorySize;
m_rootDirectoryTime = header.m_rootDirectoryTime;
m_archiveTime = header.m_archiveTime;
m_version = header.m_version;
```

Moving only `m_version` from the front of the reconstructed block to this position closed
the final 1.2563%. The reusable lever is the source statement order, not the emitted store
order; VC5 uses the former to build the IL and schedules the latter.

## Revision-skew controls

Use the corpus only in its positive direction:

* The surviving `CRezItm::Read` has an additional cursor-range guard. Adding it changes
  Gruntz retail's CFG, so the guard belongs to the later revision and was rejected.
* The surviving Bute scanner uses `unsigned char`; Gruntz's already-exact helper family
  and mangled claims support `char`, so the width was not imported.
* Bute's boolean state fields are useful layout-compatible type evidence, but changing
  them did not move `Save`; a correct type correction is not automatically a wall lever.
* The surviving `Save` declares its 4096-byte transfer buffer at function scope before
  the input stream. Gruntz retains that humane scope correction byte-flat. The later
  implementation's optional output filename, `is_open` guard, zero-length promotion,
  stream-failure exit, and format-flag writes are absent from retail's 38-call/20-branch
  topology and were not imported. Forty-four syntax-aware compositions from the corrected
  base all emitted one 1028-byte compiler island; the remaining EBX literal pinning is a
  bounded C2 residue, not permission to invent a carrier local.
* The surviving Bute setter style commonly assigns lookup results in conditions. In
  `CButeMgr::SetString`, changing one through five of the six lookup sites was byte-flat;
  the sixth moved to a 77.0314% C1 island. An explicit surviving-style `else` was flat on
  the baseline but composed with that dip by returning exactly to the 81.7868% baseline
  island. No intermediate texture or generated deleting-destructor cutoff appeared, so
  neither experimental spelling is retained.
* The first PCX audit stopped at a later runtime decoder and therefore mistook a
  green/blue cursor spelling for the best available source prior. The same public tree's
  overlooked 1996 `libs/dibmgr/dib.cpp::CDib::InitPcx` is the direct same-era ancestor:
  it declares `i`, `j`, remaining count, row, byte, source, destination, and scan buffer
  at function scope; reverse-fills the scan line; and indexes all three planes directly.
  Adapting that body raised `CRezImage::DecodePcxData` from 97.6772 to 99.9240 with the
  exact 399-byte extent, 158 instructions, three calls, eighteen branches, three returns,
  and three ordered relocations. The only clean-source residue was the opening x-bound
  load pair. A 32-cell expression Cartesian was one flat island; target-adjacent C1 forest
  trial 4 then reproduced audited 100.0000 for the unchanged function hash. MAX was banked
  while exact and the generated probe was removed. This corrects the older cursor-order
  interpretation: a higher transcription can be a local maximum that authentic source
  composition escapes.
* The same 1996 file's `CDib::InitPid` invalidated a hash-scoped bounded review of
  `CRezImage::DecodePidData`. The old body had already recovered retail's sequential
  header cursor and exact call/branch/return/relocation counts, then exhausted direct-byte,
  post-increment, payload-alias, final-advance, byte-token, and 33-state controls around
  that transcription. The surviving body supplied the missing composition: eight named
  unsigned header words, a named transparent index, repeated reads of the packed run byte,
  the natural row-transition guard, and a manual literal-run copy loop with the original
  function-scope census. Together they moved 81.4772% directly to 100.0000% exact. A
  same-CFG REGALLOC diagnosis bounds one source state, not the source family; a full
  surviving composition must invalidate and reopen that review through its new hash.
* The adjacent raw-byte `CDib::Init` body proves that the strideless test, buffer size,
  height, row index, and width remain inline accessor calls and that the incoming pixel
  pointer itself advances in the padded-row loop. Applying the literal signed `/ 8`
  spelling first dropped `CRezImage::DecodeBlit` from 95.2917 to 91.2917 by adding four
  sign-correction instructions absent from retail; that is revision/type skew because
  Gruntz's byte-depth domain supplies unsigned lowering. Keeping the retail-proven shift
  and restoring the parameter lifetime raised it to 99.9722 with exact extent, calls,
  CFG, returns, relocations, mnemonics, displacements, stores, and immediates. Swapping
  the two direct multiply operands was flat. `GetBufferSize` alone was also locally flat
  and perturbed three dependent rows; restoring the complete five-accessor family was
  still the authentic base, not a reason to keep the hand expansion. Target-adjacent C1
  forest trial 1 then reached audited 100.0000 for that unchanged function hash; MAX was
  banked and the probe removed. Mine and compose the semantic layer as a family—one
  individually flat helper does not falsify it.
* The same-era `CDib::Invert` body closed `CRezImage::FlipVertical` from 79.7879 to
  100.0000 exact in one composition. The old transcription had been raised from 41.61
  through cached width, a one-past bottom counter, shared decreasing copy index, and three
  hand-shaped loops, then declared bounded after roughly 750 copy/outer/declaration/
  update cells and 65 TU-state probes. The surviving body instead uses three unsigned
  offsets, one shared forward `j`, cached width and height, and three ordinary incrementing
  loops. This falsifies the final "IV reconstructed; remaining colour" doctrine in the
  older folded-local review: the exhaustive search was broad only inside the wrong source
  family.
* Branch-specific derived storage pointer types and direct member assignments in
  `Open`/`MergeArchive` compiled byte-flat in isolation. They remain good structural
  evidence, not credit for a score change.
* The surviving `CBaseHashItem::Prev` while-loop spelling falls from 99.6552 to 99.1379;
  the remaining byte is only a commuted scale-1 SIB encoding. The higher humane Gruntz
  spelling stays until further evidence appears.

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
