#!/usr/bin/env python3
"""gruntz.core.class_meta - the shared src/+include/ class-definition scanner.

Consumed by the gruntz/cleanliness gates (class_sizes, class_vtables,
vtable_bans, vtable_virtuality, vtable_slot_binding) and by
gruntz.core.vtable_hierarchy - it lives in core/ so no core module imports a
tool package upward.

SCOPING RULES (documented once here; identical for every consumer, so a class
is never in one worklist under a stricter definition than another):

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
# POSITIONAL size labels (rva.h): SIZE(<bytes>); / SIZE_UNKNOWN(); sit directly
# under the class definition (or its fwd decl / typedef alias line).
_SIZE_POS_RE = re.compile(r"^\s*SIZE\(\s*(0x[0-9a-fA-F]+|\d+)\s*\)\s*;")
_SIZEUNK_POS_RE = re.compile(r"^\s*SIZE_UNKNOWN\(\s*\)\s*;")
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



_TYPEDEF_RE = re.compile(r"^\s*typedef\b.*?\b([A-Za-z_]\w*)\s*;")
_FWD_RE = re.compile(r"^\s*(?:class|struct)\s+([A-Za-z_]\w*)\s*;")
_CLOSE_RE = re.compile(r"^\s*\}\s*;")
# a complete one-line definition: `struct X : public Y { ... };`
_ONELINE_RE = re.compile(
    r"^\s*(?:typedef\s+)?(?:class|struct|union)\s+([A-Za-z_]\w*)[^;{]*\{.*\}\s*;")


def _entity_above(lines, idx):
    """The class name a POSITIONAL SIZE at line `idx` annotates: the nearest
    preceding class-def close (brace-matched back to its head), fwd decl, or
    typedef alias, skipping blanks/comments/other annotation lines."""
    j = idx - 1
    while j >= 0:
        ln = lines[j]
        st = ln.strip()
        if not st or st.startswith("//") or st.startswith("*") or st.startswith("/*"):
            j -= 1
            continue
        if _SIZE_POS_RE.match(ln) or _SIZEUNK_POS_RE.match(ln) or st.startswith("VTBL"):
            j -= 1
            continue
        m = _TYPEDEF_RE.match(ln)
        if m:
            return m.group(1)
        m = _FWD_RE.match(ln)
        if m:
            return m.group(1)
        m = _ONELINE_RE.match(ln)
        if m:
            return m.group(1)
        if _CLOSE_RE.match(ln):
            depth = 0
            k = j
            while k >= 0:
                depth += lines[k].count("}") - lines[k].count("{")
                if depth <= 0 and "{" in lines[k]:
                    break
                k -= 1
            # the head may be on the brace line or up to a few lines above
            for h in range(k, max(-1, k - 4), -1):
                hm = re.match(r"\s*(?:typedef\s+)?(?:class|struct|union)\s+([A-Za-z_]\w*)", lines[h])
                if hm:
                    return hm.group(1)
            return None
        return None
    return None


def positional_size_annotations():
    """{class_name: (bytes_or_None, path, lineno)} for every positional
    SIZE(<bytes>); / SIZE_UNKNOWN(); label, resolved to the entity above it.
    Unresolvable labels are returned under the key ``None`` (a list)."""
    out, orphans = {}, []
    for path in source_files():
        lines = _blank_comments(path.read_text(errors="ignore")).splitlines()
        for i, ln in enumerate(lines):
            m = _SIZE_POS_RE.match(ln)
            unk = None if m else _SIZEUNK_POS_RE.match(ln)
            if not m and not unk:
                continue
            name = _entity_above(lines, i)
            if name is None:
                orphans.append((str(path), i + 1))
                continue
            val = None
            if m:
                t = m.group(1)
                val = int(t, 16 if t.startswith("0x") else 10)
            out.setdefault(name, []).append((val, path, i + 1))
    out[None] = orphans
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
    ann = positional_size_annotations()
    return {n for n in ann if n is not None}


def vtbl_annotated_names() -> set:
    """Class names carrying a VTBL(...) annotation."""
    return _annotated(_VTBL_RE)


_VTBL_ABSENT_RE = re.compile(r"\bVTBL_ABSENT\s*\(\s*([A-Za-z_]\w*)")


def vtbl_absent_names() -> set:
    """Class names carrying a VTBL_ABSENT(...) catalog entry - vtable-bearing
    classes whose ??_7 datum is PROVEN absent from the retail image (never-emitted
    bases / never-constructed dispatch facets). Catalogued, not gaps."""
    return _annotated(_VTBL_ABSENT_RE)

# The manual-vtable stamp idiom in a class BODY: an ``&...Vtbl`` / ``&..._vftable``
# address-of or an ``m_vtbl`` / ``m_vptr`` field (the WAP-engine hand-rolled-vtable
# signal shared by class_vtables and vtable_hierarchy).
MANUAL_STAMP_RE = re.compile(r"&\s*[A-Za-z_]\w*(?:[Vv]tbl|vftable)\b|\bm_v(?:tbl|ptr)")

_RTTI_VTBL_RE = re.compile(r"^\?\?_7([A-Za-z_]\w*)@@6B@$")


def rtti_vtables() -> dict:
    """{class_name: rva} for the simple (global-namespace) ``??_7<Name>@@6B@``
    vtables in config/vtable_names.csv."""
    import csv
    out = {}
    path = REPO / "config" / "vtable_names.csv"
    if not path.exists():
        return out
    for r in csv.reader(path.open()):
        if not r or r[0].strip() in ("", "name") or r[0].lstrip().startswith("#"):
            continue
        m = _RTTI_VTBL_RE.match(r[0].strip())
        if m:
            try:
                out[m.group(1)] = int(r[1], 16)
            except (ValueError, IndexError):
                pass
    return out


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
