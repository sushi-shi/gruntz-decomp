#!/usr/bin/env python3
"""max_divergence.py - every function that once scored higher than it does now.

`gruntz status` prints a `Fuzzy Max` column: current fuzzy plus the code-weighted
sum of (best-ever - current) over functions below their peak. That number is the
SIZE of the divergence but never its CONTENTS, and the contents are what a lane
can act on: each point of divergence is a source state we HAD and lost.

Two peaks, and the difference between them is the point of this tool:

  --bank     `best_pct` as committed in config/match_baseline.tsv. This is what
             `status`/the README use. It UNDERSTATES the loss, because
             `status update --accept-regressions` lowers `best` to `cur` for the
             rows it blesses, and 426 blesses have done exactly that to 3,786
             rows over the campaign. A flattened peak is gone from the file.

  --history  the max over EVERY revision of config/match_baseline.tsv in git,
             keyed by RVA (the stable identity). git still holds the peaks the
             file forgot, so this is the honest high-water mark. ~4x bigger.
             Slow the first time (a `git show` per revision); cached.

Comparability caveat, applied by --history and reported separately: a peak
recorded before the objdiff 3.7.1 -> 3.7.3 bump (13911bfb2, 2026-07-10) was
produced by a different scorer, which recalibrated partial credit DOWNWARD
(that commit re-baselined 283 rows by 322 points). Such a peak is not a target
to chase. A peak of 100.0 IS comparable across the bump - byte-exactness is
scoring-independent - so only PARTIAL pre-bump peaks are discounted.

    python -m gruntz.audit.max_divergence                 # bank peaks (fast)
    python -m gruntz.audit.max_divergence --history       # true peaks (slow, cached)
    python -m gruntz.audit.max_divergence --history --refresh   # rebuild the cache
    python -m gruntz.audit.max_divergence --history --same-source-only
    python -m gruntz.audit.max_divergence --history --restore-same-source
    python -m gruntz.audit.max_divergence --unit ddsurface
    python -m gruntz.audit.max_divergence --exact-only    # only lost 100%s
    python -m gruntz.audit.max_divergence --max N         # exit 1 above N functions
"""
from __future__ import annotations

import argparse
import collections
import json
import subprocess
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
BASELINE = REPO / "config" / "match_baseline.tsv"
REPORT = REPO / "build" / "objdiff" / "report.json"
NAMES = REPO / "build" / "gen" / "symbol_names.csv"
FUNCS = REPO / "config" / "retail" / "functions.tsv"
CACHE = REPO / "build" / "gen" / "max_peaks.json"
CACHE_VERSION = 3

EPS = 0.01
# objdiff 3.7.1 -> 3.7.3 recalibrated partial credit; peaks below 100% recorded
# before it are not comparable with today's scores.
OBJDIFF_BUMP_TS = 1783670937
# Before this commit, generated header dependency lists could be stale. An exact
# peak from that era may describe an object compiled against old headers even when
# the function's own source fingerprint is unchanged, so it is evidence to inspect,
# not safe input for automatic restoration.
BUILD_DEPS_FIX_TS = 1786153107


def parse_baseline(text):
    """{rva_hex: (best, cur, fp, unit, fn)} from one baseline revision."""
    out = {}
    sec = None
    for line in text.splitlines():
        if line.startswith("# ["):
            sec = "functions" if "[functions]" in line else "units"
            continue
        if line.startswith("#") or not line.strip() or sec != "functions":
            continue
        f = line.split("\t")
        if len(f) < 7 or not f[6]:
            continue
        try:
            best, cur = float(f[2]), float(f[3])
        except ValueError:
            continue
        out[f[6]] = (best, cur, f[5], f[0], f[1])
    return out


def bank_peaks():
    """Peaks as COMMITTED - what `status` sees. Understates; see the module docstring."""
    out = {}
    for rva, (best, cur, fp, unit, fn) in parse_baseline(BASELINE.read_text()).items():
        proof = {fp: {"rev": "(bank)", "ts": 0, "subj": "committed best_pct"}} if fp else {}
        out[rva] = dict(peak=best, rev="(bank)", ts=0, subj="committed best_pct",
                        unit=unit, fn=fn, proofs=proof)
    return out


def history_peaks(refresh=False):
    """Peaks over the WHOLE git history of the baseline, keyed by rva."""
    if CACHE.is_file() and not refresh:
        c = json.loads(CACHE.read_text())
        head = subprocess.run(["git", "-C", str(REPO), "log", "-1", "--format=%H",
                               "--", "config/match_baseline.tsv"],
                              capture_output=True, text=True).stdout.strip()
        if c.get("version") == CACHE_VERSION and c.get("head") == head:
            return c["peaks"]
    revs = subprocess.run(
        ["git", "-C", str(REPO), "log", "--reverse", "--format=%H %ct %s", "--follow",
         "--", "config/match_baseline.tsv"], capture_output=True, text=True).stdout.splitlines()
    peaks = {}
    for i, r in enumerate(revs):
        sha, ct, subj = r.split(" ", 2)
        txt = subprocess.run(["git", "-C", str(REPO), "show", f"{sha}:config/match_baseline.tsv"],
                             capture_output=True, text=True).stdout
        if not txt:
            continue
        for rva, (best, cur, fp, unit, fn) in parse_baseline(txt).items():
            v = max(best, cur)
            if rva not in peaks or v > peaks[rva]["peak"] + EPS:
                proof = {fp: {"rev": sha[:9], "ts": int(ct), "subj": subj}} if fp else {}
                peaks[rva] = dict(peak=v, rev=sha[:9], ts=int(ct), subj=subj,
                                  unit=unit, fn=fn, proofs=proof)
            elif abs(v - peaks[rva]["peak"]) <= EPS and fp:
                # A body may reach the same peak under several implementations.
                # Retain every source fingerprint that actually earned it so a
                # later identical implementation can recover a flattened bank.
                peaks[rva].setdefault("proofs", {})[fp] = {
                    "rev": sha[:9], "ts": int(ct), "subj": subj,
                }
        if i % 200 == 0:
            print(f"  ..{i}/{len(revs)} revisions", file=sys.stderr)
    head = revs[-1].split(" ", 1)[0] if revs else ""
    CACHE.parent.mkdir(parents=True, exist_ok=True)
    CACHE.write_text(json.dumps({"version": CACHE_VERSION, "head": head, "peaks": peaks}))
    return peaks


def restore_same_source(rows):
    """Restore banked peaks that this exact per-function source already earned."""
    from gruntz.match import status as match_status

    match_status.require_bankable_tree("restore same-source historical MAX proofs")
    funcs = match_status.load_baseline()
    by_rva = {f.get("addr"): (key, f) for key, f in funcs.items() if f.get("addr") is not None}
    restored = []
    for row in rows:
        if not row["same_source"] or not row["reproducible_proof"]:
            continue
        found = by_rva.get(row["rva"])
        if found is None:
            continue
        key, f = found
        if row["peak"] <= f["best"] + EPS:
            continue
        old = f["best"]
        f["best"] = row["peak"]
        f["hist"] = max(f.get("hist", old), row["peak"])
        restored.append((key, old, row["peak"]))
    if restored:
        match_status.write_baseline(funcs)
    skipped = sum(1 for row in rows if row["same_source"] and not row["reproducible_proof"])
    print(f"restored {len(restored)} same-source historical MAX proof(s)"
          f"; skipped {skipped} pre-dependency-fix proof(s)")
    for (unit, fn), old, peak in restored:
        print(f"  {unit}  {fn}: {old:.4f} -> {peak:.4f}")
    return restored


def current():
    rep = json.loads(REPORT.read_text())
    out = {}
    for u in rep.get("units", []):
        for fn in u.get("functions", []):
            out[(u["name"], fn["name"])] = float(fn.get("fuzzy_match_percent") or 0.0)
    return out


def rva_and_size():
    size_by_rva = {}
    if FUNCS.is_file():
        for i, line in enumerate(FUNCS.read_text().splitlines()):
            if i == 0 or not line.strip() or line.startswith("#"):
                continue
            p = line.split("\t")
            try:
                size_by_rva[int(p[0], 16)] = int(p[1])
            except (ValueError, IndexError):
                pass
    rva_of = {}
    for line in NAMES.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#") or line.startswith("rva"):
            continue
        p = line.split(",")
        if len(p) < 3:
            continue
        try:
            rva_of[(p[2], p[1])] = int(p[0], 16)
        except ValueError:
            pass
    return rva_of, size_by_rva


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--history", action="store_true",
                    help="peak over the whole git history of the baseline (honest, slow)")
    ap.add_argument("--refresh", action="store_true", help="rebuild the history cache")
    ap.add_argument("--unit", help="restrict to one unit")
    ap.add_argument("--exact-only", action="store_true",
                    help="only functions whose peak was 100%% (scoring-independent)")
    ap.add_argument("--same-source-only", action="store_true",
                    help="only peaks earned by the current per-function source fingerprint")
    ap.add_argument("--restore-same-source", action="store_true",
                    help="restore banked peaks already earned by this exact source (requires --history)")
    ap.add_argument("--top", type=int, default=40, help="rows to print (0 = all)")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--max", type=int, help="exit 1 when more than N functions diverge")
    a = ap.parse_args(argv)

    if a.restore_same_source and not a.history:
        ap.error("--restore-same-source requires --history")

    if not REPORT.is_file():
        print(f"no {REPORT.relative_to(REPO)} - run `gruntz build` first", file=sys.stderr)
        return 2
    peaks = history_peaks(a.refresh) if a.history else bank_peaks()
    current_baseline = parse_baseline(BASELINE.read_text())
    cur = current()
    rva_of, size_by_rva = rva_and_size()

    rows = []
    for key, c in cur.items():
        r = rva_of.get(key)
        if r is None:
            continue
        pk = peaks.get(hex(r))
        if not pk:
            continue
        d = pk["peak"] - c
        if d <= EPS:
            continue
        if a.unit and key[0] != a.unit:
            continue
        if a.exact_only and pk["peak"] < 99.995:
            continue
        current_fp = current_baseline.get(hex(r), (None, None, "", "", ""))[2]
        proof = pk.get("proofs", {}).get(current_fp)
        same_source = bool(current_fp and proof)
        if a.same_source_only and not same_source:
            continue
        rows.append(dict(drop=d, peak=pk["peak"], cur=c, unit=key[0], fn=key[1],
                         size=size_by_rva.get(r, 0), rva=r,
                         peak_rev=pk["rev"], peak_ts=pk["ts"], peak_subj=pk["subj"],
                         same_source=same_source, proof_rev=proof.get("rev") if proof else None,
                         proof_ts=proof.get("ts") if proof else None,
                         proof_subj=proof.get("subj") if proof else None,
                         reproducible_proof=bool(proof and proof.get("ts", 0) >= BUILD_DEPS_FIX_TS),
                         comparable=(pk["peak"] >= 99.995 or pk["ts"] >= OBJDIFF_BUMP_TS
                                     or not a.history)))
    rows.sort(key=lambda x: -x["drop"])
    tot_code = sum(size_by_rva.get(r, 0) for r in rva_of.values()) or 1

    if a.json:
        print(json.dumps(rows, indent=1))
    else:
        src = "git history of the baseline" if a.history else "committed best_pct"
        cw = sum(x["drop"] * x["size"] for x in rows) / tot_code
        print(f"max divergence ({src}): {len(rows)} functions below their peak")
        print(f"  {sum(x['drop'] for x in rows):.1f} percent-points  |  {cw:.4f}% code-weighted"
              f"  |  {sum(1 for x in rows if x['peak'] >= 99.995)} lost a proven EXACT")
        if a.history:
            nc = [x for x in rows if not x["comparable"]]
            if nc:
                print(f"  of which {len(nc)} ({sum(x['drop'] for x in nc):.1f} pts) carry a "
                      f"PARTIAL peak from before the objdiff 3.7.3 bump - not comparable, "
                      f"do not chase")
        b = collections.Counter()
        for x in rows:
            for lo, hi, lbl in [(0, .1, "<0.1"), (.1, 1, "0.1-1"), (1, 5, "1-5"),
                                (5, 20, "5-20"), (20, 50, "20-50"), (50, 101, ">50")]:
                if lo <= x["drop"] < hi:
                    b[lbl] += 1
                    break
        print("  spread: " + "  ".join(f"{k} {b[k]}" for k in
                                       ["<0.1", "0.1-1", "1-5", "5-20", "20-50", ">50"] if b[k]))
        print()
        show = rows if a.top == 0 else rows[:a.top]
        print("%8s %8s %8s %6s  %-22s %s" % ("drop", "peak", "cur", "bytes", "unit", "function"))
        for x in show:
            flag = "" if x["comparable"] else "  [pre-bump peak]"
            if x["same_source"]:
                flag += f"  [same-source proof {x['proof_rev']}]"
                if not x["reproducible_proof"]:
                    flag += "  [pre-dependency-fix]"
            print("%8.3f %8.3f %8.3f %6d  %-22s %s  0x%06x%s" %
                  (x["drop"], x["peak"], x["cur"], x["size"], x["unit"],
                   x["fn"][:56], x["rva"], flag))
        if a.top and len(rows) > a.top:
            print(f"  ... {len(rows) - a.top} more (--top 0 for all)")

    if a.restore_same_source:
        restore_same_source(rows)

    if a.max is not None and len(rows) > a.max:
        print(f"FAIL: {len(rows)} diverging functions exceeds --max {a.max}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
