# -*- coding: utf-8 -*-
# decomp_export.py - decompiler + xref dump for a list of target RVAs.
#
# The "decomp-xref" half of the labeling loop (companion to gruntz.sema.strings).
# String refs say WHICH data a function touches; the decompiler says what it DOES:
# this reads, for each target, the Ghidra decompiler C output plus caller/callee
# xrefs, exposing calling convention, arg count, virtual-slot identity (vfunc_N
# caller thunks), and this+offset member writes / .att attribute reads. That turns
# guesses ("LoadX from strings") into grounded labels (class, vfunc slot, prototype)
# -- and reveals library/CRT/ambiguous funcs that string-only labeling mislabels
# (e.g. CRT _heapwalk, the C++ Tools error handler).
#
# READ-ONLY. Run on a *copy* of the named DB so the shared comprehension DB and any
# orchestrator lock are untouched:
#   cp -r build/ghidra-named build/ghidra-decomp
#   printf '0x141400\n0x0b82e0\n' > /tmp/decomp_targets.txt
#   analyzeHeadless build/ghidra-decomp gruntz -process GRUNTZ.EXE -noanalysis \
#       -readOnly -scriptPath scripts/gruntz/ghidra \
#       -postScript decomp_export.py
# Reads /tmp/decomp_targets.txt (one 0xRVA per line); writes /tmp/decomp_out.txt.
# Needs Ghidra 11.4.2 (Jython); flake's default `ghidra` is 11.4.2.
#@category Gruntz
from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

IMAGE_BASE = 0x400000
prog = currentProgram
fm = prog.getFunctionManager()
af = prog.getAddressFactory().getDefaultAddressSpace()
mon = ConsoleTaskMonitor()
ifc = DecompInterface(); ifc.openProgram(prog)

targets = [int(ln.strip(), 16) for ln in open("/tmp/decomp_targets.txt")
           if ln.strip().startswith("0x")]
out = open("/tmp/decomp_out.txt", "w")
def w(s): out.write(s); out.write("\n")

for rva in targets:
    fn = fm.getFunctionContaining(af.getAddress(IMAGE_BASE + rva))
    w("\n" + "="*78)
    if fn is None:
        w("### 0x%06x  <no function>" % rva); continue
    ep = fn.getEntryPoint().getOffset() - IMAGE_BASE
    w("### 0x%06x  %s  cc=%s  sig=%s" % (ep, fn.getName(),
        fn.getCallingConventionName(), fn.getSignature(False).getPrototypeString()))
    callers = []
    for r in getReferencesTo(fn.getEntryPoint()):
        if r.getReferenceType().isCall():
            cf = fm.getFunctionContaining(r.getFromAddress())
            callers.append((cf.getName()+"@0x%06x"%(cf.getEntryPoint().getOffset()-IMAGE_BASE))
                           if cf else "0x%06x"%(r.getFromAddress().getOffset()-IMAGE_BASE))
    w("CALLERS(%d): %s" % (len(callers), ", ".join(sorted(set(callers))[:25])))
    callees = sorted(set(c.getName()+"@0x%06x"%(c.getEntryPoint().getOffset()-IMAGE_BASE)
                         for c in fn.getCalledFunctions(mon)))
    w("CALLEES(%d): %s" % (len(callees), ", ".join(callees[:40])))
    try:
        res = ifc.decompileFunction(fn, 45, mon)
        if res and res.decompileCompleted():
            lines = res.getDecompiledFunction().getC().split("\n")
            if len(lines) > 110:
                lines = lines[:110] + ["    /* ... (%d more lines) ... */" % (len(lines)-110)]
            w("\n".join(lines))
        else:
            w("/* decompile failed: %s */" % (res.getErrorMessage() if res else "null"))
    except Exception as e:
        w("/* decompile exception: %s */" % e)
out.close()
print("decomp_export: wrote /tmp/decomp_out.txt for %d targets" % len(targets))
