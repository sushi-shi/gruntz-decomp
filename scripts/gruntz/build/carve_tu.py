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

# Leading whitespace is allowed: a TU whose bodies live inside `namespace X {`
# indents every pin, and a column-anchored regex silently sees zero items there.
PIN = re.compile(r"^[ \t]*(?P<macro>RVA|RVA_COMPGEN|DATA|VTBL)"
                 r"\((?P<rva>0x[0-9a-fA-F]+)", re.M)
IDENT = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\b")


def parse_items(text):
    """(prologue, [item], epilogue) where each item is a dict:
         kind : 'fn' (RVA + body) | 'pin' (bare RVA_COMPGEN) | 'data' (DATA/VTBL + def)
         rva  : int
         text : the exact source slice, so re-emitting is byte-faithful
    A `pin` is recognised by having no `{...}` body before the next pin - that is the
    case the older block-splitter mis-read as file-scope junk.

    `epilogue` is the trailing `namespace` closer, which would otherwise ride along
    with whichever item happens to be last and get carried into the middle of the
    file by a re-sort. Peel EXACTLY as many `}` lines as the prologue opens
    namespaces - a greedy peel would swallow the last function's own closing brace
    and leave its body unterminated."""
    marks = [(m.start(), m.group("macro"), int(m.group("rva"), 16))
             for m in PIN.finditer(text)]
    if not marks:
        return text, [], ""
    prologue = text[:marks[0][0]]
    depth = len(re.findall(r"^namespace\b[^\n{]*\{", prologue, re.M))
    lines = text.splitlines(keepends=True)
    cut = len(lines)
    while depth and cut:
        stripped = lines[cut - 1].strip()
        if not stripped or stripped.startswith("//"):
            cut -= 1
            continue
        if stripped.startswith("}") and not lines[cut - 1][:1].isspace():
            cut -= 1
            depth -= 1
            continue
        break
    epilogue = "".join(lines[cut:])
    if epilogue:
        text = text[:len(text) - len(epilogue)]
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

    # A run of pins on CONSECUTIVE lines all attach to the one declaration beneath
    # them, but the two macros behave differently and must not be treated alike:
    #   * a bare RVA() has no name of its own, so it BINDS that declaration - split
    #     them and clang re-binds the rva to whatever declaration ends up next;
    #   * an RVA_COMPGEN() names its symbol explicitly, so it is free-standing and
    #     sorts to its own rva slot (which is what the compgen-order gate wants).
    merged, pending = [], []
    for it in items:
        pin_only = bool(it["text"].strip()) and \
            all(PIN.match(ln) for ln in it["text"].splitlines() if ln.strip())
        if pin_only:
            (merged if it["macro"] == "RVA_COMPGEN" else pending).append(dict(it))
            continue
        if pending and it["macro"] == "RVA_COMPGEN":
            head, rest = it["text"].split("\n", 1)      # detach the stacked pin
            merged.append({**it, "text": head + "\n", "kind": "pin"})
            it = {**it, "text": rest, "macro": pending[0]["macro"]}
        if pending:
            it = {**it, "rva": pending[0]["rva"],
                  "text": "".join(p["text"] for p in pending) + it["text"]}
            pending = []
        merged.append(dict(it))
    merged.extend(pending)
    return prologue, merged, epilogue


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
    prologue, items, epilogue = parse_items(text)
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
            "flags": flags[unit], "src": src[unit], "epilogue": epilogue}


def main(argv=None):
    ap = argparse.ArgumentParser(description="carve a TU at a proven compiland boundary")
    ap.add_argument("--unit", required=True, action="append",
                    help="donor unit (repeatable - several donors COMBINE into one new "
                         "TU, which is what an adjacency group across units needs)")
    ap.add_argument("--out", help="new TU name, e.g. GameAppComdats")
    ap.add_argument("--into", help="RE-HOME instead: append the carved items to this "
                                   "EXISTING unit (use when the surrounding block already "
                                   "belongs to a unit - no new compiland is invented)")
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
    if not args.out and not args.into:
        sys.exit("[carve] need --out (new TU) or --into (re-home into an existing TU)")
    if not plans:
        print("[carve] nothing to do")
        return 0
    if not args.apply:
        print("  (dry run - pass --apply to write)")
        return 0

    carved = sorted((it["rva"], it["text"]) for _u, pl in plans for it in pl["take"])
    if args.into:
        cfg = tomllib.loads((REPO / "config" / "units.toml").read_text())
        dest = {u["unit"]: u["source"] for u in cfg["unit"]}.get(args.into)
        if not dest:
            sys.exit(f"[carve] unknown target unit '{args.into}'")
        dp = REPO / dest
        dtext = dp.read_text()
        # A re-homed body carries its dependencies with it: union the donors'
        # #includes into the destination (the same merge --out does), else the
        # move compiles only if the target happened to include the same set.
        have = set(re.findall(r"^#include .*$", dtext, re.M))
        extra = [ln for _u, pl in plans
                 for ln in re.findall(r"^#include .*$", pl["prologue"], re.M)
                 if ln not in have]
        if extra:
            last = max(m.end() for m in re.finditer(r"^#include .*$", dtext, re.M))
            dtext = dtext[:last] + "\n" + "\n".join(dict.fromkeys(extra)) + dtext[last:]
            print(f"    merged {len(set(extra))} include(s) into {dest}")
        # insert each item at its RVA-sorted slot so the file stays ascending
        for rva, txt in carved:
            # CODE pins only: a DATA()/VTBL() RVA is in .data (0x0024xxxx) and
            # outranks every .text RVA, so including them puts the body at the
            # top of the file and breaks the ascending-RVA invariant.
            marks = [(m.start(), int(m.group("rva"), 16)) for m in PIN.finditer(dtext)
                     if m.group("macro") in ("RVA", "RVA_COMPGEN")]
            at = next((pos for pos, r in marks if r > rva), len(dtext))
            dtext = dtext[:at] + txt.rstrip() + "\n\n" + dtext[at:]
        dp.write_text(dtext)
        for _u, pl in plans:
            pl["path"].write_text(pl["prologue"]
                                  + "".join(it["text"] for it in pl["keep"])
                                  + pl["epilogue"])
        print(f"  re-homed {len(carved)} item(s) into {args.into} ({dest})")
        return 0
    # MERGE every donor's prologue: a combined TU needs the union of their includes,
    # not just the first donor's (that compiles only by luck).
    seen, merged = set(), []
    for _u, pl in plans:
        for ln in pl["prologue"].splitlines(keepends=True):
            key = ln.strip()
            if key.startswith("#include"):
                if key in seen:
                    continue
                seen.add(key)
            elif key and key in seen:
                continue
            elif key:
                seen.add(key)
            merged.append(ln)
    prologue = "".join(merged)
    newsrc = Path(plans[0][1]["src"]).parent / f"{args.out}.cpp"
    (REPO / newsrc).write_text(prologue + "".join(txt for _r, txt in carved))
    for _u, pl in plans:
        pl["path"].write_text(pl["prologue"]
                              + "".join(it["text"] for it in pl["keep"]) + pl["epilogue"])
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
