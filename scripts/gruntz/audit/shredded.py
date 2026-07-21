#!/usr/bin/env python3
"""Find SHREDDED OBJECTS: runs of DATA()-pinned "globals" that are really the FIELDS of one
object, declared as separate variables at its interior offsets.

Three instances of the same shredded registry have now been found by hand, each the same way:
a base global at R, then more globals at R+4, R+8, R+0xc, ... whose offsets reproduce a known
class's field map exactly, with the "lookup" function turning out to be that class's method
hand-inlined over the pieces. Two were in GruntVoice.cpp (0x6514d8, 0x651500) and one was the
shared name registry (0x6bf650) - and while clearing the last one off the undefined-DATA list
I nearly DEFINED five of its fields as globals, which would have made the count look better and
the tree wronger.

That is a mechanical shape, so this is a mechanical detector. It reports, for every cluster of
DATA()-pinned globals packed inside one object-sized window, the class whose field map the
cluster's offsets match. A hit is not proof - it is a place to go read the binary - but the
three known cases all fall straight out of it.

    python -m gruntz.audit.shredded            # clusters + their matching class
    python -m gruntz.audit.shredded --all      # every cluster, matched or not
"""
from __future__ import annotations

import argparse
import glob
import json
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]

# A DATA(rva) directly above a declaration/definition of a named global.
DECL = re.compile(
    r"^\s*DATA\(\s*(0x[0-9a-fA-F]+)\s*\)\s*\n"
    r"\s*(?:extern\s+)?(?:\"C\"\s*)?[\w:]+[\s*]+(\w+)\s*(?:\[\s*\])?\s*[;=]",
    re.M,
)

WINDOW = 0x40  # an object big enough to be worth shredding, small enough to stay specific
MIN_MEMBERS = 4  # a base + 3 interior fields; below that a run is just adjacent globals


def data_globals() -> dict:
    """{rva: name} for every DATA()-pinned global in the tree."""
    out = {}
    for g in ("src/**/*.cpp", "src/**/*.h", "include/**/*.h"):
        for f in glob.glob(str(REPO / g), recursive=True):
            text = Path(f).read_text(errors="ignore")
            for m in DECL.finditer(text):
                out[int(m.group(1), 16)] = m.group(2)
    return out


def class_layouts() -> dict:
    """{class: (size, {offset,...})} from the compiler's own record layouts."""
    p = REPO / "build/gen/structs.json"
    if not p.is_file():
        return {}
    out = {}
    for e in json.load(p.open()):
        offs = {f.get("offset", 0) for f in e.get("fields", [])}
        if e.get("size") and offs:
            out[e["name"]] = (e["size"], offs)
    return out


def typed_globals(g: dict) -> dict:
    """{rva: (name, class, size)} for DATA()-pinned globals whose declared type is a class we
    have a layout for. These are the objects; anything pinned INSIDE one is a field of it."""
    layouts = class_layouts()
    decl = re.compile(
        r"^\s*DATA\(\s*(0x[0-9a-fA-F]+)\s*\)\s*\n\s*(?:extern\s+)?(?:\"C\"\s*)?(\w+)\s+(\w+)\s*;",
        re.M,
    )
    out = {}
    for gl in ("src/**/*.cpp", "src/**/*.h", "include/**/*.h"):
        for f in glob.glob(str(REPO / gl), recursive=True):
            for m in decl.finditer(Path(f).read_text(errors="ignore")):
                cls = m.group(2)
                if cls in layouts and layouts[cls][0] > 4:
                    out[int(m.group(1), 16)] = (m.group(3), cls, layouts[cls][0])
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--all", action="store_true", help="show clusters with no matching class")
    a = ap.parse_args()

    g = data_globals()
    layouts = class_layouts()
    if not layouts:
        print("shredded: build/gen/structs.json missing - run `gruntz structs` first "
              "(a stale/absent layout map cannot be trusted)")
        return 0

    # Guessing "which class do these offsets fit" is NOT a usable signal - every struct of
    # dwords has fields at 0/4/8, so it named a dozen irrelevant Win32/MFC types per cluster.
    # Worse, it INVITES fabrication: "offsets fit CKeyedList" is exactly how someone talks
    # themselves into declaring it a CKeyedList. Use the one signal that cannot false-positive:
    # a global DATA()-pinned strictly INSIDE another global's typed extent IS, by construction,
    # an interior field of that object. All four known shreddings fall straight out of it.
    objs = typed_globals(g)
    hits = 0
    for orva, (oname, ocls, osize) in sorted(objs.items()):
        inside = [r for r in g if orva < r < orva + osize]
        if len(inside) < 2:
            continue
        hits += 1
        print(f"\n  {oname} : {ocls}  @ 0x{orva:06x}  (size 0x{osize:x})")
        print(f"      {len(inside)} global(s) are pinned INSIDE it - they are its FIELDS, "
              f"not globals:")
        for r in sorted(inside):
            print(f"      +0x{r - orva:<4x} 0x{r:06x}  {g[r]}")
        print("      => SUBSUME onto the object. Do NOT define the pieces: that fabricates "
              "globals at interior offsets of a real object.")
    if not hits:
        print("shredded: no global is pinned inside another object's extent")
    else:
        print(f"\nshredded: {hits} shredded object(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
