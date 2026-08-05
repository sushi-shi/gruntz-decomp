#!/usr/bin/env python3
"""gruntz.audit.enum_case_labels - name the case labels of ALREADY-typed switches.

The `magic case labels` metric counts `case 0x36:` / `case 4:`. A large share of
those sit in switches whose KEY IS ALREADY AN ENUM - the domain was modelled, the
key retyped, and only the labels were left spelled numerically. For those there is
nothing to infer: the compiler knows the key's type, so each integer label has
exactly ONE correct enumerator. This tool finds them via libclang and rewrites
them.

It NEVER invents a name and NEVER guesses a domain:

  - the switch key's type must resolve to an enum declaration (after stripping
    typedefs/refs/parens and the integral promotion the key is wrapped in);
  - the label must be an integer constant that the enum actually declares;
  - if a value has SEVERAL enumerators (aliases - e.g. PickupType's GRUNT_*
    spellings, NetMsgId's send-side STAT_* names), the file is REPORTED and left
    alone, because picking one would be a guess about which reading the developer
    meant.

Being AST-real, it cannot be fooled by a literal in a comment or a string, and it
skips the retail-vs-strict macro question entirely: it reads whatever the clangd
compdb says the TU really compiles as.

Matching-neutral by construction - an enumerator and its value are the same
integer constant to the compiler - but verify with a build anyway; that is the
house rule.

  python3 -m gruntz.audit.enum_case_labels [--apply] [--detail] [--gate] [--through-casts] [paths...]

    (default)       report what could be named, write nothing
    --apply         rewrite the labels
    --detail        write build/gen/enum_case_labels.tsv, one row per finding
    --through-casts also treat `switch (static_cast<u32>(enumExpr))` sites as
                    nameable. Reported separately by default because an explicit
                    cast is how a two-domain reinterpretation is spelled too -
                    review the site before applying.
    --gate          exit 1 if any nameable label remains (for cli.py)
    paths           limit to these files/dirs (default: every TU in the compdb)
"""
from __future__ import annotations

import argparse
import collections
import json
import re
import sys
from pathlib import Path

import clang.cindex as cidx

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
CDB = REPO / "build" / "clangd" / "compile_commands.json"
DETAIL = REPO / "build" / "gen" / "enum_case_labels.tsv"


def _enum_decl(t):
    """The EnumDecl behind a switch-key type, or None. Strips typedefs, refs,
    const/volatile and the elaborated spelling; an integral promotion of an enum
    key still reports the enum as the sub-expression type, which is why the caller
    passes the *condition expression's* type rather than the promoted one."""
    seen = 0
    while t is not None and seen < 16:
        seen += 1
        d = t.get_declaration()
        if d is not None and d.kind == cidx.CursorKind.ENUM_DECL:
            return d
        if t.kind == cidx.TypeKind.TYPEDEF and d is not None:
            t = d.underlying_typedef_type
            continue
        canon = t.get_canonical()
        if canon is not None and canon.kind != t.kind:
            t = canon
            continue
        return None
    return None


def _enum_table(decl):
    """value -> [enumerator names], in declaration order."""
    out = collections.OrderedDict()
    for c in decl.get_children():
        if c.kind == cidx.CursorKind.ENUM_CONSTANT_DECL:
            out.setdefault(c.enum_value, []).append(c.spelling)
    return out


_TRANSPARENT = (cidx.CursorKind.UNEXPOSED_EXPR, cidx.CursorKind.PAREN_EXPR)
_CASTS = (cidx.CursorKind.CXX_STATIC_CAST_EXPR, cidx.CursorKind.CSTYLE_CAST_EXPR,
          cidx.CursorKind.CXX_FUNCTIONAL_CAST_EXPR)


def _key_type(switch_cursor):
    """(EnumDecl, via_cast) for the switch condition, or (None, False).

    Sees THROUGH the implicit integral promotion that wraps an enum key. Also
    sees through an EXPLICIT cast - `switch (static_cast<u32>(config->
    m_entranceReason))` is the documented unsigned-`ja` codegen idiom
    (switch-key-unsigned-ja-vs-jg.md), and the labels are still that domain's
    values. But an explicit cast is ALSO how a deliberate two-domain
    reinterpretation is spelled, so those hits are flagged `via_cast` and are
    NOT auto-applied without --through-casts: which reading was meant is a
    judgement about the site, not something the type system settles."""
    kids = list(switch_cursor.get_children())
    if not kids:
        return None, False
    node, depth, via_cast = kids[0], 0, False
    while depth < 12:
        depth += 1
        t = _enum_decl(node.type)
        if t is not None:
            return t, via_cast
        sub = list(node.get_children())
        if node.kind in _TRANSPARENT and sub:
            node = sub[0]
            continue
        if node.kind in _CASTS and sub:
            node = sub[-1]          # the cast's operand, past any type-ref child
            via_cast = True
            continue
        break
    return None, False


def _case_labels(switch_cursor):
    """(line, col, value, spelling) for every numeric case label directly under
    this switch. Nested switches are skipped - they carry their own key."""
    out = []
    body = list(switch_cursor.get_children())
    if len(body) < 2:
        return out
    stack = [body[1]]
    while stack:
        n = stack.pop()
        if n.kind == cidx.CursorKind.SWITCH_STMT:
            continue                      # a nested switch owns its own labels
        if n.kind == cidx.CursorKind.CASE_STMT:
            kids = list(n.get_children())
            if kids:
                lit = kids[0]
                toks = [t.spelling for t in lit.get_tokens()]
                # a single integer token: `case 0x36:` / `case 4:` (NOT `case A|B:`
                # and NOT an already-named enumerator)
                if len(toks) == 1 and re.fullmatch(r'0[xX][0-9a-fA-F]+|\d+', toks[0]):
                    loc = lit.location
                    out.append((loc.line, loc.column, int(toks[0], 0), toks[0]))
        stack.extend(n.get_children())
    return out


def _flags_for(entry):
    """The TU's real flags. The compdb is MSVC-style (`/I`, `/imsvc`, `/D`), so the
    driver mode MUST be set or clang silently discards every one of them as a
    'linker input' and the parse fails on the first #include."""
    args = list(entry.get("arguments") or entry["command"].split())
    src = entry["file"]
    out = ["--driver-mode=cl"]
    for a in args[1:]:
        if a == "/c" or a == src or a.endswith(src):
            continue
        out.append(a)
    return out


def scan(limit=None):
    """-> {path: [(line, col, value, spelling, enum_name, enumerator)]},
        plus a list of ambiguous (value has aliases) sites."""
    db = json.loads(CDB.read_text())
    index = cidx.Index.create()
    found = collections.defaultdict(list)
    via_cast_hits = collections.defaultdict(list)
    ambiguous = []
    seen_files = set()
    for entry in db:
        # compdb paths are REPO-relative; resolve before comparing against the
        # absolute --paths, or the filter silently matches nothing and the tool
        # reports a false zero.
        src = Path(entry["file"])
        absolute = src if src.is_absolute() else (REPO / src)
        if limit and not any(str(absolute).startswith(str(l)) for l in limit):
            continue
        try:
            tu = index.parse(str(src), args=_flags_for(entry),
                             options=cidx.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES == 0)
        except cidx.TranslationUnitLoadError:
            continue
        stack = [tu.cursor]
        while stack:
            n = stack.pop()
            stack.extend(n.get_children())
            if n.kind != cidx.CursorKind.SWITCH_STMT:
                continue
            decl, via_cast = _key_type(n)
            if decl is None:
                continue
            table = _enum_table(decl)
            if not table:
                continue
            ename = decl.spelling or "<anonymous>"
            for line, col, val, spelling in _case_labels(n):
                f = n.location.file
                if f is None:
                    continue
                path = Path(f.name).resolve()
                try:
                    rel = path.relative_to(REPO)
                except ValueError:
                    continue
                if not str(rel).startswith(("src/", "include/")):
                    continue
                names = table.get(val)
                if not names:
                    continue                       # value the domain does not declare
                key = (str(rel), line, col)
                if key in seen_files:
                    continue
                seen_files.add(key)
                if len(names) > 1:
                    # A case label names a VALUE, never a boundary: if the only
                    # thing making the value ambiguous is a band marker sharing
                    # it (TAB_GAME vs TAB_LAST), the item spelling is the right
                    # one and there is nothing to arbitrate.
                    items = [x for x in names
                             if not re.search(r'_(FIRST|LAST|BEGIN|END|COUNT)$', x)]
                    if len(items) == 1:
                        names = items
                    else:
                        ambiguous.append((str(rel), line, val, ename, names))
                        continue
                bucket = via_cast_hits if via_cast else found
                bucket[str(rel)].append((line, col, val, spelling, ename, names[0]))
    return found, ambiguous, via_cast_hits


def apply(found):
    total = 0
    for rel, hits in sorted(found.items()):
        p = REPO / rel
        lines = p.read_text().splitlines(keepends=True)
        # right-to-left within a line so columns stay valid
        for line, col, val, spelling, ename, name in sorted(hits, key=lambda h: (-h[0], -h[1])):
            if line > len(lines):
                continue
            s = lines[line - 1]
            i = col - 1
            if s[i:i + len(spelling)] != spelling:
                continue                            # moved under us - skip, never guess
            lines[line - 1] = s[:i] + name + s[i + len(spelling):]
            total += 1
        p.write_text("".join(lines))
    return total


def write_detail(found, ambiguous, via_cast):
    """Write the complete reviewable inventory produced by this scan."""
    rows = []
    for rel, hits in found.items():
        for line, col, val, spelling, ename, name in hits:
            rows.append((rel, line, col, "nameable", ename, val, spelling, name))
    for rel, line, val, ename, names in ambiguous:
        rows.append((rel, line, 0, "ambiguous", ename, val, "", "/".join(names)))
    for rel, hits in via_cast.items():
        for line, col, val, spelling, ename, name in hits:
            rows.append((rel, line, col, "via-cast", ename, val, spelling, name))
    DETAIL.parent.mkdir(parents=True, exist_ok=True)
    DETAIL.write_text("file\tline\tcolumn\tstatus\tenum\tvalue\tspelling\treplacement\n" +
                      "".join("%s\t%d\t%d\t%s\t%s\t%d\t%s\t%s\n" % row
                              for row in sorted(rows)))
    print("[enum-case-labels] detail -> %s" % DETAIL.relative_to(REPO))


def main(argv=None):
    ap = argparse.ArgumentParser(prog="gruntz.audit.enum_case_labels")
    ap.add_argument("--apply", action="store_true", help="rewrite the labels")
    ap.add_argument("--detail", action="store_true",
                    help="write build/gen/enum_case_labels.tsv")
    ap.add_argument("--through-casts", action="store_true",
                    help="include switches whose key is an explicit cast of an enum")
    ap.add_argument("--gate", action="store_true", help="exit 1 if any remain")
    ap.add_argument("paths", nargs="*")
    a = ap.parse_args(argv)

    if not CDB.exists():
        sys.exit("[enum-case-labels] build/clangd/compile_commands.json not found - run gruntz init")

    limit = [Path(p).resolve() for p in a.paths] or None
    found, ambiguous, via_cast = scan(limit)
    if a.through_casts:
        for rel, hits in via_cast.items():
            found[rel].extend(hits)
        via_cast = {}
    n = sum(len(v) for v in found.values())

    if a.detail:
        write_detail(found, ambiguous, via_cast)

    by_enum = collections.Counter()
    for hits in found.values():
        for h in hits:
            by_enum[h[4]] += 1
    for ename, c in by_enum.most_common():
        print("  %-24s %4d label(s)" % (ename, c))
    if ambiguous:
        print("  -- %d label(s) SKIPPED: value has alias enumerators --" % len(ambiguous))
        for rel, line, val, ename, names in ambiguous[:8]:
            print("     %s:%d  %s 0x%x -> %s" % (rel, line, ename, val, "/".join(names)))
    if via_cast:
        m = sum(len(v) for v in via_cast.values())
        print("  -- %d label(s) behind an explicit cast (review, then --through-casts) --" % m)
        for rel, hits in sorted(via_cast.items())[:8]:
            print("     %s:%d  %s x%d" % (rel, hits[0][0], hits[0][4], len(hits)))

    if a.apply:
        done = apply(found)
        print("[enum-case-labels] named %d case label(s) across %d file(s)" % (done, len(found)))
        return 0

    print("[enum-case-labels] %d nameable case label(s) across %d file(s)" % (n, len(found)))
    if a.gate and n:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
