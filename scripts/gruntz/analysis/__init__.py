"""gruntz.analysis - one-shot analysis / discovery tools (run as `python -m`).

  link_order      reconstruct the original link order from the EXE
  tidy_audit      on-demand clang-tidy de-hack finder (casts/dead/unused); `gruntz lint`
  xref            who-calls-this: retail call/jmp-graph callers of an RVA/name
  clangd_query    one-shot clangd queries over the compile DB (also a helper)
  fid_generate    regenerate config/library_labels.csv (drives the fid/ stages)
  fid/            the masked-byte COFF-signature matcher (subpackage)

(The interactive navigation tools live in gruntz/sema/ - one module per
`gruntz sema` subcommand; retired one-shots live in scripts/archive/.)

Each is runnable as `python -m gruntz.analysis.<name>`; clangd_query also
exports a `Clangd` class imported by gruntz.match.fingerprints.
"""
