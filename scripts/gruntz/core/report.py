"""gruntz.core.report - the objdiff report.json, loaded once.

READ EVERY PERCENT THROUGH `fn_fuzzy()`. objdiff serializes the report with serde's
skip-the-default rule, so a function scored at **exactly 0.0%** has NO
`fuzzy_match_percent` key at all - it is indistinguishable, by key presence, from a
function objdiff never diffed. It is NOT a pairing failure: objdiff's own one-shot
`diff` output carries `"match_percent": 0.0` with a live `target_symbol` link for
these. Measured 2026-07-27: 8 functions in the tree sit at a true 0.0%, and every
one of them was invisible to `.get("fuzzy_match_percent")`.

Defaulting the missing key to anything other than 0.0 is a silent falsification -
`permute_sweep` defaulted it to 100.0 and so skipped exactly the functions that
most needed permuting, and `fn_pct` returned None so `gruntz sema rva` printed no
match line at all for them.
"""
import json

from gruntz.core.pe import REPO

REPORT = REPO / "build/objdiff/report.json"


def fn_fuzzy(fn):
    """The fuzzy match % of one report `functions[]` entry, as a float.

    A missing `fuzzy_match_percent` means 0.0 (serde omits the f32 default), never
    "unknown" and never 100.0. See the module docstring."""
    return float(fn.get("fuzzy_match_percent") or 0.0)


def data_measures(doc=None):
    """Size-weighted `.data`/`.rdata`/`.bss` match, plus the all-or-nothing figure.

    Returns `{section: {bytes, weighted, exact, sections}}` with a `total` row,
    where `weighted` is sum(size * percent) / sum(size) over the report's own
    per-section rows and `exact` is the bytes in sections at exactly 100.0.

    `matched_data` in `measures` is the second one, and it is why the headline
    reads ~16% while the sections average ~99%: `objdiff-cli report generate`
    credits a section all-or-nothing, so a `.data` at 99.99% contributes zero.
    That rule lives in objdiff's report.rs and is not configurable.
    `combine_data_sections` IS (`-c combine_data_sections=false`, and the CLI
    validates its config keys), but it only changes which sections exist -- 16.40%
    -> 17.74% measured, with `fuzzy_match_percent` and `matched_code` bit-identical
    either way -- so it does not close the gap. Report both numbers instead of
    picking one: the weighted figure tracks reconstruction, the all-or-nothing one
    tracks how many sections are finished.
    """
    doc = doc or (json.loads(REPORT.read_text()) if REPORT.is_file() else {})
    out = {}
    for u in doc.get("units", []):
        for s in u.get("sections", []):
            if s["name"] == ".text":
                continue
            row = out.setdefault(s["name"],
                                 {"bytes": 0, "weighted": 0.0, "exact": 0,
                                  "sections": 0})
            size, pct = int(s["size"]), float(s.get("fuzzy_match_percent") or 0.0)
            row["bytes"] += size
            row["weighted"] += size * pct / 100.0
            row["sections"] += 1
            if pct >= 100.0:
                row["exact"] += size
    total = {"bytes": 0, "weighted": 0.0, "exact": 0, "sections": 0}
    for row in out.values():
        for k in total:
            total[k] += row[k]
    out["total"] = total
    return out


class Report:
    def __init__(self):
        self._units = None

    @property
    def units(self):
        if self._units is None:
            self._units = (json.loads(REPORT.read_text()).get("units", [])
                           if REPORT.is_file() else [])
        return self._units

    def fn_pct(self, name, unit=None):
        """fuzzy_match_percent for a function by mangled name (optionally
        restricted to one unit).

        Returns a float for every function the report LISTS - including 0.0, whose
        key objdiff omits (see the module docstring). None means the name is not in
        the report at all, which is a different fact and the only one worth hiding
        a match line for."""
        for u in self.units:
            if unit and u.get("name") != unit:
                continue
            for fn in u.get("functions") or []:
                if fn.get("name") == name:
                    return fn_fuzzy(fn)
        return None
