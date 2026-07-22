"""data_symbol - DATA_SYMBOL is a symbol-only binding (name->rva, NO storage). It
is the RIGHT tool only for symbols we cannot emit as a plain C++ definition:

  LEGITIMATE (allowed):
    - ctor'd-object globals  (?g@@3U/V<Class>@@A): a `Class g;` needs a default
      ctor - either a compile error or a fabricated ??__E static-init retail lacks.
      Retail places these in zero-init .bss, runtime-Init'd; DATA_SYMBOL binds the
      symbol without invoking a ctor. Retiring them needs the class's ctor + static
      -init (or trivial-default-ctor) reconstructed - a matching campaign, not a
      mechanical convert.
    - compiler-generated symbols (??_7/??_R/??_8 vtables/RTTI, $S function-local
      statics/string literals): emitted only when the construct (MI base, local
      static) is reconstructed. They retire by reconstruction, not conversion.

  RETIREABLE DEFECT (this gate):
    - POD / scalar / const-value globals (int/float/char[]/void*/ptr, ?g@@3H/M/N/
      PAD/PBD/PAX...): these CAN be real `DATA(rva) type name;` defs and MUST be -
      a DATA_SYMBOL here leaves the datum with no storage (unresolved at link).

Down-only ratchet: the current retireable backlog is frozen in
config/data-symbol-baseline.tsv; FATAL on any NEW scalar/POD/const DATA_SYMBOL.
Drive to 0 by converting each to a DATA def (recover const content from core.pe).
"""
import argparse
import glob
import os
import re

DS = re.compile(r"DATA_SYMBOL\(\s*(0x[0-9a-fA-F]+)\s*,[^,]*,\s*([^)\s]+)\s*\)")
# struct/class/union type mangle (ctor'd-object - allowed): ?name@@3 [UVT] <Class>@@
STRUCT_MANGLE = re.compile(r"@@3[UVT]\w")
BASELINE = os.path.join("config", "data-symbol-baseline.tsv")


def repo_root():
    p = os.path.abspath(__file__)
    for _ in range(4):
        p = os.path.dirname(p)
    return p


def is_retireable(name):
    """A DATA_SYMBOL that should be a real DATA def: not compiler-gen, not a
    struct/class (ctor'd) type - i.e. a POD/scalar/const global."""
    if "$S" in name or name.startswith("??_") or "?$S" in name:
        return False  # compiler-generated (local static / vtable / RTTI)
    if STRUCT_MANGLE.search(name):
        return False  # U/V/T struct/class - ctor'd-object, DATA_SYMBOL is correct
    return True       # scalar/pointer/const-value -> must be a DATA def


def defects(root):
    out = []
    for f in sorted(glob.glob(os.path.join(root, "src", "**", "*.cpp"), recursive=True)):
        rel = os.path.relpath(f, root)
        for m in DS.finditer(open(f, errors="replace").read()):
            if is_retireable(m.group(2)):
                out.append((m.group(1), m.group(2), os.path.basename(rel)))
    return out


def key(d):
    return (d[0], d[1])


def load_baseline(root):
    keys = set()
    path = os.path.join(root, BASELINE)
    if os.path.exists(path):
        for ln in open(path, errors="replace"):
            ln = ln.split("#", 1)[0].strip()
            if ln and "\t" in ln:
                keys.add(tuple(ln.split("\t")[:2]))
    return keys


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ratchet", action="store_true")
    ap.add_argument("--write-baseline", action="store_true")
    ap.add_argument("--root", default=None)
    args = ap.parse_args()
    root = args.root or repo_root()
    defs = defects(root)
    keys = {key(d) for d in defs}

    if args.write_baseline:
        with open(os.path.join(root, BASELINE), "w") as fh:
            fh.write("# data-symbol baseline: POD/scalar/const globals still bound by a\n")
            fh.write("# storage-less DATA_SYMBOL (should be real DATA defs). Frozen, down-only;\n")
            fh.write("# drive to 0. Regenerate: python -m gruntz.audit.data_symbol --write-baseline\n")
            fh.write("# rva\tmangled\tfile\n")
            for d in sorted(defs):
                fh.write("\t".join(d) + "\n")
        print(f"wrote {len(keys)} retireable DATA_SYMBOL rows -> {BASELINE}")
        return

    if args.ratchet:
        base = load_baseline(root)
        new = keys - base
        if new:
            print(f"[data-symbol] FATAL: {len(new)} NEW scalar/POD/const DATA_SYMBOL(s) - "
                  f"these emit no storage; write a real DATA(rva) type name; def:")
            for rva, nm in sorted(new):
                print(f"   {rva}  {nm}")
            raise SystemExit(1)
        stale = base - keys
        print(f"[data-symbol] OK - no new POD/scalar DATA_SYMBOL ({len(base)} baselined"
              + (f", {len(stale)} now defined)" if stale else ")"))
        return

    print(f"== retireable (POD/scalar/const) DATA_SYMBOLs: {len(defs)} ==")
    for rva, nm, f in sorted(defs):
        print(f"   {rva}  {nm:40s} ({f})")


if __name__ == "__main__":
    main()
