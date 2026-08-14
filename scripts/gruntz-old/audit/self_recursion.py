#!/usr/bin/env python3
"""self_recursion.py - the seam-extraction footgun ratchet.

Extracting a cast into a named seam is the campaign's main cast-removal move:

    inline u16* Scratch16() { return reinterpret_cast<u16*>(g_scratch); }

and then a sweep rewrites every `reinterpret_cast<u16*>(g_scratch)` in the tree
to `Scratch16()`. The sweep matches the seam's OWN body too, which silently
turns it into

    inline u16* Scratch16() { return Scratch16(); }

That COMPILES. Neither cl nor the objdiff gate can see it - the function just
recurses forever at runtime, and any caller's bytes quietly stop matching
retail. It happened twice in one session (Scratch16 with 26 callers, ActFindId
with ~90); fixing both moved +5 exacts and +0.19 fuzzy, which is how it was
finally noticed.

So: a one-line accessor whose only statement calls ITSELF with the same
arguments is always this bug, never intent. FATAL, no allowlist - a genuine
recursive helper has a base case and therefore more than one statement.

  python -m gruntz.audit.self_recursion          # report
  python -m gruntz.audit.self_recursion --gate   # exit 1 on any hit (build tail)
"""
import argparse
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parents[3]
ROOTS = ("src", "include")

# `<ret> NAME(args) [const] { return NAME(...); }` - free function, member, or
# out-of-line definition, body on one line or wrapped over a few.
_SELF = re.compile(
    r"\b(?P<name>[A-Za-z_]\w*)\s*\([^;{)]*\)\s*(?:const\s*)?\{\s*"
    r"return\s+(?P=name)\s*\(",
    re.S,
)


def scan(path: pathlib.Path):
    text = path.read_text(errors="replace")
    for m in _SELF.finditer(text):
        name = m.group("name")
        if name in ("if", "for", "while", "switch", "return", "sizeof"):
            continue
        line = text.count("\n", 0, m.start()) + 1
        body = " ".join(text[m.start():m.end() + 60].split())
        yield line, name, body[:110]


def violations():
    out = []
    for root in ROOTS:
        for path in sorted((REPO / root).rglob("*")):
            if path.suffix not in (".cpp", ".h"):
                continue
            for line, name, body in scan(path):
                out.append(
                    f"{path.relative_to(REPO)}:{line}: '{name}' returns a call to "
                    f"itself - a seam sweep rewrote the seam's own body\n    {body}"
                )
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 on any violation (build-tail ratchet)")
    args = ap.parse_args()
    viol = violations()
    for v in viol:
        print(v)
    if viol:
        print(f"self-recursion: {len(viol)} accessor(s) call themselves")
        return 1 if args.gate else 0
    print("self-recursion: OK - no accessor returns a call to itself")
    return 0


if __name__ == "__main__":
    sys.exit(main())
