"""Resolve every asset-registry key the tree names against the shipped archive.

The engine installs REZ subtrees into named registries with a separator, e.g.
``InstallTree(GAME\\IMAGEZ, "GAME", "_")`` in
``CState::LoadGameAssetNamespaces`` @0xf9ea0 and
``ScanTree(AREA<n>\\SOUNDZ, "LEVEL", "_")`` in ``PlayAssetLoad.cpp``. A key such
as ``GAME_WAPWORLDONLY_TRIGGER`` is therefore the path
``GAME\\IMAGEZ\\WAPWORLDONLY\\TRIGGER`` with the separators substituted -- which
means a key is checkable against the archive's name table.

Two inputs, one predicate:

* ``--src`` scans ``src/`` + ``include/`` for ``"<NS>_..."`` string literals.
  An unresolved literal is either a reconstruction error or (like ``GAME_WAWA``)
  a genuinely dead reference retail shipped.
* ``--keys`` takes a newline- or TSV-delimited list, e.g. the ``image_set`` /
  ``animation`` columns of ``gruntz.audit.wwd_objects``.

Names are ambiguous because ``_`` is both the separator and a legal name
character, so every grouping of the ``_``-split is tried; a key resolves if any
grouping names a resource or a directory.

Usage::

    python -m gruntz.audit.asset_keys --names <rezls-grep-output> --src <repo>
    python -m gruntz.audit.asset_keys --names <names.txt> --keys keys.txt
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

# The subtree a registry install rooted at: `<NS>\<SUB>\...`. A key never names
# the subtree itself, so every candidate gets one of these spliced in -- plus
# the empty one, for `ResolvePath("GAME_IMAGEZ")`-style namespace roots.
SUBTREES = ("IMAGEZ", "SOUNDZ", "ANIZ", "TILEZ", "WORLDZ", "MIDIZ", "PALETTEZ", "SCREENZ", "")

# Registry prefixes and the archive namespace(s) they were installed from.
PREFIX_ROOTS = {
    "GAME": ("GAME",),
    "GRUNTZ": ("GRUNTZ",),
    "STATEZ": ("STATEZ",),
    # `ScanTree(AREA<n>\..., "LEVEL", "_")` -- which area depends on the level.
    "LEVEL": tuple(f"AREA{n}" for n in range(1, 9)),
}

KEY_RE = re.compile(r'"((?:GAME|LEVEL|GRUNTZ|STATEZ)_[A-Z0-9_]{2,})"')


def load_names(path: pathlib.Path) -> tuple[set, set]:
    """Read `rezls <archive> grep ''` output into (resource paths, directories)."""
    row = re.compile(r"^(\w+)\s+(\d+)\s+([0-9a-f]+)\s+(.*)$")
    paths = set()
    for line in path.read_text(errors="replace").splitlines():
        m = row.match(line)
        if m:
            paths.add(m.group(4))
    dirs = set()
    for p in paths:
        parts = p.split("\\")
        for i in range(1, len(parts)):
            dirs.add("\\".join(parts[:i]))
    return paths, dirs


def candidates(key: str):
    prefix, _, rest = key.partition("_")
    roots = PREFIX_ROOTS.get(prefix)
    if not roots:
        return
    parts = rest.split("_")
    for root in roots:
        for sub in SUBTREES:
            base = root + ("\\" + sub if sub else "")
            # every split point between "directory part" and "leaf name"
            yield base + "\\" + rest
            for i in range(1, len(parts)):
                yield base + "\\" + "\\".join(parts[:i]) + "\\" + "_".join(parts[i:])
            yield base + "\\" + "\\".join(parts)


def resolve(key: str, paths: set, dirs: set):
    for c in candidates(key):
        if c in paths:
            return c, "resource"
        if c in dirs:
            return c, "directory"
    return None, None


def harvest_src(repo: pathlib.Path) -> dict:
    out: dict = {}
    for base in ("src", "include"):
        for f in (repo / base).rglob("*"):
            if f.suffix not in (".cpp", ".h"):
                continue
            try:
                text = f.read_text(errors="replace")
            except OSError:
                continue
            for m in KEY_RE.finditer(text):
                out.setdefault(m.group(1), set()).add(str(f.relative_to(repo)))
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--names", type=pathlib.Path, required=True, help="rezls grep '' output")
    ap.add_argument("--src", type=pathlib.Path, help="repo root; harvest keys from src/ + include/")
    ap.add_argument("--keys", type=pathlib.Path, help="extra keys, one per line")
    args = ap.parse_args(argv)

    paths, dirs = load_names(args.names)
    keys: dict = {}
    if args.src:
        keys.update(harvest_src(args.src))
    if args.keys:
        for line in args.keys.read_text().splitlines():
            k = line.split("\t")[0].strip()
            if k:
                keys.setdefault(k, set()).add(str(args.keys))

    unresolved = []
    for k in sorted(keys):
        hit, _ = resolve(k, paths, dirs)
        if hit is None:
            unresolved.append(k)
    print(f"[asset_keys] {len(keys) - len(unresolved)}/{len(keys)} keys resolve against {args.names}")
    for k in unresolved:
        print(f"  UNRESOLVED  {k}\t{', '.join(sorted(keys[k]))}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
