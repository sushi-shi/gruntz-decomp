#!/usr/bin/env python3
"""gruntz.analysis.hoist_forward_decls - group forward declarations at the top.

The desired shape (see include/Gruntz/BzState.h) is ONE grouped forward-decl
block placed right after the leading #include block of every .h/.cpp, instead of
individual `class Foo;` / `struct Bar;` lines scattered mid-file next to their
first use. Forward declarations emit NO code and MSVC 5.0 codegen is position-
independent as long as the decl precedes its first use, so this hoist is
MATCHING-NEUTRAL (byte-identical objects).

  python3 -m gruntz.analysis.hoist_forward_decls [--check|--dry-run] paths...

  paths      : .h/.cpp files or directories (dirs recurse *.h/*.cpp). REQUIRED -
               there is no implicit tree-wide default (the tree-wide pass is a
               separate deliberate campaign; pass include/ src/ explicitly).
  --check    : write nothing; exit 1 if any file WOULD change (CI/idempotency).
  --dry-run  : write nothing; just report what would change.
  -q/--quiet : suppress the per-file "ok" lines.

WHAT IT COLLECTS (a "forward declaration"):
  a whole-line, file-scope OR namespace-scope statement matching
    (template<...>)? (class|struct|union|enum( class)?) NAME (: BASE)? ;
  preserving any trailing `// comment` on the line.

WHAT IT LEAVES ALONE (by design - conservative; when in doubt it SKIPs):
  - full definitions (`class Foo { ... };`) - they don't end in a bare `;`.
  - member forward decls inside a class/struct/union body (brace-depth tracked).
  - decls inside a function body, or inside an `extern "C" { }` block (any non-
    namespace brace scope) - hoisting those could change linkage/scope.
  - typedef / using / friend lines (not a plain forward decl).
  - decls guarded by a real `#if/#ifdef/#ifndef` (other than the file's header
    guard) - a conditionally-compiled decl stays inside its guard.
  - macro-generated decls / decls inside a `#define` continuation - the regex
    never matches a `#`-line, and continuation lines are skipped.
  - any file whose braces / preprocessor conditionals don't balance (parser
    couldn't safely reason about scope) - the whole file is SKIPPED + logged.

Namespace-scoped decls are grouped at the TOP of THEIR namespace block (right
after `namespace X {`), never hoisted to file top - that would break scoping.

Idempotent: re-running makes no further change (the grouped block is already at
the anchor, so it is collected and re-emitted in place).
"""
from __future__ import annotations

import argparse
import re
import sys
from collections import Counter
from pathlib import Path

# A standalone forward declaration line (matched against the COMMENT-STRIPPED
# form of the line). Rejects definitions (end in `{`), pointer/value variable
# declarations (`class Foo *bar;`), and anything not ending in a bare `;`.
DECL_RE = re.compile(
    r"^\s*(?:template\s*<[^>]*>\s*)?"
    r"(?:class|struct|union|enum(?:\s+class)?)\s+"
    r"[A-Za-z_]\w*"
    r"(?:\s*:\s*[\w:]+)?"  # enum underlying type (class fwd decls have no base)
    r"\s*;\s*$")

_SKIP_PREFIX = ("typedef", "using", "friend")


def _code_only(line: str, in_block: bool):
    """Return (line with comments/strings/chars blanked out, new in_block state).

    Whitespace/length preserved so column math stays valid; only used for
    scope/brace/decl analysis, never emitted."""
    out = []
    i, n = 0, len(line)
    while i < n:
        c = line[i]
        if in_block:
            if c == "*" and i + 1 < n and line[i + 1] == "/":
                in_block = False
                out.append("  ")
                i += 2
                continue
            out.append(" ")
            i += 1
            continue
        if c == "/" and i + 1 < n and line[i + 1] == "/":
            out.append(" " * (n - i))
            break
        if c == "/" and i + 1 < n and line[i + 1] == "*":
            in_block = True
            out.append("  ")
            i += 2
            continue
        if c == '"' or c == "'":
            q = c
            out.append(" ")
            i += 1
            while i < n:
                if line[i] == "\\" and i + 1 < n:
                    out.append("  ")
                    i += 2
                    continue
                if line[i] == q:
                    out.append(" ")
                    i += 1
                    break
                out.append(" ")
                i += 1
            continue
        out.append(c)
        i += 1
    return "".join(out), in_block


def _directive(lstripped: str) -> str:
    m = re.match(r"#\s*(\w+)", lstripped)
    return m.group(1) if m else ""


def _detect_guard(lines) -> bool:
    """True if the file opens with an `#ifndef X` / `#define X` include guard."""
    in_block = False
    name = None
    i = 0
    # first meaningful (non-blank, non-comment) line must be `#ifndef X`
    while i < len(lines):
        co, in_block = _code_only(lines[i], in_block)
        s = co.strip()
        if s:
            m = re.match(r"#\s*ifndef\s+(\w+)", s)
            if not m:
                return False
            name = m.group(1)
            i += 1
            break
        i += 1
    if name is None:
        return False
    # next meaningful line must be `#define X` (same name)
    while i < len(lines):
        co, in_block = _code_only(lines[i], in_block)
        s = co.strip()
        if s:
            m = re.match(r"#\s*define\s+(\w+)", s)
            return bool(m and m.group(1) == name)
        i += 1
    return False


def _normalize(code: str) -> str:
    """Signature key for dedup: keyword+name with whitespace collapsed."""
    return re.sub(r"\s+", " ", code.strip())


def transform(text: str):
    """Return (new_text, info). info: changed, skipped(reason|None), n_file, n_ns."""
    info = {"changed": False, "skipped": None, "n_file": 0, "n_ns": 0}
    had_final_nl = text.endswith("\n")
    lines = text.split("\n")
    if had_final_nl:
        lines = lines[:-1]
    n = len(lines)

    guard = _detect_guard(lines)
    base_pp = 1 if guard else 0

    code_lines = []
    decl_flags = [False] * n
    ns_target = [None] * n  # innermost enclosing namespace-open idx for a decl

    in_block = False
    pp_cont = False
    pp_depth = 0
    scope = []  # stack of ("ns"|"other", open_line_idx)
    pending = ""  # tokens since last ; { } - classifies the next `{`

    for i, raw in enumerate(lines):
        co, in_block_next = _code_only(raw, in_block)
        code_lines.append(co)
        lst = raw.lstrip()
        is_pp = (not in_block) and lst.startswith("#")
        continuation = pp_cont

        # --- forward-decl detection (at start-of-line scope, before brace scan) ---
        scope_ok = all(k == "ns" for k, _ in scope)
        pp_ok = pp_depth == base_pp
        if (not continuation) and (not is_pp) and (not in_block) and scope_ok and pp_ok:
            s = co.strip()
            if DECL_RE.match(co) and not s.startswith(_SKIP_PREFIX):
                decl_flags[i] = True
                ns_idx = None
                for k, openidx in scope:
                    if k == "ns":
                        ns_idx = openidx
                ns_target[i] = ns_idx

        # --- preprocessor conditional depth ---
        if is_pp and not continuation:
            d = _directive(lst)
            if d in ("if", "ifdef", "ifndef"):
                pp_depth += 1
            elif d == "endif":
                pp_depth = max(0, pp_depth - 1)

        # --- brace scope tracking (skip preprocessor / continuation lines) ---
        if (not is_pp) and (not continuation):
            for c in co:
                if c == "{":
                    kind = "ns" if re.search(r"\bnamespace\b", pending) else "other"
                    scope.append((kind, i))
                    pending = ""
                elif c == "}":
                    if scope:
                        scope.pop()
                    pending = ""
                elif c == ";":
                    pending = ""
                else:
                    pending += c

        pp_cont = raw.rstrip().endswith("\\") and (is_pp or continuation)
        in_block = in_block_next

    # Safety: unbalanced braces / conditionals => parser can't trust its scope.
    if scope or pp_depth != 0:
        info["skipped"] = "unbalanced braces/#if" if scope else "unbalanced #if"
        return text, info

    file_decls = [i for i in range(n) if decl_flags[i] and ns_target[i] is None]
    ns_groups = {}
    for i in range(n):
        if decl_flags[i] and ns_target[i] is not None:
            ns_groups.setdefault(ns_target[i], []).append(i)
    info["n_file"] = len(file_decls)
    info["n_ns"] = sum(len(v) for v in ns_groups.values())

    if not file_decls and not ns_groups:
        return text, info

    # --- anchor for the FILE-scope block: right after the leading include block ---
    prologue_end = n
    for i in range(n):
        if code_lines[i].strip() == "":  # blank or comment-only
            continue
        if lines[i].lstrip().startswith("#"):  # preprocessor
            continue
        if decl_flags[i] and ns_target[i] is None:  # a decl that will move
            continue
        prologue_end = i
        break

    anchor = -1
    for i in range(prologue_end):
        if lines[i].lstrip().startswith("#") and _directive(lines[i].lstrip()) == "include":
            anchor = i
    if anchor == -1:  # no leading include - fall back to guard/pragma/comment
        for i in range(prologue_end):
            lst = lines[i].lstrip()
            if not lst.startswith("#"):
                continue
            d = _directive(lst)
            if d == "define" and guard:
                anchor = i
            elif d == "pragma" and "once" in lst:
                anchor = i
        if anchor == -1:
            for i in range(prologue_end):
                if code_lines[i].strip() == "":
                    anchor = i

    def build_block(indices):
        seen, block = set(), []
        for i in indices:
            core = _normalize(code_lines[i])
            if core in seen:
                continue
            seen.add(core)
            block.append(lines[i].rstrip())
        return block

    blocks = {}
    if file_decls:
        blocks["FILE"] = build_block(file_decls)
    for idx, members in ns_groups.items():
        blocks[("NS", idx)] = build_block(members)

    tag_by_idx = {}
    if "FILE" in blocks and anchor >= 0:
        tag_by_idx[anchor] = "FILE"
    for idx in ns_groups:
        tag_by_idx[idx] = ("NS", idx)

    decl_idx_set = set(file_decls)
    for members in ns_groups.values():
        decl_idx_set.update(members)

    # --- strip decl lines (collapsing a blank pair a removal would create) ---
    stripped = []  # list of [text, tag]
    swallow = False
    for i, line in enumerate(lines):
        if i in decl_idx_set:
            if stripped and stripped[-1][0].strip() == "":
                swallow = True
            continue
        if swallow and line.strip() == "":
            swallow = False
            continue
        swallow = False
        stripped.append([line, tag_by_idx.get(i)])

    # --- re-insert each grouped block right after its anchor / namespace-open ---
    out = []
    j, m = 0, len(stripped)
    file_block_placed = False
    while j < m:
        text_j, tag = stripped[j]
        out.append(text_j)
        j += 1
        block = blocks.get(tag) if tag is not None else None
        if block:
            if tag == "FILE":
                file_block_placed = True
            if not text_j.rstrip().endswith("{"):  # no blank right after `namespace X {`
                out.append("")
            out.extend(block)
            while j < m and stripped[j][0].strip() == "":  # consume existing blanks
                j += 1
            if j < m:
                out.append("")

    # anchor == -1 fallback: prepend the file block at the very top.
    if "FILE" in blocks and not file_block_placed:
        out = blocks["FILE"] + [""] + out

    new_text = "\n".join(out)
    if had_final_nl:
        new_text += "\n"
    info["changed"] = new_text != text
    return new_text, info


def _iter_files(paths):
    for p in paths:
        path = Path(p)
        if path.is_dir():
            for f in sorted(path.rglob("*")):
                if f.suffix in (".h", ".cpp", ".hpp", ".cc") and "vendor" not in f.parts:
                    yield f
        elif path.is_file():
            if "vendor" not in path.parts:
                yield path


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("paths", nargs="+", help=".h/.cpp files or directories to process")
    ap.add_argument("--check", action="store_true",
                    help="write nothing; exit 1 if any file would change")
    ap.add_argument("--dry-run", action="store_true", help="write nothing; report changes")
    ap.add_argument("-q", "--quiet", action="store_true")
    args = ap.parse_args()

    would_change = 0
    skipped = 0
    for f in _iter_files(args.paths):
        text = f.read_text()
        new, info = transform(text)
        tag = f"{str(f):48} file={info['n_file']:2d} ns={info['n_ns']:2d}"
        if info["skipped"]:
            skipped += 1
            if not args.quiet:
                print(f"SKIP  {tag}  ({info['skipped']})")
        elif info["changed"]:
            would_change += 1
            if args.check or args.dry_run:
                if not args.quiet:
                    print(f"WOULD {tag}")
            else:
                # Safety: never fabricate/corrupt content. The new non-blank line
                # multiset must be a subset of the old (we only MOVE lines, DEDUP
                # duplicate decls, and add/remove blank separators).
                def _nb(s):
                    return Counter(ln.rstrip() for ln in s.split("\n") if ln.strip())
                if _nb(new) - _nb(text):
                    print(f"ABORT (content changed) {f}", file=sys.stderr)
                    sys.exit(2)
                f.write_text(new)
                print(f"HOIST {tag}")
        elif not args.quiet:
            print(f"ok    {tag}")

    if not args.quiet:
        print(f"\n{would_change} file(s) would change, {skipped} skipped.")
    if args.check and would_change:
        sys.exit(1)


if __name__ == "__main__":
    main()
