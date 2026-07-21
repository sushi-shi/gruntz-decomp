"""gruntz.permute - the source-permutation climbers (LIVE drive-to-100%
campaign engines; see the `permute` skill for when to reach for each):

  permute.py               the wine-cl + objdiff hill-climber (one fn)
  permute_sweep.py         run the permuter over a whole unit, top-down
  match_variants.py        the unified exhaustive search frontend
                           (AST x TU-state x hand axes; --state-trials)
  generate_ast_variants.py   its libclang AST axis
  batch_source_variants.py   its batch compile+score engine
  tu_state_metrics.py        COFF metric helper for the TU-state trials
  tu_state_noise.py          parser-visible TU-state variant engine

Use ONLY on a COMPLETE, correct reconstruction stuck on /O2 codegen residue -
wrong structure/types are fixed by hand first (docs/matching-patterns.md).
"""
