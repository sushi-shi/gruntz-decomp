#!/usr/bin/env python3
"""Generate vostok's `--data-manifest` + `--data-section-manifest`.

STATUS: WIRED IN - delink.py passes both. See docs/data-attribution.md §3b.

The reviewed-data-topology delinker emits each DATA()-annotated global and each
`??_C@` string literal as a real named definition in its owning target object (right
storage class + alignment, with interior base relocations converted to COFF
relocations, and function references to it becoming EXTERNALS instead of duplicated
4-byte allocations). That is what makes objdiff able to score DATA at all. The
companion SECTION manifest additionally rebuilds those definitions in the CANDIDATE's
section shape - see section_rows() for why that is what actually moves the metric.

Schemas (read out of the delinker binary; the data manifest takes 9 or 10 columns):
    name  object  rva  size  storage  alignment  [section_ordinal]  section_offset
        scope  provenance
    object  ordinal  name  rva  size  alignment  characteristics  comdat_selection
        associative_ordinal  storage  provenance
`section_ordinal = -` / `section_offset = -` selects the legacy reviewed-allocation
form (the delinker packs the row itself); an ordinal instead places the row in a
candidate section declared by the section manifest. Section ordinals are per-object
and must be CONTIGUOUS FROM ONE.

Inputs, all already generated:
  * build/gen/symbol_names.csv - kind=data rows carry rva/name/unit and, since the
    DATA()-sizeof work, an exact type-derived `size` (labels.sizeof_qualtype);
  * the retail PE - classifies each RVA's storage (.rdata / initialized .data /
    loader-zero tail => rdata/data/bss) via analysis.data_audit;
  * build/objdiff/base/*.obj - the candidate COFFs cl.exe emitted, which carry the
    authoritative section topology (name/alignment/characteristics/COMDAT selection).

Evidence rules (never fabricate an extent):
  * a row is enrolled ONLY with a proven size; the rows whose declared type is not
    resolvable are withheld, not guessed;
  * a reviewed extent must FIT the span to its neighbour. An overlap PROVES one of
    the two models is wrong but not which, so BOTH are withheld and reported - the
    overlap list is a real reconstruction-defect worklist, not noise;
  * an allocation crossing a storage boundary is withheld.

Measured (see docs/data-attribution.md §3b for the table):
    matched_data   41258/274106 (15.05%)  ->  67080/279630 (23.99%)
    exact          2384 (unchanged - this only reshapes target data containers)

Folded COMDATs are now enrolled once PER OWNING UNIT (docs/data-attribution.md §3b-i):
a literal is emitted into every TU that uses it and folded by the linker onto one rva,
so all owners are correct and each target object gets its own copy. The delinker used
to reject that (`duplicate data RVA`); its data manifest now permits a folded-COMDAT
alias exactly as its section manifest always had (nix/patches/, upstream-pending).
    matched_data   67080/279630 (23.99%)  ->  77902/292484 (26.63%)   +10822 bytes
    exact          2386 (unchanged)

Withholding the two `data-unprovable-tail` rows (the .data rawsize-edge artifact -
docs/data-attribution.md §2) then took it to 80902/292476 (27.66%), +3000 more:
asserting `.data` for them had been breaking their containers.

Usage:
    python -m gruntz.build.data_manifest              # -> build/gen/delink_data_*manifest.tsv
    python -m gruntz.build.data_manifest --report     # print the withheld/defect lists
"""
from __future__ import annotations

import argparse
import bisect
import csv
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(REPO / "scripts"))

from gruntz.core.data_audit import read_pe, classify_pe_storage  # noqa: E402

SYMBOLS = REPO / "build/gen/symbol_names.csv"
EXE = REPO / "build/exe/GRUNTZ.EXE"
OUTPUT = REPO / "build/gen/delink_data_manifest.tsv"
SECTION_OUTPUT = REPO / "build/gen/delink_data_section_manifest.tsv"

# The delinker accepts a 9- or 10-column data manifest; the 10-column form adds
# `section_ordinal`, which places the definition in a candidate section declared by
# the --data-section-manifest. Both headers are read out of the delinker binary.
HEADER = ("name", "object", "rva", "size", "storage", "alignment",
          "section_ordinal", "section_offset", "scope", "provenance")
SECTION_HEADER = ("object", "ordinal", "name", "rva", "size", "alignment",
                  "characteristics", "comdat_selection", "associative_ordinal",
                  "storage", "provenance")
# retail PE storage class -> the delinker's storage keyword
STORAGE = {"rdata": "rdata", "data-initialized": "data", "data-loader-zero-tail": "bss"}


def _alignment(rva, size):
    """Largest power of two (<=8) that divides BOTH the retail address and the size.

    Derived from observed facts rather than assumed: the retail RVA's own alignment
    is a hard property of the shipped image.
    """
    a = 1
    while a < 8 and rva % (a * 2) == 0 and size % (a * 2) == 0:
        a *= 2
    return a


def string_rows(exe=EXE, base_dir=None):
    """Enrollable `??_C@` string-literal definitions + the withheld ones.

    A data manifest de-materializes any data it does not enroll, so a unit's string
    literals must be carried too or the functions referencing them stop matching.

    Both facts are PROVEN, never guessed:
      * the retail RVA comes from content-matching each retail data symbol's bytes
        against the candidate objs' `??_C@` pools (the same oracle synth_pdb uses to
        NAME them - cl.exe's own spelling for those exact bytes);
      * the owning object is the candidate obj that defines the literal.

    A payload emitted by SEVERAL units enrolls once PER OWNING UNIT. A COMDAT is by
    definition emitted into EVERY TU that uses the literal and folded by the linker to
    one surviving rva, so ALL of them are owners and each target object gets its own
    copy - exactly as our base objs do. There is nothing to attribute and no owner to
    choose. (The delinker used to reject one rva claimed by two objects; its data
    manifest now permits a folded-COMDAT alias, as its section manifest always had.
    See docs/data-attribution.md §3b.)

    Identical payloads at two retail RVAs would collide on one content-derived name;
    both are withheld.
    """
    import sys as _sys
    _sys.path.insert(0, str(REPO / "scripts/gruntz/build"))
    from coff_oracle import _Coff, Exe  # noqa: E402
    import synth_pdb as _synth  # noqa: E402

    base_dir = Path(base_dir or REPO / "build/objdiff/base")
    if not base_dir.is_dir() or not Path(exe).is_file():
        return [], []

    owners = defaultdict(dict)          # payload -> {unit: ??_C@ name}
    for obj in sorted(base_dir.glob("*.obj")):
        try:
            c = _Coff(obj)
        except Exception:
            continue
        for idx, value, secnum in c.iter_symbols():
            name = c.sym_name(idx)
            if name.startswith("??_C@") and secnum >= 1:
                cs = c.cstring(secnum, value)
                if cs is not None:
                    owners[cs][obj.stem] = name

    pe = read_pe(exe)
    _synth.read_sections(str(exe))
    rdata_syms, data_syms = _synth.read_data_symbols(str(exe))
    image = Exe(Path(exe))
    rows, withheld, by_name = [], [], defaultdict(list)
    for syms in (rdata_syms, data_syms):
        for rva, _name in syms:
            cs = image.cstring(rva + image.base)
            if cs is None or cs not in owners:
                continue
            units = owners[cs]
            size = len(cs) + 1                      # the payload plus its NUL
            start = classify_pe_storage(pe, rva)["class"]
            end = classify_pe_storage(pe, rva + size - 1)["class"]
            if start not in STORAGE or start != end:
                withheld.append((rva, next(iter(units.values())),
                                 "string storage %s not enrollable" % start))
                continue
            # EVERY unit that emitted the literal is an owner (that is what a COMDAT
            # is) - each gets its own copy of the one folded rva.
            for unit, name in sorted(units.items()):
                by_name[name].append(
                    {"name": name, "object": "%s.c" % unit, "rva": rva, "size": size,
                     "storage": STORAGE[start], "alignment": _alignment(rva, size),
                     "provenance": "candidate-COFF-string"})
    for name, group in by_name.items():
        # N owners of ONE rva is a fold and enrolls; one content-derived name over
        # SEVERAL retail rvas cannot address them all, so that group is withheld.
        addrs = {r["rva"] for r in group}
        if len(addrs) == 1:
            rows += group
        else:
            for r in group:
                withheld.append((r["rva"], name,
                                 "identical payload at %d retail RVAs" % len(addrs)))
    return rows, withheld


def compgen_rows(exe=EXE, table=None):
    """Reviewed `DATA_COMPGEN(rva, name, value)` claims -> per-owning-unit rows.

    labels.py already authority-checked every claim against the claiming TU's
    base obj (a string payload equals a `??_C@` COMDAT there; a float's bits sit
    in the TU's `$T` FP pool) and gated the fold rule: one compiler-generated
    identity per RVA, EXCEPT byte-identical string payloads, which /Gf pooling
    (implied by /O2) folds across TUs onto one retail RVA - those claims are the
    SAME datum and enroll once per owner, the folded-COMDAT alias form the
    delinker admits (docs/string-pooling.md). This just re-shapes the table and
    applies the usual storage screen.

    String claims take the candidate-COFF section shape (they own a whole
    `??_C@` COMDAT in the base obj - section_rows handles them exactly like the
    auto-inferred literals); float claims keep the legacy packed form.
    """
    table = Path(table or REPO / "build/gen/data_compgen.csv")
    if not table.is_file():
        return [], []
    pe = read_pe(exe)
    rows, withheld = [], []
    with table.open(newline="") as f:
        for r in csv.DictReader(f):
            rva, size = int(r["rva"], 16), int(r["size"], 16)
            start = classify_pe_storage(pe, rva)["class"]
            end = classify_pe_storage(pe, rva + size - 1)["class"]
            if start not in STORAGE or start != end:
                withheld.append((rva, r["name"],
                                 "DATA_COMPGEN storage %s not enrollable" % start))
                continue
            prov = ("candidate-COFF-string" if r["kind"] == "str"
                    else "src-DATA_COMPGEN")
            rows.append({"name": r["name"], "object": "%s.c" % r["unit"],
                         "rva": rva, "size": size, "storage": STORAGE[start],
                         "alignment": _alignment(rva, size), "provenance": prov})
    return rows, withheld


def retail_col_head(pe, vtable_rva):
    """4 if retail's COMDAT for this vtable opens with an `??_R4` COL pointer, else 0.

    Under `/GR` cl emits a polymorphic class's vtable COMDAT as the complete-object-
    locator POINTER (an unnamed word) at offset 0 followed by `??_7<class>@@6B@` at
    offset 4, and gives it SELECT_LARGEST so that copy beats a non-/GR TU's bare slot
    array. Whether RETAIL's surviving copy has that word is a property of the shipped
    image, not of how WE compile, so it is read out of the image:

      * the word at `vtable-4` must carry a PE base relocation (a COL pointer always
        does; the last slot of the PREVIOUS vtable does too, which is why the reloc
        alone proves nothing and the structure below has to be walked);
      * its target must be an `.rdata` Complete Object Locator - signature word 0,
        `pTypeDescriptor` at +12 pointing at a type descriptor whose name begins
        `.?A`, MSVC's type-descriptor spelling.

    Measured over the 222 `??_7` names with a slot map: 125 have a COL and put the
    symbol at offset 4 in our objs, 86 have neither. The crude reloc-only test
    mislabels 45 of the 86 (their `vtable-4` word is the preceding vtable's last
    slot), so the structural walk is required, not decorative.
    """
    sites = pe.reloc_sites                        # sorted and cached on the PE
    site = vtable_rva - 4
    if vtable_rva < 4 or bisect.bisect_left(sites, site) >= len(sites) \
            or sites[bisect.bisect_left(sites, site)] != site:
        return 0
    va = pe.u32(vtable_rva - 4)
    if va is None:
        return 0
    col = va - pe.image_base
    if pe.sec_name(col) != ".rdata" or pe.u32(col) != 0:
        return 0
    ptd = pe.u32(col + 12)
    if ptd is None:
        return 0
    name = pe.cstr(ptd - pe.image_base + 8, 64)
    return 4 if name and name.startswith(".?A") else 0


def vtable_rows(exe=EXE, base_dir=None):
    """Enrollable `??_7` vtable definitions + the withheld ones.

    A vtable is emitted EXACTLY like a `??_C@` string literal: cl gives each one its
    own `.rdata` COMDAT (align 8, `comdat=6` = SELECT_LARGEST), emits it into EVERY TU
    that needs it, and the linker folds them onto one surviving rva. So they enroll
    through the same fold path string_rows() uses - once per owning unit - and
    section_rows() rebuilds them in the candidate's shape.

    THE COMDAT IS NOT ALWAYS JUST THE VTABLE. Under `/GR` the `??_R4` complete-object-
    locator pointer sits at offset 0 and the `??_7` symbol at offset 4. Measured over
    every base obj: 101 distinct `??_7` at offset 0 versus 140 at offset 4 (369
    definitions). An `offset == 0` enrollment test therefore SILENTLY SKIPPED the /GR
    majority - those vtables were never materialized into a target object at all, so
    objdiff could not compare them and no defect in them was scorable. Both offsets
    enroll now; the section spans the whole COMDAT (rva `vtable_rva - offset`) so the
    COL word is carried and relocated with it.

    OUR OFFSET AND RETAIL'S ARE INDEPENDENT FACTS AND ARE BOTH CHECKED. retail_col_head
    reads the shipped image; the candidate COMDAT says how WE compiled it. Three
    outcomes, each evidence-driven:

      * they AGREE -> enroll, section placed at the retail COMDAT start.
      * we carry a COL word retail's vtable does not have (`cpp-rtti` where retail
        was not) -> the section would claim four retail bytes that belong to the
        PREVIOUS datum, so the row is WITHHELD and reported. Real, measured: the five
        `CFader*` vtables, whose `vtable-4` word is the `g_faderHalfPi` / `g_faderOne`
        float constant next to them.
      * retail has a COL and only SOME of our TUs emit it (the `cpp` / `cpp-rtti`
        split over one class) -> the with-COL emitters take the placed section and
        the others are enrolled UNPLACED (no section rva). Their COMDAT is a byte-
        exact copy of the retail slot array either way; leaving it unplaced just
        keeps two different section extents from claiming one retail range, which
        the delinker rejects outright.

    NOTHING IS FABRICATED. The extent is enrolled only where TWO INDEPENDENT sources
    agree: the retail RTTI slot map (vtable_hierarchy's registry, read out of the
    shipped image's COL/base-class arrays) and the candidate COMDAT cl.exe actually
    emitted. `offset + slot_count * 4 == candidate section size` or the row is
    withheld - the same contradiction check section_rows() applies to a literal whose
    candidate payload disagrees with its retail extent. A disagreement is a real
    mis-modelling signal (our class has the wrong number of virtuals), not noise.

    Only PRIMARY vtables (base_off 0) are enrolled; a secondary/MI vtable
    (`??_7<class>@@6B<base>@@@`) is left to the next pass. A primary is usually
    spelled `??_7<class>@@6B@`, but a template specialization is not - see the
    name-bridge below.
    """
    import sys as _sys
    _sys.path.insert(0, str(REPO / "scripts/gruntz/build"))
    from coff_oracle import _Coff  # noqa: E402
    from gruntz.core.pe import PE  # noqa: E402

    try:
        from gruntz.core.vtable_hierarchy import build_registry  # noqa: E402
        reg, _src = build_registry()
    except Exception as exc:                      # no Ghidra exports -> enroll nothing
        return [], [(0, "??_7*", "vtable registry unavailable (%s)" % type(exc).__name__)]

    # class -> (primary vtable rva, slot count), straight from the RTTI slot map.
    primary, by_rva = {}, {}
    for name, ci in reg.items():
        p = ci.vtables.get(0)
        if p is not None:
            primary["??_7%s@@6B@" % name] = (p[0], p[1])
            by_rva[p[0]] = (p[0], p[1])

    # A TEMPLATE SPECIALIZATION'S KEY CANNOT BE REBUILT FROM ITS RTTI NAME. The
    # registry keys off the decorated name in the `??_R0` type descriptor, decoded on
    # `@` as a nested scope, so `.?AV?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@` becomes
    # `PAU1::PAUPLAYLISTINFOSTRUCT::?$CArray` and "??_7%s@@6B@" spells a symbol no
    # object defines. Both of the binary's template vtables missed on that alone and
    # were withheld as "no primary-vtable slot map" - the CArray one then had to be
    # hand-routed through vtables_game.csv's `unit` column, which is exactly the
    # attribution that rotted when its unit was dissolved.
    #
    # The manual catalog already states the mangled name and its retail rva, so use
    # it as the name->rva bridge and take the slot count from the RTTI map as usual.
    # Two independent sources still have to agree - the catalog rva must BE a base-0
    # vtable in the registry - and section_rows re-checks the extent against the
    # COMDAT cl actually emitted, so nothing here is asserted on the catalog's word.
    try:
        from gruntz.core import vtable_catalog  # noqa: E402
        catalog = vtable_catalog.game_rows() + vtable_catalog.library_rows()
    except Exception:
        catalog = []
    for row in catalog:
        hit = by_rva.get(row["rva"])
        # A secondary/MI table may deliberately ALIAS a primary's rva; bridging it
        # would hand it the primary's slot map. Still out of scope, still withheld.
        if hit is None or row.get("kind") == "secondary" \
                or vtable_catalog.secondary_classes(row["name"]) is not None:
            continue
        primary.setdefault(row["name"], hit)

    base_dir = Path(base_dir or REPO / "build/objdiff/base")
    # name -> {offset-in-COMDAT: [(unit, candidate section), ...]}
    emitters = defaultdict(lambda: defaultdict(list))
    for obj in sorted(base_dir.glob("*.obj")):
        try:
            c = _Coff(obj)
        except Exception:
            continue
        for sec in c.section_table:
            members = c.defined_symbols(sec["index"])
            if len(members) != 1:
                continue
            offset, name = members[0]
            if name.startswith("??_7"):
                emitters[name][offset].append((obj.stem, sec))

    pe = PE(str(exe))
    rows, withheld = [], []
    for name, by_offset in sorted(emitters.items()):
        hit = primary.get(name)
        if hit is None:                            # secondary/MI vtable, or no RTTI
            withheld.append((0, name, "no primary-vtable slot map for this name"))
            continue
        rva, slots = hit
        head = retail_col_head(pe, rva)
        for offset, group in sorted(by_offset.items()):
            if offset > head:
                withheld.append((rva, name,
                                 "candidate COMDAT opens with a ??_R4 COL word that "
                                 "retail's vtable does not have (%d obj(s): %s)"
                                 % (len(group), ", ".join(u for u, _ in group[:4]))))
                continue
            for unit, sec in group:
                if sec["size"] != offset + slots * 4:
                    withheld.append((rva, name,
                                     "candidate section 0x%x != 0x%x + RTTI %d slots"
                                     % (sec["size"], offset, slots)))
                    continue
                rows.append({"name": name, "object": "%s.c" % unit, "rva": rva,
                             "size": slots * 4, "vtable_offset": offset,
                             # Only the emitters that agree with retail's COMDAT start
                             # get a placed section; see the docstring.
                             "section_placed": offset == head, "storage": "rdata",
                             # cl.exe's own COMDAT alignment, NOT one derived from the
                             # rva: the placed and unplaced copies of a folded COMDAT
                             # must agree on every column but the object or the
                             # delinker reads them as two claims on one rva.
                             "alignment": sec["alignment"],
                             "provenance": "candidate-COFF-vtable"})
    return rows, withheld


#: MSVC's RTTI records, keyed by symbol prefix -> (field offsets we walk).
#: `??_R4` complete-object-locator: +0xc pTypeDescriptor, +0x10 pClassHierarchyDescriptor.
#: `??_R3` class-hierarchy descriptor: +0x8 numBaseClasses, +0xc pBaseClassArray.
#: `??_R2` base-class array: numBaseClasses pointers to `??_R1` (plus cl's NUL word).
#: `??_R1` base-class descriptor: +0x0 pTypeDescriptor.
#: `??_R0` type descriptor: +0x0 the `type_info` vftable, +0x8 the `.?A...` name.
COL_TYPE_DESC, COL_HIERARCHY = 0x0c, 0x10
CHD_NUM_BASES, CHD_BASE_ARRAY = 0x08, 0x0c


def rtti_rows(exe=EXE, base_dir=None):
    """Enrollable `??_R4`/`??_R3`/`??_R2`/`??_R1`/`??_R0` definitions + the withheld.

    Under `/GR` the word at offset 0 of a vtable COMDAT points at the class's
    complete-object locator, and from there the whole RTTI graph hangs off `.rdata$r`
    (`??_R1`..`??_R4`) and `.data` (`??_R0`, whose `spare` field the runtime writes).
    None of it was in the manifest, so the delinker had no definition to name that
    word by and `hypothesis_owner_and_addend_for_rva` fell back to the closest
    `.rdata` definition - the vtable itself, named `??_7<class>@@6B@+<addend>`. The
    same fallback mislabels every interior RTTI pointer.

    THE NAMES ARE READ OFF cl's OWN RELOCATIONS, the `apply_string_names` oracle
    applied to RTTI. Both graphs are walked IN PARALLEL from one anchor per class -
    the retail image gives the ADDRESSES (`vtable-4` -> COL -> hierarchy -> base-class
    array -> descriptors -> type descriptors) and the base object that emitted the
    same vtable gives the NAMES at the identical offsets. Nothing is mangled by hand:
    a `??_R1`'s spelling encodes its PMD and a `??_R0`'s encodes the decorated type
    name, and re-deriving either would be a guess where cl already wrote the answer.

    EVERY node is then proven byte-for-byte: the retail bytes at the walked address
    must equal the candidate COMDAT's payload with the relocated dwords masked out
    (the type-descriptor name string, the PMD displacements, the base counts). A node
    that fails, or whose name the walk reaches at two different addresses, is
    withheld - never enrolled on the strength of the walk alone.
    """
    import sys as _sys
    _sys.path.insert(0, str(REPO / "scripts/gruntz/build"))
    from coff_oracle import _Coff  # noqa: E402
    from gruntz.core.pe import PE  # noqa: E402

    try:
        from gruntz.core.vtable_hierarchy import build_registry  # noqa: E402
        reg, _src = build_registry()
    except Exception as exc:
        return [], [(0, "??_R*", "vtable registry unavailable (%s)" % type(exc).__name__)]

    base_dir = Path(base_dir or REPO / "build/objdiff/base")
    # name -> {unit: {"sec": section row, "payload": bytes, "rel": {site: name}}}
    defs = defaultdict(dict)
    # ??_7 name -> {unit: the ??_R4 name its COMDAT's offset-0 word points at}
    anchors = defaultdict(dict)
    for obj in sorted(base_dir.glob("*.obj")):
        try:
            c = _Coff(obj)
        except Exception:
            continue
        for sec in c.section_table:
            members = c.defined_symbols(sec["index"])
            if len(members) != 1:
                continue
            offset, name = members[0]
            if name.startswith("??_R"):
                defs[name][obj.stem] = {
                    "sec": sec, "payload": c.section_payload(sec["index"]),
                    "rel": c.relocations(sec["index"])}
            elif name.startswith("??_7") and offset == 4:
                head = c.relocations(sec["index"]).get(0)
                if head and head.startswith("??_R4"):
                    anchors[name][obj.stem] = head

    pe = PE(str(exe))
    primary = {}
    for name, ci in reg.items():
        p = ci.vtables.get(0)
        if p is not None:
            primary["??_7%s@@6B@" % name] = p[0]

    located, withheld = {}, []          # name -> rva

    def place(name, rva):
        """Record one walked node; a name reached at two addresses is a defect."""
        prev = located.get(name)
        if prev is None:
            located[name] = rva
            return True
        if prev != rva:
            withheld.append((rva, name, "RTTI walk reaches one name at two RVAs"))
            return False
        return True

    for vtable, units in sorted(anchors.items()):
        rva = primary.get(vtable)
        if rva is None or retail_col_head(pe, rva) != 4:
            continue                    # no slot map, or retail is not /GR here
        col = pe.u32(rva - 4) - pe.image_base
        unit = sorted(units)[0]

        def shape(name):
            """The candidate COMDAT for `name`, preferring the anchor's own unit."""
            group = defs.get(name)
            if not group:
                return None
            return group.get(unit) or group[sorted(group)[0]]

        chain, ok = [], True
        node = shape(units[unit])
        if node is None:
            withheld.append((col, units[unit], "no candidate COMDAT for the COL"))
            continue
        chain.append((units[unit], col))
        r3name = node["rel"].get(COL_HIERARCHY)
        for field in (COL_TYPE_DESC, COL_HIERARCHY):
            nm, ptr = node["rel"].get(field), pe.u32(col + field)
            if nm is None or ptr is None:
                ok = False
                break
            chain.append((nm, ptr - pe.image_base))
        if not ok or r3name is None:
            withheld.append((col, units[unit], "COL record is not walkable"))
            continue
        chd, r3 = shape(r3name), pe.u32(col + COL_HIERARCHY) - pe.image_base
        r2name = chd["rel"].get(CHD_BASE_ARRAY) if chd else None
        bases = pe.u32(r3 + CHD_NUM_BASES)
        r2 = pe.u32(r3 + CHD_BASE_ARRAY)
        if r2name is None or bases is None or r2 is None:
            withheld.append((r3, r3name or "??_R3?", "hierarchy record is not walkable"))
            continue
        r2 -= pe.image_base
        chain.append((r2name, r2))
        array = shape(r2name)
        for i in range(bases):
            r1name = array["rel"].get(i * 4) if array else None
            r1 = pe.u32(r2 + i * 4)
            if r1name is None or r1 is None:
                ok = False
                break
            r1 -= pe.image_base
            chain.append((r1name, r1))
            desc = shape(r1name)
            r0name = desc["rel"].get(0) if desc else None
            r0 = pe.u32(r1)
            if r0name is None or r0 is None:
                ok = False
                break
            chain.append((r0name, r0 - pe.image_base))
        if not ok:
            withheld.append((r2, r2name, "base-class array is not walkable"))
            continue
        for name, addr in chain:
            place(name, addr)

    # Every located node is re-proven against the shipped bytes before it enrolls.
    rows, storage_pe = [], read_pe(exe)
    for name, rva in sorted(located.items()):
        group = defs.get(name) or {}
        sizes = {d["sec"]["size"] for d in group.values()}
        if len(sizes) != 1:
            withheld.append((rva, name, "candidate COMDATs disagree on the extent"))
            continue
        size = sizes.pop()
        off = pe.off(rva)
        if off is None:
            withheld.append((rva, name, "RTTI record is not mapped in the image"))
            continue
        sample = next(iter(group.values()))
        want, got = bytearray(sample["payload"][:size]), bytearray(pe.data[off:off + size])
        if len(want) != size or len(got) != size:
            withheld.append((rva, name, "RTTI record is truncated"))
            continue
        for site in sample["rel"]:      # the pointers differ by construction
            want[site:site + 4] = got[site:site + 4] = b"\0\0\0\0"
        if want != got:
            withheld.append((rva, name, "retail bytes contradict the candidate record"))
            continue
        storage = classify_pe_storage(storage_pe, rva)["class"]
        if storage not in STORAGE:
            withheld.append((rva, name, "RTTI storage %s is not enrollable" % storage))
            continue
        for unit, d in sorted(group.items()):
            rows.append({"name": name, "object": "%s.c" % unit, "rva": rva,
                         "size": size, "storage": STORAGE[storage],
                         "alignment": d["sec"]["alignment"], "section_placed": True,
                         "provenance": "candidate-COFF-rtti"})
    return rows, withheld


#: cl's floating-point literal pool. `$T<n>` is a per-TU emission counter with no
#: source spelling at all, and `canonicalize_data_symbols.VOLATILE_T` content-
#: addresses BOTH sides of the comparison to `$anon_f{32,64}_<bits>`, so the digits
#: never reach objdiff. The retail ADDRESS is what has to be proven, and it is.
FP_POOL_NAME = re.compile(r"^\$T[0-9]+$")
#: `IMAGE_REL_I386_DIR32` - the only relocation type that makes the linker write an
#: absolute address, hence the only one that appears in the PE's `.reloc` directory.
COFF_DIR32 = 0x0006
#: `IMAGE_SCN_MEM_EXECUTE`.
MEM_EXECUTE = 0x20000000


def fp_pool_rows(exe=EXE, base_dir=None, symbols=SYMBOLS):
    """`$T<n>` FP-pool constants, ADDRESSED OUT OF RETAIL'S OWN RELOCATION TABLE.

    A unit's ordinary `.rdata`/`.data` is admitted only when EVERY member has a
    proven extent (ordinary_sections), and cl's floating-point pool is the member
    that has no source pin to give it one: `DATA()` needs a VarDecl and
    `DATA_COMPGEN` needs a value expression, but a `$T` entry is neither - it is the
    compiler's own spill of a literal, numbered by a per-TU counter. 18 candidate
    sections were rejected for that single missing row.

    CONTENT MATCHING CANNOT ANSWER IT. That is the oracle string_rows() uses, and it
    works there because a `??_C@` payload is /Gf-pooled to exactly one address. FP
    pools are NOT pooled, so `0.5` sits at a dozen retail rvas at once and its bits
    identify nothing. Worse, a content-derived address is self-confirming: we would
    copy the retail bytes from the address we found BY those bytes, so a wrong
    constant in our source would still score 100.

    RETAIL'S RELOCATION TABLE ANSWERS IT, with no disassembly and no guessing:

      * the PE `.reloc` directory lists EVERY site the linker wrote an absolute
        address into, so retail's DIR32 sites inside [fn_rva, fn_rva+size) are known
        exactly - and so are our base obj's, from its COFF relocations;
      * equal counts pair the two lists positionally;
      * THE PAIRING PROVES ITSELF. Every base symbol whose rva we already know must
        equal the address retail wrote at its partner site (plus the addend in our
        own instruction bytes). A single disagreement means the two instruction
        streams do not correspond, and the whole function is discarded - which is
        what happens to the ones our reconstruction still gets wrong;
      * only then is a site whose base symbol is a `$T` read off, and the address is
        re-proven against the shipped bytes: retail's payload there must EQUAL the
        candidate's. A wrong constant in our source therefore withholds the row
        instead of hiding behind it (measured: `fadereffects $T28704` is a true
        double pi where retail's site loads the float-widened one at 0x1f08a8).

    THE MANIFEST NAME IS `$T<decimal rva>`, NOT cl's COUNTER. That is already
    labels.py's spelling for a `DATA_COMPGEN` float claim, and it has to be: our TU
    partition is a reconstruction, so two of our units can both spill a literal that
    retail's TU boundary put in ONE pool slot (`kitchenslime $T35488` and
    `pathhazard $T35508` are both 0x1ea400), and the delinker admits N objects at one
    rva only under ONE name. Addressing the group by its rva makes the fold explicit
    AND makes these rows coalesce with the 43 `DATA_COMPGEN` pins that already state
    the same slots - the two channels were previously two claims on one address, and
    `candidates()` withheld BOTH as an overlap.

    Each row also carries `member`, cl's real symbol name for the constant in THAT
    object. `ordinary_sections` matches the section's COFF members by name, so
    without it the pin and the member never met - which is why 43 pinned pool entries
    still left their sections rejected. The delinker never sees `member`.
    """
    import struct  # noqa: E402
    import sys as _sys
    _sys.path.insert(0, str(REPO / "scripts/gruntz/build"))
    from coff_oracle import _Coff  # noqa: E402
    from gruntz.core.pe import PE, IMAGEBASE  # noqa: E402

    base_dir = Path(base_dir or REPO / "build/objdiff/base")
    if not base_dir.is_dir() or not Path(exe).is_file():
        return [], []
    pe = PE(str(exe))
    storage_pe = read_pe(exe)
    sites = pe.reloc_sites

    known, fn_extent, pins = {}, {}, defaultdict(list)
    with Path(symbols).open(newline="") as f:
        for r in csv.DictReader(l for l in f if not l.lstrip().startswith("#")):
            rva = int(r["rva"], 16)
            known[r["name"]] = rva
            if (r.get("kind") or "") == "func" and (r.get("size") or "").strip():
                fn_extent[r["name"]] = (rva, int(r["size"], 16))
            elif (r.get("kind") or "") == "data" and FP_POOL_NAME.fullmatch(r["name"]) \
                    and (r.get("size") or "").strip():
                pins[(r.get("unit") or "").strip()].append((rva, int(r["size"], 16)))

    rows, withheld = [], []
    for obj in sorted(base_dir.glob("*.obj")):
        try:
            c = _Coff(obj)
        except Exception:
            continue

        pool = {}                       # $T name -> (storage, offset, payload, size)
        for sec in c.section_table:
            storage = ORDINARY_STORAGE.get(sec["name"])
            if storage is None or sec["characteristics"] & LNK_COMDAT:
                continue
            members = c.section_members(sec["index"])
            offsets = sorted(o for o, _n, _s in members)
            payload = c.section_payload(sec["index"])[:sec["size"]]
            for off, name, _scl in members:
                if not FP_POOL_NAME.fullmatch(name):
                    continue
                end = next((o for o in offsets if o > off), sec["size"])
                pool[name] = (storage, off, payload[off:end], end - off)
        if not pool:
            continue

        votes = defaultdict(set)
        for sec in c.section_table:
            if not sec["characteristics"] & MEM_EXECUTE:
                continue
            ptr, count, first, rel = sec["reloc_offset"], sec["reloc_count"], 0, {}
            if not ptr:
                continue
            if sec["characteristics"] & 0x01000000 and count == 0xFFFF:
                count = struct.unpack_from("<I", c.buf, ptr)[0]
                first = 1
            for i in range(first, count):
                site, idx, typ = struct.unpack_from("<IIH", c.buf, ptr + i * 10)
                if typ == COFF_DIR32:
                    rel[site] = c.sym_name(idx)
            if not rel:
                continue
            text = c.section_payload(sec["index"])
            for off, name in c.defined_symbols(sec["index"]):
                hit = fn_extent.get(name)
                if hit is None:
                    continue
                rva, size = hit
                mine = sorted((s, n) for s, n in rel.items() if off <= s < off + size)
                lo, hi = bisect.bisect_left(sites, rva), bisect.bisect_left(sites, rva + size)
                theirs = sites[lo:hi]
                if not mine or len(mine) != len(theirs):
                    continue
                found, corroborated = [], True
                for (site, sym), target in zip(mine, theirs):
                    at = pe.off(target)
                    if at is None:
                        corroborated = False
                        break
                    value = struct.unpack("<I", pe.data[at:at + 4])[0] - IMAGEBASE
                    if FP_POOL_NAME.fullmatch(sym):
                        found.append((sym, value))
                        continue
                    anchor = known.get(sym)
                    if anchor is None:      # not ours to check (import, thunk, ...)
                        continue
                    addend = struct.unpack("<I", text[site:site + 4])[0]
                    if value != anchor + addend:
                        corroborated = False
                        break
                if corroborated:
                    for sym, value in found:
                        votes[sym].add(value)

        def emit(member, rva, storage, size, want, how):
            """One proven pool entry, addressed by its rva and byte-re-proven."""
            at = pe.off(rva)
            if at is None or pe.data[at:at + size] != want:
                withheld.append((rva, member,
                                 "retail bytes contradict the candidate FP constant"))
                return
            start = classify_pe_storage(storage_pe, rva)["class"]
            end = classify_pe_storage(storage_pe, rva + size - 1)["class"]
            if STORAGE.get(start) != storage or start != end:
                withheld.append((rva, member, "FP-pool storage %s is not %s"
                                 % (start, storage)))
                return
            rows.append({"name": "$T%d" % rva, "member": member,
                         "object": "%s.c" % obj.stem, "rva": rva, "size": size,
                         "storage": storage, "alignment": _alignment(rva, size),
                         "provenance": how})

        stranded = []
        for member in sorted(pool):
            storage, _off, want, size = pool[member]
            seen = votes.get(member) or set()
            if len(seen) == 1:
                emit(member, next(iter(seen)), storage, size, want,
                     "retail-reloc-fp-pool")
            elif seen:
                withheld.append((0, member, "referrers disagree on the rva"))
            else:
                stranded.append(member)

        # A `DATA_COMPGEN` float claim in this unit already STATES a pool slot, but
        # under labels.py's `$T<rva>` spelling - so `ordinary_sections`, which keys
        # off the COFF member name, never found it. Bridge the two when the claim and
        # exactly one still-unaddressed member agree on the extent AND on the bytes,
        # and the pairing is unique in both directions; anything ambiguous is left.
        taken = {r["rva"] for r in rows}
        pairs = defaultdict(list)
        for rva, size in pins.get(obj.stem, ()):
            if rva in taken:
                continue
            at = pe.off(rva)
            if at is None:
                continue
            want = pe.data[at:at + size]
            for member in stranded:
                if pool[member][3] == size and pool[member][2] == want:
                    pairs[rva].append(member)
        claims = Counter(m for ms in pairs.values() for m in ms)
        for rva, ms in sorted(pairs.items()):
            if len(ms) != 1 or claims[ms[0]] != 1:
                withheld.append((rva, ms[0] if ms else "$T?",
                                 "DATA_COMPGEN pin matches %d pool members" % len(ms)))
                continue
            storage, _off, want, size = pool[ms[0]]
            emit(ms[0], rva, storage, size, want, "src-DATA_COMPGEN-fp-pool")
        for member in stranded:
            if not claims.get(member):
                withheld.append((0, member, "no relocation-paired referrer"))

    # Copies of one slot must agree on the extent, and one object may claim it once.
    by_rva = defaultdict(list)
    for r in rows:
        by_rva[r["rva"]].append(r)
    kept = []
    for rva, group in sorted(by_rva.items()):
        if len({r["size"] for r in group}) != 1:
            withheld += [(rva, r["member"], "FP-pool copies disagree on the extent")
                         for r in group]
            continue
        by_object = {}
        for r in group:
            by_object.setdefault(r["object"], r)
        kept += list(by_object.values())
    return kept, withheld


def _candidate_member_storage(base_dir=None):
    """{(object, symbol): "data"|"rdata"} - the storage cl.exe ACTUALLY gave it.

    An independent fact from the retail PE's own classification, and the one that
    decides whether a named static could have been folded onto a pooled literal:
    the linker folds identical COMDATs, it does not move a `.rdata` const array into
    `.data`. Read from every section, COMDAT or not, and via `section_members` -
    `defined_symbols` lists only the EXTERNALS, and cl makes a function-local
    `static` class STATIC, which is exactly the `_name$S<n>` family this screens.
    """
    import sys as _sys
    _sys.path.insert(0, str(REPO / "scripts/gruntz/build"))
    from coff_oracle import _Coff  # noqa: E402

    base_dir = Path(base_dir or REPO / "build/objdiff/base")
    out = {}
    for obj in sorted(base_dir.glob("*.obj")):
        try:
            c = _Coff(obj)
        except Exception:
            continue
        for sec in c.section_table:
            storage = ORDINARY_STORAGE.get(sec["name"])
            if storage is None:
                continue
            for _off, name, _scl in c.section_members(sec["index"]):
                out["%s.c" % obj.stem, name] = storage
    return out


def candidates(symbols=SYMBOLS, exe=EXE):
    """Enrollable rows + the withheld ones, with a reason for each."""
    pe = read_pe(exe)
    rows, withheld = [], []
    with Path(symbols).open(newline="") as f:
        for r in csv.DictReader(l for l in f if not l.lstrip().startswith("#")):
            if (r.get("kind") or "") != "data":
                continue
            name, unit = r["name"], (r.get("unit") or "").strip()
            size_s = (r.get("size") or "").strip()
            rva = int(r["rva"], 16)
            if not size_s:
                withheld.append((rva, name, "no proven extent (type not resolvable)"))
                continue
            size = int(size_s, 16)
            start = classify_pe_storage(pe, rva)["class"]
            end = classify_pe_storage(pe, rva + size - 1)["class"]
            if start not in STORAGE:
                withheld.append((rva, name, "storage %s is not enrollable" % start))
                continue
            if start != end:
                withheld.append((rva, name, "extent crosses %s -> %s" % (start, end)))
                continue
            rows.append({"name": name, "object": "%s.c" % unit, "rva": rva, "size": size,
                         "storage": STORAGE[start], "alignment": _alignment(rva, size),
                         "provenance": "src-DATA-sizeof"})

    # The compiler-emitted string literals of each unit. Without them a manifest
    # de-materializes the literals its objects reference (measured: -3 exact).
    strings, string_withheld = string_rows(exe=exe)
    rows += strings
    withheld += string_withheld

    # The compiler-emitted vtables - same fold, same one-COMDAT-per-symbol shape.
    vtables, vtable_withheld = vtable_rows(exe=exe)
    rows += vtables
    withheld += vtable_withheld

    # The RTTI graph each /GR vtable's offset-0 word points into.
    rtti, rtti_withheld = rtti_rows(exe=exe)
    rows += rtti
    withheld += rtti_withheld

    # The reviewed DATA_COMPGEN claims (one per owning unit, labels.py-gated).
    compgen, compgen_withheld = compgen_rows(exe=exe)
    rows += compgen
    withheld += compgen_withheld

    # cl's `$T` FP pool - the one member of an ordinary section that no source pin
    # can reach, addressed out of retail's own relocation table. A slot some OTHER
    # channel already names is left to that channel: two names at one rva withhold
    # BOTH, so a pool row must never displace the row that is already enrolled. Three
    # such collisions are real modelling news, not noise - `gruntsteps $T45771` and
    # `grunt _g_slopeNegHalf$S41549` are the same retail double, spelled as an inline
    # literal in one of our TUs and as a named static in the other.
    fp, fp_withheld = fp_pool_rows(exe=exe, symbols=symbols)
    withheld += fp_withheld
    spoken_for = {r["rva"]: r["name"] for r in rows}
    for r in fp:
        other = spoken_for.get(r["rva"])
        if other in (None, r["name"]):
            rows.append(r)
        else:
            withheld.append((r["rva"], r["member"],
                             "the pool slot is already named %s" % other))

    # One definition may be stated by several provenances - the symbol_names
    # representative row, the auto-inferred candidate-COFF string/vtable, and
    # the explicit DATA_COMPGEN claim are the SAME datum. Collapse them,
    # preferring any candidate-COFF-* statement: those take the candidate
    # SECTION shape in section_rows, and a fold must not mix section-form and
    # legacy-form rows at one rva (the delinker rejects that as a duplicate).
    # Anything still duplicated after this is a real defect the checks below own.
    seen, deduped = {}, []
    for r in rows:
        key = (r["name"], r["object"], r["rva"], r["size"])
        prev = seen.get(key)
        if prev is None:
            seen[key] = r
            deduped.append(r)
        elif (not prev["provenance"].startswith("candidate-COFF")
                and r["provenance"].startswith("candidate-COFF")):
            prev.update(r)
        elif r.get("member") and "member" not in prev:
            # The `$T<rva>` pin and the pool row are one datum; only the pool row
            # knows cl's member name, and ordinary_sections needs it.
            prev["member"] = r["member"]
    rows = deduped

    # A NAMED STATIC THAT /Gf FOLDED ONTO A POOLED LITERAL IS NOT AN OVERLAP.
    # `static const char s_strLBrack[] = "["` in one TU and the `"["` literal every
    # other TU spells `??_C@_01KHLB@?$FL?$AA@` are DIFFERENT source objects that the
    # linker put at ONE rva (/Gf, implied by /O2), so both claims are true and the
    # extents are EXACTLY equal - which is what tells them apart from a real
    # contradiction, where the extents merely intersect. Read as an overlap they
    # withheld each other, and with the pooled side gone its whole owner set lost the
    # literal: 44 groups, one of them owned by 47 objects.
    #
    # The pooled literal keeps the authoritative claim - it is the identity every
    # referencing object's relocation must resolve to, and it has the owners - and the
    # named static is re-provenanced `provisional-`, which in the delinker means
    # exactly "carve this definition, but do not let it own the address"
    # (`address_authoritative` gates both `proved_rvas` and
    # `owner_and_addend_for_rva`; `object_files` carves it either way). So the static
    # gets its bytes into its object, the pool keeps naming the rva, and nothing is
    # asserted twice.
    # AN ALIAS IS ONLY A FOLD WHEN BOTH SIDES LIVE IN THE SAME STORAGE. cl puts a
    # bare literal in a `.data` COMDAT (VC5 pools with /Gf but does not make literals
    # read-only - there is no /GF here), so a named static that our source declares
    # `const` compiles into `.rdata` and CANNOT be what the linker folded onto a
    # `.data` literal: the two never met. Such a pair is not a fold at all, it is a
    # mis-modelled declaration - retail's code used the literal directly and had no
    # array - and enrolling it appends a phantom tail to the object's first `.data`
    # section. Measured: it broke `warlord` 100 -> 83.07, `gruntsteps` 100 -> 76.06,
    # `triggermgr`, `gruntassetloaders`, `directsoundmgr` and `ddrawsubmgrleaf`,
    # every one of them a `.rdata` candidate against a `.data` retail address.
    # A pair the storage class REFUTES is not withheld symmetrically. The literal
    # agrees with the PE (many objects emit it, into the same storage retail used);
    # the named static does not, and one claim has to be wrong. So the pin loses and
    # the pool keeps the address - the pin is reported as the modelling defect it is.
    candidate_storage = _candidate_member_storage()
    alias_of, refuted = {}, set()
    by_extent = defaultdict(list)
    for r in rows:
        by_extent[(r["rva"], r["size"])].append(r)
    for (rva, _size), group in by_extent.items():
        names = {r["name"] for r in group}
        pooled = {n for n in names if n.startswith("??_C@")}
        if len(names) != 2 or len(pooled) != 1:
            continue
        literal = next(iter(pooled))
        for r in group:
            if r["name"] == literal:
                continue
            mine = candidate_storage.get((r["object"], r["name"]))
            if mine != r["storage"]:
                refuted.add(r["name"])
                withheld.append((rva, r["name"],
                                 "our %s copy cannot be the %s literal %s it is "
                                 "pinned onto" % (mine or "?", r["storage"], literal)))
                continue
            r["provenance"] = "provisional-pooled-literal-alias"
            alias_of[r["name"]] = literal
    rows = [r for r in rows if r["name"] not in refuted]

    # The N owners of a folded COMDAT all claim ONE rva, so collapse each fold to a
    # single extent before the neighbour check - otherwise the copies read as mutual
    # overlaps. Every real extent still has to fit the span to its neighbour; an
    # overlap proves one of the pair is mis-modelled but not which, so neither is
    # enrolled. This scans DEFINITION extents (the delinker's own data-manifest
    # check); the placed-SECTION extents, which for a /GR vtable start four bytes
    # earlier at its `??_R4` COL word, are screened in section_rows().
    # A `provisional-` alias claims no address, so it sits out this scan exactly as
    # the delinker's own adjacent-pair check skips it.
    rows, aliases = ([r for r in rows if r["name"] not in alias_of],
                     [r for r in rows if r["name"] in alias_of])
    rows.sort(key=lambda x: (x["rva"], x["size"], x["name"]))
    extents = []                       # [{rva, size, name, copies: [row, ...]}]
    for r in rows:
        if extents and (extents[-1]["rva"], extents[-1]["size"], extents[-1]["name"]) \
                == (r["rva"], r["size"], r["name"]):
            extents[-1]["copies"].append(r)
        else:
            extents.append({"rva": r["rva"], "size": r["size"], "name": r["name"],
                            "copies": [r]})
    bad, overlaps = set(), []
    for i in range(len(extents) - 1):
        a, b = extents[i], extents[i + 1]
        if a["rva"] + a["size"] > b["rva"]:
            bad.add(i)
            bad.add(i + 1)
            overlaps.append((a, b, a["rva"] + a["size"] - b["rva"]))
    enrolled = [r for i, e in enumerate(extents) if i not in bad for r in e["copies"]]
    for i in sorted(bad):
        withheld.append((extents[i]["rva"], extents[i]["name"], "overlaps a neighbour"))
    # The delinker admits N objects claiming one rva under one name (a folded COMDAT)
    # and nothing looser: one name must still resolve to one extent, one rva to one
    # name, and no object may define a name twice.
    extent_of = defaultdict(set)
    name_at = defaultdict(set)
    for r in enrolled:
        extent_of[r["name"]].add((r["rva"], r["size"]))
        name_at[r["rva"]].add(r["name"])
    per_object = Counter((r["name"], r["object"]) for r in enrolled)
    final = []
    for r in enrolled:
        if len(extent_of[r["name"]]) > 1:
            withheld.append((r["rva"], r["name"], "duplicate name in manifest"))
        elif len(name_at[r["rva"]]) > 1:
            withheld.append((r["rva"], r["name"], "duplicate rva in manifest"))
        elif per_object[(r["name"], r["object"])] > 1:
            withheld.append((r["rva"], r["name"], "duplicate definition in one object"))
        else:
            final.append(r)
    # A pooled-literal alias enrolls only if the literal it aliases did: it exists to
    # give ONE object the bytes of a slot the pool already owns, and without that
    # owner it would be an unbacked second claim on the address.
    live = {r["name"] for r in final}
    for r in aliases:
        if alias_of[r["name"]] in live:
            final.append(r)
        else:
            withheld.append((r["rva"], r["name"], "aliases a withheld pooled literal"))
    return final, withheld, overlaps


def section_rows(rows, base_dir=None):
    """Candidate COMDAT sections for the enrolled string literals + the withheld.

    cl.exe emits every `??_C@` literal as its OWN COMDAT section holding just that
    one symbol at offset 0. The delinked target instead PACKS a unit's literals into
    a single `.data` blob, so `objdiff-cli report generate` (which runs with
    combine_data_sections=true) diffs a packed target section against the base's
    combined-COMDAT layout: the payloads are all present but at shifted offsets, so
    the section lands ~99% and NEVER at the 100.0 that `matched_data` requires
    (report.rs credits a section's bytes all-or-nothing).

    Handing the delinker these rows makes it rebuild the target in the CANDIDATE's
    shape - one COMDAT per literal - so both sides combine to the same layout.

    Nothing here is invented: `rva`/`size` stay the PROVEN retail extent from
    string_rows(), and name/alignment/characteristics/COMDAT selection are read out
    of the candidate COFF that cl.exe actually emitted.

    A section's rva is where its COMDAT STARTS in retail, which for a /GR vtable is
    four bytes before the `??_7` symbol - the `??_R4` complete-object-locator pointer
    is part of the COMDAT, so the delinker copies and relocates it too.

    A ROW THAT CANNOT CLAIM A RETAIL RANGE STILL GETS A SECTION - a NON-AFFINE one.
    vtable_rows() marks a copy unplaced when its TU emits the class without the COL
    word retail's surviving copy has: its 0x14-byte COMDAT is the byte-exact retail
    slot array, but the range [rva, rva+0x14) is already claimed by the with-COL
    emitters' [rva-4, rva+0x14) section, and the delinker BAILS on two placed
    sections overlapping. Leaving such a row in the legacy `-` allocation form is
    NOT free, and that was the bug this function used to ship: the delinker appends
    a legacy row to the object's FIRST manifest section of the same storage
    (object_files.rs picks it as `rdata_section_id`/`data_section_id`), 8-aligned,
    so a real COMDAT grew a phantom tail - `interfaceobject`'s `.rdata` came out
    0x2c (InterfaceObject's vtable + 4 pad + CObject's) where cl emits two separate
    0x14 COMDATs, and every extent behind it shifted.

    The delinker already models exactly this case: a section row with `rva = -` is
    "non-affine" - it retains the candidate COFF shape (name, size, alignment,
    COMDAT selection) but is NOT assigned a retail range, so the overlap check skips
    it, `add_data_definition` copies the definition's own retail payload into it and
    `add_legacy_data_relocations` relocates it from the definition's own rva. That
    is the correct home for an unplaceable copy, and it costs no evidence: every
    column still comes from the candidate COFF plus the definition's proven extent.

    THE SAME APPEND HITS THE ORDINARY, NON-COMDAT `.data`/`.rdata` TOO. A unit's
    plain globals do not own a COMDAT, so they were all left in the legacy form and
    packed by the delinker in manifest (rva) order into whichever section happened to
    be the object's first of that storage - usually a COMDAT, and never at the offset
    cl gave them. ordinary_sections() reconstructs those sections the same way: cl's
    own non-COMDAT `.data`/`.rdata` becomes ONE non-affine section and each global
    sits at ITS OWN candidate offset. That is admitted only when the section is
    provably COMPLETE - every external symbol in it has an enrolled row of the right
    storage, no two definitions overlap, none overruns the section, and every byte
    left uncovered is ZERO in the candidate payload. A section with a real symbol we
    cannot enroll would otherwise be published with holes, so it is left alone.
    """
    import sys as _sys
    _sys.path.insert(0, str(REPO / "scripts/gruntz/build"))
    from coff_oracle import _Coff  # noqa: E402

    base_dir = Path(base_dir or REPO / "build/objdiff/base")
    secs, withheld = [], []
    by_obj = {}
    for r in rows:
        # Only cl.exe's string literals and vtables own a whole COMDAT. The DATA()
        # globals share one `.bss`/`.data` per object, so they keep the legacy
        # allocation form.
        if r.get("provenance") in ("candidate-COFF-string", "candidate-COFF-vtable",
                                   "candidate-COFF-rtti"):
            by_obj.setdefault(r["object"], []).append(r)

    for obj, rs in sorted(by_obj.items()):
        path = base_dir / (obj[:-2] + ".obj")       # "foo.c" -> foo.obj
        if not path.exists():
            withheld += [(r["rva"], r["name"], "no candidate obj %s" % path.name)
                         for r in rs]
            continue
        c = _Coff(path)
        # The candidate section that defines each literal, keyed by symbol name, with
        # the symbol's offset in it. A string owns its COMDAT from byte 0; a /GR
        # vtable starts at 4, behind the `??_R4` complete-object-locator pointer.
        owner = {}
        for sec in c.section_table:
            members = c.defined_symbols(sec["index"])
            if len(members) != 1:
                continue
            offset, name = members[0]
            if (name.startswith("??_C@") or name.startswith("??_R")) and offset == 0:
                owner[name] = (sec, 0)
            elif name.startswith("??_7"):
                owner[name] = (sec, offset)
        for r in rs:
            hit = owner.get(r["name"])
            if hit is None:
                withheld.append((r["rva"], r["name"],
                                 "no single-symbol COMDAT in the candidate obj"))
                continue
            sec, offset = hit
            if offset != r.get("vtable_offset", 0):
                withheld.append((r["rva"], r["name"],
                                 "candidate COMDAT offset 0x%x != enrolled 0x%x"
                                 % (offset, r.get("vtable_offset", 0))))
                continue
            if sec["size"] != offset + r["size"]:
                # The candidate's own payload disagrees with the retail extent that
                # named it - a real contradiction, so neither side is enrolled.
                withheld.append((r["rva"], r["name"],
                                 "candidate section 0x%x != 0x%x + retail extent 0x%x"
                                 % (sec["size"], offset, r["size"])))
                continue
            r["section"] = sec
            r["section_offset"] = offset
            # An unplaceable copy takes a NON-AFFINE section (`rva = -`): same
            # candidate shape, no claim on a retail range.
            placed_here = r.get("section_placed", True)
            secs.append({"object": obj, "index": sec["index"], "name": sec["name"],
                         "rva": r["rva"] - offset if placed_here else None,
                         "size": sec["size"],
                         "alignment": sec["alignment"],
                         "characteristics": sec["characteristics"],
                         "comdat": sec["comdat"], "assoc": sec["assoc"],
                         "storage": r["storage"],
                         "provenance": "candidate-COFF-section" if placed_here
                         else "candidate-COFF-section-nonaffine"})

    # Two placed sections may not claim overlapping retail bytes unless they are the
    # SAME folded COMDAT seen from two objects (identical rva/size/shape). The
    # delinker enforces this and BAILS on a violation, taking the whole delink with
    # it, so screen it here and withhold instead - a conflict is a real contradiction
    # about where a COMDAT starts, and dropping the placement leaves both definitions
    # enrolled in the legacy form rather than losing them.
    placed = sorted((s for s in secs if s["rva"] is not None),
                    key=lambda s: (s["rva"], s["size"], s["object"], s["index"]))
    conflicted = set()
    for first, second in zip(placed, placed[1:]):
        if first["rva"] + first["size"] <= second["rva"]:
            continue
        alias = (first["object"] != second["object"]
                 and (first["rva"], first["size"], first["name"], first["alignment"],
                      first["characteristics"], first["comdat"])
                 == (second["rva"], second["size"], second["name"],
                     second["alignment"], second["characteristics"], second["comdat"]))
        if not alias:
            conflicted.add((first["rva"], first["size"]))
            conflicted.add((second["rva"], second["size"]))
    if conflicted:
        secs = [s for s in secs
                if s["rva"] is None or (s["rva"], s["size"]) not in conflicted]
        for obj, rs in by_obj.items():
            for r in rs:
                if "section" not in r or not r.get("section_placed", True):
                    continue
                key = (r["rva"] - r["section_offset"], r["section"]["size"])
                if key in conflicted:
                    withheld.append((r["rva"], r["name"],
                                     "candidate section 0x%x+0x%x overlaps another "
                                     "object's" % key))
                    del r["section"], r["section_offset"]

    # cl's ordinary, non-COMDAT `.data`/`.rdata` - the globals that own no COMDAT.
    ordinary, ordinary_rows = ordinary_sections(rows, base_dir)
    secs += ordinary
    for r in ordinary_rows:
        by_obj.setdefault(r["object"], []).append(r)

    # Manifest ordinals are per-object and must be CONTIGUOUS FROM ONE. Number them
    # in the candidate COFF's own section order: objdiff stable-sorts same-named
    # sections when combining, so section order decides the combined layout and must
    # agree on both sides.
    for obj in {s["object"] for s in secs}:
        mine = sorted([s for s in secs if s["object"] == obj], key=lambda s: s["index"])
        remap = {s["index"]: i for i, s in enumerate(mine, 1)}
        for s in mine:
            s["ordinal"] = remap[s["index"]]
        for r in by_obj.get(obj, []):
            if "section" in r:
                r["section_ordinal"] = remap[r["section"]["index"]]
    secs.sort(key=lambda s: (s["object"], s["ordinal"]))
    return secs, withheld


#: `IMAGE_SCN_LNK_COMDAT` - the bit that separates a per-symbol COMDAT from the
#: unit's own ordinary data section.
LNK_COMDAT = 0x00001000
#: candidate section name -> the delinker's storage keyword. Only these two carry
#: initialized bytes an ordinary (non-COMDAT) definition can sit in.
ORDINARY_STORAGE = {".data": "data", ".rdata": "rdata"}


def ordinary_sections(rows, base_dir):
    """Non-affine candidate sections for cl's ordinary, non-COMDAT `.data`/`.rdata`.

    Returns `(section rows, definition rows annotated with section/section_offset)`.

    A unit's plain globals share one section per storage class, at the offsets cl
    chose. Publishing that section rebuilds the target in the candidate's shape - the
    same thing the COMDAT path does for a literal - and stops the delinker packing
    the definitions into another section by manifest order.

    THE SECTION IS ADMITTED ONLY WHEN IT IS PROVABLY COMPLETE. Every external symbol
    cl defined in it must have an enrolled definition of the matching storage that
    is still unplaced, no two may overlap, none may overrun the section, and every
    byte no definition covers must be ZERO in the candidate payload (cl's own
    inter-symbol padding). Fail any of those and the section is skipped entirely:
    an incomplete section would publish a hole as retail content, which is exactly
    the kind of fabrication the rest of this module refuses.
    """
    import sys as _sys
    _sys.path.insert(0, str(REPO / "scripts/gruntz/build"))
    from coff_oracle import _Coff  # noqa: E402

    # Keyed by the name cl gave the symbol IN THIS OBJECT. For everything but the FP
    # pool that IS the manifest name; a pool slot is addressed as `$T<rva>` because N
    # objects may own it, so it carries cl's per-object spelling separately.
    by_obj = defaultdict(dict)
    for r in rows:
        by_obj[r["object"]][r.get("member") or r["name"]] = r

    secs, placed = [], []
    for obj, named in sorted(by_obj.items()):
        path = Path(base_dir) / (obj[:-2] + ".obj")     # "foo.c" -> foo.obj
        if not path.exists():
            continue
        c = _Coff(path)
        for sec in c.section_table:
            storage = ORDINARY_STORAGE.get(sec["name"])
            if storage is None or sec["characteristics"] & LNK_COMDAT:
                continue
            # EVERY member, not just the externals: cl gives a function-local
            # `static` a class-STATIC `$S<id>` symbol, and those are most of the
            # bytes in a unit's ordinary `.data`. Miss them and the coverage proof
            # below passes on a section full of holes.
            members = c.section_members(sec["index"])
            if not members:
                continue
            payload = c.section_payload(sec["index"])[:sec["size"]]
            covered, mine, complete = bytearray(sec["size"]), [], True
            for offset, name, _scl in members:
                r = named.get(name)
                if r is None or r["storage"] != storage or "section" in r:
                    complete = False
                    break
                end = offset + r["size"]
                if end > sec["size"] or any(covered[offset:end]):
                    complete = False
                    break
                covered[offset:end] = b"\1" * r["size"]
                mine.append((r, offset))
            if not complete or len(payload) != sec["size"]:
                continue
            # cl's inter-symbol padding is zero; a NON-zero uncovered byte is real
            # content we have no definition for, so the section stays unpublished.
            if any(payload[i] for i in range(sec["size"]) if not covered[i]):
                continue
            for r, offset in mine:
                r["section"] = sec
                r["section_offset"] = offset
            placed += [r for r, _o in mine]
            secs.append({"object": obj, "index": sec["index"], "name": sec["name"],
                         "rva": None, "size": sec["size"],
                         "alignment": sec["alignment"],
                         "characteristics": sec["characteristics"],
                         "comdat": sec["comdat"], "assoc": sec["assoc"],
                         "storage": storage,
                         "provenance": "candidate-COFF-ordinary-nonaffine"})
    return secs, placed


def manifest_bytes(rows):
    """The --data-manifest. A row placed in a candidate section carries its
    (section_ordinal, section_offset); the rest keep the legacy `-` allocation
    form, which lets the delinker pack them itself."""
    out = ["\t".join(HEADER)]
    for r in rows:
        placed = "section_ordinal" in r
        out.append("\t".join([
            r["name"], r["object"], "0x%x" % r["rva"], "0x%x" % r["size"],
            r["storage"],
            "0x%x" % (r["section"]["alignment"] if placed else r["alignment"]),
            str(r["section_ordinal"]) if placed else "-",
            # The value cl.exe gives the symbol in the candidate obj: 0 for a literal
            # (it owns its whole COMDAT) and 4 for a /GR vtable, which sits behind the
            # `??_R4` complete-object-locator pointer.
            "0x%x" % r["section_offset"] if placed else "-",
            "external", r.get("provenance", "src-DATA-sizeof")]))
    return ("\n".join(out) + "\n").encode("utf-8")


def section_manifest_bytes(secs):
    """The --data-section-manifest. `rva = -` is a NON-AFFINE section: the candidate
    COFF shape without a claim on a retail range (see section_rows)."""
    out = ["\t".join(SECTION_HEADER)]
    for s in secs:
        out.append("\t".join([
            s["object"], str(s["ordinal"]), s["name"],
            "0x%x" % s["rva"] if s["rva"] is not None else "-",
            "0x%x" % s["size"], "0x%x" % s["alignment"],
            "0x%x" % s["characteristics"], str(s["comdat"]),
            str(s["assoc"]) if s["assoc"] else "-", s["storage"], s["provenance"]]))
    return ("\n".join(out) + "\n").encode("utf-8")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-o", "--output", type=Path, default=OUTPUT)
    ap.add_argument("--section-output", type=Path, default=SECTION_OUTPUT)
    ap.add_argument("--report", action="store_true", help="print withheld + overlaps")
    args = ap.parse_args(argv)

    enrolled, withheld, overlaps = candidates()
    secs, sec_withheld = section_rows(enrolled)
    withheld += sec_withheld
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(manifest_bytes(enrolled))
    args.section_output.parent.mkdir(parents=True, exist_ok=True)
    args.section_output.write_bytes(section_manifest_bytes(secs))
    folds = len(enrolled) - len({(r["name"], r["rva"]) for r in enrolled})
    print("[data-manifest] enrolled %d row(s) (%d folded-COMDAT copies) -> %s"
          % (len(enrolled), folds, args.output))
    print("[data-manifest] storage: " + ", ".join(
        "%s=%d" % kv for kv in sorted(Counter(r["storage"] for r in enrolled).items())))
    print("[data-manifest] %d row(s) placed in %d candidate section(s) -> %s"
          % (sum(1 for r in enrolled if "section_ordinal" in r), len(secs),
             args.section_output))
    print("[data-manifest] withheld %d (never guessed): %s" % (
        len(withheld), ", ".join("%s=%d" % kv for kv in sorted(
            Counter(w[2].split("(")[0].strip() for w in withheld).items()))))
    if args.report:
        print("\n--- overlap contradictions (a real mis-modelling worklist) ---")
        for a, b, by in overlaps:
            print("  0x%06x %-42s +0x%-4x overlaps 0x%06x %s by 0x%x"
                  % (a["rva"], a["name"][:42], a["size"], b["rva"], b["name"][:38], by))
        print("\n--- withheld (no proven extent) ---")
        for rva, name, why in withheld[:20]:
            print("  0x%06x %-46s %s" % (rva, name[:46], why))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
