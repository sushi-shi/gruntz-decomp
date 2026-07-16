# Brief: the TU-partition lane gates BOTH .text layout AND the data loop

For the dedicated lane. Written by the DATA lane after the contribution-manifest
attempt was measured and found blocked. Companion: `docs/data-attribution.md`.

## What a contribution range is, and why we need it

Per (compiland, storage) a **contiguous** `[rva, size)` interval. A linker lays each
object's contribution to a section down in one piece, so real contributions never
interleave. homm2 reads them free from CodeView NB09 `sstModule`; **GRUNTZ.EXE has no
debug stream**, so ours must be derived from our own TU partition. They gate:

- `--contribution-manifest` → owner attribution for data we cannot otherwise place:
  the 290 COMDAT string payloads (296 retail copies) whose retail owner is currently
  unprovable, plus the `$T`/`$S` constant pools;
- `--data-section-manifest` → placing definitions at their candidate `section_offset`
  (candidate-shaped target sections), which is the rest of `matched_data`;
- absolute-RVA layout: `exe-diff` §B abs-rva ≈ 0, §E 0/771 at the correct
  section-relative offset.

## Measured state: our TU partition != retail's compilands

- `gruntz.analysis.tu_order_check`: **GATE FAIL — 52 TUs with intra-order violations,
  12134 interleaving TU-pairs** (e.g. `Fader [0x17e450-0x182935]` interleaves
  `RezBufferObjectDtor`/`RecordFill`/`CircleShadeBlit`; `MenuItem [0x184610-0x185a0e]`
  interleaves `RezColl`/`DebugPrintf`/`RezList`/`WapUncompress`).
- Independent check (hull of each unit's labelled symbols per storage, **all-pairs**
  overlap): `.text` **134/259** unit hulls interleave. Data is no better once measured
  correctly: only **8 of 86** bands overlap no other band — rdata **2/15**, data
  **5/17**, bss **1/54**.
- Consequence, measured: **0 of 296** ambiguous COMDAT string copies can be attributed
  by clean-band containment. The data contribution manifest is NOT independently
  recoverable; it waits on this lane.

CAUTION for whoever measures this next: check overlap **all-pairs**, not just adjacent
bands in a sorted list. Adjacent-only is blind to one band swallowing many others and
reports ~70-80% clean where the truth is ~9%. That error is why this brief exists.

## Worst .text offenders by span (a single compiland cannot do this)

| unit | fns | .text hull span |
|---|---:|---:|
| motionstate | 6 | 0x15bd60 (1.4 MB) |
| movinglogic | 6 | 0x15bb60 |
| gametext | 16 | 0x155280 |
| butemgr | 54 | 0x1539a0 |
| gruntzrandom | 5 | 0x14fee0 |
| movieplayer | 3 | 0x143730 |
| ddrawsurfacepair | 44 | 0x12bee0 |
| gruntvoice | 19 | 0x107530 |

Aggregates (`globals`, `vtables`) span by construction and are not compilands — exclude
them from band analysis, but they must drain eventually too. (`src/Stub/All.cpp` /
`engine_label_stubs` is **gone** — that backlog is fully drained; only prose references
survive in `config/units.toml` comments and in `CLAUDE.md`, which is stale on this
point. Excluding it changes nothing: 8/86 clean either way.)

## Instruments already in place

- `gruntz link` → candidate EXE + `.map` (392 objs, 4886 unresolved externs under
  `/FORCE`); `gruntz link --analyze` / `gruntz.analysis.link_order` for build order.
- `gruntz.analysis.tu_order_check` — the one-contiguous-block invariant gate.
- `gruntz exe-diff` §B — `.text` intra-TU order / block-exact / abs-RVA + a ranked
  reorder worklist; §E — data static-storage: 771/925 data symbols defined+placed,
  715/771 in the right storage class, 0/771 at the right section-relative offset, with
  the first section-relative divergence per section.

## Method (homm2's, and it is the right one here)

Fix the **earliest** divergence in a section, relink, re-read: later rows are cumulative
consequences, not independent defects. Split holding TUs by RVA gap and chase the real
owner by xref (see the holding-TU-drain notes). The metric to drive is
`tu_order_check`'s violation/interleave count to 0; `exe-diff` §B/§E then start moving,
and the data lane can generate contributions and finish `matched_data`.
