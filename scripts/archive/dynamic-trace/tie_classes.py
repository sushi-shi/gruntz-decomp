#!/usr/bin/env python3
"""gruntz.analysis.tie_classes - tie functions to classes from a this/ecx trace,
grounded in the reconstructed class hierarchy (incl. PR #52's RTTI parents).

Inheritance matters: a method seen on a CProjectile object may be CProjectile's
own OR an inherited CMovingLogic method - we can't tell from ecx alone. So we
resolve each function to the **lowest common ancestor** of the classes of the
objects it was called on:
  - seen on one class A            -> A  (note: could be A's own or inherited)
  - seen across siblings X,Y:Base  -> Base (a base-class method, NOT X or Y)
Objects that never see a known method form NEW groups named ClassUnknown_<n>
(connected components of co-called functions).

Class hierarchy + rva->class come from:
  - the reconstructed headers/sources (`class X : public Y`, methods named
    `X_<rva>` encode rva->X) - this includes the recovered entity classes;
  - `build/trace/labels.csv` (demangled known symbols rva->class).

Requires a LEAK-PATCHED trace (GRUNTZ_TRACE_PATCH=1): every shared ecx is taken as ground
truth (no purity vote, unlike this_cluster), so address reuse from an unpatched run would
mis-merge classes.

Run: python -m gruntz.analysis.tie_classes [edges.csv] [--out build/trace/this-pointer-classes.csv]
"""
import argparse
import collections
import csv
import re
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])

CLASS_RE = re.compile(r"\b(?:class|struct)\s+([A-Za-z_]\w*)\s*(?::\s*public\s+([A-Za-z_]\w*))?")
# an identifier "<Class>_<rva>" where rva is 5-6 hex digits (the recovered-method convention)
MEMBER_RE = re.compile(r"\b([A-Za-z_]\w*)_([0-9a-fA-F]{5,6})\b")


def rint(s):
    return int(str(s).strip().strip('"'), 16)


def load_hierarchy_and_classes():
    """Scan reconstructed headers/sources -> (rva->class, child->parent)."""
    parent = {}
    classes = set()
    texts = []
    for sub in ("include", "src"):
        for f in (REPO / sub).rglob("*"):
            if f.name in ("discovered.h", "Discovered.cpp"):
                continue                         # our own generated output - don't feed it back
            if f.suffix in (".h", ".cpp"):
                try:
                    t = f.read_text(errors="ignore")
                except OSError:
                    continue
                texts.append(t)
                for m in CLASS_RE.finditer(t):
                    classes.add(m.group(1))
                    if m.group(2):
                        parent[m.group(1)] = m.group(2)
    rva2cls = {}
    for t in texts:
        for m in MEMBER_RE.finditer(t):
            cls, hx = m.group(1), m.group(2)
            if cls in classes and cls != "Stub":   # Stub is the backlog placeholder
                rva2cls[int(hx, 16)] = cls
    # Authoritative rva->class straight from each `RVA(0x..)` macro + the `Class::method` def
    # that follows it. The source is the single source of truth, so renames/dissolutions are
    # picked up immediately and the trace's (stale) labels.csv never overrides them.
    DEF = re.compile(r"\b([A-Za-z_]\w*)::~?[A-Za-z_]\w*\s*\(")
    for t in texts:
        lines = t.split("\n")
        for i, ln in enumerate(lines):
            rm = re.search(r"\bRVA\w*\(\s*(0x[0-9a-fA-F]+)[^)]*\)", ln)
            if not rm:
                continue
            rest = ln[rm.end():].strip()            # def may follow the macro on the same line
            cand = rest if (rest and not rest.startswith("//")) else None
            if cand is None:                         # else it's the next non-comment code line
                for j in range(i + 1, min(i + 4, len(lines))):
                    s = lines[j].strip()
                    if s and not s.startswith("//"):
                        cand = lines[j]
                        break
            dm = DEF.search(cand) if cand else None
            if dm:                                   # free functions (no `Class::`) -> unmapped
                rva2cls[int(rm.group(1), 16)] = dm.group(1)
    return rva2cls, parent


# Junk-attractor placeholder "classes" (the CURRENT source umbrellas, since rva->class is now
# derived from the source above) - not real types. Listing a name here SUPPRESSES anchoring to
# it, so its members re-tie to real classes / ClassUnknown: `ThisStubOwnerUnknown` (ApiCallers.cpp,
# the ex-ApiThisStub residual) and `EngineThisStub`/`EngineLabelBacklog` (Backlog.cpp). Dissolved
# names (ApiThisStub) drop off automatically. The `<Name>_<rva>` hosts and CChatBox*/CCreditz*/
# CHelp* structs ARE specific objects -> kept.
SYNTH_CLASSES = {"ThisStubOwnerUnknown", "EngineThisStub", "EngineLabelBacklog"}


def load_labels(path):
    cls = {}
    if Path(path).exists():
        with open(path) as f:
            for r in csv.DictReader(f):
                c = (r.get("class") or "").strip()
                if c:
                    cls[rint(r["rva"])] = c
    return cls


def load_names(path):
    nm = {}
    if Path(path).exists():
        with open(path) as f:
            for row in csv.reader(f):
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
    return func_objs, obj_funcs


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("edges", nargs="?", default=str(REPO / "build/game/retail/gruntz_edges.csv"))
    ap.add_argument("--labels", default=str(REPO / "build/trace/labels.csv"))
    ap.add_argument("--names", default=str(REPO / "build/trace/cc_all.csv"))
    ap.add_argument("--out", default=str(REPO / "build/trace/this-pointer-classes.csv"))
    args = ap.parse_args()

    func_objs, obj_funcs = load_edges(args.edges)
    src_cls, parent = load_hierarchy_and_classes()    # authoritative, from the source
    names = load_names(args.names)
    # REGENERATE build/trace/labels.csv so it stops drifting from the source: the source wins,
    # stale entries are kept only where the source is silent AND the class isn't a dirty
    # placeholder umbrella (drops Ghidra-vintage names like the dissolved ApiThisStub). Then the
    # snapshot reflects current renames/dissolutions and nothing re-anchors to a dead name.
    DIRTY = SYNTH_CLASSES | {"ApiThisStub", "ApiCallerStubs"}
    rva2cls = {r: c for r, c in load_labels(args.labels).items() if c.split("::")[-1] not in DIRTY}
    rva2cls.update(src_cls)
    with open(args.labels, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["rva", "name", "class"])
        for r in sorted(rva2cls):
            w.writerow(["0x%x" % r, names.get(r, ""), rva2cls[r]])
    # suppress anchoring to the current placeholder umbrellas (their members re-tie below)
    rva2cls = {r: c for r, c in rva2cls.items() if c.split("::")[-1] not in SYNTH_CLASSES}

    def ancestors(c):
        chain, seen = [], set()
        while c and c not in seen:
            chain.append(c); seen.add(c); c = parent.get(c)
        return chain

    def depth(c):
        return len(ancestors(c))

    def join(classes):
        """Lowest common ancestor of a set of classes (most-derived shared one)."""
        classes = [c for c in classes if c]
        if not classes:
            return None
        common = set(ancestors(classes[0]))
        for c in classes[1:]:
            common &= set(ancestors(c))
        return max(common, key=lambda c: (depth(c), c)) if common else None  # name tiebreak -> deterministic

    # object's class := the most-derived known method seen on it
    obj_class = {}
    for ecx, fns in obj_funcs.items():
        cls = [rva2cls[f] for f in fns if f in rva2cls]
        if cls:
            obj_class[ecx] = max(cls, key=lambda c: (depth(c), c))  # name tiebreak -> deterministic

    # ClassUnknown groups: union unanchored functions sharing an object
    uf = {}
    def find(x):
        uf.setdefault(x, x)
        while uf[x] != x:
            uf[x] = uf[uf[x]]; x = uf[x]
        return x
    def union(a, b):
        uf[find(a)] = find(b)
    for ecx, fns in obj_funcs.items():
        if ecx in obj_class:
            continue   # anchored object -> handled by hierarchy join
        fns = list(fns)
        for f in fns[1:]:
            union(fns[0], f)

    rows = []
    unknown_id = {}
    for rva, objs in func_objs.items():
        obs = [obj_class[o] for o in objs if o in obj_class]
        name = names.get(rva, "FUN_%08x" % (0x400000 + rva))
        if rva in rva2cls:                       # already reconstructed
            rows.append((rva2cls[rva], rva, name, "known", "", len(objs), ""))
            continue
        if obs:                                  # tie via hierarchy
            distinct = sorted(set(obs))
            asg = join(distinct)
            if asg is None:
                rows.append(("(ambiguous)", rva, name, "trace", "/".join(distinct), len(objs),
                             "no common ancestor"))
            else:
                note = ("own-or-inherited" if len(distinct) == 1
                        else "base of %s" % ",".join(distinct))
                rows.append((asg, rva, name, "trace", "/".join(distinct), len(objs), note))
        else:                                    # unanchored -> ClassUnknown_n
            root = find(rva)
            cid = unknown_id.setdefault(root, "ClassUnknown_%d" % (len(unknown_id) + 1))
            rows.append((cid, rva, name, "trace-new", "", len(objs), ""))

    rows.sort(key=lambda r: (r[0].startswith("ClassUnknown"), r[0], -r[5]))
    with open(args.out, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["class", "rva", "name", "source", "observed_on", "objects", "note"])
        for cls, rva, name, src, obs, nobj, note in rows:
            w.writerow([cls, "0x%x" % rva, name, src, obs, nobj, note])

    n_known = sum(1 for r in rows if r[3] == "known")
    n_trace = sum(1 for r in rows if r[3] == "trace")
    n_new = sum(1 for r in rows if r[3] == "trace-new")
    n_unk = len(set(r[0] for r in rows if r[0].startswith("ClassUnknown")))
    print("wrote %s" % args.out)
    print("  %d functions tied: %d known, %d trace->existing class, %d trace->ClassUnknown (%d new groups)"
          % (len(rows), n_known, n_trace, n_new, n_unk))


if __name__ == "__main__":
    main()
