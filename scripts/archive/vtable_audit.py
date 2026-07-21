#!/usr/bin/env python3
"""gruntz.analysis.vtable_audit - the PER-SLOT function-RVA virtuality census.

Where the existing vtable checks work at the CLASS level (vtable_coverage: is each
retail vtable bound by a VTBL()/??_7? / vtable_virtuality: does the bound class carry
>= N virtual decls? / vtable_hierarchy --audit: inheritance + override-macro slot
counts), this tool works at the SLOT level: it walks EVERY slot of EVERY retail vtable
(vtable_scan enumerates them from PE base-relocs + RTTI, chasing ILT thunks to the
method body) and asks, for the exact function RVA that occupies each slot:

  1. is that function present in our source and bound by an ``RVA()`` (in
     build/gen/symbol_names.csv)?
  2. is it declared a real C++ ``virtual`` (the MSVC-emitted symbol's access code /
     the llvm-undname "virtual" tag), not a plain method and not a free function?
  3. is it in the CORRECT class (self or a spine ancestor of the vtable's owner)?

and categorizes the discrepancies:

  [A] SLOT-FN-NOT-VIRTUAL  the slot's function is reconstructed & bound, but declared a
                           plain (non-virtual) method or a free function - it SHOULD be a
                           ``virtual`` (this is the manual-vtable / de-virtualized residue).
  [B] OVER-MODELED         a source method emitted ``virtual`` whose RVA occupies NO retail
                           vtable slot at all (a fabricated virtual - demote to a method).
  [C] WRONG-CLASS          the slot's function is a virtual, but its declaring class is not
                           the vtable owner nor any ancestor on the owner's RTTI spine.
  [D] MISSING              the slot's function is not reconstructed / not bound anywhere.

Library vtables (config/library_vtables.csv - statically-linked MFC/CRT) are excluded from
[A]/[C]/[D] (we model them minimally, never reconstruct), but their slots ARE counted in
the "occupies a retail slot" set so a legit override of an MFC virtual is not flagged [B].

IMPORTANT - what [A] really means (the proxy pattern). vtable_coverage + vtable_virtuality
are green: every source-bound vtable already has ENOUGH virtual decls to fill its slots. So
an [A] slot's SLOT is almost always ALREADY realized - by a declared-only PROXY virtual
(a ``VtSlotFill`` filler, or a repurposed ``virtual Create() OVERRIDE`` whose retail target
is the slot fn) that reloc-masks the retail slot. The [A] flag fires because the slot's
FUNCTION BODY is ALSO reconstructed, under a plain non-virtual name - deliberately, because
in retail that same address is BOTH the vtable slot AND direct-called by callers (a virtual
can be direct-called), and modeling it non-virtual keeps those direct callers byte-matching.
So [A] is NOT "unrealized slots"; it is the worklist to model the REAL function AS the class's
virtual (dissolving the proxy filler + re-homing A-other bodies) - a per-class re-model that
is NOT matching-neutral (it changes caller dispatch + the ctor vptr boundary), so each needs
a build-verify + revert-on-regress, and many owners overlap live homing workers. A-self =
body in the owner class (fewest moving parts); A-other = body under a sibling/placeholder
class (COMDAT fold or re-home first).

Usage:
    python3 -m gruntz.analysis.vtable_audit                 # census summary + per-category worklist
    python3 -m gruntz.analysis.vtable_audit --csv out.csv   # per-slot machine-readable rows
    python3 -m gruntz.analysis.vtable_audit --category A    # only [A] (or B/C/D) rows, full detail
    python3 -m gruntz.analysis.vtable_audit --class CFoo    # one class / vtable-owner (substring)
    python3 -m gruntz.analysis.vtable_audit --realizable    # [A] vtables whose EVERY own slot is
                                                            #   reconstructed (ready to make poly)
"""
from __future__ import annotations

import argparse
import csv
import re
import shutil
import subprocess
import sys
from collections import defaultdict

from gruntz.analysis import vtable_scan as vs
from gruntz.analysis import vtable_hierarchy as vh
from gruntz.match import class_meta

IB = vs.IMAGEBASE
REPO = vs.REPO

# vtable_scan confidences that ARE real vtables (excludes 'unref' = EH / switch tables).
REAL_CONF = {"rtti", "code-ref", "code-ref-weak"}


# ---------------------------------------------------------------------------
# our reconstructed symbols: rva -> (mangled_name, unit, kind)
# ---------------------------------------------------------------------------
def load_symbols():
    m = {}
    p = REPO / "build/gen/symbol_names.csv"
    if p.exists():
        with open(p) as f:
            for r in csv.DictReader(f):
                try:
                    m[int(r["rva"], 16)] = (r["name"], r.get("unit", ""), r.get("kind", ""))
                except (ValueError, KeyError):
                    pass
    return m


def library_vtable_rvas():
    out = set()
    p = REPO / "config/library_vtables.csv"
    if p.exists():
        for r in csv.reader(p.open()):
            if len(r) >= 2:
                try:
                    out.add(int(r[1], 16))
                except ValueError:
                    pass
    return out


def library_fn_rvas():
    """RVAs of statically-linked MFC/CRT library FUNCTION bodies (FID matches). A [D]
    slot whose body is one of these is an INHERITED library virtual (an MFC CWnd/CDialog
    method), NOT a game gap we are expected to reconstruct - split it out of [D]."""
    out = set()
    p = REPO / "config/library_labels.csv"
    if p.exists():
        for r in csv.DictReader(p.open()):
            try:
                out.add(int(r["rva"], 16))
            except (ValueError, KeyError):
                pass
    return out


def source_vtbl_owner():
    """rva -> class name from every VTBL(name, rva) in src/+include/ (non-RTTI owners)."""
    out = {}
    for name, rva, _p, _l in class_meta.vtbl_annotations():
        out.setdefault(rva, name)
    return out


# ---------------------------------------------------------------------------
# demangle every relevant symbol once (llvm-undname, authoritative). Falls back to
# the MSVC access-code heuristic if llvm-undname is not on PATH.
# ---------------------------------------------------------------------------
_CALLCONV = re.compile(r"__(?:thiscall|cdecl|stdcall|fastcall|vectorcall|clrcall)\s+(.*)$")
# MSVC member-function access codes: E F M N U V = virtual; G H O P W X = virtual thunk.
_VIRT_CODES = set("EFMNUVGHOPWX")
_MEMBER_CODES = set("ABEFGHIJMNOPQRUVWX")  # near member functions (static S/T excluded)


def _access_code(mangled):
    """The MSVC member-function access/storage char (right after the qualifier `@@`),
    or None for a free function / data / un-parseable name. Fallback for no-undname."""
    if not mangled.startswith("?"):
        return None
    i = mangled.find("@@")
    if i < 0 or i + 2 >= len(mangled):
        return None
    return mangled[i + 2]


def parse_demangled(s):
    """(is_virtual, cls_inner, kind) from an llvm-undname line. kind in
    {method, free, data, other}. cls_inner is the innermost enclosing class or None."""
    if not s or s.startswith("error:"):
        return None
    is_virtual = bool(re.search(r"\bvirtual\b", s))
    head = s.split("(", 1)[0]
    m = _CALLCONV.search(head)
    if not m:
        # no calling convention -> data symbol ("int g_x") or an odd form
        return (is_virtual, None, "data")
    qual = m.group(1).strip()
    if "::" in qual:
        cls_full = qual.rsplit("::", 1)[0]
        return (is_virtual, cls_full.rsplit("::", 1)[-1], "method")
    return (is_virtual, None, "free")


def demangle_all(names):
    """{mangled: (is_virtual, cls_inner, kind)} for every name via llvm-undname (batched)."""
    out = {}
    tool = shutil.which("llvm-undname")
    names = [n for n in names if n and n.startswith("?")]
    if tool and names:
        proc = subprocess.run([tool], input="\n".join(names) + "\n",
                              capture_output=True, text=True)
        lines = proc.stdout.splitlines()
        nameset = set(names)
        i = 0
        while i < len(lines):
            ln = lines[i]
            if ln in nameset and i + 1 < len(lines):
                parsed = parse_demangled(lines[i + 1])
                if parsed is not None:
                    out[ln] = parsed
                i += 2
            else:
                i += 1
    # fallback / fill gaps with the access-code heuristic
    for n in names:
        if n in out:
            continue
        c = _access_code(n)
        if c is None:
            out[n] = (False, None, "free" if n.startswith("?") else "other")
        else:
            out[n] = (c in _VIRT_CODES, None, "method" if c in _MEMBER_CODES else "free")
    return out


# ---------------------------------------------------------------------------
# audit core
# ---------------------------------------------------------------------------
PURE = "__purecall"


class Slot:
    __slots__ = ("vt", "owner", "is_rtti", "k", "body", "sym", "gh",
                 "cat", "detail", "cls")

    def __init__(self, vt, owner, is_rtti, k, body):
        self.vt, self.owner, self.is_rtti, self.k, self.body = vt, owner, is_rtti, k, body
        self.sym = self.gh = self.cls = None
        self.cat = self.detail = ""


def build_audit():
    SYM = load_symbols()
    LIB = library_vtable_rvas()
    LIBFN = library_fn_rvas()
    SRC_VTBL = source_vtbl_owner()
    reg, _ = vh.build_registry()

    # confidence-tag every scanned vtable
    for v in vs.VTABLES:
        v["conf"] = vs.confidence(v)

    real_vts = [v for v in vs.VTABLES if v["conf"] in REAL_CONF]

    # ------- the full "occupies a retail slot" body set (ALL vtables, incl. library) -------
    slot_bodies = set()
    for v in real_vts:
        for _k, _sr, raw, body in vs.iter_slots(v):
            slot_bodies.add(body)
            slot_bodies.add(raw)

    # ------- gather every mangled name we must demangle (slot bodies + all SYM funcs) -------
    want = set()
    for v in real_vts:
        for _k, _sr, _raw, body in vs.iter_slots(v):
            s = SYM.get(body)
            if s:
                want.add(s[0])
    for rva, (nm, _u, kind) in SYM.items():
        if kind == "func":
            want.add(nm)
    DM = demangle_all(want)

    def owner_of(v):
        if v["rtti"] and v["base_off"] in (0, None):
            return v["rtti"], True
        if v["rtti"]:
            return v["rtti"], True   # MI secondary: still the derived class
        return SRC_VTBL.get(v["start"]), False

    def spine_set(owner):
        ci = reg.get(owner)
        if not ci:
            return None
        return {owner} | set(vh.spine_names(ci))

    # ------- per-slot categorization for GAME (non-library) vtables -------
    slots = []
    for v in sorted(real_vts, key=lambda v: v["start"]):
        if v["start"] in LIB:
            continue
        owner, is_rtti = owner_of(v)
        sp = spine_set(owner) if owner else None
        for k, _sr, _raw, body in vs.iter_slots(v):
            sl = Slot(v, owner, is_rtti, k, body)
            gh = vs.FN.get(body)
            sl.gh = gh[0] if gh else None
            sl.sym = SYM.get(body)
            if sl.sym is None and vs.chase_thunk(body) is not None:
                # a slot pointing at a thunk band that resolves elsewhere; already chased
                pass
            # categorize
            if sl.gh == PURE or (sl.sym and sl.sym[0] == PURE):
                sl.cat = "OK-pure"
            elif sl.sym is None:
                # [D-lib]: an inherited statically-linked MFC/CRT virtual (not our gap);
                # [D]: a genuine game/engine function we have not reconstructed.
                sl.cat = "D-lib" if body in LIBFN else "D"
                sl.detail = sl.gh or ("sub_%06x" % body)
            else:
                mangled = sl.sym[0]
                dm = DM.get(mangled)
                is_virtual, cls, kind = dm if dm else (False, None, "?")
                sl.cls = cls
                if is_virtual:
                    if sp is not None and cls and cls not in sp:
                        sl.cat = "C"
                        sl.detail = "%s (owner spine: %s)" % (cls, ",".join(sorted(sp)))
                    else:
                        sl.cat = "OK-virtual"
                elif kind == "free":
                    sl.cat = "A"
                    sl.detail = "free-fn " + mangled
                else:
                    sl.cat = "A"
                    # A-self: the slot fn is declared in the owner class itself (a genuine
                    # "make this method virtual" candidate). A-other: reconstructed under a
                    # DIFFERENT / placeholder class - a COMDAT fold or a not-yet-re-homed
                    # slot method, which realizing needs to re-home first.
                    sl.detail = ("[self] " if cls and owner and cls == owner else "[other] ") \
                        + "non-virtual method " + mangled
            slots.append(sl)

    # ------- [B] over-modeled: SYM virtual whose rva occupies no retail slot -------
    # A non-deleting destructor (??1) is virtual in source but its OWN rva is never a
    # vtable slot - the slot holds the scalar/vector DELETING dtor (??_G / ??_E), which
    # tail-calls ??1. So ??1 not being a slot body is expected, NOT over-modeling; skip it.
    over = []
    for rva, (nm, unit, kind) in sorted(SYM.items()):
        if kind != "func":
            continue
        if nm.startswith("??1"):
            continue
        dm = DM.get(nm)
        if dm and dm[0] and rva not in slot_bodies:
            over.append((rva, nm, unit))

    return dict(SYM=SYM, LIB=LIB, DM=DM, reg=reg, real_vts=real_vts,
                slots=slots, over=over, slot_bodies=slot_bodies)


# ---------------------------------------------------------------------------
# reporting
# ---------------------------------------------------------------------------
def _vt_label(v, owner):
    cls = owner or (v["rtti"] or vs.fn_label(v["first"]))
    return "0x%06x %-13s sz=%-3d %s" % (v["start"], v["conf"], v["size"], cls)


def cmd_summary(A, want_class=None):
    slots = A["slots"]
    by_cat = defaultdict(int)
    for s in slots:
        by_cat[s.cat] += 1
    # per-vtable rollup
    vt_slots = defaultdict(list)
    for s in slots:
        vt_slots[s.vt["start"]].append(s)

    total = len(slots)
    print("# PER-SLOT VTABLE VIRTUALITY CENSUS (game/engine vtables; library excluded)")
    print("# retail vtable slots audited : %d  across %d game vtables"
          % (total, len(vt_slots)))
    print("#   OK-virtual : %-5d  (slot fn reconstructed AND a real virtual, right class)"
          % by_cat["OK-virtual"])
    print("#   OK-pure    : %-5d  (__purecall pure-virtual slot)" % by_cat["OK-pure"])
    a_self = sum(1 for s in slots if s.cat == "A" and s.detail.startswith("[self]"))
    a_other = by_cat["A"] - a_self
    print("#   [A] not-virt : %-5d (slot fn body reconstructed non-virtual; slot itself usually"
          % by_cat["A"])
    print("#                       PROXY-realized by a declared-only filler - model the fn AS the virtual)")
    print("#       A-self   : %-5d  (slot fn declared in the owner class - fewest moving parts)"
          % a_self)
    print("#       A-other  : %-5d  (slot fn under a sibling/placeholder class - COMDAT fold or re-home first)"
          % a_other)
    print("#   [C] wrong-cls: %-5d (virtual, but declaring class off the owner's spine)"
          % by_cat["C"])
    print("#   [D] missing  : %-5d (GAME slot fn not reconstructed / not bound)" % by_cat["D"])
    print("#   D-lib      : %-5d  (inherited MFC/CRT library virtual - expected, not a gap)"
          % by_cat["D-lib"])
    print("#   [B] over-modeled (virtual in src, in NO retail slot) : %d" % len(A["over"]))
    print()

    # vtables that carry [A]/[C]/[D] discrepancies, worst first
    problem = []
    for st, sl in vt_slots.items():
        bad = [s for s in sl if s.cat in ("A", "C", "D")]
        if bad:
            v = sl[0].vt
            owner = sl[0].owner
            problem.append((len(bad), v, owner, sl))
    problem.sort(key=lambda x: (-x[0], x[1]["start"]))
    if want_class:
        q = want_class.lower()
        problem = [p for p in problem if (p[2] or p[1]["rtti"] or "").lower().find(q) >= 0]
    print("# vtables with unrealized slots (owner :: #A #C #D of size) - realize worklist")
    for nbad, v, owner, sl in problem:
        na = sum(1 for s in sl if s.cat == "A")
        nc = sum(1 for s in sl if s.cat == "C")
        nd = sum(1 for s in sl if s.cat == "D")
        tag = "rtti" if sl[0].is_rtti else "src-vtbl" if owner else "NON-RTTI-UNBOUND"
        print("  %-34s [%-8s] A=%-2d C=%-2d D=%-2d / %-2d  vt@0x%06x"
              % (owner or ("(" + vs.fn_label(v["first"]) + ")"), tag, na, nc, nd, v["size"], v["start"]))


def cmd_category(A, cat, want_class=None):
    slots = [s for s in A["slots"] if s.cat == cat.upper()] if cat.upper() in ("A", "C", "D") else []
    print("# category [%s] rows" % cat.upper())
    if cat.upper() == "B":
        rows = A["over"]
        if want_class:
            q = want_class.lower()
            rows = [r for r in rows if q in r[1].lower()]
        for rva, nm, unit in rows:
            print("  0x%06x  %-40s  [%s]" % (rva, nm, unit))
        print("# %d over-modeled virtual(s)" % len(rows))
        return
    if want_class:
        q = want_class.lower()
        slots = [s for s in slots if (s.owner or s.vt["rtti"] or "").lower().find(q) >= 0]
    for s in sorted(slots, key=lambda s: (s.vt["start"], s.k)):
        owner = s.owner or ("(" + vs.fn_label(s.vt["first"]) + ")")
        print("  vt@0x%06x[%2d] %-30s -> 0x%06x  %s" % (s.vt["start"], s.k, owner, s.body, s.detail))
    print("# %d slot(s) in category [%s]" % (len(slots), cat.upper()))


def cmd_realizable(A):
    """[A]-bearing GAME vtables where EVERY slot is reconstructed (OK-virtual/OK-pure/[A],
    i.e. no [D] missing and no [C] wrong-class): these are ready to convert to real
    polymorphism (declare all slots virtual in order) with no reconstruction blocker."""
    vt_slots = defaultdict(list)
    for s in A["slots"]:
        vt_slots[s.vt["start"]].append(s)
    print("# REALIZABLE: [A]-bearing vtables with no [D]/[C] blocker (all slots reconstructed)")
    n = 0
    for st, sl in sorted(vt_slots.items()):
        cats = {s.cat for s in sl}
        na = sum(1 for s in sl if s.cat == "A")
        if na and not (cats & {"C", "D"}):
            v, owner = sl[0].vt, sl[0].owner
            tag = "rtti" if sl[0].is_rtti else "src-vtbl" if owner else "UNBOUND"
            print("  %-34s [%-8s] size=%-2d A=%-2d  vt@0x%06x"
                  % (owner or ("(" + vs.fn_label(v["first"]) + ")"), tag, v["size"], na, st))
            n += 1
    print("# %d realizable vtable(s)" % n)


def write_csv(A, path):
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["vtable_rva", "slot", "owner", "is_rtti", "fn_rva",
                    "category", "sym_name", "ghidra", "detail"])
        for s in A["slots"]:
            w.writerow(["0x%06x" % s.vt["start"], s.k, s.owner or "", int(s.is_rtti),
                        "0x%06x" % s.body, s.cat, s.sym[0] if s.sym else "",
                        s.gh or "", s.detail])
    print("# wrote %s (%d slot rows)" % (path, len(A["slots"])))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--csv", metavar="PATH", help="write the per-slot CSV")
    ap.add_argument("--category", metavar="A|B|C|D", help="dump only this category's rows")
    ap.add_argument("--class", dest="klass", metavar="NAME", help="restrict to one owner (substring)")
    ap.add_argument("--realizable", action="store_true",
                    help="[A]-bearing vtables with no [D]/[C] blocker (ready to make polymorphic)")
    args = ap.parse_args()

    A = build_audit()
    if args.csv:
        write_csv(A, args.csv)
        return
    if args.realizable:
        cmd_realizable(A)
        return
    if args.category:
        cmd_category(A, args.category, args.klass)
        return
    cmd_summary(A, args.klass)


if __name__ == "__main__":
    main()
