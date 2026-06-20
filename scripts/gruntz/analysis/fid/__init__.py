"""gruntz.analysis.fid - the masked-byte COFF-signature matcher (NOT Ghidra FID).

Driven by gruntz.analysis.fid_generate; the stages run as `python -m`:

  coff_sig    extract masked per-symbol signatures from the static libs
  classify    anchored matches at Ghidra function starts
  unanchored  bodies at starts Ghidra missed
  common      shared PE/COFF helpers imported by the stages

`common` is a real module here: classify/unanchored import it via
`from gruntz.analysis.fid.common import ...` (no sys.path hacks).
"""
