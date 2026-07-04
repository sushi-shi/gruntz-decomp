"""gruntz.analysis - one-shot analysis / discovery tools (run as `python -m`).

  link_order      reconstruct the original link order from the EXE
  tidy_audit      on-demand clang-tidy de-hack finder (casts/dead/unused); `gruntz lint`
  gen_match_queue build config/match-queue.md (the prioritized worklist)
  dump_target     dump a delinked target object for inspection
  xref            who-calls-this: retail call/jmp-graph callers of an RVA/name
  clangd_query    one-shot clangd queries over the compile DB (also a helper)
  string_xref     string-xref labeling aid for @stub metadata
  fid_generate    regenerate config/library_labels.csv (drives the fid/ stages)
  fid/            the masked-byte COFF-signature matcher (subpackage)

Each is runnable as `python -m gruntz.analysis.<name>`; clangd_query also
exports a `Clangd` class imported by gruntz.match.fingerprints.
"""
