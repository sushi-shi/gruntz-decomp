#!/usr/bin/env python3
"""wall_reasons.py - bucket every `@early-stop` marker by its STATED CAUSE, and say
whether that cause is still believable.

WHY THIS EXISTS.

`rg '@early-stop' src` is the documented final-sweep worklist and `stale_markers` already
answers "is this function still below 100?". Neither answers the question a matcher
actually has when it opens the list: *is the reason written here worth believing?*

That matters because the reasons are not uniform in quality. Some name a byte difference
you can go and look at. Some report an A/B with numbers. And some assert a compiler
behaviour with nothing behind it - and those have been falsified over and over:

  * "cl cross-jumped the two drains"      -> both drains were ALWAYS emitted; the body was
                                             wrong in five places.  0.00 -> 85.14
  * "no source lever for it yet"          -> 100.00 EXACT
  * "a three-register regalloc permutation" -> branch POLARITY in ten switch arms; the
                                             register the note complained about landed on
                                             retail's by itself once polarity was fixed.
  * "accept the rand-modulo gap"          -> a lost `GetRandom(lo,hi)` helper
  * "retail duplicates small return epilogues" -> `break;`-per-arm closed it outright

The common shape is a note that names a MECHANISM but no OBSERVATION. A mechanism is a
hypothesis; only the observation is evidence. So this tool splits the corpus on exactly
that line and lets a lane spend its budget on the half that was never measured.

THE TWO AXES.

  BUCKET   the mechanism the note blames (UNSTATED when there is no note at all)
  VERDICT  CORROBORATED  the note carries an OBSERVATION - a named byte/register/address
                         difference, a measured A/B with numbers, an exhausted search, or
                         a documented artefact family (delinker dup-symbol, pooled-literal
                         naming, __except_list typing)
           SUSPECT       the note names a mechanism the last two campaign days refuted,
                         with no observation attached
           UNSTATED      no reason prose at all - the marker asserts a wall and cites
                         nothing.  Re-derive from the disassembly before believing it.

SUSPECT MECHANISMS, and why each is on the list:

  CROSSJUMP     cl's cross-jumper compares ENCODED BYTES and runs BEFORE the register
                peephole, so "cl merged these" is only meaningful with the byte difference
                named. Without it the note has not shown the blocks were ever identical.
  REGALLOC      the symptom three separate times.  Check the FRAME SIZE first: a frame-size
                delta is never noise, and a hand-carried loop index can leave cl one
                register RICHER than retail (66.97 -> 100.00 after five spellings had
                measured byte-identical).
  NO-LEVER      the strongest claim in the corpus and the most often wrong.
  INLINE-BUDGET run the xref on the callee ONE LEVEL UP and check whether retail splits it
                by CLASS. That dissolved a wall and took two functions to EXACT.
  INLINE-DEPTH  cl 5.0 IGNORES `#pragma inline_depth`.  Measured twice.

    python -m gruntz.audit.wall_reasons                  # histogram + the re-open worklist
    python -m gruntz.audit.wall_reasons --summary        # histogram only
    python -m gruntz.audit.wall_reasons --bucket REGALLOC
    python -m gruntz.audit.wall_reasons --verdict SUSPECT
    python -m gruntz.audit.wall_reasons --unit fader     # one objdiff unit
    python -m gruntz.audit.wall_reasons --units u1,u2    # the lane's own units
    python -m gruntz.audit.wall_reasons --tsv out.tsv    # the full ledger
"""
import argparse
import collections
import csv
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
ROOTS = ("src", "include")
MARKER = re.compile(r"^\s*//\s*@early-stop\b")
RVA = re.compile(r"\bRVA\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+)")
RVA_COMPGEN = re.compile(r"\bRVA_COMPGEN\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+)")

# ---------------------------------------------------------------- evidence detectors
# An OBSERVATION is something a reader can go and re-check against the binary. These are
# the four forms the corpus actually uses.
AB_NUMBERS = re.compile(r"\d{1,3}\.\d{1,2}\s*(?:->|-->|→)\s*\d{1,3}\.\d{1,2}")
REGISTERS = re.compile(r"\b(?:e[abcd]x|e[sd]i|e[bs]p|[abcd][lhx]|st\(\d\))\b")
RETAIL_ADDR = re.compile(r"\b0x[0-9a-fA-F]{4,6}\b")
PATTERN_DOC = re.compile(r"docs/patterns/[a-z0-9\-]+\.md")
EXHAUSTED = re.compile(
    r"\b(?:\d+\s+(?:AST\s+)?variants?|exhausted|measured|byte-identical|byte-neutral"
    r"|re-measured|spellings? (?:measured|tested)|state-trials)\b", re.I)
# A documented artefact family: the note is not claiming a wall at all, it is naming a
# known SCORING behaviour. Those were independently corroborated by the sieves.
ARTEFACT = re.compile(
    r"\b(?:scoring artifact|dup-symbol|delinker|pooled|__except_list|reloc typing"
    r"|naming noise|undercount)\b", re.I)

# ---------------------------------------------------------------- bucket detectors
# Ordered: the first match wins, so the more specific mechanism is the primary label.
BUCKETS = [
    ("SCORING-ARTIFACT", re.compile(r"scoring artifact|dup-symbol|undercount|naming noise", re.I)),
    ("INLINE-DEPTH", re.compile(r"inline_depth", re.I)),
    ("INLINE-BUDGET", re.compile(r"/Ob1|inline[- ]budget|inline-cut|inline cut|inliner", re.I)),
    ("CROSSJUMP", re.compile(r"cross[- ]?jump|tail[- ]merg|cl merge[sd]?|merges (?:the|its|identical|ours)|collapses", re.I)),
    ("FRAME-SLOT", re.compile(r"\bframe\b|sub esp|\bspill|stack slot|frame slot|slot map|4[- ]byte frame|home slot|parameter home", re.I)),
    ("REGALLOC", re.compile(r"regalloc|register alloc|register[- ]colo|colour|coloring|colouring"
                            r"|register rotation|scratch register|register renaming|transpos"
                            r"|register (?:name|choice|assignment|swap)|\bswapped\b|rotation", re.I)),
    ("SCHEDULING", re.compile(r"schedul|hoist|sink[s]?\b|sunk|store order|operand order|statement order|interleav|defers", re.I)),
    ("BLOCK-LAYOUT", re.compile(r"block layout|block LAYOUT|layout|topology|lays (?:it|the|out)|falls through|fallthrough", re.I)),
    ("NO-LEVER", re.compile(r"no source lever|irreducible|not reachable from source|no spelling"
                            r"|no (?:cl|declaration) .{0,20}(?:moves|expresses)|\bwall\b|cannot name|we cannot", re.I)),
]

# A SUSPECT bucket is one the last two campaign days refuted as a stated cause.
SUSPECT_BUCKETS = {"CROSSJUMP", "REGALLOC", "NO-LEVER", "INLINE-BUDGET", "INLINE-DEPTH"}


def symbol_by_rva():
    path = REPO / "build/gen/symbol_names.csv"
    if not path.is_file():
        raise SystemExit("[wall-reasons] %s missing - run `gruntz build`" % path)
    out = {}
    with path.open() as fh:
        for row in csv.DictReader(fh):
            addr = row.get("rva") or row.get("address") or row.get("addr")
            name = row.get("symbol") or row.get("name")
            if addr and name:
                try:
                    out[int(addr, 16)] = name
                except ValueError:
                    pass
    return out


def scores():
    """mangled name -> (unit, fuzzy%). report.json omits the field when it is 0.0."""
    path = REPO / "build/objdiff/report.json"
    if not path.is_file():
        raise SystemExit("[wall-reasons] %s missing - run `gruntz build`" % path)
    rep = json.loads(path.read_text())
    out = {}
    for unit in rep["units"]:
        for fn in unit.get("functions", []):
            out[fn["name"]] = (unit.get("name"), fn.get("fuzzy_match_percent", 0.0))
    return out


def classify(reason):
    """(bucket, verdict, evidence-tags) for one reason string."""
    if not reason:
        return "UNSTATED", "UNSTATED", ()
    ev = []
    if AB_NUMBERS.search(reason):
        ev.append("ab")
    if REGISTERS.search(reason):
        ev.append("reg")
    if RETAIL_ADDR.search(reason):
        ev.append("addr")
    if PATTERN_DOC.search(reason):
        ev.append("doc")
    if EXHAUSTED.search(reason):
        ev.append("srch")
    if ARTEFACT.search(reason):
        ev.append("artf")

    bucket = "OTHER"
    for name, rx in BUCKETS:
        if rx.search(reason):
            bucket = name
            break

    # An artefact-family note is a scoring statement, not a wall claim: always keep.
    if "artf" in ev:
        return bucket, "CORROBORATED", tuple(ev)
    # inline_depth is refuted outright - cl 5.0 ignores the pragma, measured twice.
    if bucket == "INLINE-DEPTH":
        return bucket, "SUSPECT", tuple(ev)
    if bucket in SUSPECT_BUCKETS:
        # A mechanism claim survives only on an OBSERVATION: a measured A/B, an exhausted
        # search, or a named byte-level difference (register + retail address).  A bare
        # register name is not enough for CROSSJUMP/NO-LEVER, which are claims ABOUT the
        # compiler rather than about the bytes.
        strong = ("ab" in ev) or ("srch" in ev)
        named_bytes = ("reg" in ev) and ("addr" in ev)
        if strong or named_bytes:
            return bucket, "CORROBORATED", tuple(ev)
        return bucket, "SUSPECT", tuple(ev)
    # The remaining buckets describe an OBSERVED difference rather than a mechanism, so a
    # single evidence tag carries them.
    return bucket, ("CORROBORATED" if ev else "SUSPECT"), tuple(ev)


def scan():
    syms, pct = symbol_by_rva(), scores()
    rows = []
    for root in ROOTS:
        for path in sorted((REPO / root).rglob("*")):
            if path.suffix not in (".cpp", ".h"):
                continue
            lines = path.read_text(errors="replace").split("\n")
            for i, line in enumerate(lines):
                if not MARKER.match(line):
                    continue
                # the contiguous `//` block between the marker and the next real line
                j, reason = i + 1, []
                while j < len(lines) and lines[j].lstrip().startswith("//"):
                    reason.append(lines[j].strip().lstrip("/").strip())
                    j += 1
                reason = " ".join(reason).strip()

                addr = size = None
                for k in range(i + 1, min(i + 40, len(lines))):
                    m = RVA.search(lines[k])
                    if m:
                        addr, size = int(m.group(1), 16), int(m.group(2), 16)
                        break
                if addr is None:
                    for k in range(i + 1, min(i + 4, len(lines))):
                        m = RVA_COMPGEN.search(lines[k])
                        if m:
                            addr, size = int(m.group(1), 16), int(m.group(2), 16)
                            break
                name = syms.get(addr) if addr is not None else None
                unit, cur = pct.get(name, (None, None))
                bucket, verdict, ev = classify(reason)
                rows.append(dict(
                    file=str(path.relative_to(REPO)), line=i + 1, rva=addr,
                    size=size or 0, name=name or "", unit=unit or "", pct=cur,
                    bucket=bucket, verdict=verdict, evidence="+".join(ev),
                    reason=reason))
    return rows


def rank(r):
    """(suspect bucket) x (function size) x (distance from 100) - the re-open order."""
    if r["pct"] is None or r["verdict"] == "CORROBORATED":
        return 0.0
    gap = 100.0 - r["pct"]
    weight = 1.0 if r["verdict"] == "SUSPECT" else 0.6   # UNSTATED cites nothing but
    return weight * r["size"] * gap                      # also blames nothing


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--summary", action="store_true")
    ap.add_argument("--bucket")
    ap.add_argument("--verdict")
    ap.add_argument("--unit")
    ap.add_argument("--units", help="comma-separated objdiff unit names")
    ap.add_argument("--top", type=int, default=40)
    ap.add_argument("--tsv")
    a = ap.parse_args()

    rows = scan()
    print("early-stop markers: %d  (%d with a reason, %d bare)"
          % (len(rows), sum(1 for r in rows if r["reason"]),
             sum(1 for r in rows if not r["reason"])))

    hist = collections.Counter((r["bucket"], r["verdict"]) for r in rows)
    buckets = collections.Counter(r["bucket"] for r in rows)
    print("\n%-18s %6s %6s %6s %6s" % ("BUCKET", "total", "SUSP", "CORR", "UNST"))
    for b, n in buckets.most_common():
        print("%-18s %6d %6d %6d %6d" % (b, n, hist[(b, "SUSPECT")],
                                         hist[(b, "CORROBORATED")], hist[(b, "UNSTATED")]))
    v = collections.Counter(r["verdict"] for r in rows)
    print("\nVERDICT   SUSPECT %d   CORROBORATED %d   UNSTATED %d"
          % (v["SUSPECT"], v["CORROBORATED"], v["UNSTATED"]))
    print("re-openable (SUSPECT + UNSTATED) = %d of %d"
          % (v["SUSPECT"] + v["UNSTATED"], len(rows)))

    if a.tsv:
        with open(a.tsv, "w", newline="") as fh:
            w = csv.DictWriter(fh, fieldnames=list(rows[0].keys()), delimiter="\t")
            w.writeheader()
            for r in rows:
                w.writerow(r)
        print("wrote %s" % a.tsv)

    if a.summary:
        return 0

    sel = rows
    if a.bucket:
        sel = [r for r in sel if r["bucket"] == a.bucket.upper()]
    if a.verdict:
        sel = [r for r in sel if r["verdict"] == a.verdict.upper()]
    if a.unit:
        sel = [r for r in sel if r["unit"] == a.unit]
    if a.units:
        want = {u.strip() for u in a.units.split(",") if u.strip()}
        sel = [r for r in sel if r["unit"] in want]
    sel = [r for r in sel if rank(r) > 0]
    sel.sort(key=rank, reverse=True)

    print("\nRE-OPEN worklist (rank = weight x size x gap), top %d of %d:" % (a.top, len(sel)))
    print("%-9s %-6s %-16s %-13s %-8s %s"
          % ("rva", "size", "unit", "bucket", "pct", "file:line"))
    for r in sel[:a.top]:
        print("%-9s %-6s %-16s %-13s %-8s %s:%d"
              % ("%06x" % r["rva"] if r["rva"] else "-", "0x%x" % r["size"],
                 r["unit"][:16], r["bucket"][:13],
                 "%.2f" % r["pct"] if r["pct"] is not None else "-",
                 r["file"], r["line"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
