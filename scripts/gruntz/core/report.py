"""gruntz.core.report - the objdiff report.json, loaded once."""
import json

from gruntz.core.pe import REPO

REPORT = REPO / "build/objdiff/report.json"


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
        restricted to one unit), or None."""
        for u in self.units:
            if unit and u.get("name") != unit:
                continue
            for fn in u.get("functions") or []:
                if fn.get("name") == name:
                    return fn.get("fuzzy_match_percent")
        return None
