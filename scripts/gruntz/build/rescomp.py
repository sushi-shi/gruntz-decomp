#!/usr/bin/env python3
"""rescomp.py - the `.rsrc` data-segment generator (there is no `rc.exe`).

The candidate EXE has **no resource section at all**; retail's is 123,260 B - the
single largest divergence in the link. The toolchain tarball ships `cvtres.exe`
(5.00.1668.1, the era-correct converter) and `link.exe` consumes a `.RES`
directly, but it ships **no `rc.exe`**, so the `.rc` -> `.res` step has no tool.
This module is that step: it writes the Win32 32-bit `.RES` container itself.

    python -m gruntz.build.rescomp extract      # retail .rsrc -> config/retail/rsrc/
    python -m gruntz.build.rescomp build        # config/retail/rsrc/ -> build/gen/gruntz.res
    python -m gruntz.build.rescomp census       # what retail's .rsrc contains
    python -m gruntz.build.rescomp verify       # candidate .rsrc vs retail, byte for byte

PROVENANCE - the line this module does NOT let you blur. Two very different things
live in `config/retail/rsrc/`:

  * **authorable** - DIALOG/DIALOGEX templates, STRINGTABLE blocks, ACCELERATORS,
    VERSIONINFO, DLGINIT. Every byte of these is a deterministic function of `.rc`
    TEXT that a person wrote, and `rescomp rc` prints that text back. Regenerating
    them from a `.rc` source would be reconstruction.
  * **copied** - the ICON and CURSOR image bits (8,092 B, 8.3% of the payload) and
    the GROUP_ICON/GROUP_CURSOR directories computed from them. These came from
    `.ico`/`.cur` art files we do not have. Recovering the bits from the retail
    image and re-emitting them is DATA PROVENANCE, not reconstruction, and the
    manifest's `provenance` column says so on every row.

Today every row is carried as extracted bytes, so the whole section is `copied`.
That is honest and it is what makes the game link with resources; it is not a
match claim. `rescomp rc` is the path to converting the authorable 91.7% into
real source.

The container (each entry, 4-byte aligned, after a 32-byte null entry):
    DWORD DataSize; DWORD HeaderSize; Type; Name;   (ordinal = FFFF <WORD>, else
    NUL-terminated UTF-16) pad4; DWORD DataVersion; WORD MemoryFlags;
    WORD LanguageId; DWORD Version; DWORD Characteristics; <data> pad4
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
SRC = REPO / "config/retail/rsrc"
MANIFEST = SRC / "manifest.tsv"
PAYLOADS = SRC / "data"
RES_OUT = REPO / "build/gen/gruntz.res"

RT_NAME = {
    1: "CURSOR", 2: "BITMAP", 3: "ICON", 4: "MENU", 5: "DIALOG", 6: "STRING",
    7: "FONTDIR", 8: "FONT", 9: "ACCELERATOR", 10: "RCDATA", 11: "MESSAGETABLE",
    12: "GROUP_CURSOR", 14: "GROUP_ICON", 16: "VERSION", 17: "DLGINCLUDE",
    19: "PLUGPLAY", 20: "VXD", 21: "ANICURSOR", 22: "ANIICON", 23: "HTML",
    24: "MANIFEST",
    # MFC's private types (afxres.h): RT_DLGINIT is the combo/list box seed data
    # rc.exe emits beside a dialog, RT_TOOLBAR the toolbar button layout.
    240: "DLGINIT", 241: "TOOLBAR",
}
NAME_RT = {v: k for k, v in RT_NAME.items()}

# The default MEMORYFLAGS rc.exe writes per type (MOVEABLE|PURE|DISCARDABLE etc).
# They live only in the .RES container - the PE resource directory has no field for
# them - so they cannot change a byte of the linked .rsrc. Recorded for fidelity.
MEMFLAGS = {1: 0x1030, 3: 0x1030, 5: 0x1030, 6: 0x1030, 9: 0x1030,
            12: 0x1030, 14: 0x1030, 16: 0x0030, 240: 0x1030}

# `authorable` = every byte is a deterministic function of .rc text someone wrote.
# `copied`     = art bits (or a directory computed from them); no source form here.
AUTHORABLE = {5, 6, 9, 16, 240}


# ---------------------------------------------------------------- retail reader
def _pe(path=None):
    from gruntz.core.pe import PE
    return PE(path)


def read_rsrc(pe):
    """[(type, name, lang, codepage, bytes)] in PAYLOAD-ADDRESS order.

    Payload order is the order the resources appeared in the original `.rc`
    (rc.exe emits, and cvtres/link keep, statement order); the DIRECTORY is
    sorted separately by the linker. So this order is itself recovered source
    structure, not an implementation detail.
    """
    rs = next((s for s in pe.sections if s["name"] == ".rsrc"), None)
    if rs is None:
        return [], None
    base, off0, d = rs["rva"], rs["raw_offset"], pe.data
    out = []

    def walk(dir_rva, ids):
        o = off0 + (dir_rva - base)
        nnamed, nid = struct.unpack_from("<HH", d, o + 12)
        for i in range(nnamed + nid):
            nameoff, dataoff = struct.unpack_from("<II", d, o + 16 + i * 8)
            if nameoff & 0x80000000:
                so = off0 + (nameoff & 0x7FFFFFFF)
                ln = struct.unpack_from("<H", d, so)[0]
                ident = d[so + 2:so + 2 + ln * 2].decode("utf-16-le")
            else:
                ident = nameoff
            if dataoff & 0x80000000:
                walk(base + (dataoff & 0x7FFFFFFF), ids + [ident])
            else:
                drva, dsz, cp, _ = struct.unpack_from("<IIII", d, off0 + dataoff)
                po = off0 + (drva - base)
                t, nm, lg = (ids + [ident])[:3]
                out.append((t, nm, lg, cp, d[po:po + dsz], drva))
    walk(base, [])
    out.sort(key=lambda r: r[5])
    return [r[:5] for r in out], rs


# ---------------------------------------------------------------- .RES writer
def _ident(v):
    if isinstance(v, int):
        return struct.pack("<HH", 0xFFFF, v)
    return v.encode("utf-16-le") + b"\0\0"


def res_entry(rtype, name, lang, data, memflags=None, version=0, chars=0):
    hdr = _ident(rtype) + _ident(name)
    hdr += b"\0" * ((-len(hdr)) % 4)
    if memflags is None:
        memflags = MEMFLAGS.get(rtype, 0x1030)
    hdr += struct.pack("<IHHII", 0, memflags, lang, version, chars)
    head = struct.pack("<II", len(data), len(hdr) + 8) + hdr
    body = data + b"\0" * ((-len(data)) % 4)
    return head + body


def res_file(entries):
    """entries: [(type, name, lang, data)] -> the whole .RES image."""
    out = bytearray(res_entry(0xFFFF_0000 and 0, 0, 0, b""))  # placeholder, replaced
    out = bytearray()
    # the mandatory 32-byte null entry
    out += struct.pack("<II", 0, 0x20) + struct.pack("<HHHH", 0xFFFF, 0, 0xFFFF, 0)
    out += struct.pack("<IHHII", 0, 0, 0, 0, 0)
    for rtype, name, lang, data in entries:
        out += res_entry(rtype, name, lang, data)
    return bytes(out)


# ---------------------------------------------------------------- manifest I/O
HEADER = ("ordinal\ttype\tname\tlang\tcodepage\tsize\tpayload\tprovenance\n")


def slug(rtype, name):
    t = RT_NAME.get(rtype, f"type{rtype}")
    n = name if isinstance(name, int) else "".join(
        c if c.isalnum() or c in "_-" else "_" for c in name)
    return f"{t}_{n}"


def write_manifest(rows):
    SRC.mkdir(parents=True, exist_ok=True)
    PAYLOADS.mkdir(parents=True, exist_ok=True)
    with open(MANIFEST, "w") as f:
        f.write(HEADER)
        for i, (rtype, name, lang, cp, data) in enumerate(rows, 1):
            fn = f"{i:03d}_{slug(rtype, name)}.bin"
            (PAYLOADS / fn).write_bytes(data)
            prov = "authorable" if rtype in AUTHORABLE else "copied"
            nm = str(name) if isinstance(name, int) else f'"{name}"'
            f.write(f"{i}\t{RT_NAME.get(rtype, rtype)}\t{nm}\t{lang}\t{cp}\t"
                    f"{len(data)}\t{fn}\t{prov}\n")


def read_manifest():
    rows = []
    for ln in open(MANIFEST):
        if ln.startswith("ordinal\t") or not ln.strip():
            continue
        _o, t, n, lang, _cp, size, fn, _prov = ln.rstrip("\n").split("\t")
        rtype = NAME_RT.get(t, None)
        if rtype is None:
            rtype = int(t)
        name = int(n) if not n.startswith('"') else n[1:-1]
        data = (PAYLOADS / fn).read_bytes()
        assert len(data) == int(size), (fn, len(data), size)
        rows.append((rtype, name, int(lang), data))
    return rows


# ---------------------------------------------------------------- commands
def cmd_extract(_a):
    pe = _pe()
    rows, rs = read_rsrc(pe)
    if not rows:
        sys.exit("retail has no .rsrc?!")
    write_manifest(rows)
    payload = sum(len(r[4]) for r in rows)
    print(f"[rescomp] extracted {len(rows)} resources ({payload:,} B payload) "
          f"-> {MANIFEST.relative_to(REPO)}")
    auth = sum(len(r[4]) for r in rows if r[0] in AUTHORABLE)
    print(f"[rescomp]   authorable {auth:,} B ({100.0*auth/payload:.1f}%)  "
          f"copied {payload-auth:,} B ({100.0*(payload-auth)/payload:.1f}%)")


def cmd_build(a):
    rows = read_manifest()
    img = res_file([(t, n, lang, d) for t, n, lang, d in rows])
    out = Path(a.out) if a.out else RES_OUT
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(img)
    print(f"[rescomp] {len(rows)} resources -> {out} ({len(img):,} B)")


def cmd_census(a):
    pe = _pe(a.exe)
    rows, rs = read_rsrc(pe)
    if not rows:
        print("no .rsrc section")
        return
    import collections
    by = collections.Counter()
    n = collections.Counter()
    for t, name, lang, cp, data in rows:
        by[RT_NAME.get(t, f"type{t}")] += len(data)
        n[RT_NAME.get(t, f"type{t}")] += 1
    payload = sum(by.values())
    print(f".rsrc rva 0x{rs['rva']:06x}  vsize {rs['virtual_size']:,}  "
          f"raw {rs['raw_size']:,}")
    print(f"{'TYPE':<14}{'#':>4}{'BYTES':>10}{'share':>8}  provenance")
    for k in sorted(by, key=lambda k: -by[k]):
        t = NAME_RT.get(k, -1)
        print(f"{k:<14}{n[k]:>4}{by[k]:>10,}{100.0*by[k]/payload:>7.1f}%  "
              f"{'authorable' if t in AUTHORABLE else 'copied'}")
    print(f"{'TOTAL':<14}{sum(n.values()):>4}{payload:>10,}")
    auth = sum(by[k] for k in by if NAME_RT.get(k, -1) in AUTHORABLE)
    print(f"\nauthorable {auth:,} B ({100.0*auth/payload:.1f}%)   "
          f"copied {payload-auth:,} B ({100.0*(payload-auth)/payload:.1f}%)")
    # overhead + the zero tail
    hi = 0
    d, base, off0 = pe.data, rs["rva"], rs["raw_offset"]
    tail = rs["raw_size"]
    body = d[off0:off0 + rs["raw_size"]]
    z = len(body) - len(body.rstrip(b"\0"))
    print(f"directory + name strings + entries: "
          f"{rs['virtual_size'] - payload - z:,} B")
    print(f"trailing zero fill                : {z:,} B "
          f"({100.0*z/rs['raw_size']:.1f}% of the section)")


def cmd_verify(a):
    r = _pe()
    c = _pe(a.exe or REPO / "build/exe/GRUNTZ.candidate.EXE")
    rr, rs = read_rsrc(r)
    cr, cs = read_rsrc(c)
    if cs is None:
        print("candidate has NO .rsrc section")
        return 1
    print(f"retail    {len(rr)} resources, {sum(len(x[4]) for x in rr):,} B payload, "
          f"section vsize {rs['virtual_size']:,}")
    print(f"candidate {len(cr)} resources, {sum(len(x[4]) for x in cr):,} B payload, "
          f"section vsize {cs['virtual_size']:,}")
    rk = {(t, n, l): d for t, n, l, cp, d in rr}
    ck = {(t, n, l): d for t, n, l, cp, d in cr}
    same = sum(1 for k in rk if k in ck and ck[k] == rk[k])
    diff = sorted(k for k in rk if k in ck and ck[k] != rk[k])
    miss = sorted(set(rk) - set(ck))
    extra = sorted(set(ck) - set(rk))
    print(f"identical payloads {same}/{len(rk)}   differing {len(diff)}   "
          f"missing {len(miss)}   extra {len(extra)}")
    for k in diff[:20]:
        print(f"  DIFF  {RT_NAME.get(k[0], k[0])} {k[1]!r} lang {k[2]}: "
              f"{len(rk[k])} vs {len(ck[k])} B")
    for k in miss[:20]:
        print(f"  MISS  {RT_NAME.get(k[0], k[0])} {k[1]!r} lang {k[2]}")
    for k in extra[:20]:
        print(f"  EXTRA {RT_NAME.get(k[0], k[0])} {k[1]!r} lang {k[2]}")
    # byte-level on the whole section
    rb = r.data[rs["raw_offset"]:rs["raw_offset"] + rs["raw_size"]]
    cb = c.data[cs["raw_offset"]:cs["raw_offset"] + cs["raw_size"]]
    n = min(len(rb), len(cb))
    eq = sum(1 for i in range(n) if rb[i] == cb[i])
    print(f"raw section bytes: {eq:,}/{max(len(rb), len(cb)):,} equal "
          f"(sections are RVA-relative, so pointer fields differ by construction)")
    return 0 if not (diff or miss or extra) else 1


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("extract").set_defaults(fn=cmd_extract)
    b = sub.add_parser("build")
    b.add_argument("--out")
    b.set_defaults(fn=cmd_build)
    c = sub.add_parser("census")
    c.add_argument("--exe")
    c.set_defaults(fn=cmd_census)
    v = sub.add_parser("verify")
    v.add_argument("--exe")
    v.set_defaults(fn=cmd_verify)
    a = ap.parse_args(argv)
    sys.exit(a.fn(a) or 0)


if __name__ == "__main__":
    sys.path.insert(0, str(REPO / "scripts"))
    main()
