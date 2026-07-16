#!/usr/bin/env python3
"""Synthesize a fabricated PDB for vostok-delinker from the Ghidra inventory.

No real PDB exists for Gruntz (1999); this builds one good enough for the
delinker (srp-survarium/vostok-delinker) to slice GRUNTZ.EXE into per-symbol
COFF .obj files. It is a preservation / matching-decompilation tool for a
binary the project owns.

Pipeline:
  1. Read build/ghidra/exports/functions.csv (.text functions w/ sizes) and
     symbols.csv (.rdata constants, .data statics).
  2. Emit an llvm-pdbutil yaml2pdb YAML description:
       - one DBI module whose symbol stream holds S_GPROC32 (function,
         section:offset, code length) + S_LDATA32 (rdata/data) records.
       - PdbStream / TpiStream / IpiStream / DbiStream scaffolding.
  3. Run `llvm-pdbutil yaml2pdb` to produce the PDB.
  4. yaml2pdb cannot emit a GSI/publics symbol-records stream, and the
     delinker's `pdb.global_symbols()` (pdb2 crate) errors with
     GlobalSymbolsNotFound if the DBI header's symbol_records_stream index is
     nil (0xFFFF).  We don't actually need any global symbols (everything the
     delinker reads is available from the module S_GPROC32 / S_LDATA32
     records), so we byte-patch the DBI header's symbol_records_stream field to
     point at an existing empty stream.  pdb2 then returns an empty global
     symbol table and the module iteration supplies all the real data.

Section -> (1-based PE/object-crate section index, section base RVA):
    .text  -> segment 1, base 0x1000
    .rdata -> segment 2, base 0x1e7000
    .data  -> segment 3, base 0x208000
(The delinker compares offset.section against object-crate section indices,
 which are 1-based in PE section order: .text=1, .rdata=2, .data=3.)
"""

import argparse
import csv
import os
import re
import struct
import subprocess
import sys

# PE section layout (RVAs) is read from the EXE at runtime via read_sections();
# these are filled in for retail_en/GRUNTZ.EXE (ImageBase 0x00400000):
#   .text  RVA 0x1000   vsize 0x1e526b   -> segment 1
#   .rdata RVA 0x1e7000 vsize 0x20fa8    -> segment 2
#   .data  RVA 0x208000 vsize 0xba7ac    -> segment 3
# NOTE: section bounds use the *virtual* size (what the object/pdb2 crates use),
# not the smaller raw/file size that `objdump -h` prints under "Size".
TEXT_BASE = TEXT_END = 0
RDATA_BASE = RDATA_END = 0
DATA_BASE = DATA_END = 0
IDATA_BASE = IDATA_END = 0

SEG_TEXT = 1
SEG_RDATA = 2
SEG_DATA = 3
SEG_IDATA = 4


def read_sections(exe_path):
    """Read .text/.rdata/.data/.idata RVA bounds (virtual size) from the PE.

    Sets the module-level *_BASE / *_END globals so the rest of the generator
    classifies symbols against the real section layout.
    """
    global TEXT_BASE, TEXT_END, RDATA_BASE, RDATA_END, DATA_BASE, DATA_END
    global IDATA_BASE, IDATA_END
    with open(exe_path, "rb") as f:
        data = f.read()
    pe_off = struct.unpack_from("<I", data, 0x3C)[0]
    num_sec = struct.unpack_from("<H", data, pe_off + 6)[0]
    opt_size = struct.unpack_from("<H", data, pe_off + 20)[0]
    sec_off = pe_off + 24 + opt_size
    bounds = {}
    for i in range(num_sec):
        b = sec_off + i * 40
        name = data[b:b + 8].rstrip(b"\0").decode()
        vsize, vaddr = struct.unpack_from("<II", data, b + 8)
        bounds[name] = (vaddr, vaddr + vsize)
    TEXT_BASE, TEXT_END = bounds[".text"]
    RDATA_BASE, RDATA_END = bounds[".rdata"]
    DATA_BASE, DATA_END = bounds[".data"]
    # .idata is retail PE section 4 -> segment 4 (1-based PE order, like the rest).
    IDATA_BASE, IDATA_END = bounds.get(".idata", (0, 0))


# --- IAT (.idata) import symbols -------------------------------------------
# The data-topology delinker reconstructs `__imp__...` COFF relocations from the
# PDB's .idata symbols and hard-errors ("IAT relocation target 0x... has no exact
# PDB symbol") when a relocation lands on an IAT slot we did not name. Retail has
# 456 slots (449 by name, 7 by ordinal) in .idata @0x2c3000.
#
# The decorated spelling is NEVER invented: a wrong stdcall @N is silent
# corruption. It is sourced only from real artifacts, via the two REAL conventions:
#   * vendor DLLs (mss32/smackw32) export an ALREADY-decorated name
#     ("_AIL_init_sequence@12") -> the COFF symbol is "__imp_" + that export name;
#   * Win32 DLLs export undecorated ("CreateFileA") -> the COFF symbol carries the
#     arg size ("__imp__CreateFileA@28"), recovered by normalizing the candidates.
# Evidence: our own base objs (the compiler's real spellings) + the MSVC 5.0 / DX
# import libraries. An ambiguous or unfound name is SKIPPED and reported.
_IMP_RE = re.compile(r"\b(__imp_[@_]?[^\s]+)")


def _imp_normalize(dec):
    """`__imp__CreateFileA@28` -> `CreateFileA` (the undecorated export name)."""
    b = dec[len("__imp_"):]
    if b[:1] in ("_", "@"):
        b = b[1:]
    return re.sub(r"@\d+$", "", b)


def collect_imp_decorations(paths, nm="llvm-nm"):
    """(exact-set, {undecorated: decorated}) of every `__imp_` symbol in `paths`.

    A name that maps to two different decorations is dropped from the normalized
    index (ambiguous => we refuse to choose).
    """
    exact = set()
    for p in paths:
        try:
            # errors="replace": an import .LIB carries raw non-UTF8 bytes in its
            # archive members; only the ASCII symbol names matter here.
            r = subprocess.run([nm, p], capture_output=True, text=True,
                               errors="replace", timeout=120)
        except (OSError, subprocess.SubprocessError):
            continue
        exact.update(_IMP_RE.findall(r.stdout))
    by_norm = {}
    for dec in exact:
        n = _imp_normalize(dec)
        if n in by_norm and by_norm[n] != dec:
            by_norm[n] = None
        else:
            by_norm.setdefault(n, dec)
    return exact, {k: v for k, v in by_norm.items() if v}


def _ar_members(path):
    """Yield (member-name, body) for each member of a `!<arch>` library."""
    try:
        d = open(path, "rb").read()
    except OSError:
        return
    if d[:8] != b"!<arch>\n":
        return
    i = 8
    while i + 60 <= len(d):
        name = d[i:i + 16].decode("latin1").strip()
        try:
            size = int(d[i + 48:i + 58].decode("latin1").strip() or 0)
        except ValueError:
            return
        yield name, d[i + 60:i + 60 + size]
        i += 60 + size + (size & 1)


def _coff_import_ordinal_and_imp(body):
    """(ordinal, `__imp_` symbol) for one long-format import-library member.

    An MSVC 5.0 import lib member is a real i386 COFF. For an ORDINAL import its
    `.idata$4` ILT entry holds the ordinal with the high bit set (a by-name import
    instead carries a relocation to `.idata$6`), and the member defines the
    `__imp__...` symbol. That pairing is the library's OWN ordinal->decoration
    binding: authoritative, so no `@N` is ever guessed.
    """
    if len(body) < 20 or struct.unpack_from("<H", body, 0)[0] != 0x014C:
        return None, None
    nsec = struct.unpack_from("<H", body, 2)[0]
    symoff = struct.unpack_from("<I", body, 8)[0]
    nsym = struct.unpack_from("<I", body, 12)[0]
    optsz = struct.unpack_from("<H", body, 16)[0]
    stroff = symoff + nsym * 18
    ordinal = None
    for s in range(nsec):
        o = 20 + optsz + s * 40
        nm = body[o:o + 8].rstrip(b"\0").decode("latin1", "replace")
        rawsz, rawptr = struct.unpack_from("<II", body, o + 16)
        if nm == ".idata$4" and rawsz >= 4 and rawptr:
            v = struct.unpack_from("<I", body, rawptr)[0]
            if v & 0x80000000:
                ordinal = v & 0xFFFF
    imp = None
    k = 0
    while k < nsym:
        o = symoff + k * 18
        raw = body[o:o + 8]
        zero, stro = struct.unpack("<II", raw)
        if zero == 0:
            try:
                nm = body[stroff + stro:body.index(b"\0", stroff + stro)].decode("latin1")
            except ValueError:
                nm = ""
        else:
            nm = raw.rstrip(b"\0").decode("latin1", "replace")
        if nm.startswith("__imp_"):
            imp = nm
        k += 1 + body[o + 17]
    return ordinal, imp


def collect_ordinal_decorations(lib_paths):
    """{(dll-lowercase, ordinal): `__imp__...`} from the import libraries.

    Retail imports 7 slots by ORDINAL (COMCTL32 #13/#14/#17, DPLAYX #1/#2/#4,
    DSOUND #1), which carry no name in the PE. The import lib binds each ordinal to
    its decorated symbol, which is the only exact source. Conflicting bindings are
    dropped rather than chosen between.
    """
    table = {}
    for p in lib_paths:
        for name, body in _ar_members(p):
            dll = name.rstrip("/").lower()
            if not dll.endswith(".dll"):
                continue
            ordinal, imp = _coff_import_ordinal_and_imp(body)
            if ordinal is None or not imp:
                continue
            key = (dll, ordinal)
            if key in table and table[key] != imp:
                table[key] = None
            else:
                table.setdefault(key, imp)
    return {k: v for k, v in table.items() if v}


def read_imports(exe_path):
    """[(iat_slot_rva, import_name_or_None, dll, ordinal_or_None)] from the PE."""
    with open(exe_path, "rb") as f:
        data = f.read()
    pe_off = struct.unpack_from("<I", data, 0x3C)[0]
    coff = pe_off + 4
    num_sec = struct.unpack_from("<H", data, coff + 2)[0]
    opt_size = struct.unpack_from("<H", data, coff + 16)[0]
    opt = coff + 20
    sec_off = opt + opt_size
    secs = []
    for i in range(num_sec):
        b = sec_off + i * 40
        vsize, vaddr, rawsz, rawptr = struct.unpack_from("<IIII", data, b + 8)
        secs.append((vaddr, max(vsize, rawsz), rawptr))

    def raw(rva):
        for va, sz, ptr in secs:
            if va <= rva < va + sz:
                return ptr + (rva - va)
        raise ValueError("RVA 0x%x outside PE sections" % rva)

    def cstr(off):
        return data[off:data.index(0, off)].decode("ascii", "replace")

    imp_rva = struct.unpack_from("<II", data, opt + 96 + 1 * 8)[0]
    if not imp_rva:
        return []
    out = []
    d = raw(imp_rva)
    while True:
        lookup, _ts, _fw, name_rva, addr_rva = struct.unpack_from("<IIIII", data, d)
        if not any((lookup, _ts, _fw, name_rva, addr_rva)):
            break
        dll = cstr(raw(name_rva))
        thunk = raw(lookup or addr_rva)
        i = 0
        while True:
            v = struct.unpack_from("<I", data, thunk + i * 4)[0]
            if not v:
                break
            slot = addr_rva + i * 4
            if v & 0x80000000:
                out.append((slot, None, dll, v & 0xFFFF))
            else:
                out.append((slot, cstr(raw(v) + 2), dll, None))
            i += 1
        d += 20
    return out


def read_iat_symbols(exe_path, base_dir=None, lib_dirs=(), nm="llvm-nm"):
    """[(rva, `__imp__...`)] for every IAT slot whose decoration is PROVEN.

    Unresolvable slots (ordinal-only imports, or a name absent/ambiguous in every
    evidence source) are skipped and reported - never guessed.
    """
    import glob as _glob
    libs = []
    for d in lib_dirs:
        if d and os.path.isdir(d):
            for pat in ("*.LIB", "*.lib"):
                # Non-recursive on purpose: the DX SDK ships a Borland/ subdir whose
                # libraries use a different toolchain's conventions.
                libs += sorted(_glob.glob(os.path.join(d, pat)))
    sources = []
    if base_dir and os.path.isdir(base_dir):
        sources += sorted(_glob.glob(os.path.join(base_dir, "*.obj")))
    sources += libs
    exact, by_norm = collect_imp_decorations(sources, nm=nm)
    by_ordinal = collect_ordinal_decorations(libs)

    syms, unresolved = [], []
    for slot, name, dll, ordinal in read_imports(exe_path):
        dec = None
        if name is None:
            # Imported by ordinal: the PE carries no name; the import lib's own
            # ordinal->symbol binding is the exact source.
            dec = by_ordinal.get((dll.lower(), ordinal))
            label = "%s ordinal #%s" % (dll, ordinal)
        else:
            if "__imp_" + name in exact:   # vendor: the export IS the decoration
                dec = "__imp_" + name
            elif name in by_norm:          # win32: undecorated export -> @N spelling
                dec = by_norm[name]
            label = "%s!%s" % (dll, name)
        if dec:
            syms.append((slot, dec))
        else:
            unresolved.append((slot, label))
    syms.sort()
    print("[synth_pdb] .idata: named %d/%d IAT slot(s) from %d evidence file(s)"
          % (len(syms), len(syms) + len(unresolved), len(sources)), file=sys.stderr)
    for slot, label in unresolved:
        print("[synth_pdb] .idata SKIP 0x%06x %s: no exact __imp_ decoration in the base "
              "objs or import libs (never guessing @N)" % (slot, label), file=sys.stderr)
    return syms


def sanitize_name(name: str) -> str:
    """Make a symbol name safe to embed in single-quoted YAML.

    yaml2pdb takes the DisplayName verbatim into the CodeView record, so the
    only escaping needed is for the YAML layer. We single-quote and double any
    embedded single quotes. We also strip control characters that would break
    the record.
    """
    cleaned = "".join(c if (32 <= ord(c) < 127) else "_" for c in name)
    return cleaned.replace("'", "''")


def read_names_map(path):
    """Read the curated RVA -> (symbol name, source unit, size) overlay.

    The CSV has columns ``rva,name,unit[,size]`` (rva hex, e.g. ``0x1882d0``;
    size hex from the src ``RVA(addr, size)`` annotation, possibly empty). This is the
    human-grown mapping at the heart of the matching loop: it renames the
    delinker's address-named ``FUN_<rva>`` placeholders to the real source
    symbols (``_adler32``) and records WHICH translation unit each belongs to,
    so the delinker can emit one ``<unit>.c.obj`` per source TU instead of
    address-bucketed segs. The optional ``size`` is the authoritative byte extent
    used to SYNTHESIZE a function record for a matched RVA that Ghidra's
    auto-analysis never recovered as an object (so it is absent from
    functions.csv) - this keeps a single build self-contained.

    Returns ``{rva: (name, unit, size, kind)}`` (size is an int, 0 when
    absent/empty; kind is ``"func"`` or ``"data"``, defaulting to ``"func"`` for
    older rows). Backward-tolerant of a 3/4-column CSV. Empty if no path.
    """
    overlay = {}
    if not path:
        return overlay
    with open(path, newline="") as f:
        # Drop full-line '#' comments before the header so DictReader binds the
        # real 'rva,name,unit[,size[,kind]]' columns (the CSV is documented with
        # a comment preamble).
        lines = [ln for ln in f if not ln.lstrip().startswith("#")]
    for row in csv.DictReader(lines):
        rva_s = (row.get("rva") or "").strip()
        if not rva_s:
            continue
        size_s = (row.get("size") or "").strip()       # absent in 3-col CSVs
        size = int(size_s, 0) if size_s else 0
        kind = (row.get("kind") or "func").strip() or "func"
        overlay[int(rva_s, 16)] = (row["name"].strip(), row["unit"].strip(), size, kind)
    return overlay


def read_functions(path, names_map=None):
    """Yield (rva, size, name) for .text functions with size > 0.

    If ``names_map`` (``{rva: (name, unit, size)}``) supplies an entry for an
    RVA, its curated name overrides the Ghidra ``FUN_<rva>`` placeholder.

    A matched function that Ghidra's auto-analysis never recovered as an object
    (e.g. a vtable-slot-only target with no direct ``call``) is ABSENT from
    functions.csv, so the delinker would never emit it. For any names-map RVA
    not present here we SYNTHESIZE a record from the curated ``@size`` boundary,
    making a single build self-contained (no ghidra-refresh round-trip). The
    Ghidra boundary always wins for RVAs already in functions.csv - ``@size`` is
    purely the fallback for the missing ones, so the functions that already work
    stay byte-identical.
    """
    names_map = names_map or {}
    out = []
    seen = set()
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            rva = int(row["entry_rva"], 16)
            size = int(row["byte_size"])
            name = row["name"]
            if size <= 0:
                continue
            if not (TEXT_BASE <= rva < TEXT_END):
                continue
            if rva in names_map:
                name = names_map[rva][0]
                at_size = names_map[rva][2]
                # Ghidra's recovered boundary wins for RVAs already in
                # functions.csv (keeps the working set byte-identical), but if the
                # curated @size disagrees, one of the two is wrong - surface it so
                # a bad boundary isn't silently delinked.
                if at_size and at_size != size:
                    # Ghidra usually wins (keeps the working set byte-identical). BUT a
                    # tiny Ghidra boundary (<= 8 B) where the src annotates a much larger
                    # @size is a MIS-CARVE - Ghidra failed to recover the function (e.g. a
                    # spurious 1-byte carve blocks the delinker from extracting the target
                    # bytes, so the fn scores 0%). The curated @size (matcher-verified) is
                    # then authoritative: use it and delink the real extent. Durable across
                    # ghidra reimports (functions.csv is regenerated, this logic is not).
                    if size <= 8 and at_size > size:
                        print("[synth_pdb] 0x%x %r: Ghidra mis-carve %d B -> @size %d B"
                              % (rva, name, size, at_size), file=sys.stderr)
                        size = at_size
                    else:
                        print("[synth_pdb] WARN: 0x%x %r boundary mismatch - "
                              "functions.csv=%d B, @size=%d B; using functions.csv "
                              "(Ghidra wins). Check the @size or the recovered boundary."
                              % (rva, name, size, at_size), file=sys.stderr)
            out.append((rva, size, name))
            seen.add(rva)

    synth = 0
    for rva, (name, _unit, size) in sorted(names_map.items()):
        if rva in seen:
            continue
        if not (TEXT_BASE <= rva < TEXT_END):
            continue
        if size <= 0:
            print("[synth_pdb] WARN: 0x%x %r absent from functions.csv and has "
                  "no @size - cannot synthesize a boundary; skipping." % (rva, name),
                  file=sys.stderr)
            continue
        out.append((rva, size, name))
        seen.add(rva)
        synth += 1
    if synth:
        print("[synth_pdb] synthesized %d function record(s) from @size for "
              "matched RVAs absent from functions.csv" % synth, file=sys.stderr)
    out.sort()
    return out


def read_data_symbols(path):
    """Return (rdata_syms, data_syms), each a list of (rva, name).

    Deduplicates by RVA (keeps the first / primary symbol seen) so we emit one
    record per address; the delinker keys its constant/static maps by RVA.
    """
    rdata = {}
    data = {}
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            addr = row["address_rva"]
            if addr.startswith("0x-"):
                continue  # imported / negative pseudo-RVA
            rva = int(addr, 16)
            name = row["name"]
            if RDATA_BASE <= rva < RDATA_END:
                rdata.setdefault(rva, name)
            elif DATA_BASE <= rva < DATA_END:
                data.setdefault(rva, name)
    rdata_list = sorted((rva, nm) for rva, nm in rdata.items())
    data_list = sorted((rva, nm) for rva, nm in data.items())
    return rdata_list, data_list


def func_source_file(rva, bucket_shift, names_map=None):
    """Synthetic source path under the engine root for a function at `rva`.

    A function named in ``names_map`` is attributed to its curated source unit,
    ``c:\\proj\\<unit>.c``, so the delinker emits a clean per-TU ``<unit>.c.obj``
    that objdiff pairs directly against the recompiled ``<unit>.obj``. Otherwise
    functions are grouped by the high bits of their .text offset into
    address-bucketed ``seg_NNNN.cpp`` (per-TU granularity for the un-named
    remainder). Either way the path lives under `c:\\proj\\` so the delinker's
    get_function_location strips the engine prefix and writes ``<stem>.obj``.
    """
    names_map = names_map or {}
    if rva in names_map:
        return r"c:\proj\%s.c" % names_map[rva][1]
    bucket = (rva - TEXT_BASE) >> bucket_shift
    return r"c:\proj\seg_%04x.cpp" % bucket


def emit_yaml(funcs, rdata_syms, data_syms, module_path, out, bucket_shift=0,
              names_map=None, iat_syms=()):
    """Write the yaml2pdb description to file object `out`.

    If bucket_shift > 0, emit C13 line info (FileChecksums + Lines subsections
    + a top-level StringTable) so each function is attributed to a synthetic
    source file `seg_NNNN.cpp`, which the delinker turns into a separate .obj.
    If bucket_shift == 0, no line info is emitted and every function falls into
    the delinker's name-based bucket (a single `_msvc_internal/c_lang.obj`).
    """
    w = out.write
    split = bucket_shift > 0

    # Pre-compute the per-function source file and the ordered set of files.
    func_files = []
    files_seen = []
    files_set = set()
    if split:
        for rva, size, name in funcs:
            sf = func_source_file(rva, bucket_shift, names_map)
            func_files.append(sf)
            if sf not in files_set:
                files_set.add(sf)
                files_seen.append(sf)

    w("MSF:\n")
    w("  SuperBlock:\n")
    w("    BlockSize:       4096\n")
    w("    FreeBlockMap:    2\n")
    w("    NumBlocks:       0\n")
    w("    NumDirectoryBytes: 0\n")
    w("    Unknown1:        0\n")
    w("    BlockMapAddr:    0\n")
    w("PdbStream:\n")
    w("  Age:             1\n")
    w("  Guid:            '{00000000-0000-0000-0000-000000000000}'\n")
    w("  Signature:       0\n")
    w("  Features:        [ VC140 ]\n")
    w("  Version:         VC70\n")
    w("DbiStream:\n")
    w("  VerHeader:       V70\n")
    w("  Age:             1\n")
    w("  BuildNumber:     0\n")
    w("  PdbDllVersion:   0\n")
    w("  PdbDllRbld:      0\n")
    w("  Flags:           0\n")
    w("  MachineType:     x86\n")
    w("  Modules:\n")
    w("    - Module:          '%s'\n" % module_path)
    w("      ObjFile:         '%s'\n" % module_path)

    if split:
        # The module's source-file list and the C13 debug subsections.
        w("      SourceFiles:\n")
        for sf in files_seen:
            w("        - '%s'\n" % sf)
        w("      Subsections:\n")
        w("        - !FileChecksums\n")
        w("          Checksums:\n")
        import hashlib
        for sf in files_seen:
            # Deterministic 16-byte checksum from the path (content is fake).
            cks = hashlib.md5(sf.encode()).hexdigest()
            w("            - FileName:        '%s'\n" % sf)
            w("              Kind:            MD5\n")
            w("              Checksum:        %s\n" % cks.upper())
        # One !Lines block per function (one line entry at offset 0).
        for (rva, size, name), sf in zip(funcs, func_files):
            off = rva - TEXT_BASE
            w("        - !Lines\n")
            w("          CodeSize:        %d\n" % size)
            w("          Flags:           [  ]\n")
            w("          RelocOffset:     %d\n" % off)
            w("          RelocSegment:    %d\n" % SEG_TEXT)
            w("          Blocks:\n")
            w("            - FileName:        '%s'\n" % sf)
            w("              Lines:\n")
            w("                - Offset:          0\n")
            w("                  LineStart:       1\n")
            w("                  EndDelta:        0\n")
            w("                  IsStatement:     true\n")
            w("              Columns:         []\n")

    w("      Modi:\n")
    w("        Records:\n")

    # .text functions -> S_GPROC32 (+ S_END to close the scope).
    for rva, size, name in funcs:
        off = rva - TEXT_BASE
        w("          - Kind:            S_GPROC32\n")
        w("            ProcSym:\n")
        w("              CodeSize:        %d\n" % size)
        w("              DbgStart:        0\n")
        w("              DbgEnd:          0\n")
        w("              FunctionType:    0\n")
        w("              Offset:          %d\n" % off)
        w("              Segment:         %d\n" % SEG_TEXT)
        w("              Flags:           [  ]\n")
        w("              DisplayName:     '%s'\n" % sanitize_name(name))
        w("          - Kind:            S_END\n")
        w("            ScopeEndSym:     {}\n")

    # .rdata constants -> S_LDATA32 (segment 2).
    for rva, name in rdata_syms:
        off = rva - RDATA_BASE
        w("          - Kind:            S_LDATA32\n")
        w("            DataSym:\n")
        w("              Type:            0\n")
        w("              Offset:          %d\n" % off)
        w("              Segment:         %d\n" % SEG_RDATA)
        w("              DisplayName:     '%s'\n" % sanitize_name(name))

    # .data statics -> S_LDATA32 (segment 3).
    for rva, name in data_syms:
        off = rva - DATA_BASE
        w("          - Kind:            S_LDATA32\n")
        w("            DataSym:\n")
        w("              Type:            0\n")
        w("              Offset:          %d\n" % off)
        w("              Segment:         %d\n" % SEG_DATA)
        w("              DisplayName:     '%s'\n" % sanitize_name(name))

    # .idata IAT slots -> S_LDATA32 (segment 4). Each is the `__imp__...` symbol a
    # call through the import table resolves to; the delinker rebuilds the COFF
    # relocation from these and errors on any IAT target it cannot name.
    for rva, name in iat_syms:
        off = rva - IDATA_BASE
        w("          - Kind:            S_LDATA32\n")
        w("            DataSym:\n")
        w("              Type:            0\n")
        w("              Offset:          %d\n" % off)
        w("              Segment:         %d\n" % SEG_IDATA)
        w("              DisplayName:     '%s'\n" % sanitize_name(name))

    # Top-level PDB string table: the source paths referenced by line info.
    # The delinker reads file names through this table via get_file_info ->
    # string_table.get(). Without split mode it can be empty.
    w("StringTable:\n")
    if split:
        for sf in files_seen:
            w("  - '%s'\n" % sf)
    else:
        w("  []\n")


# --- DBI header patch -------------------------------------------------------
#
# DBIHeader layout (matches pdb2 crate / microsoft-pdb):
#   u32 signature            @ 0x00
#   u32 version              @ 0x04
#   u32 age                  @ 0x08
#   u16 gs_symbols_stream    @ 0x0c
#   u16 internal_version     @ 0x0e
#   u16 ps_symbols_stream    @ 0x10
#   u16 pdb_dll_build_version@ 0x12
#   u16 symbol_records_stream@ 0x14   <-- patch target
# yaml2pdb writes 0xFFFF (nil) here when no GSI is present. We repoint it at an
# existing empty stream so pdb2's global_symbols() returns an empty table.

SYMREC_OFFSET_IN_DBI = 0x14


def find_empty_stream_index(pdb_path):
    """Return the index of an existing 0-byte stream (e.g. /LinkInfo)."""
    res = subprocess.run(
        ["llvm-pdbutil", "dump", "--streams", pdb_path],
        capture_output=True, text=True, check=True,
    )
    # Lines look like: "  Stream 5 (  0 bytes): [Named Stream \"/LinkInfo\"]"
    import re
    for line in res.stdout.splitlines():
        m = re.search(r"Stream\s+(\d+)\s+\(\s*0 bytes\)", line)
        if m:
            return int(m.group(1))
    return None


def dbi_stream_file_offset(pdb_path):
    """Return the file offset of the DBI stream (stream 3) first byte.

    Parses the MSF superblock + stream directory to find stream 3's first
    block. Robust against block fragmentation as long as the DBI header
    (first 0x16 bytes) lives in the first block (always true: block size 4096).
    """
    with open(pdb_path, "rb") as f:
        data = f.read()
    # MSF magic is 32 bytes.
    magic = b"Microsoft C/C++ MSF 7.00\r\n\x1aDS\x00\x00\x00"
    assert data.startswith(magic), "unexpected MSF magic"
    (block_size, free_map, num_blocks, num_dir_bytes,
     _unknown, block_map_addr) = struct.unpack_from("<IIIIII", data, 32)
    # Directory block list lives at block_map_addr; it lists the blocks that
    # hold the stream directory itself.
    num_dir_blocks = (num_dir_bytes + block_size - 1) // block_size
    dir_block_ptrs = struct.unpack_from(
        "<%dI" % num_dir_blocks, data, block_map_addr * block_size)
    directory = b"".join(
        data[b * block_size: b * block_size + block_size] for b in dir_block_ptrs
    )[:num_dir_bytes]
    pos = 0
    (num_streams,) = struct.unpack_from("<I", directory, pos)
    pos += 4
    sizes = list(struct.unpack_from("<%dI" % num_streams, directory, pos))
    pos += 4 * num_streams
    # Per-stream block lists follow, in order.
    stream_blocks = []
    for sz in sizes:
        if sz == 0xFFFFFFFF:
            sz = 0
        nblk = (sz + block_size - 1) // block_size
        blks = list(struct.unpack_from("<%dI" % nblk, directory, pos))
        pos += 4 * nblk
        stream_blocks.append(blks)
    DBI_STREAM = 3
    if DBI_STREAM >= num_streams or not stream_blocks[DBI_STREAM]:
        raise RuntimeError("DBI stream missing or empty")
    first_block = stream_blocks[DBI_STREAM][0]
    return first_block * block_size


def patch_symbol_records_stream(pdb_path, target_stream_index):
    off = dbi_stream_file_offset(pdb_path) + SYMREC_OFFSET_IN_DBI
    with open(pdb_path, "r+b") as f:
        f.seek(off)
        cur = struct.unpack("<H", f.read(2))[0]
        f.seek(off)
        f.write(struct.pack("<H", target_stream_index))
    return off, cur


def apply_named_data(rdata_syms, data_syms, data_names):
    """Name the DATA symbols of matched globals (labels.py `data` rows).

    ``data_names`` is ``{rva: mangled-name}`` - clang's MS-ABI name for a global
    a source `extern` references (`?g_foo@@3...` / `_g_foo`), already authority-
    checked against the base object. Overrides the Ghidra `DAT_`/`s_` placeholder
    at that RVA (adding the symbol if Ghidra never recorded one) so the delinked
    reference matches the recompiled one by name. Mutates the lists; returns the
    count applied.
    """
    n = 0
    for syms, lo, hi in ((rdata_syms, RDATA_BASE, RDATA_END),
                         (data_syms, DATA_BASE, DATA_END)):
        seen = {rva for rva, _ in syms}
        for i, (rva, _name) in enumerate(syms):
            if rva in data_names:
                syms[i] = (rva, data_names[rva])
                n += 1
        for rva, name in data_names.items():
            if rva not in seen and lo <= rva < hi:
                syms.append((rva, name))
                n += 1
    rdata_syms.sort()
    data_syms.sort()
    return n


def apply_string_names(rdata_syms, data_syms, exe_path, base_dir):
    """Rename string-constant data symbols to their MSVC ??_C@... pool names.

    Ghidra labels string literals `DAT_<addr>` / `s_<text>_<addr>`; cl.exe names
    them `??_C@_0<len>@<crc>@<text>@` (a 16-bit checksum on VC5 we do not try to
    recompute). The base objects ARE cl.exe output, so their ??_C@ symbols give
    the exact name for each literal's bytes - we read each candidate symbol's
    bytes from the EXE and look the name up there. Mutates the lists in place;
    returns the count renamed.
    """
    from coff_oracle import Exe, build_string_map
    from pathlib import Path

    str_map = build_string_map(Path(base_dir))   # {string bytes -> ??_C@ name}
    if not str_map:
        return 0
    exe = Exe(Path(exe_path))
    n = 0
    for syms in (rdata_syms, data_syms):
        for i, (rva, name) in enumerate(syms):
            if name.startswith("??_C@"):
                continue
            cs = exe.cstring(rva + exe.base)
            if cs and cs in str_map:
                syms[i] = (rva, str_map[cs])
                n += 1
    return n


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--exe", required=True,
                    help="PE EXE to read .text/.rdata/.data section bounds from.")
    ap.add_argument("--functions", required=True)
    ap.add_argument("--symbols", required=True)
    ap.add_argument("--out-yaml", required=True)
    ap.add_argument("--out-pdb", required=True)
    ap.add_argument("--module-path", default=r"c:\proj\gruntz.obj",
                    help="DBI module/obj path.")
    ap.add_argument("--bucket-shift", type=int, default=0,
                    help="If > 0, emit C13 line info splitting functions into "
                         "synthetic source files seg_NNNN.cpp keyed by "
                         "(text_offset >> bucket-shift), so the delinker emits "
                         "one .obj per bucket. 0 = single name-based .obj. "
                         "e.g. 16 -> ~31 TUs of 64 KiB .text each.")
    ap.add_argument("--yaml-only", action="store_true",
                    help="emit YAML and stop (skip yaml2pdb + patch).")
    ap.add_argument("--names-map",
                    help="curated RVA->name,unit[,size] overlay CSV (build/gen/"
                         "symbol_names.csv): renames matched FUN_<rva> to the source "
                         "symbol and groups them into a per-unit <unit>.c.obj (requires "
                         "--bucket-shift>0). The optional @size column synthesizes a "
                         "function record for any matched RVA absent from functions.csv.")
    ap.add_argument("--base-dir",
                    help="dir of cl.exe-compiled base objects (build/objdiff/base). "
                         "Their ??_C@... string-pool symbols are used as an oracle to "
                         "rename string constants from Ghidra's DAT_/s_ placeholders to "
                         "the exact MSVC mangling, so source string literals match.")
    args = ap.parse_args()

    read_sections(args.exe)
    print("[synth_pdb] sections: .text 0x%x-0x%x .rdata 0x%x-0x%x .data 0x%x-0x%x"
          % (TEXT_BASE, TEXT_END, RDATA_BASE, RDATA_END, DATA_BASE, DATA_END),
          file=sys.stderr)

    overlay = read_names_map(args.names_map)
    # func rows drive function records (old 3-tuple shape); data rows name the
    # global-variable DATA symbols a matched global is referenced through.
    names_map = {rva: t[:3] for rva, t in overlay.items() if t[3] != "data"}
    data_names = {rva: t[0] for rva, t in overlay.items() if t[3] == "data"}
    funcs = read_functions(args.functions, names_map)
    rdata_syms, data_syms = read_data_symbols(args.symbols)
    if data_names:
        ndat = apply_named_data(rdata_syms, data_syms, data_names)
        print("[synth_pdb] named %d global data symbol(s) from symbol_names.csv" % ndat,
              file=sys.stderr)
    if args.base_dir:
        nstr = apply_string_names(rdata_syms, data_syms, args.exe, args.base_dir)
        print("[synth_pdb] renamed %d string constant(s) to MSVC ??_C@ names" % nstr,
              file=sys.stderr)
    # .idata IAT symbols: proven `__imp__...` decorations only (base objs + the
    # MSVC 5.0 / DX import libs). Required by the data-topology delinker; harmless
    # to the current one (extra records it does not read).
    lib_dirs = [os.path.join(d, "lib") for d in
                (os.environ.get("MSVC_DIR"), os.environ.get("DXSDK_DIR")) if d]
    lib_dirs += [os.path.join(d, "Lib") for d in
                 (os.environ.get("DXSDK_DIR"),) if d]
    iat_syms = read_iat_symbols(args.exe, args.base_dir, lib_dirs)

    print("[synth_pdb] functions: %d  rdata: %d  data: %d  idata: %d  named: %d"
          % (len(funcs), len(rdata_syms), len(data_syms), len(iat_syms),
             len(names_map)),
          file=sys.stderr)

    with open(args.out_yaml, "w") as out:
        emit_yaml(funcs, rdata_syms, data_syms, args.module_path, out,
                  bucket_shift=args.bucket_shift, names_map=names_map,
                  iat_syms=iat_syms)
    print("[synth_pdb] wrote YAML -> %s (%d bytes)"
          % (args.out_yaml, os.path.getsize(args.out_yaml)), file=sys.stderr)

    if args.yaml_only:
        return

    print("[synth_pdb] running llvm-pdbutil yaml2pdb ...", file=sys.stderr)
    subprocess.run(
        ["llvm-pdbutil", "yaml2pdb", "--pdb=%s" % args.out_pdb, args.out_yaml],
        check=True,
    )
    print("[synth_pdb] wrote PDB -> %s (%d bytes)"
          % (args.out_pdb, os.path.getsize(args.out_pdb)), file=sys.stderr)

    empty = find_empty_stream_index(args.out_pdb)
    if empty is None:
        raise RuntimeError("no empty stream found to repoint symbol records at")
    off, cur = patch_symbol_records_stream(args.out_pdb, empty)
    print("[synth_pdb] patched DBI symbol_records_stream @0x%x: 0x%04x -> %d"
          % (off, cur, empty), file=sys.stderr)


if __name__ == "__main__":
    main()
