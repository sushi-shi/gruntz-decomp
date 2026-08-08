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

    Only PRIMARY vtables (base_off 0, spelled `??_7<class>@@6B@`) are enrolled; a
    secondary/MI vtable (`??_7<class>@@6B<base>@@@`) is left to the next pass.
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
    primary = {}
    for name, ci in reg.items():
        p = ci.vtables.get(0)
        if p is not None:
            primary["??_7%s@@6B@" % name] = (p[0], p[1])

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

    # The reviewed DATA_COMPGEN claims (one per owning unit, labels.py-gated).
    compgen, compgen_withheld = compgen_rows(exe=exe)
    rows += compgen
    withheld += compgen_withheld

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
    rows = deduped

    # The N owners of a folded COMDAT all claim ONE rva, so collapse each fold to a
    # single extent before the neighbour check - otherwise the copies read as mutual
    # overlaps. Every real extent still has to fit the span to its neighbour; an
    # overlap proves one of the pair is mis-modelled but not which, so neither is
    # enrolled. This scans DEFINITION extents (the delinker's own data-manifest
    # check); the placed-SECTION extents, which for a /GR vtable start four bytes
    # earlier at its `??_R4` COL word, are screened in section_rows().
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
    is part of the COMDAT, so the delinker copies and relocates it too. A row that
    vtable_rows() marked unplaced (its TU emits the class without the COL word that
    retail's copy has) keeps the legacy allocation form: its payload is still the
    byte-exact retail slot array, it just does not claim a retail RANGE that another
    object's section already claims.
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
        if r.get("provenance") in ("candidate-COFF-string", "candidate-COFF-vtable") \
                and r.get("section_placed", True):
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
            if name.startswith("??_C@") and offset == 0:
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
            secs.append({"object": obj, "index": sec["index"], "name": sec["name"],
                         "rva": r["rva"] - offset, "size": sec["size"],
                         "alignment": sec["alignment"],
                         "characteristics": sec["characteristics"],
                         "comdat": sec["comdat"], "assoc": sec["assoc"],
                         "storage": r["storage"],
                         "provenance": "candidate-COFF-section"})

    # Two placed sections may not claim overlapping retail bytes unless they are the
    # SAME folded COMDAT seen from two objects (identical rva/size/shape). The
    # delinker enforces this and BAILS on a violation, taking the whole delink with
    # it, so screen it here and withhold instead - a conflict is a real contradiction
    # about where a COMDAT starts, and dropping the placement leaves both definitions
    # enrolled in the legacy form rather than losing them.
    placed = sorted(secs, key=lambda s: (s["rva"], s["size"], s["object"], s["index"]))
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
        secs = [s for s in secs if (s["rva"], s["size"]) not in conflicted]
        for obj, rs in by_obj.items():
            for r in rs:
                if "section" not in r:
                    continue
                key = (r["rva"] - r["section_offset"], r["section"]["size"])
                if key in conflicted:
                    withheld.append((r["rva"], r["name"],
                                     "candidate section 0x%x+0x%x overlaps another "
                                     "object's" % key))
                    del r["section"], r["section_offset"]

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
    out = ["\t".join(SECTION_HEADER)]
    for s in secs:
        out.append("\t".join([
            s["object"], str(s["ordinal"]), s["name"], "0x%x" % s["rva"],
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
