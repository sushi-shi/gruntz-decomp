#!/usr/bin/env python3
"""gruntz.analysis.this_cluster - recover class membership from a this/ecx trace.

Input: an edges CSV (rva,ecx,seq) from the Frida this-tracer (the game's
gruntz_edges.csv) - each row means "function rva was called with this==ecx".
With _free patched to leak, every distinct ecx is a unique object for the run, so
functions sharing an ecx were called on the same object => the same class.

Method - label propagation over the bipartite functions<->objects graph:
  1. object_class[ecx] = majority class among the KNOWN-class functions seen on
     that object (objects are near class-pure in practice).
  2. each UNKNOWN function (FUN_/unlabeled) gets a candidate class = the majority
     object_class over the objects it was called on, with support + purity.

Output: a discovery worklist (unknown rva -> candidate class) and per-class member
lists, plus validation stats. Feeds the matching campaign: a FUN_ landing in a
known class with high support/purity is a strong "this is a <Class> method" lead.

Run: python -m gruntz.analysis.this_cluster [edges.csv]
       [--labels build/trace/labels.csv] [--names build/trace/cc_all.csv]
       [--min-support N] [--json out.json]
"""
import argparse
import collections
import csv
import json
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
T = REPO / "build" / "trace"


def rint(s):
    return int(str(s).strip().strip('"'), 16)


def load_labels(path):
    """rva -> class, for functions with a known (demangled) class."""
    cls = {}
    if not Path(path).exists():
        return cls
    with open(path) as f:
        for r in csv.DictReader(f):
            c = (r.get("class") or "").strip()
            if c:
                cls[rint(r["rva"])] = c
    return cls


def load_names(path):
    """rva -> ghidra/demangled name (FUN_xxx for unknown bodies)."""
    nm = {}
    if not Path(path).exists():
        return nm
    with open(path) as f:
        rd = csv.reader(f)
        for row in rd:
            if not row or row[0].lower().startswith(("rva", "entry_rva")):
                continue
            try:
                nm[rint(row[0])] = row[-1]
            except ValueError:
                pass
    return nm


def load_edges(path):
    func_objs = collections.defaultdict(set)
    obj_funcs = collections.defaultdict(set)
    n = 0
    with open(path) as f:
        for r in csv.reader(f):
            if not r or r[0].lower() == "rva":
                continue
            try:
                rva, ecx = rint(r[0]), rint(r[1])
            except (ValueError, IndexError):
                continue
            func_objs[rva].add(ecx)
            obj_funcs[ecx].add(rva)
            n += 1
    return func_objs, obj_funcs, n


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("edges", nargs="?", default=str(REPO / "build/game/retail/gruntz_edges.csv"))
    ap.add_argument("--labels", default=str(T / "labels.csv"))
    ap.add_argument("--names", default=str(T / "cc_all.csv"))
    ap.add_argument("--min-support", type=int, default=2,
                    help="min #objects backing a candidate to report it")
    ap.add_argument("--json", default=None)
    ap.add_argument("--classes-out", default=None,
                    help="write the full function->class table (known + trace-discovered)")
    args = ap.parse_args()

    func_objs, obj_funcs, nedges = load_edges(args.edges)
    cls = load_labels(args.labels)
    names = load_names(args.names)

    # 1. classify each object from the known functions called on it
    obj_class = {}
    pure = impure = 0
    for ecx, funcs in obj_funcs.items():
        known = [cls[f] for f in funcs if f in cls]
        if not known:
            continue
        cnt = collections.Counter(known)
        top, n = cnt.most_common(1)[0]
        obj_class[ecx] = top
        if len(cnt) == 1:
            pure += 1
        else:
            impure += 1

    # 2. propagate object classes to UNKNOWN functions
    cand = []
    for rva, objs in func_objs.items():
        if rva in cls:
            continue  # already known
        votes = collections.Counter(obj_class[o] for o in objs if o in obj_class)
        if not votes:
            continue
        top, n = votes.most_common(1)[0]
        total = sum(votes.values())
        cand.append({
            "rva": "0x%x" % rva, "name": names.get(rva, "FUN_%08x" % (0x400000 + rva)),
            "candidate_class": top, "support": n, "voted": total,
            "purity": round(n / total, 3), "objects": len(objs),
        })
    cand.sort(key=lambda c: (-c["support"], -c["purity"]))
    strong = [c for c in cand if c["support"] >= args.min_support]

    by_class = collections.defaultdict(list)
    for c in strong:
        by_class[c["candidate_class"]].append(c)

    print("=== this/ecx trace clustering ===")
    print("edges=%d  functions=%d  objects=%d" % (nedges, len(func_objs), len(obj_funcs)))
    print("known-labeled functions hit: %d" % sum(1 for f in func_objs if f in cls))
    print("objects classified: %d (class-pure %d, mixed %d)" % (len(obj_class), pure, impure))
    print("unknown functions with a candidate class: %d (>=%d objs: %d)"
          % (len(cand), args.min_support, len(strong)))
    print()
    print("=== candidate members per class (unknown fns -> class) ===")
    for klass, members in sorted(by_class.items(), key=lambda kv: -len(kv[1])):
        print("  %-28s %d candidates" % (klass, len(members)))
        for m in members[:6]:
            print("      %s  %-24s support=%d/%d purity=%.2f objs=%d"
                  % (m["rva"], m["name"][:24], m["support"], m["voted"], m["purity"], m["objects"]))
    if args.json:
        Path(args.json).write_text(json.dumps(
            {"summary": {"edges": nedges, "functions": len(func_objs), "objects": len(obj_funcs)},
             "candidates": strong}, indent=2))
        print("\nwrote %s (%d candidates)" % (args.json, len(strong)))

    if args.classes_out:
        rows = []
        for rva, klass in cls.items():   # already-known classes (from mangled symbols)
            rows.append((klass, rva, names.get(rva, ""), "known", "", "", len(func_objs.get(rva, ()))))
        for c in cand:                   # trace-discovered (unknown fn -> candidate class)
            rows.append((c["candidate_class"], int(c["rva"], 16), c["name"], "trace",
                         c["support"], c["purity"], c["objects"]))
        rows.sort(key=lambda r: (r[0], r[1]))
        with open(args.classes_out, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["class", "rva", "name", "source", "support", "purity", "objects"])
            for klass, rva, name, src, sup, pur, objs in rows:
                w.writerow([klass, "0x%x" % rva, name, src, sup, pur, objs])
        print("\nwrote %s: %d functions tied to classes (%d known + %d trace-discovered)"
              % (args.classes_out, len(rows), len(cls), len(cand)))


if __name__ == "__main__":
    main()
