"""gruntz.walls.inventory - the wall worklist, derived, never hand-kept.

    gruntz walls inventory [--unit U] [--below PCT] [--todo]
                           [--json] [--limit N]

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

The report also scores the carved EH band (`__ehreg$*` / `__ehunwind$*`),
which gruntz.verify.scores excludes from the gate because those funclets are
not reconstruction targets. They are KEPT here (they are real sub-100 rows)
but counted separately in the header, so the worklist total is never read as
a body count.

``--todo`` is Codex's explicit campaign queue.  It removes EH-band funclets,
functions already proven exact historically, and only those functions Codex
personally recorded as ``closed`` at the current source hash.  Inherited
``@early-stop`` markers do not affect it.  Hash-valid ``open`` reviews remain
in the queue with their recorded class and next evidence-bearing action.
"""

from __future__ import annotations

import json

from gruntz.core.paths import BUILD, REPO
from gruntz.verify.scores import is_eh_band

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


EPS = 0.01   # the report's raw float vs the 4-decimal stored best: strict `<`
#              flags pure quantization jitter (352 rows measured) as regression


def baseline_rows() -> dict[int, tuple[float, float, str]]:
    """{rva: (best_pct, hist_pct, src_hash)} from the baseline's function
    rows: unit function best_pct cur_pct tries src_hash rva hist_pct state.
    hist is the cross-hash historical MAX (campaign order); best is the
    current implementation's bank (the regression gate)."""
    out: dict[int, tuple[float, float, str]] = {}
    if not BASELINE.is_file():
        return out
    for line in BASELINE.read_text().splitlines():
        if line.startswith("#") or not line.strip():
            continue
        cols = line.split("\t")
        if len(cols) >= 7 and cols[6].startswith("0x"):
            try:
                best = float(cols[2])
                hist = float(cols[7]) if len(cols) >= 8 and cols[7] else best
                out[int(cols[6], 16)] = (best, hist, cols[5])
            except ValueError:
                continue
    return out


def build(
    unit: str | None = None,
    below: float = 100.0,
    todo: bool = False,
) -> list[dict]:
    from gruntz.model import resolve
    from gruntz.walls.reviews import current as current_reviews
    _path, scores = report_scores()
    best = baseline_rows()
    reviews = current_reviews() if todo else {}
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
        bank, hist, src_hash = best.get(rva, (None, None, ""))
        review = reviews.get(rva)
        if todo and (
            is_eh_band(sym)
            or hist == 100.0
            or (review is not None and review["status"] == "closed")
        ):
            continue
        rows.append({
            "rva": f"0x{rva:06x}" if rva is not None else "",
            "unit": u, "symbol": sym, "cur": pct,
            "hist_max": hist, "size": f"0x{b.size:x}" if b else "",
            "regressed": bank is not None and pct < bank - EPS,
            "proven": hist == 100.0,
            "review_status": review["status"] if review else "",
            "review_class": review["wall_class"] if review else "",
            "review_evidence": review["evidence"] if review else "",
        })
    # ascending historical MAX, unknowns last, then ascending current
    rows.sort(key=lambda r: (r["hist_max"] is None,
                             r["hist_max"] if r["hist_max"] is not None else 0,
                             r["cur"]))
    return rows


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="gruntz walls inventory", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit", help="restrict to one unit of config/units.toml")
    ap.add_argument("--below", type=float, default=100.0,
                    help="score ceiling for a row to count as a wall")
    ap.add_argument("--limit", type=int, default=40, help="rows to print")
    ap.add_argument(
        "--todo",
        action="store_true",
        help="exclude only proven rows and Codex-closed reviews at this source hash",
    )
    ap.add_argument("--json", action="store_true", help="the rows as JSON")
    a = ap.parse_args(argv)
    from gruntz.walls import check_unit
    check_unit(a.unit)
    rows = build(a.unit, a.below, a.todo)
    if a.json:
        print(json.dumps(rows, indent=2))
        return 0
    n_reg = sum(r["regressed"] for r in rows)
    n_prov = sum(r["proven"] for r in rows)
    n_eh = sum(1 for r in rows if is_eh_band(r["symbol"]))
    queue = " todo" if a.todo else ""
    print(f"[walls] {len(rows)}{queue} function(s) below {a.below:g}%  "
          f"({n_prov} proven-at-100 dips, {n_reg} below their bank"
          + (f", {n_eh} EH-band funclets - scored, NOT reconstruction targets"
             if n_eh else "") + ")")
    print(f"{'rva':>10}  {'hist':>6}  {'cur':>6}  {'size':>7}  unit/symbol")
    for r in rows[:a.limit]:
        hist = f"{r['hist_max']:6.2f}" if r["hist_max"] is not None else "     ?"
        flag = " R" if r["regressed"] else ("  " if not r["proven"] else " P")
        review = ""
        if a.todo and r["review_status"]:
            review = f" [{r['review_status']}/{r['review_class']}]"
        print(f"{r['rva']:>10}  {hist}  {r['cur']:6.2f}  {r['size']:>7}"
              f"{flag} {r['unit']}/{r['symbol'][:70]}{review}")
    if len(rows) > a.limit:
        print(f"  ... {len(rows) - a.limit} more (--limit)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
