"""gruntz.delink.pdb_synth - synthesize the delinker's named PDB from the Model.

No real PDB exists for Gruntz (1999); this builds one good enough for
vostok-delinker to slice GRUNTZ.EXE into per-unit COFF objects:

  1. Function records from the Model's function bindings: the claimed name (or
     a FUN_<va> placeholder), the claim-resolved extent, and the owning unit as
     a synthetic `c:\\proj\\<unit>.c` source file (C13 line info) so the
     delinker emits one `<unit>.c.obj` per TU. kind=pad rows are never emitted;
     kind=eh rows are superseded by the per-owner EH-band records
     (gruntz.delink.eh_band). ILT/import thunks inherit the body/import name so
     relocations pair by name on both sides.
  2. Data records for every relocation-target address (S_LDATA32), renamed to
     the claimed source names and cl's own `??_C@` string-pool spellings (the
     base objs are the oracle), plus the proven `__imp_` IAT decorations.
  3. `llvm-pdbutil yaml2pdb`, then the DBI-header byte-patch: yaml2pdb cannot
     emit a GSI symbol-records stream and pdb2's `global_symbols()` errors on a
     nil index, so the header is repointed at an existing empty stream.
"""

from __future__ import annotations

import bisect
import hashlib
import struct
from pathlib import Path

from gruntz.core.paths import BUILD
from gruntz.delink import coffx, eh_band, implib
from gruntz.delink.image import retail, sections_of
from gruntz.model import Model

PDB_DIR = BUILD / "pdb"
BASE_DIR = BUILD / "objdiff/base"
MODULE_PATH = r"c:\proj\gruntz.obj"

# Section -> (1-based PE/object-crate section index). The delinker compares
# offset.section against object-crate section indices, 1-based in PE order.
SEG_TEXT, SEG_RDATA, SEG_DATA, SEG_IDATA = 1, 2, 3, 4

#: Functions with no claimed unit fall into address buckets of 2**BUCKET_SHIFT
#: bytes (the granularity the matched functions were delinked under).
BUCKET_SHIFT = 16

#: End of retail's leading incremental-link thunk (ILT) band: every `E9 rel32`
#: below it is a linker forwarder in front of a real body.
ILT_BAND_END = 0x7C20

#: Function channels that attribute a unit (a static-lib label names a library
#: body; its code is never partitioned into a TU).
UNIT_CHANNELS = ("src", "src_compgen", "src_dyninit", "functions_zlib")


def sanitize_name(name: str) -> str:
    """Make a symbol name safe to embed in single-quoted YAML (yaml2pdb takes
    the DisplayName verbatim into the CodeView record)."""
    cleaned = "".join(c if (32 <= ord(c) < 127) else "_" for c in name)
    return cleaned.replace("'", "''")


# --- name derivation from the Model -----------------------------------------

def unit_names(model: Model) -> dict[int, tuple[str, str, int]]:
    """{rva: (name, unit, size)} for the unit-attributed function claims.

    A NAMELESS binding (a src_dyninit pin - the body is a volatile `_$E<n>`
    ordinal cl never lets us spell) stays out: like the old spine it keeps its
    FUN_ placeholder in the address bucket, never a unit attribution."""
    return {b.rva: (b.name, b.unit, b.size) for b in model.functions
            if b.channel in UNIT_CHANNELS and b.name}


def ilt_thunk_names(model: Model, names_map: dict) -> dict[int, str]:
    """Map retail ILT forwarding thunks to curated body names.

    A census row whose bytes are a 5-byte `E9 rel32` (any kind=thunk row, plus
    any derived-size-5 row) inherits a name only when its exact target RVA is
    already in `names_map`; chains resolve to a fixpoint. The raw leading band
    is also walked - a creator table may reference a body through an ILT slot
    the census never carved.
    """
    img = retail()
    text = img.pe.section(".text")
    text_lo, raw_size, raw_ptr = text["va"], text["rsize"], text["rptr"]

    e9: dict[int, int] = {}
    for b in model.functions:
        if b.size != 5 and b.kind != "thunk":
            continue
        off = raw_ptr + b.rva - text_lo
        if not raw_ptr <= off <= raw_ptr + raw_size - 5:
            continue
        if img.data[off] != 0xE9:
            continue
        e9[b.rva] = b.rva + 5 + struct.unpack_from("<i", img.data, off + 1)[0]

    aliases: dict[int, str] = {}
    while True:
        grew = False
        for rva, target in e9.items():
            if rva in aliases or rva in names_map:
                continue
            name = names_map[target][0] if target in names_map \
                else aliases.get(target)
            if name is not None:
                aliases[rva] = name
                grew = True
        if not grew:
            break

    lo = text_lo
    hi = min(ILT_BAND_END, text_lo + raw_size - 5)
    for rva in range(lo, hi):
        if rva in aliases or rva in names_map:
            continue
        off = raw_ptr + rva - text_lo
        if img.data[off] != 0xE9:
            continue
        target = rva + 5 + struct.unpack_from("<i", img.data, off + 1)[0]
        if target in names_map:
            aliases[rva] = names_map[target][0]
    return aliases


def import_thunk_names(iat_syms, names_map: dict) -> dict[int, str]:
    """{rva: `_Foo@N`} for every 6-byte `FF 25 <IAT slot>` import thunk.

    MSVC 5.0 compiles a non-dllimport Win32 call as `call rel32`; the linker
    plants `jmp DWORD PTR [__imp__Foo@N]` and points the call at it. The base
    obj relocates against `_Foo@N` directly, so the thunk must carry that name
    or the same call compares unequal against an anonymous FUN_<va>. Derived
    only from IAT slots with a PROVEN `__imp_` decoration.
    """
    slots = {}
    for slot, dec in iat_syms:
        slots[slot] = "_" + dec[len("__imp__"):] if dec.startswith("__imp__") \
            else dec[len("__imp_"):]
    img = retail()
    text = img.pe.section(".text")
    text_lo, raw_size, raw_ptr = text["va"], text["rsize"], text["rptr"]
    blob = img.data[raw_ptr:raw_ptr + raw_size]
    out = {}
    pos = blob.find(b"\xff\x25")
    while pos != -1 and pos + 6 <= len(blob):
        target = struct.unpack_from("<I", blob, pos + 2)[0] - img.image_base
        rva = text_lo + pos
        if target in slots and rva not in names_map:
            out[rva] = slots[target]
        pos = blob.find(b"\xff\x25", pos + 1)
    return out


def function_records(model: Model, names_map, thunk_names, import_thunks,
                     drop_spans, log) -> list[tuple[int, int, str]]:
    """(rva, size, name) for every emitted function record, sorted.

    kind=pad rows never emit; rows inside `drop_spans` (the EH funclet band)
    are superseded by the per-owner records already merged into `names_map`.
    """
    image_base = retail().image_base
    spans = sorted(drop_spans)
    span_lo = [lo for lo, _hi in spans]

    def superseded(rva: int) -> bool:
        index = bisect.bisect_right(span_lo, rva) - 1
        return index >= 0 and rva < spans[index][1]

    out, seen, dropped = [], set(), 0
    for b in model.functions:
        if b.size <= 0 or b.kind == "pad":
            continue
        if superseded(b.rva):
            dropped += 1
            continue
        size = b.size
        if b.rva in names_map:
            name = names_map[b.rva][0]
            at_size = names_map[b.rva][2]
            if at_size:
                size = min(at_size, size)
        elif b.rva in thunk_names:
            name = thunk_names[b.rva]
        elif b.rva in import_thunks:
            name = import_thunks[b.rva]
        else:
            name = f"FUN_{image_base + b.rva:08x}"
        out.append((b.rva, size, name))
        seen.add(b.rva)
    if dropped:
        log(f"superseded {dropped} census row(s) inside the EH funclet band")

    # A band thunk absent from the census still needs a record so its callers'
    # relocations resolve to the curated name (exactly the 5-byte `E9 rel32`).
    synth_thunks = 0
    for rva, name in sorted(thunk_names.items()):
        if rva not in seen:
            out.append((rva, 5, name))
            seen.add(rva)
            synth_thunks += 1
    if synth_thunks:
        log(f"synthesized {synth_thunks} ILT thunk record(s) absent from the census")

    synth_imp = 0
    for rva, name in sorted(import_thunks.items()):
        if rva not in seen:
            out.append((rva, 6, name))
            seen.add(rva)
            synth_imp += 1
    if synth_imp:
        log(f"synthesized {synth_imp} import thunk record(s) from named IAT slots")

    # names_map entries absent from the census: the EH-band per-owner records.
    synth = 0
    for rva, (name, _unit, size) in sorted(names_map.items()):
        if rva in seen or size <= 0:
            continue
        out.append((rva, size, name))
        seen.add(rva)
        synth += 1
    if synth:
        log(f"synthesized {synth} record(s) at RVAs the census never carved")
    out.sort()
    return out


# --- data symbols ------------------------------------------------------------

def reloc_data_symbols() -> tuple[list, list]:
    """(rdata, data) [(rva, DAT_<va>)] for every PE relocation target.

    Every absolute address operand is listed in the PE's HIGHLOW relocation
    directory; its stored value identifies the exact target that needs a PDB
    symbol. Source names are overlaid later.
    """
    img = retail()
    bounds = sections_of()
    rd_lo, rd_hi = bounds[".rdata"]
    da_lo, da_hi = bounds[".data"]
    rdata: dict[int, str] = {}
    data: dict[int, str] = {}
    for site in img.reloc_sites:
        value = img.u32(site)
        if value is None:
            continue
        rva = value - img.image_base
        name = f"DAT_{value:08x}"
        if rd_lo <= rva < rd_hi:
            rdata.setdefault(rva, name)
        elif da_lo <= rva < da_hi:
            data.setdefault(rva, name)
    return sorted(rdata.items()), sorted(data.items())


def apply_named_data(rdata_syms, data_syms, data_names) -> int:
    """Overlay the claimed names onto the DAT_ placeholders (adding a symbol
    when the address is not a relocation target). Mutates; returns count."""
    bounds = sections_of()
    n = 0
    for syms, (lo, hi) in ((rdata_syms, bounds[".rdata"]),
                           (data_syms, bounds[".data"])):
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


def apply_string_names(rdata_syms, data_syms, base_dir) -> int:
    """Rename string-constant symbols to their MSVC `??_C@` pool names.

    cl names a pooled literal by length + a VC5 16-bit checksum + the text; we
    never recompute it - the base objects ARE cl output, so their `??_C@`
    symbols give the exact name for each literal's bytes. Mutates; returns the
    count renamed.
    """
    str_map = coffx.build_string_map(Path(base_dir))
    if not str_map:
        return 0
    img = retail()
    n = 0
    for syms in (rdata_syms, data_syms):
        for i, (rva, name) in enumerate(syms):
            if name.startswith("??_C@"):
                continue
            cs = img.cstring(rva)
            if cs and cs in str_map:
                syms[i] = (rva, str_map[cs])
                n += 1
    return n


# --- YAML emission -----------------------------------------------------------

def func_source_file(rva: int, names_map) -> str:
    """`c:\\proj\\<unit>.c` for a claimed function, else the address bucket
    `c:\\proj\\seg_NNNN.cpp` (both under the engine root the delinker strips)."""
    if rva in names_map:
        return r"c:\proj\%s.c" % names_map[rva][1]
    text_lo, _hi = sections_of()[".text"]
    bucket = (rva - text_lo) >> BUCKET_SHIFT
    return r"c:\proj\seg_%04x.cpp" % bucket


def emit_yaml(funcs, rdata_syms, data_syms, iat_syms, names_map, out) -> None:
    """Write the yaml2pdb description: one DBI module, C13 line info that
    attributes each function to its synthetic source file, S_GPROC32 records
    for .text and S_LDATA32 for .rdata/.data/.idata."""
    bounds = sections_of()
    text_base = bounds[".text"][0]
    rdata_base = bounds[".rdata"][0]
    data_base = bounds[".data"][0]
    idata_base = bounds[".idata"][0]
    w = out.write

    func_files, files_seen, files_set = [], [], set()
    for rva, _size, _name in funcs:
        sf = func_source_file(rva, names_map)
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
    w("    - Module:          '%s'\n" % MODULE_PATH)
    w("      ObjFile:         '%s'\n" % MODULE_PATH)

    w("      SourceFiles:\n")
    for sf in files_seen:
        w("        - '%s'\n" % sf)
    w("      Subsections:\n")
    w("        - !FileChecksums\n")
    w("          Checksums:\n")
    for sf in files_seen:
        # Deterministic 16-byte checksum from the path (content is fake).
        cks = hashlib.md5(sf.encode()).hexdigest()
        w("            - FileName:        '%s'\n" % sf)
        w("              Kind:            MD5\n")
        w("              Checksum:        %s\n" % cks.upper())
    for (rva, size, _name), sf in zip(funcs, func_files):
        off = rva - text_base
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
    for rva, size, name in funcs:
        off = rva - text_base
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
    for syms, base, seg in ((rdata_syms, rdata_base, SEG_RDATA),
                            (data_syms, data_base, SEG_DATA),
                            (iat_syms, idata_base, SEG_IDATA)):
        for rva, name in syms:
            w("          - Kind:            S_LDATA32\n")
            w("            DataSym:\n")
            w("              Type:            0\n")
            w("              Offset:          %d\n" % (rva - base))
            w("              Segment:         %d\n" % seg)
            w("              DisplayName:     '%s'\n" % sanitize_name(name))

    # Top-level PDB string table: the source paths line info references.
    w("StringTable:\n")
    for sf in files_seen:
        w("  - '%s'\n" % sf)


# --- DBI header patch ---------------------------------------------------------
#
# DBIHeader layout (pdb2 crate / microsoft-pdb): u16 symbol_records_stream at
# offset 0x14. yaml2pdb writes 0xFFFF (nil) there when no GSI is present; we
# repoint it at an existing empty stream so pdb2's global_symbols() returns an
# empty table and the module iteration supplies all the real data.

SYMREC_OFFSET_IN_DBI = 0x14
_MSF_MAGIC = b"Microsoft C/C++ MSF 7.00\r\n\x1aDS\x00\x00\x00"


def find_empty_stream_index(pdb_path: Path) -> int | None:
    """Index of an existing 0-byte stream (e.g. /LinkInfo), via pdbutil dump."""
    import re
    from gruntz.tool import pdbutil
    for line in pdbutil.dump(pdb_path, "--streams").splitlines():
        m = re.search(r"Stream\s+(\d+)\s+\(\s*0 bytes\)", line)
        if m:
            return int(m.group(1))
    return None


def dbi_stream_file_offset(pdb_path: Path) -> int:
    """File offset of the DBI stream (stream 3) first byte, from the MSF
    superblock + stream directory. The DBI header always fits the first
    block (block size 4096)."""
    data = Path(pdb_path).read_bytes()
    if not data.startswith(_MSF_MAGIC):
        raise ValueError(f"{pdb_path}: unexpected MSF magic")
    (block_size, _free_map, _num_blocks, num_dir_bytes,
     _unknown, block_map_addr) = struct.unpack_from("<IIIIII", data, 32)
    num_dir_blocks = (num_dir_bytes + block_size - 1) // block_size
    dir_block_ptrs = struct.unpack_from(
        "<%dI" % num_dir_blocks, data, block_map_addr * block_size)
    directory = b"".join(
        data[b * block_size: b * block_size + block_size]
        for b in dir_block_ptrs)[:num_dir_bytes]
    pos = 0
    (num_streams,) = struct.unpack_from("<I", directory, pos)
    pos += 4
    sizes = list(struct.unpack_from("<%dI" % num_streams, directory, pos))
    pos += 4 * num_streams
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
    return stream_blocks[DBI_STREAM][0] * block_size


def patch_symbol_records_stream(pdb_path: Path, target_stream_index: int):
    off = dbi_stream_file_offset(pdb_path) + SYMREC_OFFSET_IN_DBI
    with open(pdb_path, "r+b") as f:
        f.seek(off)
        cur = struct.unpack("<H", f.read(2))[0]
        f.seek(off)
        f.write(struct.pack("<H", target_stream_index))
    return off, cur


# --- driver -------------------------------------------------------------------

def synth(model: Model, out_yaml: Path | None = None, out_pdb: Path | None = None,
          base_dir: Path = BASE_DIR, log=None, yaml_only: bool = False) -> dict:
    """The whole synthesis; returns {names_map, band, funcs, ...} for reuse."""
    import sys
    log = log or (lambda m: print(f"[pdb_synth] {m}", file=sys.stderr))
    out_yaml = Path(out_yaml or PDB_DIR / "gruntz_named.yaml")
    out_pdb = Path(out_pdb or PDB_DIR / "gruntz_named.pdb")
    exe = retail().pe.path

    names_map = unit_names(model)
    band = eh_band.groups(exe, names_map)
    band_spans = [(g.start, g.end) for g in band]
    for rva, name, unit, size in eh_band.records(band):
        names_map.setdefault(rva, (name, unit, size))
    log(f"EH funclet band: {len(band)} group(s), "
        f"{sum(g.end - g.start for g in band)} B, "
        f"over {len({g.unit for g in band})} owning unit(s)")

    # Static-lib labels: rename-only (the unit stays empty, so library code is
    # partitioned into the linker bucket, never a TU). An ILT-band forwarder's
    # label is deferred so the forwarded BODY's name can win the slot.
    library_rvas: set[int] = set()
    deferred_thunks: dict[int, str] = {}
    nlib = 0
    for b in model.functions:
        if b.channel != "functions_static_libs" or b.rva in names_map:
            continue
        if b.kind == "thunk" and b.rva < ILT_BAND_END:
            deferred_thunks[b.rva] = b.name
            continue
        names_map[b.rva] = (b.name, "", 0)
        library_rvas.add(b.rva)
        nlib += 1
    if nlib:
        log(f"applied {nlib} tracked library symbol name(s)")

    data_names = {b.rva: b.name for b in model.data if b.channel and b.name}
    neh = 0
    for rva, name, _unit, _size in eh_band.data_records(band):
        if data_names.setdefault(rva, name) == name:
            neh += 1
    log(f"EH funcinfo: named {neh} `.xdata$x` datum(s)")

    thunk_names = ilt_thunk_names(model, names_map)
    if thunk_names:
        log(f"propagated {len(thunk_names)} curated body name(s) to ILT thunks")
    nfold = 0
    for rva, name in sorted(deferred_thunks.items()):
        if rva in thunk_names:
            nfold += 1
            continue
        names_map[rva] = (name, "", 0)
        library_rvas.add(rva)
    if nfold:
        log(f"{nfold} carved ILT thunk label(s) superseded by the body name")

    iat_syms, unresolved = implib.resolve_iat(retail().import_slots(), base_dir)
    log(f".idata: named {len(iat_syms)}/{len(iat_syms) + len(unresolved)} "
        "IAT slot(s)")
    for slot, label in unresolved:
        log(f".idata SKIP 0x{slot:06x} {label}: no exact __imp_ decoration "
            "(never guessing @N)")
    import_thunks = import_thunk_names(iat_syms, names_map)

    funcs = function_records(model, names_map, thunk_names, import_thunks,
                             band_spans, log)
    rdata_syms, data_syms = reloc_data_symbols()
    ndat = apply_named_data(rdata_syms, data_syms, data_names)
    log(f"named {ndat} global data symbol(s) from the Model")
    nstr = apply_string_names(rdata_syms, data_syms, base_dir)
    log(f"renamed {nstr} string constant(s) to MSVC ??_C@ names")
    log(f"functions: {len(funcs)}  rdata: {len(rdata_syms)}  "
        f"data: {len(data_syms)}  idata: {len(iat_syms)}  "
        f"named: {len(names_map)}")

    out_yaml.parent.mkdir(parents=True, exist_ok=True)
    with open(out_yaml, "w") as out:
        emit_yaml(funcs, rdata_syms, data_syms, iat_syms, names_map, out)
    log(f"wrote YAML -> {out_yaml}")
    result = {"names_map": names_map, "band": band, "funcs": funcs,
              "iat_syms": iat_syms, "yaml": out_yaml, "pdb": out_pdb}
    if yaml_only:
        return result

    from gruntz.tool import pdbutil
    pdbutil.yaml2pdb(out_yaml, out_pdb)
    empty = find_empty_stream_index(out_pdb)
    if empty is None:
        raise RuntimeError("no empty stream found to repoint symbol records at")
    off, cur = patch_symbol_records_stream(out_pdb, empty)
    log(f"patched DBI symbol_records_stream @0x{off:x}: 0x{cur:04x} -> {empty}")
    return result


def main() -> int:
    import argparse
    from gruntz.model import resolve
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out-yaml", type=Path, default=PDB_DIR / "gruntz_named.yaml")
    ap.add_argument("--out-pdb", type=Path, default=PDB_DIR / "gruntz_named.pdb")
    ap.add_argument("--yaml-only", action="store_true",
                    help="emit YAML and stop (skip yaml2pdb + patch)")
    a = ap.parse_args()
    synth(resolve(), a.out_yaml, a.out_pdb, yaml_only=a.yaml_only)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
