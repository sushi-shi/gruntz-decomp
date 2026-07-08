# exe-map — a visual map of retail `GRUNTZ.EXE`'s `.text`

A small, self-contained website that answers *what lives where* in the retail
binary and *what might be in the wrong `src/` file*. It is built from the same data
the matching pipeline produces (`gruntz build`), read through
`gruntz.analysis.exe_map`.

## The pages

| file | what it shows |
|---|---|
| `scatter.html` | **all functions** — one dot per source file: functions vs contiguous *fragments* (log–log), plus the fragmentation-ratio distribution. Overview of how the linker laid TUs out. |
| `scatter_methods.html` | **destructors removed** — same, but the base `??1` + deleting `??_G`/`??_E` destructors (which MSVC COMDAT-pools away from the TU block) are dropped. Shows the *true* per-TU layout; constructors are kept (they cluster with the methods). |
| `misplacement.html` | **the misplacement finder** — with destructors removed, functions far from their file's main block are flagged as either a **mis-attribution** (belongs in another file) or a **conflated TU** (one `src/` name covering two real TUs). Five graphs incl. an RVA "genome" of where each file's functions actually sit and a suggested-home lollipop. |

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
- `probe.py` — ad-hoc layout probe (e.g. per-file cluster breakdown); handy when
  investigating a specific unit.
