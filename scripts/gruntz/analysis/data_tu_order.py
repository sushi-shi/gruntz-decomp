"""data_tu_order - the DATA analog of tu_order_check (the .text invariant).

Each retail .obj contributes a CONTIGUOUS run to .data/.rdata/.bss, exactly like
it does to .text. So every global a given .cpp *defines* (a DATA(0xRVA) on a
STORAGE-EMITTING declaration - not a bare `extern`) should fall inside that TU's
own contiguous RVA band. A definition whose RVA lands strictly inside ANOTHER
TU's band is either misplaced (defined in the wrong .cpp) or evidence the
partition is wrong - the DATA counterpart of the .text interleaving flagged by
`tu_order_check` / `interleavers`.

Two reports:

  --within   per-file: DATA blocks that are out of ascending RVA order WITHIN a
             contiguous globals run (only comment/decl/blank lines between them).
             These can be tidied by an in-place reorder (byte-neutral).

  (default)  cross-file: every definition RVA that falls strictly inside another
             file's band. Pools (files whose band swallows many other files'
             defs - e.g. the consolidated src/Globals.cpp) are listed separately
             and NOT counted as violations.

Byte-neutral: DATA is delinked per-symbol by absolute RVA, so neither reorder nor
re-home may move match %. Keep each DATA() glued to its declaration.

Usage:
    python -m gruntz.analysis.data_tu_order            # cross-file interleave audit
    python -m gruntz.analysis.data_tu_order --within   # per-file ordering audit
    python -m gruntz.analysis.data_tu_order --json
"""

import argparse
import glob
import os
import re
import sys

DATA_RE = re.compile(r"\bDATA\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")
# strip // and /* */ comments crudely for classification (not for line tracking)
IDENT_RE = re.compile(r"([A-Za-z_]\w*)\s*(\[|;|=|\()")


def repo_root():
    here = os.path.abspath(__file__)
    # scripts/gruntz/analysis/data_tu_order.py -> repo root is 3 up from scripts
    p = here
    for _ in range(4):
        p = os.path.dirname(p)
    return p


def strip_line_comment(s):
    # remove a trailing // comment (naive: no // inside strings in these decls)
    i = s.find("//")
    return s[:i] if i >= 0 else s


class Block:
    """One DATA()+declaration unit."""
    __slots__ = ("rva", "line", "is_def", "name", "decl_text")

    def __init__(self, rva, line, is_def, name, decl_text):
        self.rva = rva
        self.line = line
        self.is_def = is_def
        self.name = name
        self.decl_text = decl_text


TYPE_KW = {"extern", "static", "const", "volatile", "unsigned", "signed",
           "struct", "class", "enum", "register", "inline"}


def classify(decl_text):
    """Return (is_definition, name). decl_text is the joined declaration that the
    DATA() binds to (from just after the macro up to the terminating ';').

    C++ linkage rule ([dcl.link]): a declaration directly contained in an
    `extern "C"` linkage-spec is treated as if it carried the `extern` specifier,
    so it is a DECLARATION (emits no storage) UNLESS it has an initializer. Same
    for a plain `extern T x;`. Everything else (a tentative definition `T x;`, or
    any `... = ...`) emits storage in THIS obj -> it is the band owner."""
    t = decl_text.strip()
    linkage_extern = False
    m = re.match(r'extern\s*"C"\s*\{?', t)
    if m:
        linkage_extern = True  # extern "C" implies the extern specifier
        t = t[m.end():].strip()
    if re.match(r"\bextern\b", t):
        linkage_extern = True
        t = re.sub(r"^\bextern\b\s*", "", t)
    has_init = "=" in decl_text
    is_def = (not linkage_extern) or has_init
    # variable name = the last identifier before the first declarator char ([ = ;)
    stops = [p for p in (t.find("["), t.find("="), t.find(";")) if p >= 0]
    head = t[: min(stops)] if stops else t
    ids = [i for i in re.findall(r"[A-Za-z_]\w*", head) if i not in TYPE_KW]
    name = ids[-1] if ids else None
    return is_def, name


def parse_file(path):
    """Return list[Block] for one file, in file order."""
    with open(path, errors="replace") as fh:
        lines = fh.readlines()
    blocks = []
    in_block_comment = False
    i = 0
    n = len(lines)
    while i < n:
        raw = lines[i]
        # track /* */ block comments (line-granular; good enough - DATA is never
        # split across a block comment)
        line_wo = raw
        if in_block_comment:
            end = line_wo.find("*/")
            if end < 0:
                i += 1
                continue
            line_wo = line_wo[end + 2:]
            in_block_comment = False
        # remove inline /* */ and detect an opening one
        while True:
            s = line_wo.find("/*")
            if s < 0:
                break
            e = line_wo.find("*/", s + 2)
            if e < 0:
                in_block_comment = True
                line_wo = line_wo[:s]
                break
            line_wo = line_wo[:s] + line_wo[e + 2:]
        code = strip_line_comment(line_wo)
        for m in DATA_RE.finditer(code):
            rva = int(m.group(1), 16)
            # gather the declaration text starting just after this macro
            tail = code[m.end():]
            decl_parts = [tail]
            j = i
            # if the ';' isn't on this line, keep pulling following lines
            def joined():
                return " ".join(p.strip() for p in decl_parts)
            while ";" not in "".join(decl_parts) and j + 1 < n:
                j += 1
                nxt = strip_line_comment(lines[j])
                # stop if we hit another DATA (orphan) - shouldn't happen in valid tree
                if DATA_RE.search(nxt):
                    break
                decl_parts.append(nxt)
            decl_text = joined()
            is_def, name = classify(decl_text)
            blocks.append(Block(rva, i + 1, is_def, name, decl_text.strip()[:80]))
        i += 1
    return blocks


def within_file_audit(files):
    """Detect out-of-order DATA within a contiguous run (only blocks whose file
    lines are 'close' - separated by <= GAP lines, i.e. a globals section rather
    than globals scattered next to their functions)."""
    problems = []
    for path in files:
        blocks = parse_file(path)
        if len(blocks) < 2:
            continue
        # group into runs: consecutive blocks whose line distance is small
        runs = []
        cur = [blocks[0]]
        for b in blocks[1:]:
            if b.line - cur[-1].line <= 12:
                cur.append(b)
            else:
                runs.append(cur)
                cur = [b]
        runs.append(cur)
        for run in runs:
            if len(run) < 2:
                continue
            rvas = [b.rva for b in run]
            if any(rvas[k] > rvas[k + 1] for k in range(len(rvas) - 1)):
                problems.append((path, run))
    return problems


def cross_file_audit(files, pool_threshold=4):
    """Map def-rva -> file; find defs sitting strictly inside another file's band."""
    defs = []  # (rva, path, name)
    for path in files:
        for b in parse_file(path):
            if b.is_def:
                defs.append((b.rva, path, b.name))
    # per-file band over definitions
    bands = {}
    byfile = {}
    for rva, path, name in defs:
        byfile.setdefault(path, []).append(rva)
    for path, rvas in byfile.items():
        bands[path] = (min(rvas), max(rvas), len(rvas))
    # for each file, which OTHER files' defs fall strictly inside its band
    contains = {}  # path -> set of foreign paths whose defs land inside
    crossings = []  # (rva, owner_path(where defined), container_path)
    foreign_inside = {}  # container -> count of foreign DEFS strictly inside its band
    def_sorted = sorted(defs)
    for rva, path, name in def_sorted:
        for cpath, (lo, hi, cnt) in bands.items():
            if cpath == path:
                continue
            if lo < rva < hi:
                contains.setdefault(cpath, set()).add(path)
                foreign_inside[cpath] = foreign_inside.get(cpath, 0) + 1
                crossings.append((rva, path, name, cpath))
    # A "pool"/scattered TU: its band holds as many (or more) FOREIGN defs than its
    # own -> the band model does not describe a clean contiguous run for it (either
    # a designed consolidation like Globals.cpp, or its own defs are placed next to
    # their functions). Re-homing into/out of these needs the real owner proven.
    pools = set()
    for p, (lo, hi, cnt) in bands.items():
        fi = foreign_inside.get(p, 0)
        if fi >= cnt or len(contains.get(p, ())) >= pool_threshold:
            pools.add(p)
    return defs, bands, crossings, pools, contains


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--within", action="store_true", help="per-file ordering audit")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--pool-threshold", type=int, default=4)
    ap.add_argument("--root", default=None)
    args = ap.parse_args()

    root = args.root or repo_root()
    files = sorted(
        glob.glob(os.path.join(root, "src", "**", "*.cpp"), recursive=True)
    )
    files = [f for f in files if "/Stub/" not in f]

    if args.within:
        probs = within_file_audit(files)
        print(f"== within-file DATA ordering: {len(probs)} file(s) with an out-of-order run ==")
        for path, run in probs:
            rel = os.path.relpath(path, root)
            print(f"\n{rel}")
            for b in run:
                flag = "def " if b.is_def else "extn"
                print(f"    L{b.line:<5} {b.rva:#010x} {flag} {b.name}")
        return

    defs, bands, crossings, pools, contains = cross_file_audit(
        files, args.pool_threshold
    )
    # filter crossings: drop those whose container is a pool
    real = [c for c in crossings if c[3] not in pools]
    print(f"== cross-TU DATA interleave audit ==")
    print(f"   {len(defs)} storage-emitting DATA definitions across {len(bands)} files")
    print(f"   pools (band swallows >= {args.pool_threshold} other files' defs): {len(pools)}")
    for p in sorted(pools):
        lo, hi, cnt = bands[p]
        print(f"     POOL  {os.path.relpath(p, root):40s} band [{lo:#08x},{hi:#08x}] {cnt} defs, contains {len(contains[p])} files")
    print()
    print(f"   {len(real)} interleave violation(s) (container is NOT a pool):")
    # group by (container, owner)
    for rva, owner, name, container in sorted(real):
        lo, hi, cnt = bands[container]
        print(f"     {rva:#010x} {name or '?':28s} defined in {os.path.relpath(owner, root):34s} "
              f"INSIDE band of {os.path.relpath(container, root)} [{lo:#08x},{hi:#08x}]")


if __name__ == "__main__":
    main()
