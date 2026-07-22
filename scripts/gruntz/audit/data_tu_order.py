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
    python -m gruntz.audit.data_tu_order            # cross-file interleave audit
    python -m gruntz.audit.data_tu_order --within   # per-file ordering audit
    python -m gruntz.audit.data_tu_order --json
"""

import argparse
import glob
import os
import re

DATA_RE = re.compile(r"\bDATA\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")
# VTBL(name, rva) and DATA_SYMBOL(rva, size, sym) rows live beside DATA() in the
# owning TU and join the within-file ordering audit (they are data rows).
VTBL_ORD_RE = re.compile(r"\bVTBL\s*\(\s*\w+\s*,\s*(0x[0-9a-fA-F]+)\s*\)")
DSYM_ORD_RE = re.compile(r"\bDATA_SYMBOL\s*\(\s*(0x[0-9a-fA-F]+)\s*,[^)]*,\s*([^\s,)]+)\s*\)")
# strip // and /* */ comments crudely for classification (not for line tracking)
IDENT_RE = re.compile(r"([A-Za-z_]\w*)\s*(\[|;|=|\()")


def repo_root():
    here = os.path.abspath(__file__)
    # scripts/gruntz/audit/data_tu_order.py -> repo root is 3 up from scripts
    p = here
    for _ in range(4):
        p = os.path.dirname(p)
    return p


# A datum is COMDAT (linker-pooled per-segment, NON-linear) if its symbol name is
# a compiler-generated vague-linkage kind (vtable ??_7, RTTI ??_R, string ??_C /
# $S, vbtable ??_8, local static guard/init ??__), or if the delinker attributed
# the SAME rva to more than one object (a folded/replicated COMDAT). Only ORDINARY
# (single-owner, ordinarily-named) data is linearly TU-attributed and thus subject
# to the per-(TU, storage) band invariant.
COMDAT_NAME_RE = re.compile(r"^(?:\?\?_[789A-DR]|\?\?__|\$S|_?\$SG)")


def load_manifest(root):
    """rva -> storage, plus the set of COMDAT (pooled) rvas, from the delinker's
    data manifest. Returns ({}, set()) if the manifest is absent (audit degrades
    to name-only COMDAT detection)."""
    path = os.path.join(root, "build", "gen", "delink_data_manifest.tsv")
    rva2storage = {}
    objs_at = {}  # rva -> set(object)
    names_at = {}  # rva -> a symbol name
    if not os.path.exists(path):
        return rva2storage, set()
    with open(path, errors="replace") as fh:
        header = fh.readline()
        for ln in fh:
            c = ln.rstrip("\n").split("\t")
            if len(c) < 5:
                continue
            try:
                rva = int(c[2], 16)
            except ValueError:
                continue
            rva2storage.setdefault(rva, c[4])
            objs_at.setdefault(rva, set()).add(c[1])
            names_at.setdefault(rva, c[0])
    comdat = set()
    for rva, objs in objs_at.items():
        if len(objs) > 1 or COMDAT_NAME_RE.match(names_at.get(rva, "")):
            comdat.add(rva)
    return rva2storage, comdat


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
        for m in VTBL_ORD_RE.finditer(code):
            blocks.append(Block(int(m.group(1), 16), i + 1, True, "??_7", code.strip()[:80]))
        for m in DSYM_ORD_RE.finditer(code):
            blocks.append(Block(int(m.group(1), 16), i + 1, True, m.group(2)[:40],
                                code.strip()[:80]))
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


def cross_file_audit(files, pool_threshold=4, rva2storage=None, comdat=None):
    """Band ORDINARY (non-COMDAT) data per (file, storage) and find defs sitting
    strictly inside ANOTHER file's SAME-storage band.

    Storage-aware: a TU contributes an independent contiguous run to EACH of
    .rdata/.data/.bss, so bands are keyed by (file, storage) - a single all-storage
    envelope (the old model) engulfed every multi-segment TU into a false pool.
    COMDAT data (vtables/RTTI/strings/guards) is linker-pooled, not linearly
    attributed, so it is excluded from the band model entirely."""
    rva2storage = rva2storage or {}
    comdat = comdat or set()

    def is_comdat(rva, name):
        return (rva in comdat) or (name is not None and COMDAT_NAME_RE.match(name)) \
            or name == "??_7"

    # (rva, path, name, storage) for every ordinary storage-emitting def
    defs = []
    for path in files:
        for b in parse_file(path):
            if not b.is_def or is_comdat(b.rva, b.name):
                continue
            storage = rva2storage.get(b.rva, "data")
            defs.append((b.rva, path, b.name, storage))

    # band per (path, storage)
    byband = {}
    for rva, path, name, storage in defs:
        byband.setdefault((path, storage), []).append(rva)
    bands = {key: (min(v), max(v), len(v)) for key, v in byband.items()}

    contains = {}       # (cpath,storage) -> set of foreign paths inside
    crossings = []      # (rva, owner_path, name, container_path, storage)
    foreign_inside = {}
    for rva, path, name, storage in sorted(defs):
        for (cpath, cstorage), (lo, hi, cnt) in bands.items():
            if cpath == path or cstorage != storage:
                continue
            if lo < rva < hi:
                contains.setdefault((cpath, cstorage), set()).add(path)
                foreign_inside[(cpath, cstorage)] = foreign_inside.get((cpath, cstorage), 0) + 1
                crossings.append((rva, path, name, cpath, storage))
    # a (file,storage) band that swallows many foreign defs is a pool/scattered run
    pools = set()  # set of (path, storage)
    for key, (lo, hi, cnt) in bands.items():
        if foreign_inside.get(key, 0) >= cnt or len(contains.get(key, ())) >= pool_threshold:
            pools.add(key)
    return defs, bands, crossings, pools, contains


BASELINE = os.path.join("config", "data-tu-order-baseline.tsv")


def real_crossings(root, pool_threshold=4):
    """The ordinary-data interleave violations (container is not a pool)."""
    files = sorted(glob.glob(os.path.join(root, "src", "**", "*.cpp"), recursive=True))
    files = [f for f in files if "/Stub/" not in f]
    rva2storage, comdat = load_manifest(root)
    defs, bands, crossings, pools, contains = cross_file_audit(
        files, pool_threshold, rva2storage, comdat)
    real = [c for c in crossings if (c[3], c[4]) not in pools]
    return real, defs, bands, pools, contains


def crossing_key(c):
    rva, owner, name, container, storage = c
    return (f"{rva:#010x}", name or "?", os.path.basename(owner),
            os.path.basename(container), storage)


def load_baseline(root):
    path = os.path.join(root, BASELINE)
    keys = set()
    if not os.path.exists(path):
        return keys
    with open(path, errors="replace") as fh:
        for ln in fh:
            ln = ln.split("#", 1)[0].strip()
            if not ln:
                continue
            parts = ln.split("\t")
            if len(parts) == 5:
                keys.add(tuple(parts))
    return keys


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--within", action="store_true", help="per-file ordering audit")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--pool-threshold", type=int, default=4)
    ap.add_argument("--ratchet", action="store_true",
                    help="FATAL if an ordinary-data def lands in another TU's band "
                         "and is not in config/data-tu-order-baseline.tsv")
    ap.add_argument("--write-baseline", action="store_true",
                    help="(re)write the baseline from the current crossings")
    ap.add_argument("--root", default=None)
    args = ap.parse_args()

    root = args.root or repo_root()

    if args.within:
        files = sorted(glob.glob(os.path.join(root, "src", "**", "*.cpp"), recursive=True))
        files = [f for f in files if "/Stub/" not in f]
        probs = within_file_audit(files)
        print(f"== within-file DATA ordering: {len(probs)} file(s) with an out-of-order run ==")
        for path, run in probs:
            rel = os.path.relpath(path, root)
            print(f"\n{rel}")
            for b in run:
                flag = "def " if b.is_def else "extn"
                print(f"    L{b.line:<5} {b.rva:#010x} {flag} {b.name}")
        return

    real, defs, bands, pools, contains = real_crossings(root, args.pool_threshold)
    keys = {crossing_key(c) for c in real}

    if args.write_baseline:
        path = os.path.join(root, BASELINE)
        with open(path, "w") as fh:
            fh.write("# data-tu-order baseline: ordinary-data defs that land inside another\n")
            fh.write("# TU's same-storage band (known scattered singletons / accepted homes).\n")
            fh.write("# Regenerate: python -m gruntz.audit.data_tu_order --write-baseline\n")
            fh.write("# rva\tname\towner_cpp\tcontainer_cpp\tstorage\n")
            for k in sorted(keys):
                fh.write("\t".join(k) + "\n")
        print(f"wrote {len(keys)} baseline crossings -> {BASELINE}")
        return

    if args.ratchet:
        base = load_baseline(root)
        new = keys - base
        stale = base - keys
        if new:
            print(f"[data-tu-order] FATAL: {len(new)} NEW ordinary-data interleave(s) "
                  f"(a DATA def landed inside another TU's same-storage band):")
            for k in sorted(new):
                rva, name, owner, container, storage = k
                print(f"   {rva} {name:28s} in {owner:32s} INSIDE {container} .{storage} band")
            print("   Home the def in its real owner TU (RVA-contiguous), or - if it is a")
            print("   proven scattered singleton - bless via --write-baseline.")
            raise SystemExit(1)
        msg = f"[data-tu-order] OK - no new ordinary-data interleaves ({len(base)} baselined"
        print(msg + (f", {len(stale)} baseline row(s) now clean)" if stale else ")"))
        return

    print(f"== cross-TU DATA interleave audit (storage-aware) ==")
    print(f"   {len(defs)} ordinary storage-emitting defs across {len(bands)} (file,storage) bands")
    print(f"   pools (band swallows >= {args.pool_threshold} other files' defs): {len(pools)}")
    for (p, storage) in sorted(pools):
        lo, hi, cnt = bands[(p, storage)]
        print(f"     POOL  {os.path.relpath(p, root):40s} .{storage:5s} [{lo:#08x},{hi:#08x}] "
              f"{cnt} defs, contains {len(contains[(p, storage)])} files")
    print()
    print(f"   {len(real)} interleave violation(s) (container is NOT a pool):")
    for rva, owner, name, container, storage in sorted(real):
        lo, hi, cnt = bands[(container, storage)]
        print(f"     {rva:#010x} {name or '?':28s} in {os.path.relpath(owner, root):30s} "
              f"INSIDE {os.path.relpath(container, root)} .{storage} [{lo:#08x},{hi:#08x}]")


if __name__ == "__main__":
    main()
