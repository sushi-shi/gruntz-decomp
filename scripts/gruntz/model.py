"""gruntz.model - the one join: census rows x provider claims -> bindings.

    gruntz model            resolve, write build/gen/bindings.tsv +
                            violations.tsv (both write-if-changed - the
                            bindings file is the delink key), print a summary

THE RULE: functions.tsv / data.tsv contribute ONLY structure - starts, kinds,
derived extents. Every identity (name, unit, exact matched size) comes from a
channel: the extracted source claims (RVA/DATA/RVA_COMPGEN/RVA_DYNINIT) and
the committed provider tables. A claim whose rva is not an admitted census
row is a violation; a claim size may never cross the next admitted start.

Resolution policy (this module is the ONLY place policy lives):
  * LOW-confidence static-lib rows are leads, not claims - filtered here;
  * channel precedence per rva: src > src_compgen > src_dyninit >
    functions_zlib/data_zlib > data_vtables > data_compgen >
    data_static_libs > functions_static_libs; later claims on the same rva
    become recorded ALIASES, never silent losers;
  * function extent = claimed size when the winning channel states one
    (src / zlib), else the census-derived extent;
  * kind compatibility: func claims bind kind ''|helper (static-lib labels
    also thunk - retail interleaves are real); data claims must match their
    census kind where the channel implies one (vtable/rtti/common/copy).
"""

from __future__ import annotations

from typing import NamedTuple

from gruntz.core.paths import BUILD
from gruntz.core.tsv import write as write_tsv
from gruntz.retail_labels import Claim, censuses, fragments as src_claims, providers

BINDINGS = BUILD / "gen/bindings.tsv"
VIOLATIONS = BUILD / "gen/violations.tsv"

_PRECEDENCE = ["src", "src_compgen", "src_dyninit", "functions_zlib",
               "data_zlib", "data_vtables", "data_compgen",
               "data_static_libs", "functions_static_libs"]

#: channels whose claimed size is the exact matched extent (overrides derived)
_SIZE_AUTHORITY = {"src", "src_compgen", "functions_zlib", "data_zlib"}

#: census kinds a func claim may bind, per channel
_FUNC_KINDS = {"src": {"", "helper"}, "src_compgen": {"", "helper"},
               "src_dyninit": {""}, "functions_zlib": {""},
               "functions_static_libs": {"", "thunk", "helper"}}

#: census kind a data channel implies (None = any non-bookkeeping kind)
_DATA_KIND = {"data_vtables": "vtable", "data_zlib": None, "src": None}


class Binding(NamedTuple):
    rva: int
    size: int
    kind: str          # the census kind
    space: str         # 'text' | 'rdata' | 'data' | 'bss'
    name: str          # winning claim's name, or '' (unclaimed)
    unit: str
    channel: str       # winning channel, or ''
    aliases: tuple     # losing Claims on the same rva


class Model(NamedTuple):
    functions: list[Binding]
    data: list[Binding]
    violations: list[str]

    def claimed(self, kind=None):
        rows = (self.functions if kind == "func" else
                self.data if kind == "data" else self.functions + self.data)
        return [b for b in rows if b.channel]


def _active(claim: Claim) -> bool:
    if claim.channel == "functions_static_libs":
        return claim.meta.get("confidence", "").upper() != "LOW"
    return True


def _data_expected_kind(claim: Claim) -> str | None:
    if claim.channel == "data_compgen":
        return claim.meta.get("class")            # 'common' | 'copy'
    if claim.channel == "data_static_libs":
        if claim.name.startswith("??_7"):
            return "vtable"
        if claim.name.startswith("??_R"):
            return "rtti"
        return None
    return _DATA_KIND.get(claim.channel)


def resolve() -> Model:
    violations: list[str] = []

    fn_rows = {r["rva"]: r for r in censuses.functions()}
    dt_rows = {r["rva"]: r for r in censuses.data()}

    all_claims = [c for c in providers.all_claims() + src_claims.all_claims()
                  if _active(c)]
    # A header-inline definition carries its RVA()/DATA() macro into EVERY
    # including TU, so identical (kind, rva, channel, name) claims arrive from
    # several units. Collapse them deterministically: alphabetically-first
    # unit owns, the rest ride along in meta["also_units"].
    merged: dict[tuple, Claim] = {}
    for c in sorted(all_claims, key=lambda c: (c.kind, c.rva, c.channel,
                                               c.name, c.unit)):
        key = (c.kind, c.rva, c.channel, c.name)
        prev = merged.get(key)
        if prev is None:
            merged[key] = c
        elif c.unit and c.unit != prev.unit:
            prev.meta.setdefault("also_units", []).append(c.unit)
    per_rva: dict[tuple[str, int], list[Claim]] = {}
    for c in merged.values():
        per_rva.setdefault((c.kind, c.rva), []).append(c)

    def pick(cands: list[Claim]) -> tuple[Claim, list[Claim]]:
        ordered = sorted(cands, key=lambda c: _PRECEDENCE.index(c.channel))
        return ordered[0], ordered[1:]

    functions: list[Binding] = []
    for rva, row in sorted(fn_rows.items()):
        cands = per_rva.pop(("func", rva), [])
        if not cands:
            functions.append(Binding(rva, row["size"], row["kind"], "text",
                                     "", "", "", ()))
            continue
        win, rest = pick(cands)
        allowed = _FUNC_KINDS.get(win.channel, {""})
        if row["kind"] not in allowed:
            violations.append(
                f"func claim {win.name} ({win.channel}) binds kind="
                f"{row['kind']!r} row 0x{rva:06x} (allowed {sorted(allowed)})")
        size = row["size"]
        if win.channel in _SIZE_AUTHORITY and win.size:
            if win.size > row["size"]:
                violations.append(
                    f"claim {win.name} size 0x{win.size:x} crosses the next "
                    f"admitted start (derived 0x{row['size']:x}) at 0x{rva:06x}")
            else:
                size = win.size
        functions.append(Binding(rva, size, row["kind"], "text",
                                 win.name, win.unit, win.channel, tuple(rest)))

    data: list[Binding] = []
    for rva, row in sorted(dt_rows.items()):
        cands = per_rva.pop(("data", rva), [])
        if not cands:
            data.append(Binding(rva, row["size"], row["kind"], row["region"],
                                "", "", "", ()))
            continue
        win, rest = pick(cands)
        expected = _data_expected_kind(win)
        if expected is not None and row["kind"] != expected:
            violations.append(
                f"data claim {win.name} ({win.channel}) expects kind="
                f"{expected!r} but census row 0x{rva:06x} is {row['kind']!r}")
        elif expected is None and row["kind"] in ("pad", "ehtable"):
            violations.append(
                f"data claim {win.name} ({win.channel}) binds bookkeeping "
                f"kind={row['kind']!r} row 0x{rva:06x}")
        size = row["size"]
        if win.channel in _SIZE_AUTHORITY and win.size:
            if win.size > row["size"]:
                violations.append(
                    f"data claim {win.name} size 0x{win.size:x} crosses the "
                    f"next admitted start at 0x{rva:06x}")
            else:
                size = win.size
        data.append(Binding(rva, size, row["kind"], row["region"],
                            win.name, win.unit, win.channel, tuple(rest)))

    # claims that hit no census row at all
    for (kind, rva), cands in sorted(per_rva.items()):
        for c in cands:
            violations.append(
                f"{kind} claim {c.name} ({c.channel}) at 0x{rva:06x} is not "
                f"an admitted census row")

    return Model(functions, data, violations)


def serialize(model: Model) -> tuple[bool, bool]:
    """Write bindings.tsv (the delink key) + violations.tsv, write-if-changed."""
    header = ["rva", "size", "kind", "space", "name", "unit", "channel", "aliases"]
    rows = [[f"0x{b.rva:08x}", f"0x{b.size:x}", b.kind, b.space, b.name,
             b.unit, b.channel,
             ";".join(f"{a.channel}:{a.name}" for a in b.aliases)]
            for b in model.functions + model.data]
    changed_b = write_tsv(BINDINGS, ["# GENERATED by gruntz.model - the "
                                     "resolved claim set (the delink key)."],
                          header, rows)
    changed_v = write_tsv(VIOLATIONS, ["# GENERATED by gruntz.model."],
                          ["violation"], [[v] for v in model.violations])
    return changed_b, changed_v


def main() -> int:
    from collections import Counter
    model = resolve()
    changed_b, _ = serialize(model)
    fn_ch = Counter(b.channel or "(unclaimed)" for b in model.functions)
    dt_ch = Counter(b.channel or "(unclaimed)" for b in model.data)
    print("functions:", dict(fn_ch.most_common()))
    print("data:     ", dict(dt_ch.most_common()))
    print(f"violations: {len(model.violations)}"
          + (f" (first: {model.violations[0]})" if model.violations else ""))
    print(f"bindings.tsv {'UPDATED' if changed_b else 'unchanged'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
