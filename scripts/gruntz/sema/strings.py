"""gruntz.sema.strings - the string literals a function reaches, and back.

    python3 -m gruntz.sema.strings 0x153790         # a function: its literals
    python3 -m gruntz.sema.strings 0x20bcf4         # a datum: the literal there
    python3 -m gruntz.sema.strings --find GRUNTZ_NORMALGRUNT
    python3 -m gruntz.sema.strings --ranked         # unclaimed fns worth naming

Built from the relocation map, so a hit is a REAL reference: the operand's
address was fixed up by the linker. Each literal is reported with the census
row that owns it - a pooled literal cl emitted with no source spelling shows
as `bare pooled literal`, which is the normal shape for a `??_C@` string that
no DATA() claim names.
"""

from __future__ import annotations

import re
import sys
from functools import lru_cache

from gruntz.sema import die, parse_rva, run
from gruntz.sema.image import retail
from gruntz.sema.index import index

DISTINCTIVE = re.compile(
    r"GRUNTZ|AREA|STAGE|WORLDZ?|QUESTZ|TOOLZ|TOYZ|WARLORDZ|POWERUPZ|\.wwd|\.rez|"
    r"\.vob|\.SF2|BOOTY|MULTI|STATEZ|SECRET|TELEPORT|CHEAT|DDERR_|DIERR_|DSERR_|"
    r"CURSORZ|DEATHZ", re.I)


@lru_cache(maxsize=1)
def by_function() -> dict[int, list[tuple[int, str]]]:
    """{function rva: [(string rva, text)]} over every relocated reference from
    .text into a printable run."""
    idx, img = index(), retail()
    out: dict[int, list[tuple[int, str]]] = {}
    for site, tgt in img.reloc.items():
        if not img.is_text(site) or img.is_text(tgt):
            continue
        s = img.string_at(tgt)
        if s is None or s[0] != tgt:
            continue                       # only a run's START is a literal
        fn = idx.owner(site)
        if fn is None:
            continue
        row = out.setdefault(fn.rva, [])
        if (tgt, s[1]) not in row:
            row.append((tgt, s[1]))
    for row in out.values():
        row.sort()
    return out


def owner_note(rva: int) -> str:
    """How the Model spells the row holding a literal."""
    idx = index()
    b = idx.at(rva)
    if b is None:
        b = idx.data_owner(rva)
        if b is None:
            return "no admitted row"
        return f"inside {idx.display(b, rva)}+0x{rva - b.rva:x}"
    if b.name:
        return f"{b.name} [{b.unit or '-'}] ({b.channel})"
    return f"bare pooled literal (census kind={b.kind or 'plain'}, 0x{b.size:x} B)"


def show_function(rva: int) -> tuple[list[str], int]:
    idx = index()
    fn = idx.func(rva) or idx.owner(rva)
    if fn is None:
        return ([f"0x{rva:08x} is in no admitted function row"], 1)
    hits = by_function().get(fn.rva, [])
    out = [f"### {idx.label(fn.rva)}  0x{fn.size:x} B  - {len(hits)} literal(s)"]
    for saddr, text in hits:
        out.append(f"   0x{saddr:08x}  {text!r}")
        out.append(f"              {owner_note(saddr)}")
    return out, (0 if hits else 1)


def show_datum(rva: int) -> tuple[list[str], int]:
    idx, img = index(), retail()
    s = img.string_at(rva)
    if s is None:
        return ([f"0x{rva:08x} holds no printable run"], 1)
    start, text = s
    out = [f"0x{start:08x}  {text!r}"
           + (f"   (queried 0x{rva:08x} = +0x{rva - start:x})" if start != rva else ""),
           f"  row       : {owner_note(start)}"]
    users = []
    for site in img.referents.get(start, ()):
        fn = idx.owner(site)
        users.append(f"     @0x{site:06x}  "
                     + (idx.label(fn.rva) if fn is not None
                        else f"in {img.section_name(site)} (no function row)"))
    out.append(f"  referenced: {len(users)} site(s)")
    out += users[:24]
    if len(users) > 24:
        out.append(f"     ... (+{len(users) - 24} more)")
    return out, 0


def find(needle: str) -> tuple[list[str], int]:
    idx, img = index(), retail()
    needle = needle.lower()
    out, n = [], 0
    for start in sorted(img.strings):
        text = img.strings[start]
        if needle not in text.lower():
            continue
        n += 1
        users = [idx.owner(site) for site in img.referents.get(start, ())]
        named = [u for u in users if u is not None]
        out.append(f"0x{start:08x}  {text!r}")
        out.append(f"     {owner_note(start)}")
        for fn in named[:6]:
            out.append(f"     <- {idx.label(fn.rva)}")
        if len(users) > len(named):
            out.append(f"     <- {len(users) - len(named)} site(s) outside any "
                       "function row")
        if not users:
            out.append("     <- (no relocated reference: unreferenced or "
                       "reached by computed address)")
    return out, (0 if n else 1)


def ranked(limit: int = 60) -> list[str]:
    """Unclaimed functions whose literals would name them - the labeling aid."""
    idx = index()
    rows = []
    for rva, hits in by_function().items():
        b = idx.func(rva)
        if b is None or b.name:
            continue
        good = [t for _a, t in hits if len(t) >= 4]
        score = sum(3 if DISTINCTIVE.search(t) else 1 for t in good)
        if score:
            rows.append((score, rva, b.size, good))
    rows.sort(key=lambda r: (-r[0], r[1]))
    out = []
    for score, rva, size, good in rows[:limit]:
        head = " | ".join(good[:6]) + (f" (+{len(good) - 6})" if len(good) > 6 else "")
        out.append(f"[{score:4d}] 0x{rva:06x} sz={size:<5d} {head}")
    out.append(f"[{len(rows)} unclaimed function(s) reference a literal]")
    return out


def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="gruntz sema strings",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("rva", nargs="*", help="function or datum rvas")
    ap.add_argument("--find", help="substring search over the image's literals")
    ap.add_argument("--ranked", action="store_true",
                    help="unclaimed functions ranked by literal distinctiveness")
    args = ap.parse_args(argv)
    if args.find:
        lines, rc = find(args.find)
        print("\n".join(lines) or f"no literal contains {args.find!r}")
        return rc
    if args.ranked or not args.rva:
        if not args.rva and not args.ranked:
            die("give an <rva>, --find <text> or --ranked")
        print("\n".join(ranked()))
        return 0
    rc = 0
    for token in args.rva:
        rva = parse_rva(token)
        lines, r = (show_function(rva) if retail().is_text(rva)
                    else show_datum(rva))
        print("\n".join(lines))
        rc = rc or r
    return rc


if __name__ == "__main__":
    sys.exit(run(__name__, sys.argv[1:]))
