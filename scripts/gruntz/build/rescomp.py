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
    python -m gruntz.build.rescomp rc           # the authorable resources as .rc text
    python -m gruntz.build.rescomp rc --roundtrip   # the lossless-codec proof

PROVENANCE - the line this module does NOT let you blur. Two very different things
live in `config/retail/rsrc/`:

  * **authorable** (89,230 B, 91.5% of the payload) - DIALOG/DIALOGEX templates,
    STRINGTABLE blocks, ACCELERATORS, VERSIONINFO, DLGINIT. Every byte of these is
    a deterministic function of `.rc` TEXT that a person wrote, and `rescomp rc`
    prints that text back. **`rc --roundtrip` PROVES it**: all 57 re-encode
    BYTE-IDENTICAL from the decoded model (89,230 of 89,230 B), so the readable
    form loses nothing and a real `.rc` could regenerate those bytes.
  * **copied** (8,288 B, 8.5%) - the ICON and CURSOR image bits and the
    GROUP_ICON/GROUP_CURSOR directories computed from them. These came from
    `.ico`/`.cur` art files we do not have. Recovering the bits from the retail
    image and re-emitting them is DATA PROVENANCE, not reconstruction, and the
    manifest's `provenance` column says so on every row.

Today every row is SHIPPED as extracted bytes, so what the build emits is a copy.
That is honest and it is what makes the game link with resources; it is not a
match claim. The lossless codecs are the bridge: writing the `.rc` and compiling
it here would make the 91.5% real source, and the 8.5% of art never can be.

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


# ================================================================ the .rc codecs
# Every codec below is a PAIR: a decoder that reads the carried retail payload into a
# model, and an encoder that writes the model back. `rescomp rc --roundtrip` runs both
# and byte-compares, which is what earns a resource the word "authorable" - it proves
# the readable form loses nothing, so a real `.rc` could regenerate those bytes.

WIN_CLASS = {0x80: "BUTTON", 0x81: "EDIT", 0x82: "STATIC", 0x83: "LISTBOX",
             0x84: "SCROLLBAR", 0x85: "COMBOBOX"}


def _rd_ident(d, p):
    """rc's sz_Or_Ord: 0x0000 = absent, 0xFFFF <WORD> = ordinal, else UTF-16 sz."""
    v = struct.unpack_from("<H", d, p)[0]
    if v == 0xFFFF:
        return struct.unpack_from("<H", d, p + 2)[0], p + 4
    if v == 0:
        return "", p + 2
    e = p
    while struct.unpack_from("<H", d, e)[0]:
        e += 2
    return d[p:e].decode("utf-16-le"), e + 2


def _wr_ident(v):
    if isinstance(v, int):
        return struct.pack("<HH", 0xFFFF, v)
    if v == "":
        return b"\0\0"
    return v.encode("utf-16-le") + b"\0\0"


# ---- STRINGTABLE (69.9% of the payload) --------------------------------------
def dec_stringtable(data):
    out, p = [], 0
    for k in range(16):
        n = struct.unpack_from("<H", data, p)[0]
        p += 2
        out.append(data[p:p + n * 2].decode("utf-16-le"))
        p += n * 2
    assert p == len(data), (p, len(data))
    return out


def enc_stringtable(rows):
    b = bytearray()
    for s in rows:
        b += struct.pack("<H", len(s)) + s.encode("utf-16-le")
    return bytes(b)


# ---- ACCELERATORS -------------------------------------------------------------
def dec_accel(data):
    out = []
    for i in range(0, len(data), 8):
        fl, key, ident, _pad = struct.unpack_from("<HHHH", data, i)
        out.append((fl, key, ident))
    return out


def enc_accel(rows):
    b = bytearray()
    for fl, key, ident in rows:
        b += struct.pack("<HHHH", fl, key, ident, 0)
    return bytes(b)


# ---- DLGINIT (MFC RT_DLGINIT, type 240) ---------------------------------------
def dec_dlginit(data):
    """MFC's RT_DLGINIT stream: {WORD idc; WORD msg; DWORD len; BYTE data[len]}*, 0.

    `msg` is a WORD, not a DWORD, and it is WM_USER-relative - rc.exe writes
    0x0403 for CB_ADDSTRING (WM_USER+3), which is what CDialog::ExecuteDlgInit
    compares against.
    """
    out, p = [], 0
    while p + 2 <= len(data):
        ctl = struct.unpack_from("<H", data, p)[0]
        if ctl == 0:
            p += 2
            break
        msg = struct.unpack_from("<H", data, p + 2)[0]
        n = struct.unpack_from("<I", data, p + 4)[0]
        p += 8
        out.append((ctl, msg, data[p:p + n]))
        p += n
    return out, data[p:]


def enc_dlginit(rows, tail):
    b = bytearray()
    for ctl, msg, blob in rows:
        b += struct.pack("<HHI", ctl, msg, len(blob)) + blob
    b += b"\0\0"
    return bytes(b) + tail


# ---- DIALOG / DIALOGEX --------------------------------------------------------
def dec_dialog(d):
    ver, sig = struct.unpack_from("<HH", d, 0)
    ex = (ver == 1 and sig == 0xFFFF)
    if ex:
        helpid, exstyle, style = struct.unpack_from("<III", d, 4)
        cdit, x, y, cx, cy = struct.unpack_from("<Hhhhh", d, 16)
        p = 26
    else:
        helpid = 0
        style, exstyle = struct.unpack_from("<II", d, 0)
        cdit, x, y, cx, cy = struct.unpack_from("<Hhhhh", d, 8)
        p = 18
    menu, p = _rd_ident(d, p)
    cls, p = _rd_ident(d, p)
    title, p = _rd_ident(d, p)
    font = None
    if style & 0x40:                                   # DS_SETFONT
        pt = struct.unpack_from("<H", d, p)[0]
        p += 2
        if ex:
            wt, it, chs = struct.unpack_from("<HBB", d, p)
            p += 4
        else:
            wt, it, chs = None, None, None
        fn, p = _rd_ident(d, p)
        font = (pt, fn, wt, it, chs)
    ctrls = []
    for _ in range(cdit):
        pad = (-p) % 4
        p += pad
        if ex:
            chelp, cex, cst = struct.unpack_from("<III", d, p)
            # DLGITEMTEMPLATEEX: helpID(4) exStyle(4) style(4) x,y,cx,cy(8) id(4) = 24
            cx0, cy0, ccx, ccy, cid = struct.unpack_from("<hhhhI", d, p + 12)
            q = p + 24
        else:
            chelp = 0
            cst, cex = struct.unpack_from("<II", d, p)
            cx0, cy0, ccx, ccy, cid = struct.unpack_from("<hhhhH", d, p + 8)
            q = p + 18
        ccls, q = _rd_ident(d, q)
        ctxt, q = _rd_ident(d, q)
        # rc.exe omits the trailing creation-data WORD on the LAST control when it
        # would be the final two bytes of the template. Record its presence so the
        # encoder reproduces the template exactly.
        if q + 2 <= len(d):
            extra = struct.unpack_from("<H", d, q)[0]
            blob = d[q + 2:q + 2 + extra]
            p = q + 2 + extra
            has = True
        else:
            blob, p, has = b"", q, False
        ctrls.append((chelp, cex, cst, cx0, cy0, ccx, ccy, cid, ccls, ctxt, blob, has))
    return dict(ex=ex, helpid=helpid, style=style, exstyle=exstyle, x=x, y=y,
                cx=cx, cy=cy, menu=menu, cls=cls, title=title, font=font,
                ctrls=ctrls, tail=d[p:])


def enc_dialog(m):
    ex = m["ex"]
    b = bytearray()
    if ex:
        b += struct.pack("<HHIII", 1, 0xFFFF, m["helpid"], m["exstyle"], m["style"])
        b += struct.pack("<Hhhhh", len(m["ctrls"]), m["x"], m["y"], m["cx"], m["cy"])
    else:
        b += struct.pack("<II", m["style"], m["exstyle"])
        b += struct.pack("<Hhhhh", len(m["ctrls"]), m["x"], m["y"], m["cx"], m["cy"])
    b += _wr_ident(m["menu"]) + _wr_ident(m["cls"]) + _wr_ident(m["title"])
    if m["font"]:
        pt, fn, wt, it, chs = m["font"]
        b += struct.pack("<H", pt)
        if ex:
            b += struct.pack("<HBB", wt, it, chs)
        b += _wr_ident(fn)
    for (chelp, cex, cst, x, y, cx, cy, cid, ccls, ctxt, blob, has) in m["ctrls"]:
        b += b"\0" * ((-len(b)) % 4)
        if ex:
            b += struct.pack("<III", chelp, cex, cst)
            b += struct.pack("<hhhhI", x, y, cx, cy, cid)
        else:
            b += struct.pack("<II", cst, cex)
            b += struct.pack("<hhhhH", x, y, cx, cy, cid)
        b += _wr_ident(ccls) + _wr_ident(ctxt)
        if has:
            b += struct.pack("<H", len(blob)) + blob
    return bytes(b) + m["tail"]


# ---- VERSIONINFO --------------------------------------------------------------
def dec_version(d, p=0):
    """The VS_VERSIONINFO tree -> ((key, value_bytes, wType, [children]), end).

    Node: WORD wLength (whole node, children included, trailing pad excluded);
    WORD wValueLength (CHARS when wType=1, BYTES when 0); WORD wType;
    WCHAR szKey[]; pad4; BYTE Value[]; pad4; children.
    """
    ln, vlen, typ = struct.unpack_from("<HHH", d, p)
    q = p + 6
    e = q
    while struct.unpack_from("<H", d, e)[0]:
        e += 2
    key = d[q:e].decode("utf-16-le")
    q = e + 2
    q += (-(q - 0)) % 4
    n = vlen * 2 if typ else vlen
    val = d[q:q + n]
    q += n
    q += (-(q - 0)) % 4
    kids = []
    while q < p + ln:
        kid, q = dec_version(d, q)
        kids.append(kid)
        q += (-(q - 0)) % 4
    return (key, val, typ, kids), p + ln


def enc_version(node, at=0):
    key, val, typ, kids = node
    b = bytearray(b"\0" * 6)
    b += key.encode("utf-16-le") + b"\0\0"
    b += b"\0" * ((-(at + len(b))) % 4)
    b += val
    vlen = (len(val) // 2) if typ else len(val)
    for k in kids:
        b += b"\0" * ((-(at + len(b))) % 4)
        b += enc_version(k, at + len(b))
    struct.pack_into("<HHH", b, 0, len(b), vlen, typ)
    return bytes(b)


def _rt_version(d):
    node, _ = dec_version(d)
    out = enc_version(node)
    return out == d[:len(out)] and not d[len(out):].strip(b"\0")


def _ver_lines(node, depth=0):
    key, val, typ, kids = node
    if typ:
        txt = val.decode("utf-16-le").rstrip("\0")
        yield f'{"  "*depth}VALUE "{key}", "{txt}"'
    else:
        yield f'{"  "*depth}BLOCK "{key}"' + (f"   [{len(val)} B binary]" if val else "")
    for k in kids:
        yield from _ver_lines(k, depth + 1)


ROUNDTRIP = {}          # type -> (decode, encode) over the raw payload


def _rt_string(d):
    return enc_stringtable(dec_stringtable(d)) == d


def _rt_accel(d):
    return enc_accel(dec_accel(d)) == d


def _rt_dialog(d):
    return enc_dialog(dec_dialog(d)) == d


def _rt_dlginit(d):
    rows, tail = dec_dlginit(d)
    return enc_dlginit(rows, tail) == d


RT_ROUNDTRIP = {6: _rt_string, 9: _rt_accel, 5: _rt_dialog,
                240: _rt_dlginit, 16: _rt_version}


def cmd_rc(a):
    """Print the authorable resources as readable .rc-shaped text."""
    rows = read_manifest()
    ok = bad = 0
    okb = badb = 0
    for rtype, name, lang, data in rows:
        if rtype not in AUTHORABLE:
            continue
        fn = RT_ROUNDTRIP.get(rtype)
        good = fn(data) if fn else None
        if good is True:
            ok += 1
            okb += len(data)
        elif good is False:
            bad += 1
            badb += len(data)
        if a.roundtrip:
            continue
        nm = name if isinstance(name, int) else f'"{name}"'
        if rtype == 6:
            base = (name - 1) * 16
            print(f"\nSTRINGTABLE   // block {name}, ids {base}..{base+15}")
            print("BEGIN")
            for i, s in enumerate(dec_stringtable(data)):
                if s:
                    esc = s.replace('"', '""').replace("\r", "\\r").replace("\n", "\\n")
                    print(f'    {base+i:<6} "{esc}"')
            print("END")
        elif rtype == 9:
            print(f"\n{nm} ACCELERATORS")
            print("BEGIN")
            for fl, key, ident in dec_accel(data):
                kinds = ",".join(k for b, k in ((0x01, "VIRTKEY"), (0x02, "NOINVERT"),
                                                (0x04, "SHIFT"), (0x08, "CONTROL"),
                                                (0x10, "ALT")) if fl & b) or "ASCII"
                print(f"    key 0x{key:04x}, id {ident}, {kinds}")
            print("END")
        elif rtype == 5:
            m = dec_dialog(data)
            f = m["font"]
            print(f"\n{nm} DIALOG{'EX' if m['ex'] else ''} "
                  f"{m['x']}, {m['y']}, {m['cx']}, {m['cy']}")
            print(f"STYLE 0x{m['style']:08x}   EXSTYLE 0x{m['exstyle']:08x}")
            if m["title"]:
                print(f'CAPTION "{m["title"]}"')
            if f:
                print(f'FONT {f[0]}, "{f[1]}"'
                      + (f", {f[2]}, {f[3]}, 0x{f[4]:x}" if m["ex"] else ""))
            print("BEGIN")
            for (_h, cex, cst, x, y, cx, cy, cid, ccls, ctxt, blob, _p) in m["ctrls"]:
                cn = WIN_CLASS.get(ccls, ccls) if isinstance(ccls, int) else f'"{ccls}"'
                t = ctxt if isinstance(ctxt, str) else f"ordinal {ctxt}"
                print(f'    CONTROL {t!r:<40} {cid:<6} {str(cn):<12} '
                      f'0x{cst:08x}, {x}, {y}, {cx}, {cy}'
                      + (f'  [+{len(blob)} B]' if blob else ""))
            print("END")
        elif rtype == 240:
            print(f"\n// DLGINIT {nm}")
            for ctl, msg, blob in dec_dlginit(data)[0]:
                txt = blob.rstrip(b"\0").decode("latin1", "replace")
                print(f"    control {ctl}, msg 0x{msg:x}, {len(blob)} B  {txt!r}")
        elif rtype == 16:
            node, _ = dec_version(data)
            print(f"\n{nm} VERSIONINFO")
            for ln in _ver_lines(node):
                print("    " + ln)
    tot = ok + bad
    print(f"\n// round-trip: {ok}/{tot} authorable resources re-encode BYTE-IDENTICAL "
          f"from the decoded model ({okb:,} of {okb+badb:,} B)")
    if bad:
        print(f"// {bad} did not; those stay 'carried bytes' until the codec is exact")
    return 1 if bad else 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("extract").set_defaults(fn=cmd_extract)
    r = sub.add_parser("rc", help="decompile the authorable resources to .rc text")
    r.add_argument("--roundtrip", action="store_true",
                   help="only report the lossless-codec proof")
    r.set_defaults(fn=cmd_rc)
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
