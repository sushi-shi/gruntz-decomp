#!/usr/bin/env python3
"""Class-size checks.

(1) COMPLETENESS - every class/struct definition carries a size annotation: EITHER
    ``SIZE(Class, bytes)`` (exact, verified against retail) OR ``SIZE_UNKNOWN(Class)``
    (tracked, byte size not yet pinned). No class silently lacks a size record.

(2) CORRECTNESS - a class that DECLARES ``SIZE(C, N)`` must actually COMPUTE N.

Nothing checked (2) until now, and the gap was expensive. ``SIZE()`` is only a comment
unless something compares it to the layout the compiler really produces, so a class could
declare the right retail size while its body quietly disagreed - and when ``sizeof`` is
load-bearing (``new C``), the wrong number goes straight into the emitted
``push <size>; call ??2`` immediate. Both of these were live and invisible:
    CFaderRadial  declared 0x5c   computed 0x54   (+0x54/+0x58 members missing)
    CHelpState    declared 0x1b8  computed 0x1ac  (base spine mis-stated as 0x1b4)
and both were additionally MASKED by a hand-written
``operator new(u32) { return ::operator new(0x5c); }`` that hard-coded the right immediate
over a wrong class - so even the bytes looked fine while the layout was not.

SOUNDNESS, the hard part. The computed size comes from build/gen/structs.json, which keeps
ONE size per class NAME. When a name has SEVERAL definitions (per-TU views, different bases)
that number may belong to a definition no ``new``-site TU ever sees: CSBI_Image has FOUR
definitions and structs.json reports 0x3c, while the TU that news it sees the 0x34 one and
emits 0x34 - correctly. Flagging that would be a false alarm that BREAKS 12 correct
new-sites if "fixed". So correctness is asserted ONLY for names with exactly ONE definition,
where structs.json's number is unambiguous. Multiply-defined names are reported separately
as UNVERIFIABLE (a real gap - the cure is to fold them to one definition, not to guess).

Runnable as ``python -m gruntz.match.class_sizes``.
"""
from __future__ import annotations

import json
import re
from collections import defaultdict
from pathlib import Path

import sys

from gruntz.match.class_meta import (iter_class_defs, rel, size_annotated_names,
                                     source_files, unique_class_defs)

_SIZE_DECL_RE = re.compile(r"\bSIZE\(\s*(\w+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*\)")
_REPO = Path(__file__).resolve().parents[3]
_STRUCTS = _REPO / "build/gen/structs.json"


# A SIZE() written in PROSE is not a declaration. Without this, a note like
# `// (2) SIZE(CUserLogic, 0x30) revisited - a base at +0x34 requires ...` parses as a
# real decl, and since `out[name]` overwrites, whichever file sorts last WINS - so a
# comment silently overrode the true `SIZE(CUserLogic, 0x34)` and turned this FATAL gate
# red against correct code. Strip comments before matching (same rule the cleanliness
# metrics use). Real decls are also reported per-name below when multiply-defined.
_BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
_LINE_COMMENT = re.compile(r"//[^\n]*")


def _strip_comments(text: str) -> str:
    return _LINE_COMMENT.sub("", _BLOCK_COMMENT.sub("", text))


def _declared_sizes() -> dict:
    out = {}
    for path in source_files():
        code = _strip_comments(path.read_text(errors="ignore"))
        for m in _SIZE_DECL_RE.finditer(code):
            n = m.group(2)
            out[m.group(1)] = int(n, 16 if n.startswith("0x") else 10)
    return out


def _stale_sources() -> list:
    """Source files NEWER than build/gen/structs.json.

    A stale structs.json is not a smaller problem than a missing one - it is a WORSE one,
    because it answers. It reports the sizes of a tree that no longer exists, so this check
    can both FALSE-FAIL (it flagged CSbiRectSub as emitting the wrong operator-new immediate
    while the obj was in fact already emitting the correct 0x30) and, far worse, FALSE-PASS -
    silently blessing the exact defect it exists to catch. `gruntz build --fast` does not
    regenerate structs.json, so this is easy to hit. Refuse to answer instead of guessing.
    """
    if not _STRUCTS.is_file():
        return []
    ts = _STRUCTS.stat().st_mtime
    return [p for p in source_files() if p.stat().st_mtime > ts]


def _computed_sizes() -> dict:
    if not _STRUCTS.is_file():
        return {}
    return {e["name"]: e.get("size") for e in json.load(_STRUCTS.open())}


def _def_counts() -> dict:
    c = defaultdict(int)
    for name, _p, _l, _b in iter_class_defs():
        c[name] += 1
    return c


def _loadbearing() -> set:
    """Class names whose sizeof the COMPILER actually emits: `new C` or `sizeof(C)`.

    This is the line between a bug and a documented partial model. Many classes here are
    deliberately partial (only the offsets a matched body touches are modelled) while their
    SIZE() records the RETAIL object's true size - and that is fine, because nothing takes
    their sizeof, so the short body can never reach the instruction stream. The moment a
    class is `new`ed, its sizeof becomes the operator-new immediate and a short body is a
    hard byte bug (CFaderRadial, CHelpState).
    """
    names = set()
    pat = re.compile(r"\bnew\s+([A-Za-z_]\w*)|\bsizeof\s*\(\s*([A-Za-z_]\w*)\s*\)")
    for path in source_files():
        for m in pat.finditer(path.read_text(errors="ignore")):
            names.add(m.group(1) or m.group(2))
    return names


def check_correctness() -> int:
    """SIZE(C, N) must equal what C computes - for names with ONE definition (see SOUNDNESS)."""
    computed = _computed_sizes()
    if not computed:
        print("class-size correctness: SKIPPED - build/gen/structs.json missing "
              "(run a build first; a stale/absent structs.json cannot be trusted)")
        return 0
    stale = _stale_sources()
    if stale:
        print(f"class-size correctness: REFUSING TO ANSWER - build/gen/structs.json is STALE "
              f"({len(stale)} source file(s) are newer, e.g. {rel(stale[0])}). It would report "
              f"the sizes of a tree that no longer exists - which can false-FAIL and, worse, "
              f"false-PASS the very defect this check exists to catch. Run `gruntz structs` "
              f"(or a full `gruntz build`); `--fast` does NOT regenerate it.", file=sys.stderr)
        return 1
    declared, ndefs, lb = _declared_sizes(), _def_counts(), _loadbearing()
    bad, partial, unverifiable = [], [], []
    for c, n in sorted(declared.items()):
        s = computed.get(c)
        if s is None or s == n:
            continue
        if ndefs.get(c, 0) > 1:
            unverifiable.append((c, n, s, ndefs[c]))
        elif c in lb:
            bad.append((c, n, s))          # sizeof reaches the instruction stream -> BUG
        else:
            partial.append((c, n, s))      # documented partial model -> not a byte defect
    if bad:
        print(f"class-size correctness: {len(bad)} class(es) DECLARE a SIZE they do not "
              f"compute AND are `new`ed/sizeof'd - the wrong operator-new immediate is being "
              f"emitted:", file=sys.stderr)
        for c, n, s in bad:
            print(f"  {c}: SIZE says 0x{n:x}, the class computes 0x{s:x} "
                  f"(off by {s - n:+#x})", file=sys.stderr)
        return 1
    print("class-size correctness: OK - every load-bearing SIZE matches what the class computes")
    if partial:
        print(f"    ({len(partial)} partial model(s): SIZE records the retail object, the body "
              f"is short, and nothing takes sizeof - harmless, but they are NOT verified)")
        for c, n, s in partial:
            print(f"      {c}: SIZE 0x{n:x}, body 0x{s:x}")
    if unverifiable:
        print(f"    ({len(unverifiable)} multiply-defined name(s) UNVERIFIABLE - structs.json "
              f"keeps ONE size per NAME, which may be a def no new-site sees; fold them)")
        for c, n, s, k in unverifiable:
            print(f"      {c}: {k} definitions; SIZE 0x{n:x} vs structs.json 0x{s:x}")
    return 0


def main() -> int:
    defs = unique_class_defs()
    annotated = size_annotated_names()
    violators = sorted((n, defs[n]) for n in defs if n not in annotated)
    total = len(defs)
    ok = total - len(violators)
    if violators:
        print(f"class-size completeness: {ok}/{total} class names annotated; "
              f"{len(violators)} MISSING SIZE/SIZE_UNKNOWN:", file=sys.stderr)
        for name, (path, lineno) in violators:
            print(f"  {rel(path)}:{lineno}: {name}", file=sys.stderr)
        return 1
    print(f"class-size completeness: all {total} class names carry SIZE/SIZE_UNKNOWN")
    return check_correctness()


if __name__ == "__main__":
    raise SystemExit(main())
