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

Per-resource codecs (PID / RID / ANI / PAL / FNT / WWD / XMI / PCX / RLE16) are
documented in the module docs of [`tools/gruntz-codec`](../../tools/gruntz-codec),
each citing the retail RVA that proves it, with the results summarised in
[`tools/README.md`](../../tools/README.md).
