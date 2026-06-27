# -*- coding: utf-8 -*-
# decomp_force.py - force-create + decompile a list of target RVAs.
#@category Gruntz
from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor
from ghidra.app.cmd.function import CreateFunctionCmd

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
    addr = af.getAddress(IMAGE_BASE + rva)
    fn = fm.getFunctionContaining(addr)
    if fn is None:
        tx = prog.startTransaction("create-fn-%x" % rva)
        try:
            CreateFunctionCmd(addr).applyTo(prog, mon)
        finally:
            prog.endTransaction(tx, True)
        fn = fm.getFunctionContaining(addr)
    w("\n" + "="*78)
    if fn is None:
        w("### 0x%06x  <could not create>" % rva); continue
    ep = fn.getEntryPoint().getOffset() - IMAGE_BASE
    w("### 0x%06x  %s  cc=%s  sig=%s" % (ep, fn.getName(),
        fn.getCallingConventionName(), fn.getSignature(False).getPrototypeString()))
    callees = sorted(set(c.getName()+"@0x%06x"%(c.getEntryPoint().getOffset()-IMAGE_BASE)
                         for c in fn.getCalledFunctions(mon)))
    w("CALLEES(%d): %s" % (len(callees), ", ".join(callees[:60])))
    try:
        res = ifc.decompileFunction(fn, 90, mon)
        if res and res.decompileCompleted():
            w(res.getDecompiledFunction().getC())
        else:
            w("/* decompile failed: %s */" % (res.getErrorMessage() if res else "null"))
    except Exception as e:
        w("/* decompile exception: %s */" % e)
out.close()
print("decomp_force: wrote /tmp/decomp_out.txt for %d targets" % len(targets))
