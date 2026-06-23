#!/usr/bin/env python3
"""gruntz.analysis.ctors - find constructors by analysing every function.

The named backlog only knows ~77 ctors (the @source:rtti-vptr stubs). RTTI is a
FLOOR, not the ceiling: type descriptors + Complete Object Locators exist only for
polymorphic classes whose RTTI was emitted. Non-polymorphic classes have ctors but
stamp NO vtable, so they're invisible to any vtable-anchored scan.

Stage 1 (this file, for now): the high-confidence + free-name signal. Ghidra did
NOT materialise vftables/COLs, so we parse RTTI ourselves:
    string ".?AV/.?AU...@@"  ->  Type Descriptor (name)
    dword in .rdata whose [+0xC] is a TD  ->  Complete Object Locator
    location L where [L] == COL            ->  vftable = L+4   (since [vftable-4]=COL)
Then scan .text for `mov m32, <vftable>` (C7 /0 imm32) stores; the storing function
is a ctor or dtor of that class. The vftable->TD walk names it for free. We classify
ctor-vs-dtor cheaply (reached-from-`operator new` => ctor; calls `operator delete`
=> dtor) and cross-ref the current src/ inventory to split KNOWN vs NEW.

Non-polymorphic ctors (no vtable: returns-this + field-init + new-site caller) are a
later structural pass - see TODO at the bottom.

Run: python -m gruntz.analysis.ctors
"""
import os, re, csv, struct, bisect, sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
EXE = Path(os.environ.get("GRUNTZ_EXE") or REPO / "build/exe/GRUNTZ.EXE")
FUNCS = REPO / "build/ghidra-enrich/exports/functions.csv"
MATCHED = REPO / "build/gen/symbol_names.csv"
SRC = REPO / "src"
IMAGE_BASE = 0x400000
OPNEW = 0x5b9b46  # operator new ??2@YAPAXI@Z (confirmed: push <size>; call here)


def load_pe():
    data = EXE.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    opt = struct.unpack_from("<H", data, pe + 20)[0]
    so = pe + 24 + opt
    secs = []
    for i in range(nsec):
        o = so + i * 40
        nm = data[o:o + 8].rstrip(b"\0").decode("latin1")
        vsize, vaddr, rawsz, rawp = struct.unpack_from("<IIII", data, o + 8)
        secs.append((nm, vaddr, vsize, rawp, rawsz))
    return data, secs


def main():
    data, secs = load_pe()
    text = next(s for s in secs if s[0] == ".text")
    tbytes = data[text[3]:text[3] + text[4]]
    trva_va = IMAGE_BASE + text[1]
    text_lo, text_hi = trva_va, trva_va + text[2]

    # VA -> file offset (only in mapped sections), for reading dwords by VA.
    def rd32(va):
        for nm, vaddr, vsize, rawp, rawsz in secs:
            base = IMAGE_BASE + vaddr
            if base <= va < base + vsize:
                fo = rawp + (va - base)
                if fo + 4 <= len(data):
                    return struct.unpack_from("<I", data, fo)[0]
        return None

    # ---- RTTI: type descriptors (name lives at TD+8) ----
    td_name = {}  # TD_VA -> mangled name
    tdpat = re.compile(rb"\.\?A[UV][\w@?$]+@@")
    for nm, vaddr, vsize, rawp, rawsz in secs:
        if nm == ".text":
            continue
        blob = data[rawp:rawp + rawsz]
        for m in tdpat.finditer(blob):
            name_va = IMAGE_BASE + vaddr + m.start()
            td_name[name_va - 8] = m.group().decode("latin1")
    td_set = set(td_name)

    def demangle(mn):  # ".?AVCFoo@@" -> "CFoo"; keep templates raw-ish
        s = mn[4:] if mn[:4] in (".?AV", ".?AU") else mn
        return s[:-2] if s.endswith("@@") and "?$" not in s else mn

    # ---- COLs + vftables: scan .rdata dwords ----
    vtab_name = {}  # vftable_VA -> mangled name
    for nm, vaddr, vsize, rawp, rawsz in secs:
        if nm == ".text":
            continue
        base = IMAGE_BASE + vaddr
        end = rawp + (rawsz & ~3)
        for fo in range(rawp, end, 4):
            col = struct.unpack_from("<I", data, fo)[0]
            ptd = rd32(col + 0xC) if col else None
            if ptd in td_set:                      # [col] is a COL pointing at a TD
                vft = base + (fo - rawp) + 4        # vftable starts right after the COL ptr
                slot0 = rd32(vft)
                if slot0 and text_lo <= slot0 < text_hi:  # first vslot is real code
                    vtab_name[vft] = td_name[ptd]
    vtab_set = set(vtab_name)

    # ---- functions.csv -> bisect mapping ----
    starts, fsize, fname = [], {}, {}
    with open(FUNCS) as f:
        for r in csv.DictReader(f):
            rva = int(r["entry_rva"], 16)
            starts.append(rva); fsize[rva] = int(r["byte_size"]); fname[rva] = r["name"]
    starts.sort()

    def func_of(rva):
        i = bisect.bisect_right(starts, rva) - 1
        return starts[i] if i >= 0 and rva < starts[i] + fsize[starts[i]] else None

    # ---- scan .text for C7 /0 stores of a vftable immediate ----
    # forms: o-2=C7 modrm(mod00,reg000,rm!=4,5); o-3=C7 mod01; o-6=C7 mod10; o-2=C7 mod11(reg load)
    def is_store_at(o):
        def c7_reg0(b):  # /0 = reg field 000
            return (b >> 3) & 7 == 0
        if o >= 2 and tbytes[o - 2] == 0xC7 and c7_reg0(tbytes[o - 1]) and (tbytes[o - 1] >> 6) in (0, 3) and (tbytes[o - 1] & 7) not in (4, 5):
            return True
        if o >= 3 and tbytes[o - 3] == 0xC7 and c7_reg0(tbytes[o - 2]) and (tbytes[o - 2] >> 6) == 1 and (tbytes[o - 2] & 7) != 4:
            return True
        if o >= 6 and tbytes[o - 6] == 0xC7 and c7_reg0(tbytes[o - 5]) and (tbytes[o - 5] >> 6) == 2 and (tbytes[o - 5] & 7) != 4:
            return True
        return False

    func_vtabs = {}  # func_rva -> set(vftable VA stored)
    n = len(tbytes)
    lo = min(vtab_set); hi = max(vtab_set)
    for o in range(n - 3):
        v = tbytes[o] | (tbytes[o+1] << 8) | (tbytes[o+2] << 16) | (tbytes[o+3] << 24)
        if v < lo or v > hi or v not in vtab_set:
            continue
        if not is_store_at(o):
            continue
        fn = func_of(trva_va - IMAGE_BASE + o)  # rva of this byte
        if fn is not None:
            func_vtabs.setdefault(fn, set()).add(v)

    # ---- ctor-vs-dtor hints: callees per func (E8 rel32) ----
    def callees_of(frva):
        cs = set()
        b = trva_va - IMAGE_BASE
        for o in range(frva - b, frva - b + fsize.get(frva, 0) - 4):
            if 0 <= o < n and tbytes[o] == 0xE8:
                rel = struct.unpack_from("<i", tbytes, o + 1)[0]
                cs.add((trva_va - IMAGE_BASE + o + 5) + rel)
        return cs

    # find operator delete: the callee shared by deleting-dtors. Heuristic: most-called
    # single-arg cleanup. We approximate via "calls OPNEW => ctor-ish"; refine later.
    # new-site -> ctor: any func body containing `call OPNEW`.
    news_callers = set(f for f in starts if OPNEW in callees_of(f)) if False else None  # (skip; per-func below)

    # ---- FID library RVAs (exclude: MFC/CRT/COM ctors are link-artifacts, not targets) ----
    lib = set()
    libcsv = REPO / "config/library_labels.csv"
    if libcsv.exists():
        for r in csv.DictReader(open(libcsv)):
            try: lib.add(int(r["rva"], 16))
            except (ValueError, KeyError): pass

    # ---- existing inventory: RVAs already labeled/matched ----
    labeled = set()
    if MATCHED.exists():
        for line in MATCHED.read_text().splitlines()[1:]:
            tok = line.split(",", 1)[0].strip()
            try: labeled.add(int(tok, 16))
            except ValueError: pass
    rvapat = re.compile(r"RVAU?\(\s*0x([0-9a-fA-F]+)")
    for p in SRC.rglob("*.cpp"):
        for m in rvapat.finditer(p.read_text(errors="ignore")):
            labeled.add(int(m.group(1), 16))
    for p in SRC.rglob("*.h"):
        for m in rvapat.finditer(p.read_text(errors="ignore")):
            labeled.add(int(m.group(1), 16))

    # ---- report ----
    # library class names (from FID-tagged candidates) so we can mark coverage by target
    classes_covered = {}  # name -> set(func rvas)
    rows = []
    for frva, vts in sorted(func_vtabs.items()):
        names = sorted({demangle(vtab_name[v]) for v in vts})
        primary = names[0] if len(names) == 1 else "+".join(names)
        target = "lib" if frva in lib else "game"
        known = "known" if frva in labeled else "NEW"
        rows.append((primary, frva, fsize.get(frva, 0), len(vts), target, known))
        for nm in names:
            classes_covered.setdefault(nm, set()).add((frva, target))

    all_rtti = {demangle(n) for n in td_name.values()}
    game_rows = [r for r in rows if r[4] == "game"]
    # a class is a "game/engine target" if NONE of its storing funcs are FID-library
    game_classes = {n for n, fs in classes_covered.items() if all(t == "game" for _, t in fs)}
    covered_game = {n for n, fs in classes_covered.items() if any(t == "game" for _, t in fs)}
    missing = sorted(all_rtti - set(classes_covered))

    print(f"RTTI type descriptors                : {len(td_name)}")
    print(f"vftables resolved (COL->TD)           : {len(vtab_name)}")
    print(f"functions storing a vftable           : {len(rows)}  ({len(game_rows)} game/engine, {len(rows)-len(game_rows)} FID-library)")
    print(f"  game/engine NEW (not yet labeled)   : {sum(1 for r in game_rows if r[5]=='NEW')}")
    print(f"  game/engine already labeled         : {sum(1 for r in game_rows if r[5]=='known')}")
    print(f"distinct RTTI classes covered         : {len(set(classes_covered))} / {len(all_rtti)}  ({len(covered_game)} have a game/engine ctor)")
    print(f"RTTI classes with NO vtable-store      : {len(missing)}")
    print()
    print("=== game/engine vtable-storing functions (ctor OR dtor; not yet split) ===")
    print("class                                  rva        size  #vt  known")
    for primary, frva, sz, nvt, target, known in sorted(game_rows, key=lambda r: (r[5] != "NEW", r[0])):
        print(f"{primary[:38]:38} 0x{frva:06x} {sz:5d} {nvt:4d}  {known}")
    print()
    print("RTTI classes with NO vtable-storing function (abstract? dtor inlined-only?):")
    print("  " + ", ".join(missing) if missing else "  (none)")

    out = REPO / "ctor-survey"
    out.mkdir(exist_ok=True)
    with open(out / "ctor_candidates.csv", "w", newline="") as f:
        w = csv.writer(f); w.writerow(["class", "rva", "size", "n_vtables", "target", "known"])
        for primary, frva, sz, nvt, target, known in sorted(rows):
            w.writerow([primary, f"0x{frva:06x}", sz, nvt, target, known])
    # vftable VA -> class, for new-site leaf attribution (gruntz.analysis.news)
    with open(out / "vtables.csv", "w", newline="") as f:
        w = csv.writer(f); w.writerow(["vftable_va", "class"])
        for va, mn in sorted(vtab_name.items()):
            w.writerow([f"0x{va:x}", demangle(mn)])
    print(f"\nwrote {out / 'ctor_candidates.csv'}  ({len(rows)} rows) + vtables.csv ({len(vtab_name)})")


if __name__ == "__main__":
    main()
