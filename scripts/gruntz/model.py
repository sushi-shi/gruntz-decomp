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

import re

from gruntz.core.paths import BUILD
from gruntz.core.tsv import write as write_tsv
from gruntz.retail_labels import Claim, censuses, fragments as src_claims, providers

BINDINGS = BUILD / "gen/bindings.tsv"
VIOLATIONS = BUILD / "gen/violations.tsv"

_PRECEDENCE = ["src", "src_compgen", "src_dyninit", "functions_zlib",
               "data_zlib", "data_vtables", "data_compgen",
               "data_static_libs", "functions_static_libs"]

#: channels whose claimed size is the exact matched extent (overrides derived,
#: bounded by it - the overrun check guards the other direction). Every channel
#: that states a size means it; label-only channels state None.
_SIZE_AUTHORITY = {"src", "src_compgen", "src_dyninit", "functions_zlib",
                   "data_zlib", "data_vtables", "data_static_libs",
                   "data_compgen"}

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
    also: tuple = ()   # other units carrying the same header-inline claim


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


_CANON_S_RE = re.compile(r"\$S[0-9]*$")


def _repair_ordinals(claims: list[Claim], violations: list[str]) -> list[Claim]:
    """Resolve canonical `$S`-suffixed names to cl's CURRENT ordinal spelling.

    No checked-in file may carry a `$S<n>` ordinal (a per-object CodeView
    counter that renumbers on any TU churn), so tables store the ordinal-free
    form and the model re-derives the live one from the unit's base obj -
    accepted only on a UNIQUE ordinal-stripped match, exactly like the old
    _repair_static_ordinal. Unresolvable rows become ONE aggregated violation."""
    from gruntz.core.coff import Coff
    from gruntz.core.paths import BUILD as _B
    objs: dict[str, set[str] | None] = {}
    unresolved: list[str] = []
    out: list[Claim] = []
    strip = lambda n: re.sub(r"\$S[0-9]+", "$S", n)  # noqa: E731
    for c in claims:
        if not (c.name.startswith("_") and _CANON_S_RE.search(c.name)) \
                or c.name.endswith(tuple("0123456789")):
            out.append(c)
            continue
        if c.unit not in objs:
            obj = _B / "objdiff/base" / f"{c.unit}.obj"
            try:
                objs[c.unit] = Coff(obj).all_names() if obj.is_file() else None
            except ValueError:
                objs[c.unit] = None
        syms = objs[c.unit]
        hits = [n for n in syms if strip(n) == c.name] if syms else []
        if len(hits) == 1:
            out.append(c._replace(name=hits[0]))
        else:
            unresolved.append(f"{c.name} [{c.unit}] ({len(hits)} matches)")
            out.append(c)
    if unresolved:
        violations.append(
            f"{len(unresolved)} canonical $S name(s) resolve to no unique obj "
            f"symbol (first: {unresolved[0]}) - a real modelling question each")
    return out


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


def _band_owner_fn():
    """rva -> owning unit per link_order.tsv's contribution bands, or None."""
    import bisect
    try:
        bands = censuses.link_order_bands()
    except Exception:
        return lambda rva: None
    los = [lo for lo, _hi, _u in bands]

    def owner(rva: int):
        i = bisect.bisect_right(los, rva) - 1
        if i >= 0 and bands[i][0] <= rva < bands[i][1]:
            return bands[i][2]
        return None
    return owner


def resolve() -> Model:
    violations: list[str] = []

    fn_rows = {r["rva"]: r for r in censuses.functions()}
    dt_rows = {r["rva"]: r for r in censuses.data()}

    all_claims = [c for c in providers.all_claims() + src_claims.all_claims()
                  if _active(c)]
    unknown = [c for c in all_claims if c.channel not in _PRECEDENCE]
    if unknown:
        violations.append(f"{len(unknown)} claim(s) from unknown channel "
                          f"{unknown[0].channel!r} - skipped")
        all_claims = [c for c in all_claims if c.channel in _PRECEDENCE]
    all_claims = _repair_ordinals(all_claims, violations)
    # A dyninit pin's OWNER is not a symbol cl emits (the body is a volatile
    # _$E<n>); the binding stays unnamed like the old spine, the owner rides
    # as an alias so audits keep it. Keyword owners are a source-hygiene wart,
    # reported ONCE.
    kw_owners = sum(1 for c in all_claims
                    if c.channel == "src_dyninit" and c.name in ("int", "char"))
    if kw_owners:
        violations.append(f"{kw_owners} RVA_DYNINIT pin(s) spell a KEYWORD as "
                          f"the owner ('int') - name the real owning datum")
    # A header-inline definition carries its RVA()/DATA() macro into EVERY
    # including TU, so identical (kind, rva, channel, name) claims arrive from
    # several units. The OWNER is the unit whose retail link band contains the
    # rva (link_order.tsv is the authority); alphabetical only as fallback.
    band_owner = _band_owner_fn()
    merged: dict[tuple, Claim] = {}
    for c in sorted(all_claims, key=lambda c: (c.kind, c.rva, c.channel,
                                               c.name, c.unit)):
        key = (c.kind, c.rva, c.channel, c.name)
        prev = merged.get(key)
        if prev is None:
            merged[key] = c
        elif c.unit and c.unit != prev.unit:
            prev.meta.setdefault("also_units", []).append(c.unit)
    for key, c in merged.items():
        also = c.meta.get("also_units")
        if also:
            owner = band_owner(c.rva)
            if owner and owner != c.unit and owner in also:
                also.remove(owner)
                also.append(c.unit)
                merged[key] = c._replace(unit=owner)
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
        if win.channel == "src_dyninit":
            rest = [win] + list(rest)          # keep the owner pin visible
            win = win._replace(name="")
        allowed = _FUNC_KINDS.get(win.channel, {""})
        if row["kind"] not in allowed:
            violations.append(
                f"func claim {win.name} ({win.channel}) binds kind="
                f"{row['kind']!r} row 0x{rva:06x} (allowed {sorted(allowed)})")
        size = row["size"]
        if win.channel in _SIZE_AUTHORITY and win.size:
            if win.size > row["size"]:
                violations.append(
                    f"claim {win.name} at 0x{rva:06x} size 0x{win.size:x} "
                    f"crosses the next admitted start 0x{rva + row['size']:06x}"
                    f" (derived extent 0x{row['size']:x})")
            else:
                size = win.size
        functions.append(Binding(rva, size, row["kind"], "text",
                                 win.name, win.unit, win.channel, tuple(rest),
                                 tuple(win.meta.get("also_units", ()))))

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
                    f"data claim {win.name} at 0x{rva:06x} size 0x{win.size:x}"
                    f" crosses the next admitted start 0x{rva + row['size']:06x}"
                    f" (derived extent 0x{row['size']:x})")
            else:
                size = win.size
        data.append(Binding(rva, size, row["kind"], row["region"],
                            win.name, win.unit, win.channel, tuple(rest),
                            tuple(win.meta.get("also_units", ()))))

    # the delink data manifest needs ONE name -> ONE extent image-wide
    from collections import Counter
    dup = Counter(b.name for b in data if b.name)
    dups = {n: k for n, k in dup.items() if k > 1}
    if dups:
        first = next(iter(sorted(dups)))
        violations.append(
            f"{len(dups)} data name(s) bind at multiple rvas (first: {first} "
            f"x{dups[first]}) - per-rva alias spellings needed")

    # claims that hit no census row at all
    for (kind, rva), cands in sorted(per_rva.items()):
        for c in cands:
            violations.append(
                f"{kind} claim {c.name} ({c.channel}) at 0x{rva:06x} is not "
                f"an admitted census row")

    return Model(functions, data, violations)


def serialize(model: Model) -> tuple[bool, bool]:
    """Write bindings.tsv (the delink key) + violations.tsv, write-if-changed."""
    header = ["rva", "size", "kind", "space", "name", "unit", "channel",
              "also_units", "aliases"]

    def alias(a):
        return f"{a.channel}:{a.name}:0x{a.size or 0:x}:{a.unit}"

    rows = []
    for b in model.functions + model.data:
        rows.append([f"0x{b.rva:08x}", f"0x{b.size:x}", b.kind, b.space,
                     b.name, b.unit, b.channel, ";".join(b.also),
                     ";".join(alias(a) for a in b.aliases)])
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
