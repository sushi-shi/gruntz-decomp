# Surviving sibling source is a positive-only oracle for typed layers and authored order

tags: cpp:inheritance cpp:inline cpp:member cpp:local cpp:loop | asm:lea asm:mov | topic:source-oracle topic:codegen-idiom topic:evidence-discipline
symptoms: a reconstructed subsystem has correct broad behavior but repeated near-exact
register/schedule walls, generic base pointers where consumers always recover one owner
type, payload unions at a common trailing offset, or hand-expanded one-field stores
confidence: 10/10 (eleven exact closures, exact controls, revision-history audit, and retail
negative controls)
variants: inline-expansion-boundary-pins-a-neighbour.md, ctor-body-first-statement-is-an-inline-member.md

## Evidence

The [public LithTech tree](https://github.com/jsj2008/lithtech) contains `libs/rezmgr`
plus the `libs/lith` hash/list and debug libraries it uses. File headers date the family
to 1995-1997. Its public Git history is also unusually clean for reverse use: the
implementation was imported in commit `845119c`; later commits changed only build files
in RezMgr/Lith. ButeMgr received one 2012 portability patch, so its changed stream-size
types and casts are excluded from the old-MSVC prior.

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

The exact controls are strong: all three typed hash lookups stayed exact, both hash-owning
destructors stayed exact, and `rezarchive` reached 115/115 exact functions.

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
* The later PCX decoder proves a one-byte marker/payload temporary, which is retained as
  a compiler-flat source correction. Hoisting its wider local census dropped
  `CRezImage::DecodePcxData` from 97.6772 to 93.3291 but introduced retail's previously
  absent `push edi` before `mov ebx,this` prologue texture. Two further evidenced
  compositions (channel-pointer scope/order and one-byte reuse) did not recover retail's
  width stack home, so the lower base is recorded as explored and the bank-preserving
  scoped form remains. A real move toward retail is a reason to compose, not permission
  for unbounded churn or a requirement to commit the dip.
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
   feature gained by the dip was absent from the higher baseline; the PCX prologue is a
   positive example whose bounded follow-ups did not yet converge.
6. Run the full build. Shared header declarations are C1/TU-state inputs; preserve correct
   structure and bank MAX rather than deleting a proven layer to recover incidental
   current scores.
