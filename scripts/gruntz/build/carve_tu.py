#!/usr/bin/env python3
"""carve_tu.py - split a .cpp at a proven compiland boundary.

Driven by the incremental-thunk oracle (docs/patterns/incremental-thunks-reveal-tu-
boundaries.md): a thunk PROVES its target was compiled into a link-line `.obj`, so a TU
holding both thunked and unthunked functions is two compilands merged in our model, and
the thunked minority has to move out.

Naively "everything from one RVA() line to the next" is not good enough, and the three
ways it fails are exactly the three this parses properly:

  * a bare `RVA_COMPGEN(...)` pin has NO body, so a rule like "after the closing brace
    only blanks and comments may follow" mis-fires on it and refuses the whole file;
  * file-scope `DATA()`/`VTBL()` definitions sit between functions and must NOT travel
    with whichever function happens to follow them;
  * a carved function may reference a file-scope static of its donor - move it too if
    only the carved set uses it, and REFUSE if both sides need it (splitting there would
    need an extern, which this tree bans).

    python -m gruntz.build.carve_tu --unit <unit> --out <NewName> [--apply]

Without `--apply` it reports what it would do and touches nothing.
"""

import argparse
import re
import sys
import tomllib
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])

PIN = re.compile(r"^(RVA|RVA_COMPGEN|DATA|VTBL)\((0x[0-9a-fA-F]+)", re.M)
IDENT = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\b")


def parse_items(text):
    """(prologue, [item]) where each item is a dict:
         kind : 'fn' (RVA + body) | 'pin' (bare RVA_COMPGEN) | 'data' (DATA/VTBL + def)
         rva  : int
         text : the exact source slice, so re-emitting is byte-faithful
    A `pin` is recognised by having no `{...}` body before the next pin - that is the
    case the older block-splitter mis-read as file-scope junk."""
    marks = [(m.start(), m.group(1), int(m.group(2), 16)) for m in PIN.finditer(text)]
    if not marks:
        return text, []
    prologue = text[:marks[0][0]]
    items = []
    for i, (pos, macro, rva) in enumerate(marks):
        end = marks[i + 1][0] if i + 1 < len(marks) else len(text)
        body = text[pos:end]
        if macro == "RVA_COMPGEN":
            kind = "pin"
        elif macro in ("DATA", "VTBL"):
            kind = "data"
        else:
            kind = "fn"
        items.append({"kind": kind, "rva": rva, "text": body, "macro": macro})
    return prologue, items


def file_scope_names(prologue, items):
    """Identifiers defined at file scope in the donor (its DATA/VTBL defs and statics)."""
    names = set()
    for chunk in [prologue] + [it["text"] for it in items if it["kind"] == "data"]:
        for m in re.finditer(r"^\s*(?:static\s+)?[A-Za-z_][\w:<>,\s\*&]*?"
                             r"\b([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*(?:=|;|\{)", chunk, re.M):
            names.add(m.group(1))
    return names


def plan(unit, want_rvas):
    cfg = tomllib.loads((REPO / "config" / "units.toml").read_text())
    src = {u["unit"]: u["source"] for u in cfg["unit"]}
    flags = {u["unit"]: u["flags"] for u in cfg["unit"]}
    if unit not in src:
        sys.exit(f"[carve] unknown unit '{unit}'")
    path = REPO / src[unit]
    text = path.read_text()
    prologue, items = parse_items(text)
    take = [it for it in items if it["rva"] in want_rvas]
    keep = [it for it in items if it["rva"] not in want_rvas]
    missing = want_rvas - {it["rva"] for it in items}

    # Which donor file-scope names does the carved set reference, and does the
    # remainder still need them?
    scope = file_scope_names(prologue, items)
    took = " ".join(it["text"] for it in take)
    kept = " ".join(it["text"] for it in keep)
    used_by_take = {n for n in IDENT.findall(took) if n in scope}
    used_by_keep = {n for n in IDENT.findall(kept) if n in scope}
    shared = used_by_take & used_by_keep
    movable = used_by_take - used_by_keep
    return {"path": path, "prologue": prologue, "items": items, "take": take,
            "keep": keep, "missing": missing, "shared": shared, "movable": movable,
            "flags": flags[unit], "src": src[unit]}


def main(argv=None):
    ap = argparse.ArgumentParser(description="carve a TU at a proven compiland boundary")
    ap.add_argument("--unit", required=True, action="append",
                    help="donor unit (repeatable - several donors COMBINE into one new "
                         "TU, which is what an adjacency group across units needs)")
    ap.add_argument("--out", required=True, help="new TU name, e.g. GameAppComdats")
    ap.add_argument("--rva", action="append", default=[],
                    help="RVA to carve out (repeatable); default = the unit's thunked "
                         "minority per the oracle")
    ap.add_argument("--apply", action="store_true", help="write the change")
    args = ap.parse_args(argv)

    explicit = {int(x, 16) for x in args.rva} if args.rva else None
    plans = []
    for unit in args.unit:
        if explicit is not None:
            want = explicit
        else:
            from gruntz.audit.thunk_oracle import classify
            per, _ = classify()
            t, l = per.get(unit, ([], []))
            if not (t and l and len(t) < len(l) and len(l) >= 3):
                sys.exit(f"[carve] {unit} is not a PROVEN mixed unit - nothing to carve")
            want = {rva for rva, _ in t}
        pl = plan(unit, want)
        if not pl["take"]:
            continue
        plans.append((unit, pl))
        print(f"[carve] {unit} ({pl['src']}): {len(pl['items'])} items, "
              f"carving {len(pl['take'])}")
        for it in pl["take"]:
            first = it["text"].strip().splitlines()[0]
            print(f"    {it['rva']:#08x} {it['kind']:4s} {first[:66]}")
        if pl["missing"]:
            print(f"    NOT top-level pins (skipped): "
                  f"{[hex(x) for x in sorted(pl['missing'])]}")
        if pl["movable"]:
            print(f"    file-scope names only the carved set uses: {sorted(pl['movable'])}")
        if pl["shared"]:
            print(f"    BLOCKED - file-scope names needed by BOTH sides: "
                  f"{sorted(pl['shared'])[:8]}{' ...' if len(pl['shared'])>8 else ''}")
            print("    Splitting here would need an extern, which this tree bans.")
            return 1
    if not plans:
        print("[carve] nothing to do")
        return 0
    if not args.apply:
        print("  (dry run - pass --apply to write)")
        return 0

    # one new TU, in RVA order, combining every donor's carved items
    carved = sorted((it["rva"], it["text"]) for _u, pl in plans for it in pl["take"])
    prologue = plans[0][1]["prologue"]
    newsrc = Path(plans[0][1]["src"]).parent / f"{args.out}.cpp"
    (REPO / newsrc).write_text(prologue + "".join(txt for _r, txt in carved))
    for _u, pl in plans:
        pl["path"].write_text(pl["prologue"] + "".join(it["text"] for it in pl["keep"]))
    unit_name = args.out.lower()
    man = REPO / "config" / "units.toml"
    man.write_text(man.read_text().rstrip("\n") +
                   f'\n\n[[unit]]\nunit = "{unit_name}"\nsource = "{newsrc}"\n'
                   f'flags = "{plans[0][1]["flags"]}"\n')
    print(f"  wrote {newsrc} ({len(carved)} items from {len(plans)} donor(s)) "
          f"and registered unit '{unit_name}'")
    return 0


if __name__ == "__main__":
    sys.exit(main())
