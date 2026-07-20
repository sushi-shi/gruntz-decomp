#!/usr/bin/env python3
"""gruntz.match.vtable_slot_binding - every vtable SLOT must be WIRED to a real virtual.

THE GAP THIS CLOSES
-------------------
Two sibling gates already guard the vtable graph, and neither can see a WIRING defect:

  * ``vtable_coverage``   - every analysed vtable has a ``VTBL(Name, rva)`` binding.
  * ``vtable_virtuality`` - COUNTING only: the class + bases must declare *at least* N
                            virtuals for an N-slot vtable.

A wiring defect passes BOTH. The class declares the virtual (so the count is satisfied)
and the body exists in src/ (so it compiles and even matches) - but the body is bound at
the slot's RVA under a NON-virtual name, and *nothing joins the two*. The declared slot
and the real body sit a few lines apart in the same file, unconnected.

Proven real, found by hand: ``Gap_17f660`` / ``17ff30`` / ``180640`` / ``181b00`` were
``CFaderFlat/Sine/Light/Shape::RenderFrame`` - vtable slot 1 of each class - sitting as
FREE FUNCTIONS while the class declared the slot immediately above them. Four silent
wrong-dispatch bugs that every gate called green. A machine should find these.

THE JOIN
--------
Per vtable V bound ``VTBL(C, rva_v)``, per slot index i:

  LEFT   the retail slot's target RVA ``R_i``, from ``vtable_scan`` (stride-4 DIR32 reloc
         runs, exact per-vtable size). CRITICAL: a C++ slot points at a one-instruction
         ILT jmp-thunk in the incremental-link band, NOT at the method body - so the slot
         is resolved with ``chase_thunk`` before joining (``vtable_scan.iter_slots``).
  RIGHT  the symbol OUR source emits at ``R_i`` (build/gen/symbol_names.csv, generated
         from the ``RVA()`` annotations).
  ASSERT the symbol is a ``virtual`` of ``C`` - or of a base of ``C``, for an inherited
         slot.

Virtuality is read straight off the MANGLED name rather than re-parsed out of the source:
MSVC encodes the access+storage class in the char right after the ``@@`` qualifier
terminator, and virtual has its own codes (``E``/``M``/``U`` = private/protected/public
virtual; ``G``/``O``/``W`` = adjustor thunks). So ``?RenderFrame@CFaderFlat@@UAEXH@Z`` is
provably virtual and ``?Gap_17f660@@YAXXZ`` is provably a free function - as the compiler
itself emitted them, with no regex guess about what the class body says.

CLASSIFICATION
--------------
  (a) WIRING DEFECT  R_i binds a NON-virtual (free fn / Gap_* / non-virtual method /
                     static): the body exists but is not wired to the declared slot.
                     THE TARGET. Fail-closed.
  (b) MISBOUND       R_i binds a virtual of a class OUTSIDE C's base closure: the
                     binding, the hierarchy, or the RVA() is wrong. Fail-closed.
  (c) UNBOUND        nothing in src/ claims R_i - unreconstructed. Expected for the
                     stub backlog: reported as INFO, never a failure.

Library vtables (config/library_vtables.csv) are exempt, as in the sibling gates.
Pure-virtual slots (``__purecall``) are correct-by-construction and pass.

THE RATCHET (why this is not simply fail-closed today)
------------------------------------------------------
The first run found 259 pre-existing violations, so a bare fail-closed gate would brick
the build for everyone rather than protect anything. Instead the known backlog is frozen
in ``config/vtable-slot-binding-baseline.tsv`` and the gate is fail-closed on ANYTHING
NOT IN IT - so no NEW wiring defect can ever land, while the backlog drains. When the
baseline reaches 0 rows it is a pure fail-closed gate with no special case, exactly as
briefed.

The baseline is a SET, not a count (the campaign's "measure SETS, not counts" rule): it
is keyed on (vtable_rva, slot, symbol), so fixing one defect and introducing another
cannot net out to "still 259" and slip through - the new row simply is not in the set.
Fixing a defect makes its row stale, which ``--update`` prunes and the gate reports.

    python -m gruntz.match.vtable_slot_binding           # the gate (fail-closed vs baseline)
    python -m gruntz.match.vtable_slot_binding --strict  # ignore the baseline: ALL violations
    python -m gruntz.match.vtable_slot_binding --update  # bless the current set as baseline
    python -m gruntz.match.vtable_slot_binding --list    # every slot, every vtable
    python -m gruntz.match.vtable_slot_binding --info    # ... plus the (c) UNBOUND table
"""
from __future__ import annotations

import csv
import sys
from pathlib import Path

from gruntz.analysis import vtable_scan as vs
from gruntz.match.class_meta import rel, vtbl_annotations
from gruntz.match.vtable_virtuality import _index_classes

REPO = vs.REPO
LIB_CSV = REPO / "config" / "library_vtables.csv"
BASELINE = REPO / "config" / "vtable-slot-binding-baseline.tsv"
IB = vs.IMAGEBASE

# MSVC access+storage codes, taken at the char after the `@@` that terminates the
# name's qualifier list. Near codes are what 32-bit cl emits; the far twins are
# listed for completeness so an unexpected one is never silently mis-read.
VIRTUAL = set("EFMNUV")          # private / protected / public  virtual
VTHUNK = set("GHOPWX")           # ... adjustor thunks (still virtual dispatch)
NONVIRT_METHOD = set("ABIJQR")   # private / protected / public  non-virtual
STATIC_METHOD = set("CDKLST")    # ... static
FREE_FN = set("YZ")              # global (free) function

_KIND = [(VIRTUAL, "virtual"), (VTHUNK, "virtual-thunk"), (NONVIRT_METHOD, "non-virtual"),
         (STATIC_METHOD, "static"), (FREE_FN, "free function")]

# A slot legitimately holding a compiler/CRT symbol rather than a modelled virtual.
PURE = ("__purecall", "___purecall", "_purecall")


def split_mangled(sym):
    """``sym`` -> (name, [qualifiers], storage_char), or None if not a C++ symbol.

    Handles the three shapes a vtable slot can carry:
        ?Meth@Class@@UAEXH@Z    -> ("Meth",  ["Class"], "U")   ordinary method
        ??1Class@@UAE@XZ        -> ("??1",   ["Class"], "U")   dtor / other ?? special
        ?Foo@@YAXXZ             -> ("Foo",   [],        "Y")   free function
    Qualifiers are innermost-first, exactly as MSVC nests them (Class, then namespaces).
    """
    if not sym.startswith("?"):
        return None                                   # C symbol: _foo / _foo@8
    body = sym[1:]
    if body.startswith("?"):                          # `??`-prefixed special name
        rest = body[1:]
        if not rest:
            return None
        width = 2 if rest[0] == "_" else 1
        name, rest = "??" + rest[:width], "@" + rest[width:]
    else:
        i = body.find("@")
        if i < 0:
            return None
        name, rest = body[:i], body[i:]               # `rest` starts at that '@'
    j = rest.find("@@")
    if j < 0:
        return None
    quals = [q for q in rest[:j].split("@") if q]
    tail = rest[j + 2:]
    return name, quals, (tail[0] if tail else "")


def classify_storage(storage):
    for codes, label in _KIND:
        if storage in codes:
            return label
    return "unknown"


def load_symbols():
    """{rva: (mangled_name, unit)} for every FUNCTION our source binds."""
    out = {}
    p = REPO / "build/gen/symbol_names.csv"
    if not p.exists():
        return out
    with p.open() as f:
        for r in csv.DictReader(f):
            if (r.get("kind") or "").strip() != "func":
                continue
            try:
                out[int(r["rva"], 16)] = (r["name"], r.get("unit") or "")
            except (ValueError, KeyError):
                pass
    return out


def load_lib_rvas():
    rvas = set()
    if LIB_CSV.exists():
        with LIB_CSV.open() as f:
            for r in csv.DictReader(f):
                try:
                    rvas.add(int(r["rva"], 16))
                except (ValueError, TypeError, KeyError):
                    pass
    return rvas


def base_closure(name, classes, seen=None):
    """{name} u every base reachable from it in the SOURCE hierarchy (transitive).

    A slot may hold an INHERITED virtual - the body then belongs to a base, not to C -
    so the accepted set for C's vtable is C's whole ancestry, not C alone.
    """
    if seen is None:
        seen = set()
    if name in seen:
        return seen
    seen.add(name)
    for b in classes.get(name, (0, []))[1]:
        base_closure(b, classes, seen)
    return seen


def resolve_slot(symbols, raw, body):
    """The symbol our source binds for a slot, as (rva, name, unit) or None.

    The vtable reloc points at the RAW slot value, so a symbol defined THERE is the
    binding - honour it first. Only when raw is an anonymous linker ILT thunk (no
    symbol) do we chase to the BODY, where an ordinary reconstruction lands. This
    order also keeps a genuine tail-call virtual correct: e.g. slot 7 binds a real
    5-byte `Unload` (`jmp FreeAll`) at raw - the override IS Unload, not the FreeAll
    helper the jmp lands on. (Chasing body-first would mis-attribute it to FreeAll.)
    """
    for r in (raw, body):
        if r in symbols:
            return (r, *symbols[r])
    return None


def analyse():
    """-> (violations, unbound, checked_vtables, checked_slots).

    violations: [(kind, rva_v, cls, slot, r, sym, unit, why, path, ln)] for (a)/(b).
    unbound:    [(rva_v, cls, slot, r, path, ln)] for (c).
    """
    symbols = load_symbols()
    classes = _index_classes()
    lib = load_lib_rvas()

    violations, unbound = [], []
    n_vt = n_slots = 0
    for cls, rva_v, path, ln in sorted(vtbl_annotations(), key=lambda a: a[1]):
        norm = rva_v - IB if rva_v >= IB else rva_v
        if norm in lib:
            continue
        vt = vs.vtable_at(norm)
        if vt is None:
            continue                     # not a discovered vtable: vtable_coverage's job
        n_vt += 1
        allowed = base_closure(cls, classes)
        for k, _sr, raw, body in vs.iter_slots(vt):
            n_slots += 1
            hit = resolve_slot(symbols, raw, body)
            if hit is None:
                unbound.append((norm, cls, k, body, path, ln))
                continue
            r, sym, unit = hit
            if sym in PURE:
                continue                 # pure-virtual slot: correct by construction
            parts = split_mangled(sym)
            if parts is None:
                violations.append(("WIRING", norm, cls, k, r, sym, unit,
                                   "C-linkage symbol at a vtable slot (not a virtual)",
                                   path, ln))
                continue
            _name, quals, storage = parts
            kind = classify_storage(storage)
            if kind not in ("virtual", "virtual-thunk"):
                violations.append(("WIRING", norm, cls, k, r, sym, unit,
                                   f"bound as a {kind}, not a virtual", path, ln))
                continue
            owner = quals[0] if quals else "?"
            if owner not in allowed:
                violations.append(("MISBOUND", norm, cls, k, r, sym, unit,
                                   f"virtual of {owner}, not in {cls}'s base closure",
                                   path, ln))
    return violations, unbound, n_vt, n_slots


def key_of(v):
    """The baseline identity of a violation: (vtable_rva, slot, symbol).

    Deliberately NOT the count and NOT the body rva: keyed this way, fixing one defect
    while introducing another cannot cancel out - the new (vtable, slot, symbol) triple
    simply is not in the frozen set, so it fails. Re-spelling a symbol at a slot is a
    NEW row too, which is correct: it is a different binding claim.
    """
    _kind, rva_v, _cls, slot, _r, sym, _unit, _why, _path, _ln = v
    return (rva_v, slot, sym)


def load_baseline():
    """{(vtable_rva, slot, symbol)} - the frozen, known backlog. Missing file = empty
    set = a pure fail-closed gate."""
    out = set()
    if not BASELINE.exists():
        return out
    with BASELINE.open() as f:
        # Strip the `#` banner first: csv.DictReader would take a comment line as the
        # header row and every lookup below would silently miss (the whole baseline
        # would read as empty, and the gate would flag the entire frozen backlog).
        rows = [ln for ln in f if not ln.lstrip().startswith("#")]
    for row in csv.DictReader(rows, delimiter="\t"):
        try:
            out.add((int(row["vtable_rva"], 16), int(row["slot"]), row["symbol"]))
        except (ValueError, KeyError, TypeError):
            pass
    return out


def write_baseline(violations):
    BASELINE.parent.mkdir(parents=True, exist_ok=True)
    with BASELINE.open("w", newline="") as f:
        f.write("# gruntz.match.vtable_slot_binding - the FROZEN backlog of vtable slots whose\n"
                "# body is bound under a non-virtual / wrong-class name (see the module docstring).\n"
                "# The gate fails on any violation NOT listed here, so no NEW wiring defect can land.\n"
                "# Drive these to 0 by wiring each body to its class's declared virtual; re-bless\n"
                "# with `python -m gruntz.match.vtable_slot_binding --update`.\n")
        w = csv.writer(f, delimiter="\t")
        w.writerow(["kind", "vtable_rva", "class", "slot", "body_rva", "symbol", "unit"])
        for v in sorted(violations, key=lambda x: (x[1], x[3])):
            kind, rva_v, cls, slot, r, sym, unit, _why, _path, _ln = v
            w.writerow([kind, f"0x{rva_v:06x}", cls, slot, f"0x{r:06x}", sym, unit])


def _print_list():
    symbols = load_symbols()
    lib = load_lib_rvas()
    for cls, rva_v, _path, _ln in sorted(vtbl_annotations(), key=lambda a: a[1]):
        norm = rva_v - IB if rva_v >= IB else rva_v
        if norm in lib:
            continue
        vt = vs.vtable_at(norm)
        if vt is None:
            continue
        print(f"\n# VTBL({cls}, 0x{norm:06x})  slots={vt['size']}  {vt['conf'] if 'conf' in vt else vs.confidence(vt)}")
        for k, _sr, raw, body in vs.iter_slots(vt):
            hit = resolve_slot(symbols, raw, body)
            if hit is None:
                print(f"  slot[{k:2}] 0x{body:06x}  (unbound)  {vs.fn_label(body)}")
                continue
            r, sym, _unit = hit
            parts = split_mangled(sym)
            kind = classify_storage(parts[2]) if parts else "C symbol"
            via = "  via-thunk" if r != body else ""
            print(f"  slot[{k:2}] 0x{r:06x}  {kind:<14} {sym}{via}")


def _report(violations, stream=sys.stderr):
    for kind, rva_v, cls, k, r, sym, unit, why, path, ln in violations:
        print(f"  [{kind:8}] 0x{r:06x}  {cls}[{k}]  {sym}  ({unit})\n"
              f"             {why}   vtable 0x{rva_v:06x}  ({rel(path)}:{ln})", file=stream)


def main() -> int:
    if "--list" in sys.argv:
        _print_list()
        return 0

    violations, unbound, n_vt, n_slots = analyse()

    if "--update" in sys.argv:
        write_baseline(violations)
        print(f"vtable-slot-binding: blessed {len(violations)} known violation(s) -> "
              f"{rel(BASELINE)}")
        return 0

    if "--info" in sys.argv and unbound:
        print(f"vtable-slot-binding: {len(unbound)} slot(s) UNBOUND (unreconstructed - info only):")
        for rva_v, cls, k, r, path, ln in unbound:
            print(f"  0x{r:06x}  {cls}[{k}]  {vs.fn_label(r)}   ({rel(path)}:{ln})")
        print()

    strict = "--strict" in sys.argv
    base = set() if strict else load_baseline()
    fresh = [v for v in violations if key_of(v) not in base]
    known = [v for v in violations if key_of(v) in base]
    stale = base - {key_of(v) for v in violations}   # baselined rows now fixed

    if fresh:
        wiring = [v for v in fresh if v[0] == "WIRING"]
        mis = [v for v in fresh if v[0] == "MISBOUND"]
        print(f"vtable-slot-binding: {len(fresh)} NEW slot(s) NOT wired to a real virtual "
              f"({len(wiring)} WIRING DEFECT, {len(mis)} MISBOUND) "
              f"across {n_vt} vtable(s) / {n_slots} slot(s):", file=sys.stderr)
        _report(fresh)
        print("\n  A WIRING DEFECT means the body EXISTS but is not the class's virtual: the\n"
              "  class declares the slot and the body sits beside it under a non-virtual name,\n"
              "  so the vtable's reloc dangles onto a symbol that is not the override. Make the\n"
              "  body the class's declared virtual at that slot (byte-neutral when the body is\n"
              "  unchanged). NEVER fabricate a virtual for a slot you have not proven - read the\n"
              "  slot map (gruntz.analysis.vtable_scan --dump / --holds) instead.\n"
              f"  If a row here is a deliberate, proven exception, bless it into {rel(BASELINE)}\n"
              "  with --update (and say why in the commit).", file=sys.stderr)
        return 1

    if stale:
        print(f"vtable-slot-binding: {len(stale)} baselined violation(s) FIXED - re-bless with "
              f"`python -m gruntz.match.vtable_slot_binding --update` to keep the ratchet tight")
    if known:
        print(f"vtable-slot-binding: no new wiring defects; {len(known)} known violation(s) "
              f"remain in the frozen backlog ({rel(BASELINE)})")
        return 0

    print(f"vtable-slot-binding: all {n_slots} bound slot(s) across {n_vt} vtable(s) "
          f"wired to real virtuals ({len(unbound)} unbound/unreconstructed)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
