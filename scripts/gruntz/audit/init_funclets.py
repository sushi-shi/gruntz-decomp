"""init_funclets - name every dynamically-initialized static via the XCU walk.

cl emits one `$E`/`$S` initializer funclet per dynamically-initialized static
group and a 4-byte cell in `.CRT$XCU` pointing at it; the linker concatenates
the cells IN OBJECT LINK ORDER into the retail initializer table (walked by
`_initterm`). That table is therefore a positional index of every funclet, and
each funclet's absolute-address operands (all of them `.reloc` HIGHLOW sites)
name the statics it constructs - including statics NOTHING else references.

The pairing is self-proving, `assert_relocs`-style:

  * a retail funclet must lie inside the `.text` span of the unit the walk
    attributes it to (spans from the claimed function inventory);
  * the funclet's DIR32 reloc count must equal its base-obj twin's;
  * every binding whose base symbol rva is ALREADY KNOWN (symbol_names) must
    equal what retail stored at the paired site.  One disagreement discards
    the whole unit - a candidate binding never survives a broken proof.

What survives is (retail rva, cl's verbatim symbol, unit, size-from-obj-extent)
for statics with NO claim - the exact shape `config/static_data_copies.tsv`
rows take.

USAGE
    python -m gruntz.audit.init_funclets            # report + candidates
    python -m gruntz.audit.init_funclets --tsv P    # write candidate rows
"""
from __future__ import annotations

import argparse
import bisect
import csv
import struct
import sys
from collections import defaultdict
from pathlib import Path

from gruntz.core.pe import PE, REPO

sys.path.insert(0, str(REPO / "scripts/gruntz/build"))

BASE = REPO / "build/objdiff/base"
SYMBOL_NAMES = REPO / "build/gen/symbol_names.csv"
DIR32 = 6


# --------------------------------------------------------------------------- #
# retail side
# --------------------------------------------------------------------------- #
def retail_table(pe: PE):
    """Ordered [(cell_rva, funclet_rva)] of the merged initializer table.

    Detection is structural: the densest cluster of consecutive 4-byte `.data`
    reloc cells whose stored VAs land in `.text`, allowing the <=4 null cells
    cl's contribution padding leaves between objects.
    """
    sec = {s["name"]: s for s in pe.sections}
    da, tx = sec[".data"], sec[".text"]
    tlo, thi = tx["rva"], tx["rva"] + tx["virtual_size"]
    rel = sorted(s for s in pe.reloc_sites
                 if da["rva"] <= s < da["rva"] + da["raw_size"])
    ptr = {}
    for s in rel:
        v = struct.unpack_from("<I", pe.data, pe.off(s))[0] - pe.image_base
        if tlo <= v < thi:
            ptr[s] = v

    def is_null(rva):
        o = pe.off(rva)
        return (o is not None and rva not in pe.reloc_sites
                and struct.unpack_from("<I", pe.data, o)[0] == 0)

    clusters, cur = [], None
    for s in sorted(ptr):
        if cur is not None:
            gap_cells = (s - cur[-1][0]) // 4 - 1
            if 0 <= gap_cells <= 4 and all(
                    is_null(cur[-1][0] + 4 * (i + 1)) for i in range(gap_cells)):
                cur.append((s, ptr[s]))
                continue
        cur = [(s, ptr[s])]
        clusters.append(cur)
    return max(clusters, key=len)


# --------------------------------------------------------------------------- #
# base side
# --------------------------------------------------------------------------- #
def _relocs(coff, sec_idx):
    """[(vaddr, symidx, type)] of one section, ascending vaddr."""
    st = coff.section_table[sec_idx]
    out = []
    for i in range(st["reloc_count"]):
        o = st["reloc_offset"] + i * 10
        va, sym, ty = struct.unpack_from("<IIH", coff.buf, o)
        out.append((va, sym, ty))
    out.sort()
    return out


def _sym(coff, idx):
    """(name, value, secnum, storage_class) for symbol table index idx."""
    base = coff.symptr + idx * 18
    value, secnum, _t, scl, _n = struct.unpack_from("<IhHBB", coff.buf, base + 8)
    return coff.sym_name(idx), value, secnum, scl


def unit_funclets(obj_path: Path):
    """[(funclet_name, [(sym_name, value, secnum, scl), ...]), ...] in XCU order.

    Each funclet's list is its DIR32 reloc targets in instruction order.
    cl 5.0 emits the XCU-visible funclet as a tiny thunk whose single REL32
    calls the real initializer body (its own COMDAT `.text` section), so a
    funclet with no DIR32s of its own is followed through that call, bounded.
    """
    from coff_oracle import _Coff
    coff = _Coff(obj_path)
    xcu = next((i for i, st in enumerate(coff.section_table)
                if st["name"] == ".CRT$XCU"), None)
    if xcu is None:
        return []
    cells = _relocs(coff, xcu)
    # boundaries of every symbol per code section, for funclet extents
    sec_syms = defaultdict(list)
    for idx, value, sn in coff.iter_symbols():
        if sn > 0:
            sec_syms[sn].append(value)
    for v in sec_syms.values():
        v.sort()

    def body_relocs(fval, fsec, depth=0):
        bounds = sec_syms[fsec]
        j = bisect.bisect_right(bounds, fval)
        fend = (bounds[j] if j < len(bounds)
                else coff.section_table[fsec - 1]["size"])
        payload = coff.section_payload(fsec)
        dir32, rel32 = [], []
        for va, sidx, rty in _relocs(coff, fsec - 1):
            if not (fval <= va < fend):
                continue
            if rty == DIR32:
                inline = (struct.unpack_from("<I", payload, va)[0]
                          if va + 4 <= len(payload) else 0)
                name, value, secnum, scl = _sym(coff, sidx)
                # COFF stores the ADDEND inline: final = S + A. A reloc against
                # the SECTION symbol reaches a member at A = member_off + k;
                # resolve it so the binding names the member and its k.
                if (scl == 3 and 1 <= secnum <= coff.nsec
                        and name == coff.section_table[secnum - 1]["name"]):
                    members = coff.section_members(secnum)
                    j = bisect.bisect_right([m[0] for m in members], inline) - 1
                    if j >= 0:
                        moff, mname, _mscl = members[j]
                        dir32.append((mname, inline - moff))
                        continue
                dir32.append((name, inline))
            elif rty == 0x14:                      # REL32
                rel32.append(_sym(coff, sidx))
        if not dir32 and len(rel32) == 1 and depth < 2:
            cname, cval, csec, _cscl = rel32[0]
            if csec > 0:
                return body_relocs(cval, csec, depth + 1)
        return dir32

    out = []
    for _va, symidx, ty in cells:
        if ty != DIR32:
            continue
        fname, fval, fsec, _scl = _sym(coff, symidx)
        out.append((fname, body_relocs(fval, fsec)))
    return out


def base_extents(obj_path: Path):
    """{symbol name: (size, section_name)} from the obj's own topology."""
    from coff_oracle import _Coff
    coff = _Coff(obj_path)
    out = {}
    for i, st in enumerate(coff.section_table):
        members = coff.section_members(i + 1)
        for k, (off, name, _scl) in enumerate(members):
            end = members[k + 1][0] if k + 1 < len(members) else st["size"]
            out[name] = (max(end - off, 0), st["name"])
    return out


# --------------------------------------------------------------------------- #
# the walk
# --------------------------------------------------------------------------- #
def known_rvas():
    """{symbol name: rva} for every name the build already binds."""
    out = {}
    if SYMBOL_NAMES.is_file():
        with SYMBOL_NAMES.open() as f:
            for r in csv.DictReader(f):
                out[r["name"]] = int(r["rva"], 16)
    return out


def unit_spans():
    """([(unit, lo, hi)] in link order, [every function start rva])."""
    from gruntz.core.exe_map import load
    funcs, _meta = load()
    firsts = {}
    for f in funcs:
        if f["category"] == "unit" and f["unit"]:
            firsts.setdefault(f["unit"], f["rva"])
            firsts[f["unit"]] = min(firsts[f["unit"]], f["rva"])
    order = sorted(firsts.items(), key=lambda kv: kv[1])
    out = []
    for i, (u, lo) in enumerate(order):
        hi = order[i + 1][1] if i + 1 < len(order) else 0x7fffffff
        out.append((u, lo, hi))
    return out, sorted(f["rva"] for f in funcs)


def walk(pe: PE | None = None, quiet=False):
    pe = pe or PE()
    table = retail_table(pe)
    spans, fn_starts = unit_spans()
    starts = [lo for _u, lo, _hi in spans]
    known = known_rvas()
    rel_sorted = pe.reloc_sites

    claim_iv, claim_starts, claim_owner = [], set(), {}
    man = REPO / "build/gen/delink_data_manifest.tsv"
    if man.is_file():
        with man.open() as f:
            for r in csv.DictReader(f, delimiter="\t"):
                a, sz = int(r["rva"], 16), int(r["size"], 16)
                claim_iv.append((a, a + sz))
                claim_starts.add(a)
                claim_owner[a] = r["object"].removesuffix(".c")
    claim_iv.sort()
    claim_lo = [a for a, _b in claim_iv]

    def inside_claim(rva):
        i = bisect.bisect_right(claim_lo, rva) - 1
        return i >= 0 and claim_iv[i][0] <= rva < claim_iv[i][1]

    def unit_at(rva):
        # cl 5.0 emits the funclet COMDATs BEFORE the functions (proven from
        # the base objs' own section order, worldsoundset.obj: funclets in
        # sections 4-38, functions 40-136), and the linker preserves object
        # section order - so a funclet belongs to the unit whose FIRST
        # inventoried function follows it.
        i = bisect.bisect_right(starts, rva)
        return spans[i][0] if i < len(spans) else None

    # group consecutive retail entries by attributed unit
    groups = []
    for cell, target in table:
        u = unit_at(target)
        if groups and groups[-1][0] == u:
            groups[-1][1].append(target)
        else:
            groups.append([u, [target]])

    stats = defaultdict(int)
    candidates, mismatched_units, misowned = [], [], []

    def funclet_end(i, t):
        """A funclet ends at the next table target, the next INVENTORIED
        function (funclets live in inventory gaps), or a hard cap."""
        nxt = table[i + 1][1] if i + 1 < len(table) else t + 0x200
        j = bisect.bisect_right(fn_starts, t)
        nxt_fn = fn_starts[j] if j < len(fn_starts) else t + 0x200
        return min(nxt, nxt_fn, t + 0x200)

    next_target = {t: funclet_end(i, t) for i, (_c, t) in enumerate(table)}

    for u, targets in groups:
        if u is None:
            stats["library-tail funclets"] += len(targets)
            continue
        obj = BASE / f"{u}.obj"
        if not obj.is_file():
            stats["unit without base obj"] += 1
            continue
        base = unit_funclets(obj)
        if len(base) != len(targets):
            mismatched_units.append((u, len(base), len(targets)))
            stats["units with count mismatch"] += 1
            continue
        unit_ok, unit_bind = True, {}
        for (fname, brelocs), t in zip(base, targets):
            lo_i = bisect.bisect_left(rel_sorted, t)
            hi_i = bisect.bisect_left(rel_sorted, next_target[t])
            sites = rel_sorted[lo_i:hi_i]
            if len(sites) != len(brelocs):
                stats["funclet reloc-count mismatch"] += 1
                unit_ok = False
                break
            stored_vals = [struct.unpack_from("<I", pe.data, pe.off(s))[0]
                           - pe.image_base for s in sites]
            syms = {n for n, _o in brelocs}
            if len(syms) == 1:
                # One static, N member stores: the two compilers schedule the
                # stores differently, so pair the VALUE multisets by rank.
                # {stored - S} must reproduce the base offset multiset exactly.
                name = next(iter(syms))
                sv = sorted(stored_vals)
                ov = sorted(o for _n, o in brelocs)
                bases = {v - o for v, o in zip(sv, ov)}
                if len(bases) != 1:
                    unit_ok = False
                    stats["offset multiset mismatch"] += 1
                    if not quiet:
                        print(f"  DISCARD {u}: {name} stores {sv} do not "
                              f"reproduce offsets {ov}")
                    break
                pairs = [(name, bases.pop())]
            else:
                pairs = [(n, v - o) for v, (n, o)
                         in zip(stored_vals, brelocs)]
            for sname, addr in pairs:
                prev = unit_bind.setdefault(sname, addr)
                if prev != addr:
                    unit_ok = False
                    stats["inconsistent binding"] += 1
                    if not quiet:
                        print(f"  DISCARD {u}: {sname} binds both "
                              f"0x{prev:06x} and 0x{addr:06x}")
                    break
                k = known.get(sname)
                if k is not None:
                    if k != addr:
                        unit_ok = False
                        stats["known-binding DISAGREEMENT"] += 1
                        if not quiet:
                            print(f"  DISCARD {u}: {sname} known 0x{k:06x} "
                                  f"but funclet stores 0x{addr:06x}")
                        break
                    stats["known bindings verified"] += 1
                elif inside_claim(addr):
                    if addr not in claim_starts:
                        unit_ok = False
                        stats["binding inside a claim, not at its start"] += 1
                        if not quiet:
                            print(f"  DISCARD {u}: {sname} binds 0x{addr:06x} "
                                  "interior to an existing claim")
                        break
                    stats["claim-start bindings verified"] += 1
                    if claim_owner.get(addr) != u:
                        stats["claim OWNER disagrees with funclet"] += 1
                        misowned.append((addr, sname, claim_owner.get(addr), u))
            if not unit_ok:
                break
        if unit_ok:
            stats["units fully paired"] += 1
            candidates.extend((a, n, u) for n, a in unit_bind.items())
        else:
            mismatched_units.append((u, len(base), len(targets)))

    # attach sizes from the owning obj
    sized = []
    ext_cache = {}
    for addr, sname, u in candidates:
        if u not in ext_cache:
            ext_cache[u] = base_extents(BASE / f"{u}.obj")
        size, secname = ext_cache[u].get(sname, (0, "?"))
        sized.append((addr, sname, u, size, secname))
    return {"table": table, "groups": groups, "stats": stats,
            "candidates": sized, "mismatched": mismatched_units,
            "misowned": misowned}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--tsv", help="write candidate rows (static_data_copies shape)")
    ap.add_argument("--all", action="store_true",
                    help="list candidates already claimed too")
    args = ap.parse_args()
    pe = PE()
    r = walk(pe)
    print(f"retail initializer table: {len(r['table'])} funclet cells, "
          f"{len(r['groups'])} unit groups")
    for k, v in sorted(r["stats"].items()):
        print(f"  {k:34} {v}")
    if r["misowned"]:
        print("\nclaim-owner disagreements (sidecar unit vs funclet proof):")
        for a, n, old, new in r["misowned"]:
            print(f"  0x{a:06x} {n[:52]:52} {old} -> {new}")
    if r["mismatched"]:
        print("\nunpaired units (base funclets vs retail):")
        for u, nb, nr in r["mismatched"]:
            print(f"  {u:28} base {nb:2}  retail {nr:2}")
    # candidate bindings for UNCLAIMED addresses only (claims = enrolled runs)
    from gruntz.core.data_universe import enrolled_runs
    runs = enrolled_runs()
    rlo = [a for a, _b in runs]

    def claimed(rva):
        i = bisect.bisect_right(rlo, rva) - 1
        return i >= 0 and runs[i][0] <= rva < runs[i][1]

    fresh = [(a, n, u, sz, sec) for a, n, u, sz, sec in r["candidates"]
             if args.all or not claimed(a)]
    fresh.sort()
    print(f"\ncandidate bindings ({'all' if args.all else 'unclaimed only'}): "
          f"{len(fresh)}")
    for a, n, u, sz, sec in fresh:
        print(f"  0x{a:06x} {n[:64]:64} {u:24} 0x{sz:x} [{sec}]")
    if args.tsv:
        with open(args.tsv, "w") as f:
            for a, n, u, sz, _sec in fresh:
                f.write(f"0x{a:x}\t{n}\t{u}\t0x{sz:x}\tdata\n")
        print(f"wrote {args.tsv}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
