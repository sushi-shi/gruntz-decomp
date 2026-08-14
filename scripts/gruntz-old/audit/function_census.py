#!/usr/bin/env python3
"""gruntz.audit.function_census - labelled retail functions may not vanish.

`config/retail/gruntz_functions.tsv` is the committed census of every retail
function RVA the source tree labels: rva, current name, owning unit. The gate
compares it against the build's `build/gen/symbol_names.csv`; an RVA present in
the census but absent from the build is a LOST function and fails the audit.

Why this must be a gate: losing a labelled function shrinks every metric's
denominator, which makes the numbers look BETTER while the tree gets WORSE, and
nothing else in the build can notice because every metric is computed from the
very file that just lost the rows. The floor is therefore the union of HEAD's
census (a value no build could have written) and the working-tree census (which
holds an uncommitted within-session gain).

The gate keys on RVAs alone, so a move between units and a rename are NOT
losses - the census row simply follows the label. (The predecessor,
`config/labels_manifest.tsv`, kept per-unit COUNTS and could not tell a move
from a loss, so moves needed a GRUNTZ_LABELS_ACK acknowledgement; keyed by RVA
the distinction is structural and the env var is retired.) The one legitimate
removal is a label that was WRONG - e.g. pinned to a COMDAT retail never
emitted: delete its row and COMMIT the deletion; an uncommitted deletion does
not lower the floor.

USAGE
    python -m gruntz.audit.function_census    # gate, then refresh the census
"""
from __future__ import annotations

import csv
import subprocess
import sys
from pathlib import Path

from gruntz.core.pe import REPO

CENSUS = REPO / "config/retail/gruntz_functions.tsv"
SYMBOLS = REPO / "build/gen/symbol_names.csv"

HEADER = (
    "# gruntz_functions.tsv - census of every retail function RVA the source\n"
    "# tree labels (rva, current name, owning unit). BUILD-INTEGRITY floor: an\n"
    "# RVA that vanishes from a build fails `gruntz build --full`; see\n"
    "# gruntz.audit.function_census. Moves and renames are not losses - the row\n"
    "# follows the label. The floor is HEAD's copy of this file unioned with\n"
    "# the working tree, so deliberately removing a WRONG label means deleting\n"
    "# its row here and committing the deletion.\n"
    "rva\tname\tunit\n"
)


def _parse(text: str) -> dict[int, tuple[str, str]]:
    """``{rva: (name, unit)}`` from census text; comments/header tolerated."""
    out: dict[int, tuple[str, str]] = {}
    for line in (text or "").splitlines():
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        rva_s, _, rest = line.partition("\t")
        name, _, unit = rest.partition("\t")
        try:
            out[int(rva_s, 16)] = (name, unit)
        except ValueError:
            continue
    return out


def _head_census() -> dict[int, tuple[str, str]]:
    """HEAD's copy via git - the one floor no build could have written."""
    try:
        r = subprocess.run(
            ["git", "-C", str(REPO), "show",
             f"HEAD:{CENSUS.relative_to(REPO).as_posix()}"],
            capture_output=True, text=True, timeout=30)
        if r.returncode == 0:
            return _parse(r.stdout)
    except (OSError, subprocess.SubprocessError):
        pass
    return {}


def current() -> dict[int, tuple[str, str]]:
    """``{rva: (name, unit)}`` for every labelled function in the build."""
    if not SYMBOLS.is_file():
        sys.exit(f"no {SYMBOLS.relative_to(REPO)} - run `gruntz build` first")
    out: dict[int, tuple[str, str]] = {}
    with SYMBOLS.open(newline="") as stream:
        for row in csv.DictReader(
                line for line in stream if not line.lstrip().startswith("#")):
            if (row.get("kind") or "func") == "data":
                continue
            out[int(row["rva"], 16)] = (row["name"], row["unit"])
    return out


def main() -> int:
    cur = current()
    work = _parse(CENSUS.read_text()) if CENSUS.is_file() else {}
    floor = {**_head_census(), **work}

    lost = sorted(rva for rva in floor if rva not in cur)
    if lost:
        for rva in lost:
            name, unit = floor[rva]
            print(f"ERROR 0x{rva:06x} {name} ({unit}): labelled retail function "
                  f"VANISHED from the build", file=sys.stderr)
        print(f"{len(lost)} labelled function(s) lost vs "
              f"{CENSUS.relative_to(REPO)}. A move between units or a rename is "
              f"NOT a loss (the gate keys on RVAs), so this is a label that "
              f"stopped being emitted. If the label itself was WRONG, delete its "
              f"row and COMMIT the deletion - the floor includes HEAD's copy, so "
              f"an uncommitted deletion does not stick.", file=sys.stderr)
        return 1

    body = "".join(f"0x{rva:06x}\t{name}\t{unit}\n"
                   for rva, (name, unit) in sorted(cur.items()))
    new = HEADER + body
    if not CENSUS.is_file() or CENSUS.read_text() != new:
        CENSUS.write_text(new)
        gained = len(cur) - len(floor)
        units = len({unit for _name, unit in cur.values()})
        print(f"function census: {len(cur)} labelled retail function(s) across "
              f"{units} unit(s)" + (f"  (+{gained})" if gained > 0 else ""))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
