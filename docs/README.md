# Project documentation

Start with the root [README](../README.md) for setup and the build loop.

- [Build system](build-system.md) and [tooling map](tooling-map.md): commands and pipeline ownership.
- [Compiler profiles](compiler-flags.md), [linking](linker-flags.md), [toolchain setup](toolchain-vc50-sp3.md), and [compiler identification](compiler-detection.md).
- [Match tracking](match-status.md), [permuter](permuter.md), and the small [compiler-pattern reference](patterns/INDEX.md).
- [Data attribution](data-attribution.md), [linked-image comparison](image-diff.md), [cleanliness](cleanliness-metrics.md), and [source markers](comment-markers.md).
- [clangd](clangd.md) and [runtime DLLs](runtime-dlls.md).

## Source of truth and storage

Tool inputs belong in `config/`; generated reports belong in ignored `build/`.
Documentation is neither a runtime input nor a maintained copy of generated state.
The two review ledgers live in [config/reviews](../config/reviews/):
`enum-reuse.tsv` is read by the enum verifier; `compiler-methods.tsv` by
`scripts/audit-template-models.py`. Their schemas and validation live in those consumers.
Source-lineage decisions remain in [the lineage ledger](../config/lithtech_lineage.tsv).

Format layouts belong beside the implementations: the
[REZ writer](../tools/gruntz-rez/src/write.rs) and
[WWD parser](../tools/gruntz-codec/src/wwd.rs) include ASCII diagrams.
Do not maintain a parallel field-by-field specification here.

Generate the optional layout map with `python3 -m gruntz.sema.exe_map` inside
`nix develop`; output goes to `build/exe-map/`. It is a heuristic view of
current model attribution, not proof of original TU ownership.

## Historical investigations

Old audits, experiment diaries, mirrored community pages, and frozen dashboards
are recoverable from Git at `b27b05deb`, for example:

```sh
git show b27b05deb:docs/experiments/gy-scatter.md
```

Commit-pinned links are historical provenance, not maintained instructions or
endorsement of old conclusions. Use current code, retail evidence, and the
relevant tool's help rather than old commands or score tables.
Keep new docs about ongoing usage and contracts; do not add another PR diary.
