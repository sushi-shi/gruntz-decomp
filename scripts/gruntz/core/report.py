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
