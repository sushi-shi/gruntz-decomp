# -*- coding: utf-8 -*-
# decomp_gaps.py - force-create + decompile a list of target RVAs and dump, per
# function: calling convention, stack purge, prototype, CALLERS, CALLEES, and the
# decompiler C body. The C body exposes `this+offset` member reads/writes and the
# param usage, which is what re-homing needs (owning class + arg/member offsets).
#
# Companion of decomp_force.py; reads/writes paths from env so it can target the
# scratchpad instead of hardcoded /tmp. Run under the PyGhidra driver
# (ghidra_metadata_apply.py) against a COPY of build/ghidra-named (it mutates the
# DB by force-creating functions).
#@category Gruntz
import os
from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor
from ghidra.app.cmd.function import CreateFunctionCmd

IMAGE_BASE = 0x400000
TARGETS = os.environ.get("GAP_TARGETS", "/tmp/decomp_targets.txt")
OUTPATH = os.environ.get("GAP_OUT", "/tmp/decomp_out.txt")

prog = currentProgram
fm = prog.getFunctionManager()
af = prog.getAddressFactory().getDefaultAddressSpace()
mon = ConsoleTaskMonitor()
ifc = DecompInterface()
ifc.openProgram(prog)

targets = [int(ln.strip(), 16) for ln in open(TARGETS) if ln.strip().startswith("0x")]
out = open(OUTPATH, "w")
def w(s): out.write(s); out.write("\n")

created = 0
for rva in targets:
    addr = af.getAddress(IMAGE_BASE + rva)
    fn = fm.getFunctionContaining(addr)
    if fn is None:
        tx = prog.startTransaction("create-fn-%x" % rva)
        try:
            CreateFunctionCmd(addr).applyTo(prog, mon)
        finally:
            prog.endTransaction(tx, True)
        fn = fm.getFunctionContaining(addr)
        if fn is not None:
            created += 1
    w("\n" + "=" * 78)
    if fn is None:
        w("### 0x%06x  <could not create>" % rva)
        continue
    ep = fn.getEntryPoint().getOffset() - IMAGE_BASE
    try:
        purge = fn.getStackPurgeSize()
    except Exception:
        purge = -1
    w("### 0x%06x  %s  cc=%s  purge=%d  sig=%s" % (
        ep, fn.getName(), fn.getCallingConventionName(), purge,
        fn.getSignature(False).getPrototypeString()))
    # callers
    callers = []
    for r in getReferencesTo(fn.getEntryPoint()):
        if r.getReferenceType().isCall():
            cf = fm.getFunctionContaining(r.getFromAddress())
            callers.append((cf.getName() + "@0x%06x" % (cf.getEntryPoint().getOffset() - IMAGE_BASE))
                           if cf else "site@0x%06x" % (r.getFromAddress().getOffset() - IMAGE_BASE))
    w("CALLERS(%d): %s" % (len(callers), ", ".join(sorted(set(callers))[:30])))
    callees = sorted(set(c.getName() + "@0x%06x" % (c.getEntryPoint().getOffset() - IMAGE_BASE)
                         for c in fn.getCalledFunctions(mon)))
    w("CALLEES(%d): %s" % (len(callees), ", ".join(callees[:60])))
    try:
        res = ifc.decompileFunction(fn, 60, mon)
        if res and res.decompileCompleted():
            w(res.getDecompiledFunction().getC())
        else:
            w("/* decompile failed: %s */" % (res.getErrorMessage() if res else "null"))
    except Exception as e:
        w("/* decompile exception: %s */" % e)

out.close()
print("decomp_gaps: created %d fns, wrote %s for %d targets" % (created, OUTPATH, len(targets)))
