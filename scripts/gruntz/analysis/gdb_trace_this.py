# gdb_trace_this.py - gdb-side driver for dynamic this/ecx tracing of GRUNTZ.EXE.
#
# NOT a `python -m` module: this runs *inside* gdb (it imports the `gdb` module),
# sourced via `gdb -x`. winedbg's gdb proxy launches gdb with its own -x init
# (which does `target remote localhost:<port>`); this script is appended after,
# so when it runs the 32-bit inferior is connected and stopped at startup.
#
# What it does, once, at load (inferior stopped, image mapped at base 0x400000):
#   1. (optional) patch free / operator delete entry -> `ret` so nothing is ever
#      deallocated. With no frees, an address is never reused => every distinct
#      ecx value is a globally-unique object identity for the whole run, so
#      class clustering reduces to grouping functions by shared ecx.
#   2. plant a self-retiring breakpoint at each thiscall entry (base+rva). Each
#      stop() reads $ecx (the `this`), dedups on (rva,ecx), and appends NEW edges
#      to GRUNTZ_TRACE_OUT incrementally (crash-safe under an abrupt kill). A bp
#      retires (disables -> int3 removed -> zero overhead) once it has seen K
#      distinct objects or M total hits, so hot per-frame methods stop costing
#      traps once saturated.
#
# Config via env:
#   GRUNTZ_TRACE_FUNCS  CSV with a hex rva first column (name/class cols optional)
#   GRUNTZ_TRACE_OUT    output edge CSV (rva,ecx,seq)
#   GRUNTZ_TRACE_K      retire after K distinct ecx        (default 16)
#   GRUNTZ_TRACE_MAXHITS retire after M total hits         (default 1024)
#   GRUNTZ_TRACE_BASE   image base                         (default 0x400000)
#   GRUNTZ_TRACE_PATCH  "1" to neutralize free/delete      (default 1)
import gdb
import csv
import os

BASE = int(os.environ.get("GRUNTZ_TRACE_BASE", "0x400000"), 0)
FUNCS = os.environ["GRUNTZ_TRACE_FUNCS"]
OUT = os.environ["GRUNTZ_TRACE_OUT"]
K = int(os.environ.get("GRUNTZ_TRACE_K", "16"))
MAXHITS = int(os.environ.get("GRUNTZ_TRACE_MAXHITS", "1024"))
PATCH = os.environ.get("GRUNTZ_TRACE_PATCH", "1") == "1"

# free entry RVA (retail GRUNTZ.EXE). __cdecl: a bare `ret` (0xC3) at entry is
# caller-cleaned and ABI-safe, so every free becomes a no-op. Patching _free
# alone also neutralizes operator delete (MSVC `delete` -> `free`), so we avoid
# poking the LOW-confidence ??3@ slot.
FREE_RVAS = [0x120c30]  # _free  (HIGH confidence; covers operator delete via free)

_log = open("/tmp/gruntz_trace.log", "w")
def log(m):
    _log.write(m + "\n"); _log.flush()
    try: gdb.write("TRACE: " + m + "\n"); gdb.flush()
    except Exception: pass

def _u32(v):
    return int(v) & 0xffffffff

_out = open(OUT, "w", buffering=1)   # line-buffered => crash-safe
_out.write("rva,ecx,seq\n")
_seq = [0]

class TBP(gdb.Breakpoint):
    __slots__ = ("rva", "seen", "hits")
    def __init__(self, rva):
        super().__init__("*0x%x" % (BASE + rva), type=gdb.BP_BREAKPOINT, internal=True)
        self.rva = rva
        self.seen = set()
        self.hits = 0
    def stop(self):
        self.hits += 1
        try:
            ecx = _u32(gdb.selected_frame().read_register("ecx"))
        except Exception:
            return False
        if ecx not in self.seen:
            self.seen.add(ecx)
            _seq[0] += 1
            _out.write("0x%x,0x%x,%d\n" % (self.rva, ecx, _seq[0]))
        if len(self.seen) >= K or self.hits >= MAXHITS:
            self.enabled = False   # retire: gdb removes the int3 -> no more overhead
        return False               # never halt; auto-continue

def load_rvas(path):
    rvas = []
    with open(path) as f:
        for row in csv.reader(f):
            if not row:
                continue
            c0 = row[0].strip().strip('"')
            if not c0 or c0.lower() == "rva":
                continue
            try:
                rvas.append(int(c0, 16))
            except ValueError:
                pass
    return rvas

def main():
    log("driver loaded; base=0x%x funcs=%s out=%s K=%d M=%d patch=%s"
        % (BASE, FUNCS, OUT, K, MAXHITS, PATCH))
    # The game (and wine's wow64 layer) raise CPU exceptions and handle them via
    # Windows SEH. gdb halts on these by default, freezing the game. Pass them
    # through silently so the inferior's own handlers run and it keeps executing.
    for sig in ("SIGSEGV", "SIGILL", "SIGFPE", "SIGBUS", "SIGUSR1"):
        try:
            gdb.execute("handle %s nostop noprint pass" % sig)
        except Exception:
            pass
    inf = gdb.selected_inferior()
    if PATCH:
        for rva in FREE_RVAS:
            try:
                inf.write_memory(BASE + rva, b"\xc3")
                log("patched free/delete @0x%x -> ret" % (BASE + rva))
            except Exception as e:
                log("patch failed @0x%x: %r" % (BASE + rva, e))
    rvas = load_rvas(FUNCS)
    log("planting %d thiscall breakpoints..." % len(rvas))
    n = 0
    for rva in rvas:
        try:
            TBP(rva); n += 1
        except Exception as e:
            log("bp fail 0x%x: %r" % (rva, e))
    log("planted %d/%d breakpoints; continuing" % (n, len(rvas)))
    try:
        gdb.execute("continue")
    except Exception as e:
        log("continue ended: %r" % e)
    _out.flush()
    log("done; %d distinct (rva,ecx) edges written" % _seq[0])

main()
