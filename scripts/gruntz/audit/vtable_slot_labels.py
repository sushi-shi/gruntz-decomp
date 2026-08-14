#!/usr/bin/env python3
"""Recover retail addresses for library virtuals from the vtable slots that hold them.

For every class whose primary vtable both sides carry, TWO INDEPENDENT SOURCES state
the same thing about slot *i*: the base object cl.exe emitted names it (a `DIR32`
relocation against the virtual), and the retail image's vtable holds its ADDRESS.
Pairing them gives `(name -> retail rva)` for functions no other oracle reaches -
the MFC base-class virtuals (`CObject::Serialize`, `CCmdTarget::GetDispatchMap`,
`CWnd::PreSubclassWindow`, ...) that FLIRT either missed or mislabelled.

WHY IT MATTERS. The delinked target names a vtable slot by looking the address up in
the synthesised PDB. When nothing claims it the lookup falls back to the closest
PRECEDING function, and for a slot holding an `/INCREMENTAL` jmp thunk that
"preceding function" is *the previous 5-byte thunk*, which forwards somewhere
entirely unrelated. So an unlabelled `CObject::Serialize` does not show up as a
blank - it shows up as `?ToggleRegionA@CTriggerMgr@@QAEHXZ+5`, once per vtable.

NOTHING IS ACCEPTED ON ONE VOTE OR ONE DIRECTION:

  * the map must be 1:1 - one name at one rva AND one rva under one name. A name
    seen at two addresses means our model puts different functions in one slot
    across classes, and neither reading is trusted;
  * the rva may not already be claimed, by a `symbol_names.csv` src claim (address
    or extent) or by an active `functions_static_libs.tsv` row;
  * a name our source ALREADY claims at a different rva is not a label at all - it
    is a MODEL CONTRADICTION (our vtable says slot *i* holds `X`, retail's slot *i*
    points somewhere `X` is not) and is reported as a defect worklist instead;
  * where our objects also EMIT the body - an `_AFX_INLINE` from `AFX.INL` that cl
    materialises as a COMDAT in every TU - the retail bytes at the recovered address
    are compared against ours with the relocated dwords masked. That is a byte proof,
    and it is reported per row.

Usage:
    python -m gruntz.audit.vtable_slot_labels              # the census
    python -m gruntz.audit.vtable_slot_labels --rows       # functions_static_libs.tsv rows
    python -m gruntz.audit.vtable_slot_labels --contradictions
"""
from __future__ import annotations

import argparse
import bisect
import csv
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(REPO / "scripts"))
sys.path.insert(0, str(REPO / "scripts/gruntz/build"))

import canonicalize_data_symbols as canon  # noqa: E402
from gruntz.core.library_labels import active_rvas  # noqa: E402
from gruntz.core.pe import PE  # noqa: E402

EXE = REPO / "build/exe/GRUNTZ.EXE"
BASE_DIR = REPO / "build/objdiff/base"
SYMBOLS = REPO / "build/gen/symbol_names.csv"
LIBRARY = REPO / "config/retail/functions_static_libs.tsv"
WEAK_EXTERNAL = 105
JMP_REL32, INT3, THUNK_LEN = 0xE9, 0xCC, 5


def ilt_map(pe):
    """{thunk rva -> body rva} for link.exe's /INCREMENTAL jmp table."""
    text = next(s for s in pe.secs if s[0] == ".text")
    rva, end = text[1], text[1] + max(text[2], text[4])
    while rva < end and pe.data[pe.off(rva)] == INT3:
        rva += 1
    out = {}
    while rva + THUNK_LEN <= end and pe.data[pe.off(rva)] == JMP_REL32:
        disp = struct.unpack_from("<i", pe.data, pe.off(rva) + 1)[0]
        out[rva] = rva + THUNK_LEN + disp
        rva += THUNK_LEN
    return out


def owning_class(mangled):
    """`?Method@CFoo@@...` -> `CFoo`, or None for a non-member name."""
    if not mangled.startswith("?"):
        return None
    parts = mangled.lstrip("?").split("@")
    return parts[1] if len(parts) > 2 and parts[1] else None


def src_claims(path=SYMBOLS):
    """({rva}, {name -> rva}, sorted [(rva, end)] extents, {class we reconstruct})."""
    rvas, names, extents, classes = set(), {}, [], set()
    if not Path(path).is_file():
        return rvas, names, extents, classes
    with Path(path).open() as f:
        for row in csv.DictReader(l for l in f if not l.lstrip().startswith("#")):
            rva = int(row["rva"], 16)
            rvas.add(rva)
            names.setdefault(row["name"], rva)
            owner = owning_class(row["name"])
            if owner:
                classes.add(owner)
            size = (row.get("size") or "").strip()
            if size:
                extents.append((rva, rva + int(size, 16)))
    extents.sort()
    return rvas, names, extents, classes


def _inside(extents, rva):
    i = bisect.bisect_right(extents, (rva, 1 << 62)) - 1
    return i >= 0 and extents[i][0] <= rva < extents[i][1]


def collect(exe=EXE, base_dir=BASE_DIR):
    """(name -> {rva}), (rva -> {name}), vote counts, our own COMDAT payloads."""
    from gruntz.core.vtable_hierarchy import build_registry

    pe = PE(str(exe))
    ilt = ilt_map(pe)
    reg, _src = build_registry()
    primary = {"??_7%s@@6B@" % n: ci.vtables[0]
               for n, ci in reg.items() if ci.vtables.get(0)}

    by_name, by_rva, votes = defaultdict(set), defaultdict(set), Counter()
    emitted = {}                       # name -> (payload, {relocated site})
    for obj in sorted(Path(base_dir).glob("*.obj")):
        try:
            coff = canon.CoffObject(obj.read_bytes())
        except Exception:
            continue
        sites = defaultdict(dict)
        for rel in coff.relocations:
            sites[rel.section][rel.site] = rel.symbol_index
        for sym in coff.symbols.values():
            if sym.section < 1 or sym.storage_class != 2:
                continue
            sec = coff.sections[sym.section - 1]
            if sym.name not in emitted and sec.characteristics & canon.MEM_EXECUTE \
                    and len(_defined(coff, sec)) == 1:
                emitted[sym.name] = (coff.section_bytes(sec),
                                     {s for s in sites[sec.index]})
            if not sym.name.startswith("??_7"):
                continue
            hit = primary.get(sym.name)
            if hit is None or sec.raw_size - sym.value != hit[1] * 4:
                continue
            for slot in range(hit[1]):
                index = sites[sec.index].get(sym.value + slot * 4)
                body = pe.u32(hit[0] + slot * 4)
                if index is None or body is None:
                    continue
                name = coff.symbols[index].name
                if coff.symbols[index].storage_class == WEAK_EXTERNAL:
                    tag = struct.unpack_from(
                        "<I", coff.data, coff.symbols[index].offset + 18)[0]
                    name = coff.symbols[tag].name
                body = body - pe.image_base
                body = ilt.get(body, body)
                by_name[name].add(body)
                by_rva[body].add(name)
                votes[(name, body)] += 1
    return pe, by_name, by_rva, votes, emitted


def _defined(coff, section):
    return [s for s in coff.symbols.values()
            if s.section == section.index and s.storage_class == 2]


def classify(exe=EXE, base_dir=BASE_DIR):
    """(accepted rows, model contradictions, ambiguous names)."""
    pe, by_name, by_rva, votes, emitted = collect(exe, base_dir)
    claimed_rvas, claimed_names, extents, our_classes = src_claims()
    labelled = active_rvas(LIBRARY)

    accepted, contradictions, ambiguous = [], [], []
    for name in sorted(by_name):
        addrs = by_name[name]
        if len(addrs) != 1:
            ambiguous.append((name, sorted(addrs)))
            continue
        rva = next(iter(addrs))
        if len(by_rva[rva]) != 1:
            ambiguous.append((name, [rva]))
            continue
        if name in claimed_names:
            if claimed_names[name] != rva:
                contradictions.append((name, claimed_names[name], rva,
                                       votes[(name, rva)]))
            continue
        if rva in claimed_rvas or rva in labelled or _inside(extents, rva):
            continue
        proof = "vtable-slot"
        payload = emitted.get(name)
        if payload is not None:
            off = pe.off(rva)
            body, mask = payload
            if off is not None and len(body) >= 4:
                ours, theirs = bytearray(body), bytearray(pe.data[off:off + len(body)])
                for site in mask:
                    ours[site:site + 4] = theirs[site:site + 4] = b"\0\0\0\0"
                proof = ("byte-identical" if ours == theirs
                         else "candidate-body-differs")
        # A name whose CLASS our source reconstructs is not a library body - it is a
        # compiler-generated COMDAT copy of one of OUR header inlines, whose home is
        # an `RVA_COMPGEN` pin in the owning TU, not `functions_static_libs.tsv`.
        owner = owning_class(name)
        accepted.append({"rva": rva, "name": name, "votes": votes[(name, rva)],
                         "proof": proof, "class": owner,
                         "home": "src-RVA_COMPGEN" if owner in our_classes
                                 else "functions_static_libs.tsv"})
    accepted.sort(key=lambda r: r["rva"])
    return accepted, contradictions, ambiguous


def main(argv=None):
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rows", action="store_true",
                    help="print functions_static_libs.tsv rows for the accepted set")
    ap.add_argument("--contradictions", action="store_true",
                    help="print the names our source claims at a different rva")
    args = ap.parse_args(argv)

    accepted, contradictions, ambiguous = classify()
    print("[vtable-slot-labels] %d accepted, %d model contradiction(s), "
          "%d ambiguous name(s)" % (len(accepted), len(contradictions), len(ambiguous)))
    print("  proof: " + ", ".join(
        "%s=%d" % kv for kv in sorted(Counter(r["proof"] for r in accepted).items())))
    print("  home : " + ", ".join(
        "%s=%d" % kv for kv in sorted(Counter(r["home"] for r in accepted).items())))
    if args.rows:
        for row in accepted:
            if row["home"] == "functions_static_libs.tsv":
                print("0x%06x\t%s\tNAFXCW\tHIGH\tvtable-slot-oracle"
                      % (row["rva"], row["name"]))
    else:
        for row in accepted:
            print("  0x%06x  %-5d votes  %-22s %-18s %s"
                  % (row["rva"], row["votes"], row["proof"], row["home"], row["name"]))
    if args.contradictions or not args.rows:
        print("\n--- model contradictions (our slot says X; retail's slot is not X) ---")
        for name, claimed, seen, n in sorted(contradictions, key=lambda x: x[1]):
            print("  src 0x%06x  retail slot -> 0x%06x  (%d vote(s))  %s"
                  % (claimed, seen, n, name))
        print("\n--- ambiguous (one name over several rvas) ---")
        for name, addrs in ambiguous:
            print("  %-58s %s" % (name[:58], [hex(a) for a in addrs]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
