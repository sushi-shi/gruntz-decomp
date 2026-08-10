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


# The compiler EH funclets `gruntz.build.eh_band` carves out of retail's packed
# band. They ARE scored (that is the point of carving them), but they are NOT
# reconstruction targets: the README's function universe classifies every one of
# them `eh` and excludes the whole category from the denominator. Leaving them in
# objdiff's aggregate would count them in the numerator against a denominator that
# never had them - the headline read `5,137 / 4,314` before this filter existed.
EH_BAND_PREFIXES = ("__ehreg$", "__ehunwind$")
EXACT = 99.995


def is_eh_band(name):
    return name.startswith(EH_BAND_PREFIXES)


def split_eh_band(doc):
    """Strip the EH band rows from `doc` (in place) and return them.

    objdiff's measures are exact sums over the per-function rows - `total_code` is
    the size sum, `matched_code` the size sum of the rows at 100%, `matched_functions`
    their count, and `fuzzy_match_percent` their size-weighted mean - so removing a
    known subset is arithmetic, not re-estimation.  Returns
    `[(unit, name, size, pct)]` for the removed rows.
    """
    removed = []
    for unit in doc.get("units", []):
        rows = unit.get("functions")
        if not rows:
            continue
        band = [row for row in rows if is_eh_band(row["name"])]
        if not band:
            continue
        unit["functions"] = [row for row in rows if not is_eh_band(row["name"])]
        _subtract(unit.setdefault("measures", {}), band)
        removed.extend((unit.get("name", ""), row["name"], int(row.get("size") or 0),
                        fn_fuzzy(row)) for row in band)
    if removed:
        _subtract(doc.setdefault("measures", {}),
                  [{"size": size, "fuzzy_match_percent": pct}
                   for _unit, _name, size, pct in removed])
    return removed


def _subtract(measures, rows):
    """Remove `rows`' contribution from one objdiff `measures` block."""
    total_code = int(measures.get("total_code") or 0)
    weighted = float(measures.get("fuzzy_match_percent") or 0.0) * total_code
    for row in rows:
        size = int(row.get("size") or 0)
        pct = fn_fuzzy(row)
        total_code -= size
        weighted -= size * pct
        measures["total_functions"] = int(measures.get("total_functions") or 0) - 1
        if pct >= EXACT:
            measures["matched_functions"] = int(measures.get("matched_functions") or 0) - 1
            measures["matched_code"] = int(measures.get("matched_code") or 0) - size
    measures["total_code"] = str(total_code)
    measures["matched_code"] = str(int(measures.get("matched_code") or 0))
    measures["fuzzy_match_percent"] = (weighted / total_code) if total_code else 0.0
    measures["matched_code_percent"] = (
        100.0 * int(measures["matched_code"]) / total_code if total_code else 0.0)
    total_functions = int(measures.get("total_functions") or 0)
    measures["matched_functions_percent"] = (
        100.0 * int(measures.get("matched_functions") or 0) / total_functions
        if total_functions else 0.0)


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
