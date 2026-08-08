"""Sieve: our base references a DIFFERENT CRT symbol than retail does.

A C function's COFF name carries one decoration underscore, so source `strcmp`
emits `_strcmp`.  A name with TWO leading underscores therefore means the source
itself wrote the underscore form: retail's `__strcmpi` proves the devs typed
`_strcmpi`, not the OLDNAMES alias `strcmpi`.  Where two CRT names are the same
routine (`_stricmp` / `_strcmpi`, `_chkstk` / `_alloca_probe`) the link succeeds
either way, so nothing but this comparison catches the wrong spelling.

A count mismatch on a name means one of three things, in rough order of value:
  * we call a routine retail never calls, or vice versa -- a MISSING or EXTRA
    call in our reconstruction (`_rand` 131 vs 126 found three functions short a
    `rand()`; `_atoi` 10 vs 11 is one extra `atoi` in `CPlay::LoadByMode`)
  * we spelled a CRT call differently from retail  (`__stricmp` 2 vs 0)
  * a construct difference upstream  (`_atexit` counts an object with a
    destructor at static lifetime; `__alloca_probe` means retail called `_alloca`
    where we declared a large local, `__chkstk` is the large-frame probe)

READ THIS BEFORE TREATING A ROW AS A DEFECT
-------------------------------------------
A target obj's relocations are produced by the DELINKER, not by a compiler, so
the two sides are not automatically symmetric.  One asymmetry is structural and
is filtered out here (`--no-filter` to see it again):

  * **compiler-generated `$E` COMDATs.**  cl puts each file-scope object's
    dynamic initializer in its own `$E<n>` COMDAT, which has no RVA pin and so is
    never delinked into a target obj.  Un-filtered, that alone reads `_atexit`
    12 vs 100.  Filtered, both sides are 12 -- the same twelve function-local
    statics, function for function.
Four more are NOT filtered, because a filter for them would be guesswork or
would hide a real finding:

  * **intra-unit references and unpaired COMDATs.**  Our base is
    COMDAT-per-function, so a call to a sibling in the same TU needs a
    relocation; the delinked target sometimes resolves the same call as a direct
    displacement instead (`_deflate_slow` 0 vs 6, `_deflate_fast` 0 vs 3) and
    sometimes does not (`_flush_pending` 10 vs 10) -- so "the referent is defined
    in this obj" is NOT a safe filter, it invents rows in the other direction.
    Likewise a base COMDAT objdiff never pairs still counts (`_memmove` 0 vs 2,
    two `CArray::InsertAt` bodies in a unit that is 100%).  Check the unit's
    score before believing a zlib/template row.

  * an unsized/unenrolled datum makes the delinker fall back to
    `<previous named symbol> + addend`, so references to it land on whatever
    happens to precede it (`_inflate_mask` 176 vs 12 is the 0x2293xx band of
    unclaimed statics resolving onto zlib's mask table 0x3db4 bytes earlier);
  * a local static's name differs between the sides when a
    `config/static_data_copies.tsv` row spells it C-style while cl emits the
    mangled `?name@?1??Fn@@...@4U2@A$S<n>` form (`_s_default_rect_butemgr` 7 vs 0;
    those five `CButeMgr::Get*` functions are all 100%, so it costs nothing).

    python -m gruntz.audit.crt_symbols [--min-delta 1] [--all] [--no-filter] [--normalized]
"""

import argparse
import collections
import glob
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
# Names whose absence/presence is a delinker artefact, not a source fact.
IGNORE = {"__except_list"}
C_NAME = re.compile(r"^_{1,2}[a-z][a-z0-9_]*$")
RELOC = re.compile(r"IMAGE_REL_I386_(?:REL32|DIR32)\s+(\S+)")
# cl's compiler-generated dynamic-initializer / cleanup funclet COMDAT, raw
# (`_$E116`) and after canonicalize_data_symbols content-addresses it.
COMPGEN_OWNER = re.compile(r"^_?\$[ES][0-9]+$|^\$anon_data_[0-9a-f]{16,}_[0-9]+$")


def _coff(path: Path):
    """(sections, defined_names) where a section is {rel: [(off, sym)], owners: [name]}."""
    d = path.read_bytes()
    nsec = struct.unpack_from("<H", d, 2)[0]
    symptr, nsym = struct.unpack_from("<II", d, 8)
    optsz = struct.unpack_from("<H", d, 16)[0]
    strtab = symptr + nsym * 18

    def sname(off):
        return d[off:d.find(b"\0", off)].decode("latin1")

    idxname, owners, defined = {}, collections.defaultdict(list), set()
    i = 0
    while i < nsym:
        o = symptr + i * 18
        raw = d[o:o + 8]
        nm = (sname(strtab + struct.unpack_from("<I", d, o + 4)[0])
              if raw[:4] == b"\0\0\0\0" else raw.rstrip(b"\0").decode("latin1"))
        val = struct.unpack_from("<I", d, o + 8)[0]
        sec = struct.unpack_from("<h", d, o + 12)[0]
        idxname[i] = nm
        if sec > 0 and not nm.startswith("."):
            owners[sec].append((val, nm))
            defined.add(nm)
        i += 1 + d[o + 17]

    secs = []
    for s in range(nsec):
        o = 20 + optsz + s * 40
        rrp = struct.unpack_from("<I", d, o + 24)[0]
        nrel = struct.unpack_from("<H", d, o + 32)[0]
        rel = []
        for r in range(nrel):
            ro = rrp + r * 10
            off, si, ty = struct.unpack_from("<IIH", d, ro)
            if ty in (0x06, 0x14):     # DIR32 / REL32 only (not DIR32NB, which
                rel.append((off, idxname.get(si, "?")))   # only appears in .debug$F
        secs.append({"rel": rel, "owners": sorted(owners.get(s + 1, []))})
    return secs, defined


def _owner_at(section, off):
    best = None
    for val, nm in section["owners"]:
        if val <= off:
            best = nm
        else:
            break
    return best


def _counts(directory: str, filter_structural: bool):
    """Counter of C-decorated relocation names under `directory`."""
    c = collections.Counter()
    dropped = collections.Counter()
    for obj in sorted(glob.glob(os.path.join(directory, "*.obj"))):
        if not filter_structural:
            out = subprocess.run(["llvm-objdump", "-r", obj],
                                 capture_output=True, text=True).stdout
            for name in RELOC.findall(out):
                if C_NAME.match(name) and name not in IGNORE:
                    c[name] += 1
            continue
        try:
            secs, _defined = _coff(Path(obj))
        except (OSError, struct.error, ValueError):
            continue
        for sec in secs:
            for off, name in sec["rel"]:
                if not C_NAME.match(name) or name in IGNORE:
                    continue
                owner = _owner_at(sec, off)
                # a compiler-generated $E/$S COMDAT is never delinked
                if owner and COMPGEN_OWNER.match(owner):
                    dropped["compiler-generated $E COMDAT"] += 1
                    continue
                c[name] += 1
    return c, dropped


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--min-delta", type=int, default=1)
    ap.add_argument("--all", action="store_true", help="also list matching names")
    ap.add_argument("--no-filter", action="store_true",
                    help="do not drop the three structural asymmetries (see the docstring)")
    ap.add_argument("--normalized", action="store_true",
                    help="read build/objdiff/normalized/ (the copies objdiff actually "
                         "scores) instead of the raw base/target objs. It reconciles "
                         "jump-table labels (_inflate_blocks 17 vs 1 disappears) but "
                         "also rewrites intra-function table entries to <fn>+addend, "
                         "which adds base-only rows in zlib's deflate")
    args = ap.parse_args(argv)

    root = REPO / "build" / "objdiff"
    if args.normalized and (root / "normalized").is_dir():
        root = root / "normalized"
    filt = not args.no_filter
    t, _ = _counts(str(root / "target"), filt)
    b, dropped = _counts(str(root / "base"), filt)

    rows = []
    for name in sorted(set(t) | set(b)):
        dt, db = t.get(name, 0), b.get(name, 0)
        if args.all or (dt != db and abs(dt - db) >= args.min_delta):
            rows.append((abs(dt - db), name, dt, db))
    rows.sort(reverse=True)

    print(f"objs: {root.relative_to(REPO)}")
    if dropped:
        print("base relocs dropped as structurally unpairable: "
              + ", ".join(f"{k}={v}" for k, v in sorted(dropped.items())))
    print(f"crt-symbols: {len(rows)} name(s) where base and target disagree\n")
    print(f"{'target':>7} {'base':>7}  {'delta':>6}  symbol")
    for d, name, dt, db in rows:
        print(f"{dt:>7} {db:>7}  {db - dt:>+6}  {name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
