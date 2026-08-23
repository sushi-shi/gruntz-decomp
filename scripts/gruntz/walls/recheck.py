"""gruntz.walls recheck - re-run a review's certifications against today's pair.

A review that says "base/retail agree on 24 calls, 96 branches and 64 relocs" is
not prose: it is a checkable assertion about the normalized pair.  Nothing
re-ran it.  `CTriggerMgr::PlaceObjectFull` carried exactly that certification
while the base had drifted to 25/95/65 - a later commit traded one cross-jump
for another, the score ROSE, and no source reader could see it because the
review's own numbers were never measured again.

This re-measures every count a review asserts and prints HOLD or BROKEN per
claim.  It reads the same normalized base/target objs `gruntz walls diagnose`
reads, so a BROKEN row is a live divergence, not a stale cache.

The recorded wall CLASS is re-run too, through `diagnose.ladder` itself so the
sweep and the single-row view cannot answer differently.  It is scored
SEPARATELY from the counts and a disagreement is not a failure: the review names
the CAUSE it traced, the ladder names the FIRST divergence, and on this ledger
that vocabulary gap accounts for most disagreements (a regalloc cause whose
branch delta is downstream reads `cfg` here).  A STALE review whose class AND
counts both still hold is a verdict the edit did not invalidate.

    gruntz walls recheck                 every review
    gruntz walls recheck <rva|name>...   selected rows
    gruntz walls recheck --broken        only rows with a failed count claim
    gruntz walls recheck --strict        exit 1 when any count claim is broken

`gate_findings` is the same sweep as the `review-claims` row of `gruntz verify
check --tier normal`, so every build re-measures the ledger.  It belongs in a
tier because it is the drift the MAX gate cannot see: the MAX gate watches the
SCORE, and the commit that broke `PlaceObjectFull`'s certification raised it.

Only AGREEMENT claims are extracted, and deliberately so.  "base 59/retail 43
calls" states a known divergence whose direction depends on the writer's word
order; asserting it back would test the parser, not the tree.  An agreement
claim has one reading: both sides hold N.  Two spellings carry it -

    N/N <unit>                     "2/2 calls", "4/4 DispatchMove calls"
    <agreement trigger> ... N <unit>   "Base/retail agree on 18 calls, 1 return"

- and a sentence that also carries a divergence marker (vs, versus, against,
  differ, while, whereas, short, only) is skipped whole, because those sentences
  mix both kinds and the parser cannot tell which number belongs to which.

`referents` is NOT checked: a review's "ordered referents" is the semdiff
referent SEQUENCE, which is shorter than the relocation count (PlaceObjectFull:
43 referents against 65 relocs), so scoring it against `relocs` would invent a
failure.  Unmatched quantities are reported as such rather than guessed.
"""

from __future__ import annotations

from collections import Counter
import re

from gruntz.delink.coffx import Obj

# The quantities this tool can measure from the normalized pair.  `returns` and
# `rets` are one quantity; `instructions` and `insns` are one quantity.
_UNIT = {
    "call": "calls", "calls": "calls",
    "branch": "branches", "branches": "branches",
    "return": "returns", "returns": "returns", "ret": "returns", "rets": "returns",
    "reloc": "relocs", "relocs": "relocs",
    "relocation": "relocs", "relocations": "relocs",
    "instruction": "insns", "instructions": "insns",
    "insn": "insns", "insns": "insns",
}
_UNIT_RE = "|".join(sorted(_UNIT, key=len, reverse=True))

# "4/4 DispatchMove calls" - one qualifier may sit between the pair and the unit.
# It must not be a connective: `returns 2/2 and relocs 249/249` otherwise reads
# as "relocs 2", which is how the first pass invented two failures out of two
# holds (0x065e80, 0x0f42f0).  Both word orders occur - `22/22 calls` and
# `Calls 31/31` - and the reverse spelling is where `ordered relocs 312/312`
# lives, so both are matched.
_STOP = frozenset(
    "and or the a an plus with of in at on to versus vs but then also each both "
    "all only now still exact exactly are is was were has have had its their "
    "than from for".split()
)
_PAIR = re.compile(
    rf"(?<![\d.])(?P<a>\d+)\s*/\s*(?P<b>\d+)\s+(?P<qual>[A-Za-z_][\w:]*\s+)?"
    rf"(?P<unit>{_UNIT_RE})\b",
    re.I,
)
_PAIR_REV = re.compile(
    rf"\b(?P<unit>{_UNIT_RE})\s+(?P<a>\d+)\s*/\s*(?P<b>\d+)(?!\d)(?!\.\d)", re.I
)
_COUNT = re.compile(rf"(?<![\d./x])(\d+)\s+(?:ordered\s+)?({_UNIT_RE})\b", re.I)
_EXTENT = re.compile(r"(?<![\w.])(?:0x([0-9a-f]+))\s*(?:bytes?|B\b|extent)", re.I)

_AGREE = re.compile(
    r"\b(agree|agrees|agreed|identical|exact|exactly|both|same|match|matches)\b", re.I
)
_DIVERGE = re.compile(
    r"\b(vs|versus|against|differ|differs|differing|while|whereas|short|only|"
    r"instead|prior|banked|historical|recorded|fell|falls|rose|raising|"
    r"delta|deltas|because|though|although|but)\b",
    re.I,
)
# A clause measuring two SOURCE CANDIDATES against each other is not a claim
# about base against retail, and its "both" means the two spellings.
# `InitFromSurface` reads "...both compile byte-identically at 77.50 with 30
# instructions, 3 branches and 2 returns" while that same review states retail
# has 3 returns to the base's 2; `DrawWrapped` states 59/59 calls measured under
# a DISPOSABLE inline_depth(0) probe that the tree does not carry.  Neither is a
# certification of the committed pair, so the clause is skipped whole.
_CANDIDATE = re.compile(
    r"\b(compiles?|compiled|byte-identical(?:ly)?|byte-flat|variants?|controls?|"
    r"campaigns?|islands?|trials?|disposable|probes?|experiments?|candidates?|"
    r"rejected|tested)\b",
    re.I,
)


def _sentences(text: str) -> list[str]:
    """Split into clauses on `. ` and `; `, never inside a hex literal or a
    decimal score.  The semicolon matters: the certification that drifted was
    written `...agree at 24 calls, 96 branches, and 64 relocs; base has 15 vs
    retail 16 returns because...` - one sentence carrying both an agreement and
    a divergence.  Splitting only on `.` reads the `vs` and discards the whole
    assertion, which is how the known positive escaped the first pass."""
    out, buf = [], []
    for i, ch in enumerate(text):
        buf.append(ch)
        if ch in ".;" and (i + 1 >= len(text) or text[i + 1] in " \t"):
            out.append("".join(buf))
            buf = []
    if buf:
        out.append("".join(buf))
    return out


def claims(evidence: str) -> tuple[list[tuple[str, int, str]], list[str]]:
    """([(unit, n, sentence)], [skipped sentence]) - the agreement assertions.

    A claim means BOTH sides were certified at `n`.  Sentences that mix an
    agreement with a divergence are skipped and returned so the caller can say
    what it declined to read."""
    found: list[tuple[str, int, str]] = []
    skipped: list[str] = []
    for s in _sentences(evidence):
        if _CANDIDATE.search(s):
            if _COUNT.search(s) or _PAIR.search(s) or _PAIR_REV.search(s):
                skipped.append(s.strip())
            continue
        pairs = []
        for m in _PAIR.finditer(s):
            qual = (m.group("qual") or "").strip().lower()
            if qual in _STOP:
                continue
            pairs.append((m, _UNIT[m.group("unit").lower()]))
        pairs += [(m, _UNIT[m.group("unit").lower()]) for m in _PAIR_REV.finditer(s)]
        equal = [(u, int(m.group("a")), s)
                 for m, u in pairs if m.group("a") == m.group("b")]
        found.extend(equal)
        if not _AGREE.search(s):
            continue
        if _DIVERGE.search(s):
            if not equal and _COUNT.search(s):
                skipped.append(s.strip())
            continue
        spans = [m.span() for m, _u in pairs]
        for m in _COUNT.finditer(s):
            if any(a <= m.start() < b for a, b in spans):
                continue  # already read as the N/N form
            found.append((_UNIT[m.group(2).lower()], int(m.group(1)), s))
        for m in _EXTENT.finditer(s):
            found.append(("bytes", int(m.group(1), 16), s))
    # de-duplicate: one sentence may state the same quantity twice
    seen, uniq = set(), []
    for unit, n, s in found:
        if (unit, n) in seen:
            continue
        seen.add((unit, n))
        uniq.append((unit, n, s))
    return uniq, skipped


def measure(binding) -> tuple[dict[str, tuple[int, int]], str] | str:
    """({unit: (base, target)}, wall class) from the pair, or an error string.

    The wall class comes from `diagnose.ladder`, the same function that prints,
    so the sweep and the single-row view can never answer differently."""
    from collections import Counter

    from gruntz.tool import ToolError
    from gruntz.walls.diagnose import (
        NORM, _call_targets, _find_function, _jump_table_bytes, _referents,
        _skeleton, ladder,
    )

    base_p = NORM / "base" / f"{binding.unit}.obj"
    tgt_p = next(
        (p for p in (NORM / "target" / f"{binding.unit}.c.obj",
                     NORM / "target" / f"{binding.unit}.obj") if p.is_file()),
        None,
    )
    if not base_p.is_file() or tgt_p is None:
        return f"normalized pair missing for {binding.unit}"
    side, extra = {}, {}
    for tag, path in (("base", base_p), ("target", tgt_p)):
        payload, rel, size = _find_function(Obj(path), binding.name)
        if payload is None:
            return f"{tag} obj does not define {binding.name}"
        try:
            mask, calls, br, rets, insns, asm = _skeleton(
                payload, rel, data=_jump_table_bytes(rel, binding.name)
            )
        except ToolError as e:
            return str(e)
        side[tag] = {"calls": calls, "branches": br, "returns": rets,
                     "insns": insns, "relocs": len(rel), "bytes": size}
        extra[tag] = (mask, _referents(rel),
                      Counter(n for n, _a in _call_targets(rel, asm, binding.name)))
    wall = ladder(extra["base"][0], extra["target"][0],
                  extra["base"][1], extra["target"][1],
                  extra["base"][2], extra["target"][2],
                  (side["base"]["branches"], side["base"]["returns"],
                   side["target"]["branches"], side["target"]["returns"]))
    return {u: (side["base"][u], side["target"][u]) for u in side["base"]}, wall


def sweep(wanted: set[int] | None = None) -> list[dict]:
    """One record per review row: the re-measured verdicts, in rva order.

    The printer and the build gate both consume this, so the two can never
    answer differently - the same reason `measure` calls `diagnose.ladder`
    instead of reimplementing the class ladder."""
    from gruntz.walls import reviews
    from gruntz.walls.diagnose import _locate, named_functions

    rows = reviews.load()
    fresh = set(reviews.current())
    named = named_functions()
    out: list[dict] = []
    for rva in sorted(rows):
        if wanted and rva not in wanted:
            continue
        row = rows[rva]
        stated, skipped = claims(row["evidence"])
        rec = {
            "rva": rva, "row": row, "stated": stated, "skipped": skipped,
            "fresh": rva in fresh, "binding": None, "verdicts": [],
            "wall": None, "error": None,
        }
        b, why = _locate(f"0x{rva:x}", named)
        if b is None:
            rec["error"] = ("unresolved", why)
            out.append(rec)
            continue
        rec["binding"] = b
        got = measure(b)
        if isinstance(got, str):
            rec["error"] = ("unmeasured", got)
            out.append(rec)
            continue
        counts, wall = got
        rec["wall"] = wall
        rec["verdicts"] = [(counts[u][0] == n == counts[u][1], u, n, *counts[u])
                           for u, n, _s in stated]
        out.append(rec)
    return out


def gate_findings() -> list[str]:
    """Every count a review certifies, re-measured against today's pair.

    This is the drift the MAX gate structurally CANNOT see.  The MAX gate
    watches the SCORE, so a commit that trades one cross-jump for another -
    `PlaceObjectFull` gaining the 16th `ret` retail has while losing a merge
    elsewhere, calls +1 / branches -1 / relocs +1 - passes it with the score
    going UP, and no source reader can see it either because the source hash
    did not move.  A review's "base and retail agree on 24 calls, 96 branches
    and 64 relocs" is the only record of that shape, and until it is
    re-measured it is prose."""
    out: list[str] = []
    for rec in sweep():
        rva = rec["rva"]
        if rec["error"]:
            why, detail = rec["error"]
            if why == "unresolved":
                out.append(f"0x{rva:06x}: review row names no claimed function "
                           f"({detail})")
            elif rec["stated"]:
                # A row whose pair cannot be read is not a row that passed.
                out.append(f"0x{rva:06x}: {len(rec['stated'])} certified "
                           f"count(s) unmeasurable - {detail}")
            continue
        name = rec["binding"].name
        for ok, unit, n, base, target in rec["verdicts"]:
            if ok:
                continue
            fmt = (lambda v: f"{v:#x}") if unit == "bytes" else str
            out.append(f"0x{rva:06x} {name}: review certifies both sides at "
                       f"{fmt(n)} {unit}, now base {fmt(base)} / target "
                       f"{fmt(target)} - re-review or re-certify "
                       f"(`gruntz walls diagnose 0x{rva:x}`)")
    return out


def main(argv=None) -> int:
    import argparse

    from gruntz.walls.diagnose import _locate

    ap = argparse.ArgumentParser(
        prog="gruntz walls recheck",
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("target", nargs="*", help="hex rva, mangled name, or CClass::Member")
    ap.add_argument("--broken", action="store_true", help="only rows with a failed claim")
    ap.add_argument("--strict", action="store_true", help="exit 1 when any claim is broken")
    ap.add_argument("--skipped", action="store_true",
                    help="also print the mixed sentences the parser declined to read")
    a = ap.parse_args(argv)

    wanted = set()
    for token in a.target:
        b, why = _locate(token)
        if b is None:
            print(f"[recheck] {why}")
            return 2
        wanted.add(b.rva)

    n_rows = n_claims = n_hold = n_broken = 0
    unparsed: list[int] = []
    broken_rows: list[int] = []
    n_class = Counter()
    for rec in sweep(wanted):
        rva, row = rec["rva"], rec["row"]
        if rec["error"]:
            why, detail = rec["error"]
            if why == "unresolved":
                print(f"  0x{rva:06x} UNRESOLVED {detail}")
            elif rec["stated"]:
                print(f"  0x{rva:06x} UNMEASURED {detail}")
            if rec["stated"]:
                broken_rows.append(rva)
            continue
        # The CLASS is an assertion too, and it is the one a matcher acts on.
        # It is scored separately: the review names the CAUSE it traced, this
        # names the FIRST divergence, so a disagreement is a lead, not a defect.
        same_class = rec["wall"] == row["wall_class"]
        n_class["same" if same_class else "differs"] += 1
        verdicts = rec["verdicts"]
        n_claims += len(verdicts)
        n_hold += sum(1 for ok, *_ in verdicts if ok)
        n_broken += sum(1 for ok, *_ in verdicts if not ok)
        n_rows += bool(rec["stated"])
        if not rec["stated"]:
            unparsed.append(rva)
        row_broken = any(not ok for ok, *_ in verdicts)
        if row_broken:
            broken_rows.append(rva)
        if a.broken and not row_broken:
            continue
        freshness = "current" if rec["fresh"] else "STALE"
        print(f"0x{rva:06x} {freshness:7} {row['status']:8} {row['wall_class']:8} "
              f"{rec['binding'].name}  [{rec['binding'].unit}]")
        print(f"    {'HOLD  ' if same_class else 'DIFFERS'} class"
              + ("" if same_class else
                 f"     review says {row['wall_class']}, first divergence is "
                 f"{rec['wall']}"))
        for ok, unit, n, base, target in verdicts:
            tag = "HOLD  " if ok else "BROKEN"
            fmt = (lambda v: f"{v:#x}") if unit == "bytes" else str
            print(f"    {tag} {unit:9} certified both {fmt(n):>7}   "
                  f"now base {fmt(base):>7}  target {fmt(target):>7}")
        if a.skipped:
            for s in rec["skipped"]:
                print(f"    (skipped, mixed sentence) {s}")

    print(f"\n[recheck] {n_rows} review(s) state counts: {n_claims} claim(s), "
          f"{n_hold} hold, {n_broken} broken across {len(broken_rows)} row(s)")
    print(f"[recheck] class: {n_class['same']} agree with today's first "
          f"divergence, {n_class['differs']} differ (a differing class is a "
          f"vocabulary gap as often as a stale verdict - the review names the "
          f"CAUSE, this names the FIRST divergence)")
    if unparsed:
        print(f"[recheck] {len(unparsed)} review(s) state no measurable count: "
              + " ".join(f"0x{r:06x}" for r in unparsed))
    if broken_rows:
        print("[recheck] re-review: "
              + " ".join(f"0x{r:06x}" for r in broken_rows))
    return 1 if (a.strict and broken_rows) else 0


if __name__ == "__main__":
    raise SystemExit(main())
