"""gruntz - the Gruntz matching pipeline package (driven by ../../gruntz.py).

Grouped by area; this package holds ONLY the pipeline (build / link-future /
ghidra-refresh / init):
  build/   matching build - cc_wrap, labels, structs, synth_pdb, delink, reloc,
           ninja_syntax (the ninja compile -> labels -> delink chain)
  ghidra/  comprehension-DB enrichment - apply, export (Jython postScripts)
  init/    local-environment setup - toolchain, clangd

One-shot / analysis tools live OUTSIDE this package, in scripts/analysis/.
"""
