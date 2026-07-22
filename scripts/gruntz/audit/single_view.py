"""single_view - every global must have ONE view: its real extern.

A global declared with two different (type, linkage) signatures across the tree
is a SPLIT VIEW - two names/types for one datum at one RVA. Only one can match
retail's actual symbol; the other is a fake alias (a `void*` placeholder, a
cross-class reinterpret, or a stale linkage) that mis-models the datum and, worse,
emits a symbol that does not exist in the real image (so a candidate link would
leave it unresolved). The clean-room rule (docs/cleanup-plan.md,
no-cross-casts-nothing-is-a-keep): recover the ONE real class/type and delete the
view.

  (default)   report every global declared with >1 distinct (type, linkage)
  --ratchet   FATAL if a split not in config/single-view-baseline.tsv appears
  --write-baseline   (re)freeze the current split backlog

The backlog is frozen and driven to 0; the ratchet keeps NEW splits from landing.
"""
import argparse
import glob
import os
import re

# extern TYPE [*&...] NAME [array] ;   (TYPE may be qualified / templated)
DECL_RE = re.compile(
    r'^[ \t]*(extern\s+"C"\s+|extern\s+)'
    r'([A-Za-z_][\w:<>]*(?:\s*\*+)?)\s+'
    r'\**([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*;',
    re.M)
BASELINE = os.path.join("config", "single-view-baseline.tsv")


def repo_root():
    p = os.path.abspath(__file__)
    for _ in range(4):
        p = os.path.dirname(p)
    return p


def collect(root):
    """name -> { (type, linkage): set(headers) }"""
    byname = {}
    for h in sorted(glob.glob(os.path.join(root, "include", "**", "*.h"), recursive=True)):
        s = open(h, errors="replace").read()
        rel = os.path.relpath(h, os.path.join(root, "include"))
        for m in DECL_RE.finditer(s):
            link = "C" if '"C"' in m.group(1) else "cpp"
            typ = re.sub(r"\s+", "", m.group(2))
            name = m.group(3)
            byname.setdefault(name, {}).setdefault((typ, link), set()).add(rel)
    return byname


def splits(root):
    return {n: v for n, v in collect(root).items() if len(v) > 1}


def split_key(name, views):
    """stable one-line signature: name + sorted distinct (linkage:type)."""
    sigs = sorted(f"{link}:{typ}" for (typ, link) in views)
    return (name, "|".join(sigs))


def load_baseline(root):
    keys = set()
    path = os.path.join(root, BASELINE)
    if not os.path.exists(path):
        return keys
    for ln in open(path, errors="replace"):
        ln = ln.split("#", 1)[0].strip()
        if ln and "\t" in ln:
            keys.add(tuple(ln.split("\t", 1)))
    return keys


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ratchet", action="store_true")
    ap.add_argument("--write-baseline", action="store_true")
    ap.add_argument("--root", default=None)
    args = ap.parse_args()
    root = args.root or repo_root()

    sp = splits(root)
    keys = {split_key(n, v) for n, v in sp.items()}

    if args.write_baseline:
        with open(os.path.join(root, BASELINE), "w") as fh:
            fh.write("# single-view baseline: globals declared with >1 (type, linkage) - a\n")
            fh.write("# datum wearing two types. Frozen backlog; recover the real extern and\n")
            fh.write("# delete the view (docs/cleanup-plan.md). Regenerate: python -m\n")
            fh.write("# gruntz.audit.single_view --write-baseline\n")
            fh.write("# name\tsignature (linkage:type | ...)\n")
            for k in sorted(keys):
                fh.write("\t".join(k) + "\n")
        print(f"wrote {len(keys)} split-view backlog rows -> {BASELINE}")
        return

    if args.ratchet:
        base = load_baseline(root)
        new = keys - base
        if new:
            print(f"[single-view] FATAL: {len(new)} NEW split view(s) - a global declared "
                  f"with two types/linkages:")
            for name, sig in sorted(new):
                print(f"   {name:28s} {sig}")
            print("   Recover the ONE real extern (its retail symbol) and delete the view.")
            raise SystemExit(1)
        stale = base - keys
        print(f"[single-view] OK - no new split views ({len(base)} baselined"
              + (f", {len(stale)} now unified)" if stale else ")"))
        return

    print(f"== split-view audit: {len(sp)} global(s) declared with >1 (type, linkage) ==")
    for name in sorted(sp):
        views = sp[name]
        kind = "LINKAGE" if len({t for t, _ in views}) == 1 else "TYPE"
        print(f"\n  {name}  [{kind} split]")
        for (typ, link), hs in sorted(views.items()):
            print(f"      {link:3s} {typ:26s} {sorted(hs)}")


if __name__ == "__main__":
    main()
