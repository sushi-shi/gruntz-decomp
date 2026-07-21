"""gruntz - THE package for all importable Gruntz tooling.

Everything importable lives here (no sys.path hacks, nothing at the top of
scripts/). Grouped by area:

  cli       the matching-pipeline CLI (`gruntz`/`python -m gruntz`):
            build / labels / structs / ghidra-refresh / init / status / ...
  build/    matching build - cc_wrap, labels, ghidra_metadata_generate, synth_pdb,
            delink, ninja_syntax (the ninja compile -> labels -> delink chain)
  ghidra/   comprehension-DB enrichment - apply, export, decomp_export (PyGhidra)
  init/     local-environment setup - toolchain, clangd
  match/    matching-progress tooling - status (the match CLI), fingerprints
            (its helper), verify_stubs (the build gate)
  core/     the shared engine library - pe, symbols, report, vtable_scan,
            vtable_hierarchy, exe_map, clangd_query, data_audit
  sema/     the `gruntz sema` navigation surface - one module per subcommand
  cleanliness/ the drive-to-0 board + quality gates
  permute/  the source-permutation climbers (the permute skill)
  audit/    one-shot campaign audits (`gruntz audit <tool>`) (retired ones: scripts/archive/)

Runnable entrypoints: the pipeline build steps (build/, init/, ghidra/) are
path-invoked by ninja/the CLI; the CLI/match/audit tools run as
`python -m gruntz[.<module>]`. Either way `scripts/` must be on PYTHONPATH
(the nix dev shells + the `gruntz` wrapper set it).
"""
