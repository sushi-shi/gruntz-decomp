#!/usr/bin/env python3
"""cast_reasons.py - order the parked casts for the next fold pass.

`cast_ledger` sorts every reinterpret_cast into FORCED or OPEN, and drives OPEN to
zero. But its FORCED test is lexical: a cast counts as explained the moment one of
the closed-vocabulary words (`byte-forced`, `PROVEN`, `faithful`, ...) appears within
three lines. That is a fine gate for "did anyone look at this", and no gate at all on
whether the reason is TRUE - a comment reading

    // byte-forced (see the note above)

closes a site while asserting nothing a reviewer can check.

So OPEN reaching 0 does not mean the campaign is finished; it means no site is
UNTOUCHED. This tool asks the follow-up: of the parked casts, which reasons carry
anything checkable against the binary?

  CITED  the window names a retail RVA, a relocation, a mangled symbol, an x86
         mnemonic or register, or a concrete layout fact (sizeof/vtable/slot). A
         reviewer can go to that address and agree or disagree.
  SOFT   the vocabulary word is there and nothing else is. Indistinguishable from
         an unexamined cast that someone labelled.

Neither bucket is a keep. Per the standing ruling, a reason buys a little confidence
and nothing more - essentially all of these are expected to fold on a real second
pass, and every reason retested so far has. CITED has not "passed" anything; it is
just better supported than SOFT.

So read this as TRIAGE ORDER for the fold campaign - SOFT first, because those claims
rest on nothing - not as a quality bar that closes sites. The number that has to reach
zero is the total cast count, which `reinterpret_casts` ratchets.

    python -m gruntz.audit.cast_reasons             # counts + the SOFT queue
    python -m gruntz.audit.cast_reasons --summary   # counts only
    python -m gruntz.audit.cast_reasons --max N     # exit 1 if SOFT exceeds N
"""
import argparse
import collections
import re
import sys

from gruntz.audit.cast_ledger import CAST, FORCED, REASON, REPO, ROOTS

# Something a reviewer can take to the disassembler and check.
CITE = re.compile(
    r"0x[0-9a-fA-F]{4,}"                       # a retail RVA / offset / immediate
    r"|\breloc\b"                              # a relocation claim
    r"|\?[A-Za-z_?@$][\w?@$]*@@"               # a mangled symbol
    r"|\b(?:mov|lea|push|pop|cmp|test|call|jmp|add|sub|shl|shr|xor|and|or|"
    r"j[a-z]{1,3})\b"                          # an instruction
    r"|\b(?:eax|ebx|ecx|edx|esi|edi|ebp|esp|ax|bx|cx|dx|al|ah|bl|bh|cl|ch|dl|dh)\b"
    r"|\bsizeof\b|\bvtable\b|\bslot\b|\bRVA\b|\bCOMDAT\b",
)


def scan():
    """-> (cited, {file: [(line, src)]}) over the prose-explained casts only."""
    cited = 0
    soft = collections.defaultdict(list)
    for root in ROOTS:
        for path in sorted((REPO / root).rglob("*")):
            if path.suffix not in (".cpp", ".h"):
                continue
            lines = path.read_text(errors="replace").split("\n")
            for i, line in enumerate(lines):
                for _ in CAST.finditer(line.split("//", 1)[0]):
                    ctx = " ".join(lines[max(0, i - 3):i + 2])
                    if any(re.search(p, line) or re.search(p, ctx) for _n, p in FORCED):
                        continue          # a STRUCTURAL seam, not a prose reason
                    if not REASON.search(ctx):
                        continue          # that is an OPEN; cast_ledger owns it
                    if CITE.search(ctx):
                        cited += 1
                    else:
                        soft[str(path.relative_to(REPO))].append(
                            (i + 1, line.strip()[:88]))
    return cited, soft


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--summary", action="store_true", help="counts only")
    ap.add_argument("--max", type=int, default=None,
                    help="exit 1 when SOFT exceeds N (ratchet)")
    a = ap.parse_args()

    cited, soft = scan()
    n_soft = sum(len(v) for v in soft.values())
    total = cited + n_soft
    pct = (100.0 * cited / total) if total else 100.0
    print("cast reasons: %d prose-explained  |  %d CITED (%.0f%%)  |  %d SOFT"
          % (total, cited, pct, n_soft))

    if not a.summary:
        print("\nSOFT - the reason names no address, instruction, reloc or symbol, so "
              "nothing in it can be checked:")
        listed = 0
        for f, rows in sorted(soft.items(), key=lambda kv: (-len(kv[1]), kv[0])):
            print("   %3d  %s" % (len(rows), f))
            for ln, src in rows:
                print("        %5d  %s" % (ln, src))
            listed += len(rows)
        # The listing IS the worklist; a cap would hide work while the header still
        # claimed the full count (that exact bug shipped in cast_ledger).
        assert listed == n_soft, "listing (%d) != SOFT total (%d)" % (listed, n_soft)

    if a.max is not None and n_soft > a.max:
        print("cast-reasons: SOFT %d exceeds the %d ratchet" % (n_soft, a.max))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
