"""gruntz.walls.inventory - the wall worklist, derived, never hand-kept.

    gruntz walls inventory [--unit U] [--below PCT] [--json] [--limit N]

A wall is a function scoring below 100% fuzzy in the CURRENT compare report.
The worklist is a JOIN of three read-only inputs and nothing else:

  * build/objdiff/compare-new/report.json (falling back to
    build/objdiff/report.json) - the current per-function scores;
  * the Model (gruntz.model.resolve) - rva/unit/size/channel per name;
  * config/match_baseline.tsv - best_pct (historical MAX) + src_hash per rva.

Campaign order (CLAUDE.md): ascending historical MAX - the lowest bank is
the biggest structural question. cur < best is a REGRESSION flag, not a
wall class; best == 100 with cur < 100 means the implementation already
proved the body and the current dip is TU-state, not structure.
"""

from __future__ import annotations

import json

from gruntz.core.paths import BUILD, REPO

BASELINE = REPO / "config/match_baseline.tsv"
REPORTS = (BUILD / "objdiff/compare-new/report.json",
           BUILD / "objdiff/report.json")


def report_scores() -> tuple[str, dict[tuple[str, str], float]]:
    """(report path used, {(unit, symbol): fuzzy%})."""
    for path in REPORTS:
        if path.is_file():
            doc = json.loads(path.read_text())
            out = {}
            for u in doc.get("units", []):
                uname = u["name"].split("/")[-1]
                for f in u.get("functions", []):
                    out[(uname, f["name"])] = float(
                        f.get("fuzzy_match_percent") or 0.0)
            return str(path), out
    raise SystemExit("[walls] no report.json - run `gruntz compare` first")


def baseline_rows() -> dict[int, tuple[float, str]]:
    """{rva: (best_pct, src_hash)} from the function rows of the baseline."""
    out: dict[int, tuple[float, str]] = {}
    if not BASELINE.is_file():
        return out
    for line in BASELINE.read_text().splitlines():
        if line.startswith("#") or not line.strip():
            continue
        cols = line.split("\t")
        # function rows: unit  function  best_pct  cur_pct  tries  src_hash  rva
        if len(cols) >= 7 and cols[6].startswith("0x"):
            try:
                out[int(cols[6], 16)] = (float(cols[2]), cols[5])
            except ValueError:
                continue
    return out


def build(unit: str | None = None, below: float = 100.0) -> list[dict]:
    from gruntz.model import resolve
    _path, scores = report_scores()
    best = baseline_rows()
    by_name: dict[tuple[str, str], object] = {}
    for b in resolve().functions:
        if b.name:
            by_name[(b.unit, b.name)] = b
    rows = []
    for (u, sym), pct in scores.items():
        if pct >= below or (unit and u != unit):
            continue
        b = by_name.get((u, sym))
        rva = b.rva if b else None
        hist, src_hash = best.get(rva, (None, ""))
        rows.append({
            "rva": f"0x{rva:06x}" if rva is not None else "",
            "unit": u, "symbol": sym, "cur": pct,
            "hist_max": hist, "size": f"0x{b.size:x}" if b else "",
            "regressed": hist is not None and pct < hist,
            "proven": hist == 100.0,
        })
    # ascending historical MAX, unknowns last, then ascending current
    rows.sort(key=lambda r: (r["hist_max"] is None,
                             r["hist_max"] if r["hist_max"] is not None else 0,
                             r["cur"]))
    return rows


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="gruntz walls inventory",
                                 description=__doc__)
    ap.add_argument("--unit")
    ap.add_argument("--below", type=float, default=100.0)
    ap.add_argument("--limit", type=int, default=40)
    ap.add_argument("--json", action="store_true")
    a = ap.parse_args(argv)
    rows = build(a.unit, a.below)
    if a.json:
        print(json.dumps(rows, indent=2))
        return 0
    n_reg = sum(r["regressed"] for r in rows)
    n_prov = sum(r["proven"] for r in rows)
    print(f"[walls] {len(rows)} function(s) below {a.below:g}%  "
          f"({n_prov} proven-at-100 dips, {n_reg} below their bank)")
    print(f"{'rva':>10}  {'hist':>6}  {'cur':>6}  {'size':>7}  unit/symbol")
    for r in rows[:a.limit]:
        hist = f"{r['hist_max']:6.2f}" if r["hist_max"] is not None else "     ?"
        flag = " R" if r["regressed"] else ("  " if not r["proven"] else " P")
        print(f"{r['rva']:>10}  {hist}  {r['cur']:6.2f}  {r['size']:>7}"
              f"{flag} {r['unit']}/{r['symbol'][:70]}")
    if len(rows) > a.limit:
        print(f"  ... {len(rows) - a.limit} more (--limit)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
