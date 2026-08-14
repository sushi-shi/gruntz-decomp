#!/usr/bin/env python3
"""dual_definition.py - find functions DEFINED more than once in the tree.

A member defined both in a class header (implicitly inline) and out of line in a
.cpp, or defined out of line in two places, is an ODR violation. It compiles only
while no translation unit sees both definitions, so it is invisible to the build
until an include is added - at which point cl 5.0 stops with

    error C2084: function 'X::f(void)' already has a body

These arise when a per-TU inline-visibility split is modelled by giving one TU a
satellite header with a second copy of the body (see
docs/patterns/two-shapes-need-two-entities.md for the shape that does NOT need
one, and zero-emission-statements-cross-the-ob1-cb-exemption.md for why a callee
sometimes has to be declinable instead).

Usage:
    python3 -m gruntz.audit.dual_definition            # report
    python3 -m gruntz.audit.dual_definition --gate     # exit 1 if any exist
"""
from __future__ import annotations

import re
import sys
from collections import defaultdict
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])

# An out-of-class definition at column 0: [inline] <ret> Class::name(args) [const] {
OUT_OF_CLASS = re.compile(
    r"^(?P<inline>inline\s+)?(?:[\w:*&<>\[\]]+\s+)*?"
    r"(?P<cls>\w+)::(?P<fn>~?\w+|operator[^\s(]*)\s*\(", re.M)
# A class/struct opening a scope we must track to spot in-class bodies.
CLASS_OPEN = re.compile(r"^\s*(?:class|struct)\s+(\w+)\b[^;{]*\{")


def normalize(params: str) -> str:
    """Collapse a parameter list to a comparable shape: types only, no names."""
    p = re.sub(r"//.*", "", params)
    p = re.sub(r"\s+", " ", p).strip()
    p = re.sub(r"\b(\w+)\s*(?=[,)]|$)", "", p)          # drop parameter names
    p = p.replace(" ", "").replace("const", "").replace("class", "").replace("struct", "")
    return p


def param_text(text: str, open_paren: int) -> str | None:
    depth, i = 0, open_paren
    while i < len(text):
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0:
                return text[open_paren + 1:i]
        elif text[i] == ";":
            return None
        i += 1
    return None


def has_body(text: str, close_paren: int) -> bool:
    """A definition is followed by `{` (possibly past a mem-init list or const)."""
    tail = text[close_paren:close_paren + 400]
    tail = re.sub(r"//.*", "", tail)
    for ch in tail:
        if ch == "{":
            return True
        if ch == ";":
            return False
    return False


def scan():
    defs = defaultdict(list)
    for root in ("include", "src"):
        for path in sorted((REPO / root).rglob("*")):
            if path.suffix not in (".h", ".cpp") or "/Stub/" in str(path):
                continue
            text = path.read_text(errors="replace")
            lines = text.splitlines()
            # --- out-of-class definitions ---
            for m in OUT_OF_CLASS.finditer(text):
                op = text.index("(", m.end() - 1)
                params = param_text(text, op)
                if params is None:
                    continue
                cp = op + len(params) + 1
                if not has_body(text, cp):
                    continue
                line = text[:m.start()].count("\n") + 1
                defs[(m.group("cls"), m.group("fn"), normalize(params))].append(
                    (str(path.relative_to(REPO)), line, bool(m.group("inline")), "out-of-class"))
            # --- in-class bodies (implicitly inline) ---
            cls, depth = None, 0
            for i, ln in enumerate(lines):
                if cls is None:
                    cm = CLASS_OPEN.match(ln)
                    if cm:
                        cls, depth = cm.group(1), ln.count("{") - ln.count("}")
                        continue
                else:
                    depth += ln.count("{") - ln.count("}")
                    if depth <= 0:
                        cls = None
                        continue
                    fm = re.match(r"\s*(?:virtual\s+|static\s+|explicit\s+)*"
                                  r"(?:[\w:*&<>\[\]]+\s+)*?(~?\w+|operator[^\s(]*)\s*\(", ln)
                    if fm and "{" in "".join(lines[i:i + 3]) and ";" not in ln.split("(")[0]:
                        seg = "\n".join(lines[i:i + 40])
                        op = seg.index("(")
                        params = param_text(seg, op)
                        if params is None:
                            continue
                        if not has_body(seg, op + len(params) + 1):
                            continue
                        defs[(cls, fm.group(1), normalize(params))].append(
                            (str(path.relative_to(REPO)), i + 1, True, "in-class"))
    return defs


def main():
    defs = scan()
    dup = {k: v for k, v in defs.items()
           if len({(f, l) for f, l, _, _ in v}) > 1}
    # keep only genuinely conflicting pairs: at least one inline and one not, or
    # two bodies in different files
    real = {}
    for k, v in dup.items():
        files = {f for f, _, _, _ in v}
        kinds = {inl for _, _, inl, _ in v}
        if len(files) > 1 or len(kinds) > 1:
            real[k] = v
    print(f"scanned {sum(len(v) for v in defs.values())} definitions; "
          f"{len(real)} function(s) defined more than once\n")
    for (cls, fn, params), sites in sorted(real.items()):
        print(f"{cls}::{fn}({params})")
        for f, l, inl, kind in sorted(sites):
            print(f"    {'inline    ' if inl else 'NOT inline'}  {kind:<12} {f}:{l}")
        print()
    if "--gate" in sys.argv and real:
        print(f"dual-definition: FAIL - {len(real)} function(s) with two bodies", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
