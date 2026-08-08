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

## The per-resource codecs

PID / RID / ANI / PAL / FNT / WWD / XMI / PCX / RLE16 live in the module docs of
[`tools/gruntz-codec`](../../tools/gruntz-codec), with the results summarised in
[`tools/README.md`](../../tools/README.md). **They do not have a doc here yet**,
and the coverage is uneven — audited 2026-08-08:

| module | module doc | retail RVAs cited | state |
|---|---|---|---|
| `pid.rs` | 56 lines | 30 | the two grammars, every flag bit, both exporters — thorough |
| `rle16.rs` | 42 | 14 | the row-end split and why it is unobservable |
| `pcx.rs` | 29 | 11 | good |
| `ani.rs` | 40 | 6 | header proven complete (20 of 32 bytes are never read); **record mode fields are named but their value domains are not enumerated** |
| `bmp.rs` | 11 | 3 | good for what it covers |
| `wwd.rs` | 9 | 1 | **~25 header offsets read and 15 plane fields named, with no field map written down anywhere** |
| `pal.rs` | 6 | 2 | small format, adequately covered |
| `fnt.rs` | 6 | 1 | small format, adequately covered |
| `rid.rs` | 5 | 1 | defers to PID's header, correctly |
| `fec.rs` | 5 | 2 | the name/padding scrambling is in code only |
| `xmi.rs` | 7 | 0 | derived from Miles' headers, not retail — legitimately RVA-free, and says so |
| `oracle/map.rs` | **0** | 0 | 650 lines of WWD rendering with no module doc |
| `oracle/midi.rs` | **0** | 0 | XMI→SMF conversion; constants are documented, the module is not |

The two rows worth attention are `wwd.rs` and `oracle/map.rs`: between them they
carry a recovered WWD plane-header field map (`movement_x_percent`, `z_coord`,
`fill_color`, tile-grid geometry, the image-set registry) that exists **only as
Rust struct field names**. It is not in `docs/`, not in `docs/domain/`, and not
in `src/Wwd/`. That is the same failure this directory was created to fix.
