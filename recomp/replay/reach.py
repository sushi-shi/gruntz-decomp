#!/usr/bin/env python3
r"""reach.py - what the replay oracle can actually reach, measured, not claimed.

    python recomp/replay/reach.py                       # the table
    python recomp/replay/reach.py --list                # the runnable worklist
    python recomp/replay/reach.py --probe-cfg           # a probe.cfg.tmpl for them
    python recomp/replay/reach.py --why '?Step@CMotionState@@QAEXXZ'

Three independent conditions have to hold before a function can be replayed, and
each is answered by a different tool, so they are answered separately here:

  TIER      `gruntz.audit.iat_tiers` says CLEAN - the transitive call closure
            never reaches the IAT, so nothing the body does leaves the process
            and a memory diff sees all of it.
  HOOKABLE  `hookgen` finds a direct `call rel32` site to patch. The capture
            hooks CALL SITES, not prologues, so a function reached only through
            virtual dispatch has nothing to patch.
  BOUND     `objbind` resolves every relocation in the function's COMDAT to a
            retail address (or to a benign intra-function label). This is the
            condition the run-time object loader removed: before it, the
            function had to have NO relocations at all.

The point of splitting them is that each failure has a different remedy, and
lumping them into one "reachable" number hides which one is biting.
"""
import argparse
import collections
import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = next(p for p in HERE.parents if (p / "flake.nix").exists())
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(REPO / "scripts"))

import hookgen  # noqa: E402
import objbind  # noqa: E402


def load_tiers():
    from gruntz.audit import iat_tiers
    edges, defined, direct_imp = iat_tiers.build_graph()
    return iat_tiers.classify(edges, defined, direct_imp)


def load_scores():
    """mangled name -> (fuzzy%, size)."""
    from gruntz.audit import iat_tiers
    return iat_tiers.scores()


def load_units():
    """mangled name -> (unit, rva)."""
    import csv
    out = {}
    p = REPO / "build" / "gen" / "symbol_names.csv"
    with p.open() as f:
        for row in csv.DictReader(f):
            if row["kind"] == "func":
                out[row["name"]] = (row["unit"], int(row["rva"], 16))
    return out


def bound_map(units, rvamap):
    """mangled name -> the objbind row, for every unit we can find an object for."""
    out = {}
    for unit in sorted(units):
        p = REPO / "build" / "objdiff" / "base" / (unit + ".obj")
        if not p.exists():
            continue
        try:
            o = objbind.Obj(p)
        except Exception as e:                                  # noqa: BLE001
            print("[reach] %s: %s" % (p.name, e), file=sys.stderr)
            continue
        m = objbind.Module([o], rvamap, objbind.DEFAULT_LOAD_BASE,
                           objbind.DEFAULT_IMAGE_BASE)
        m.layout()
        m.bind()
        for s in m.symbols:
            if s["flags"] & objbind.OBJSYM_FUNC:
                out[s["name"]] = s
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--min-size", type=lambda s: int(s, 0), default=0x40)
    ap.add_argument("--list", action="store_true", help="the runnable worklist")
    ap.add_argument("--probe-cfg", action="store_true",
                    help="write build/replay/probe.cfg.tmpl for the runnable set")
    ap.add_argument("--why", default=None, help="explain one function")
    ap.add_argument("--all", action="store_true",
                    help="include already-exact functions (the regression set)")
    a = ap.parse_args()

    verdicts = load_tiers()
    pct = load_scores()
    units = load_units()
    rvamap = objbind.load_rvamap()
    img = hookgen.Image(REPO / "build" / "exe" / "GRUNTZ.EXE")

    wanted_units = {units[f][0] for f in verdicts if f in units}
    bound = bound_map(wanted_units, rvamap)

    rows = []
    for f, tier in verdicts.items():
        if tier != "CLEAN":
            continue
        p, size = pct.get(f, (None, 0))
        if p is None:
            continue
        unit, rva = units.get(f, (None, None))
        b = bound.get(f)
        _, sites = sites_cache(img, rva) if rva else (None, [])
        rows.append(dict(name=f, pct=p, size=size, unit=unit, rva=rva,
                         hookable=bool(sites), nsites=len(sites), bound=b))

    if a.why:
        for r in rows:
            if r["name"] != a.why:
                continue
            print("%s\n  unit %s  rva %08x  size %#x  fuzzy %.2f%%"
                  % (r["name"], r["unit"], r["rva"] or 0, r["size"], r["pct"]))
            print("  HOOKABLE  %s (%d direct call site(s))"
                  % ("yes" if r["hookable"] else "NO", r["nsites"]))
            b = r["bound"]
            if not b:
                print("  BOUND     NO - no compiled object row for it")
            else:
                print("  BOUND     %s - %d reloc(s): %d retail, %d intra, %d ours, "
                      "%d unresolved"
                      % ("no" if b["unres"] else "yes", b["n"], b["retail_n"],
                         b["intra"], b["foreign"], b["unres"]))
                for n in sorted(set(b["unres_names"])):
                    print("      UNRESOLVED %s" % n)
                for n in sorted(set(b["foreign_names"])):
                    print("      our copy   %s" % n)
            return 0
        print("reach: '%s' is not a CLEAN function with a score" % a.why)
        return 1

    def sel(rs, exact=None, minsz=None):
        out = rs
        if exact is False:
            out = [r for r in out if r["pct"] < 100.0]
        if minsz:
            out = [r for r in out if r["size"] >= minsz]
        return out

    def runnable(rs):
        return [r for r in rs
                if r["hookable"] and r["bound"] and not r["bound"]["unres"]]

    sets = [("all CLEAN", rows),
            ("CLEAN, not yet exact", sel(rows, exact=False)),
            ("CLEAN, not exact, >= %#x B" % a.min_size,
             sel(rows, exact=False, minsz=a.min_size))]
    print("| set | total | hookable | binds cleanly | BOTH (runnable) |")
    print("|---|---|---|---|---|")
    for label, rs in sets:
        hk = [r for r in rs if r["hookable"]]
        bd = [r for r in rs if r["bound"] and not r["bound"]["unres"]]
        print("| %s | %d | %d | %d | **%d** |"
              % (label, len(rs), len(hk), len(bd), len(runnable(rs))))

    work = runnable(sel(rows, exact=None if a.all else False, minsz=a.min_size))
    work.sort(key=lambda r: (r["pct"], -r["size"]))

    if a.list:
        print()
        for r in work:
            print("%08x %6.2f%% %6d  %-22s %s"
                  % (r["rva"], r["pct"], r["size"], r["unit"], r["name"]))

    if a.probe_cfg:
        out = REPO / "build" / "replay" / "probe.cfg.tmpl"
        lines = ["mode=probe", "out=OUTDIR"]
        for r in work:
            _, sites = sites_cache(img, r["rva"])
            lines.append("probe=" + ",".join("0x%08x" % x
                                             for x in [r["rva"]] + sites))
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text("\n".join(lines) + "\n")
        print("\n[reach] wrote %s: %d candidate(s), %d site(s)"
              % (out, len(work), sum(len(sites_cache(img, r["rva"])[1])
                                     for r in work)))

    blocked = collections.Counter()
    for r in sel(rows, exact=False, minsz=a.min_size):
        if not r["hookable"]:
            blocked["no direct call site (needs a prologue detour)"] += 1
        elif not r["bound"]:
            blocked["no compiled object row"] += 1
        elif r["bound"]["unres"]:
            for n in set(r["bound"]["unres_names"]):
                blocked["unresolved: " + n] += 1
    if blocked:
        print("\nwhat blocks the rest (CLEAN, not exact, >= %#x B):" % a.min_size)
        for k, v in blocked.most_common(20):
            print("   %4d  %s" % (v, k))
    return 0


_SITES = {}


def sites_cache(img, rva):
    if rva not in _SITES:
        _SITES[rva] = hookgen.sites_for(img, rva)
    return _SITES[rva]


if __name__ == "__main__":
    sys.exit(main())
