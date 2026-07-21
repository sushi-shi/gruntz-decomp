"""gruntz.core.manifest - config/units.toml, parsed once per process.

The per-TU manifest is the single source of truth for unit -> source/flags
(docs/build-system.md). Every reader goes through here; the raw document is
cached so N tools in one process (batch sema, the build tail) parse once.
"""
import tomllib

from gruntz.core.pe import REPO

MANIFEST = REPO / "config" / "units.toml"

_DOC = None


def load() -> dict:
    """The whole parsed manifest (cached)."""
    global _DOC
    if _DOC is None:
        with MANIFEST.open("rb") as f:
            _DOC = tomllib.load(f)
    return _DOC


def units() -> list[dict]:
    """The [[unit]] blocks."""
    return load().get("unit", [])


def flags_for(udef: dict) -> list:
    """Resolve a unit's flags-profile name to its cl flag list ([flags])."""
    return list(load().get("flags", {}).get(udef.get("flags", ""), []))


def unit_names() -> set:
    return {u["unit"] for u in units()}


def by_unit() -> dict:
    """unit stem -> its [[unit]] block."""
    return {u["unit"]: u for u in units()}


def source_to_unit() -> dict:
    """src path (as written in the manifest) -> unit stem."""
    return {u["source"]: u["unit"] for u in units() if u.get("source")}
