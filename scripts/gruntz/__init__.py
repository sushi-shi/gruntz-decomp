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
  analysis/ one-shot analysis / discovery tools - link_order, gen_match_queue,
            dump_target, clangd_query, string_xref, fid_generate + fid/

Runnable entrypoints: the pipeline build steps (build/, init/, ghidra/) are
path-invoked by ninja/the CLI; the CLI/match/analysis tools run as
`python -m gruntz[.<module>]`. Either way `scripts/` must be on PYTHONPATH
(the nix dev shells + the `gruntz` wrapper set it).
"""
