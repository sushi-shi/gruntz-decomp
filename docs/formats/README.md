# docs/formats — the on-disk asset formats

What the shipped data files *are*, derived from the archived bytes and from
retail `GRUNTZ.EXE`'s own reader disassembly. Distinct from
[`docs/domain/`](../domain/), which covers what the data *means* to the game.

The executable code for these lives in [`tools/`](../../tools) — clean-room
Rust, independent of the C++ reconstruction under `src/`, so the two can
disagree and be caught.

| Doc | Covers |
|---|---|
| [rez-v1.md](rez-v1.md) | Monolith **REZ v1** container (`.REZ`, `.VRZ`): the 168-byte header, the directory/resource entry grammar, what `is_sorted` actually asserts, and the reader + writer in `tools/gruntz-rez` |
| [wwd-v1.md](wwd-v1.md) | **WWD** level geometry: the 1524-byte file header and 160-byte plane header field-by-field, the tile grid and handle split, the tile-attribute table and its value domain, the object record, and the checksum. Every field tiered proven / proven-unread / inferred / contradicted |
| [ani-v1.md](ani-v1.md) | **ANI** record **value domains**: `flags`, `step_mode`, `loop_mode`, `position_mode`, `param`, `duration`, `draw_value` — the complete observed value set over 1038 resources / 13 480 records, against the retail consumers and the `GZ_ENUM_*` types we already have |

### A note on third-party specs

A public WWD specification exists
([datashenanigans.pl](https://datashenanigans.pl/2020/06/gruntz-wwd-specification/),
transcribed at [`docs/reference/wwd-spec-datashenanigans.md`](../reference/wwd-spec-datashenanigans.md)),
and `tools/gruntz-codec/src/wwd.rs` plus `include/Wwd/WwdFile.h` carry field
names that match it closely. Neither is therefore independent evidence. A
third-party spec is a **hypothesis**; the deliverable is checking it against
retail's disassembly and the shipped bytes, and saying per field which way it
went. `wwd-v1.md` does that — including the places where the answer is "retail
never reads this", which no external spec can tell you.

## The per-resource codecs

PID / RID / PAL / FNT / XMI / PCX / RLE16 live only in the module docs of
[`tools/gruntz-codec`](../../tools/gruntz-codec), with the results summarised in
[`tools/README.md`](../../tools/README.md). **They do not have a doc here yet**,
and the coverage is uneven — audited 2026-08-08, revised after the WWD/ANI pass:

| module | module doc | retail RVAs cited | state |
|---|---|---|---|
| `pid.rs` | 56 lines | 30 | the two grammars, every flag bit, both exporters — thorough |
| `rle16.rs` | 42 | 14 | the row-end split and why it is unobservable |
| `pcx.rs` | 29 | 11 | good |
| `ani.rs` | 40 | 6 | header proven complete (20 of 32 bytes are never read); value domains now in [ani-v1.md](ani-v1.md) |
| `bmp.rs` | 11 | 3 | good for what it covers |
| `wwd.rs` | 9 | 1 | thin, but the field map it implies is now verified in [wwd-v1.md](wwd-v1.md) |
| `pal.rs` | 6 | 2 | small format, adequately covered |
| `fnt.rs` | 6 | 1 | small format, adequately covered |
| `rid.rs` | 5 | 1 | defers to PID's header, correctly |
| `fec.rs` | 5 | 2 | the name/padding scrambling is in code only |
| `xmi.rs` | 7 | 0 | derived from Miles' headers, not retail — legitimately RVA-free, and says so |
| `oracle/map.rs` | 60 | 12 | the WWD -> PNG renderer, each step against its retail counterpart |
| `oracle/midi.rs` | **0** | 0 | XMI→SMF conversion; constants are documented, the module is not |

`oracle/midi.rs` is the remaining undocumented module.
