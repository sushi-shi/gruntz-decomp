# Monolith REZ version 1 (`.REZ`, `.VRZ`)

The container Gruntz ships its assets in: `Gruntz.REZ` (21 303 resources),
`GRUNTZ.VRZ` (1 517 voice WAVs) and the demo's `GRUNTDEM.REZ` (10 553). One
format, three files, no variation.

Everything below is derived from two sources only: the archived bytes, and
retail `GRUNTZ.EXE`'s own reader disassembled with `gruntz sema disasm`. The
reconstruction under `src/Rez/` is **not** an authority here — despite the
names, `RezFile.cpp` models the file-driver layer (`CRezItm` / `CRezDir` /
`CRezFile` wrapping `FILE*`), `RezMgr.cpp` holds `CGruntzMgr::PerFrameTick`,
and `RezColl.cpp` is a hash collection. The container reader itself is not
reconstructed; it occupies retail 0x138000–0x13c4cx.

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

`CRezMgr::Open` @0x13ad00 issues a single `Read(0, 0, 0xa8, buf)` and then
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
| 0xa7 | 1 | `is_sorted` | |

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
| `is_sorted` | 1 | 1 | 1 |
| file size | 77 253 149 | 90 809 128 | 36 749 729 |

### `next_write_pos` is the end of the payload region

Not a free-space pointer into a hole. In all three archives it equals
`max(pos + size)` over every resource, **exactly**, and the payloads run from
0xa8 to there with no gap. `CRezMgr::Open` @0x13af21 sets it to 0xa8 when
creating a fresh archive, i.e. "allocate the next resource here", which is the
same thing on a file that has never been rewritten.

Everything past it is directory bodies. That is why it is less than
`root_dir_pos` and less than EOF.

### `largest_*` are `max(strlen) + 1` over *present* strings

They are maxima over what was written, not limits. Measured:

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

A packed list of variable-length entries, parsed by `CRezDir::ReadDirBlock`
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

## `is_sorted` — what it actually asserts

**It is not an ordering claim about sibling entries, and it drives no search.**

The on-disk sibling order is not lexicographic and not ascending by position:
`rezls Gruntz.REZ grep AREA1` walks 323, 304, 285, 266, … — a stride-19 pattern
that falls straight out of retail's own data structure. `CRezMgr`'s constructor
@0x13aa10 sets `m_70 = m_74 = 0x13` (19) — the bucket counts it hands each
resource-name hash — so the packer's own enumeration order was hash-bucket
order, and that is what got written.

Lookup never cares. `CRezDir::ReadDirBlock` inserts each resource into its
type's name hash (@0x13a7d5), and a lookup hashes the name and walks one bucket
chain (0x13c270 → `CHashBase::Lookup` @0x184b40, then `strcmp`/`stricmp`).
Order-independent.

What the flag does gate is a **bulk preload**:

* While parsing a body, `ReadDirBlock` accumulates per directory `min(pos)` and
  `sum(size)` over that directory's own resources (@0x13a8c8–0x13a8e6).
* `CRezDir::Load` @0x13a0f0 mallocs `sum(size)` and reads
  `[min(pos), min(pos) + sum(size))` into it in one call — but only after
  checking `mgr->is_sorted != 0` **and** `mgr->open_file_count <= 1`. Otherwise
  it prints `CRezDir::Load Failed! (File is not sorted!)` and returns 0.
* Once that block exists, `CRezItm::Read` @0x139a40 serves resource bytes from
  `blob + (item.pos - dir.min_pos)` and never touches the file.

That last line is the whole meaning: it is correct only if the directory's
resources **tile that span exactly** — contiguous, no gap, no overlap, in any
order. `is_sorted = 1` is the writer promising they do.

Measured on the corpus:

| | dirs whose payloads tile exactly | entries ascending by pos | entries lexicographic |
|---|---|---|---|
| `Gruntz.REZ` | **1784 / 1784** | 290 / 1617 | 290 / 1617 |
| `GRUNTZ.VRZ` | **58 / 58** | 0 / 57 | 0 / 57 |
| `GRUNTDEM.REZ` | **915 / 915** | 171 / 842 | 171 / 842 |

The contiguity reading holds universally; the ordering readings do not hold at
all. `Rez::is_contiguous()` and `rezpack check` test the real predicate.

### …and in the shipped game it is inert

`CRezDir::Load` is **dead code in `GRUNTZ.EXE`**. A byte scan of `.text` for
`E8`/`E9` rel32 targeting 0x13a0f0 finds exactly one hit — its own recursion at
0x13a15b — and the dword `0x0053a0f0` appears nowhere in `.rdata`/`.data`, so
no vtable slot holds it either. `dir->blob` is therefore always null and the
fast path at 0x139a40 is never taken; every resource read goes through the file.

The consequence for a writer: setting `is_sorted = 1` on a non-contiguous
archive would not break *this* game, but it would silently hand wrong bytes to
anything that does call `Load` (the level editor, `RezComp`, a later Monolith
title). `gruntz-rez` lays out contiguously and earns the flag rather than
asserting it.

`mgr->open_file_count <= 1` is the second half of the same idea: the mgr
increments a counter per opened archive (@0x13ad0c, @0x13aefe) and the second
`Open` overload @0x13b0c0 forces `is_sorted = 0`, because a merged multi-file
view has no single file to slurp from.

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

## Open questions

* `root_dir_time`'s encoding. Not a `time_t`; looks like uninitialised memory.
* What a key array holds, and whether `largest_key_ary` counts elements or
  bytes. No shipped archive has one, so both are unobservable. The writer emits
  the element count.
* Whether banner line 2 was ever used. It is 60 spaces in all three archives.
* Whether `id` means anything. It is stored and never indexed, and it is not
  unique.
