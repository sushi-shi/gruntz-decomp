"""gruntz.core.dyninit - the RVA_DYNINIT source pins for `$E` dynamic initializers.

MSVC 5's `$E<n>` dynamic-init helpers carry a VOLATILE emission ordinal, not a
semantic identity - the ordinal shifts whenever declarations or emission order
change, so it must never be stored or bound (labels.py rejects it as an
RVA_COMPGEN symbol). The stable identity of such a helper is its OWNER: the
file-scope object whose dynamic initializer it is. That is where the pin
lives - a no-op macro at the owner's definition:

    RVA_DYNINIT(0x00008040, 0xa, g_actionAreaSpawner)

anchored in source exactly like homm2's DATA_COMPGEN_GUARD, scraped lexically
here. The current build's `$E` name for a pinned rva is DERIVED from the
emitting TU's base obj when an audit needs it; it is never persisted.

This module replaced config/retail/compiler-generated-functions.tsv, whose
`observed_name` column was one build's ordinal frozen forever and whose
`former_source` file:line rotted as sources moved. A source-anchored pin moves
with its owner.

Placement is deliberately EXEMPT from compgen_order's rva-order ratchet: the
pin follows the OWNER's definition, not the TU's function label sequence.
"""
from __future__ import annotations

import re
from functools import lru_cache
from pathlib import Path

DYNINIT_RE = re.compile(
    r"RVA_DYNINIT\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*,"
    r"\s*([A-Za-z_][A-Za-z0-9_:]*)\s*\)")


def _unit_map(repo: Path) -> dict[str, str]:
    """source path -> unit name, from config/units.toml (stem fallback)."""
    toml_path = repo / "config/units.toml"
    if not toml_path.is_file():
        return {}
    import tomllib
    doc = tomllib.load(toml_path.open("rb"))
    out = {}
    for u in doc.get("unit", []):
        src, name = u.get("source"), u.get("unit")
        if src and name:
            out[src] = name
    return out


@lru_cache(maxsize=4)
def rows(repo: Path) -> tuple[dict, ...]:
    """Every RVA_DYNINIT pin in the tree, as
    {rva, size, owner, unit, where} sorted by rva.

    `unit` comes from config/units.toml for the containing source; a file
    outside the manifest falls back to its stem (test fixtures)."""
    repo = Path(repo)
    units = _unit_map(repo)
    out = []
    for base in ("src", "include"):
        root = repo / base
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix not in (".cpp", ".h"):
                continue
            try:
                text = path.read_text(encoding="utf-8", errors="replace")
            except OSError:
                continue
            rel = path.relative_to(repo).as_posix()
            for i, line in enumerate(text.splitlines(), 1):
                m = DYNINIT_RE.search(line)
                if not m:
                    continue
                out.append({
                    "rva": int(m.group(1), 16),
                    "size": int(m.group(2), 0),
                    "owner": m.group(3),
                    "unit": units.get(rel, path.stem.lower()),
                    "where": f"{rel}:{i}",
                })
    out.sort(key=lambda r: r["rva"])
    seen: dict[int, dict] = {}
    for r in out:
        if r["rva"] in seen:
            raise ValueError(
                f"duplicate RVA_DYNINIT 0x{r['rva']:08x}: {seen[r['rva']]['where']} "
                f"and {r['where']}")
        seen[r["rva"]] = r
    return tuple(out)
