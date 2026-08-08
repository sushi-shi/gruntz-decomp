#!/usr/bin/env python3
"""Reconcile our CButeMgr call sites against retail - the binary, and the data.

Retail reads its tuning out of one file, `GAME\\ATTRIBUTEZ` inside Gruntz.REZ,
through `g_buteMgr`. Two independent things can be checked against that, and
they fail in different ways:

  * ACCESSOR MIX (needs only GRUNTZ.EXE). `GetIntDef` @0x171aa0 and
    `GetDwordDef` @0x1721e0 are different functions, so picking the wrong one is
    a `call` to the wrong address. Resolve every retail call that reaches a
    CButeMgr accessor (through the ILT thunks), bucket the sites by our own
    `RVA()` spans, and compare per function. A function that disagrees on WHICH
    accessor is called has a source bug; a function that disagrees only on the
    COUNT is a weaker signal (our spans can absorb a neighbouring COMDAT), but
    it is still a lead - MSVC does not elide calls.

  * DATA JOIN (needs the decrypted archive, `butez ... cat 'GAME\\ATTRIBUTEZ'`).
    Each accessor returns the stored value only when the record's type matches -
    `GetIntDef` wants BUTE_INT, `GetDwordDef` wants BUTE_DWORD - and on a
    mismatch reports an error and returns the DEFAULT. So joining every literal
    (section, key) against the file's `(DWORD)`/bare-int/quoted annotation finds
    both type mismatches and reads of keys the shipped file never defines.

    Absent is not automatically a bug: retail genuinely reads keys the shipped
    data omits, and the default is then the only value that ever applies. What
    IS a bug is a key we spell differently from retail - cross-checked here
    against the EXE's own string table.

    usage: python -m gruntz.audit.bute_reconcile [--bute <decrypted ATTRIBUTEZ>]
"""
import argparse
import os
import re
import sys
from collections import Counter, defaultdict

from gruntz.core.pe import PE, REPO

# rva -> accessor, from src/Bute/ButeMgr.cpp's own RVA() claims.
ACCESSOR_RVA = {
    0x00171A60: "Exists",
    0x00171AA0: "GetIntDef",
    0x00171AF0: "GetInt",
    0x001721E0: "GetDwordDef",
    0x00172240: "GetDword",
    0x001726C0: "GetFloatDef",
    0x00172730: "GetFloat",
    0x00172BD0: "GetDoubleDef",
    0x00172C40: "GetDouble",
    0x00173180: "GetStringDef",
    0x001731D0: "GetString",
    0x00173720: "GetRectDef",
    0x00173770: "GetRect",
    0x00173CB0: "GetPointDef",
    0x00173D00: "GetPoint",
    0x001741F0: "GetVectorDef",
    0x00174240: "GetVector",
    0x00174770: "GetRangeDef",
    0x001747C0: "GetRange",
}

# what each accessor DEMANDS of the stored record's type
WANTS = {
    "Exists": "ANY", "GetInt": "INT", "GetIntDef": "INT",
    "GetDword": "DWORD", "GetDwordDef": "DWORD",
    "GetFloat": "FLOAT", "GetFloatDef": "FLOAT",
    "GetDouble": "DOUBLE", "GetDoubleDef": "DOUBLE",
    "GetString": "STRING", "GetStringDef": "STRING",
    "GetRect": "RECT", "GetRectDef": "RECT",
    "GetPoint": "POINT", "GetPointDef": "POINT",
    "GetVector": "VECTOR", "GetVectorDef": "VECTOR",
    "GetRange": "RANGE", "GetRangeDef": "RANGE",
}

ACC_RE = re.compile(r"\b(g_buteMgr|bute)\s*(?:\.|->)\s*(" + "|".join(sorted(WANTS)) + r")\s*\(")
RVA_RE = re.compile(r"^RVA(?:_COMPGEN)?\(0x([0-9a-fA-F]{8}),\s*(0x[0-9a-fA-F]+)", re.M)
LIT_RE = re.compile(
    r'\b(?:static\s+)?(?:const\s+)?char\s*\*?\s*(\w+)\s*(?:\[\s*\d*\s*\])?\s*=\s*"((?:[^"\\]|\\.)*)"'
)


# ----------------------------------------------------------------- the sources

def split_args(s):
    """Top-level comma split that does not cut inside a string or a nested ()."""
    args, depth, cur, i = [], 0, "", 0
    while i < len(s):
        c = s[i]
        if c == '"':
            j = i + 1
            while j < len(s) and s[j] != '"':
                j += 2 if s[j] == "\\" else 1
            cur += s[i:j + 1]
            i = j + 1
            continue
        depth += (c in "([{") - (c in ")]}")
        if c == "," and depth == 0:
            args.append(cur.strip())
            cur = ""
        else:
            cur += c
        i += 1
    if cur.strip():
        args.append(cur.strip())
    return args


def resolve(arg, lits):
    """(text, is_literal) for a tag/key argument."""
    arg = arg.strip()
    m = re.match(r'^"((?:[^"\\]|\\.)*)"$', arg)
    if m:
        return m.group(1), True
    m = re.match(r'^DATA_COMPGEN\(0x[0-9a-f]+,\s*\w+,\s*"((?:[^"\\]|\\.)*)"\)$', arg)
    if m:
        return m.group(1), True
    m = re.match(r"^(?:const_cast<char\*>\()?static_cast<(?:const char\*|LPCTSTR)>\((\w+)\)\)?$", arg)
    if m:
        arg = m.group(1)
    m = re.match(r"^&?(\w+)$", arg)
    if m and m.group(1) in lits:
        return lits[m.group(1)], True
    if arg in ("NULL", "0"):
        return None, True
    return arg, False


def scan_sources(roots):
    """Every g_buteMgr/bute accessor call site under `roots`."""
    sites = []
    for root in roots:
        for dirpath, _, names in os.walk(root):
            for n in sorted(names):
                if not n.endswith((".cpp", ".h")):
                    continue
                p = os.path.join(dirpath, n)
                text = open(p, encoding="utf-8", errors="replace").read()
                if "buteMgr" not in text and "bute." not in text:
                    continue
                lits = {m.group(1): m.group(2) for m in LIT_RE.finditer(text)}
                marks = [(m.start(), int(m.group(1), 16), int(m.group(2), 16))
                         for m in RVA_RE.finditer(text)]
                for m in ACC_RE.finditer(text):
                    i, depth, j = m.end(), 1, m.end()
                    while j < len(text) and depth:
                        if text[j] == '"':
                            j += 1
                            while j < len(text) and text[j] != '"':
                                j += 2 if text[j] == "\\" else 1
                        else:
                            depth += (text[j] == "(") - (text[j] == ")")
                        j += 1
                    args = split_args(text[i:j - 1])
                    if not args:
                        continue
                    tag, tag_lit = resolve(args[0], lits)
                    key, key_lit = resolve(args[1], lits) if len(args) > 1 else (None, True)
                    owner = [x for x in marks if x[0] < m.start()]
                    sites.append(dict(
                        file=os.path.relpath(p, REPO),
                        line=text.count("\n", 0, m.start()) + 1,
                        recv=m.group(1), acc=m.group(2), want=WANTS[m.group(2)],
                        tag=tag, tag_lit=tag_lit, key=key, key_lit=key_lit,
                        owner_rva=owner[-1][1] if owner else None,
                        owner_size=owner[-1][2] if owner else None,
                    ))
    return sites


# ------------------------------------------------------------- the bute file

def parse_bute(path):
    """{section: {key: INT|DWORD|STRING}} from a DECRYPTED bute file.

    Read as latin1, not utf-8: the obfuscated `[CheatN] Text` fields are >0x7f.
    """
    raw = re.sub(r"/\*.*?\*/", "", open(path, "rb").read().decode("latin1"), flags=re.S)
    out, cur = {}, None
    for line in raw.splitlines():
        s = line.strip()
        if not s or s.startswith("//"):
            continue
        m = re.match(r"^\[([^\]]+)\]", s)
        if m:
            cur = m.group(1)
            out.setdefault(cur, {})
            continue
        m = re.match(r"^([A-Za-z_]\w*)\s*=\s*(.*)$", s)
        if m and cur is not None:
            v = m.group(2).strip()
            out[cur][m.group(1)] = ("STRING" if v.startswith('"')
                                    else "DWORD" if v.startswith("(DWORD)")
                                    else "INT" if re.match(r"^-?\d+\s*(//.*)?$", v)
                                    else "OTHER")
    return out


# ------------------------------------------------------------------- reports

def accessor_mix(sites, pe):
    """Per-function {accessor: count}, ours vs retail. Returns the disagreements."""
    reach = {}
    for rva, name in ACCESSOR_RVA.items():
        reach[rva] = name
        for t in pe.thunks_to(rva):
            reach[t] = name
    retail_sites = {site: name for target, name in reach.items()
                    for site, op in pe.call_index.get(target, []) if op == 0xE8}

    ours = defaultdict(Counter)
    spans = {}
    for s in sites:
        if s["owner_rva"] is None or not s["file"].startswith("src/"):
            continue
        ours[s["owner_rva"]][s["acc"]] += 1
        spans[s["owner_rva"]] = (s["owner_rva"] + s["owner_size"], s["file"])

    theirs = defaultdict(Counter)
    ordered = sorted(spans.items())
    for site, name in retail_sites.items():
        for lo, (hi, _) in ordered:
            if lo <= site < hi:
                theirs[lo][name] += 1
                break

    rows = []
    for rva in sorted(ours):
        if ours[rva] != theirs[rva]:
            kind = "WRONG-ACCESSOR" if set(ours[rva]) != set(theirs[rva]) else "COUNT"
            rows.append((kind, rva, spans[rva][1], dict(ours[rva]), dict(theirs[rva])))
    return rows, len(retail_sites)


def data_join(sites, bute, exe_strings):
    verdicts = defaultdict(list)
    for s in sites:
        if s["recv"] == "bute":
            v = "CHEATZ-FILE"          # the user-supplied file, not ATTRIBUTEZ
        elif not s["tag_lit"] or not s["key_lit"]:
            v = "COMPUTED"
        elif s["tag"] not in bute:
            v = "SECTION-ABSENT"
        elif s["key"] is None:
            v = "OK"                   # Exists(tag, NULL): a section probe
        elif s["key"] not in bute[s["tag"]]:
            v = "KEY-ABSENT" if s["key"] in exe_strings else "KEY-ABSENT-AND-MISSPELLED"
        else:
            have = bute[s["tag"]][s["key"]]
            v = "OK" if s["want"] in ("ANY", have) else f"TYPE-MISMATCH({s['want']}!={have})"
        s["verdict"] = v
        verdicts[v.split("(")[0]].append(s)
    return verdicts


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--bute", help="decrypted GAME\\ATTRIBUTEZ "
                                   "(butez <rez> cat 'GAME\\ATTRIBUTEZ' > f)")
    args = ap.parse_args()

    sites = scan_sources([REPO / "src", REPO / "include"])
    pe = PE()

    print(f"=== accessor mix: our RVA() spans vs retail's call targets "
          f"({len(sites)} call site(s) in src/) ===")
    rows, n_retail = accessor_mix(sites, pe)
    print(f"{n_retail} retail call site(s) reach a CButeMgr accessor")
    for kind, rva, f, o, t in rows:
        print(f"  {kind:<15} 0x{rva:08x}  {f}")
        print(f"      ours   {o}")
        print(f"      retail {t}")
    if not rows:
        print("  (no disagreement)")

    if not args.bute:
        print("\n(no --bute: skipping the data join)")
        return 0

    bute = parse_bute(args.bute)
    exe_strings = set(pe.data.split(b"\0"))
    exe_strings = {s.decode("latin1") for s in exe_strings if s}
    verdicts = data_join(sites, bute, exe_strings)

    print(f"\n=== data join vs {args.bute} "
          f"({len(bute)} sections) ===")
    for k in sorted(verdicts, key=lambda k: -len(verdicts[k])):
        print(f"{len(verdicts[k]):>5}  {k}")
    for kind in ("TYPE-MISMATCH", "KEY-ABSENT-AND-MISSPELLED", "SECTION-ABSENT", "KEY-ABSENT"):
        rows = [s for s in sites if s["verdict"].startswith(kind)]
        if not rows:
            continue
        print(f"\n--- {kind} ({len(rows)}) ---")
        for s in sorted(rows, key=lambda r: (r["tag"] or "", r["key"] or "")):
            print(f"  [{s['tag']}] {s['key']:<26} {s['acc']:<13} "
                  f"{s['file']}:{s['line']}  {s['verdict']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
