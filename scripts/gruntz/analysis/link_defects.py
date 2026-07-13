#!/usr/bin/env python3
"""link_defects.py - the two defect shapes that break a LINK, hunted tree-wide.

objdiff scores a relocation site by MASKING it: a reference bound to a fake symbol,
or a symbol defined twice with different bytes, still scores 100%. Both are invisible
to every match metric we have - and both make the reconstruction unlinkable. Retail
LINKED, so neither can exist in the real source.

  (1) UNRESOLVED  a symbol some obj REFERENCES that NO obj defines and NO shippable
                  .LIB can supply -> `unresolved external symbol` at link.
                  Typical cause: a fake per-TU VIEW base class whose declared-only
                  virtual dtor/method is a phantom (??1CSbConfigItem@@UAE@XZ).

  (2) DIVERGENT   one mangled name DEFINED in >1 obj with DIFFERENT bytes -> an ODR
                  landmine. MSVC keeps ONE COMDAT copy per name, chosen arbitrarily
                  (first-seen), so the linker may pick a 5-byte `jmp <view-dtor>`
                  stub OVER the real teardown body. Typical cause: a class whose dtor
                  is left IMPLICIT in a view header, so cl5 synthesises a per-TU inline
                  copy under the same name as the real out-of-line body.
                  (N objs emitting the SAME bytes is a normal COMDAT - not a defect.)

Ground truth, not heuristics: "can the linker resolve it" is answered by reading the
symbol tables of the real toolchain archives ($MSVC_DIR/lib/*.LIB + $DXSDK_DIR/Lib),
so CRT/MFC/Win32-import symbols are never mistaken for defects.

Usage:
  python -m gruntz.analysis.link_defects            # headline + both inventories
  python -m gruntz.analysis.link_defects --csv      # machine-readable worklist
  python -m gruntz.analysis.link_defects --unit sbi_image
"""
import argparse
import glob
import hashlib
import os
import re
import struct
import subprocess
import sys
from collections import defaultdict
from pathlib import Path


def _find_repo():
    # CWD first: in a worktree the shell PYTHONPATH can point at MAIN's scripts/
    # ([[worktree-pythonpath-leaks-to-main]]), so __file__ would mis-resolve to main.
    for base in (Path.cwd(), Path(__file__).resolve().parent):
        for p in (base, *base.parents):
            if (p / "flake.nix").exists() and (p / "build" / "objdiff").exists():
                return p
    return next(p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists())


REPO = _find_repo()

# Symbols the CRT/EH machinery or the compiler itself emits per-TU; they are either
# supplied by a lib or are local scaffolding, never a reconstruction defect.
_EXEMPT_PREFIX = (
    "__imp_", "$L", "$S", "$T", "??_C@", "__real@", "__xmm@",
    "__except_list", "__load_config", "__ehhandler", "?__L",
)


class Coff:
    """Minimal COFF reader: defined/undefined symbols + per-section content shape."""

    def __init__(self, path):
        b = self.b = open(path, "rb").read()
        self.path = path
        self.nsec = struct.unpack_from("<H", b, 2)[0]
        self.symptr = struct.unpack_from("<I", b, 8)[0]
        self.nsym = struct.unpack_from("<I", b, 12)[0]
        opt = struct.unpack_from("<H", b, 16)[0]
        self.strtab = self.symptr + self.nsym * 18
        self.secs = []
        for i in range(self.nsec):
            o = 20 + opt + i * 40
            rawsize, rawptr = struct.unpack_from("<II", b, o + 16)
            relptr = struct.unpack_from("<I", b, o + 24)[0]
            nrel = struct.unpack_from("<H", b, o + 32)[0]
            self.secs.append((relptr, nrel, rawptr, rawsize))

    def name(self, idx):
        base = self.symptr + idx * 18
        if struct.unpack_from("<I", self.b, base)[0] == 0:
            off = struct.unpack_from("<I", self.b, base + 4)[0]
            e = self.b.index(b"\0", self.strtab + off)
            return self.b[self.strtab + off:e].decode("latin1")
        return self.b[base:base + 8].split(b"\0")[0].decode("latin1")

    def _relocs(self, sec):
        relptr, nrel = self.secs[sec - 1][:2]
        out = []
        for r in range(nrel):
            ro = relptr + r * 10
            vaddr, symidx, typ = struct.unpack_from("<IIH", self.b, ro)
            out.append((vaddr, self.name(symidx), typ))
        return sorted(out)

    def shape(self, sec):
        """A link-relevant fingerprint of a section: its raw bytes plus every reloc as
        (offset, TARGET NAME, type). Two objs emitting the SAME COMDAT hash the same; a
        5-byte `jmp view_dtor` stub vs the real body does not.

        Compiler-LOCAL reloc targets are normalised away first. cl5 numbers its per-TU
        scaffolding ($L EH scope tables, $T/$S temporaries, ??_C@ string constants)
        sequentially per TU, so the SAME inline body emitted in two TUs legitimately
        references $L18650 in one and $L20123 in the other. Hashing those raw names
        would report every ordinary multi-TU COMDAT as divergent (they are not - the
        linker keeps one copy plus its own scaffolding). Only EXTERNAL targets - the
        real callees/vtables that decide what the code DOES - are identity-bearing."""
        _rp, _nr, rawptr, rawsize = self.secs[sec - 1]
        raw = self.b[rawptr:rawptr + rawsize] if rawptr else b""
        h = hashlib.md5()
        h.update(raw)
        for vaddr, nm, typ in self._relocs(sec):
            if nm.startswith(("$L", "$T", "$S", "??_C@", "__real@", "__xmm@")):
                nm = "<local>"
            h.update(b"|%d|%s|%d" % (vaddr, nm.encode("latin1"), typ))
        return h.hexdigest(), rawsize

    def symbols(self):
        """(defined {name: section}, undefined {names}). scl==2 is EXTERNAL; section 0
        with a nonzero value is a COMMON, with zero value an undefined import."""
        defined, undef = {}, set()
        i = 0
        while i < self.nsym:
            base = self.symptr + i * 18
            val, sec, _t, scl, naux = struct.unpack_from("<IhHBB", self.b, base + 8)
            nm = self.name(i)
            if scl == 2:
                if sec >= 1:
                    defined.setdefault(nm, sec)
                elif sec == 0 and val == 0:
                    undef.add(nm)
            i += 1 + naux
        return defined, undef


def current_unit_objs():
    """Only objs of CURRENT units - build/objdiff/base/ accumulates stale orphan objs
    from retired/renamed units, and a stale COMDAT would fake a divergence."""
    try:
        import tomllib
        units = {u["unit"] for u in
                 tomllib.loads((REPO / "config/units.toml").read_text()).get("unit", [])}
    except Exception:
        units = None
    out = []
    for p in sorted(glob.glob(str(REPO / "build/objdiff/base/*.obj"))):
        stem = os.path.basename(p)[:-4]
        if units is None or stem in units:
            out.append((stem, p))
    return out


def lib_symbols():
    """Every symbol the real toolchain archives can supply (CRT, MFC, Win32 imports,
    DirectX). Read straight out of the .LIB symbol tables - the linker's own answer to
    'can this resolve', so no CRT/MFC symbol is ever miscalled a defect."""
    cache = REPO / "build" / "gen" / "lib_symbols.txt"
    if cache.is_file():
        return set(cache.read_text(errors="ignore").split("\n"))
    libs = []
    for env in ("MSVC_DIR", "DXSDK_DIR"):
        d = os.environ.get(env)
        if d:
            for sub in ("lib", "Lib"):
                libs += glob.glob(os.path.join(d, sub, "*"))
    libs = [p for p in libs if p.lower().endswith(".lib")]
    syms = set()
    for i in range(0, len(libs), 24):                       # batch: llvm-nm is slow to spawn
        try:
            out = subprocess.run(["llvm-nm", "--defined-only", "--no-sort", *libs[i:i + 24]],
                                 capture_output=True, text=True, timeout=600).stdout
        except Exception:
            continue
        for line in out.split("\n"):
            f = line.split(" ", 2)
            if len(f) == 3 and f[1] not in ("", "U"):
                syms.add(f[2].strip())
            elif len(f) == 2 and f[0] in ("T", "D", "B", "R", "t"):
                syms.add(f[1].strip())
    cache.parent.mkdir(parents=True, exist_ok=True)
    cache.write_text("\n".join(sorted(syms)))
    return syms


def is_exempt(sym, libs):
    return sym.startswith(_EXEMPT_PREFIX) or sym in libs or sym.lstrip("_") in libs


_CLASS_RE = re.compile(r"^\?\?[01][A-Za-z_]\w*@@|^\?\w+@(\w+)@@")


def owning_class(sym):
    """The class a mangled member symbol belongs to (best effort, for grouping)."""
    m = re.match(r"^\?\?[01]([A-Za-z_]\w*)@@", sym)        # ctor/dtor: ??0Foo@@ / ??1Foo@@
    if m:
        return m.group(1)
    m = re.match(r"^\?[\w@]+?@([A-Za-z_]\w*)@@", sym)      # method:   ?Meth@Foo@@
    if m:
        return m.group(1)
    m = re.match(r"^\?\?_[7GE]([A-Za-z_]\w*)@@", sym)      # vtable / deleting dtors
    if m:
        return m.group(1)
    return ""


def real_classes():
    """Classes PROVEN to exist in retail: they own at least one symbol bound to a real
    retail RVA (symbol_names.csv) or a catalogued vtable. Used to split the unresolved
    set in two, because those two halves need opposite responses:

      BACKLOG  the class is real, this member just is not reconstructed yet. Every
               unreconstructed engine callee is deliberately declared-only so its
               rel32 reloc-masks - the campaign's standard technique. It resolves for
               free the day someone reconstructs it. NOT a modelling defect.

      PHANTOM  the owning class owns NO retail address anywhere. The class itself is a
               fabricated per-TU VIEW, so this member can NEVER resolve - retail has no
               such function to bind it to. THIS is the real link-blocker (the
               ??1CSbConfigItem shape): it must be fixed by modelling the real class,
               not by reconstructing anything.

    A class counts as REAL only if at least one of ITS OWN METHODS is bound to a retail
    rva in symbol_names.csv.

    A VTBL() binding deliberately does NOT count, and this used to be a hole that
    UNDERCOUNTED the metric. VTBL() only says the class's VTABLE has a retail address -
    it says nothing about whether any given declared-only METHOD NAME corresponds to a
    real retail function. A hand-rolled stand-in for a library class is exactly that
    case: <Gruntz/Wnd.h>'s CWnd carries VTBL(CWnd, 0x1eb5c4) (the real MFC vtable IS at
    that address) while every one of its ~26 methods is a fabricated WndVslN placeholder
    that no obj and no .LIB defines and that NOTHING can ever define, because the real
    body in NAFXCW.LIB is exported under a different mangled name. Those are phantoms in
    the strictest sense, and the VTBL() was laundering them into the "backlog" bucket."""
    real = set()
    p = REPO / "build/gen/symbol_names.csv"
    if p.is_file():
        import csv as _csv
        for r in _csv.DictReader(open(p)):
            c = owning_class(r["name"])
            if c:
                real.add(c)
    return real


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--csv", action="store_true")
    ap.add_argument("--unit", help="restrict the report to defects touching this unit")
    ap.add_argument("--phantom-only", action="store_true",
                    help="only unresolved symbols of classes with NO retail address "
                         "(the can-never-link set); hides the reconstruct-me backlog")
    args = ap.parse_args()

    libs = lib_symbols()
    objs = current_unit_objs()
    real = real_classes()

    defined = defaultdict(list)     # name -> [(unit, shape_hash, size)]
    referenced = defaultdict(set)   # name -> {units}
    for unit, path in objs:
        c = Coff(path)
        d, u = c.symbols()
        for nm, sec in d.items():
            h, sz = c.shape(sec)
            defined[nm].append((unit, h, sz))
        for nm in u:
            referenced[nm].add(unit)

    # (1) UNRESOLVED: referenced, defined by no obj, and no .LIB can supply it.
    # Split PHANTOM (owning class owns no retail address at all -> can never link)
    # from BACKLOG (real class, member simply not reconstructed yet).
    unresolved, backlog = [], []
    for nm, units in sorted(referenced.items()):
        if nm in defined or is_exempt(nm, libs):
            continue
        cls = owning_class(nm)
        (unresolved if (cls and cls not in real) else backlog).append((nm, sorted(units)))

    # (2) DIVERGENT: one name, >1 obj, >1 distinct byte shape.
    divergent = []
    for nm, defs in sorted(defined.items()):
        shapes = {h for _u, h, _s in defs}
        if len(defs) > 1 and len(shapes) > 1:
            by_shape = defaultdict(list)
            sizes = {}
            for u, h, s in defs:
                by_shape[h].append(u)
                sizes[h] = s
            variants = sorted(by_shape.items(), key=lambda kv: sizes[kv[0]])
            divergent.append((nm, [(sizes[h], sorted(us)) for h, us in variants]))

    if args.unit:
        unresolved = [r for r in unresolved if args.unit in r[1]]
        backlog = [r for r in backlog if args.unit in r[1]]
        divergent = [d for d in divergent
                     if any(args.unit in us for _sz, us in d[1])]

    if args.csv:
        w = sys.stdout
        w.write("kind,symbol,class,detail\n")
        for nm, units in unresolved:
            w.write('UNRESOLVED,%s,%s,"referenced by: %s"\n'
                    % (nm, owning_class(nm), " ".join(units)))
        for nm, variants in divergent:
            det = " | ".join("%dB in %s" % (sz, ",".join(us)) for sz, us in variants)
            w.write('DIVERGENT,%s,%s,"%s"\n' % (nm, owning_class(nm), det))
        return 1 if (unresolved or divergent) else 0

    print("link-defect scan over %d current-unit objs (%d lib symbols resolvable)\n"
          % (len(objs), len(libs)))

    print("=" * 78)
    print("(1) PHANTOM EXTERNALS - referenced; owning class owns NO retail address")
    print("    -> can NEVER link: retail has no such function. Fix = model the real")
    print("       class (these are fabricated per-TU views). %d symbol(s)." % len(unresolved))
    print("=" * 78)
    by_cls = defaultdict(list)
    for nm, units in unresolved:
        by_cls[owning_class(nm) or "(free)"].append((nm, units))
    for cls, rows in sorted(by_cls.items(), key=lambda kv: -len(kv[1])):
        print("  %s  (%d)" % (cls, len(rows)))
        for nm, units in rows:
            print("      %-52s <- %s" % (nm, ",".join(units)))

    if not args.phantom_only:
        print()
        print("  [backlog] %d further unresolved symbol(s) belong to REAL classes/free "
              "functions" % len(backlog))
        print("            not reconstructed yet (declared-only so their rel32 reloc-masks).")
        print("            They resolve for free as those bodies land - not modelling defects.")

    print()
    print("=" * 78)
    print("(2) DIVERGENT DUPLICATE COMDATs - one name, different bytes in >1 obj")
    print("    -> ODR landmine: the linker may pick the stub over the real body.")
    print("       %d symbol(s)." % len(divergent))
    print("=" * 78)
    for nm, variants in divergent:
        print("  %s" % nm)
        for sz, us in variants:
            print("      %5d B  %s" % (sz, ",".join(us)))

    print("\nTOTAL: %d PHANTOM, %d DIVERGENT  (target: 0 / 0)   [+%d backlog]"
          % (len(unresolved), len(divergent), len(backlog)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
