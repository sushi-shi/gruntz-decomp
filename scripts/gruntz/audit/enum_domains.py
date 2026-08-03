#!/usr/bin/env python3
"""gruntz.audit.enum_domains - the enum-domain layer's structural gate.

Checks the invariants the numeric-domain campaign depends on
(docs/enum-modeling-plan.md, docs/patterns/enum-domains.md):

  1. SPLIT-WIDTH AGREEMENT (fatal). A domain declared
     `GZ_ENUM_BEGIN_SPLIT(N, S)` states that its narrow storage is S. Every
     `GZ_ENUM_STORAGE(N, S2)` for that domain must use the SAME S. A mismatch
     means two different beliefs about how many bytes retail stores - exactly
     the "u8 here, i32 there" defect the layer exists to prevent. It is also the
     one check that can catch a class-layout change before class_sizes does.

  2. STORAGE NAMES A REAL DOMAIN (fatal). `GZ_ENUM_STORAGE(N, S)` where N was
     never declared is a typo that silently expands to the raw storage type in
     the retail build and would only fail in the strict build.

  3. NO BARE `enum X { ... }` IN A HEADER (fatal). Shared domains go through the
     macros, or the strict C++20 view never sees them as `enum class` and the
     dual-build guarantee is void. TU-private enums in a .cpp are allowed and
     tracked by the `.cpp-local enums` cleanliness metric instead.
     EXEMPT: a single-enumerator enum is a TAG TYPE for overload dispatch
     (`enum ENoSeed { NO_SEED };` -> `CResolveNode(ENoSeed)`), not a numeric
     domain. It models no value, so `enum class ... : i32` + `using enum` would
     be actively wrong for it.

  4. EXPLICIT ENUMERATOR VALUES (report). Implicit numbering is how `enum Tool`
     silently ended up one off from the documented ID space. Reported, not fatal,
     because a genuine 0..N-1 index domain is legitimately implicit.

  5. REVIEW-MANIFEST CONSISTENCY (fatal). config/enum-review.tsv must list every
     source file exactly once with a known state.

    python -m gruntz.audit.enum_domains            # report
    python -m gruntz.audit.enum_domains --gate     # exit 1 on any fatal finding
    python -m gruntz.audit.enum_domains --sync-review   # add missing review rows
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
REVIEW = REPO / "config" / "enum-review.tsv"
STATES = {"pending", "reviewed", "third-party"}

DECL = re.compile(r"\bGZ_ENUM_BEGIN\(\s*(\w+)\s*\)")
DECL_SPLIT = re.compile(r"\bGZ_ENUM_BEGIN_SPLIT\(\s*(\w+)\s*,\s*(\w+)\s*\)")
DECL_FLAGS = re.compile(r"\bGZ_ENUM_FLAGS_BEGIN\(\s*(\w+)\s*,\s*(\w+)\s*\)")
DECL_CONST = re.compile(r"\bGZ_ENUM_CONST_BEGIN\(\s*(\w+)\s*\)")
FORWARD = re.compile(r"\bGZ_ENUM_FORWARD(?:_SPLIT)?\(\s*(\w+)\s*(?:,\s*(\w+)\s*)?\)")
STORAGE = re.compile(r"\bGZ_ENUM_(?:STORAGE|STORAGE_STEPPED|PARAM|RETURN|BITFIELD)"
                     r"\(\s*(\w+)\s*,\s*(\w+)\s*\)")
BARE_ENUM = re.compile(r"^[ \t]*(?:typedef[ \t]+)?enum[ \t]+(\w+)[ \t]*\{(?P<body>[^}]*)\}", re.M)


def is_tag_type(body: str) -> bool:
    """A single-enumerator, value-less enum is an overload-dispatch TAG, not a domain."""
    names = [x.strip() for x in body.split(",") if x.strip()]
    return len(names) == 1 and "=" not in body
# an enumerator line inside a domain block: `NAME = value,` or a bare `NAME,`
ENUMERATOR = re.compile(r"^[ \t]*([A-Za-z_]\w*)[ \t]*(=)?", re.M)

_BLOCK = re.compile(r"/\*.*?\*/", re.DOTALL)
_LINE = re.compile(r"//[^\n]*")


def strip(text: str) -> str:
    return _LINE.sub("", _BLOCK.sub(" ", text))


def sources() -> list[Path]:
    out: list[Path] = []
    for root in ("src", "include"):
        for ext in ("*.cpp", "*.h"):
            out.extend((REPO / root).rglob(ext))
    return sorted(out)


def domain_blocks(text: str):
    """Yield (name, kind, storage, body) for each GZ_ENUM_*_BEGIN .. _END block."""
    pat = re.compile(
        r"\bGZ_ENUM_(BEGIN|BEGIN_SPLIT|CONST_BEGIN|FLAGS_BEGIN)"
        r"\(\s*(\w+)\s*(?:,\s*(\w+)\s*)?\)(?P<body>.*?)"
        r"\bGZ_ENUM_(?:END|END_SPLIT|CONST_END|FLAGS_END)\(", re.S)
    for m in pat.finditer(text):
        yield m.group(2), m.group(1), m.group(3), m.group("body")


def audit():
    fatal: list[str] = []
    warn: list[str] = []

    declared: dict[str, str | None] = {}      # domain -> declared narrow storage
    decl_site: dict[str, str] = {}

    files = sources()
    for f in files:
        rel = str(f.relative_to(REPO))
        t = strip(f.read_text(errors="replace"))
        for m in DECL.finditer(t):
            declared.setdefault(m.group(1), None); decl_site.setdefault(m.group(1), rel)
        for m in DECL_CONST.finditer(t):
            declared.setdefault(m.group(1), None); decl_site.setdefault(m.group(1), rel)
        for rx in (DECL_SPLIT, DECL_FLAGS):
            for m in rx.finditer(t):
                declared[m.group(1)] = m.group(2); decl_site.setdefault(m.group(1), rel)
        for m in FORWARD.finditer(t):
            declared.setdefault(m.group(1), m.group(2))

    # (1) + (2): every storage use agrees with its domain's declaration
    for f in files:
        rel = str(f.relative_to(REPO))
        t = strip(f.read_text(errors="replace"))
        for m in STORAGE.finditer(t):
            dom, st = m.group(1), m.group(2)
            line = t[:m.start()].count("\n") + 1
            if dom not in declared:
                fatal.append(f"{rel}:{line}: GZ_ENUM_STORAGE names undeclared domain "
                             f"'{dom}' - typo, or the domain header is missing")
                continue
            want = declared[dom]
            if want is not None and st != want:
                fatal.append(f"{rel}:{line}: '{dom}' is declared with narrow storage "
                             f"'{want}' ({decl_site.get(dom, '?')}) but stored as '{st}' here "
                             f"- two beliefs about retail's field width")

    # (3) bare enum in a HEADER
    for f in files:
        if f.suffix != ".h":
            continue
        rel = str(f.relative_to(REPO))
        t = strip(f.read_text(errors="replace"))
        for m in BARE_ENUM.finditer(t):
            if is_tag_type(m.group("body")):
                continue
            line = t[:m.start()].count("\n") + 1
            fatal.append(f"{rel}:{line}: bare `enum {m.group(1)}` in a header - declare it "
                         f"with GZ_ENUM_BEGIN/END so the strict build sees a real domain")

    # (4) implicit enumerator values
    for f in files:
        rel = str(f.relative_to(REPO))
        t = strip(f.read_text(errors="replace"))
        for name, kind, _st, body in domain_blocks(t):
            for em in ENUMERATOR.finditer(body):
                if em.group(2) is None and em.group(1).isupper():
                    warn.append(f"{rel}: {name}::{em.group(1)} has no explicit value")

    # (5) review manifest
    if not REVIEW.exists():
        fatal.append(f"{REVIEW.relative_to(REPO)} is missing - run --sync-review")
    else:
        rows = {}
        for i, ln in enumerate(REVIEW.read_text().splitlines()[1:], start=2):
            if not ln.strip():
                continue
            parts = ln.split("\t")
            if len(parts) < 2 or parts[1] not in STATES:
                fatal.append(f"enum-review.tsv:{i}: bad row (state must be one of {sorted(STATES)})")
                continue
            rows[parts[0]] = parts[1]
        have = {str(f.relative_to(REPO)) for f in files}
        missing = have - set(rows)
        stale = set(rows) - have
        if missing:
            fatal.append(f"enum-review.tsv: {len(missing)} source file(s) not listed "
                         f"(e.g. {sorted(missing)[:3]}) - run --sync-review")
        if stale:
            fatal.append(f"enum-review.tsv: {len(stale)} row(s) name files that no longer exist "
                         f"(e.g. {sorted(stale)[:3]}) - run --sync-review")

    return fatal, warn, declared


def sync_review():
    have = [str(f.relative_to(REPO)) for f in sources()]
    old = {}
    if REVIEW.exists():
        for ln in REVIEW.read_text().splitlines()[1:]:
            if ln.strip():
                p = ln.split("\t")
                if len(p) >= 2:
                    old[p[0]] = p[1:]
    lines = ["path\tstatus\tnotes"]
    for p in have:
        prev = old.get(p, ["pending", ""])
        prev = (prev + [""])[:2]
        lines.append(f"{p}\t{prev[0]}\t{prev[1]}")
    REVIEW.parent.mkdir(parents=True, exist_ok=True)
    REVIEW.write_text("\n".join(lines) + "\n")
    print(f"[enum-domains] enum-review.tsv synced: {len(have)} row(s)")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gate", action="store_true", help="exit 1 on any fatal finding")
    ap.add_argument("--sync-review", action="store_true",
                    help="rewrite config/enum-review.tsv from the current file list")
    ap.add_argument("-v", "--verbose", action="store_true", help="also print the warnings")
    args = ap.parse_args()

    if args.sync_review:
        sync_review()
        return 0

    fatal, warn, declared = audit()
    for f in fatal:
        print(f"   {f}")
    if args.verbose:
        for w in warn[:40]:
            print(f"   (warn) {w}")
        if len(warn) > 40:
            print(f"   (warn) ... and {len(warn) - 40} more")

    if fatal:
        print(f"[enum-domains] FATAL: {len(fatal)} defect(s); {len(warn)} implicit-value warning(s)")
        return 1 if args.gate else 0
    print(f"[enum-domains] OK - {len(declared)} domain(s), split widths agree, "
          f"no bare header enums ({len(warn)} implicit-value warning(s))")
    return 0


if __name__ == "__main__":
    sys.exit(main())
