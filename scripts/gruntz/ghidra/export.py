# -*- coding: utf-8 -*-
# export.py - dump functions.csv + symbols.csv from the (enriched) Ghidra DB.
#
# The TARGET side of the pipeline (synth_pdb -> vostok-delinker) reads these two
# CSVs:
#   functions.csv : entry_rva (hex), byte_size, name   -> S_GPROC32 (code, sizes)
#   symbols.csv   : address_rva (hex), name, is_primary -> S_LDATA32 (.rdata/.data)
# `gruntz ghidra-refresh` runs apply.py (push generated names/structs/enums) then
# this, so the re-exported CSVs reflect the latest labels and feed the next build.
#
# Run as a GhidraScript under PyGhidra (CPython3 + JPype), paired with apply.py in
# one run_enrich.py invocation (which boots PyGhidra and runs apply.py then this).
#@category Gruntz
import os

IMAGE_BASE = 0x400000
prog = currentProgram
fm = prog.getFunctionManager()
st = prog.getSymbolTable()

ROOT = os.environ.get("GRUNTZ_DIR", ".")
OUT = ROOT + "/build/ghidra-enrich/exports"
from java.io import File as _File
_File(OUT).mkdirs()


def _rva(addr):
    """image RVA for a memory address, or None for external/non-memory symbols."""
    if addr is None or not addr.isMemoryAddress():
        return None
    return addr.getOffset() - IMAGE_BASE


# --- functions.csv: every .text function (entry RVA, code size, name) ----------
nf = 0
fh = open(OUT + "/functions.csv", "w")
try:
    fh.write("entry_rva,byte_size,name\n")
    for fn in fm.getFunctions(True):           # True = forward (entry order)
        rva = _rva(fn.getEntryPoint())
        if rva is None:
            continue
        size = fn.getBody().getNumAddresses()  # bytes of code in the body
        fh.write("0x%x,%d,%s\n" % (rva, size, fn.getName()))
        nf += 1
finally:
    fh.close()

# --- symbols.csv: data symbols (.rdata/.data); synth_pdb filters by section ----
ns = 0
sh = open(OUT + "/symbols.csv", "w")
try:
    sh.write("address_rva,name,is_primary\n")
    for sym in st.getAllSymbols(True):
        rva = _rva(sym.getAddress())
        if rva is None:                        # external/import -> skip
            continue
        prim = 1 if sym.isPrimary() else 0
        sh.write("0x%x,%s,%d\n" % (rva, sym.getName(), prim))
        ns += 1
finally:
    sh.close()

print("[export] wrote %d functions, %d symbols -> %s" % (nf, ns, OUT))
