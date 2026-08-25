# WWD — WAP World Data level geometry (Gruntz dialect)

The level format. 54 resources in `Gruntz.REZ`, 9 in `GRUNTDEM.REZ` — **63
files, all parsed here**, plus the retail loader's own disassembly.

There is a **third-party specification** for this format:
[datashenanigans.pl](https://datashenanigans.pl/2020/06/gruntz-wwd-specification/),
transcribed at [`docs/reference/wwd-spec-datashenanigans.md`](../reference/wwd-spec-datashenanigans.md).
`tools/gruntz-codec/src/wwd.rs` and `include/Wwd/WwdFile.h` both carry field
names that match it, so neither is independent evidence — treat the spec as the
**hypothesis** and this document as the **verification**. It describes the
WAP32 / Claw-family format generally; Gruntz ships one dialect, and where they
differ the binary wins.

## Evidence tiers

Every row below is tagged:

| Tag | Means |
|---|---|
| **P** | **Proven** — a retail instruction reads or writes it, cited by RVA |
| **U** | **Proven unread** — retail's readers demonstrably never load it. A definite result, not a gap |
| **I** | **Inferred** — consistent with all 63 files and with one reading, but no instruction forces it |
| **C** | **Contradicted** — the third-party spec says one thing, the binary says another |
| **?** | **Undetermined** |

Corpus statements say "all 63" only where all 63 were actually checked.
Constancy across the corpus is evidence of *invariance*, not of *meaning*: a
field that is 0 in every shipped level tells you the game never exercised it.

Assembly only — `gruntz sema disasm`; the Ghidra decompiler is banned here.

## File shape

```
[0x000, 0x5f4)                  fixed header, 1524 bytes
[0x5f4, EOF)                    main block, optionally raw-deflate/zlib compressed
```

Once inflated, the *image* is `header || main block` and **every offset in both
headers is absolute from byte 0 of that image** — the header is included in the
addressing. `WwdFile_InflateMainBlock` @0x160790 builds exactly that layout:

```
memcpy(dest, src, src->headerSize);                 ; the header, verbatim
uncompress(dest + src->headerSize, &outLen,
           (u8*)src + src->headerSize,              ; compressed bytes start right after it
           src->mainBlockLength);
return outLen == src->mainBlockLength ? dest : 0;
```

Note what `mainBlockLength` is doing there: it is passed as zlib's *source*
length **and** compared against the *decompressed* length. It is the
**uncompressed** size, used as a safe over-bound on the compressed input.

Measured on all 63: `headerSize + mainBlockLength == len(inflated image)`,
`zlib.unused_data` is empty (the compressed stream runs exactly to EOF), and
every file has `flags & 2` set, so **no shipped WWD is stored uncompressed**.
The uncompressed path exists in the reader and is untested by shipped data.

Region order, from the offsets (verified on every file):

```
0x000              file header
0x5f4              plane headers, num_planes x 0xa0, contiguous
                   plane 0 tile grid, then plane 1's, ...
                   image-set name table (one shared NUL-separated run)
                   plane objects, in plane order
tileDescriptionsOffset
                   tile-attribute table, running to EOF
```

## Main header — 1524 bytes (0x5f4)

Read by `CGameLevel::LoadWwd` @0x15d280, `CGameLevel::ReadWwdHeaderName`
@0x160660, `CGameLevel::IsValidWwd` @0x160530 and `WwdFile_InflateMainBlock`
@0x160790.

| Offset | Size | Field | | Evidence |
|---|---|---|---|---|
| 0x000 | 4 | **`headerSize`** | **C** | Spec calls it `signature = 0x5f4`. It is a **size**: `mov eax,[ebx]; cmp eax,0x5f4; jbe` @0x15d29b rejects only values **greater** than `sizeof(WwdHeader)`, and the value is then used in arithmetic — `lea edi,[mainBlockLength+headerSize+0x20]` @0x15d2db sizes the inflate buffer, and 0x160790 uses it as both the `memcpy` length and the offset of the compressed bytes. 0x5f4 in all 63 |
| 0x004 | 4 | **`reserved04`** | **U** | Never loaded by any of the four readers; 0 in all 63 |
| 0x008 | 4 | **`flags`** | **P** | `lea eax,[ebx+8]; mov cl,[eax]; test cl,0x2` @0x15d2ca selects the inflate path; the whole dword is stored to `CGameLevel+0x8`. **3 in all 63** |
| | | bit 0 — *use z coords* | **P** | `CGameLevel::VisitVisible` @0x15dc90 gates the **z-interleaved** draw on it: planes are drawn in order and, before each, every `CGameObject` whose `m_sortKey <` that plane's `m_zBound` (the plane header's `zCoord`) is rendered. Matches the spec |
| | | bit 1 — *compressed* | **P** | as above. Matches the spec |
| 0x00c | 4 | **`reserved0c`** | **U** | never loaded; 0 in all 63 |
| 0x010 | 0x40 | **`levelName`** | **P** | `lea edi,[edx+0x10]` + `repnz scas`/`rep movs` @0x15d328 copies it to `CGameLevel::m_levelName`; also the sole output of `ReadWwdHeaderName` @0x160660. `WwdFile::ValidateMainBlock` @0x3b470 runs `atoi` over its first digit run to get the **level number**. Sixteen distinct values, e.g. `"Gruntz - Level Set 13"` |
| 0x050 | 0x40 | **`author`** | **P** | `FillLevelInfoDialog` @0x3b1a0 and `CustomWorldInfoDlgProc` @0x3b600 both `SetDlgItemTextA(hDlg, 0x428, header + 0x50)`. `"Monolith Productions Inc."` in all 63 |
| 0x090 | 0x40 | **`created`** | **P** | same two functions, control 0x429, `header + 0x90`. Fourteen distinct values, all of the form `"December 7, 1998"` |
| 0x0d0 | 0x100 | **`rezFile`** | **U** | never read. `"C:\PROJ\GRUNTZ\GRUNTZ.REZ"` in all 63 |
| 0x1d0 | 0x80 | **`tileDirectory`** | **U** | never read — see [Image-set registry keys](#image-set-registry-keys). `"\AREA<n>\TILEZ"`, 8 distinct |
| 0x250 | 0x80 | **`palette`** | **U** | never read; **empty string in all 63** |
| 0x2d0 | 4 | **`startX`** | **P** | `mov edx,[edx+0x2d0]; fild` @0x15d43b — the initial scroll origin, multiplied by the main plane's parallax scale unless that plane has flag bit 0 |
| 0x2d4 | 4 | **`startY`** | **P** | same instruction pair. Also the only field of `CGameLevel::m_header` any other code reads (`CPlay::ResetGoals`, `Play.cpp:4617`) |
| 0x2d8 | 4 | **`reserved2d8`** | **U** | never loaded; 0 in all 63 |
| 0x2dc | 4 | **`numPlanes`** | **P** | `cmp edi,[ecx+0x2dc]` @0x15d39a bounds the plane loop. **1 (45 files) or 2 (18 files)** |
| 0x2e0 | 4 | **`planesOffset`** | **P** | `mov esi,[edx+0x2e0]; add esi,ebx` @0x15d368. **0x5f4 in all 63** — the plane headers immediately follow |
| 0x2e4 | 4 | **`tileDescriptionsOffset`** | **P** | `mov eax,[edx+0x2e4]` @0x15d3a4; zero means "no attribute table" and skips the block |
| 0x2e8 | 4 | **`mainBlockLength`** | **P** | `mov ecx,[ebx+0x2e8]` @0x15d2d3 sizes the inflate buffer; 0x160790 uses it as the zlib source bound and asserts the inflated size equals it |
| 0x2ec | 4 | **`checksum`** | **P** *(stored)* | `mov eax,[edx+0x2ec]; mov [ebp+0xac],eax` @0x15d362. **Retail never verifies it** — it is stored, then returned by `CGruntzMgr::ResolveLevelChecksum` @0x93d40 as the level's multiplayer identity token. Algorithm: [below](#checksum) |
| 0x2f0 | 4 | **`reserved2f0`** | **U** | never loaded; 0 in all 63 |
| 0x2f4 | 0x80 | **`launchApp`** | **U** | never read. Three distinct, incl. `"C:\PROJ\GRUNTZ\DEBUG\GRUNTZ.EXE"` |
| 0x374 | 0x80 | **`imageDirectory[0]`** | **U** | never read. `"AREA<n>\IMAGEZ"`, 8 distinct |
| 0x3f4 | 0x80 | **`imageDirectory[1]`** | **U** | never read. `"GAME\IMAGEZ"` in all 63 |
| 0x474 | 0x80 | **`imageDirectory[2]`** | **U** | never read; empty in all 63 |
| 0x4f4 | 0x80 | **`imageDirectory[3]`** | **U** | never read; empty in all 63 |
| 0x574 | 0x20 | **`imagePrefix[0]`** | **U** | never read. `"LEVEL"` in all 63 |
| 0x594 | 0x20 | **`imagePrefix[1]`** | **U** | never read. `"GAME"` in all 63 |
| 0x5b4 | 0x20 | **`imagePrefix[2]`** | **U** | never read; empty in all 63 |
| 0x5d4 | 0x20 | **`imagePrefix[3]`** | **U** | never read; empty in all 63 |

Each string region is NUL-terminated with **clean zero padding to the end of
its slot** in all 63 files — checked byte by byte, which is what fixes the
region boundaries independently of the disassembly. The `author` and `created`
splits at 0x50 and 0x90 are additionally forced by the two dialog procs, which
index `header + 0x40` and `header + 0x80` off the level-name pointer.

`LoadWwd` `rep movs` the whole 0x5f4 into `CGameLevel::m_header` (+0xe0)
regardless. So an unreconstructed consumer of any **U** row could in principle
exist; none is known, and the reconstruction reads only `startX`/`startY`
back out of that copy.

## Plane header — 160 bytes (0xa0)

`numPlanes` of these, contiguous from `planesOffset`. Consumed by
`CDDrawWorkerHost::Read` @0x161640, which is the single authority for this table
— the read set below is the complete list of offsets that function touches.

| Offset | Size | Field | | Evidence |
|---|---|---|---|---|
| 0x00 | 4 | **`headerSize`** | **P** | `mov eax,[edx]; cmp eax,0xa0; je` @0x16164f — exact match required, unlike the file header's `<=`. 0xa0 in all 81 planes |
| 0x04 | 4 | **`reserved04`** | **U** | never loaded; 0 in all 81 |
| 0x08 | 4 | **`flags`** | **P** | `mov eax,[edx+0x8]` @0x161721, re-read at 0x161864. Observed: **1** (63 planes) and **0xc** (18) |
| | | bit 0 — *main plane* | **P** | `CGameLevel::ReadPlane` @0x15d8d0 sets `m_mainPlane`/`m_mainIndex` on it; and `test al,0x1` @0x161989 suppresses the parallax multiply for the scroll origin. Every file has exactly one |
| | | bit 1 — *no draw* | **P** | `CDDrawWorkerHost::Draw` @0x162010 returns immediately. Never set in shipped data |
| | | bit 2 — *wrap X* | **P** | `CDDrawWorkerHost::WrapCoord` @0xa000 `m_flags & 0x4`; also `ActivateVisibleObjects` @0x163300 |
| | | bit 3 — *wrap Y* | **P** | same, `& 0x8` |
| | | bit 4 — *auto tile size* | **P** | `test al,0x10` @0x1617ad — take the tile size from the first frame of image set 0 instead of 0x58/0x5c. Never set in shipped data |
| 0x0c | 4 | **`reserved0c`** | **U** | never loaded; 0 in all 81 |
| 0x10 | 0x40 | **`name`** | **P** | `lea edi,[edx+0x10]` + `rep movs` @0x16183a → `m_name`. Two distinct: `"Action"`, `"Back"` |
| 0x50 | 4 | **`pixelWidth`** | **U** | **never loaded.** Redundant: equals `tilesWide * tilePixelWidth` in all 81 planes |
| 0x54 | 4 | **`pixelHeight`** | **U** | **never loaded.** Equals `tilesHigh * tilePixelHeight` in all 81 planes |
| 0x58 | 4 | **`tilePixelWidth`** | **P** | `mov ecx,[edx+0x58]` @0x16175e → `m_tilePxW`; `m_wrapW = m_tilePxW * m_gridW`. **32 in all 81** |
| 0x5c | 4 | **`tilePixelHeight`** | **P** | `mov ecx,[edx+0x5c]` @0x161764. **32 in all 81** |
| 0x60 | 4 | **`tilesWide`** | **P** | `mov ecx,[edx+0x60]` @0x161752 → `m_gridW`, the tile-grid row stride. 25–90 |
| 0x64 | 4 | **`tilesHigh`** | **P** | `mov ecx,[edx+0x64]` @0x161758 → `m_gridH`, the row count. 25–80 |
| 0x68 | 4 | **`scrollX`** | **P** | `mov ecx,[esi+0x68]; fild` @0x161973 — the plane's own scroll origin. **0 in all 81** |
| 0x6c | 4 | **`scrollY`** | **P** | `mov eax,[esi+0x6c]` @0x161970. **0 in all 81** |
| 0x70 | 4 | **`movementXPercent`** | **P** | `mov ecx,[edx+0x70]` @0x16172e; `m_scaleX = value * 0.01f` (the `fmul ds:0x5f02a0` @0x1618e0), the parallax factor. **100 in all 81** |
| 0x74 | 4 | **`movementYPercent`** | **P** | `mov eax,[edx+0x74]` @0x161737. **100 in all 81** |
| 0x78 | 4 | **`fillColor`** | **P** | `mov ecx,[edx+0x78]` @0x16185b → `m_bltFx.dwFillColor`, used by the `TILE_FILL` blit in `Draw`. **0x80 in all 81** — and no cell in any shipped level is `TILE_FILL`, so the value is never exercised |
| 0x7c | 4 | **`imageSetsCount`** | **P** | `mov eax,[edx+0x7c]` @0x161675 bounds the name-table walk. **1 in all 81** |
| 0x80 | 4 | **`objectsCount`** | **P** | `mov edx,[esi+0x80]` @0x1619ae — the object-record count. 0–1281 |
| 0x84 | 4 | **`tilesOffset`** | **P** | `mov ecx,[esi+0x84]` @0x161919 — start of the u32 grid. Always `planesOffset + numPlanes*0xa0` for plane 0 |
| 0x88 | 4 | **`imageSetsOffset`** | **P** | `mov edi,[edx+0x88]` @0x161668 — start of this plane's slice of the shared name table |
| 0x8c | 4 | **`objectsOffset`** | **P** | `mov eax,[esi+0x8c]; test eax,eax; je` @0x1619a4 — zero means "no objects" |
| 0x90 | 4 | **`zCoord`** | **P** | `mov ecx,[edx+0x90]` @0x16176a → `m_zBound`, the render-order threshold `VisitVisible` compares `CGameObject::m_sortKey` against. **0 in all 81** |
| 0x94..0x9f | 12 | **`reserved94[3]`** | **U** | never loaded; 0 in all 81 |

Naming note: the third-party spec calls 0x58/0x5C `tiles_width`/`tiles_height`
and 0x60/0x64 `tiles_wide`/`tiles_high`, which is easy to read backwards.
`include/Wwd/WwdFile.h`'s `tilePixelWidth` / `tilesWide` is unambiguous and
matches retail's use; prefer it.

## Tile grid

`tilesHigh * tilesWide` little-endian u32s at `tilesOffset`, row-major with
`tilesWide` per row. `CDDrawWorkerHost::Read` copies them verbatim into
`m_tileGrid` and builds `m_rowOffsets[r] = r * tilesWide`.

A handle splits into two 16-bit halves, and retail's own diagnostics name them.
`CDDrawWorkerHost::ValidateTiles` @0x163510 formats
`"Plane %s: Bad map image set value (%i) at %i,%i"` for `handle >> 16` and
`"Plane %s: Bad map tile value (%i) at %i,%i"` for `handle & 0xffff` — **P**.

| Handle | Meaning | | |
|---|---|---|---|
| `0xffffffff` | draw nothing | **P** | `Draw` @0x162010 skips the cell. 8585 of 261129 cells |
| `0xeeeeeeee` | colour-fill the cell with `fillColor` | **P** | `Draw` issues `BltEx(&dst, 0, 0, 0x1000400, &m_bltFx)`. **Never occurs in shipped data** |
| otherwise | high 16 = image-set index into the plane's set table; low 16 = frame index within it, and simultaneously the index into the tile-attribute table | **P** | `Draw` / `ValidateTiles`; `CGameLevel::AxisProbe` @0x161270 uses `m_imageSets[handle & 0xffff]` |

**The high word is 0 in all 261 129 cells of all 63 files.** Consistent with
`imageSetsCount == 1` everywhere: shipped Gruntz levels use exactly one image
set per plane. The multi-set machinery is implemented and unexercised.

Low words run 0–464 against 910 attribute entries, 338 distinct.

## Image-set name table

One shared run of NUL-terminated ASCII names sitting between the last tile grid
and the first object block. Each plane's `imageSetsOffset` points at *its* first
name, so a two-plane file overlaps: plane 0 sees `"BACK\0ACTION\0…"` and plane 1
`"ACTION\0…"`.

The walk is a character-class scanner, not a NUL split —
`CDDrawWorkerHost::Read` @0x161688 skips bytes outside `['0', 0x80)` and then
takes the run inside it, `imageSetsCount` times. Every shipped level's table is
`"ACTION"` (single-plane) or `"BACK\0ACTION"`.

### Image-set registry keys

The names are keys into `CDDrawWorkerRegistry::m_workersByName`, and the keys are built
by `CDDrawWorkerRegistry::InstallTree` @0x154f80: it walks a `CSymTab`
directory tree and joins each level with a separator onto a root prefix. The
three roots are **hardcoded in `CPlay`**, not read from the WWD header:

| Retail call | Tree | Prefix | Key shape |
|---|---|---|---|
| `CPlay::LoadActionTileSprites` @0xdb600 | `<level bank>\TILEZ` | *(empty)* | `ROCKZ_EDGE` |
| `CPlay::LoadLevelImages` @0xdb7e0 | `<level bank>\IMAGEZ` | `LEVEL` | `LEVEL_WATER` |
| `CPlay::LoadGameImages` @0xdb8a0 | `<game bank>\IMAGEZ` | `GAME` | `GAME_CURSORZ` |

That is exactly the `(directory, prefix)` triple the header carries at
0x1d0 / 0x374 / 0x3f4 and 0x574 / 0x594 — the editor recorded what it used, the
game re-derives the same thing from the resource path. Which is why those
header fields can be **U** and the game still resolves every name: the two
agree by construction, and only one of them is load-bearing.

## Tile-attribute table

At `tileDescriptionsOffset`, running to EOF. Header then a packed array of
variable-stride records, indexed by a tile handle's low word.

| Offset | Field | | Evidence |
|---|---|---|---|
| 0x00 | — | **U** | `LoadWwd` hardcodes `lea ebx,[eax+0x20]` for the first record; it never reads this. 0x20 in all 63 |
| 0x04 | — | **U** | never read; 0 in all 63 |
| 0x08 | **`count`** | **P** | `mov ecx,[eax+0x8]` @0x15d3cf bounds the loop. **910 in all 63** |
| 0x0c..0x1f | — | **U** | never read; 0 in all 63 |

Records are dispatched on their first dword by `CGameLevel::ReadImageSet`
@0x15d820, and each subclass's `GetStride()` advances the cursor. The
spec's `tile_type` 1/2/3 is right as a discriminator; the payloads name it
better:

| Tag | Class | Stride | Payload after `+0x08 width`, `+0x0c height` | |
|---|---|---|---|---|
| 1 | `CImageSet1` — **uniform** | 0x14 | `+0x10` one attribute for the whole tile | **P** `Parse` @0x166d40, `GetStride` @0x161410, `GetCollisionAt` @0x161380 |
| 2 | `CImageSet2` — **rect** | 0x28 | `+0x10` outside, `+0x14` inside, `+0x18..0x24` l/t/r/b | **P** `Parse` @0x166990, `GetStride` @0x1614a0, `GetCollisionAt` @0x161470 |
| 3 | `CImageSet3` — **pixel map** | `0x10 + w*h` | `+0x10` one attribute byte per pixel; `Parse` requires `1 << log2(height) == width` | **P** `Parse` @0x166d70, `GetStride` @0x161590, `GetCollisionAt` @0x161570 |

Record `+0x04` is skipped by all three `Parse` bodies (`m_fields` starts at
`+0x08`) — **U**; 0 in all 57 330 shipped records.

**Every shipped record is tag 1, 32x32.** 63 files x 910 records, no
exceptions: the rect and pixel-map encodings are implemented and unused. The
cursor lands exactly on EOF in all 63, which is what validates the strides.

### The attribute value domain

63 files x 910 = 57 330 values. Observed set:

```
0 1 2 3 4
0x0a..0x13   0x1e..0x24   0x33..0x42   0x5d..0x74   0x96..0x9a
```

plus one outlier: **301 (0x12d) at index 301 of `AREA1\WORLDZ\LEVEL2`**, the
only value outside the domain in the whole corpus and equal to its own index.
Treat it as an authoring slip, not a member of the domain.

Values 0x0a and up are modelled in `include/Gruntz/TileCollisionKind.h`, and the
corpus corroborates it independently: the community editor reference
(`docs/reference/gooroosgruntz/editor/TileAttributez.html`) says tiles **#257 and
#258 are "Water bridge up & down"**, and attribute-table indices 257 and 258
carry **0x6b and 0x6c**, which that header already names
`TILEKIND_WATERBRIDGE_DOWN` / `TILEKIND_WATERBRIDGE_UP`. Two independent
sources, same two tiles.

The **0..4 band** was a naming conflict; it is now resolved in favour of the
editor's vocabulary, because retail's own behaviour matches it arm for arm.
Run-length mapping index -> attribute over the shipped tables, against GooRoo's
examples and against what the movement code actually does with each value:

| Value | Editor / spec name | Corpus indices | Retail behaviour | `TileCollisionKind.h` |
|---|---|---|---|---|
| 0 | Clear (`#1..#36`) | 0..38, and most of 330..900 | passable | `TILEKIND_PASSABLE` |
| 1 | **Solid** (`#39..#76`) | **39..76** | blocks on *every* axis: `StepAxisLo`/`StepAxisHi` (horizontal), `ResolveCeilingCollision` (up), `ResolveFloorCollision`/`FreeMove` (down) | `TILEKIND_SOLID` |
| 2 | Ground ("no examples") | 69, 74, 180, 181 | tested only on the **floor** paths, and `MoveStepXHi` rewrites it to 0 when the mover has flag 0x400 (`cmp eax,0x2; test ch,0x4; xor eax,eax`) — a one-way platform | `TILEKIND_GROUND` |
| 3 | Climb ("no examples") | 162, 167 | blocks like 2 **except** when the mover is already `MOVE_CLIMBING` (`m_moveMode != MOVE_CLIMBING && result == 3`; retail `cmp eax,0x3` twice in `MoveGrounded` @0x15e130) | `TILEKIND_CLIMB` |
| 4 | **Death** (`#99..#140`) | **102..139** | walked onto, not backed off: `ResolveFloorCollision` sets `m_flags 0x400000`, the cell classifier gives it bit 0x2 (1 gets 0x1), and `CGrunt`/`CRollingBall` handle it in the same switch arm as `TILEKIND_DEATHBRIDGE_UP` | `TILEKIND_DEATH` |
| >=5 | "User" | the rest | the Gruntz-specific band | the 0x0a..0x9a names |

The index ranges line up with GooRoo's tile numbers on two independent bands, and
the `MOVE_CLIMBING` guard on value 3 is a third, independent confirmation — the
editor's "Climb" is exactly a tile that is solid to walkers and transparent to
climbers. Values 2 and 3 being near-absent from the corpus is what GooRoo's "there
don't appear to be any of these" records; it is *not* evidence that retail ignores
them, and an earlier draft of this table wrongly said value 3 had no consumer in
`src/`. It has about thirty, in `GameLevel.cpp` alone.

## Object records

`objectsCount` records at `objectsOffset`, each a **0x11c-byte fixed part
followed by four packed strings**. Scattered into a fresh `CWwdGameObjectA` by
`CDDrawWorkerHost::ReadPlaneObjects` @0x162af0, which returns
`0x11c + <string bytes consumed>` as the stride — **P**.

The fixed part is read as a linear `i32` cursor, so the scatter *is* the field
map. Selected rows (offsets proven by the store sequence at 0x162e9c onward):

| Record | Field | Lands at | |
|---|---|---|---|
| 0x00 | object id | `CWwdGameObjectA` ctor arg | **P** |
| 0x04..0x10 | byte lengths of the four strings: name, logic, image set, animation | consumed in order from `record + 0x11c` | **P** |
| 0x14 / 0x18 / 0x1c | x / y / z | `Setup(x, y, z, template)`; out-of-range x/y drops the object | **P** |
| 0x20 | grid index | selects `ApplyLookupSprite` vs `ApplyName`; -1 = no index | **P** |
| 0x24 | `flags_add` | **nothing** | **U** |
| 0x28 | `flags_dynamic` | OR-ed into `CGameObject::m_flags` | **P** |
| 0x2c | `flags_draw` | `CGameObject::m_stateFlags` | **P** |
| 0x30 | `flags_user` | `CLogicRecord::m_userFlags` | **P** |
| 0x34..0x48 | score, points, powerup, damage, smarts, health | **`CGameObject` +0x114..+0x128** | **P** |
| 0x4c..0x8b | extent, area, switch, clip rects | `m_extent`/`m_area`/`m_switchRect`/`m_clip`; an all-zero l/r pair becomes `COORD_UNSET` | **P** |
| 0x10c | `object_type` | `CGameObject::m_objectType` | **P** |
| 0x110 | hit-type mask | `m_hitTypeFlags` | **P** |
| 0x114 / 0x118 | `move_res_x` / `move_res_y` | `m_strideX` / `m_strideY`, **only if > 0** | **P** |

### The four strings

All four are read into a 0x400 stack buffer and handed to a `CString`; the walk
of all 54 retail files is in
[`game-data-strings.md`](game-data-strings.md#3-the-wwd-object-corpus)
(27 110 records) and is produced by `the WWD object walker (retired).

| # | Field | Consumed as | | |
|---|---|---|---|---|
| 0 | **`name`** | `obj->m_name` — `lea ecx,[ebx+0xdc]` @0x162f05 + `CString::operator=` | **P** | Present on **14 of 27 110** objects, all `WEENIE_SWITCH` in `AREA2\WORLDZ\LEVEL7`. The only reader of `m_name` in the tree is the release-dead `TRACE` in `CDDrawChildGroup::Deserialize` @0x15b0e0, so it is a designer annotation the shipped build never looks at |
| 1 | **`logic`** | `m_workerCache->m_workers.Lookup(logic)`; a miss **drops the object** | **P** | 34 distinct, every one registered by `RegisterGameObjectTypes` @0xa3b0 |
| 2 | **`image_set`** | `ApplyLookupSprite(s, gridIndex)` when `gridIndex != -1`, else `ApplyName(s)` | **P** | 186 distinct; all 27 110 references resolve to a real `<NS>\IMAGEZ` path |
| 3 | **`animation`** | `ApplyLookupGeometry(s, 0)` **and** `SetSoundCueByName(s)` | **P** | 29 distinct. Resolves in **two** registries — 2 945 references name an `<NS>\ANIZ` resource, 211 an `<NS>\SOUNDZ\AMBIENT` WAV (the `GlobalAmbientSound` objects). One reference in the corpus dangles |

The third-party spec calls field 3 "animation" and earlier revisions of this
document called it "sound". Both are half right: it is a **registry key that may
name either**, and which registry answers depends on the logic.

### `flags_add` is proven unread

Retail's cursor steps over record `+0x24` without loading it. In the target
disassembly the tell is two increments against one load, at 0x162f10:

```
162f10:  8b 45 04     mov eax,DWORD PTR [ebp+0x4]  ; record +0x28, cursor still at +0x24
162f13:  8b 53 08     mov edx,DWORD PTR [ebx+0x8]
162f16:  83 c5 04     add ebp,0x4                  ; skip +0x24 -- no load
162f19:  0b d0        or  edx,eax
162f1b:  83 c5 04     add ebp,0x4                  ; consume +0x28
162f1e:  89 53 08     mov DWORD PTR [ebx+0x8],edx  ; obj->m_flags |= flags_dynamic
```

Every other field is `mov e?x,[ebp+0x0]` / `add ebp,0x4` / store, e.g. the six
user values land with `mov [ebx+0x114],eax` at 0x162f39 through
`mov [ebx+0x128],edx` at 0x162f75.

So none of the spec's six `flags_add` bits (difficult / eye candy / high detail
/ multiplayer / extra memory / fast cpu) can affect Gruntz. That agrees with
GooRoo's `ObjectFlags.html`, which marks every Add Flag "Not used for the game
of Gruntz".

### Resolving "0x34..0x48" against CLAUDE.md's "+0x114 union"

Not a contradiction — **two structures, one scatter between them**. The
on-disk record holds score/points/powerup/damage/smarts/health at **0x34..0x48**;
the runtime `CGameObject` holds the same six at **+0x114..+0x128**. The six
`mov [ebx+0x114+4n]` stores above are the map. `docs/domain/README.md` describes
the runtime side (and each `CUserLogic` leaf's reinterpretation of those slots);
this document describes the file side. Both statements stand.

## Checksum

`CGruntzMgr::ResolveLevelChecksum` @0x93d40 reads the field and returns it as a
level identity token; nothing in `GRUNTZ.EXE` recomputes it. So the algorithm is
recoverable only from the corpus, and this is a corpus result — **I**, not P.

The spec's formula, applied as written to the *decompressed* main block,
matches **0 of 63** files. The operand is the **compressed** block, and one
index is skipped:

```
n = fileSize - headerSize          # the compressed bytes, exactly as stored
checksum = -n
for i in 1 .. n-1:                 # note: i starts at 1
    checksum += block[i] - i       # 32-bit wraparound
```

**52 of 63 exact.** The remaining 11 are off by a small negative amount, all
within one byte's worth:

| delta | files |
|---|---|
| -255 | `GAME\{BATTLEZ,MULTI}\FLYING FIASCOZ` |
| -198 | `AREA6\WORLDZ\LEVEL23` |
| -191 | `GAME\{BATTLEZ,MULTI}\RUMBLE IN THE ROCKZ` |
| -157 | `GAME\BATTLEZ\TRAINING` |
| -27 | `AREA3\WORLDZ\LEVEL12` |
| -20 | `AREA2\WORLDZ\LEVEL6` |
| -1 | `AREA4\WORLDZ\LEVEL13`, `AREA8\WORLDZ\LEVEL29`, `AREA8\WORLDZ\LEVEL32` |

Every delta has magnitude <= 255, which is what a single differing byte between
the checksummed buffer and the archived one would produce. No further structure
was found in them.

**Two readings of the skipped index are indistinguishable on this corpus.**
Summing from `i = 0` and subtracting a constant 120 gives identical results for
all 63 files, because `block[0]` is `0x78` in every one of them — that is the
zlib CMF byte (deflate, 32K window), followed by `0x9c`. So "the loop starts at
1" and "the accumulator starts at `-n - 120`" cannot be told apart without a
WWD whose compressed stream begins differently, and none exists. Both are
recorded; neither is asserted.

## Undetermined

* The 11 checksum outliers. One-byte-scale, no pattern found.
* Whether the checksum loop skips index 0 or subtracts a constant (above).
* Whether main-header `flags` has bits beyond 0 and 1. Only `0x3` was ever
  written, and only bit 1 is tested at load; bit 0's meaning comes from
  `CGameLevel::m_flags`, which is the same dword.
* What plane-header 0x04 / 0x0c and file-header 0x004 / 0x00c / 0x2d8 / 0x2f0
  were *for*. Proven unread and zero everywhere; the editor may not write them
  either.
* Tile attribute 3. Present in the shipped tables (indices 162, 167), named
  "Climb" by the spec and by the editor reference, and with no reader.
* The `unknown1..7` the spec lists. Every offset in the 1524-byte and
  160-byte headers is accounted for above as named / proven-unread, so if a
  Gruntz-specific meaning hides in them, it is in a slot that is zero in all 63
  shipped files and never loaded — i.e. unobservable from this binary and this
  corpus.

## Reproducing

The 63 resources come out of the two archives with `rezls`:

```sh
cd tools && cargo build --release
./target/release/rezls "$REZ" grep WWD                 # 54 in Gruntz.REZ, 9 in GRUNTDEM.REZ
./target/release/rezls "$REZ" extract 'AREA1\WORLDZ\LEVEL1' /tmp/level1.wwd
```

`tools/gruntz-codec/src/wwd.rs` parses them (`split` walks every plane and name
table, so a bad idea about the layout is an error rather than silence), and
`tools/gruntz-oracle/src/map.rs` renders them to PNG. Neither has been run
against the game: this project matches statically and nothing here was produced
by executing `GRUNTZ.EXE`.
