# exe-map — a visual map of retail `GRUNTZ.EXE`'s `.text`

A small, self-contained website that answers *what lives where* in the retail
binary and *what might be in the wrong `src/` file*. It is built from the same data
the matching pipeline produces (`gruntz build`), read through
`gruntz.core.exe_map`.

## The pages

| file | what it shows |
|---|---|
| `scatter.html` | **all functions** — one dot per source file: functions vs contiguous *fragments* (log–log), plus the fragmentation-ratio distribution. Overview of how the linker laid TUs out. |
| `scatter_methods.html` | **destructors removed** — same, but the base `??1` + deleting `??_G`/`??_E` destructors (which MSVC COMDAT-pools away from the TU block) are dropped. Shows the *true* per-TU layout; constructors are kept (they cluster with the methods). |
| `misplacement.html` | **the misplacement finder** — with destructors removed, functions far from their file's main block are flagged as either a **mis-attribution** (belongs in another file) or a **conflated TU** (one `src/` name covering two real TUs). Five graphs incl. an RVA "genome" (with named game/engine/library **regions**) of where each file's functions actually sit and a suggested-home lollipop. |
| `homm2.html` | **ground-truth baseline** — HoMM2 (HEROES2W) ships a real CodeView debug stream, so its function→file grouping is *truth*. Every one of its 94 TUs is a single contiguous block (the linker scatters nothing for correctly-grouped, non-`/Gy` code). Overlaid against Gruntz to show how much of Gruntz's "scatter" is our grouping vs the linker. Reads a read-only VA snapshot from the `homm2-decomp` project. |
| `deep.html` | **the deep map** — genome strip (code family per bin + int3 holes + `__FILE__` anchors), the reconstructed **original-TU interval track**, the **original link order** (CRT init-table skeleton), and the measured mechanism verdicts for every outlier. Actionable output: `TU_MIGRATION.md`. |

## Concepts

- **Fragment** — a maximal run of a file's functions that are adjacent in global RVA
  order. `1` fragment = one solid block; `fragments == function count` = every
  function isolated. This is a property of the **retail link layout**, verified in
  [`../link-order-investigation.md`](../link-order-investigation.md): MSVC `/O2`
  emits each function as a COMDAT and the linker concatenates a TU's COMDATs in
  source order → a TU is *mostly* one contiguous block.
- **Cluster / main block** — the misplacement finder groups a file's functions by
  RVA gap (`> 0x4000`); the largest group is the *main block*, the rest are
  outliers. A file with ≥2 blocks of ≥3 methods is flagged **conflated**.
- **Suggested home** — for an outlier, the other file owning the methods within
  `0x2000` of it (the TU whose territory it landed in).
- **Caveat** — a heuristic for review, not ground truth. `Serialize` (like
  destructors) is also COMDAT-pooled, so a lone `Serialize` outlier can be a benign
  exile rather than a real misplacement.

## Regenerate

```sh
nix develop --command python docs/exe-map/build_site.py
```

Each step writes its output (JSON + HTML) into this directory. Intermediate data:
`scatter.json`, `scatter_methods.json`, `flags.json`.

## Files

- `build_site.py` — driver (runs the four generators in order).
- `scatter.py` — per-file fragment stats → `scatter*.json`.
- `flag_outliers.py` — cluster/outlier/conflated detection → `flags.json`.
- `make_chart.py` — builds the two scatter pages.
- `make_dashboard.py` — builds the misplacement dashboard.
- `homm2_baseline.py` — **read-only** extract of the HoMM2 ground-truth VAs from
  `/home/sheep/Projects/homm2/homm2-decomp` (its CodeView-derived `symbol_names.csv`
  + `units.toml`) into `homm2_va.csv`; skips cleanly if that project is absent.
- `make_homm2.py` — builds `homm2.html` from the snapshot + Gruntz's data.
- `probe.py` — ad-hoc layout probe (e.g. per-file cluster breakdown); handy when
  investigating a specific unit.
- `demo_oracle.py` — **placement oracle**: finds each outlier in `GruntDem.exe`
  (a different link of the same build session) by reloc-masked byte search and
  compares its relative position → `demo_oracle.json`. Measured: 170/181
  comparable outliers sit identically in both links, only 3 true ilink moves —
  placements are **first-link birth positions**. Slow; not in the default
  pipeline; rerun manually after big re-homing waves.
- `deep_layout.py` — consolidates the 2026-07-10 layout investigation: gap census
  (switch tables / int3 pads / unclaimed code ⇒ the EXE is `/INCREMENTAL`-linked),
  the CRT init-table skeleton (original obj **link order** from the never-moving
  `$E` fragments; 501 zeroed slots), `__FILE__` assert-string anchors, per-outlier
  mechanism verdicts (REHOME / COMDAT-POOL / COMDAT-AT-USAGE / ILINK-MOVED), and
  the **original-TU interval partition** (only ≥3-function same-unit runs may span
  a boundary) → `deep_layout.json` + **`TU_MIGRATION.md`** (merge/split/move
  instructions, our units → original TUs).
- `make_deep.py` — builds `deep.html` from `deep_layout.json`.

## Baseline finding

With **ground-truth grouping** (HoMM2), the MSVC linker produces **100% contiguous
per-TU blocks** — zero scatter. Gruntz, on our *reconstructed* grouping, is only
~16% contiguous.

The difference is partly the **toolchain**: HoMM2 is **MSVC 4.2** (whose `/O2` does
NOT force per-function COMDATs — 39% of its functions are 16-byte aligned, i.e.
whole-TU blocks), while Gruntz is **MSVC 5.0** (whose `/O2` *forces* `/Gy`/COMDAT on
— 100% of its functions are 16-byte aligned in their own section; proven by
byte-exact zlib COMDATs, see `../zlib-matching.md`). So Gruntz's functions are
individually orderable and its destructors get pooled, whereas HoMM2's can't move.

Net: Gruntz's measured "scatter" is **our grouping errors** (conflated TUs,
mis-attributions) **plus** its COMDAT/destructor build behaviour — not the linker
mixing up correctly-grouped code. HoMM2 is the no-COMDAT floor. See `homm2.html`.
