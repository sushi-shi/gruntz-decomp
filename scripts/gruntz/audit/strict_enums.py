#!/usr/bin/env python3
"""gruntz.audit.strict_enums - the STRICT C++20 view of the enum-domain layer.

The matching build is MSVC 5.0, which has no scoped enums: every domain is a
plain 4-byte `enum` there, so it catches a wrong-domain argument and a raw-int
assignment but NOT a domain used as an array index, a domain silently widened to
`i32` through a parameter, or two domains conflated behind one `i32`.

This runs clang over the SAME sources at `/std:c++20`, where `include/Enums.h`
takes its strict branch (`enum class N : i32`, `GzEnumStorage<N,S>` for narrow
fields) and those defects become hard errors. It is a TYPE ORACLE ONLY - it never
produces an object, and the Wine MSVC 5.0 build remains the sole verdict on a
match (docs/enum-modeling-plan.md).

Reads build/clangd/compile_commands.json for each unit's real flags, so run
`gruntz clangd` first if it is stale.

    python -m gruntz.audit.strict_enums              # report, grouped by defect
    python -m gruntz.audit.strict_enums --gate       # exit 1 if any error
    python -m gruntz.audit.strict_enums --files src/Gruntz/Grunt.cpp
    python -m gruntz.audit.strict_enums --baseline   # write the accepted floor
"""
from __future__ import annotations

import argparse
import json
import re
import shlex
import subprocess
import sys
from collections import Counter
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
DB = REPO / "build" / "clangd" / "compile_commands.json"
BASELINE = REPO / "config" / "strict-enums-baseline.tsv"

ERR = re.compile(r"^(?P<path>[^:()]+)[(](?P<line>\d+),(?P<col>\d+)[)]\s*:\s*error:\s*(?P<msg>.*)$")

# The defect classes this view exists to find (docs/enum-modeling-plan.md).
CLASSES = (
    ("domain-to-int",     re.compile(r"is not implicitly convertible to|cannot initialize .* with an rvalue of type")),
    ("domain-as-index",   re.compile(r"array subscript is not an integer")),
    ("wrong-domain",      re.compile(r"no matching function|cannot convert .* to")),
    ("int-into-domain",   re.compile(r"assigning to .* from incompatible type")),
)


def classify(msg: str) -> str:
    for name, rx in CLASSES:
        if rx.search(msg):
            return name
    return "other"


def units(only: list[str] | None):
    if not DB.exists():
        print(f"[strict-enums] {DB.relative_to(REPO)} missing - run `gruntz clangd` first",
              file=sys.stderr)
        sys.exit(2)
    for e in json.load(open(DB)):
        f = e["file"]
        if only and not any(f.endswith(o) or o in f for o in only):
            continue
        args = e.get("arguments") or shlex.split(e["command"])
        yield f, args, e.get("directory", str(REPO))


def check(unit) -> tuple[str, list[tuple[str, int, str, str]]]:
    f, args, cwd = unit
    cmd = [a for a in args if not a.startswith("/std:")] + [
        "/std:c++20", "-fsyntax-only", "-ferror-limit=0"]
    r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    out = []
    for ln in r.stderr.splitlines():
        m = ERR.match(ln.strip())
        if not m:
            continue
        p = m.group("path").strip()
        try:
            p = str(Path(p).resolve().relative_to(REPO))
        except ValueError:
            pass
        out.append((p, int(m.group("line")), m.group("msg").strip(), classify(m.group("msg"))))
    return f, out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gate", action="store_true", help="exit 1 if the error count exceeds the baseline")
    ap.add_argument("--baseline", action="store_true", help="write the accepted floor and exit")
    ap.add_argument("--files", nargs="*", help="limit to these paths")
    ap.add_argument("-j", type=int, default=12)
    ap.add_argument("--top", type=int, default=30)
    ap.add_argument("--dump", metavar="TSV", help="write every finding as path\tline\tclass\tmsg")
    ap.add_argument("--roots", action="store_true",
                    help="report only the FIRST error of each unit - the root a cascade grows from")
    args = ap.parse_args()

    todo = list(units(args.files))
    if not todo:
        print("[strict-enums] no matching units"); return 0

    findings: dict[tuple[str, int, str], str] = {}
    roots: dict[tuple[str, int, str], int] = {}
    failed_units = 0
    with ThreadPoolExecutor(max_workers=args.j) as ex:
        futs = [ex.submit(check, u) for u in todo]
        for fut in as_completed(futs):
            _f, errs = fut.result()
            if errs:
                failed_units += 1
            if args.roots and errs:
                p, line, msg, cls = errs[0]
                roots[(p, line, msg)] = roots.get((p, line, msg), 0) + 1
            for p, line, msg, cls in errs:
                findings[(p, line, msg)] = cls   # dedupe: a header error repeats per TU

    by_class = Counter(findings.values())
    by_file = Counter(p for (p, _l, _m) in findings)

    print(f"[strict-enums] {len(todo)} unit(s), {failed_units} with errors, "
          f"{len(findings)} distinct defect(s)")
    for cls, n in by_class.most_common():
        print(f"    {n:5d}  {cls}")
    print("  worst files:")
    for p, n in by_file.most_common(args.top):
        print(f"    {n:5d}  {p}")

    if args.roots:
        print("  ROOTS (first error per unit; the count is how many units it heads):")
        for (p, line, msg), n in sorted(roots.items(), key=lambda kv: -kv[1])[:25]:
            print(f"    x{n:<4d} {p}:{line}: {msg[:110]}")

    if args.dump:
        Path(args.dump).write_text("path\tline\tclass\tmsg\n" + "".join(
            f"{p}\t{l}\t{c}\t{m}\n" for (p, l, m), c in sorted(findings.items())))
        print(f"[strict-enums] dumped {len(findings)} finding(s) -> {args.dump}")

    if args.baseline:
        BASELINE.write_text("metric\tcount\n" + "".join(
            f"{c}\t{n}\n" for c, n in sorted(by_class.items())))
        print(f"[strict-enums] baseline written: {BASELINE.relative_to(REPO)}")
        return 0

    if args.gate:
        floor = 0
        if BASELINE.exists():
            floor = sum(int(l.split("\t")[1]) for l in BASELINE.read_text().splitlines()[1:] if l.strip())
        if len(findings) > floor:
            print(f"[strict-enums] FATAL: {len(findings)} defect(s) > baseline {floor}")
            for (p, line, msg), cls in sorted(findings.items())[:40]:
                print(f"   {p}:{line}: [{cls}] {msg}")
            return 1
        print(f"[strict-enums] OK - {len(findings)} defect(s) <= baseline {floor}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
