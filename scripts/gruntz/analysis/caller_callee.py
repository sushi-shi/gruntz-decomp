#!/usr/bin/env python3
"""gruntz.analysis.caller_callee - retail<->reconstruction CALL-GRAPH reconciliation metric.

Ties the retail call graph (xref: who calls whom, direct `call`/`jmp rel32`, ILT thunks
followed) to OUR reconstruction's emitted call graph (the mangled callees each src TU
compiles to, read from clang IR). Every retail edge caller->callee, where BOTH ends are
reconstructed functions, MUST be reproduced by our source as a call resolving to the same
callee RVA. The count of retail edges we FAIL to reproduce is the reconciliation metric -
drive it to 0.

Each unreconciled (missing) edge is a byte-blocker for the caller, and its CAUSE is the
worklist:
  * FAKE-VIEW  our source reaches the callee by casting to a local view whose method
    mangles to a name that does NOT resolve to the callee's RVA (`(CFooView*)x->M()`
    where the real callee is `?M@CBar@@`). Retype x to CBar / declare M on CBar -> the
    emitted symbol resolves, the edge reconciles, the view dissolves. (The cast whose
    view NAME already equals the real owner - e.g. NetMgr's `(CMulti*)this` - resolves
    fine and is NOT counted here; it is separate `.cpp-local view` cleanliness debt.)
  * STUB       the caller body is not reconstructed (src/Stub/) - reconstruction backlog.
  * MISSING    the caller is a real body but emits no call for this edge (indirect/PMF
    dispatch our source fakes, an @early-stop partial, or a genuinely-absent call).

`extra` edges (our source emits a call retail lacks at that spot) are reported too - usually
an inlining-boundary difference, occasionally a spurious call.

Usage (inside `nix develop` - needs clang + $GRUNTZ_EXE + build/clangd + symbol_names):
    python3 -m gruntz.analysis.caller_callee              # summary + metric + top causes
    python3 -m gruntz.analysis.caller_callee --metric     # just the numbers (tracking)
    python3 -m gruntz.analysis.caller_callee --worklist    # FAKE-VIEW dissolution worklist
    python3 -m gruntz.analysis.caller_callee --check 0xb5460   # one caller: retail vs emitted
    python3 -m gruntz.analysis.caller_callee --csv         # unreconciled edges, machine-readable
    python3 -m gruntz.analysis.caller_callee --md          # -> config/caller-callee-reconcile.md
    python3 -m gruntz.analysis.caller_callee --jobs 24
"""
import argparse
import bisect
import concurrent.futures
import csv
import os
import re
import struct
import sys
from pathlib import Path

from gruntz.analysis import xref
from gruntz.build import labels

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
SRC = REPO / "src"
SYMCSV = REPO / "build/gen/symbol_names.csv"
OUT_MD = REPO / "config" / "caller-callee-reconcile.md"

# special-member member tokens parse_mangled emits (ctor/dtor/deleting dtors/operators):
# their name is generic so a view's `??1V@@` cannot be tied to R by member name.
_SPECIAL = {"ctor", "dtor", "vec-dtor", "scalar-dtor", "op"}


# --- mangling / IR helpers (ex caller_audit, archived; the metric stays live) ---
# special member codes we can still attribute to a class (ctor/dtor/deleting
# dtors/operators). `?_7`/`?_8`/`?_R*` are vftable/RTTI DATA - skip.
_CTORDTOR = {"?0": "ctor", "?1": "dtor", "?_G": "vec-dtor", "?_E": "scalar-dtor"}


def parse_mangled(m):
    """(class_or_None, member_or_None, access_char_or_None) for a MSVC function
    mangling. No templates in this binary, so `find('@@')` reliably terminates
    the qualified name. Returns None for non-C++ / data-ish symbols."""
    if not m.startswith("?"):
        return None                        # extern "C" (_foo, _foo@N)
    b = m[1:]
    special = None
    if b.startswith("?"):
        b = b[1:]
        if b.startswith("_"):
            special = "?_" + b[1:2]
            b = b[2:]
        else:
            special = "?" + b[:1]
            b = b[1:]
        if special.startswith("?_") and special not in _CTORDTOR:
            return None                    # ?_7 vftable, ?_R0 RTTI, ... = data
    end = b.find("@@")
    if end < 0:
        return None
    tokens = b[:end].split("@")
    rest = b[end + 2:]
    access = rest[0] if rest else None
    if special is not None:
        cls = tokens[0] if tokens else None
        member = _CTORDTOR.get(special, "op")
        return (cls, member, access)
    member = tokens[0] if tokens else None
    cls = tokens[1] if len(tokens) > 1 else None   # None => free function
    return (cls, member, access)


_DEF_RE = re.compile(r'^define\b[^@\n]*@("(?:[^"\\]|\\.)*"|[-\w.$?@]+)\s*\(', re.M)
# an instruction line that is a call/invoke (labels sit at col 0; instructions
# are indented and start with an optional `%res =` then call/invoke).
_CALL_RE = re.compile(r'^\s+(?:%\S+\s*=\s*)?(?:tail |musttail |notail )?'
                      r'(?:call|invoke)\b')
_CALL_TGT_RE = re.compile(r'@("(?:[^"\\]|\\.)*"|[-\w.$?@]+)\s*(?:\(|to\b)')


def _ir_name(tok):
    if tok.startswith('"') and tok.endswith('"'):
        tok = tok[1:-1]
    if tok.startswith("\\01"):
        tok = tok[3:]
    return tok


def _tu_edges(args):
    """(caller_mangled -> set(callee_mangled)) for one TU, or None if IR failed.

    A function body spans `define ... {` .. the closing `}` at column 0; basic-block
    labels also sit at column 0, so scope is tracked by define/`}` only."""
    clang, tu, cl_flags = args
    ir = labels.emit_ir(clang, tu, [], cl_flags)
    if not ir:
        return tu, None
    out = {}
    cur = None
    for line in ir.splitlines():
        dm = _DEF_RE.match(line)
        if dm:
            cur = _ir_name(dm.group(1))
            out.setdefault(cur, set())
            continue
        if cur is None:
            continue
        if line.startswith("}"):
            cur = None                       # end of the function body
            continue
        if _CALL_RE.match(line):
            for tok in _CALL_TGT_RE.findall(line):
                out[cur].add(_ir_name(tok))
    return tu, out


# --- symbol data -----------------------------------------------------------
def load_symbols():
    """rva2sym: rva -> (mangled, unit, class, member); m2rva: mangled -> rva;
    func_rvas: set of reconstructed function starts."""
    rva2sym, m2rva, func_rvas = {}, {}, set()
    with open(SYMCSV) as f:
        for r in csv.DictReader(f):
            try:
                rva = int(r["rva"], 16)
            except (ValueError, KeyError):
                continue
            if r.get("kind") != "func":
                m2rva.setdefault(r["name"], rva)
                continue
            pm = parse_mangled(r["name"])
            cls = member = None
            if pm:
                cls, member, _ = pm
            rva2sym[rva] = (r["name"], r.get("unit", ""), cls, member)
            m2rva.setdefault(r["name"], rva)
            func_rvas.add(rva)
    return rva2sym, m2rva, func_rvas


# --- retail TARGET graph (thunk-followed) ----------------------------------
def _thunk_map(d, secs, names):
    """thunk_rva -> body_rva for every ILT-band `E9 rel32` entry (and ghidra thunk_*
    single-jmp). Chases up to 4 hops to a non-thunk body."""
    _n, tva, tvsz, trp, trsz = xref._text(secs)
    tmap = {}
    tb = d[trp:trp + trsz]
    # ILT band entries
    i = 0
    while i < len(tb) - 4:
        rva = tva + i
        if tb[i] == 0xE9 and xref.ILT_LO <= rva < xref.ILT_HI:
            rel = struct.unpack_from("<i", tb, i + 1)[0]
            tmap[rva] = rva + 5 + rel
        i += 1

    def resolve(x):
        seen = set()
        while x in tmap and x not in seen:
            seen.add(x)
            x = tmap[x]
        return x

    return {k: resolve(v) for k, v in tmap.items()}


def target_edges(d, secs, names, fstarts, fsize, func_rvas):
    """{(caller_rva, callee_rva)} for retail direct call/jmp, thunks followed, where
    BOTH caller and callee are reconstructed (in func_rvas). Also returns per-caller
    the full set of thunk-followed callee RVAs (reconstructed or not) for diagnosis."""
    _n, tva, tvsz, trp, trsz = xref._text(secs)
    tmap = _thunk_map(d, secs, names)
    tb = d[trp:trp + trsz]

    def owner(site):
        k = bisect.bisect_right(fstarts, site) - 1
        if k < 0:
            return None
        start = fstarts[k]
        sz = fsize.get(start)
        if sz and site >= start + sz:
            return None
        return start

    edges = set()
    allcallees = {}
    n = len(tb) - 4
    i = 0
    while i < n:
        op = tb[i]
        if op == 0xE8 or op == 0xE9:
            rel = struct.unpack_from("<i", tb, i + 1)[0]
            site = tva + i
            tgt = site + 5 + rel
            body = tmap.get(tgt, tgt)          # follow thunk to body
            o = owner(site)
            if o is not None and o != body and o in func_rvas:
                allcallees.setdefault(o, set()).add(body)
                if body in func_rvas:
                    edges.add((o, body))
        i += 1
    return edges, allcallees


# --- BASE graph (clang IR) -------------------------------------------------
def base_graph(m2rva, jobs=None):
    """({(caller_rva, callee_rva)}, base_defined:set[caller_rva],
       unresolved: caller_rva -> set[callee_mangled not in symbol_names], ir_failed:list).

    Compiles every src TU with a compile command to IR (fast, ~0.2s each) and reads
    every `call`/`invoke` target. A callee mangled name that keys into symbol_names is a
    resolved edge; one that does NOT is kept as `unresolved` (the fake-view / external
    candidates)."""
    clang = os.environ.get("GRUNTZ_CLANG") or "clang"
    compdb = labels.load_compdb(str(REPO / "build/clangd/compile_commands.json"))
    tus = sorted(k for k in compdb if k.replace("\\", "/").split("/src/")[-1] and
                 "/src/" in k.replace("\\", "/") and k.endswith(".cpp"))
    work = [(clang, tu, compdb[tu]) for tu in tus]
    edges, defined, unresolved, failed = set(), set(), {}, []
    jobs = jobs or min(24, (os.cpu_count() or 4) * 2)
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as ex:
        for tu, res in ex.map(_tu_edges, work):
            if res is None:
                failed.append(os.path.relpath(tu, REPO))
                continue
            for caller, callees in res.items():
                cr = m2rva.get(caller)
                if cr is None:
                    continue
                defined.add(cr)
                for callee in callees:
                    ce = m2rva.get(callee)
                    if ce is not None:
                        if ce != cr:
                            edges.add((cr, ce))
                    else:
                        unresolved.setdefault(cr, set()).add(callee)
    return edges, defined, unresolved, failed


# --- reconcile -------------------------------------------------------------
class Recon:
    def __init__(self, jobs=None):
        self.rva2sym, self.m2rva, self.func_rvas = load_symbols()
        d, secs = xref._load()
        self.names, self.byname, self.fstarts, self.fsize = xref._names()
        self.tgt, self.allcallees = target_edges(
            d, secs, self.names, self.fstarts, self.fsize, self.func_rvas)
        self.base, self.base_defined, self.unresolved, self.ir_failed = base_graph(
            self.m2rva, jobs)
        self.r2f = _rva2file()

    def name(self, rva):
        s = self.rva2sym.get(rva)
        if s:
            return s[0]
        return self.names.get(rva, (f"FUN_{rva:x}", ""))[0]

    def unit(self, rva):
        s = self.rva2sym.get(rva)
        return s[1] if s else self.names.get(rva, ("", "?"))[1]

    def is_stub(self, rva):
        return self.r2f.get(rva, "").startswith("src/Stub/")

    def classify(self, F, R):
        """cause of a missing edge (F,R) + the fake-view name(s) if any.

        FAKE-VIEW tie is by REGULAR method name only: F emits an unresolved `?M@V@@`
        whose bare method M equals R's method. Special members (ctor/dtor/operator)
        share a generic token and are NOT tieable this way (that produced a flood of
        false dtor->dtor matches), so a missing special-member edge is MISSING-SPECIAL."""
        if F not in self.base_defined:
            return ("STUB" if self.is_stub(F) else "UNANALYZED", None, None)
        rmember = self.rva2sym.get(R, (None, None, None, None))[3]
        if rmember in _SPECIAL or not rmember:
            return ("MISSING-SPECIAL" if rmember in _SPECIAL else "MISSING", None, None)
        views = []
        for mangled in self.unresolved.get(F, ()):
            pm = parse_mangled(mangled)
            if not pm:
                continue
            vcls, vmember, _ = pm
            if vmember == rmember and vmember not in _SPECIAL and vcls:
                views.append((vcls, mangled))
        if views:
            uniq = sorted(set(v for v, _ in views))
            view = uniq[0] if len(uniq) == 1 else "|".join(uniq)
            emitted = views[0][1]
            return ("FAKE-VIEW", view, emitted)
        return ("MISSING", None, None)

    def missing(self):
        """[(F, R, cause, view, emitted)] retail edges our IR does not emit."""
        out = []
        for (F, R) in self.tgt:
            if (F, R) in self.base:
                continue
            cause, view, emitted = self.classify(F, R)
            out.append((F, R, cause, view, emitted))
        return out

    def sig_kind(self, emitted, R):
        """Classify a FAKE-VIEW mismatch by comparing the emitted view-method mangling
        to the real callee's mangling (everything after `@@`): RETURN-ONLY (safe,
        matching-neutral to fix - only the return type / class-vs-struct keyword /
        access char differs, args identical), ARG-POINTEE (args push-byte-identical -
        same count, differences only in a pointer's pointee type or a V<->U keyword -
        also matching-neutral to retype), or ARG-COUNT (arg count or a value-type size
        differs - needs disasm verification before changing)."""
        real = self.rva2sym.get(R, (None,))[0]
        if not emitted or not real:
            return "UNKNOWN"
        es = emitted.split("@@", 1)[-1]
        rs = real.split("@@", 1)[-1]
        if es == rs:
            return "RETURN-ONLY"          # identical sig -> only the CLASS name differs
        ea, ra = _args_of(es), _args_of(rs)
        if ea == ra:
            return "RETURN-ONLY"          # same args, differ only in return/access
        if args_neutral(es, rs):
            return "ARG-POINTEE"          # args push-identical (pointer-pointee / keyword) -> byte-neutral, mine
        return "ARG-COUNT"                # arg count / value-size differs -> needs disasm (Agent A)

    def extra(self):
        """[(F, R)] our IR emits, retail lacks (both reconstructed)."""
        return [(F, R) for (F, R) in self.base
                if (F, R) not in self.tgt and F in self.func_rvas and R in self.func_rvas]


def _demangle_member(mangled):
    pm = parse_mangled(mangled)
    return pm[1] if pm else None


# minimal MSVC type-code tokenizer, enough to split a member-function signature's
# RETURN token from its ARG list so a return-type-only mismatch (safe, matching-
# neutral to fix) is told apart from an arg-count/type mismatch (needs disasm).
_PRIM = set("XDEFGHIJKMNOZ")  # void/char/.../double/... (single-char primitives)


def _tok(s, i):
    """(token, next_i) for one MSVC type at s[i]; (None, i) if unparseable."""
    if i >= len(s):
        return None, i
    c = s[i]
    if c == "_":                       # extended primitive (_N bool, _J int64, ...)
        return s[i:i + 2], i + 2
    if c in _PRIM or c.isdigit():       # primitive or 0-9 backref
        return c, i + 1
    if c in "PQRAS":                    # pointer/ref: <ptr><cv><pointee>
        _t, j = _tok(s, i + 2)          # skip ptr char + cv char
        return (None, i) if _t is None else (s[i:j], j)
    if c in "VUTW":                     # named class/struct/union/enum: V<name>@@
        e = s.find("@@", i)
        if e < 0:
            return None, i
        return s[i:e + 2], e + 2
    if c == "?":                        # cv-qualified / by-value return: ?<cv><type>
        _t, j = _tok(s, i + 2)
        return (None, i) if _t is None else (s[i:j], j)
    return None, i


def _args_of(sig):
    """The arg-list portion of a post-`@@` member signature (return token removed)."""
    body = sig[:-2] if sig.endswith("@Z") else (sig[:-1] if sig.endswith("Z") else sig)
    body = body[3:] if len(body) > 3 else body   # strip access(1)+callconv(2), e.g. QAE
    tok, j = _tok(body, 0)
    if tok is None:
        return None
    return body[j:]


def _tokenize(argstr):
    """List of MSVC type tokens for an arg-list string (None if unparseable)."""
    if argstr is None:
        return None
    out, i = [], 0
    n = len(argstr)
    while i < n:
        if argstr[i] == "@":                 # trailing @Z remnant
            break
        t, j = _tok(argstr, i)
        if t is None or j <= i:
            return None
        out.append(t)
        i = j
    return out


_PTR_PREFIX = "PQRAS"  # pointer/reference lead chars - a pointer arg is 4 bytes regardless of pointee


def _slot_neutral(a, b):
    """Two arg tokens are PUSH-byte-identical: equal, or both pointers/refs (any pointee),
    or the same primitive differing only in the class-vs-struct keyword (V<->U in a named
    type). NOT neutral if one is a value type and the other a pointer, or the value sizes
    differ (short F vs int H)."""
    if a == b:
        return True
    if a[0] in _PTR_PREFIX and b[0] in _PTR_PREFIX:
        return True                          # pointer arg: 4-byte push either way
    if a.replace("V", "U") == b.replace("V", "U"):
        return True                          # class/struct keyword only
    return False


def args_neutral(sig_a, sig_b):
    """True if two member signatures push IDENTICAL arg bytes: same arg count and every
    arg slot is push-neutral (pointer-pointee / keyword diffs only). This is the line
    between a byte-neutral shadow retype (mine) and an arg-count/value-size mismatch that
    needs disasm (Agent A)."""
    ta, tb = _tokenize(_args_of(sig_a)), _tokenize(_args_of(sig_b))
    if ta is None or tb is None or len(ta) != len(tb):
        return False
    return all(_slot_neutral(x, y) for x, y in zip(ta, tb))


def _rva2file():
    import re
    RVA = re.compile(r"\bRVA\w*\(\s*(0x[0-9a-fA-F]+)")
    r2f = {}
    for p in SRC.rglob("*.cpp"):
        try:
            txt = p.read_text(errors="replace")
        except OSError:
            continue
        for m in RVA.finditer(txt):
            r2f[int(m.group(1), 16)] = p.relative_to(REPO).as_posix()
    return r2f


# --- reporting -------------------------------------------------------------
def summarize(rc: Recon):
    miss = rc.missing()
    causes = {}
    for _F, _R, c, _v, _e in miss:
        causes[c] = causes.get(c, 0) + 1
    extra = rc.extra()
    return miss, causes, extra


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--metric", action="store_true", help="just the metric numbers")
    ap.add_argument("--worklist", action="store_true", help="FAKE-VIEW dissolution worklist")
    ap.add_argument("--check", help="explain one caller (rva or mangled name)")
    ap.add_argument("--csv", action="store_true", help="unreconciled edges to stdout")
    ap.add_argument("--md", action="store_true", help="write config/caller-callee-reconcile.md")
    ap.add_argument("--jobs", type=int, default=None)
    args = ap.parse_args()

    rc = Recon(args.jobs)

    if args.check:
        try:
            F = int(args.check, 16)
        except ValueError:
            F = rc.m2rva.get(args.check)
            if F is None:
                sys.exit(f"unknown {args.check}")
        _check(rc, F)
        return

    miss, causes, extra = summarize(rc)
    reconciled = len(rc.tgt) - len(miss)

    if args.csv:
        w = csv.writer(sys.stdout)
        w.writerow(["caller_rva", "caller", "callee_rva", "callee", "cause", "view",
                    "sig_kind", "emitted"])
        for F, R, c, v, e in sorted(miss):
            sk = rc.sig_kind(e, R) if c == "FAKE-VIEW" else ""
            w.writerow([f"0x{F:08x}", rc.name(F), f"0x{R:08x}", rc.name(R), c, v or "",
                        sk, e or ""])
        return

    print(f"[caller_callee] retail reconstructed<->reconstructed edges: {len(rc.tgt)}")
    print(f"  reconciled (emitted by our IR): {reconciled}")
    print(f"  UNRECONCILED (metric, drive to 0): {len(miss)}")
    for c in sorted(causes, key=lambda k: -causes[k]):
        print(f"      {causes[c]:5d}  {c}")
    print(f"  extra (our IR emits, retail lacks): {len(extra)}")
    print(f"  base IR: {len(rc.base_defined)} callers analysed, {len(rc.base)} edges, "
          f"{len(rc.ir_failed)} TU IR failures")

    if args.metric:
        return

    if args.worklist:
        fv = [(F, R, v, e) for F, R, c, v, e in miss if c == "FAKE-VIEW"]
        skc = {}
        for F, R, v, e in fv:
            skc[rc.sig_kind(e, R)] = skc.get(rc.sig_kind(e, R), 0) + 1
        print(f"\n== FAKE-VIEW unreconciled edges ({len(fv)}) ==")
        print("  by signature-mismatch kind (RETURN-ONLY + ARG-POINTEE = matching-neutral; "
              "ARG-COUNT needs disasm):")
        for k in sorted(skc, key=lambda x: -skc[x]):
            print(f"      {skc[k]:5d}  {k}")
        vcount = {}
        for _F, _R, v, _e in fv:
            vcount[v] = vcount.get(v, 0) + 1
        print("\n  by view fanout:")
        for v in sorted(vcount, key=lambda k: -vcount[k]):
            print(f"  view `{v}`  ({vcount[v]} edges):")
            for F, R, vv, e in sorted(fv):
                if vv != v:
                    continue
                C = rc.rva2sym.get(R, (None, None, None, None))[2]
                print(f"     [{rc.sig_kind(e, R):11s}] 0x{F:08x} {rc.name(F)} [{rc.unit(F)}]"
                      f"  -> 0x{R:08x} {rc.name(R)}  real-owner={C}")
        return

    # default: show the top MISSING (non-stub) edges as a worklist preview
    print("\n== top unreconciled edges (real-body callers first) ==")
    ranked = sorted(miss, key=lambda t: (t[2] not in ("FAKE-VIEW", "MISSING"), t[0]))
    for F, R, c, v, e in ranked[:40]:
        vs = f" via `{v}`" if v else ""
        print(f"  {c:10s} 0x{F:08x} {rc.name(F)} [{rc.unit(F)}] -> "
              f"0x{R:08x} {rc.name(R)}{vs}")

    if args.md:
        _write_md(rc, miss, causes, extra, reconciled)
        print(f"\nwrote {OUT_MD.relative_to(REPO)}")


def _check(rc: Recon, F):
    print(f"caller 0x{F:08x} {rc.name(F)} [{rc.unit(F)}]  analysed={F in rc.base_defined}")
    tgt = sorted(r for (f, r) in rc.tgt if f == F)
    base = sorted(r for (f, r) in rc.base if f == F)
    print(f"  retail callees (reconstructed, thunk-followed): {len(tgt)}")
    for R in tgt:
        ok = (F, R) in rc.base
        C = rc.rva2sym.get(R, (None, None, None, None))[2]
        tag = "OK" if ok else "MISSING"
        print(f"    [{tag:7s}] 0x{R:08x} {rc.name(R)}  owner={C}")
    ex = sorted(r for (f, r) in rc.extra() if f == F)
    if ex:
        print(f"  extra (our IR emits, retail lacks): {len(ex)}")
        for R in ex:
            print(f"    0x{R:08x} {rc.name(R)}")
    un = rc.unresolved.get(F, set())
    if un:
        print(f"  emitted-but-unresolved callee symbols ({len(un)}): "
              "(fake-view / external / unreconstructed)")
        for m in sorted(un)[:30]:
            print(f"    {m}")


def _write_md(rc, miss, causes, extra, reconciled):
    out = ["# caller<->callee reconciliation", "",
           f"retail reconstructed<->reconstructed edges: **{len(rc.tgt)}**  ",
           f"reconciled: **{reconciled}**  unreconciled (metric): **{len(miss)}**  "
           f"extra: **{len(extra)}**", "",
           "| cause | n |", "|---|--:|"]
    for c in sorted(causes, key=lambda k: -causes[k]):
        out.append(f"| {c} | {causes[c]} |")
    out += ["", "## FAKE-VIEW unreconciled edges (retype receiver / rename view -> real owner)",
            "", "| sig | caller | callee | real owner | view |", "|---|---|---|---|---|"]
    fv = sorted((t for t in miss if t[2] == "FAKE-VIEW"), key=lambda t: t[0])
    for F, R, c, v, e in fv:
        C = rc.rva2sym.get(R, (None, None, None, None))[2]
        out.append(f"| {rc.sig_kind(e, R)} | `{rc.name(F)}` | `{rc.name(R)}` "
                   f"| {C or ''} | {v or ''} |")
    OUT_MD.write_text("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
