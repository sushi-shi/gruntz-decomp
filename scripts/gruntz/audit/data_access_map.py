#!/usr/bin/env python3
"""gruntz.audit.data_access_map - build and QUERY the retail data-access map.

The instrument for the question the match score structurally cannot answer.
We choose the extent of every datum we claim, so a too-small claim ALWAYS
scores 100: objdiff only compares what we told it to compare. This map works
from RETAIL's side - which bytes does retail's code actually touch, and how
wide - so a silently unmodelled field, a wrong width, or a phantom object shows
up as a disagreement between what retail does and what we declared.

Engine + schema: gruntz.core.access_map. The artifacts are
build/gen/data_access_map.sqlite (the query index) and
build/gen/data_access_map.tsv (the grep-able access table).

BUILD
  gruntz audit data_access_map --build            # sweep -> sqlite + tsv + findings

QUERY (all read-only against the persisted map)
  --at 0x4d2a44             every access/cell touching one address
  --range 0x4d2a00:0x4d2b00 every access in a byte range
  --symbol <name|0xrva>     one claim: field map, per-offset accesses, findings
  --fn <name|0xrva>         every data access one function makes
  --findings [category]     the derived worklist
  --sql "SELECT ..."        raw sqlite over the schema

DERIVED CATEGORIES (--build computes them; --findings reads them back)
  unclaimed      retail accesses bytes no DATA() claim covers -> unmodelled data
  unaccessed     a claim nothing in the image references -> phantom candidate
  width          access width disagrees with the declared field -> wrong type
  stride         an index scale inside a claim disagrees with its element size
  adjacent       two claims reached through ONE base register -> one object

CALIBRATION
  --calibrate    run the sieve over the control set (claims in data sections at
                 exactly 100.0 whose declared type fully resolves) and print the
                 flag rate, so a finding count is reported WITH its denominator
  --selftest     inject known defects (a narrowed width, a float declared int, a
                 halved extent, a wrong element size) into the type oracle and
                 assert the sieve reports each - a sieve that returns 0 rows
                 while blind is the failure mode this control exists to catch
"""
import argparse
import bisect
import json
from pathlib import Path
from collections import Counter, defaultdict

from gruntz.core import get_context
from gruntz.core.access_map import (SQLITE, TSV, Claim, Types, build_claims,
                                    connect, persist, sweep)
from gruntz.core.pe import REPO

STRUCTS = REPO / "build/gen/structs.json"
REPORT = REPO / "build/objdiff/report.json"

# forms that TOUCH bytes (as opposed to taking an address or holding a pointer)
TOUCH = ("direct", "indexed", "derived-disp")
# forms that merely reference the object without reading/writing it here
REFER = ("lea", "imm", "indcall", "iat")


# --- section match %, for the calibration control set -------------------------
def section_pct():
    """{(unit, section): fuzzy%} from the objdiff report."""
    out = {}
    if not REPORT.is_file():
        return out
    for u in json.loads(REPORT.read_text()).get("units", []):
        for s in u.get("sections", []):
            if s["name"] != ".text":
                out[(u["name"], s["name"])] = float(
                    s.get("fuzzy_match_percent") or 0.0)
    return out


def attach_pct(claims):
    pct = section_pct()
    for c in claims:
        c.pct = pct.get((c.unit, c.section))
    return claims


# --- the five derived analyses ------------------------------------------------
def _fieldmap(claim):
    """[(off, size, path, type)] sorted, for offset->field resolution."""
    return sorted(claim.fields)


def _synth_field_at(c, off):
    """Field lookup over an INJECTED (self-test) field list - the synthetic
    layouts the injector builds are not derivable from a type name."""
    fs = sorted(c.fields)
    hit = None
    for f in fs:
        if f[0] <= off < f[0] + f[1]:
            hit = f
    if hit is None:
        return ("hole", off, 0, "", c.type, 1)
    return hit


def _field_at(fields, offs, off):
    k = bisect.bisect_right(offs, off) - 1
    if k < 0:
        return None
    f = fields[k]
    return f if off < f[0] + f[1] else None


def _run_kind(pe, ctx, run, in_idata):
    """Triage an unclaimed run so the worklist is not drowned by data that can
    never take a DATA() pin. Only `data` is the real worklist."""
    from gruntz.core.symbols import owner
    db = ctx.symbols
    acc = run["acc"]
    if in_idata(run["start"]):
        return "idata"                        # import thunk slots: the linker's
    if all(a.fpu.startswith("f") for a in acc if a.fpu) and \
            any(a.fpu for a in acc) and not any("w" in a.rw for a in acc):
        return "fp-pool"                      # an unpinnable x87 constant pool
    o = pe.off(run["start"])
    blob = pe.data[o:o + min(24, run["end"] - run["start"])] if o else b""
    printable = bool(blob) and all(32 <= b < 127 or b in (0, 9, 10, 13)
                                   for b in blob)
    if printable and not any("w" in a.rw for a in acc):
        return "string-pool"                  # pooled literals (inline strcmp)
    units = {db.names.get(owner(a.insn_rva, db.fstarts, db.fsize),
                          (None, None))[1] for a in acc}
    units.discard(None)
    if units and all(u in ("ghidra", "retail") for u in units):
        return "library"
    return "data"


def derive_findings(ctx, types, accesses, cells, claims, quiet=True):
    """(rows, stats) - the five derived categories, each row a sqlite `finding`.

    Every suppression below is a MEASURED false-positive class, named so the
    next reader can re-argue it; nothing is filtered because it was noisy."""
    pe = ctx.pe
    starts = [c.rva for c in claims]
    by_rva = {c.rva: c for c in claims}
    idata = next(((va, va + max(vsz, rsz)) for name, va, vsz, rp, rsz in pe.secs
                  if name == ".idata"), (0, 0))

    def in_idata(rva):
        return idata[0] <= rva < idata[1]

    rows = []
    st = Counter()

    def locate(rva):
        k = bisect.bisect_right(starts, rva) - 1
        if k < 0:
            return None, -1
        c = claims[k]
        return (c, rva - c.rva) if rva < c.end else (None, -1)

    # index the accesses by owning claim ONCE (the per-claim loops below are
    # otherwise O(claims x accesses) and the map has ~24k of each)
    per = defaultdict(list)
    unclaimed = defaultdict(list)
    for ac in accesses:
        c, off = locate(ac.target_rva)
        if c is None:
            if ac.form in TOUCH:
                unclaimed[ac.target_rva].append(ac)
        else:
            per[c.rva].append(ac)

    # ---- 1. accessed but unclaimed -> unmodelled data ----------------------
    runs, cur = [], None
    for rva in sorted(unclaimed):
        w = max((a.width or 4) for a in unclaimed[rva])
        if cur and rva - cur["end"] <= 8:
            cur["end"] = max(cur["end"], rva + w)
            cur["acc"] += unclaimed[rva]
        else:
            cur = {"start": rva, "end": rva + w, "acc": list(unclaimed[rva])}
            runs.append(cur)
    for r in runs:
        kind = _run_kind(pe, ctx, r, in_idata)
        st[f"unclaimed-{kind}"] += 1
        if kind != "data":
            continue
        k = bisect.bisect_right(starts, r["start"]) - 1
        prev = claims[k] if k >= 0 else None
        widths = Counter(a.width for a in r["acc"] if a.width)
        wrote = any("w" in a.rw for a in r["acc"])
        rows.append(("unclaimed", "high" if wrote else "med",
                     prev.rva if prev else 0, prev.name if prev else "",
                     r["start"],
                     f"{r['end'] - r['start']} B accessed by {len(r['acc'])} "
                     f"site(s){' incl. a WRITE' if wrote else ''}, no DATA() "
                     f"claim covers it",
                     " ".join(f"w{w}x{n}" for w, n in sorted(widths.items()))
                     + (f"  {r['start'] - prev.end} B past the end of "
                        f"{prev.name}" if prev and r["start"] - prev.end < 0x100
                        else f"  in {pe.sec_name(r['start']) or '?'}, no claim "
                             f"within 0x{r['start'] - prev.end:x} B behind it"
                        if prev else "")))
        st["unclaimed"] += 1

    # ---- 2. claimed but never referenced -> phantom candidates -------------
    pointed = set()
    for cl in cells:
        c, off = locate(cl["target"])
        if c is not None:
            pointed.add(c.rva)
        s, soff = locate(cl["site"])          # a claim whose OWN bytes relocate
        if s is not None:
            pointed.add(s.rva)
    for c in claims:
        if per[c.rva] or c.rva in pointed:
            continue
        rows.append(("unaccessed", "high" if c.section != ".idata" else "low",
                     c.rva, c.name, c.rva,
                     f"nothing in the image reads, writes, takes the address of "
                     f"or points at it (extent 0x{c.extent:x} {c.extent_src})",
                     f"unit={c.unit} section={c.section} type={c.type or '?'}"))
        st["unaccessed"] += 1

    # ---- 3. access width vs the declared field -> wrong type ---------------
    nxt_start = {c.rva: (claims[i + 1].rva if i + 1 < len(claims) else None)
                 for i, c in enumerate(claims)}
    # a `mov [obj+N],&??_7...` is a vptr STAMP, so +N is a base sub-object
    # boundary, not an unmodelled member. structs.json carries no vptr at all -
    # primary or MI-secondary - so this is our blind spot, not a layout bug.
    vtbl = {c.rva for c in claims if c.name.startswith("??_7")}
    stamps = {ac.insn_rva for ac in accesses
              if ac.form in ("imm", "lea") and ac.target_rva in vtbl}

    def is_vptr_stamp(acs):
        return bool(acs) and all(a.width == 4 and "w" in a.rw
                                 and a.insn_rva in stamps for a in acs)
    for c in claims:
        if not c.fields or not per[c.rva]:
            continue
        seen = defaultdict(lambda: [Counter(), Counter(), Counter(), Counter(),
                                    Counter()])
        acc_at = defaultdict(list)
        for ac in per[c.rva]:
            if ac.form not in TOUCH or not ac.width:
                continue
            off = ac.target_rva - c.rva
            acc_at[off].append(ac)
            seen[off][0][ac.width] += 1
            if ac.fpu:
                seen[off][1][ac.fpu] += 1
            seen[off][2][(ac.width, ac.rw)] += 1
            seen[off][3][ac.form] += 1
            seen[off][4][(ac.width, ac.ext)] += 1
        for off, (widths, fpus, rws, forms, exts) in sorted(seen.items()):
            # "does retail STORE fewer bytes than the field" must be asked of
            # the NARROW access itself: a 4-byte store plus a 2-byte read is
            # `(u16)x`, not evidence of a u16 field
            def stores(w):
                return any("w" in rw for (ww, rw) in rws if ww == w)
            ev = " ".join(f"w{w}x{n}" for w, n in sorted(widths.items()))
            nxt = nxt_start.get(c.rva)
            if set(forms) == {"indexed"} and nxt is not None \
                    and 0 < nxt - (c.rva + off) <= 4:
                # `[reg + &next - k]` is the negative-addend spelling of the
                # FOLLOWING symbol (a 1-based index into the next array), not an
                # access to this claim - assert_relocs knows the same idiom
                st["width-skip-negative-addend"] += 1
                continue
            f = (_synth_field_at(c, off) if c.extent_src == "INJECTED"
                 and c.fields else types.field_at(c.type, off))
            if f is None:
                st["width-skip-unresolved"] += 1
                continue
            if f[0] == "vptr" or is_vptr_stamp(acc_at[off]):
                # structs.json omits the vptr of every polymorphic class - the
                # primary one before the first declared field, and every
                # MI-secondary one at a base sub-object boundary
                st["width-skip-vptr"] += 1
                continue
            if f[0] in ("out", "hole"):
                rows.append(("width", "high", c.rva, c.name, c.rva + off,
                             f"+0x{off:x} accessed but no declared field of "
                             f"{c.type or '?'} covers that offset "
                             f"({'past the type' if f[0] == 'out' else 'a HOLE between members'})",
                             ev))
                st["width"] += 1
                continue
            foff, fsz, path, fty, resolved = f
            if not resolved:
                st["width-skip-unresolved"] += 1
                continue                      # never accuse through an unknown
            if off != foff:
                # FP class: an 8-byte scalar copied as two dwords - MSVC5 moves
                # a double/i64 constant with a pair of dword loads, so a 4-byte
                # access at +4 of an 8-byte field is the copy, not a layout bug
                if fsz == 8 and off == foff + 4 and set(widths) == {4} \
                        and not fpus:
                    st["width-skip-dword-pair"] += 1
                    continue
                rows.append(("width", "high", c.rva, c.name, c.rva + off,
                             f"+0x{off:x} lands INSIDE field {path or '.'} "
                             f"(+0x{foff:x} {fty}, {fsz} B) - layout is wrong",
                             ev))
                st["width"] += 1
                continue
            wmax, wmin = max(widths), min(widths)
            elem = "[" in path              # an ARRAY element, not a scalar
            pair = fsz == 8 and set(widths) == {4} and not fpus \
                and (off + 4) in seen and set(seen[off + 4][0]) == {4}
            if wmax > fsz:
                # FP class: a wider access on a byte-ARRAY element is the
                # inlined CRT block move/compare (rep movsd over a char buffer).
                # A byte SCALAR read 4 bytes wide is a type error, not a block op.
                if elem and types.is_byteish(fty):
                    st["width-skip-byte-buffer"] += 1
                elif fsz == 4 and wmax == 8 and not fpus:
                    st["width-skip-dword-pair"] += 1
                elif exts[(wmax, f"m{fsz}")] == widths[wmax]:
                    # cl 5.0 loads a narrow global with a FULL-WIDTH read and
                    # masks the register (movzx was slow on the Pentium), so
                    # every over-wide access here is a `fsz`-byte one in disguise
                    st["width-skip-and-mask"] += 1
                elif not stores(wmax) and stores(fsz):
                    # the same movzx-avoidance, caught by its STORE side: the
                    # mask can sit past a branch or behind a register copy, but
                    # nobody writes a wmax-byte object only fsz bytes at a time
                    st["width-skip-and-mask"] += 1
                else:
                    rows.append(("width", "high", c.rva, c.name, c.rva + off,
                                 f"+0x{off:x} {path or '.'} declared {fty} "
                                 f"({fsz} B), retail accesses {wmax} B", ev))
                    st["width"] += 1
            elif pair:
                # MSVC5 copies an 8-byte constant with a pair of dword loads
                st["width-skip-dword-pair"] += 1
            elif wmin < fsz and types.is_float(fty) and not fpus:
                rows.append(("width", "high", c.rva, c.name, c.rva + off,
                             f"+0x{off:x} {path or '.'} declared {fty} ({fsz} B) "
                             f"but retail accesses {wmin} B with integer ops and "
                             f"never touches it with x87", ev))
                st["width"] += 1
            elif wmin < fsz and not types.is_ptr(fty) and not elem \
                    and stores(wmin):
                # a sub-field STORE is strong: nobody writes half a scalar
                rows.append(("width", "med", c.rva, c.name, c.rva + off,
                             f"+0x{off:x} {path or '.'} declared {fty} ({fsz} B) "
                             f"but retail STORES {wmin} B", ev))
                st["width"] += 1
            if any(t.startswith("f") for t in fpus) and not types.is_float(fty):
                rows.append(("width", "high", c.rva, c.name, c.rva + off,
                             f"+0x{off:x} {path or '.'} declared {fty} but retail "
                             f"uses an x87 FLOAT access ({'/'.join(sorted(fpus))})",
                             ev))
                st["width"] += 1
            if any(t.startswith("i") for t in fpus) and types.is_float(fty):
                rows.append(("width", "high", c.rva, c.name, c.rva + off,
                             f"+0x{off:x} {path or '.'} declared {fty} but retail "
                             f"uses an x87 INTEGER access "
                             f"({'/'.join(sorted(fpus))})", ev))
                st["width"] += 1

    # ---- 4. stride evidence inside a claim -> wrong element size -----------
    for c in claims:
        scales = Counter()
        for ac in per[c.rva]:
            if ac.form == "indexed" and ac.scale and ac.target_rva == c.rva:
                scales[ac.scale] += 1
        if not scales:
            continue
        dims, base = types.dims(c.type) if c.type else ([], "")
        elem = types.base_size(base) if c.type else None
        for sc, n in sorted(scales.items()):
            if elem is None:
                rows.append(("stride", "med", c.rva, c.name, c.rva,
                             f"indexed by *{sc} but the claim's element type "
                             f"does not resolve", f"type={c.type or '?'} sites={n}"))
                st["stride"] += 1
            elif not dims and sc != elem:
                rows.append(("stride", "high", c.rva, c.name, c.rva,
                             f"indexed by *{sc} but declared as the SCALAR "
                             f"{c.type} ({elem} B) - retail treats it as a table",
                             f"sites={n}"))
                st["stride"] += 1
            elif dims and sc > elem:
                rows.append(("stride", "high", c.rva, c.name, c.rva,
                             f"indexed by *{sc} but the declared element {base} "
                             f"is only {elem} B - the element is {sc // elem}x "
                             f"too small (a pair/record, not a scalar array)",
                             f"type={c.type} sites={n}"))
                st["stride"] += 1
            elif dims and sc < elem and elem % sc == 0:
                # `[i*4 + base + k]` inside a 12-byte record: MSVC scales the
                # index by the DWORD, not by the element. Benign, counted only.
                st["stride-skip-subelement"] += 1
            elif dims and sc != elem:
                rows.append(("stride", "high", c.rva, c.name, c.rva,
                             f"indexed by *{sc}, declared element {base} is "
                             f"{elem} B - neither divides the other",
                             f"type={c.type} sites={n}"))
                st["stride"] += 1

    # ---- 5. two claims that are really ONE object --------------------------
    # 5a a single access whose byte RANGE crosses a claim boundary: retail
    #    reads both claims with one instruction, so they are one datum
    spans = Counter()
    for ac in accesses:
        if ac.form not in TOUCH or not ac.width:
            continue
        a, aoff = locate(ac.target_rva)
        if a is None or ac.target_rva + ac.width <= a.end:
            continue
        b, boff = locate(a.end)
        if b is None or b.rva == a.rva:
            continue
        spans[(a.rva, b.rva, ac.width)] += 1
    for (arva, brva, w), n in sorted(spans.items(), key=lambda kv: -kv[1]):
        a, b = by_rva[arva], by_rva[brva]
        rows.append(("adjacent", "high", arva, a.name, brva,
                     f"one {w}-byte access at {a.name} runs past its 0x"
                     f"{a.extent:x}-byte extent into {b.name} - one object",
                     f"other=0x{brva:x} {b.name} sites={n}"))
        st["adjacent"] += 1
    # 5b a derived `[reg+disp]` whose base register held claim A but whose
    #    target lands in claim B: retail addresses both from one base
    joins = Counter()
    for ac in accesses:
        if ac.form != "derived-disp" or not ac.disp:
            continue
        a, aoff = locate(ac.target_rva - ac.disp)
        b, boff = locate(ac.target_rva)
        if a is None or b is None or a.rva == b.rva:
            continue
        joins[(a.rva, b.rva)] += 1
    for (arva, brva), n in sorted(joins.items(), key=lambda kv: -kv[1]):
        a, b = by_rva[arva], by_rva[brva]
        rows.append(("adjacent", "high" if b.rva == a.end else "med", arva,
                     a.name, brva,
                     f"reached as {a.name}+0x{brva - arva:x} through one base "
                     f"register - {'contiguous' if b.rva == a.end else 'gapped'}",
                     f"other=0x{brva:x} {b.name} sites={n}"))
        st["adjacent"] += 1
    return rows, st


# --- build --------------------------------------------------------------------
def do_build(args):
    ctx = get_context()
    types = Types(STRUCTS)
    accesses, cells, stats = sweep(ctx)
    claims = attach_pct(build_claims(types, ctx.pe))
    findings, fstats = derive_findings(ctx, types, accesses, cells, claims)
    na, nc = persist(ctx, types, accesses, cells, claims, stats,
                     args.sqlite, args.tsv, findings)
    touch = sum(1 for a in accesses if a.form in TOUCH)
    refer = sum(1 for a in accesses if a.form in REFER)
    print(f"[access-map] {na} references, {nc} pointer cells, {len(claims)} claims "
          f"-> {args.sqlite}")
    print(f"[access-map] byte-touching {touch}, address-taking {refer}")
    print("[access-map] forms: " + ", ".join(
        f"{k[5:]}={v}" for k, v in sorted(stats.items()) if k.startswith("form-")))
    print("[access-map] out of scope: " + ", ".join(
        f"{k}={v}" for k, v in sorted(stats.items()) if k.startswith("to-")))
    print("[access-map] cells: " + ", ".join(
        f"{k[5:]}={v}" for k, v in sorted(stats.items()) if k.startswith("cell-")))
    print("[access-map] findings: " + (", ".join(
        f"{k}={v}" for k, v in sorted(fstats.items())) or "none"))
    print_coverage(ctx, accesses, stats)
    return 0


def print_coverage(ctx, accesses, stats):
    """State what the map sees and what it structurally cannot.

    The .reloc table is a COMPLETE index of absolute data references, so the
    absolute half of the map is exhaustive by construction. Register-relative
    accesses carry no relocation: only the ones whose base was loaded from an
    absolute operand in the same basic block are recoverable, and the rest are
    invisible. The escape counts below bound that blind region - they are the
    number of times an object's ADDRESS leaves the block without our being able
    to follow it."""
    esc = Counter()
    for a in accesses:
        if a.form not in ("imm", "lea"):
            continue
        m = (a.text or "").split(None, 1)[0] if a.text else "?"
        if m == "push":
            esc["push (call argument)"] += 1
        elif m == "mov" and a.text and "PTR" in a.text.split(",")[0]:
            esc["stored into memory"] += 1
        elif m in ("mov", "lea"):
            esc["loaded into a register"] += 1
        else:
            esc[m] += 1
    touch = sum(1 for a in accesses if a.form in TOUCH)
    derived = sum(1 for a in accesses if a.form == "derived-disp")
    print("[coverage] SEEN - reloc-anchored byte accesses: "
          f"{touch - derived} (exhaustive: every absolute operand is relocated)")
    print(f"[coverage] SEEN - register-relative recovered by provenance: {derived}")
    print("[coverage] BLIND - address escapes we cannot follow: " + ", ".join(
        f"{k}={v}" for k, v in esc.most_common()))
    print(f"[coverage] BLIND - register loads handed straight to a callee: "
          f"{stats.get('seed-handed-to-callee', 0)} of "
          f"{stats.get('seed-total', 0)} seeds (the callee's field accesses are "
          f"`this`-relative)")
    print("[coverage] BLIND - structurally invisible classes: `this`-relative "
          "field accesses inside a callee; any access through a pointer loaded "
          "FROM memory; a member SWAP between two same-sized members (no width "
          "difference exists to observe)")
    print(f"[coverage] undecodable .text reloc sites: "
          f"{stats.get('form-undecoded', 0)} "
          f"(+{stats.get('cell-in-text', 0)} reloc cells that are DATA living "
          f"in .text, not instruction operands)")


TOUCHED = REPO / "build/gen/data_touched_ranges.tsv"


def do_touched(args):
    """Emit the coalesced byte ranges retail's code TOUCHES.

    The interface for the completeness lane, which computes the complementary
    set (which bytes no claim of ours covers) from our side. Neither set alone
    distinguishes padding from an unmodelled field; intersected they do:

        uncovered AND touched   -> unmodelled data
        uncovered AND untouched -> padding

    One row per maximal run of bytes reached by at least one byte-touching
    access, with the widest access over it, the read/write split and the
    claim (if any) it falls in. Address-taking references are NOT included -
    they prove the object is used, not which of its bytes are."""
    con = connect(args.sqlite)
    rows = con.execute(
        "SELECT target_rva, end_rva, width, rw, form, sym_rva, sym_name "
        "FROM access WHERE width>0 AND form IN ('direct','indexed',"
        "'derived-disp') ORDER BY target_rva").fetchall()
    out, cur = [], None
    for r in rows:
        if cur and r["target_rva"] <= cur["end"]:
            cur["end"] = max(cur["end"], r["end_rva"])
        else:
            cur = {"start": r["target_rva"], "end": r["end_rva"], "sites": 0,
                   "reads": 0, "writes": 0, "maxw": 0, "forms": set(),
                   "sym": r["sym_name"], "sym_rva": r["sym_rva"]}
            out.append(cur)
        cur["sites"] += 1
        cur["reads"] += "r" in r["rw"]
        cur["writes"] += "w" in r["rw"]
        cur["maxw"] = max(cur["maxw"], r["width"])
        cur["forms"].add(r["form"])
    args.touched.parent.mkdir(parents=True, exist_ok=True)
    with args.touched.open("w") as f:
        f.write("start\tend\tbytes\tsites\treads\twrites\tmax_width\t"
                "forms\tclaim_rva\tclaim\n")
        for r in out:
            f.write(f"0x{r['start']:x}\t0x{r['end']:x}\t{r['end'] - r['start']}\t"
                    f"{r['sites']}\t{r['reads']}\t{r['writes']}\t{r['maxw']}\t"
                    f"{','.join(sorted(r['forms']))}\t"
                    f"{'0x%x' % r['sym_rva'] if r['sym_rva'] else ''}\t"
                    f"{r['sym'] or ''}\n")
    total = sum(r["end"] - r["start"] for r in out)
    print(f"[access-map] {len(out)} touched ranges, {total} bytes -> "
          f"{args.touched}")
    return 0


# --- queries ------------------------------------------------------------------
def _hex(v):
    return f"0x{v:x}" if isinstance(v, int) else str(v)


def _print_accesses(rows, base=None):
    for r in rows:
        off = f"+0x{r['target_rva'] - base:<5x}" if base is not None else \
            f"0x{r['target_rva']:08x}"
        w = f"w{r['width']}" if r["width"] else "addr"
        tag = r["fpu"] or r["ext"] or ""
        print(f"  0x{r['insn_rva']:06x} {r['form']:12} {r['rw']:2} {w:<5} "
              f"{tag:4} {off} {(r['text'] or '')[:46]:46} "
              f"{r['fn_name'] or '<gap>'}")


def do_at(con, rva):
    a = con.execute(
        "SELECT * FROM access WHERE target_rva<=? AND (target_rva+width)>? "
        "OR (width=0 AND target_rva=?) ORDER BY insn_rva", (rva, rva, rva)).fetchall()
    print(f"address 0x{rva:x}: {len(a)} reference(s)")
    c = con.execute("SELECT * FROM claim WHERE rva<=? AND rva+extent>? ",
                    (rva, rva)).fetchone()
    if c:
        print(f"  claim {c['name']} [{c['unit']}] 0x{c['rva']:x} "
              f"+0x{c['extent']:x} ({c['extent_src']}) type={c['type'] or '?'} "
              f"offset +0x{rva - c['rva']:x}")
        f = con.execute("SELECT * FROM field WHERE sym_rva=? AND off<=? AND "
                        "off+size>? ", (c["rva"], rva - c["rva"], rva - c["rva"])
                        ).fetchone()
        if f:
            print(f"  field {f['path'] or '.'} +0x{f['off']:x} {f['type']} "
                  f"{f['size']} B")
    else:
        print("  claim: NONE - unclaimed byte")
    _print_accesses(a)
    for cl in con.execute("SELECT * FROM cell WHERE target_rva=? OR site_rva=?",
                          (rva, rva)):
        print(f"  cell @0x{cl['site_rva']:x} ({cl['where_sec']}) {cl['kind']} "
              f"-> 0x{cl['target_rva']:x} {cl['tgt_sym_name'] or ''}")
    return 0


def do_range(con, lo, hi):
    a = con.execute("SELECT * FROM access WHERE target_rva>=? AND target_rva<? "
                    "ORDER BY target_rva, insn_rva", (lo, hi)).fetchall()
    print(f"range 0x{lo:x}..0x{hi:x}: {len(a)} reference(s)")
    _print_accesses(a)
    return 0


def _resolve_claim(con, key):
    if key.startswith("0x"):
        return con.execute("SELECT * FROM claim WHERE rva=?",
                           (int(key, 16),)).fetchone()
    r = con.execute("SELECT * FROM claim WHERE name=?", (key,)).fetchone()
    if r:
        return r
    return con.execute("SELECT * FROM claim WHERE name LIKE ?",
                       (f"%{key}%",)).fetchone()


def do_symbol(con, key):
    c = _resolve_claim(con, key)
    if c is None:
        print(f"no claim matching {key}")
        return 1
    print(f"{c['name']}  [{c['unit']}]  0x{c['rva']:x} +0x{c['extent']:x} "
          f"({c['extent_src']})  {c['section']} "
          f"{'%.2f%%' % c['sect_pct'] if c['sect_pct'] >= 0 else 'n/a'}")
    print(f"  type: {c['type'] or '?'}")
    print(f"  accesses={c['n_access']} (r{c['n_read']}/w{c['n_write']}) "
          f"address-taken={c['n_addr']} reloc-cells-inside={c['n_cells']}")
    flds = con.execute("SELECT * FROM field WHERE sym_rva=? ORDER BY off",
                       (c["rva"],)).fetchall()
    acc = con.execute("SELECT * FROM access WHERE sym_rva=? AND in_extent=1 "
                      "ORDER BY sym_off, insn_rva", (c["rva"],)).fetchall()
    hits = defaultdict(Counter)
    for a in acc:
        if a["form"] in TOUCH:
            hits[a["sym_off"]][a["width"]] += 1
    if flds:
        print(f"  field map ({len(flds)}):")
        for f in flds[:64]:
            h = hits.get(f["off"])
            mark = ("  <- " + " ".join(f"w{w}x{n}" for w, n in sorted(h.items()))
                    if h else "")
            print(f"    +0x{f['off']:<5x} {f['size']:<3} {f['type']:<16} "
                  f"{f['path'] or '.'}{mark}")
        if len(flds) > 64:
            print(f"    ... {len(flds) - 64} more")
    untouched = sorted(set(hits) - {f["off"] for f in flds})
    if untouched:
        print("  accessed offsets with NO declared field: " +
              " ".join(f"+0x{o:x}" for o in untouched[:24]))
    print(f"  {len(acc)} reference(s):")
    _print_accesses(acc, c["rva"])
    fnd = con.execute("SELECT * FROM finding WHERE sym_rva=?",
                      (c["rva"],)).fetchall()
    for f in fnd:
        print(f"  [{f['category']}/{f['severity']}] 0x{f['addr']:x} "
              f"{f['detail']}  ({f['evidence']})")
    return 0


def do_fn(con, key):
    if key.startswith("0x"):
        rows = con.execute("SELECT * FROM access WHERE fn_rva=? ORDER BY insn_rva",
                           (int(key, 16),)).fetchall()
    else:
        rows = con.execute("SELECT * FROM access WHERE fn_name LIKE ? "
                           "ORDER BY insn_rva", (f"%{key}%",)).fetchall()
    print(f"{key}: {len(rows)} data reference(s)")
    for r in rows:
        w = f"w{r['width']}" if r["width"] else "addr"
        off = f"+0x{r['sym_off']:x}" if r["in_extent"] else ""
        print(f"  0x{r['insn_rva']:06x} {r['form']:12} {r['rw']:2} {w:<5} "
              f"0x{r['target_rva']:08x} "
              f"{(r['sym_name'] if r['in_extent'] else '<unclaimed>')[:44]:44} "
              f"{off}")
    return 0


def do_findings(con, cat, limit):
    q = "SELECT * FROM finding"
    args = ()
    if cat:
        q += " WHERE category=?"
        args = (cat,)
    q += " ORDER BY category, severity DESC, sym_rva"
    rows = con.execute(q, args).fetchall()
    by = Counter((r["category"], r["severity"]) for r in rows)
    print(f"{len(rows)} finding(s): " +
          ", ".join(f"{c}/{s}={n}" for (c, s), n in sorted(by.items())))
    for r in rows[:limit]:
        print(f"  [{r['category']}/{r['severity']}] 0x{r['addr']:08x} "
              f"{(r['sym_name'] or '-')[:48]:48} {r['detail']}")
        if r["evidence"]:
            print(f"      {r['evidence']}")
    if len(rows) > limit:
        print(f"  ... {len(rows) - limit} more (raise --limit)")
    return 0


# --- calibration --------------------------------------------------------------
def do_calibrate(args):
    """Measure the sieve's flag rate against a control set, with the denominator.

    CONTROL SET: claims that live in a data section objdiff scores at exactly
    100.0, whose declared type fully resolves, and that retail actually
    accesses. Those bytes are byte-identical to retail, so nothing about their
    CONTENT can be wrong.

    What that does and does NOT prove, stated precisely, because getting this
    wrong is how a sieve gets believed:
      * It does NOT make a `width` finding a false positive. A section at 100.0
        means the BYTES match; the declared TYPE can still be wrong, and that is
        the entire premise of this map. So every control-set finding has to be
        ADJUDICATED against the retail disassembly by hand - the number below is
        a FLAG RATE, and the report states the adjudicated split.
      * It DOES bound the noise: a sieve that flags a large fraction of
        byte-exact, fully-typed claims is measuring its own bugs. Two such bugs
        were found and fixed this way (a narrow READ counted as a STORE, and an
        array flattening cap that made every offset past 2048 look unmodelled).

    `unclaimed` is reported separately and split by whether the run begins
    exactly at a control claim's END (an extent claim about that claim) or
    somewhere else (a claim about nobody's claim), because attributing an
    unclaimed run to the nearest preceding symbol would inflate the rate."""
    ctx = get_context()
    types = Types(STRUCTS)
    accesses, cells, stats = sweep(ctx)
    claims = attach_pct(build_claims(types, ctx.pe))
    findings, fstats = derive_findings(ctx, types, accesses, cells, claims)

    exact = {c.rva for c in claims if c.pct is not None and c.pct >= 100.0}
    resolved = {c.rva for c in claims if c.type and types.sizeof(c.type) is not None}
    starts = [c.rva for c in claims]
    ends = {c.end: c for c in claims}
    ntouch = Counter()
    for a in accesses:
        if a.form not in TOUCH:
            continue
        k = bisect.bisect_right(starts, a.target_rva) - 1
        if k >= 0 and a.target_rva < claims[k].end:
            ntouch[claims[k].rva] += 1
    control = {r for r in (exact & resolved) if ntouch[r]}

    print(f"[calibrate] claims                              {len(claims)}")
    print(f"[calibrate]   in a data section at exactly 100.0  {len(exact)}")
    print(f"[calibrate]   declared type fully resolves        {len(resolved)}")
    print(f"[calibrate]   retail accesses it                  "
          f"{sum(1 for r in ntouch if ntouch[r])}")
    print(f"[calibrate]   CONTROL SET (all three)             {len(control)}")

    direct = defaultdict(list)                # findings ABOUT a control claim
    contig, elsewhere = [], []
    for f in findings:
        cat, sev, srva, sname, addr, detail, ev = f
        if cat == "unclaimed":
            owner = ends.get(addr)
            if owner is not None and owner.rva in control:
                contig.append(f)
            else:
                elsewhere.append(f)
        elif srva in control:
            direct[cat].append(f)
    n = sum(len(v) for v in direct.values())
    print(f"[calibrate] TYPE findings on the control set: {n} over "
          f"{len(control)} claims = {100.0 * n / max(len(control), 1):.2f}%")
    for cat, v in sorted(direct.items()):
        print(f"[calibrate]   {cat:11} {len(v):4} on "
              f"{len({f[2] for f in v})} claim(s)")
    print(f"[calibrate] unclaimed runs starting exactly at a control claim's "
          f"end: {len(contig)}  (elsewhere, not attributable: {len(elsewhere)})")
    for cat, v in sorted(direct.items()):
        for f in v[:args.limit]:
            print(f"    [{cat}/{f[1]}] 0x{f[4]:08x} {f[3][:50]:50} {f[5]}")
            if f[6]:
                print(f"        {f[6]}")
    for f in contig[:args.limit]:
        print(f"    [unclaimed-contiguous] 0x{f[4]:08x} {f[3][:50]:50} {f[5]}")
    return 0


def _dead_space(pe, claims, accesses, cells):
    """An address in .data that no claim covers and nothing in the image
    references - where a planted phantom must show up as unaccessed."""
    hot = {a.target_rva for a in accesses} | {c["target"] for c in cells} \
        | {c["site"] for c in cells}
    sec = next((s for s in pe.secs if s[0] == ".data"), None)
    lo, hi = sec[1], sec[1] + max(sec[2], sec[4])
    ends = sorted((c.rva, c.end) for c in claims)
    import bisect as _b
    starts = [e[0] for e in ends]
    for rva in range(hi - 0x400, lo, -0x40):
        k = _b.bisect_right(starts, rva) - 1
        if k >= 0 and rva < ends[k][1]:
            continue
        if any((rva + d) in hot for d in range(-8, 24)):
            continue
        return rva
    return None


def do_selftest(args):
    """Plant known defects and require the map to report each one.

    A sieve that returns 0 rows because it is BLIND is indistinguishable from a
    sieve that returns 0 rows because the tree is clean; a recent lane shipped
    exactly that and only an injected control caught it. Each injection below is
    a defect class this campaign has actually shipped, applied to the in-memory
    claim set only - src/ is never touched:

      narrow   declare u8 where retail does 4-byte accesses   -> width
      widen    declare double where retail does 1-byte accesses -> width
      float    declare i32 where retail uses fld/fst          -> width
      swap     reverse a two-field struct's member order      -> width (g_idleGeom)
      halve    halve the extent of an accessed claim          -> unclaimed
      stride   declare a *4-indexed table as i8[]             -> stride
      split    cut a claim in two mid-object                  -> adjacent
      phantom  a claim in dead space                          -> unaccessed
    """
    ctx = get_context()
    types = Types(STRUCTS)
    accesses, cells, stats = sweep(ctx)
    base = attach_pct(build_claims(types, ctx.pe))
    starts = [c.rva for c in base]

    per = defaultdict(Counter)
    fpu = defaultdict(Counter)
    scale = defaultdict(Counter)
    for a in accesses:
        if a.form not in TOUCH:
            continue
        k = bisect.bisect_right(starts, a.target_rva) - 1
        if k < 0 or a.target_rva >= base[k].end:
            continue
        c = base[k]
        per[c.rva][(a.target_rva - c.rva, a.width)] += 1
        if a.fpu:
            fpu[c.rva][(a.target_rva - c.rva, a.fpu)] += 1
        if a.form == "indexed" and a.scale and a.target_rva == c.rva:
            scale[c.rva][a.scale] += 1

    def pick(pred):
        return next((c for c in base if pred(c)), None)

    def clone(c, **kw):
        f = {k: getattr(c, k) for k in
             ("rva", "name", "unit", "type", "section", "extent", "extent_src",
              "fields", "pct")}
        f.update(kw)
        return Claim(**f)

    plans = []
    c = pick(lambda c: per[c.rva] and
             all(o == 0 and w == 4 for (o, w) in per[c.rva]) and c.extent >= 4)
    if c:
        plans.append(("narrow", "width", c,
                      lambda c: [clone(c, type="u8", extent=1,
                                       fields=types.flatten("u8"),
                                       extent_src="INJECTED")]))
    c = pick(lambda c: per[c.rva] and
             all(o == 0 and w == 1 for (o, w) in per[c.rva]))
    if c:
        plans.append(("widen", "width", c,
                      lambda c: [clone(c, type="double", extent=8,
                                       fields=types.flatten("double"),
                                       extent_src="INJECTED")]))
    c = pick(lambda c: any(o == 0 and t.startswith("f") for (o, t) in fpu[c.rva])
             and c.extent >= 4)
    if c:
        plans.append(("float", "width", c,
                      lambda c: [clone(
                          c, type=f"i32[{max(c.extent // 4, 1)}]",
                          fields=types.flatten(f"i32[{max(c.extent // 4, 1)}]"),
                          extent_src="INJECTED")]))
    # the g_idleGeom bug: two members declared in the wrong order. Retail has
    # no two-field claim whose members differ in SIZE, so the victim carries a
    # synthetic pair built from its OWN observed widths at +0 and +4, reversed.
    # (A swap between two SAME-SIZED members is invisible to a width map at all
    # - see the coverage note; that needs value evidence, not access evidence.)
    _TY = {1: "u8", 2: "u16", 4: "i32", 8: "double"}

    def _wat(c, o):
        return {w for (oo, w) in per[c.rva] if oo == o and w in _TY}
    c = pick(lambda c: c.extent >= 8 and len(_wat(c, 0)) == 1
             and len(_wat(c, 4)) == 1 and _wat(c, 0) != _wat(c, 4))
    if c:
        def _swap(c):
            a, b = next(iter(_wat(c, 0))), next(iter(_wat(c, 4)))
            return [clone(c, type="SwappedPair", extent_src="INJECTED", fields=[
                (0, b, ".m_second", _TY[b], 1),
                (4, a, ".m_first", _TY[a], 1)])]
        plans.append(("swap", "width", c, _swap))
    wrote = defaultdict(set)
    for a in accesses:
        if a.form in TOUCH and "w" in a.rw:
            k = bisect.bisect_right(starts, a.target_rva) - 1
            if k >= 0 and a.target_rva < base[k].end:
                wrote[base[k].rva].add(a.target_rva - base[k].rva)
    c = pick(lambda c: c.extent >= 16 and
             any(o >= c.extent // 2 for o in wrote[c.rva]))
    if c:
        plans.append(("halve", "unclaimed", c,
                      lambda c: [clone(c, extent=c.extent // 2,
                                       extent_src="INJECTED")]))
    c = pick(lambda c: scale[c.rva] and max(scale[c.rva]) >= 4)
    if c:
        plans.append(("stride", "stride", c,
                      lambda c: [clone(c, type=f"i8[{c.extent}]",
                                       fields=types.flatten(f"i8[{min(c.extent, 64)}]"),
                                       extent_src="INJECTED")]))
    c = pick(lambda c: c.extent >= 8 and
             any(o + w > 4 and o < 4 for (o, w) in per[c.rva]))
    if c:
        plans.append(("split", "adjacent", c, lambda c: [
            clone(c, extent=4, extent_src="INJECTED",
                  fields=types.flatten("i32")),
            clone(c, rva=c.rva + 4, name=c.name + "$SPLIT",
                  extent=c.extent - 4, extent_src="INJECTED",
                  fields=types.flatten("i32"))]))
    dead = _dead_space(ctx.pe, base, accesses, cells)
    if dead is not None:
        plans.append(("phantom", "unaccessed", None, None))

    print(f"[selftest] {len(plans)} injection(s) planted")
    ok = 0
    for tag, want, victim, mutate in plans:
        if tag == "phantom":
            ghost = Claim(rva=dead, name="?g_injectedPhantom@@3HA", unit="selftest",
                          type="i32", section=".data", extent=4,
                          extent_src="INJECTED", fields=types.flatten("i32"),
                          pct=100.0)
            mutated = sorted(base + [ghost], key=lambda c: c.rva)
            key = dead
            label = f"synthetic claim at 0x{dead:x} in dead space"
        else:
            mutated = []
            for c in base:
                mutated.extend(mutate(c) if c.rva == victim.rva else [c])
            mutated.sort(key=lambda c: c.rva)
            key = victim.rva
            label = f"{victim.name[:40]} 0x{victim.rva:x}"
        rows, _st = derive_findings(ctx, types, accesses, cells, mutated)
        caught = [r for r in rows if r[0] == want and
                  (r[2] == key or r[4] == key or
                   (victim is not None and victim.rva <= r[4] < victim.rva + victim.extent))]
        ok += bool(caught)
        print(f"  {'CAUGHT' if caught else 'MISSED':6} {tag:8} -> {want:10} {label}")
        for r in caught[:1]:
            print(f"           [{r[0]}/{r[1]}] {r[5]}")
    print(f"[selftest] {ok}/{len(plans)} injected defects detected")
    return 0 if ok == len(plans) else 1


# --- main ---------------------------------------------------------------------
def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--build", action="store_true", help="sweep and persist")
    ap.add_argument("--at", help="every reference touching one address")
    ap.add_argument("--range", dest="rng", help="LO:HI byte range")
    ap.add_argument("--symbol", help="one claim: field map + every reference")
    ap.add_argument("--fn", help="every data reference one function makes")
    ap.add_argument("--findings", nargs="?", const="", help="the derived worklist")
    ap.add_argument("--sql", help="raw SQL over the map")
    ap.add_argument("--touched", nargs="?", type=Path, const=TOUCHED,
                    help="emit the coalesced byte ranges retail TOUCHES "
                         "(the completeness lane's input)")
    ap.add_argument("--calibrate", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--limit", type=int, default=40)
    ap.add_argument("--sqlite", type=Path, default=SQLITE)
    ap.add_argument("--tsv", type=Path, default=TSV)
    args = ap.parse_args(argv)

    if args.calibrate:
        return do_calibrate(args)
    if args.selftest:
        return do_selftest(args)
    if args.touched:
        return do_touched(args)
    if args.build or not any((args.at, args.rng, args.symbol, args.fn,
                              args.touched,
                              args.findings is not None, args.sql)):
        return do_build(args)

    con = connect(args.sqlite)
    if args.at:
        return do_at(con, int(args.at, 16))
    if args.rng:
        lo, hi = (int(x, 16) for x in args.rng.replace("-", ":").split(":"))
        return do_range(con, lo, hi)
    if args.symbol:
        return do_symbol(con, args.symbol)
    if args.fn:
        return do_fn(con, args.fn)
    if args.sql:
        cur = con.execute(args.sql)
        cols = [d[0] for d in cur.description]
        addrish = [c.endswith("_rva") or c in ("addr", "start", "end", "off")
                   for c in cols]
        print("\t".join(cols))
        for row in cur:
            print("\t".join(
                _hex(v) if a and isinstance(v, int) else str(v)
                for v, a in zip(row, addrish)))
        return 0
    if args.findings is not None:
        return do_findings(con, args.findings or None, args.limit)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
