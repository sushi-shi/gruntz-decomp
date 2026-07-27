#!/usr/bin/env python3
"""cast_worklist.py - the OPEN-cast drain worklist, enumerated and re-runnable.

`cast_ledger` answers "how many are left"; this answers "what do I do next, and in
what order". It re-classifies the ledger's OPEN set every run, so it reflects the tree
rather than a snapshot someone pasted into a doc (docs/cast-open-drain-plan.md holds the
tier rationale; this holds the rows).

Tiers, in execution order:

  T1  banned / redundant   an offset-cast `(char*)x + N` (CLAUDE.md bans these outright)
                           or a no-op cast whose operand already has the target type.
                           No judgement needed - the only question is which member the
                           offset names.
  T2  multi-site retype    2+ OPEN casts share one operand in one file: fixing that ONE
                           declaration kills all of them. Highest yield per edit.
  T3  single-site          one cast, one decision. Converts if the declaration is wrong,
                           otherwise takes a ledger-visible reason.
  T4  blocked              already carries @identity-TODO - do not guess, these are
                           counted as accounted-for by the ledger and listed here only
                           so they stay visible.

Usage:
    python -m gruntz.audit.cast_worklist              # tier summary + T1/T2 rows
    python -m gruntz.audit.cast_worklist --tier 3     # one tier's rows
    python -m gruntz.audit.cast_worklist --file X     # every OPEN row in one file
    python -m gruntz.audit.cast_worklist --next       # the single next item to do
"""
import argparse
import collections
import re

from gruntz.audit import cast_ledger as CL

CASTX = re.compile(r"reinterpret_cast\s*<\s*([^<>]*?)\s*>\s*\(")
OPERAND = re.compile(
    r"reinterpret_cast\s*<[^<>]*>\s*\(\s*&?\s*([A-Za-z_]\w*(?:(?:->|\.)\w+)*)")
INTS = {"i32", "u32", "i16", "u16", "i8", "u8", "int", "unsigned int", "long",
        "DWORD", "WORD", "BYTE"}

# an offset-cast: byte-cursor arithmetic inside the cast operand
OFFSET = re.compile(r"\+\s*0x[0-9a-f]+\s*\)|"
                    r"static_cast<char\*>\([^)]*\)\s*\+|"
                    r"reinterpret_cast<char\*>\([^)]*\)\s*\+")


def shape(target, text):
    """the syntactic family, which decides the likely fix"""
    t = target.strip()
    base = t.rstrip("*").strip()
    if OFFSET.search(text):
        return "offset-cast"
    if not t.endswith("*"):
        return "ptr->int" if base in INTS else "to-value"
    if base in INTS or base in ("char", "void"):
        if re.search(r"&\s*m_|&\s*\w+\.|&\s*\w+\[", text):
            return "struct->cursor"
        return "blob-cursor"
    if re.search(r"GetHead|GetAt|GetNext|GetData|GetTail|Lookup|m_head|m_freeHead", text):
        return "MFC-element"
    if re.search(r"reinterpret_cast<[^>]*>\(\s*&", text):
        return "view-of-member"
    return "to-class"


ACTION = {
    "offset-cast": "name the member the offset resolves to, or declare the packed record",
    "ptr->int": "retype the DESTINATION slot (member/param/return) - check non-casting callers",
    "to-value": "per-site: usually a byte/dword pun -> reason it",
    "struct->cursor": "real union/overlay -> reason; phantom view -> dissolve",
    "blob-cursor": "verify the format/width forces it -> byte-forced reason; else declare a record",
    "MFC-element": "typed accessor on the owning class (one seam), then reason it",
    "view-of-member": "real overlay -> reason; else fold into the canonical class",
    "to-class": "retype the operand's declaration; identity via xref, never RVA proximity",
}


def rows():
    _forced, openv = CL.scan()
    out = []
    for path, sites in openv.items():
        for line, text in sites:
            m = CASTX.search(text)
            target = m.group(1) if m else "?"
            o = OPERAND.search(text)
            operand = o.group(1) if o else ""
            out.append({"file": path, "line": line, "text": text,
                        "target": target, "operand": operand,
                        "shape": shape(target, text)})
    # T2 = an operand shared by 2+ OPEN casts in the same file
    clusters = collections.Counter(
        (r["file"], r["operand"]) for r in out if r["operand"])
    for r in out:
        key = (r["file"], r["operand"])
        if r["shape"] == "offset-cast":
            r["tier"] = 1
        elif clusters.get(key, 0) >= 2:
            r["tier"] = 2
            r["cluster"] = clusters[key]
        else:
            r["tier"] = 3
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tier", type=int)
    ap.add_argument("--file")
    ap.add_argument("--next", action="store_true")
    ap.add_argument("--limit", type=int, default=0)
    a = ap.parse_args()

    all_rows = rows()
    if a.file:
        sel = [r for r in all_rows if a.file in r["file"]]
    elif a.tier:
        sel = [r for r in all_rows if r["tier"] == a.tier]
    else:
        sel = [r for r in all_rows if r["tier"] <= 2]

    by_tier = collections.Counter(r["tier"] for r in all_rows)
    by_shape = collections.Counter(r["shape"] for r in all_rows)
    print("OPEN %d   T1 %d (banned/redundant)  T2 %d (multi-site)  T3 %d (single-site)"
          % (len(all_rows), by_tier[1], by_tier[2], by_tier[3]))
    print("shapes: " + "  ".join("%s=%d" % kv for kv in by_shape.most_common()))
    print()

    if a.next:
        sel = sorted(sel, key=lambda r: (r["tier"], -r.get("cluster", 1)))[:1]

    # cluster T2 rows so one edit is one entry
    groups = collections.defaultdict(list)
    for r in sorted(sel, key=lambda r: (r["tier"], r["file"], r["line"])):
        groups[(r["tier"], r["file"], r["operand"] if r["tier"] == 2 else r["line"])].append(r)

    n = 0
    for (tier, path, _k), rs in groups.items():
        n += 1
        if a.limit and n > a.limit:
            print("   ... %d more groups" % (len(groups) - a.limit))
            break
        head = rs[0]
        tag = "T%d" % tier
        if tier == 2:
            print("%s  %s  operand `%s` x%d" % (tag, path, head["operand"], len(rs)))
        else:
            print("%s  %s:%d" % (tag, path, head["line"]))
        print("      shape : %s" % head["shape"])
        print("      action: %s" % ACTION[head["shape"]])
        for r in rs:
            print("      %5d  %s" % (r["line"], r["text"][:88]))
        print()


main()
