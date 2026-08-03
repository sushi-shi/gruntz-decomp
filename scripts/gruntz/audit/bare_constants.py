#!/usr/bin/env python3
"""gruntz.audit.bare_constants - numeric literals that a TYPE already names.

The enum metrics catch `case 0x36:` and `x == 0x36`. This is the rest of the
same problem: a literal written where the surrounding type says what it means.
It needs libclang, not a regex, because every class here is decided by the type
of the OTHER operand - which is exactly what a regex cannot see.

Classes counted:

  pointer-vs-0   `p == 0`, `p != 0`, `p = 0` where p is a POINTER. The zero is a
                 null pointer constant, so NULL says so and 0 does not. This is
                 the biggest class by far and the cheapest to fix; it is also
                 byte-neutral, since NULL expands to 0.

  enum-vs-int    a literal compared against an enum-typed expression, where the
                 enum actually declares that value. The switch form of this is
                 gruntz.audit.enum_case_labels; this is the `==` form, which
                 that tool does not see.

  sizeof-literal a literal byte count passed beside a pointer whose pointee is
                 exactly that many bytes - `ar->Read(&m_thing, 4)` where m_thing
                 is 4 bytes. sizeof(m_thing) is the same constant and cannot
                 drift when the field's type changes.

  bool-vs-int    0 or 1 against a field PROVEN boolean by its own use. MSVC 5.0
                 does have bool/true/false (measured), so the value spellings
                 cost nothing; only the STORAGE has to stay 4 bytes, since
                 sizeof(bool) == 1 would move every member after it.

                 The proof is deliberately strict, because the loose version is
                 wrong: "only ever compared against 0" also describes a COUNT
                 tested for emptiness and the HIGH DWORD of an i64 pair, and a
                 first attempt at this duly reported m_nodeCount and half the
                 timer fields as booleans. A field qualifies only if every use is
                 a condition, a `!`, or a compare/assign against 0 or 1, AND it
                 is set to 1 somewhere - a field only ever zeroed is a handle or
                 a counter - AND it is not inside an anonymous struct, which is
                 how the i64 pairs are spelled.

NOT counted, deliberately:
  - arithmetic literals (`x * 32`, `>> 5`, masks). A shift or a stride is a
    number, not a name waiting to happen.
  - 0 and 1 against plain integers - those are overwhelmingly counts and
    null/bool-ish tests, and counting them would swamp the signal.

  python3 -m gruntz.audit.bare_constants [--gate] [--update] [--detail]

    (default)  print the per-class counts against the ratchet
    --gate     exit 1 if any class is ABOVE its baseline (ratchet: down-only)
    --update   bless the current counts (only ever downward)
    --detail   also write build/gen/bare_constants.tsv, one row per site
"""
from __future__ import annotations

import argparse
import collections
import json
import sys
from pathlib import Path

import clang.cindex as cidx

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
CDB = REPO / "build" / "clangd" / "compile_commands.json"
BASELINE = REPO / "config" / "bare-constants-baseline.tsv"
DETAIL = REPO / "build" / "gen" / "bare_constants.tsv"

CLASSES = ("pointer-vs-0", "enum-vs-int", "sizeof-literal", "bool-vs-int")

# size-taking calls whose second argument is a byte count beside a pointer
_SIZED_CALLS = {"Read": 1, "Write": 1, "memset": 2, "memcpy": 2}

# fields whose every use proves them boolean (see the bool-vs-int note above).
# Membership is EVIDENCE, not preference: gruntz ships no PDB, so unlike
# homm2-decomp there is no `gb` symbol prefix to read a flag off, and the recovered
# names only corroborate what the uses already showed.
PROVEN_BOOL = {
    "CChatBoxOwner::m_attached", "CMoviePlayer::m_frameDecoded",
    "CRezImage::m_transparent", "CGruntzMgr::m_haveMoviez",
    "CGruntzMgr::m_haveRez", "CSymParser::m_newArchive",
    "CFaderMgr::m_active", "CImagePaletteNode::m_systemTuned",
}


def _field_owner(n):
    d = n.referenced
    if d is None or d.kind != cidx.CursorKind.FIELD_DECL:
        return None
    p = d.semantic_parent
    return "%s::%s" % (p.spelling, d.spelling) if p is not None and p.spelling else None


def _flags_for(entry):
    """The TU's real flags. The compdb is MSVC-style, so the driver mode must be
    set or clang discards every flag as a 'linker input'."""
    args = list(entry.get("arguments") or entry["command"].split())
    src = entry["file"]
    out = ["--driver-mode=cl"]
    for a in args[1:]:
        if a == "/c" or a == src or a.endswith(src):
            continue
        out.append(a)
    return out


def _rel(cursor):
    f = cursor.location.file
    if f is None:
        return None
    try:
        rel = Path(f.name).resolve().relative_to(REPO)
    except ValueError:
        return None
    s = str(rel)
    return s if s.startswith(("src/", "include/")) else None


def _int_token(node):
    """The literal's text if `node` is a single integer token, else None."""
    toks = [t.spelling for t in node.get_tokens()]
    if len(toks) != 1:
        return None
    t = toks[0]
    if t.startswith(("0x", "0X")):
        try:
            return int(t, 16)
        except ValueError:
            return None
    return int(t) if t.isdigit() else None


def _enum_values(node):
    """{value} the enum behind `node` declares, or None if it is not enum-typed.

    Descends past the INTEGRAL PROMOTION first. An enum operand of `==` is
    promoted to int before the comparison, so the expression's own type is
    `int` and asking it directly always answers None - the same trap the switch
    key had in enum_case_labels."""
    probe, depth = node, 0
    while depth < 8:
        depth += 1
        d = probe.type.get_declaration()
        if d is not None and d.kind == cidx.CursorKind.ENUM_DECL:
            return {c.enum_value for c in d.get_children()
                    if c.kind == cidx.CursorKind.ENUM_CONSTANT_DECL}
        sub = list(probe.get_children())
        if probe.kind not in (cidx.CursorKind.UNEXPOSED_EXPR,
                              cidx.CursorKind.PAREN_EXPR) or not sub:
            return None
        probe = sub[0]
    return None


def _scan_tu(tu, hits):
    stack = [tu.cursor]
    while stack:
        n = stack.pop()
        stack.extend(n.get_children())

        if n.kind in (cidx.CursorKind.BINARY_OPERATOR,
                      cidx.CursorKind.COMPOUND_ASSIGNMENT_OPERATOR):
            toks = [t.spelling for t in n.get_tokens()]
            if not any(t in ("==", "!=", "=") for t in toks):
                continue
            kids = list(n.get_children())
            if len(kids) != 2:
                continue
            for a, b in ((kids[0], kids[1]), (kids[1], kids[0])):
                v = _int_token(b)
                if v is None:
                    continue
                at = a.type.get_canonical()
                rel = _rel(n)
                if rel is None:
                    break
                if (v in (0, 1) and a.kind == cidx.CursorKind.MEMBER_REF_EXPR
                        and _field_owner(a) in PROVEN_BOOL):
                    lit = b.location
                    hits["bool-vs-int"].append(
                        (rel, lit.line, lit.column, str(v),
                         "true" if v == 1 else "false"))
                elif at.kind == cidx.TypeKind.POINTER and v == 0:
                    lit = b.location
                    hits["pointer-vs-0"].append((rel, lit.line, lit.column, "0", "NULL"))
                else:
                    vals = _enum_values(a)
                    if vals and v in vals:
                        hits["enum-vs-int"].append((rel, n.location.line, 0, "", ""))
                break

        elif n.kind == cidx.CursorKind.CALL_EXPR and n.spelling in _SIZED_CALLS:
            args = [k for k in n.get_children()]
            want = _SIZED_CALLS[n.spelling]
            # the callee is child 0 for a method call, so look from the end
            if len(args) < want + 1:
                continue
            size_node = args[-1]
            ptr_node = args[-2]
            v = _int_token(size_node)
            if v is None or v == 0:
                continue
            # Read/Write take `void*`, so by the time the argument reaches the
            # call it has decayed and its pointee size is unknown. Descend past
            # the implicit conversion to the `&field` the caller actually wrote.
            probe, depth = ptr_node, 0
            sz = -1
            while depth < 8:
                depth += 1
                pt = probe.type.get_canonical()
                if pt.kind == cidx.TypeKind.POINTER:
                    cand = pt.get_pointee().get_size()
                    if cand > 0:
                        sz = cand
                        break
                sub = list(probe.get_children())
                if not sub:
                    break
                probe = sub[0]
            rel = _rel(n)
            if rel is not None and sz == v:
                # `&thing` -> sizeof(thing): the operand's own tokens are the
                # expression to size, so the rewrite never invents a name.
                inner = [x.spelling for x in ptr_node.get_tokens()]
                if inner and inner[0] == "&":
                    expr = "".join(inner[1:])          # &thing  -> sizeof(thing)
                elif inner and inner[0].isidentifier() and (
                        len(inner) == 1 or (inner[1] in "([" and inner[-1] in ")]")):
                    # p -> sizeof(*p), and f(i) -> sizeof(*f(i)). sizeof does not
                    # evaluate its operand, so the call is not made twice.
                    expr = "*" + "".join(inner)
                else:
                    expr = ""
                lit = size_node.location
                old = [x.spelling for x in size_node.get_tokens()]
                hits["sizeof-literal"].append(
                    (rel, lit.line, lit.column, old[0] if old else "",
                     ("sizeof(%s)" % expr) if expr else ""))


def scan():
    db = json.loads(CDB.read_text())
    index = cidx.Index.create()
    hits = {c: [] for c in CLASSES}
    for entry in db:
        src = Path(entry["file"])
        try:
            tu = index.parse(str(src), args=_flags_for(entry))
        except cidx.TranslationUnitLoadError:
            continue
        _scan_tu(tu, hits)
    # one TU can be parsed once but a header seen many times; de-duplicate by site
    return {c: sorted(set(v)) for c, v in hits.items()}


def apply(hits, classes):
    """Rewrite the recorded literals in place. Right-to-left within each line so
    earlier columns stay valid, and each site is checked against the text it was
    recorded from - a moved line is skipped, never guessed at."""
    per = collections.defaultdict(list)
    for c in classes:
        for rel, line, col, old, new in hits[c]:
            if new and old:
                per[rel].append((line, col, old, new))
    total = 0
    for rel, sites in per.items():
        p = REPO / rel
        lines = p.read_text().splitlines(keepends=True)
        for line, col, old, new in sorted(sites, key=lambda s: (-s[0], -s[1])):
            if line > len(lines):
                continue
            s = lines[line - 1]
            i = col - 1
            if s[i:i + len(old)] != old:
                continue
            lines[line - 1] = s[:i] + new + s[i + len(old):]
            total += 1
        p.write_text("".join(lines))
    return total


def read_baseline():
    if not BASELINE.exists():
        return {}
    out = {}
    for ln in BASELINE.read_text().splitlines():
        if not ln.strip() or ln.startswith("#"):
            continue
        k, _, v = ln.partition("\t")
        out[k.strip()] = int(v)
    return out


def write_baseline(counts):
    BASELINE.write_text(
        "# gruntz.audit.bare_constants - numeric literals a TYPE already names.\n"
        "# RATCHET: down-only. Bless a lower number with --update; a higher one is\n"
        "# a regression and fails the gate.\n"
        + "".join("%s\t%d\n" % (c, counts[c]) for c in CLASSES))


def main(argv=None):
    ap = argparse.ArgumentParser(prog="gruntz.audit.bare_constants")
    ap.add_argument("--gate", action="store_true")
    ap.add_argument("--update", action="store_true")
    ap.add_argument("--detail", action="store_true")
    ap.add_argument("--apply", metavar="CLASS",
                    help="rewrite one class in place: pointer-vs-0 or sizeof-literal")
    a = ap.parse_args(argv)

    if not CDB.exists():
        sys.exit("[bare-constants] build/clangd/compile_commands.json not found - run gruntz init")

    hits = scan()
    counts = {c: len(hits[c]) for c in CLASSES}
    base = read_baseline()

    worse = []
    for c in CLASSES:
        b = base.get(c)
        mark = ""
        if b is not None:
            if counts[c] > b:
                mark = "  REGRESSED (+%d over %d)" % (counts[c] - b, b)
                worse.append(c)
            elif counts[c] < b:
                mark = "  (-%d, bless with --update)" % (b - counts[c])
        print("  %-16s %6d%s" % (c, counts[c], mark))

    if a.apply:
        if a.apply not in CLASSES:
            sys.exit("[bare-constants] unknown class %r (%s)" % (a.apply, ", ".join(CLASSES)))
        done = apply(hits, [a.apply])
        print("[bare-constants] rewrote %d %s site(s)" % (done, a.apply))
        return 0

    if a.detail:
        DETAIL.parent.mkdir(parents=True, exist_ok=True)
        DETAIL.write_text("class\tfile\tline\n" + "".join(
            "%s\t%s\t%d\n" % (c, r[0], r[1]) for c in CLASSES for r in hits[c]))
        print("[bare-constants] detail -> %s" % DETAIL.relative_to(REPO))

    if a.update:
        keep = {c: min(counts[c], base[c]) if c in base else counts[c] for c in CLASSES}
        write_baseline(keep)
        print("[bare-constants] baseline blessed")
        return 0

    total = sum(counts.values())
    if worse:
        print("[bare-constants] FATAL: %d class(es) above the ratchet" % len(worse))
        return 1 if a.gate else 0
    print("[bare-constants] OK - %d literal(s) a type already names" % total)
    return 0


if __name__ == "__main__":
    sys.exit(main())
