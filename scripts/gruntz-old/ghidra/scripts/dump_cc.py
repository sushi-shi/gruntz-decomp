# -*- coding: utf-8 -*-
# dump_cc.py - dump per-function calling convention + stack purge for EVERY
# function in the (analyzed) Ghidra DB. Feeds the dynamic this/ecx tracer: the
# __thiscall set (incl. unknown FUN_ bodies) is the breakpoint list.
#
# Run as a GhidraScript under the PyGhidra driver (ghidra_metadata_apply.py),
# typically with --no-analyze against the already-analyzed build/ghidra-named DB.
# Output path from $GRUNTZ_CC_OUT (default /tmp/cc_all.csv).
#@category Gruntz
import os

IMAGE_BASE = 0x400000
prog = currentProgram
fm = prog.getFunctionManager()
OUT = os.environ.get("GRUNTZ_CC_OUT", "/tmp/cc_all.csv")

fh = open(OUT, "w")
n = 0
try:
    fh.write("entry_rva,cc,purge,name\n")
    for fn in fm.getFunctions(True):
        ep = fn.getEntryPoint()
        if ep is None or not ep.isMemoryAddress():
            continue
        rva = ep.getOffset() - IMAGE_BASE
        try:
            cc = fn.getCallingConventionName()
        except Exception:
            cc = "?"
        try:
            purge = fn.getStackPurgeSize()
        except Exception:
            purge = -1
        fh.write("0x%x,%s,%d,%s\n" % (rva, cc, purge, fn.getName()))
        n += 1
finally:
    fh.close()
print("[dump_cc] wrote %d functions -> %s" % (n, OUT))
