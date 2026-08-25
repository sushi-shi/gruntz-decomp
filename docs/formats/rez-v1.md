# Monolith REZ version 1 (`.REZ`, `.VRZ`)

The container Gruntz ships its assets in: `Gruntz.REZ` (21 303 resources),
`GRUNTZ.VRZ` (1 517 voice WAVs) and the demo's `GRUNTDEM.REZ` (10 553). One
format, three files, no variation.

Everything below is derived from two sources only: the archived bytes, and
retail `GRUNTZ.EXE`'s own reader disassembled with `gruntz sema disasm`. The
reconstruction under `src/Rez/` is **not** an authority here. The container
reader is now modeled as `CRezArchive`, `CRezArchiveDir`, `CRezArchiveType`, and
`CRezArchiveEntry` in `RezArchive.cpp`; `RezFile.cpp` separately models the
storage-driver layer (`CRezItm` / `CRezDir` / `CRezFile` wrapping `FILE*`), and
`RezColl.cpp` supplies the hash collection used by the archive model.

Implementation: [`tools/gruntz-rez`](../../tools/gruntz-rez) — reader in
`src/lib.rs`, writer in `src/write.rs`, CLI in `src/bin/{rezls,rezpack}.rs`.

## File shape

```
[0x00, 0xa8)                    header — exactly 168 bytes
[0xa8, next_write_pos)          resource payloads
[next_write_pos, EOF)           directory bodies
root_dir_pos                    the root body, inside that region
```

Retail archives add one wrinkle to the last region — see
[Why we cannot be byte-identical](#why-we-cannot-be-byte-identical).

## Header — 168 bytes, fixed offsets

`CRezArchive::Open` @0x13ad00 issues a single `Read(0, 0, 0xa8, buf)` and then
indexes fixed offsets into `buf`. There is no length field and no scan.

| Offset | Size | Field | Notes |
|---|---|---|---|
| 0x00 | 2 | `"\r\n"` | retail asserts `buf[0x00] == '\r'` @0x13b004 |
| 0x02 | 60 | banner line 1, space-padded | `RezMgr Version 1 Copyright (C) 1995 MONOLITH INC.` |
| 0x3e | 2 | `"\r\n"` | retail asserts `buf[0x3f] == '\n'` @0x13b016 |
| 0x40 | 60 | banner line 2, space-padded | all spaces in every shipped archive |
| 0x7c | 2 | `"\r\n"` | |
| 0x7e | 1 | `0x1a` | retail asserts it @0x13b021; DOS `TYPE` stops here |
| 0x7f | 4 | `version` | must be 1 @0x13b02f |
| 0x83 | 4 | `root_dir_pos` | |
| 0x87 | 4 | `root_dir_size` | |
| 0x8b | 4 | `root_dir_time` | **encoding undetermined**, see below |
| 0x8f | 4 | `next_write_pos` | |
| 0x93 | 4 | `time` | `time_t` |
| 0x97 | 4 | `largest_key_ary` | |
| 0x9b | 4 | `largest_dir_name_size` | |
| 0x9f | 4 | `largest_rez_name_size` | |
| 0xa3 | 4 | `largest_comment_size` | |
| 0xa7 | 1 | `is_data_contiguous` | |

The field order is read straight off the store sequence at 0x13af92–0x13b00a
(each `mov <reg>,[esp+N]` / `mov [mgr+M],<reg>` pair names one field), and every
value it predicts matches the three shipped archives.

`gruntz-rez`'s reader locates the fields by scanning for the `0x1a` instead of
indexing 0x7e. That is a superset of retail's behaviour and agrees on any
archive with the standard 127-byte banner block, which is all of them.

The three shipped headers:

| | `Gruntz.REZ` | `GRUNTZ.VRZ` | `GRUNTDEM.REZ` |
|---|---|---|---|
| `root_dir_pos` | 0x049ac92a | 0x0569a311 | 0x0230c132 |
| `root_dir_size` | 0xf3 | 0x17 | 0x6f |
| `root_dir_time` | 0x0012fd1c | 0x0040c9d8 | 0x0040c9d8 |
| `next_write_pos` | 75 552 552 | 90 691 075 | 35 899 372 |
| `time` | 0x366c4c5c (1998-12-07) | 0x36491d1b (1998-11-10) | 0x364cb929 (1998-11-13) |
| `largest_key_ary` | 0 | 0 | 0 |
| `largest_dir_name_size` | 21 | 18 | 21 |
| `largest_rez_name_size` | 25 | 28 | 23 |
| `largest_comment_size` | 0 | 0 | 0 |
| `is_data_contiguous` | 1 | 1 | 1 |
| file size | 77 253 149 | 90 809 128 | 36 749 729 |

### `next_write_pos` is the end of the payload region

Not a free-space pointer into a hole. In all three archives it equals
`max(pos + size)` over every resource, **exactly**, and the payloads run from
0xa8 to there with no gap. `CRezArchive::Open` @0x13af21 sets it to 0xa8 when
creating a fresh archive, i.e. "allocate the next resource here", which is the
same thing on a file that has never been rewritten.

Everything past it is directory bodies. That is why it is less than
`root_dir_pos` and less than EOF.

### `largest_*` are `max(strlen) + 1` over *present* strings

They are maxima over what was written, not limits — and retail says so itself.
The second `Open` overload @0x13b0c0, which merges an additional archive into an
open manager, reads the newcomer's header and folds each of the four fields in
with a `max`:

```
mov eax,[esp+0xab]      ; the new header's largest_key_ary        (buf+0x97)
cmp eax,[ebx+0x54]      ; ... against the manager's
jbe  +                  ; keep the larger
mov [ebx+0x54],eax
                        ; then the same for buf+0x9b, +0x9f, +0xa3
```

That also confirms, independently of the primary parse, which header offset maps
to which field.

Measured:

* longest directory name in `Gruntz.REZ` is 20 (`largest_dir_name_size` 21);
  in the VRZ 17 (18); in the demo 20 (21).
* longest resource name 24 / 27 / 22 against 25 / 28 / 23.

So the value is the size of the buffer retail allocates for the string, NUL
included. `largest_comment_size` is 0 in all three while every comment is the
empty string, which fixes the other half of the rule: an **absent** string does
not participate. That matches the parser, which nulls an empty comment pointer
at 0x13a83a.

### `root_dir_time` — undetermined

It is not a `time_t`, unlike `time`. Retail `Gruntz.REZ` carries 0x0012fd1c;
`GRUNTZ.VRZ` and `GRUNTDEM.REZ` both carry 0x0040c9d8 despite being built three
days apart. As `time_t` those would be 1970-01-15 and 1970-02-19. They look
much more like a Win32 stack address and an image-base address respectively —
i.e. uninitialised memory read by the packer, identical across runs because the
tool's own layout is. That is a plausible reading, not a proven one, so it is
recorded as **undetermined**; `gruntz-rez` writes 0 and preserves whatever a
source archive had.

Nothing in `GRUNTZ.EXE` consumes it except to pass it to the root `CRezDir`
constructor as that directory's `time` (@0x13b062).

## Directory body

A packed list of variable-length entries, parsed by
`CRezArchiveDir::ReadDirectoryBody`
@0x13a640. It must consume its declared `size` **exactly** — that is the check
that pinned the layout in the first place, before the disassembly confirmed it.

```
u32  kind            1 = directory, 0 = resource
u32  pos             offset of the child body / the file data
u32  size            length of same
u32  time
-- resource only --
u32  id
u32  type            little-endian 4CC: bytes "DIP\0" read as "PID"
u32  num_keys
-- both --
cstr name
-- resource only --
cstr comment
u32  keys[num_keys]
```

Directories carry a name and **no** comment; resources carry both. The key
array trailing the comment is allocated and copied at 0x13a856–0x13a86e. It is
empty in every shipped archive (`largest_key_ary == 0`), so what a key *means*
is undetermined; `gruntz-rez` preserves them verbatim.

The root directory has no entry anywhere — the header points straight at its
body.

Observed conventions (not enforced by the parser):

* Subdirectory entries come **before** resource entries in a body. All 20 mixed
  bodies in `Gruntz.REZ` and all 11 in the demo are `d*r*`; none is `r*d*` and
  none interleaves.
* Nesting is three deep at most (`AREA2\IMAGEZ\TREE2`).
* Empty directories exist: 25 in `Gruntz.REZ`, 11 in the demo.
* Names are uppercase ASCII, and no `(name, type)` collides within a directory
  in any shipped archive.
* `id` is **not** unique: 458 distinct values across 21 303 resources. Nothing
  indexes by it.

## `is_data_contiguous` — what it actually asserts

**It is not an ordering claim about sibling entries, and it drives no search.**

The on-disk sibling order is not lexicographic and not ascending by position:
`rezls Gruntz.REZ grep AREA1` walks 323, 304, 285, 266, … — a stride-19 pattern
that falls straight out of retail's own data structure. Follow the 19:

```
CRezArchive ctor                 @0x13aa10   resource-name bucket count = 19
CRezArchiveDir::FindOrCreateType @0x13a95c   passes it to CRezArchiveType
CRezArchiveType ctor             @0x139c38   constructs m_nameIndex with it
ReadDirectoryBody                @0x13a7e5   inserts each entry into m_nameIndex
```

So each resource type indexes its members in a 19-bucket name hash, and the
packer that wrote these archives enumerated through that hash. Bucket order is
what got written; the stride is the bucket count.

Lookup never cares. `CRezArchiveDir::ReadDirectoryBody` inserts each resource into its
type's name hash (@0x13a7d5), and a lookup hashes the name and walks one bucket
chain (0x13c270 → `CHashBase::Lookup` @0x184b40, then `strcmp`/`stricmp`).
Order-independent.

What the flag does gate is a **bulk preload**:

* While parsing a body, `ReadDirectoryBody` accumulates per directory `min(pos)` and
  `sum(size)` over that directory's own resources (@0x13a8c8–0x13a8e6).
* `CRezArchiveDir::PreloadData` @0x13a0f0 allocates `sum(size)` and reads
  `[min(pos), min(pos) + sum(size))` into it in one call — but only after
  checking `archive->m_isDataContiguous != 0` **and**
  `archive->m_storages.m_storageCount <= 1`. Otherwise
  it prints `CRezDir::Load Failed! (File is not sorted!)` and returns 0.
* Once that block exists, `CRezArchiveEntry::Read` @0x139a40 serves resource bytes from
  `blob + (item.pos - dir.min_pos)` and never touches the file.

That last line is the whole meaning: it is correct only if the directory's
resources **tile that span exactly** — contiguous, no gap, no overlap, in any
order. `m_isDataContiguous = 1` is the writer promising they do.

Measured on the corpus:

| | dirs whose payloads tile exactly | entries ascending by pos | entries lexicographic |
|---|---|---|---|
| `Gruntz.REZ` | **1784 / 1784** | 290 / 1617 | 290 / 1617 |
| `GRUNTZ.VRZ` | **58 / 58** | 0 / 57 | 0 / 57 |
| `GRUNTDEM.REZ` | **915 / 915** | 171 / 842 | 171 / 842 |

The contiguity reading holds universally; the ordering readings do not hold at
all. `Rez::is_contiguous()` and `rezpack check` test the real predicate.

### …and in the shipped game it is inert

`CRezArchiveDir::PreloadData` is **dead code in `GRUNTZ.EXE`**. A byte scan of `.text` for
`E8`/`E9` rel32 targeting 0x13a0f0 finds exactly one hit — its own recursion at
0x13a15b — and the dword `0x0053a0f0` appears nowhere in `.rdata`/`.data`, so
no vtable slot holds it either. `dir->blob` is therefore always null and the
fast path at 0x139a40 is never taken; every resource read goes through the file.

The consequence for a writer: setting `is_data_contiguous = 1` on a non-contiguous
archive would not break *this* game, but it would silently hand wrong bytes to
anything that does call `Load` (the level editor, `RezComp`, a later Monolith
title). `gruntz-rez` lays out contiguously and earns the flag rather than
asserting it.

`m_storages.m_storageCount <= 1` is the second half of the same idea: the archive
increments a counter per opened archive (@0x13ae0c, @0x13aefe, @0x13b13e) and
the second `Open` overload @0x13b0c0 sets `m_isDataContiguous = 0` on its very first
statement, because a merged multi-file view has no single file to slurp a
directory from. The default in the constructor @0x13aa10 is 1, so an archive
created by `CRezArchive` and never merged into claims the flag; that is where the
shipped `1` comes from.

## Why we cannot be byte-identical

`rezpack roundtrip` re-encodes each archive and compares. Every resource and
every directory comes back identical. The images differ in exactly one way:

| | retail | re-encoded | difference |
|---|---|---|---|
| `Gruntz.REZ` | 77 253 149 | 76 402 972 | −850 177 |
| `GRUNTZ.VRZ` | 90 809 128 | 90 750 113 | −59 015 |
| `GRUNTDEM.REZ` | 36 749 729 | 36 324 606 | −425 123 |

Those numbers are not approximate — each is **exactly** the orphaned-directory
slack measured independently in the source file. Retail's directory region is
about twice the size of the directory data it contains:

| | live directory bytes | region size | dead bytes | gaps |
|---|---|---|---|---|
| `Gruntz.REZ` | 850 420 | 1 700 597 | 850 177 | 2099 |
| `GRUNTZ.VRZ` | 59 038 | 118 053 | 59 015 | 60 |
| `GRUNTDEM.REZ` | 425 234 | 850 357 | 425 123 | 1083 |

Dead bytes ≈ live bytes, and the gap count ≈ the directory count. That is the
signature of one incremental rewrite: each directory body was re-emitted at a
fresh offset rather than overwritten in place, orphaning the previous copy of
the same size. `next_write_pos` did not move because no payload changed. A
one-pass writer has nothing to orphan.

`root_dir_pos` differs for the same reason — the root body is written last, and
ours lands earlier. `next_write_pos` is **identical** in all three, because the
payload region is byte-for-byte the same extent.

So the differences are: the free-space slack, `root_dir_pos`, and any field the
caller chooses not to reproduce (`root_dir_time` defaults to 0). Nothing else.

## Verification

```sh
cd tools && cargo test                       # 15 tests, no game files needed
cargo build --release

REZ=/path/to/Gruntz.REZ
./target/release/rezpack check     "$REZ"    # validate + the contiguity predicate
./target/release/rezpack roundtrip "$REZ"    # decode -> encode -> decode -> compare
./target/release/rezpack unpack    "$REZ" /tmp/out
./target/release/rezpack pack      /tmp/out /tmp/rebuilt.rez
```

`unpack` writes `NAME.TYPE` files plus a `REZ.TSV` manifest carrying what a
filesystem cannot: per-resource `id`, `time`, `comment` and keys, per-directory
`time`, and the two header times. `pack` reads it back, including the row order,
so `unpack` → `pack` reproduces the in-memory re-encoding **byte for byte**
(verified on `GRUNTDEM.REZ`).

Results:

| check | result |
|---|---|
| retail `Gruntz.REZ` round-trip | 21 303 resources + 2124 directories identical |
| retail `GRUNTZ.VRZ` round-trip | 1 517 + 60 identical |
| `GRUNTDEM.REZ` round-trip | 10 553 + 1094 identical |
| contiguity predicate | holds on all three |
| `unpack` → `pack` vs in-memory re-encode | byte-identical |

**Not verified by running the game.** The project matches statically; no
archive produced here has been loaded by `GRUNTZ.EXE`. The evidence that one
would load is the reader disassembly above plus the round-trip, not an
observation.

## Appendix: the reader's runtime classes

Not part of the on-disk format — this is the **evidence base** for everything
above. The archive classes below are reconstructed in `include/Rez/RezArchive*.h`
and `src/Rez/RezArchive.cpp`; the storage drivers remain in `RezFile.cpp`.

Confidence is marked per field: **P** proven (a store/load whose meaning is
forced by how the value is produced or consumed), **I** inferred (consistent
with every use, but only one witness), **?** unknown.

Sizes are ground truth from `push <n>; call operator new`:

| class | size | allocated at |
|---|---|---|
| `CRezArchive` | 0x94 | 0x83c66, 0x91bba |
| `CRezArchiveDir` | 0x4c | 0x13ae27, 0x13af1f, 0x13b037, 0x13a73a |
| `CRezArchiveType` | 0x30 | 0x13a961 |
| `CRezArchiveEntry` | 0x3c | pooled 100 at a time, 0x13c133 |
| `CRezItm` (file driver, = `src/Rez/RezFile.cpp`) | 0x24 | 0x13ae90 |
| `CRezDir` (file driver, = `src/Rez/RezFile.cpp`) | 0x38 | 0x13ad9f |

### `CRezArchive` — 0x94

| Off | Meaning | | Evidence |
|---|---|---|---|
| 0x00 | vptr (0x5ef750) | P | ctor @0x13aa10 |
| 0x08 | **`m_isDataContiguous`** | P | header buf+0xa7 `and ecx,0xff` @0x13afe3; tested @0x13a108; forced to 0 by `MergeArchive` @0x13b0f2; ctor default 1 |
| 0x0c | `m_isOpen` | I | set to 1 on both successful Open paths |
| 0x10 | `m_storages` (vptr 0x5ef75c, head/tail at 0x14/0x18) | P | `lea ecx,[archive+0x10]` + AddHead @0x1851e0 per opened driver |
| 0x1c | `m_storages.m_storageCount` | P | ctor 0; incremented per Open @0x13ae0c/@0x13aefe/@0x13b13e; `<= 1` gate in `PreloadData` |
| 0x20 | `m_primaryStorage` | P | `mov [archive+0x20],esi` @0x13adf3; `PreloadData` calls its `Read` slot |
| 0x24 | `m_reserved24` = 1 | ? | ctor only; deliberately left unknown in source |
| 0x28 | `m_nextGeneratedResourceId` = 2 000 000 000 | P | post-incremented for imported filenames without a leading numeric ID @0x13b6e3 |
| 0x2c | `m_maxOpenFiles` = 3 | P | passed as `maxOpen` to the file-driver `CRezDir` ctor |
| 0x30 | `m_rootDirectoryOffset` | P | header buf+0x83 |
| 0x34 | `m_rootDirectorySize` | P | header buf+0x87 |
| 0x38 | `m_rootDirectoryTime` | P | header buf+0x8b; handed to the root directory ctor @0x13b062 |
| 0x3c | `m_nextWritePos` | P | header buf+0x8f; set to 0xa8 when creating @0x13af21 |
| 0x40 | `m_readOnly` | P | Open returns 0 immediately if it is 0; ctor default 1 |
| 0x44 | `m_rootDirectory` | P | `mov [archive+0x44],eax` after the root directory ctor |
| 0x48 | `m_archiveTime` | P | header buf+0x93 |
| 0x4c | `m_isNewArchive` | I | set to 1 only on the create path |
| 0x50 | `m_version` | P | header buf+0x7f, compared to 1 |
| 0x54 | `m_largestKeyArrayLength` | P | header buf+0x97; max-folded @0x13b276 |
| 0x58 | `m_largestDirectoryNameSize` | P | header buf+0x9b; max-folded @0x13b287 |
| 0x5c | `m_largestResourceNameSize` | P | header buf+0x9f; max-folded @0x13b298 |
| 0x60 | `m_largestCommentSize` | P | header buf+0xa3; max-folded @0x13b2a9 |
| 0x64 | `m_archivePath` | P | copied from Open's argument; freed on every failure path |
| 0x68 | `m_caseSensitive` | P | compared with 0 to select case-insensitive lookup @0x13a750 |
| 0x6c | `m_useIdIndex` | P | selects the two-hash vs one-hash `CRezArchiveType` ctor @0x13a95c; **0 by default** |
| 0x70 | `m_resourceNameBucketCount` = **19** | P | constructs `CRezArchiveType::m_nameIndex` |
| 0x74 | `m_resourceIdBucketCount` = 19 | P | constructs `CRezArchiveType::m_idIndex` |
| 0x78 | `m_subdirectoryBucketCount` = 5 | P | constructs `CRezArchiveDir::m_subdirectories` |
| 0x7c | `m_typeBucketCount` = 9 | P | constructs `CRezArchiveDir::m_types` |
| 0x80 | `m_freeEntries` (1 bucket) | I | free-entry pool at 0x13c0c0 |
| 0x88 | `m_entryPoolBlocks` | I | AddHead @0x13c1c8 |
| 0x90 | `m_entriesPerPoolBlock` = 100 | P | allocation size is `m_entriesPerPoolBlock * 0x3c` @0x13c127 |

`m_resourceNameBucketCount` is the field behind the stride-19: the packer walked a 19-bucket
resource-name hash and wrote siblings out in bucket order.

### `CRezArchiveDir` — 0x4c

| Off | Meaning | | Evidence |
|---|---|---|---|
| 0x00 | `m_name` | P | copied from the ctor argument @0x139de0 |
| 0x04 | `m_bodyOffset` | P | ctor argument; used by `ReadDirectoryTree` recursion @0x13a5f0 |
| 0x08 | `m_bodySize` | P | same |
| 0x0c | `m_minDataOffset` | P | init 0xffffffff @0x13a65c, min-folded @0x13a8dc |
| 0x10 | `m_totalDataSize` | P | init 0, accumulated @0x13a8cd |
| 0x14 | `m_time` | P | ctor argument; refreshed on re-parse @0x13a789 |
| 0x18 | `m_archive` | P | every archive access goes through it |
| 0x1c | `m_parent` | I | ctor argument, null for the root |
| 0x20 | `m_nameNode` (vptr 0x5ef748) | P | see the cross-check below |
| 0x34 | `m_nameNode.m_archiveDirectory` | P | back-pointer to `this` |
| 0x38 | `m_subdirectories` | P | constructed with `m_subdirectoryBucketCount`; searched @0x13c3f0 |
| 0x40 | `m_types` | P | constructed with `m_typeBucketCount`; searched @0x13c360 |
| 0x48 | `m_preloadedData` | P | allocated + filled by `PreloadData` @0x13a11e; consumed by `CRezArchiveEntry::Read` @0x139a44 |

### `CRezArchiveType` — 0x30

| Off | Meaning | | Evidence |
|---|---|---|---|
| 0x00 | `m_typeTag` | P | ctor @0x139c49 |
| 0x04 | `m_typeNode` (vptr 0x5ef744) | P | back-pointer at 0x18 |
| 0x1c | `m_idIndex` | P | left empty unless `m_useIdIndex` is set |
| 0x24 | `m_nameIndex` | P | constructed with 19 buckets; `ReadDirectoryBody` inserts every resource here @0x13a7e5 |
| 0x2c | `m_directory` | P | ctor @0x139c4e |

### `CRezArchiveEntry` — 0x3c

| Off | Meaning | | Evidence |
|---|---|---|---|
| 0x00 | `m_name` | P | copied from the Initialize argument @0x139710 |
| 0x04 | `m_type` | P | `GetTypeTag` returns `m_type->m_typeTag` @0x139800 |
| 0x08 | `m_time` | P | parsed from the serialized entry |
| 0x0c | `m_size` | P | `LoadData` allocates it @0x139989 |
| 0x10 | `m_directory` | P | `Read` reaches `m_preloadedData` and `m_minDataOffset` through it |
| 0x14 | `m_dataOffset` | P | `Read` computes `m_preloadedData + (m_dataOffset - m_minDataOffset)` |
| 0x18 | `m_cursor` | P | advanced by successful reads |
| 0x1c | `m_nameNode` | P | inserted into the type's name index @0x13a8a5 |
| 0x30 | `m_nameNode.m_archiveEntry` | P | back-pointer to `this` |
| 0x34 | `m_storage` | P | slow reads dispatch through this storage driver |
| 0x38 | `m_loadedData` | P | allocated by `LoadData` @0x13998f, freed on failure |

### The cross-check that validates the embedded-element offsets

`CHashElement`'s object back-pointer sits at `element + 0x14`
(`ReadDirectoryBody` and `PreloadData` both reach the owner through it). So any ctor that
writes a table pointer at `this+X` **and** `this` at `this+X+0x14` is embedding
an element at X. All three agree:

| class | table written at | `this` written at | delta |
|---|---|---|---|
| `CRezArchiveDir` | 0x20 | 0x34 | 0x14 |
| `CRezArchiveType` | 0x04 | 0x18 | 0x14 |
| `CRezArchiveEntry` | 0x1c | 0x30 | 0x14 |

Three independent classes landing on the same delta is what makes the offset
assignments above safe to build on, rather than one reading of one function.

## Open questions

* `root_dir_time`'s encoding. Not a `time_t`; looks like uninitialised memory.
* What a key array holds, and whether `largest_key_ary` counts elements or
  bytes. No shipped archive has one, so both are unobservable. The writer emits
  the element count.
* Whether banner line 2 was ever used. It is 60 spaces in all three archives.
* Whether `id` means anything. `CRezArchive::m_useIdIndex` gates a second per-type index
  that would plausibly be keyed by it, and that flag is 0 in every code path
  here — so in this game `id` is stored, never indexed, and not unique (458
  distinct values across 21 303 resources). Whether the editor sets the flag is
  outside this binary.
* `CRezArchive::m_reserved24` (1): written by the constructor and never read in
  the module. Offset 0x28 is no longer an open question: it is the generated ID
  counter used when importing files without a leading numeric ID.
