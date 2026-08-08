# WWD specification — datashenanigans.pl (third-party, transcribed)

**Source:** <https://datashenanigans.pl/2020/06/gruntz-wwd-specification/>
**Provenance:** transcribed 2026-08-08 from the page contents. Not fetched by
this lane (no network); the field list below was relayed verbatim and is
recorded here so the claims are auditable rather than absorbed silently.

This is a **hypothesis document**, not evidence. It describes the WAP32 /
Claw-family WWD format *in general*; Gruntz ships one dialect of it. Where the
two disagree, `GRUNTZ.EXE`'s own bytes win.

The verification pass — every field checked against retail disassembly and the
63 shipped WWD resources, tiered proven / inferred / contradicted / unread —
is [`docs/formats/wwd-v1.md`](../formats/wwd-v1.md). Read that, not this.

`tools/gruntz-codec/src/wwd.rs`'s struct field names (`movement_x_percent`,
`z_coord`, `fill_color`, `num_image_sets`, `offset_tiles`, …) match this page
almost exactly, so they are best understood as **transcribed from here**, not
independently recovered. `include/Wwd/WwdFile.h`'s `WwdPlaneHeader` matches it
too, with two additions this page does not list (`objectsCount` 0x80,
`objectsOffset` 0x8C — which the page *does* list as `num_objects` /
`offset_objects`, so the agreement is total).

## What the page claims

### Main header — 1524 bytes

| Offset | Page's name | Page's note |
|---|---|---|
| 0x00 | `signature` | `0x000005F4` |
| 0x08 | `flags` | 0x1 = use z coords, 0x2 = compress |
| 0x2DC | `num_planes` | |
| 0x2E0 | `offset_planes` | |
| 0x2E4 | `offset_tile_properties` | |
| 0x2E8 | `decompressed_mainblock_size` | |
| 0x2EC | `checksum` | |
| 0x374 / 0x3F4 / 0x474 / 0x4F4 | `image_set1..4` | |
| 0x574 / 0x594 / 0x5B4 / 0x5D4 | `prefix1..4` | |

Plus `unknown1..7` at the offsets the page does not name.

### Plane header — 160 bytes

| Offset | Page's name |
|---|---|
| 0x00 | `block_size` = 0xA0 |
| 0x08 | `flags` — 0x01 main, 0x02 no draw, 0x04 x wrap, 0x08 y wrap, 0x10 auto tile size |
| 0x50 | `width_px` |
| 0x54 | `height_px` |
| 0x58 | `tiles_width` |
| 0x5C | `tiles_height` |
| 0x60 | `tiles_wide` |
| 0x64 | `tiles_high` |
| 0x70 / 0x74 | `movement_x_percent` / `movement_y_percent` |
| 0x78 | `fill_color` |
| 0x7C | `num_image_sets` |
| 0x80 | `num_objects` |
| 0x84 | `offset_tiles` |
| 0x88 | `offset_image_sets` |
| 0x8C | `offset_objects` |
| 0x90 | `z_coord` |

### Object record

`object_type` 0x10C — 0x001 generic, 0x002 player, 0x004 enemy, 0x008 powerup,
0x010 shot, 0x020 p-shot, 0x040 e-shot, 0x080 special, 0x100 / 0x200 / 0x300 /
0x400 user.
`flags_draw` 0x2C — 0x1 no draw, 0x2 mirror, 0x4 invert, 0x8 flash.
`flags_dynamic` 0x28 — 0x1 no hit, 0x2 always active, 0x4 safe, 0x8 auto hit damage.
`flags_add` 0x24 — 0x01 difficult, 0x02 eye candy, 0x04 high detail, 0x08
multiplayer, 0x10 extra memory, 0x20 fast cpu.
`score` / `points` / `powerup` / `damage` / `smarts` / `health` at 0x34..0x48.
`move_res_x` at 0x114.

### Tiles and misc

* Tiles are u32 ids; `0xFFFFFFFF` = invisible, `0xEEEEEEEE` = filled.
* Main block optionally deflate-compressed.
* Tile attributes — 0x00 clear, 0x01 solid, 0x02 ground, 0x03 climb, 0x04 death.
* `tile_type` — 1 single, 2 double, 3 mask.
* Checksum: `checksum = -mainBlockSize; for each byte[offset]: checksum += byte[offset] - offset`.

## Verification summary

Full evidence in [`docs/formats/wwd-v1.md`](../formats/wwd-v1.md). Headline
results:

| Claim | Verdict |
|---|---|
| Every plane-header offset and name above | **Proven** against `CDDrawWorkerHost::Read` @0x161640 |
| Every named main-header offset above | **Proven** against `CGameLevel::LoadWwd` @0x15d280 |
| Plane flags 0x01 / 0x02 / 0x04 / 0x08 / 0x10 | **Proven**, four separate retail consumers |
| Main flags 0x1 "use z coords" | **Proven** — `CGameLevel::VisitVisible` @0x15dc90 |
| Tile ids: `0xFFFFFFFF` invisible, `0xEEEEEEEE` filled | **Proven** — `CDDrawWorkerHost::Draw` @0x162010 |
| Object `score…health` at 0x34..0x48 | **Proven** — the scatter at 0x162af0 |
| Object `move_res_x` 0x114, `object_type` 0x10C | **Proven** offsets (semantics not re-derived) |
| Main header 0x00 is a `signature` | **Contradicted** — it is a header *size*: retail compares `<=`, then does arithmetic with it |
| `flags_add` 0x24 semantics | Cannot be checked: retail **skips the field without loading it** |
| Tile attributes 0..4 = clear/solid/ground/climb/death | **Partly contradicted for Gruntz** — see the tile-attribute section; and the domain is far larger (0x0a..0x9a) |
| `tile_type` 1 single / 2 double / 3 mask | **Proven as a discriminator**, renamed: uniform / rect / pixel-map |
| Checksum formula | **Contradicted as written** (0/63); the corrected form reproduces 52/63 |
