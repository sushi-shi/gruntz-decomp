#!/usr/bin/env python3
"""Shared source scanner for the class-metadata completeness checks.

Consumed by the two runnable checks:
  * ``python -m gruntz.cleanliness.class_sizes``   - every class has SIZE/SIZE_UNKNOWN.
  * ``python -m gruntz.cleanliness.class_vtables``  - every vtable-bearing class is
                                                catalogued (VTBL / manual / RTTI).

SCOPING RULES (documented once here; identical for both checks, so a class is
never in one worklist under a stricter definition than the other):

  * Scan ``src/`` + ``include/`` only. ``vendor/`` is NEVER read (pristine
    third-party TUs carry no rva.h macros and are not ours to annotate).
  * A "class definition" is a ``class``/``struct <Name> ... {`` whose keyword is
    at COLUMN 0 (no leading whitespace). File-scope only -> this deliberately
    EXCLUDES nested classes and function-local "view" structs (both indented),
    and forward declarations (``class X;`` - a ``;`` before any ``{``).
  * A def immediately preceded by a ``template`` line is SKIPPED: a templated
    class has a mangled ``??_7?$Name@...@@6B@`` vtable that VTBL(Name)'s simple
    ``??_7Name@@6B@`` cannot express (those stay in config/vtable_names.csv).
  * Results are keyed by class NAME. The same name defined in several per-TU shim
    headers is ONE logical class: annotating the name once (anywhere) satisfies
    every def, so the worklist is a set of NAMES, each with a representative def
    location. (The engine's per-TU placeholder re-definitions are pervasive; a
    per-def worklist would be dominated by that noise.)
"""
from __future__ import annotations

import re
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
SRC = REPO / "src"
INC = REPO / "include"
RVA_H = INC / "rva.h"

# A file-scope class/struct definition head: keyword at column 0, then the name.
_CLASS_HEAD_RE = re.compile(r"^(class|struct)\s+([A-Za-z_]\w*)\b")
# SIZE / SIZE_UNKNOWN / VTBL macro invocations (first arg = the class/type name).
_SIZE_RE = re.compile(r"\bSIZE\s*\(\s*([A-Za-z_]\w*)\b")
_SIZEUNK_RE = re.compile(r"\bSIZE_UNKNOWN\s*\(\s*([A-Za-z_]\w*)\b")
_VTBL_RE = re.compile(r"\bVTBL\s*\(\s*([A-Za-z_]\w*)\b")


def _blank_comments(text: str) -> str:
    """`text` with // and /* */ comment bodies blanked to spaces (newlines kept),
    so a macro written inside a COMMENT (an example/doc) is not read as real."""
    out, n, i, st = list(text), len(text), 0, "code"
    while i < n:
        c = text[i]
        if st == "code":
            if c == "/" and i + 1 < n and text[i + 1] == "/":
                while i < n and text[i] != "\n":
                    out[i] = " "
                    i += 1
                continue
            if c == "/" and i + 1 < n and text[i + 1] == "*":
                while i < n and not (text[i] == "*" and i + 1 < n and text[i + 1] == "/"):
                    if text[i] != "\n":
                        out[i] = " "
                    i += 1
                continue
            if c in "\"'":
                st = c
        elif c == "\\":
            i += 2
            continue
        elif c == st:
            st = "code"
        i += 1
    return "".join(out)


def source_files():
    """Every scanned source/header under src/ + include/, EXCLUDING rva.h (whose
    own `#define SIZE(type,...)` lines would otherwise register bogus names)."""
    for root in (INC, SRC):
        if not root.exists():
            continue
        for path in sorted(list(root.rglob("*.h")) + list(root.rglob("*.cpp"))):
            if path.resolve() == RVA_H.resolve():
                continue
            yield path


def iter_class_defs():
    """Yield (name, path, lineno, body) for each FILE-SCOPE class/struct
    definition (see SCOPING). `body` is the brace-matched class body text (for the
    vtable-signal check); `lineno` is 1-based. A name may repeat across files."""
    for path in source_files():
        text = _blank_comments(path.read_text(errors="ignore"))
        lines = text.splitlines(keepends=True)
        # char offset of each line start, for brace-matching from a def head.
        offs, acc = [], 0
        for ln in lines:
            offs.append(acc)
            acc += len(ln)
        for idx, raw in enumerate(lines):
            m = _CLASS_HEAD_RE.match(raw)
            if not m:
                continue
            # Skip a templated class (its def head is preceded by a `template` line).
            prev = idx - 1
            while prev >= 0 and not lines[prev].strip():
                prev -= 1
            if prev >= 0 and lines[prev].lstrip().startswith("template"):
                continue
            # Definition iff a `{` appears before the terminating `;` (else it is a
            # forward declaration / a variable of an existing type).
            start = offs[idx]
            brace = text.find("{", start)
            semi = text.find(";", start)
            if brace < 0 or (0 <= semi < brace):
                continue
            # Brace-match to the class body end.
            depth, j, end = 0, brace, len(text)
            while j < len(text):
                if text[j] == "{":
                    depth += 1
                elif text[j] == "}":
                    depth -= 1
                    if depth == 0:
                        end = j
                        break
                j += 1
            yield m.group(2), path, idx + 1, text[brace:end]


def unique_class_defs():
    """{name: (path, lineno)} - first-seen representative def per class NAME."""
    out = {}
    for name, path, lineno, _body in iter_class_defs():
        out.setdefault(name, (path, lineno))
    return out


def _annotated(rx: re.Pattern) -> set:
    """Set of class NAMES named by macro `rx` anywhere in the scanned tree."""
    names = set()
    for path in source_files():
        text = _blank_comments(path.read_text(errors="ignore"))
        for m in rx.finditer(text):
            names.add(m.group(1))
    return names


def size_annotated_names() -> set:
    """Class names carrying a SIZE(...) OR SIZE_UNKNOWN(...) annotation."""
    return _annotated(_SIZE_RE) | _annotated(_SIZEUNK_RE)


def vtbl_annotated_names() -> set:
    """Class names carrying a VTBL(...) annotation."""
    return _annotated(_VTBL_RE)


_VTBL_ABSENT_RE = re.compile(r"\bVTBL_ABSENT\s*\(\s*([A-Za-z_]\w*)")


def vtbl_absent_names() -> set:
    """Class names carrying a VTBL_ABSENT(...) catalog entry - vtable-bearing
    classes whose ??_7 datum is PROVEN absent from the retail image (never-emitted
    bases / never-constructed dispatch facets). Catalogued, not gaps."""
    return _annotated(_VTBL_ABSENT_RE)


# VTBL(name, 0xrva) with BOTH arguments captured, for rva-uniqueness auditing.
_VTBL_FULL_RE = re.compile(r"\bVTBL\s*\(\s*([A-Za-z_]\w*)\s*,\s*(0x[0-9a-fA-F]+)")


def vtbl_annotations():
    """Yield (name, rva:int, path, lineno) for each VTBL(name, 0xrva) in the tree
    (comments blanked). A vtable datum has exactly one true ??_7 name in retail, so
    an rva bound by two different names is a mis-catalog - see class_vtables."""
    for path in source_files():
        text = _blank_comments(path.read_text(errors="ignore"))
        for m in _VTBL_FULL_RE.finditer(text):
            try:
                rva = int(m.group(2), 16)
            except ValueError:
                continue
            yield m.group(1), rva, path, text.count("\n", 0, m.start()) + 1


def rel(path: Path) -> str:
    try:
        return str(path.relative_to(REPO))
    except ValueError:
        return str(path)
