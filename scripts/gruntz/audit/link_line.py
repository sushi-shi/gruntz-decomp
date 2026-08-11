#!/usr/bin/env python3
"""link_line.py - derive retail GRUNTZ.EXE's original link line from `.text` band order.

MECHANISM (read out of link.exe 5.10 + probe-proven, docs/link-text-layout.md):
contribution records append at their group's TAIL in arrival order, nothing splits
or reorders an obj's `.text` within its `$`-group, and library members always follow
ALL command-line objects (in symbol-resolution pull order). So sorting our TU bands
by ascending retail band-start RVA recovers the ARRIVAL order of the objects - and,
for the objects that were on the command line, that sequence IS the command line.

Two independent oracles corroborate/classify every row:

  * the INCREMENTAL-THUNK oracle (gruntz.audit.thunk_oracle): link.exe thunks only
    objects ON THE LINK LINE, never a .LIB member - a thunked function PROVES its
    obj was on the command line; a whole band of unthunked functions above the last
    thunked band is a library member;
  * the CRT INIT TABLE (.data 0x208000-0x2098a0): `.CRT$XCU` contributions obey the
    same append-at-tail law, so XCU slot order == obj arrival order for every obj
    with a dynamic initializer. Measured: 0 inversions against band order.

WHAT IS PROVEN vs INFERRED, per row (the `evidence` column):
  * the SEQUENCE itself is the layout mechanism - proven for every row whose band
    does not interleave another (interleaves are our partition defects, flagged);
  * `thunk(n)`  - on the command line, PROVEN (n incremental thunks target the band);
  * `position`  - side of the object/library boundary only, INFERRED (an uncalled
    command-line obj and a library member look the same to the thunk oracle);
  * `xcu:<n>`   - the unit's first CRT-init slot, independent corroboration;
  * class `lib` rows are in PULL order: a resolution artifact, NOT a command-line
    fact - the original .LIB member ORDER is unrecoverable from layout;
  * class `comdat-owner` rows are not link-line objects at all: every body our tree
    homes there was a multi-defined COMDAT kept inside some HOST's contribution
    (config/retail/kept-comdat-exiles.tsv, or a compgen-only instantiation host).
    They get no seq.

BAND EXTENTS come from RVA() pins only (the tu_order_check definition): an
RVA_COMPGEN body is a compiler-generated COMDAT - exactly the multi-defined class
the linker keeps at its FIRST definer, so its address marks the keeper's
contribution, not its class-homed owner's. Also excluded: the kept-COMDAT exile
bodies and the 3 demo-oracle-proven ilink moves (a re-link position is not a birth
position; docs/exe-map/README.md). Bands wholly inside the COMDAT-pool windows
(tu_layout.POOLS) are ordered but flagged `pool-region` - kept-COMDAT contamination
of the edges is possible there.

    python -m gruntz.audit.link_line                 # derivation report
    python -m gruntz.audit.link_line --emit          # write config/retail/link-order.tsv
    python -m gruntz.audit.link_line --check         # re-derive, compare committed file
    python -m gruntz.audit.link_line --objlist FILE  # obj-stem order for `gruntz link --order`
    python -m gruntz.audit.link_line --measure MAP   # candidate .map vs the derived order
    python -m gruntz.audit.link_line --exiles        # kept-COMDAT keeper prediction vs ledger
"""
from __future__ import annotations

import argparse
import bisect
import csv
import struct
import subprocess
import sys
import tomllib
from collections import defaultdict
from pathlib import Path

from gruntz.audit.tu_layout import RVA_RE, _parse_size, pooled

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
NAMES = REPO / "build" / "gen" / "symbol_names.csv"
UNITS = REPO / "config" / "units.toml"
EXILES = REPO / "config" / "retail" / "kept-comdat-exiles.tsv"
COMPGEN_FNS = REPO / "config" / "retail" / "compiler-generated-functions.tsv"
ORDER_TSV = REPO / "config" / "retail" / "link-order.tsv"
BASE_OBJS = REPO / "build" / "objdiff" / "base"

# CRT C++ initializer table bounds (from _cinit; docs/exe-map/deep_layout.py).
XC_TABLE = (0x208000, 0x2098a0)
# Same outlier law as tu_order_check: a small remote group is a stray COMDAT /
# misplaced body, not part of the compiland's contiguous run.
OUTLIER_GAP = 0x8000
MAX_OUTLIERS = 3
# The 3 demo-oracle-proven ilink moves (AT-HOME-IN-DEMO in demo_oracle.json): the
# only functions in all of retail whose placement is a RE-link position, not a
# first-link birth position. Excluded from band extents.
ILINK_MOVED = {0x00136a30, 0x00136ce0, 0x001549d0}
# Pseudo-units in symbol_names.csv that are not TUs.
SKIP_UNITS = {"library_data", "vtables"}


# ---------------------------------------------------------------- inputs

def load_exile_rvas():
    out = {}
    if EXILES.is_file():
        for ln in EXILES.read_text().splitlines():
            if not ln.strip() or ln.startswith("#"):
                continue
            f = ln.split("\t")
            if len(f) >= 4:
                out[int(f[0], 16)] = (f[1], f[2], f[3])
    return out


def load_symbol_functions():
    """The full rva-sorted function universe [(rva, size, unit, name)]."""
    allf = []
    for r in csv.DictReader(NAMES.open()):
        if r["kind"] != "func" or r["unit"] in SKIP_UNITS:
            continue
        allf.append((int(r["rva"], 16), int(r["size"] or "0", 16),
                     r["unit"], r["name"]))
    allf.sort()
    return allf


def load_pins():
    """Per unit, from its OWN source file: the RVA()-pinned bodies (exiles and
    ilink moves excluded) - the same universe tu_order_check gates on.

    -> (unit -> [(rva, size)], unit -> module, stem -> unit,
        exiles, exile rva -> owning unit)"""
    exiles = load_exile_rvas()
    units = tomllib.load(UNITS.open("rb"))["unit"]
    pins, mod, stem2unit, exile_owner = {}, {}, {}, {}
    for u in units:
        src = REPO / u["source"]
        parts = Path(u["source"]).parts
        mod[u["unit"]] = parts[1] if u["source"].startswith("src/") and len(parts) > 1 \
            else "vendor"
        stem2unit.setdefault(Path(u["source"]).stem, u["unit"])
        seq = []
        if src.suffix == ".cpp" and src.is_file():
            for ln in src.read_text(errors="replace").splitlines():
                m = RVA_RE.search(ln)
                if not m:
                    continue
                rva = int(m.group(1), 16)
                if rva in exiles:
                    exile_owner[rva] = u["unit"]     # the pin names the owner unit
                    continue
                if rva in ILINK_MOVED:
                    continue
                seq.append((rva, _parse_size(m.group(2))))
        pins[u["unit"]] = sorted(seq)
    return pins, mod, stem2unit, exiles, exile_owner


def peel(entries):
    """(cluster, strays, multi_region) - tu_order_check.tu_cluster's law: peel
    remote minority groups (<= MAX_OUTLIERS) separated by >= OUTLIER_GAP; a large
    remote group means a genuinely multi-region unit (conflated TU)."""
    ent = sorted(entries)
    strays, multi = [], False
    while len(ent) > 1:
        gaps = [(ent[k + 1][0] - ent[k][0], k) for k in range(len(ent) - 1)]
        gap, i = max(gaps)
        if gap < OUTLIER_GAP:
            break
        lo, hi = ent[:i + 1], ent[i + 1:]
        drop = lo if len(lo) <= len(hi) else hi
        if len(drop) > MAX_OUTLIERS:
            multi = True
            break
        strays += drop
        ent = [e for e in ent if e not in drop]
    return ent, strays, multi


def thunk_target_set():
    from gruntz.core.pe import PE
    from gruntz.audit.thunk_oracle import thunk_targets
    pe = PE()
    return pe, thunk_targets(pe)


def xcu_first_slots(pe, allf):
    """unit -> first CRT-init-table slot index. Attribution: the volatile `$E`
    manifest (compiler-generated-functions.tsv) first, then function extents."""
    frag2unit = {}
    if COMPGEN_FNS.is_file():
        for ln in COMPGEN_FNS.read_text().splitlines():
            if not ln.strip() or ln.startswith("#"):
                continue
            f = ln.split("\t")
            if len(f) >= 4:
                frag2unit[int(f[0], 16)] = f[3]
    starts = [x[0] for x in allf]

    def unit_at(rva):
        if rva in frag2unit:
            return frag2unit[rva]
        i = bisect.bisect_right(starts, rva) - 1
        if i >= 0 and rva < allf[i][0] + max(allf[i][1], 1):
            return allf[i][2]
        return None

    d = pe.data

    def resolve(rva):
        o = pe.off(rva)
        if o is not None and d[o] == 0xE9:
            return rva + 5 + struct.unpack_from("<i", d, o + 1)[0]
        return rva

    first, off = {}, pe.off(XC_TABLE[0])
    for i in range((XC_TABLE[1] - XC_TABLE[0]) // 4):
        v = struct.unpack_from("<I", d, off + i * 4)[0]
        if not v:
            continue
        u = unit_at(resolve(v - pe.image_base))
        if u is not None and u not in first:
            first[u] = i
    return first


# ---------------------------------------------------------------- derivation

class Row:
    __slots__ = ("unit", "start", "end", "cls", "evidence", "notes",
                 "module", "xcu", "thunked", "n")

    def __init__(self, **kw):
        for k in self.__slots__:
            setattr(self, k, kw.get(k))


def derive():
    """-> (ordered rows [by band start], comdat_owner rows, diag dict)."""
    pins, mod, stem2unit, exiles, exile_owner = load_pins()
    allf = load_symbol_functions()
    pe, thunks = thunk_target_set()
    xcu = xcu_first_slots(pe, allf)
    sym_extent = defaultdict(list)
    for rva, size, unit, _nm in allf:
        sym_extent[unit].append((rva, size))

    rows, owners = [], []
    diag = {"strays": [], "multi": [], "conflicts": [],
            "interleaves": defaultdict(set)}

    def owner_row(unit, evidence, notes=""):
        ext = sym_extent.get(unit)
        s = min(e[0] for e in ext) if ext else None
        e = max(x[0] + x[1] for x in ext) if ext else None
        owners.append(Row(unit=unit, start=s, end=e, cls="comdat-owner",
                          evidence=evidence, notes=notes, module=mod.get(unit, "?"),
                          xcu=xcu.get(unit), thunked=0, n=len(ext or ())))

    for unit, ents in pins.items():
        notes = []
        if not ents:
            if mod.get(unit) == "vendor" and sym_extent.get(unit):
                # vendored C TU: labels come from config, not RVA() macros - a
                # real obj; take its extent from symbol_names.
                ents, notes = sorted(sym_extent[unit]), ["extent-fallback"]
            elif unit in exile_owner.values():
                hosts = sorted({stem2unit.get(h, h)
                                for rva, (o, h, _n) in exiles.items()
                                if exile_owner.get(rva) == unit})
                owner_row(unit, "exile-ledger", "kept in " + "+".join(hosts))
                continue
            elif not sym_extent.get(unit):
                owner_row(unit, "no-text")       # data-only TU: no .text to order
                continue
            else:
                owner_row(unit, "compgen-only")
                continue
        nonpool = [e for e in ents if not pooled(e[0])]
        base = nonpool
        if not nonpool:
            base = ents
            notes.append("pool-region")
        cluster, strays, multi = peel(base)
        if multi:
            notes.append("multi-region")
            diag["multi"].append(unit)
        diag["strays"] += [(unit, e) for e in strays]
        start = cluster[0][0]
        end = max(e[0] + e[1] for e in cluster)
        thunked = sum(1 for e in cluster if e[0] in thunks)
        rows.append(Row(unit=unit, start=start, end=end, cls=None, evidence=None,
                        notes=";".join(notes), module=mod.get(unit, "?"),
                        xcu=xcu.get(unit), thunked=thunked, n=len(cluster)))

    rows.sort(key=lambda r: r.start)

    # object/library boundary: every lib member follows ALL command-line objs, and
    # a thunk PROVES command-line. Boundary = end of the last thunk-proven band.
    obj_end = max((r.end for r in rows if r.thunked), default=0)
    lib_start = min((r.start for r in rows if not r.thunked and r.start > obj_end),
                    default=obj_end)
    for r in rows:
        if r.thunked:
            r.cls, r.evidence = "cmdline", f"thunk({r.thunked})"
        elif r.start >= lib_start:
            r.cls, r.evidence = "lib", "position"
        else:
            r.cls, r.evidence = "cmdline", "position"
        if r.xcu is not None:
            r.evidence += f",xcu:{r.xcu}"

    # a thunk-proven band that STARTS above the first lib band would contradict
    # the command-line/library placement law - report, never silently classify.
    for r in rows:
        if r.thunked and r.start >= lib_start:
            diag["conflicts"].append(r.unit)

    # interleaves (our partition defects): order between the members is ambiguous
    for i, a in enumerate(rows):
        for b in rows[i + 1:]:
            if b.start >= a.end:
                break
            diag["interleaves"][a.unit].add(b.unit)
            diag["interleaves"][b.unit].add(a.unit)
    for r in rows:
        il = diag["interleaves"].get(r.unit)
        if il:
            r.notes = ";".join(filter(None, [r.notes,
                                             "interleaves:" + "+".join(sorted(il))]))

    # xcu corroboration: slot order must equal band order
    seq = [(r.xcu, r.unit) for r in rows if r.xcu is not None]
    diag["xcu_units"] = len(seq)
    diag["xcu_inversions"] = [(a, b) for a, b in zip(seq, seq[1:]) if b[0] < a[0]]
    diag["obj_end"], diag["lib_start"] = obj_end, lib_start
    diag["stem2unit"], diag["exile_owner"] = stem2unit, exile_owner
    owners.sort(key=lambda r: (r.start is None, r.start or 0, r.unit))
    return rows, owners, diag


# ---------------------------------------------------------------- emit / check

HEADER = """\
# Retail GRUNTZ.EXE link order - DERIVED, do not hand-edit.
# Regenerate: python -m gruntz.audit.link_line --emit
# Verify:     python -m gruntz.audit.link_line --check
#
# Mechanism (docs/link-text-layout.md): .text contributions append in arrival
# order, so ascending band-start RVA == the order the objects reached link.exe.
# class=cmdline rows are the command line itself (evidence `thunk(n)` = proven by
# n incremental-thunk targets; `position` = inferred from the boundary only).
# class=lib rows follow in library PULL order - a resolution artifact; the .LIB
# member order on disk is NOT recoverable from layout. class=comdat-owner rows
# are not link-line objects (their bodies were COMDATs kept inside a host's
# contribution) and carry no seq. `xcu:<n>` = first CRT-init-table slot, an
# independent corroboration of the sequence (0 inversions measured). n = pinned
# bodies inside the band. Notes: `interleaves:` = tu-partition backlog, order
# between those members is ambiguous; `pool-region` = band inside a COMDAT-pool
# window; `multi-region` = conflated TU, ordered by its first region.
#
# seq\tunit\tstart\tend\tclass\tmodule\tn\tevidence\tnotes"""


def fmt_rows(rows, owners):
    out = [HEADER]
    for i, r in enumerate(rows):
        out.append((f"{i}\t{r.unit}\t{r.start:#010x}\t{r.end:#010x}\t{r.cls}"
                    f"\t{r.module}\t{r.n}\t{r.evidence}\t{r.notes}").rstrip("\t"))
    for r in owners:
        span = (f"{r.start:#010x}\t{r.end:#010x}" if r.start is not None else "-\t-")
        out.append((f"-\t{r.unit}\t{span}\tcomdat-owner\t{r.module}\t{r.n}"
                    f"\t{r.evidence}\t{r.notes}").rstrip("\t"))
    return "\n".join(out) + "\n"


def parse_order_tsv(path):
    """[(seq|None, unit, cls)] from a committed link-order.tsv."""
    rows = []
    for ln in path.read_text().splitlines():
        if not ln.strip() or ln.startswith("#"):
            continue
        f = ln.split("\t")
        rows.append((None if f[0] == "-" else int(f[0]), f[1], f[4]))
    return rows


def objlist_text() -> str:
    """Every current unit in the retail-derived arrival order.

    COMDAT-owner units have no independent retail contribution, so insert them
    at the RVA of the body their object supplied. Owners with no position remain
    explicitly unordered at the tail. Keeping this as a reusable producer stops
    consumers from passing the descriptive TSV itself to link.exe as though it
    were a one-stem-per-line response file.
    """
    rows, owners, _diag = derive()
    seq = [(r.start, r.unit) for r in rows]
    seq += [(r.start, r.unit) for r in owners if r.start is not None]
    seq.sort()
    lines = ["# derived retail link order (comdat-owner units inserted at their "
             "kept bodies' position)"]
    lines += [unit for _start, unit in seq]
    lines += [r.unit for r in owners if r.start is None]
    return "\n".join(lines) + "\n"


def check() -> int:
    """Set-tolerant, order-strict: a unit added/removed/renamed by partition work
    is a NOTICE (regen with --emit); an ORDER INVERSION or a class flip among
    surviving units is a FAIL - the layout model itself moved."""
    if not ORDER_TSV.is_file():
        print(f"[link-line] {ORDER_TSV} missing - run --emit", file=sys.stderr)
        return 2
    rows, owners, _diag = derive()
    fresh_seq = [r.unit for r in rows]
    fresh_pos = {u: i for i, u in enumerate(fresh_seq)}
    fresh_cls = {r.unit: r.cls for r in rows}
    fresh_cls.update({r.unit: r.cls for r in owners})
    committed = parse_order_tsv(ORDER_TSV)
    com_cls = {u: c for _, u, c in committed}
    com_seq = [u for s, u, _ in committed if s is not None]

    added = [u for u in fresh_cls if u not in com_cls]
    removed = [u for u in com_cls if u not in fresh_cls]
    flips = [(u, com_cls[u], fresh_cls[u]) for u in com_cls
             if u in fresh_cls and com_cls[u] != fresh_cls[u]]
    common = [u for u in com_seq if u in fresh_pos]
    inv = [(a, b) for a, b in zip(common, common[1:])
           if fresh_pos[a] > fresh_pos[b]]

    bad = 0
    for u, c0, c1 in flips:
        print(f"[link-line] CLASS FLIP: {u}  {c0} -> {c1}", file=sys.stderr)
        bad = 1
    for a, b in inv:
        print(f"[link-line] ORDER INVERSION vs committed: {a} now after {b}",
              file=sys.stderr)
        bad = 1
    if added or removed:
        print(f"[link-line] notice: unit set drifted (+{len(added)}/-{len(removed)}"
              f": {', '.join((added + removed)[:6])} ...) - regen with --emit")
    if bad:
        print("[link-line] the committed order no longer re-derives - if the change "
              "is intended (a re-home moved a band), regenerate: "
              "python -m gruntz.audit.link_line --emit", file=sys.stderr)
        return 2
    print(f"[link-line] OK: {len(common)} committed rows re-derive in the same "
          f"order ({len(added)} added / {len(removed)} removed since emit)")
    return 0


# ---------------------------------------------------------------- measurement

def measure(map_path: Path) -> int:
    """How close is a candidate link's cross-TU order to the derived retail one?"""
    from gruntz.audit.link_order import load_candidate_order
    rows, _owners, _diag = derive()
    want_pos = {r.unit: i for i, r in enumerate(rows)}
    cand = load_candidate_order(map_path)
    # dict insertion order == candidate layout order (built from an RVA-sorted
    # list); an archive member appears as `libname:member.obj` - strip both.
    got = [o.split(":", 1)[-1] for o in cand]
    got = [o[:-4] if o.endswith(".obj") else o for o in got]
    got = [u for u in got if u in want_pos]
    n = len(got)
    inv_adj = sum(1 for a, b in zip(got, got[1:]) if want_pos[a] > want_pos[b])
    disc = tot = 0
    for i in range(n):
        wi = want_pos[got[i]]
        for j in range(i + 1, n):
            tot += 1
            if wi > want_pos[got[j]]:
                disc += 1
    right_pred = sum(1 for i in range(1, n)
                     if want_pos[got[i - 1]] == want_pos[got[i]] - 1)
    print(f"[link-line] {map_path.name}: {n} units in both map and derived order")
    print(f"  adjacent inversions : {inv_adj}")
    if tot:
        print(f"  Kendall tau distance: {disc}/{tot} = {disc / tot:.4f}")
    if n > 1:
        print(f"  correct predecessor : {right_pred}/{n - 1}")
    return 0


# ---------------------------------------------------------------- exile prediction

def _mangled_class(m):
    """The class of a mangled member name (?f@C@@..., ??0C@@..., ??_GC@@...)."""
    if m.startswith("??_") and len(m) > 4:      # ??_G / ??_E / ??_7 ...
        rest = m[4:]
    elif m.startswith("??"):                    # ??0 / ??1 ctor/dtor
        rest = m[3:]
    elif m.startswith("?"):
        at = m.find("@")
        rest = m[at + 1:] if at >= 0 else ""
    else:
        return None
    return rest.split("@@", 1)[0].split("@", 1)[0] or None


def predict_exiles(use_class_refs=True) -> int:
    """Predict each kept-COMDAT exile's HOST from the derived order: the keeper is
    the FIRST obj in arrival order that would have emitted the COMDAT. Emitters are
    proxied by our base objs' symbol references (a call in our obj = a local COMDAT
    copy in retail's header-inline world; for a virtual/dtor, any instantiator of
    the class - a ??_7/??0/??1 reference - emitted the vtable and with it the slot
    bodies)."""
    rows, _owners, diag = derive()
    pos = {r.unit: i for i, r in enumerate(rows)}
    stem2unit, exile_owner = diag["stem2unit"], diag["exile_owner"]
    exiles = load_exile_rvas()
    allf = load_symbol_functions()
    name_at = {rva: nm for rva, _sz, _u, nm in allf}

    targets = {}
    for rva, (owner, host, label) in exiles.items():
        m = name_at.get(rva)
        # ledger names are source-file stems; the pin location / units.toml map
        # them to the unit names the derived order uses.
        targets[rva] = (exile_owner.get(rva, stem2unit.get(owner, owner)),
                        stem2unit.get(host, host), label, m,
                        _mangled_class(m) if m else None)
    want_syms = {m for _o, _h, _l, m, _c in targets.values() if m}
    want_classes = {c for *_x, c in targets.values() if c}

    refs = defaultdict(set)       # mangled -> {unit referencing it (undefined)}
    inst = defaultdict(set)       # class -> {unit referencing ??_7/??0/??1 of it}
    for obj in sorted(BASE_OBJS.glob("*.obj")):
        res = subprocess.run(["llvm-nm", str(obj)], capture_output=True, text=True)
        unit = obj.stem
        for ln in res.stdout.splitlines():
            p = ln.split()
            if len(p) < 2:
                continue
            kind, sym = p[-2], p[-1]
            if sym in want_syms and kind == "U":
                refs[sym].add(unit)
            if sym.startswith(("??_7", "??0", "??1")):
                c = _mangled_class(sym)
                if c in want_classes:
                    inst[c].add(unit)

    ok = near = far = 0
    print(f"{'rva':>10} {'owner':>22} {'ledger host':>18} {'predicted':>18}  verdict")
    for rva, (owner, host, label, m, cls) in sorted(targets.items()):
        cands = set(refs.get(m, ()))
        if use_class_refs and cls:
            cands |= inst.get(cls, set())
        cands.add(owner)
        cands = {u for u in cands if u in pos}          # only real link-line objs
        pred = min(cands, key=lambda u: pos[u]) if cands else None
        extra = ""
        if pred == host:
            ok += 1
            verdict = "OK"
        elif pred is not None and host in pos and abs(pos[pred] - pos[host]) <= 4:
            # the prediction lands in the same seam/knot as the ledger host - the
            # HOST COLUMN is the imprecise side (a sprawling/undecided partition),
            # not the rule
            near += 1
            verdict = "NEAR"
            extra = f"  (pred pos {pos[pred]}, host pos {pos[host]})"
        else:
            far += 1
            verdict = "MISS"
            if pred is not None and host in pos:
                extra = f"  (pred pos {pos.get(pred)}, host pos {pos.get(host)})"
            if pred == owner:
                extra += "  [owner-earlier: retail's owner obj did not emit it]"
        print(f"{rva:#010x} {owner:>22} {host:>18} {str(pred):>18}  {verdict}{extra}")
    print(f"\n[link-line] keeper rule: {ok} exact, {near} same-seam, {far} far "
          f"of {ok + near + far} ledger rows (first-arrival obj among "
          f"{'referencers+instantiators' if use_class_refs else 'referencers'}; "
          f"far rows = the emitter-set proxy diverges from retail, or the row)")
    return 0


# ---------------------------------------------------------------- report / main

def report():
    rows, owners, diag = derive()
    ncmd = sum(1 for r in rows if r.cls == "cmdline")
    nlib = sum(1 for r in rows if r.cls == "lib")
    nthunk = sum(1 for r in rows if r.cls == "cmdline" and r.evidence.startswith("thunk"))
    print(f"[link-line] {len(rows)} ordered bands: {ncmd} cmdline "
          f"({nthunk} thunk-proven, {ncmd - nthunk} position-inferred), {nlib} lib; "
          f"{len(owners)} comdat-owner units excluded")
    print(f"  object/library boundary: last cmdline band ends {diag['obj_end']:#x}, "
          f"first lib band starts {diag['lib_start']:#x}")
    print(f"  CRT-init corroboration: {diag['xcu_units']} units carry XCU slots, "
          f"{len(diag['xcu_inversions'])} order inversions")
    for a, b in diag["xcu_inversions"]:
        print(f"    XCU INVERSION: {a} vs {b}")
    if diag["conflicts"]:
        print(f"  CONFLICT (thunk-proven band above the lib boundary): "
              f"{diag['conflicts']}")
    il = sorted(diag["interleaves"])
    if il:
        print(f"  {len(il)} units in interleaving bands (order between the members "
              f"ambiguous - the tu-partition backlog)")
    if diag["multi"]:
        print(f"  multi-region units (conflated TUs, ordered by first region): "
              f"{', '.join(diag['multi'])}")

    def padstats(cls):
        pads, span = [], 0
        seq = [r for r in rows if r.cls == cls and not r.notes]
        for a, b in zip(seq, seq[1:]):
            if b.start > a.end and b.start - a.end < 0x4000:
                pads.append(b.start - a.end)
                span += b.start - a.start
        return pads, span
    for cls in ("cmdline", "lib"):
        pads, span = padstats(cls)
        if span:
            print(f"  {cls}: inter-band padding {sum(pads)}/{span} = "
                  f"{100 * sum(pads) / span:.2f}% over {len(pads)} adjacent seams "
                  f"(upper bound - unpinned tails count as padding)")
    print()
    for i, r in enumerate(rows):
        print(f"{i:3d} {r.start:8x}-{r.end:8x} {r.cls:8} {r.module:9} "
              f"{r.unit:30} {r.evidence:18} {r.notes}")
    print("\ncomdat-owner (no own contribution):")
    for r in owners:
        s = f"{r.start:8x}-{r.end:8x}" if r.start is not None else " " * 17
        print(f"    {s} {r.module:9} {r.unit:30} {r.evidence} {r.notes}")


def main(argv=None):
    ap = argparse.ArgumentParser(description="derive the retail link line")
    ap.add_argument("--emit", action="store_true", help=f"write {ORDER_TSV}")
    ap.add_argument("--check", action="store_true",
                    help="re-derive and compare against the committed file")
    ap.add_argument("--objlist", metavar="FILE",
                    help="write obj stems (every unit) in derived order, for "
                         "`gruntz link --order FILE`")
    ap.add_argument("--measure", metavar="MAP",
                    help="compare a candidate .map's cross-TU order to the derivation")
    ap.add_argument("--exiles", action="store_true",
                    help="predict each kept-COMDAT exile's host from the order")
    ap.add_argument("--no-class-refs", action="store_true",
                    help="--exiles: candidates from direct symbol referencers only")
    args = ap.parse_args(argv)

    if args.check:
        return check()
    if args.measure:
        return measure(Path(args.measure))
    if args.exiles:
        return predict_exiles(use_class_refs=not args.no_class_refs)
    if args.emit:
        rows, owners, _diag = derive()
        ORDER_TSV.write_text(fmt_rows(rows, owners))
        print(f"[link-line] wrote {ORDER_TSV} ({len(rows)} ordered + "
              f"{len(owners)} comdat-owner rows)")
        return 0
    if args.objlist:
        text = objlist_text()
        Path(args.objlist).write_text(text)
        n = len(text.splitlines()) - 1
        print(f"[link-line] wrote {args.objlist} ({n} objs)")
        return 0
    report()
    return 0


if __name__ == "__main__":
    sys.exit(main())
