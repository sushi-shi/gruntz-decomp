#!/usr/bin/env python3
"""migrate_rva_macros.py - one-shot: `// @address:` comments -> src/rva.h macros.

Rewrites every annotation SITE across src/ from the old comment convention to the
clang `annotate`-attribute macros (src/rva.h), so labels.py can read them from
LLVM IR instead of a fragile positional join. Idempotent: a TU already using the
macros (or with no annotations) is left untouched.

Per matched-FUNCTION block:
    // @address: 0x13dc70     ->   RVA(0x13dc70, 0x1d)
    // @size:    0x1d              <function definition>
    <function definition>
  (a block with no `@size` becomes `RVAU(0x13dc70)` - MSVC 5.0 has no variadic
  macros, so RVA is fixed 2-arity and RVAU is the 1-arg unsized variant.)
  An adjacent `// @symbol: ?Mangled@@...` becomes a separate `SYMBOL(...)` line.

Per matched-GLOBAL extern:
    // @data: 0x253c70        ->   DATA(0x253c70)
    extern int g_foo;              extern int g_foo;

@stub blocks keep their `// @confidence`/`// @source`/`// @stub` documentary
comments (verify_stub_labels.py owns those); only their `@address`/`@size` move
into an RVA(...) macro on the line above the definition.

The `#include "../rva.h"` is inserted after the TU's first local `#include "..."`.
Run from the repo root:  python3 scripts/analysis/migrate_rva_macros.py [--check]
"""

import argparse
import re
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[2])
SRC = REPO / "src"

ADDR_RE = re.compile(r"^\s*//\s*@address:\s*(0x[0-9a-fA-F]+)\s*$")
SIZE_RE = re.compile(r"^\s*//\s*@size:\s*(0x[0-9a-fA-F]+|\d+)\s*$")
DATA_RE = re.compile(r"^\s*//\s*@data:\s*(0x[0-9a-fA-F]+)\s*$")
SYM_RE = re.compile(r"^\s*//\s*@symbol:\s*(\S+)\s*$")
COMMENT_RE = re.compile(r"^\s*//")
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
MACRO_USE_RE = re.compile(r"\b(?:RVA|DATA|SYMBOL)\s*\(")


def rel_rva_include(path: Path) -> str:
    """Repo-relative `../`-path from the TU to src/rva.h (src/<Dir>/x.cpp ->
    ../rva.h; deeper nesting adds another ../)."""
    depth = len(path.relative_to(SRC).parts) - 1   # dirs between SRC and the file
    return "../" * depth + "rva.h"


def convert(text: str, path: Path) -> str:
    lines = text.splitlines(keepends=True)
    out = []
    emitted = False        # did we actually write a macro? (gates the include)
    i = 0
    n = len(lines)
    while i < n:
        line = lines[i]

        # @data: 0x.. on an extern -> DATA(0x..)
        md = DATA_RE.match(line)
        if md:
            indent = line[:len(line) - len(line.lstrip())]
            out.append(f"{indent}DATA({md.group(1)})\n")
            emitted = True
            i += 1
            continue

        # A `// @address:` line starts a function (or stub) block. Collect the
        # contiguous `//` comment run it belongs to, pull @address/@size/@symbol,
        # and keep any non-tag comment lines (incl. @stub documentary tags).
        if ADDR_RE.match(line):
            # @address may sit anywhere in a contiguous `//` comment run; scan
            # forward to the run's end, pulling @address/@size/@symbol and
            # preserving every other comment line (descriptions, @stub tags).
            j = i
            addr = size = sym = None
            kept = []          # comment lines to preserve (e.g. @stub tags)
            while j < n and COMMENT_RE.match(lines[j]):
                ma = ADDR_RE.match(lines[j])
                ms = SIZE_RE.match(lines[j])
                msy = SYM_RE.match(lines[j])
                if ma:
                    addr = ma.group(1)
                elif ms:
                    size = ms.group(1)
                elif msy:
                    sym = msy.group(1)
                else:
                    kept.append(lines[j])
                j += 1
            indent = line[:len(line) - len(line.lstrip())]

            def _is_blank_comment(s):
                return s.strip() == "//"

            def _is_divider(s):
                return bool(re.match(r"^\s*//\s*-{3,}\s*$", s))

            # @address/@size usually sit at the tail of a comment block under a
            # `// ----` divider, often after a blank `//`. With them gone the RVA
            # macro is the separator. Trim trailing blank-`//` / `// ----` lines
            # that bracketed the removed tags - both the ones we just gathered
            # forward (`kept`, e.g. a closing divider after @size) and the ones
            # already emitted above @address (`out`, e.g. a blank `//` before it).
            # `@stub` and description text are preserved verbatim.
            while kept and (_is_blank_comment(kept[-1]) or _is_divider(kept[-1])):
                kept.pop()
            if not kept:
                while out and (_is_blank_comment(out[-1]) or _is_divider(out[-1])):
                    out.pop()
            for k in kept:
                out.append(k)
            if sym:
                out.append(f"{indent}SYMBOL({sym})\n")
            if size is not None:
                out.append(f"{indent}RVA({addr}, {size})\n")
            else:
                out.append(f"{indent}RVAU({addr})\n")
            emitted = True
            i = j
            continue

        out.append(line)
        i += 1

    new = "".join(out)
    if emitted:
        new = ensure_include(new, path)
    return new


def ensure_include(text: str, path: Path) -> str:
    if '"../rva.h"' in text or '"rva.h"' in text or 'rva.h"' in text:
        return text
    inc = rel_rva_include(path)
    lines = text.splitlines(keepends=True)
    # Insert after the first local `#include "..."`; fall back to before the
    # first `#include <...>`, else at the top.
    last_local = None
    first_any = None
    for idx, line in enumerate(lines):
        if INCLUDE_RE.match(line):
            last_local = idx
        if first_any is None and re.match(r"^\s*#\s*include", line):
            first_any = idx
    insert_at = (last_local + 1) if last_local is not None else \
                (first_any if first_any is not None else 0)
    lines.insert(insert_at, f'#include "{inc}"\n')
    return "".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--check", action="store_true",
                    help="report files that would change; do not write.")
    ap.add_argument("paths", nargs="*", help="limit to these files (default: all "
                    "src/**/*.cpp).")
    args = ap.parse_args()

    if args.paths:
        files = [Path(p).resolve() for p in args.paths]
    else:
        files = sorted(SRC.rglob("*.cpp"))

    changed = []
    for f in files:
        text = f.read_text()
        # Skip TUs with nothing to convert (no @address/@data comment sites).
        if not re.search(r"//\s*@(?:address|data):", text):
            continue
        new = convert(text, f)
        if new != text:
            changed.append(f)
            if not args.check:
                f.write_text(new)

    rel = lambda p: p.relative_to(REPO)
    for f in changed:
        print(("would change " if args.check else "migrated ") + str(rel(f)))
    print(f"{len(changed)} file(s) "
          + ("would be migrated" if args.check else "migrated"), file=sys.stderr)
    return 1 if (args.check and changed) else 0


if __name__ == "__main__":
    raise SystemExit(main())
