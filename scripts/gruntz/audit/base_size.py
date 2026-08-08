#!/usr/bin/env python3
"""base_size.py - does the COMPILED body have the same length as its RVA(addr, size)?

`gruntz.audit.rva_size` checks the ANNOTATION against Ghidra's carve. Nothing checks our
compiled body against it - and a length mismatch is the loudest possible signal of a
structural reconstruction bug. It is also exactly what objdiff and `sema disasm --diff`
hide: both align instruction-by-instruction and report a couple of operand mismatches
while the two bodies differ by tens of bytes.

That is how CPlay::DrawDebugStats sat at 99.55% for weeks with a real bug in it. It read
the debug "Pos" from `m_viewRect.left/top` (+0x40/+0x44) where retail reads +0x84/+0x88
(the plane's snapped scroll origin). Both wrong offsets fit in a disp8 where the right
ones need disp32, so the compiled body was 812 bytes against an annotated 862 - a 50-byte
deficit that no per-instruction view mentions. See
docs/patterns/compensating-error-signatures.md.

    python -m gruntz.audit.base_size              # rank by fuzzy%, then |delta|
    python -m gruntz.audit.base_size --min-pct 99 # the compensating-error suspect list
    python -m gruntz.audit.base_size --all        # every mismatch, no fuzzy floor

NOT A GATE, and deliberately so - a nonzero delta has three innocent causes:
  * the annotation is longer than the real function (`rva_size` reports 46 such LONG rows);
  * the body is a COMDAT that the linker folded onto a shared tail, so one annotation
    covers two names (the two `OnDrawItem` rows, -1414 each);
  * a one-byte alignment tail inside the carve.
It is a SUSPECT GENERATOR. The row that matters is `fuzzy >= 99 and |delta| >= 4`: a body
that is byte-for-byte convincing yet the wrong LENGTH is a structural bug, not a codegen
preference.
"""
import argparse
import json
import re
import subprocess
from pathlib import Path

from gruntz.core.report import fn_fuzzy

REPO = Path(__file__).resolve().parents[3]
ROOTS = ("src", "include")
RVA_RE = re.compile(r"\bRVA(?:_COMPGEN)?\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+)")
SYM_CSV = REPO / "build/gen/symbol_names.csv"
REPORT = REPO / "build/objdiff/report.json"
BASE_DIR = REPO / "build/objdiff/base"


def annotated():
    """rva -> annotated size."""
    out = {}
    for root in ROOTS:
        for p in (REPO / root).rglob("*"):
            if p.suffix not in (".cpp", ".h"):
                continue
            for m in RVA_RE.finditer(p.read_text(errors="replace")):
                out[int(m.group(1), 16)] = int(m.group(2), 16)
    return out


def base_lengths(obj):
    """symbol -> compiled byte length, trailing 0x90/0xcc alignment padding trimmed."""
    r = subprocess.run(["llvm-objdump", "-d", "--section=.text", str(obj)],
                       capture_output=True, text=True)
    cur, start, last_end, res = None, None, 0, {}
    for line in r.stdout.splitlines():
        m = re.match(r"^[0-9a-f]+ <(.+)>:$", line)
        if m:
            # `$L…` / `.` labels are INTERNAL (a jump table, a shared tail) - they do not
            # end the function. Treating them as symbols truncated every jump-table body
            # (ButeValueTeardown read as 19 bytes of 124).
            if m.group(1).startswith(("$", ".")):
                continue
            if cur and start is not None:
                res[cur] = last_end - start
            cur, start, last_end = m.group(1), None, 0
            continue
        m = re.match(r"^\s*([0-9a-f]+):\s([0-9a-f]{2}(?: [0-9a-f]{2})*)\s", line)
        if m and cur:
            addr, by = int(m.group(1), 16), m.group(2).split()
            if start is None:
                start = addr
            if not all(b in ("90", "cc") for b in by):
                last_end = addr + len(by)
    if cur and start is not None:
        res[cur] = last_end - start
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--min-pct", type=float, default=90.0,
                    help="only rows whose fuzzy%% is at least this (default 90)")
    ap.add_argument("--all", action="store_true", help="no fuzzy floor")
    ap.add_argument("--min-delta", type=int, default=1, help="|compiled - annotated| floor")
    args = ap.parse_args()
    floor = -1.0 if args.all else args.min_pct

    ann, pct, units = annotated(), {}, {}
    if REPORT.exists():
        for u in json.load(open(REPORT))["units"]:
            for f in u.get("functions", []):
                # objdiff omits the key at exactly 0.0 (serde skip-default); -1.0 is
                # reserved for "not in the report at all". See gruntz.core.report.
                pct[f["name"]] = fn_fuzzy(f)
    for line in SYM_CSV.read_text().splitlines():
        f = line.split(",")
        if len(f) >= 5 and f[4] == "func":
            units.setdefault(f[2], []).append((f[1], int(f[0], 16)))

    rows = []
    for unit, lst in sorted(units.items()):
        obj = BASE_DIR / (unit + ".obj")
        if not obj.exists():
            continue
        lens = base_lengths(obj)
        for sym, rva in lst:
            if sym not in lens or rva not in ann:
                continue
            d = lens[sym] - ann[rva]
            if abs(d) >= args.min_delta:
                rows.append((pct.get(sym, -1.0), d, ann[rva], lens[sym], unit, sym))
    rows.sort(key=lambda r: (-r[0], -abs(r[1])))

    shown = [r for r in rows if r[0] >= floor]
    print(f"{len(rows)} function(s) whose compiled length != RVA() size "
          f"({len(shown)} at fuzzy >= {floor:g})\n")
    print(f'{"fuzzy":>7} {"delta":>6} {"ann":>6} {"base":>6}  unit / symbol')
    for p, d, a, b, u, s in shown:
        print(f"{p:7.2f} {d:+6d} {a:6d} {b:6d}  {u} / {s}")
    hot = [r for r in rows if r[0] >= 99.0 and abs(r[1]) >= 4]
    if hot:
        print(f"\n{len(hot)} SUSPECT(S) - fuzzy >= 99 with |delta| >= 4: a body that reads "
              f"byte-convincing but is the wrong LENGTH is a structural bug.")


if __name__ == "__main__":
    main()
