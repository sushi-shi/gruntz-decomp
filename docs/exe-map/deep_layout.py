#!/usr/bin/env python3
"""Deep .text layout - the measured WHY behind function scatter + the original-TU
partition. Writes deep_layout.json (for deep.html) and TU_MIGRATION.md (the
our-TUs -> original-TUs instructions). See README.md, "deep map".

The four measurements it consolidates (2026-07-10 investigation):

 1. GAP CENSUS. The bytes between carved functions are: MSVC switch tables
    (belonging to the preceding function), 0xCC int3 contribution padding, nop
    alignment, or unclaimed code. Together with the leading ILT (~2.7k E9 jmp
    thunks) this proves the retail EXE was linked /INCREMENTAL.
 2. INIT-TABLE SKELETON. The CRT C++ initializer table (.data 0x208000) points at
    tiny $E fragments that never grow, so ilink never moved them: table order ==
    fragment RVA order == the ORIGINAL obj link order. 501 zeroed slots are the
    fossil record of edit/relink history. Fragments are attributed to units via
    ctor calls -> data xrefs (from .reloc) -> 2-hop -> RVA neighborhood.
 3. MECHANISM VERDICTS. Every flags.json outlier is classified:
      COMDAT-POOL-EXILE  dtor/typetag/Serialize pooled by the linker (benign)
      COMDAT-AT-USAGE    header-inline kept from the first REFERENCING obj in
                         link order (proven: BltChecked asserts incs\\ddrawmgr.h
                         yet sits in the GruntzMgr.cpp block)
      ILINK-MOVED        at home in the GruntDem.exe link, far in retail (only 3!)
      REHOME-CANDIDATE   stable in BOTH links (demo oracle: 170/181 FAR-IN-BOTH),
                         sits in another TU's coherent run -> the ORIGINAL source
                         had it in that file. Placement is BIRTH position.
 4. ORIGINAL-TU PARTITION. Because obj contributions are contiguous at first link
    and placements are birth positions, the RVA-ordered non-COMDAT unit functions
    are a concatenation of original-obj blocks. Only runs of >=3 same-unit
    functions (steps <= LOCAL) may span an interval boundary; a point crossed by
    no such run is an original TU boundary. Lone strays inside foreign intervals
    fall out as the MOVE list; units with core presence in several intervals are
    conflated (SPLIT list); multi-core intervals are MERGE groups.

Inputs: $GRUNTZ_EXE, build/gen/symbol_names.csv, build/ghidra-enrich exports (via
gruntz.analysis.exe_map), flags.json, and optionally demo_oracle.json (from
demo_oracle.py; skipped cleanly if absent). Run under `nix develop`."""
import bisect
import csv
import json
import os
import struct
from collections import Counter, defaultdict

import gruntz.analysis.exe_map as em

DIR = os.path.dirname(os.path.abspath(__file__))
EXE = os.environ.get("GRUNTZ_EXE", "")
DATA = open(EXE, "rb").read()
IMG = 0x400000
TEXT = (0x1000, 0x1e626b)
RELOC = (0x249a00, 0x1b9d1)
XC_TABLE = (0x208000, 0x2098a0)   # CRT C++ initializer table bounds (from _cinit)
BIN = 0x2000                       # zone-strip bin
LOCAL = 0x2000                     # partition: max step inside a same-unit run

SEC = [(0x1000, 0x1e526b, 0x400), (0x1e7000, 0x20fa8, 0x1e5800),
       (0x208000, 0x21400, 0x206800)]


def fo(rva):
    for lo, sz, raw in SEC:
        if lo <= rva < lo + sz:
            return raw + (rva - lo)
    raise ValueError(hex(rva))


def name_kind(name):
    if name.startswith(("??1", "??_G", "??_E")) or "DeletingDtor" in name:
        return "dtor"
    if name.startswith("??0"):
        return "ctor"
    if "Serialize" in name:
        return "serialize"
    if "GetTypeTag" in name or "GetRuntimeClass" in name:
        return "typetag"
    return "method"


def family(rec):
    c = rec["category"]
    if c == "unit":
        return "game" if rec["source"].startswith("src/Gruntz/") else "engine"
    if c in ("mfc", "crt", "zlib", "eh", "asm"):
        return "library"
    return c   # thunk / unknown


def reloc_sites():
    """Every HIGHLOW fixup as (site_rva, stored_target_rva)."""
    out = []
    off, end = RELOC[0], RELOC[0] + RELOC[1]
    while off + 8 <= end:
        page, bsz = struct.unpack("<II", DATA[off:off + 8])
        if bsz < 8:
            break
        for i in range(8, bsz, 2):
            e, = struct.unpack("<H", DATA[off + i:off + i + 2])
            if e >> 12 == 3:
                s = page + (e & 0xFFF)
                if s < 0x229400:
                    out.append((s, struct.unpack("<I", DATA[fo(s):fo(s) + 4])[0] - IMG))
        off += bsz
    return out


def resolve_thunk(t, depth=0):
    while depth < 4 and TEXT[0] <= t < TEXT[1] and DATA[fo(t)] == 0xE9:
        t = t + 5 + struct.unpack("<i", DATA[fo(t) + 1:fo(t) + 5])[0]
        depth += 1
    return t


def gap_census(funcs):
    kinds, byts, holes = Counter(), Counter(), []
    for a, b in zip(funcs, funcs[1:]):
        lo, hi = a["end"], b["rva"]
        n = hi - lo
        if n <= 0:
            kinds["packed"] += 1
            continue
        raw = DATA[fo(lo):fo(hi)]
        i = len(raw)
        while i > 0 and raw[i - 1] == 0xCC:
            i -= 1
        body = raw[:i]
        tbl = any(a["rva"] <= struct.unpack("<I", body[k:k + 4])[0] - IMG < a["end"]
                  for k in range(0, min(16, max(0, len(body) - 3))))
        if tbl:
            kind = "switch-table"
        elif i == 0:
            kind = "cc-fill"
        elif all(x in (0x90, 0x8D, 0x49, 0x00) for x in body):
            kind = "nop-align"
        elif n <= 15:
            kind = "align"
        else:
            kind = "unclaimed"
        kinds[kind] += 1
        byts[kind] += n
        if len(raw) - i >= 256:
            holes.append({"rva": lo + i, "size": len(raw) - i})
    holes.sort(key=lambda h: -h["size"])
    return dict(kinds), dict(byts), holes


def load_symbols():
    frva, fdata = {}, {}
    for r in csv.DictReader(open(os.path.join(DIR, "../../build/gen/symbol_names.csv"))):
        a = int(r["rva"], 16)
        d = frva if r["kind"] == "func" else fdata
        d[a] = (r["unit"], r["name"], int(r["size"] or "0", 16))
    return frva, fdata


def init_table(frva, fdata, by_target):
    fn_list = sorted((a, v[2], v[0]) for a, v in frva.items())
    fn_rvas = [x[0] for x in fn_list]

    def unit_of_site(s):
        i = bisect.bisect_right(fn_rvas, s) - 1
        if i >= 0 and s < fn_list[i][0] + fn_list[i][1]:
            return fn_list[i][2]

    def decode(rva):
        globs, targets = [], []
        p = resolve_thunk(rva)
        end = p + 96
        while p < end:
            b, b2 = DATA[fo(p)], DATA[fo(p) + 1]
            if b in (0xC3, 0xC2):
                break
            if b in (0xE8, 0xE9):
                targets.append(resolve_thunk(
                    p + 5 + struct.unpack("<i", DATA[fo(p) + 1:fo(p) + 5])[0]))
                if b == 0xE9:
                    break
                p += 5
                continue
            if b in (0xB8, 0xB9, 0xBA, 0xBE, 0xBF, 0x68, 0xA3, 0xA0, 0xA2):
                v = struct.unpack("<I", DATA[fo(p) + 1:fo(p) + 5])[0] - IMG
                if 0x1e7000 <= v < 0x24e800:
                    globs.append(v)
                elif TEXT[0] <= v < TEXT[1] and b == 0x68:
                    targets.append(resolve_thunk(v))
                p += 5
                continue
            if b == 0xC7 and b2 == 0x05:
                globs.append(struct.unpack("<I", DATA[fo(p) + 2:fo(p) + 6])[0] - IMG)
                p += 10
                continue
            if b == 0xC6 and b2 == 0x05:
                globs.append(struct.unpack("<I", DATA[fo(p) + 2:fo(p) + 6])[0] - IMG)
                p += 7
                continue
            if b in (0x8A, 0x88) and b2 == 0x0D:
                globs.append(struct.unpack("<I", DATA[fo(p) + 2:fo(p) + 6])[0] - IMG)
                p += 6
                continue
            p += 1
        return globs, targets

    def units_reaching(g, depth=0, seen=None):
        seen = seen if seen is not None else set()
        votes = Counter()
        for base in {g, g - 4, g - 8}:
            for s in by_target.get(base, ()):
                if s in seen:
                    continue
                seen.add(s)
                if TEXT[0] <= s < TEXT[1]:
                    u = unit_of_site(s)
                    if u:
                        votes[u] += 1
                elif depth < 1:
                    votes += units_reaching(s, depth + 1, seen)
        return votes

    n = (XC_TABLE[1] - XC_TABLE[0]) // 4
    slots = struct.unpack("<%dI" % n, DATA[fo(XC_TABLE[0]):fo(XC_TABLE[0]) + 4 * n])
    frags = []
    for i, v in enumerate(slots):
        if not v:
            continue
        t = v - IMG
        globs, targets = decode(t)
        unit, how = "", ""
        for tg in targets:
            if tg in frva:
                unit, how = frva[tg][0], "ctor"
                break
        if not unit:
            for g in globs:
                if g in fdata:
                    unit, how = fdata[g][0], "data"
                    break
        if not unit:
            votes = Counter()
            for g in globs:
                votes += units_reaching(g)
            if votes:
                unit, how = votes.most_common(1)[0][0], "xref"
        if not unit:
            lo = bisect.bisect_left(fn_rvas, t - 0x1800)
            hi = bisect.bisect_right(fn_rvas, t + 0x1800)
            votes = Counter(u for _, _, u in fn_list[lo:hi])
            if votes and votes.most_common(1)[0][1] >= 2:
                unit, how = votes.most_common(1)[0][0], "hood"
        frags.append({"i": i, "rva": t, "unit": unit, "how": how})

    runs = []
    for f in frags:
        u = f["unit"] or "?"
        if not runs or runs[-1]["unit"] != u:
            runs.append({"unit": u, "n": 0, "rva": f["rva"]})
        runs[-1]["n"] += 1
    return {"slots": n, "live": len(frags), "null": n - len(frags),
            "attributed": sum(1 for f in frags if f["unit"]),
            "frags": frags, "runs": runs}


def file_anchors(by_target, frva, fn_rvas, fn_list):
    """__FILE__ path strings -> the functions that reference them."""
    import re
    out = []
    pat = re.compile(rb"[A-Za-z]:\\[Pp]roj\\[\x20-\x7e]{3,80}")
    seen = set()
    for m in pat.finditer(DATA):
        s = m.group(0).decode("latin1")
        if s in seen:
            continue
        seen.add(s)
        off = m.start()
        rva = next((lo + (off - raw) for lo, sz, raw in SEC if raw <= off < raw + sz),
                   None)
        if rva is None:
            continue
        refs = []
        for site in by_target.get(rva, ()):
            if not (TEXT[0] <= site < TEXT[1]):
                continue
            i = bisect.bisect_right(fn_rvas, site) - 1
            if i >= 0:
                a = fn_rvas[i]
                refs.append({"fn": a, "unit": fn_list[i][2],
                             "header": s.lower().endswith(".h")})
        if refs:
            out.append({"path": s, "rva": rva, "refs": refs})
    return out


def partition(frva, units_map, exempt):
    fns = []
    for a, (u, nm, sz) in sorted(frva.items()):
        src = units_map.get(u, "")
        if not src or src.startswith("src/Stub/"):
            continue
        if name_kind(nm) in ("dtor", "serialize", "typetag") or a in exempt:
            continue
        fns.append((a, sz, u, nm))
    by_unit = defaultdict(list)
    for a, sz, u, nm in fns:
        by_unit[u].append(a)
    spans = []
    for u, rv in by_unit.items():
        for a, b, c in zip(rv, rv[1:], rv[2:]):
            if b - a <= LOCAL and c - b <= LOCAL:
                spans.append((a, c))
    spans.sort()
    bounds = []
    ei, active = 0, 0
    for (r1, s1, _, _), (r2, _, _, _) in zip(fns, fns[1:]):
        while ei < len(spans) and spans[ei][0] <= r1:
            active = max(active, spans[ei][1])
            ei += 1
        if active < r2:
            bounds.append(r2)
    segs, cur, bp = [], [], 0
    for f in fns:
        if bp < len(bounds) and f[0] >= bounds[bp]:
            if cur:
                segs.append(cur)
            cur = []
            while bp < len(bounds) and f[0] >= bounds[bp]:
                bp += 1
        cur.append(f)
    if cur:
        segs.append(cur)
    out = []
    for sg in segs:
        us = Counter(f[2] for f in sg)
        core = {u: c for u, c in us.items() if c >= 3}
        rec = {"lo": sg[0][0], "hi": sg[-1][0] + sg[-1][1], "n": len(sg),
               "units": dict(us.most_common()), "core": core,
               "strays": [{"rva": f[0], "unit": f[2], "name": f[3]}
                          for f in sg if us[f[2]] < 3]}
        if len(core) > 1:
            # weave: do the member units interleave THROUGHOUT (impossible across
            # objs at first link => genuinely ONE original TU), or sit in blocks
            # with a dirty seam (either two TUs + misattributed seam fns, or one
            # TU whose methods are class-grouped - anchors/semantics decide)?
            mem = [f for f in sg if f[2] in core]
            runs = 1 + sum(1 for a, b in zip(mem, mem[1:]) if a[2] != b[2])
            k, n = len(core), len(mem)
            rec["weave"] = round((runs - k) / max(1, n - k), 2)
            rec["merge"] = ("WOVEN" if rec["weave"] > 0.25 and runs >= 2 * k
                            else "seam-glued" if runs <= 2 * k else "mixed")
        out.append(rec)
    return out


def flag_analysis(intervals, by_target):
    """Per-interval /GX verdicts + our flag-profile conflicts.

    MSVC5 /GX game code registers EH INLINE: `push offset __ehhandler` where the
    funclet tail-jumps to ___CxxFrameHandler (0x11eea0). (__EH_prolog is the MFC
    /O1 idiom - its 181 callers are all in the lib tail.) A push site inside an
    interval proves the ORIGINAL TU was compiled /GX; a profile is only wrong in
    that direction (absence of EH sites does not prove /GX was off - /GX is
    codegen-neutral for functions with nothing to unwind)."""
    import tomllib
    CXXFH = 0x11eea0

    def is_ehhandler(t):
        p = t
        for _ in range(6):
            b = DATA[fo(p)]
            if b in (0xE8, 0xE9):
                tgt = p + 5 + struct.unpack("<i", DATA[fo(p) + 1:fo(p) + 5])[0]
                return resolve_thunk(tgt) == CXXFH
            if b in (0xB8, 0xA1):
                p += 5
            elif b == 0x8D:
                p += 3
            else:
                break
        return False

    cache = {}
    eh_sites = []
    for t, ss in by_target.items():
        if not (TEXT[0] <= t < TEXT[1]):
            continue
        if t not in cache:
            cache[t] = is_ehhandler(t)
        if cache[t]:
            eh_sites.extend(s for s in ss if TEXT[0] <= s < TEXT[1])
    eh_sites.sort()

    cfg = tomllib.loads(open(os.path.join(DIR, "../../config/units.toml")).read())
    prof = {u["unit"]: u.get("flags", "base") for u in cfg.get("unit", [])}
    srcs = {u["unit"]: u.get("source", "") for u in cfg.get("unit", [])}
    GX = {"eh", "framedeh", "mfc"}

    rows = []
    for s in intervals:
        if not s["core"]:
            continue
        n_eh = bisect.bisect_left(eh_sites, s["hi"]) - bisect.bisect_left(eh_sites, s["lo"])
        profs = {u: prof.get(u, "base") for u in s["core"]}
        has_gx = bool(set(profs.values()) & GX)
        verdict = ("MISSING-GX" if n_eh and not has_gx else
                   "mixed" if len(set(profs.values())) > 1 else "ok")
        if verdict != "ok" or n_eh:
            rows.append({"lo": s["lo"], "hi": s["hi"], "n": s["n"],
                         "eh_sites": n_eh, "profiles": profs, "verdict": verdict})
    # our invented *Eh.cpp companion splits
    pairs = []
    for u, src in srcs.items():
        if src.endswith("Eh.cpp"):
            mate = [u2 for u2, s2 in srcs.items() if s2 == src[:-6] + ".cpp"]
            if mate:
                pairs.append({"base": mate[0], "eh": u, "src": src[:-6] + ".cpp"})
    single = [(u, p, srcs.get(u, "")) for u, p in prof.items()
              if p not in ("base", "eh") ]
    return {"eh_sites_total": len(eh_sites), "rows": rows, "eh_pairs": pairs,
            "singletons": single}


def main():
    funcs, meta = em.load()
    frva, fdata = load_symbols()
    units_map = meta["units"]
    by_target = defaultdict(list)
    for s, t in reloc_sites():
        by_target[t].append(s)
    fn_list = sorted((a, v[2], v[0]) for a, v in frva.items())
    fn_rvas = [x[0] for x in fn_list]

    kinds, byts, holes = gap_census(funcs)
    itab = init_table(frva, fdata, by_target)
    anchors = file_anchors(by_target, frva, fn_rvas, fn_list)

    # outlier verdicts: context + call graph + demo oracle
    oracle = {}
    op = os.path.join(DIR, "demo_oracle.json")
    if os.path.isfile(op):
        oracle = {r["rva"]: r for r in json.load(open(op))}
    callers = defaultdict(set)
    body = DATA[0x400:0x400 + (TEXT[1] - TEXT[0])]
    i = 0
    while i < len(body) - 5:
        if body[i] in (0xE8, 0xE9):
            src = 0x1000 + i
            tgt = src + 5 + struct.unpack("<i", body[i + 1:i + 5])[0]
            if TEXT[0] <= tgt < TEXT[1]:
                tgt = resolve_thunk(tgt)
                j = bisect.bisect_right(fn_rvas, src) - 1
                if j >= 0 and src < fn_list[j][0] + fn_list[j][1] and tgt in frva:
                    if fn_list[j][0] != tgt:
                        callers[tgt].add(fn_list[j][2])
            i += 5
            continue
        i += 1
    rank = {}
    for f in itab["frags"]:
        if f["unit"] and f["unit"] not in rank:
            rank[f["unit"]] = f["i"]
    src2unit = {v: k for k, v in units_map.items() if v}
    flags = json.load(open(os.path.join(DIR, "flags.json")))
    verdicts = []
    for fl in flags["flagged"]:
        own = src2unit.get(fl["file"])
        for o in fl["outliers"]:
            kind = name_kind(o["name"])
            land = src2unit.get(o["home"] or "")
            cus = callers.get(o["rva"], set())
            orc = oracle.get(o["rva"], {}).get("verdict", "")
            if kind in ("dtor", "serialize", "typetag"):
                v = "COMDAT-POOL-EXILE"
            elif land and land in cus and rank.get(land, 1 << 30) <= min(
                    (rank.get(u, 1 << 30) for u in cus), default=1 << 30):
                v = "COMDAT-AT-USAGE"
            elif orc == "AT-HOME-IN-DEMO":
                v = "ILINK-MOVED"
            else:
                v = "REHOME-CANDIDATE"
            verdicts.append({"file": fl["file"], "rva": o["rva"], "name": o["name"],
                             "kind": kind, "land": o["home"] or "", "oracle": orc,
                             "verdict": v})
    exempt = {v["rva"] for v in verdicts
              if v["verdict"] in ("COMDAT-AT-USAGE", "ILINK-MOVED")}

    intervals = partition(frva, units_map, exempt)
    flags_x = flag_analysis(intervals, by_target)

    # zone bins for the strip
    bins = defaultdict(lambda: {"fam": Counter(), "pairs": 0, "same": 0, "cc": 0})
    unit_fns = [f for f in funcs if f["category"] == "unit"]
    for f in funcs:
        bins[f["rva"] // BIN]["fam"][family(f)] += f["size"]
    for a, b in zip(unit_fns, unit_fns[1:]):
        w = a["rva"] // BIN
        bins[w]["pairs"] += 1
        bins[w]["same"] += (a["unit"] == b["unit"])
    for a, b in zip(funcs, funcs[1:]):
        lo, hi = a["end"], b["rva"]
        if hi > lo:
            bins[lo // BIN]["cc"] += sum(1 for x in DATA[fo(lo):fo(hi)] if x == 0xCC)
    zone = []
    for w in range(TEXT[0] // BIN, TEXT[1] // BIN + 1):
        d = bins.get(w)
        zone.append({
            "rva": w * BIN,
            "fam": dict(d["fam"].most_common()) if d else {},
            "coh": round(d["same"] / d["pairs"], 2) if d and d["pairs"] >= 4 else None,
            "cc": d["cc"] if d else 0})

    out = {"text": {"lo": TEXT[0], "hi": TEXT[1]},
           "gaps": {"kinds": kinds, "bytes": byts, "holes": holes[:40]},
           "init_table": {k: itab[k] for k in
                          ("slots", "live", "null", "attributed", "runs")},
           "anchors": anchors,
           "verdicts": verdicts,
           "verdict_counts": dict(Counter(v["verdict"] for v in verdicts)),
           "intervals": intervals,
           "flags": flags_x,
           "zone": zone}
    with open(os.path.join(DIR, "deep_layout.json"), "w") as f:
        json.dump(out, f)
    print("gap kinds:", kinds)
    print("init table:", {k: itab[k] for k in ("slots", "live", "null", "attributed")})
    print("verdicts:", out["verdict_counts"])
    multi = [s for s in intervals if len(s["core"]) > 1]
    strays = sum(len(s["strays"]) for s in intervals)
    print("intervals: %d  merge-groups: %d  strays: %d"
          % (len(intervals), len(multi), strays))
    print("flags: %d EH sites, %d flagged intervals, %d *Eh.cpp pairs"
          % (flags_x["eh_sites_total"], len(flags_x["rows"]), len(flags_x["eh_pairs"])))
    write_migration(out, units_map, itab)


def write_migration(out, units_map, itab):
    """TU_MIGRATION.md - the our-TUs -> original-TUs instruction list."""
    intervals = out["intervals"]
    multi = [s for s in intervals if len(s["core"]) > 1]
    ucnt = defaultdict(list)
    for s in intervals:
        for u, c in s["core"].items():
            ucnt[u].append((s["lo"], s["hi"], c))
    splits = {u: v for u, v in ucnt.items() if len(v) > 1}
    strays = [(s, st) for s in intervals for st in s["strays"]]

    L = []
    L.append("# TU migration plan — our units → original TUs\n")
    L.append("*Generated by `deep_layout.py`. Method + evidence: see README.md "
             "(deep map) and `deep.html`. Regenerate after re-homing waves.*\n")
    L.append("**Ground truth**: placements are first-link birth positions "
             "(demo-oracle: 170/181 outliers identically placed in GruntDem.exe; "
             "only 3 ilink moves in the whole EXE), and every obj's contribution "
             "is contiguous at first link. So retail `.text` order faithfully "
             "records the ORIGINAL TU composition, and the CRT init-table gives "
             "the original obj LINK ORDER.\n")
    vc = out["verdict_counts"]
    L.append("Outlier mechanisms: " + ", ".join(
        "%s %d" % (k, v) for k, v in sorted(vc.items(), key=lambda kv: -kv[1])) + ".\n")

    L.append("\n## MERGE candidates — multi-core intervals (VERIFY per group)\n")
    L.append("**WOVEN** (units interleave throughout — impossible across objs at "
             "first link) = confirmed single original TU: combine, order = RVA "
             "order. **seam-glued/mixed** (block arrangement) is ambiguous: either "
             "two adjacent TUs glued by misattributed seam functions (re-home the "
             "seam and the boundary reappears — e.g. netmgr+font, where "
             "`FontInterfaceObject::IsInterface1-5` is really NetMgr's "
             "InterfaceObject), or one TU with class-grouped sections (e.g. "
             "ddpalette+dirpal, __FILE__-anchored as one DIRPAL.CPP). Decide by: "
             "__FILE__ anchors, init-fragment table runs (2 separate runs = 2 "
             "objs), and a seam-function xref audit.\n")
    L.append("| interval | fns | verdict | weave | combine/verify these units |")
    L.append("|---|---|---|---|---|")
    for s in sorted(multi, key=lambda s: -s["n"]):
        us = ", ".join("%s (%d)" % (u, c) for u, c in
                       sorted(s["core"].items(), key=lambda kv: -kv[1]))
        L.append("| `%#08x-%#08x` | %d | %s | %.2f | %s |"
                 % (s["lo"], s["hi"], s["n"], s.get("merge", "?"),
                    s.get("weave", 0), us))

    L.append("\n## SPLIT — units with core presence in several intervals "
             "(conflated)\n")
    L.append("| unit | intervals (lo, fns) |")
    L.append("|---|---|")
    for u, v in sorted(splits.items(), key=lambda kv: -len(kv[1])):
        L.append("| %s | %s |" % (u, ", ".join("`%#x` (%d)" % (lo, c)
                                               for lo, hi, c in v)))

    L.append("\n## MOVE — lone strays inside a foreign interval\n")
    L.append("Function-level re-homes; target = the interval's dominant unit.\n")
    L.append("| rva | function | from unit | to interval (dominant) |")
    L.append("|---|---|---|---|")
    for s, st in sorted(strays, key=lambda t: t[1]["rva"]):
        dom = max(s["core"], key=s["core"].get) if s["core"] else "?"
        L.append("| `%#08x` | `%s` | %s | `%#x` %s |"
                 % (st["rva"], st["name"][:60], st["unit"], s["lo"], dom))

    fx = out.get("flags", {})
    L.append("\n## FLAGS — compiler-profile fixes the partition implies\n")
    L.append("The original build had per-.dsp (plus rare per-file) settings; one "
             "obj = ONE flag set. Rules: an interval with inline EH-registration "
             "sites (`push offset __ehhandler` -> ___CxxFrameHandler) was /GX — "
             "flip its base members to `eh`; zero sites does NOT prove /GX off. "
             "Every merge below must land on a single profile.\n")
    bad = [r for r in fx.get("rows", []) if r["verdict"] == "MISSING-GX"]
    if bad:
        L.append("**Hard errors — EH evidence but no /GX profile:**")
        for r in bad:
            L.append("- `%#08x-%#08x` (%d EH sites): %s" % (
                r["lo"], r["hi"], r["eh_sites"],
                ", ".join("%s (%s)" % (u, p) for u, p in r["profiles"].items())))
    mixed = [r for r in fx.get("rows", []) if r["verdict"] == "mixed"]
    if mixed:
        L.append("\n**Mixed-profile merge groups (unify; EH evidence decides):**")
        for r in mixed:
            L.append("- `%#08x-%#08x` (%d EH sites): %s" % (
                r["lo"], r["hi"], r["eh_sites"],
                ", ".join("%s (%s)" % (u, p) for u, p in r["profiles"].items())))
    if fx.get("eh_pairs"):
        L.append("\n**Invented `*Eh.cpp` companion splits to collapse** (the "
                 "original file was ONE /GX TU; our `base` half only matches "
                 "because /GX is neutral for its functions):")
        for p in fx["eh_pairs"]:
            L.append("- %s + %s -> `%s` (profile `eh`)"
                     % (p["base"], p["eh"], p["src"].rsplit("/", 1)[-1]))
    if fx.get("singletons"):
        L.append("\n**Singleton profile overrides — re-derive, may mask wrong "
                 "shape/TU composition:**")
        for u, p, src in fx["singletons"]:
            L.append("- %s (`%s`) — %s" % (u, p, src))

    L.append("\n## ANCHORED facts (__FILE__ strings)\n")
    for a in out["anchors"]:
        refs = Counter(r["unit"] for r in a["refs"])
        hdr = " **(header => inline-at-usage)**" if any(
            r["header"] for r in a["refs"]) else ""
        L.append("- `%s`%s -> %s" % (a["path"], hdr, ", ".join(
            "%s (%d)" % (u, c) for u, c in refs.most_common())))

    L.append("\n## Original link order (init-table skeleton)\n")
    L.append("Compressed unit sequence of the %d attributed $E initializer "
             "fragments (of %d live; %d slots zeroed by relinks). This is the "
             "obj order of the original project files:\n"
             % (out["init_table"]["attributed"], out["init_table"]["live"],
                out["init_table"]["null"]))
    L.append("```")
    line = " | ".join(("%sx%d" % (r["unit"], r["n"])) if r["n"] > 1 else r["unit"]
                      for r in out["init_table"]["runs"])
    import textwrap
    L.extend(textwrap.wrap(line, 100))
    L.append("```")
    L.append("\n*Caveats: the engine-resource mega-interval (0x1396f0+) is glued "
             "by our own coarse units and needs per-function re-attribution "
             "before it splits (DIRSURF/DDRAWMGR/DIRPAL anchors mark three "
             "distinct files inside it). Splits invisible to layout (adjacent "
             "objs, e.g. DinMgr2.cpp + InputDevice.cpp inside our "
             "directinputmgr2) are only visible via anchors/init-frags.*")
    with open(os.path.join(DIR, "TU_MIGRATION.md"), "w") as f:
        f.write("\n".join(L) + "\n")
    print("wrote TU_MIGRATION.md (%d merge rows, %d split rows, %d moves)"
          % (len(multi), len(splits), len(strays)))


if __name__ == "__main__":
    main()
