#!/usr/bin/env python3
"""rescomp.py - the `.rsrc` compiler (there is no `rc.exe`; this module is it).

The candidate EXE has **no resource section at all** without this; retail's is
123,260 B - the single largest divergence in the link. The toolchain tarball ships
`cvtres.exe` (5.00.1668.1, the era-correct converter) and `link.exe` consumes a
`.RES` directly, but it ships **no `rc.exe`**, so the `.rc` -> `.res` step has no
tool. This module is that step: it parses genuine `.rc` grammar and writes the
Win32 32-bit `.RES` container itself.

    python -m gruntz.build.rescomp extract      # retail .rsrc -> config/retail/rsrc/
    python -m gruntz.build.rescomp build        # .rc + art blobs -> build/gen/gruntz.res
    python -m gruntz.build.rescomp census       # what retail's .rsrc contains
    python -m gruntz.build.rescomp verify       # candidate .rsrc vs retail, byte for byte
    python -m gruntz.build.rescomp rc           # decompile retail's authorables to .rc text
    python -m gruntz.build.rescomp rc --emit F  # ... write it (bootstrap/regen the source)
    python -m gruntz.build.rescomp rc --roundtrip   # retail -> text -> parse -> bytes proof
    python -m gruntz.build.rescomp check        # THE GATE: src .rc + art == retail payloads

PROVENANCE - the line this module exists to keep sharp. Two different things feed
the section:

  * **authorable** (57 resources, 89,230 B, 91.5% of the payload) - DIALOG/DIALOGEX
    templates, STRINGTABLE blocks, ACCELERATORS, VERSIONINFO, DLGINIT. These are
    SOURCE: `src/Gruntz/Gruntz.rc` is tracked text in real rc.exe grammar, and the
    build COMPILES it. No retail bytes are carried for them. `rescomp check` (a
    normal-tier build gate) recompiles the `.rc` and byte-compares every payload
    against the retail image itself, so the claim "this text produces those bytes"
    is re-proven on every gated build.
  * **copied** (18 resources, 8,288 B, 8.5%) - the ICON and CURSOR image bits and
    the GROUP_ICON/GROUP_CURSOR directories computed from them. These came from
    `.ico`/`.cur` art files we do not have; they stay carried in
    `config/retail/rsrc/data/` (manifest `provenance` column: `copied`). Art has no
    text form; recovering the bits from the retail image is data provenance, not
    reconstruction.

`config/retail/rsrc/manifest.tsv` keeps the section's PAYLOAD ORDER (the original
`.rc` statement order - rc.exe emits, and cvtres/link keep, statement order) plus
the type/name/lang/size ledger for every resource; authorable rows carry `-` in the
payload column because their bytes come from the `.rc`.

The `.rc` subset grammar (all of it documented rc.exe grammar - the file compiles
under a real rc as well, see the header comment it carries):
    LANGUAGE, STRINGTABLE, ACCELERATORS, DIALOG/DIALOGEX (STYLE/EXSTYLE/CAPTION/
    FONT + generic CONTROL statements), VERSIONINFO, and MFC's DLGINIT raw-data
    blocks. Styles are numeric expressions with `NOT` for the WS_CHILD|WS_VISIBLE
    default that generic CONTROL statements imply; `""` doubles a quote; `\\ooo`
    octal escapes carry the few non-ASCII chars, so the file itself is pure ASCII.

The container (each entry, 4-byte aligned, after a 32-byte null entry):
    DWORD DataSize; DWORD HeaderSize; Type; Name;   (ordinal = FFFF <WORD>, else
    NUL-terminated UTF-16) pad4; DWORD DataVersion; WORD MemoryFlags;
    WORD LanguageId; DWORD Version; DWORD Characteristics; <data> pad4
"""

from __future__ import annotations

import argparse
import re
import struct
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
SRC = REPO / "config/retail/rsrc"
MANIFEST = SRC / "manifest.tsv"
PAYLOADS = SRC / "data"
RC_FILE = REPO / "src/Gruntz/Gruntz.rc"
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

# `authorable` = compiled from src/Gruntz/Gruntz.rc (real source, no carried bytes).
# `copied`     = art bits (or a directory computed from them); no source form exists.
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
    """Order/size ledger for every resource; blobs only for the copied art."""
    SRC.mkdir(parents=True, exist_ok=True)
    PAYLOADS.mkdir(parents=True, exist_ok=True)
    keep = set()
    with open(MANIFEST, "w") as f:
        f.write(HEADER)
        for i, (rtype, name, lang, cp, data) in enumerate(rows, 1):
            if rtype in AUTHORABLE:
                fn = "-"        # compiled from src/Gruntz/Gruntz.rc, never carried
            else:
                fn = f"{i:03d}_{slug(rtype, name)}.bin"
                (PAYLOADS / fn).write_bytes(data)
                keep.add(fn)
            prov = "authorable" if rtype in AUTHORABLE else "copied"
            nm = str(name) if isinstance(name, int) else f'"{name}"'
            f.write(f"{i}\t{RT_NAME.get(rtype, rtype)}\t{nm}\t{lang}\t{cp}\t"
                    f"{len(data)}\t{fn}\t{prov}\n")
    for stale in PAYLOADS.glob("*.bin"):
        if stale.name not in keep:
            stale.unlink()


def _manifest_rows():
    """Raw rows: [(rtype, name, lang, size, payload_fn, provenance)]."""
    rows = []
    for ln in open(MANIFEST):
        if ln.startswith("ordinal\t") or not ln.strip():
            continue
        _o, t, n, lang, _cp, size, fn, prov = ln.rstrip("\n").split("\t")
        rtype = NAME_RT.get(t)
        if rtype is None:
            rtype = int(t)
        name = int(n) if not n.startswith('"') else n[1:-1]
        rows.append((rtype, name, int(lang), int(size), fn, prov))
    return rows


def read_manifest():
    """[(rtype, name, lang, payload)] in retail payload order.

    Authorable rows COMPILE from src/Gruntz/Gruntz.rc; copied rows read their
    carried art blob. `rescomp check` proves both against retail byte-for-byte.
    """
    compiled = None
    out = []
    for rtype, name, lang, size, fn, prov in _manifest_rows():
        if fn == "-":
            if compiled is None:
                compiled = {(t, n): (l, d) for t, n, l, d in compile_rc()}
            got = compiled.get((rtype, name))
            if got is None:
                raise RcError(f"{RC_FILE.name} does not define "
                              f"{RT_NAME.get(rtype, rtype)} {name!r}")
            lg, data = got
            if lg != lang or len(data) != size:
                raise RcError(f"{RC_FILE.name}: {RT_NAME.get(rtype, rtype)} {name!r} "
                              f"compiled to {len(data)} B lang {lg}; manifest says "
                              f"{size} B lang {lang}")
        else:
            data = (PAYLOADS / fn).read_bytes()
            assert len(data) == size, (fn, len(data), size)
        out.append((rtype, name, lang, data))
    return out


# ================================================================ the payload codecs
# Every codec below is a PAIR: a decoder that reads a payload into a model, and an
# encoder that writes the model back. The .rc emitter/parser sit on top of them:
#     retail payload --dec--> model --emit--> .rc text --parse--> model --enc--> bytes
# and `rescomp check` closes the loop against the retail image on every gated build.

WIN_CLASS = {0x80: "BUTTON", 0x81: "EDIT", 0x82: "STATIC", 0x83: "LISTBOX",
             0x84: "SCROLLBAR", 0x85: "COMBOBOX"}
CLASS_ORD = {v: k for k, v in WIN_CLASS.items()}


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


# ================================================================ .rc text emission
# `rescomp rc` decompiles RETAIL's authorable payloads to the grammar below; the
# tracked src/Gruntz/Gruntz.rc was bootstrapped that way and is the build's source.
# Everything emitted is documented rc.exe grammar. An inexpressible byte raises
# RcError - there is deliberately NO binary escape hatch in the text, because a
# resource that needed one would still be carried bytes wearing a text costume.

class RcError(Exception):
    pass


VK_NAME = {
    0x08: "VK_BACK", 0x09: "VK_TAB", 0x0D: "VK_RETURN", 0x13: "VK_PAUSE",
    0x1B: "VK_ESCAPE", 0x20: "VK_SPACE", 0x21: "VK_PRIOR", 0x22: "VK_NEXT",
    0x23: "VK_END", 0x24: "VK_HOME", 0x25: "VK_LEFT", 0x26: "VK_UP",
    0x27: "VK_RIGHT", 0x28: "VK_DOWN", 0x2D: "VK_INSERT", 0x2E: "VK_DELETE",
    0x6A: "VK_MULTIPLY", 0x6B: "VK_ADD", 0x6D: "VK_SUBTRACT", 0x6E: "VK_DECIMAL",
    0x6F: "VK_DIVIDE",
}
VK_NAME.update({0x70 + i: f"VK_F{i + 1}" for i in range(24)})
NAME_VK = {v: k for k, v in VK_NAME.items()}

# fVIRT bits in the compiled table; 0x80 additionally marks the LAST entry.
ACCEL_KW = {"VIRTKEY": 0x01, "ASCII": 0x00, "NOINVERT": 0x02,
            "SHIFT": 0x04, "CONTROL": 0x08, "ALT": 0x10}

MEMFLAG_KWS = {"PRELOAD", "LOADONCALL", "FIXED", "MOVEABLE", "DISCARDABLE",
               "PURE", "IMPURE", "SHARED", "NONSHARED"}

WS_CHILD_VISIBLE = 0x50000000     # the default rc.exe ORs into generic CONTROLs
DS_SETFONT = 0x40


def _esc(s):
    """A string as an rc quoted literal - pure ASCII, exact under _lex."""
    out = ['"']
    for ch in s:
        o = ord(ch)
        if ch == '"':
            out.append('""')
        elif ch == "\\":
            out.append("\\\\")
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\t":
            out.append("\\t")
        elif ch == "\r":
            out.append("\\r")
        elif 32 <= o < 127:
            out.append(ch)
        else:
            try:
                b = ch.encode("cp1252")[0]
            except UnicodeEncodeError:
                raise RcError(f"char U+{o:04X} has no cp1252 spelling for rc text")
            out.append(f"\\{b:03o}")
    out.append('"')
    return "".join(out)


def _rc_name(name):
    if isinstance(name, int):
        return str(name)
    if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
        return name
    return _esc(name)


def _ctl_style(v):
    missing = WS_CHILD_VISIBLE & ~v
    s = f"0x{v:08x}"
    if missing:
        s += f" | NOT 0x{missing:08x}"
    return s


def _rc_stringtable(block, data):
    base = (block - 1) * 16
    rows = dec_stringtable(data)
    if not any(rows):
        raise RcError(f"STRINGTABLE block {block} is all-empty - not expressible")
    L = ["STRINGTABLE DISCARDABLE", "BEGIN"]
    for i, s in enumerate(rows):
        if s:
            L.append(f"    {base + i:<6} {_esc(s)}")
    L.append("END")
    return L


def _rc_accel(name, data):
    rows = dec_accel(data)
    L = [f"{_rc_name(name)} ACCELERATORS DISCARDABLE", "BEGIN"]
    for i, (fl, key, ident) in enumerate(rows):
        last = (i == len(rows) - 1)
        if bool(fl & 0x80) != last:
            raise RcError("accelerator last-entry marker (0x80) misplaced")
        f = fl & ~0x80
        virt = bool(f & 0x01)
        if virt and (0x41 <= key <= 0x5A or 0x30 <= key <= 0x39):
            ev = f'"{chr(key)}"'
        elif virt and key in VK_NAME:
            ev = VK_NAME[key]
        else:
            ev = f"0x{key:02x}"
        opts = ["VIRTKEY" if virt else "ASCII"]
        for bit, kw in ((0x04, "SHIFT"), (0x08, "CONTROL"),
                        (0x10, "ALT"), (0x02, "NOINVERT")):
            if f & bit:
                opts.append(kw)
        L.append(f"    {ev + ',':<12}{ident}, {', '.join(opts)}")
    L.append("END")
    return L


def _rc_dialog(name, data):
    m = dec_dialog(data)
    if m["tail"] or m["menu"] != "" or m["cls"] != "" or m["helpid"]:
        raise RcError(f"dialog {name!r}: menu/class/helpid/tail not expressible")
    if bool(m["font"]) != bool(m["style"] & DS_SETFONT):
        raise RcError(f"dialog {name!r}: FONT and DS_SETFONT disagree")
    kw = "DIALOGEX" if m["ex"] else "DIALOG"
    L = [f"{_rc_name(name)} {kw} DISCARDABLE "
         f"{m['x']}, {m['y']}, {m['cx']}, {m['cy']}",
         f"STYLE 0x{m['style']:08x}"]
    if m["exstyle"]:
        L.append(f"EXSTYLE 0x{m['exstyle']:x}")
    if m["title"]:
        L.append(f"CAPTION {_esc(m['title'])}")
    if m["font"]:
        pt, fn, wt, it, chs = m["font"]
        if m["ex"]:
            L.append(f"FONT {pt}, {_esc(fn)}, {wt}, {it}, 0x{chs:x}")
        else:
            L.append(f"FONT {pt}, {_esc(fn)}")
    L.append("BEGIN")
    idmax = 0xFFFFFFFF if m["ex"] else 0xFFFF
    for (chelp, cex, cst, x, y, cx, cy, cid, ccls, ctxt, blob, has) in m["ctrls"]:
        if chelp or blob or not has:
            raise RcError(f"dialog {name!r}: control creation-data not expressible")
        if isinstance(ccls, int):
            if ccls not in WIN_CLASS:
                raise RcError(f"dialog {name!r}: unknown control class 0x{ccls:x}")
            cn = WIN_CLASS[ccls]
        else:
            cn = _esc(ccls)
        txt = _esc(ctxt) if isinstance(ctxt, str) else str(ctxt)
        line = (f"    CONTROL {txt}, {'-1' if cid == idmax else cid}, {cn}, "
                f"{_ctl_style(cst)}, {x}, {y}, {cx}, {cy}")
        if cex:
            line += f", 0x{cex:x}"
        L.append(line)
    L.append("END")
    return L


def _rc_version(name, data):
    node, end = dec_version(data)
    if end != len(data) or enc_version(node) != data:
        raise RcError("VERSIONINFO payload does not re-encode from its tree")
    key, val, typ, kids = node
    if key != "VS_VERSION_INFO" or typ != 0 or len(val) != 52:
        raise RcError("unexpected VS_VERSION_INFO root node")
    v = struct.unpack("<13I", val)
    if v[0] != 0xFEEF04BD or v[1] != 0x00010000 or v[11] or v[12]:
        raise RcError("fixed file info not expressible as VERSIONINFO statements")
    L = [f"{_rc_name(name)} VERSIONINFO",
         f" FILEVERSION {v[2] >> 16},{v[2] & 0xFFFF},{v[3] >> 16},{v[3] & 0xFFFF}",
         f" PRODUCTVERSION {v[4] >> 16},{v[4] & 0xFFFF},{v[5] >> 16},{v[5] & 0xFFFF}",
         f" FILEFLAGSMASK 0x{v[6]:x}L",
         f" FILEFLAGS 0x{v[7]:x}L",
         f" FILEOS 0x{v[8]:x}L",
         f" FILETYPE 0x{v[9]:x}L",
         f" FILESUBTYPE 0x{v[10]:x}L",
         "BEGIN"]
    for k in kids:
        L += _rc_vnode(k, 1)
    L.append("END")
    return L


def _rc_vnode(node, depth):
    key, val, typ, kids = node
    ind = "    " * depth
    if kids:
        if val:
            raise RcError(f"BLOCK {key!r} carries a value - not expressible")
        out = [f"{ind}BLOCK {_esc(key)}", f"{ind}BEGIN"]
        for k in kids:
            out += _rc_vnode(k, depth + 1)
        out.append(f"{ind}END")
        return out
    if typ == 1:
        s = val.decode("utf-16-le")
        if not s.endswith("\0") or "\0" in s[:-1]:
            raise RcError(f"VALUE {key!r} is not one NUL-terminated string")
        return [f"{ind}VALUE {_esc(key)}, {_esc(s[:-1])}"]
    if len(val) % 2:
        raise RcError(f"binary VALUE {key!r} has odd length - not expressible")
    words = struct.unpack(f"<{len(val) // 2}H", val)
    return [f"{ind}VALUE {_esc(key)}, " + ", ".join(f"0x{w:x}" for w in words)]


def _rc_dlginit(name, data):
    entries, tail = dec_dlginit(data)
    if tail or enc_dlginit(entries, tail) != data:
        raise RcError(f"DLGINIT {name!r}: trailing bytes not expressible")
    L = [f"{_rc_name(name)} DLGINIT", "BEGIN"]
    for ctl, msg, blob in entries:
        L.append(f"    {ctl}, 0x{msg:x}, {len(blob)}, 0")
        words = []
        i = 0
        while i + 1 < len(blob):
            words.append(f"0x{struct.unpack_from('<H', blob, i)[0]:04x},")
            i += 2
        if i < len(blob):
            words.append(f'"\\{blob[i]:03o}",')
        for j in range(0, len(words), 8):
            L.append(" ".join(words[j:j + 8]))
    L.append("    0")
    L.append("END")
    return L


RC_HEADER = """\
// Gruntz.rc - retail GRUNTZ.EXE's authorable resources as SOURCE.
//
// Compiled by gruntz.build.rescomp (the VC5 toolchain ships cvtres.exe but no
// rc.exe, so rescomp is the resource compiler). `rescomp check` - a normal-tier
// build gate - recompiles this file and byte-compares every payload against the
// retail image, so this text is re-proven to produce retail's exact bytes on
// every gated build. Bootstrapped with `rescomp rc --emit`; edits that change
// any compiled byte fail the gate.
//
// Only documented rc.exe grammar is used. Spellings to know:
//   * styles are numeric expressions (the grammar allows numbers anywhere an
//     expression goes); `| NOT 0x...` clears the WS_CHILD|WS_VISIBLE default
//     that generic CONTROL statements imply, exactly as real rc.exe would.
//   * control/string ids are the raw numbers the compiled templates carry
//     (-1 = the IDC_STATIC value); the original resource.h with the symbolic
//     names is in no leak, so inventing names would be fabrication.
//   * `""` doubles a quote; \\ooo octal escapes carry the few non-ASCII chars
//     (cp1252), so this file is pure ASCII.
//
// ICON/CURSOR/GROUP art (18 resources, 8,288 B) has no text form; it stays
// carried in config/retail/rsrc/data/ - manifest.tsv `provenance` says which.
"""


def emit_rc(rows):
    """rows: [(rtype, name, lang, payload)] in retail payload order -> .rc text."""
    L = [RC_HEADER]
    L.append("LANGUAGE 9, 1 // LANG_ENGLISH, SUBLANG_ENGLISH_US (0x409)")
    i = 0
    while i < len(rows):
        rtype, name, lang, data = rows[i]
        if lang != 0x409:
            raise RcError(f"resource {name!r}: lang {lang:#x} needs its own "
                          f"LANGUAGE statement - only 0x409 is modeled")
        if rtype not in AUTHORABLE:
            j = i
            while j < len(rows) and rows[j][0] not in AUTHORABLE:
                j += 1
            kinds = sorted({RT_NAME.get(r[0], str(r[0])) for r in rows[i:j]})
            L.append("")
            L.append(f"// [{j - i} {'/'.join(kinds)} resources here - binary art, "
                     f"carried in config/retail/rsrc/data/]")
            i = j
            continue
        L.append("")
        if rtype == 5:
            L += _rc_dialog(name, data)
        elif rtype == 6:
            L += _rc_stringtable(name, data)
        elif rtype == 9:
            L += _rc_accel(name, data)
        elif rtype == 16:
            L += _rc_version(name, data)
        elif rtype == 240:
            L += _rc_dlginit(name, data)
        i += 1
    return "\n".join(L) + "\n"


# ================================================================ the .rc parser
# The compiler front end: genuine rc grammar (the subset the emitter writes,
# which is all retail needs) -> the same models the encoders above consume.

_NUM_RE = re.compile(r"(-?(?:0[xX][0-9a-fA-F]+|\d+))([lL])?")
_ID_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


_SIMPLE_ESC = {"n": "\n", "t": "\t", "r": "\r", "\\": "\\", '"': '"'}


def _lex_str(text, i, line):
    i += 1                                 # past the opening quote
    out = []
    while True:
        if i >= len(text):
            raise RcError(f"line {line}: unterminated string")
        c = text[i]
        if c == '"':
            if text.startswith('""', i):   # rc's doubled-quote escape
                out.append('"')
                i += 2
                continue
            return "".join(out), i + 1
        if c == "\n":
            raise RcError(f"line {line}: newline inside string")
        if c != "\\":
            out.append(c)
            i += 1
            continue
        e = text[i + 1:i + 2]
        if e in _SIMPLE_ESC:
            out.append(_SIMPLE_ESC[e])
            i += 2
        elif e in "01234567":              # \ooo - a cp1252 byte, 1-3 octal digits
            j = i + 1
            while j < min(i + 4, len(text)) and text[j] in "01234567":
                j += 1
            out.append(bytes([int(text[i + 1:j], 8) & 0xFF]).decode("cp1252"))
            i = j
        elif e in "xX":                    # \xNN - a cp1252 byte, 1-2 hex digits
            j = i + 2
            while j < min(i + 4, len(text)) and \
                    text[j] in "0123456789abcdefABCDEF":
                j += 1
            out.append(bytes([int(text[i + 2:j], 16)]).decode("cp1252"))
            i = j
        else:
            raise RcError(f"line {line}: unsupported escape \\{e}")


def _lex(text):
    toks = []
    i, n, line = 0, len(text), 1
    while i < n:
        c = text[i]
        if c == "\n":
            line += 1
            i += 1
        elif c in " \t\r":
            i += 1
        elif text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            if j < 0:
                raise RcError(f"line {line}: unterminated /* comment")
            line += text.count("\n", i, j)
            i = j + 2
        elif c == '"':
            s, i = _lex_str(text, i, line)
            toks.append(("str", s, line))
        elif c in ",|":
            toks.append(("punct", c, line))
            i += 1
        elif c == "-" or c.isdigit():
            m = _NUM_RE.match(text, i)
            if not m:
                raise RcError(f"line {line}: bad number")
            toks.append(("num", (int(m.group(1), 0), bool(m.group(2))), line))
            i = m.end()
        elif c.isalpha() or c == "_":
            m = _ID_RE.match(text, i)
            toks.append(("id", m.group(0), line))
            i = m.end()
        else:
            raise RcError(f"line {line}: unexpected character {c!r}")
    return toks


class _P:
    def __init__(self, toks):
        self.t = toks
        self.i = 0

    def more(self):
        return self.i < len(self.t)

    def peek(self):
        if not self.more():
            raise RcError("unexpected end of file")
        return self.t[self.i]

    def next(self):
        tok = self.peek()
        self.i += 1
        return tok

    def num(self):
        k, v, ln = self.next()
        if k != "num":
            raise RcError(f"line {ln}: expected a number, got {v!r}")
        return v[0]

    def str_(self):
        k, v, ln = self.next()
        if k != "str":
            raise RcError(f"line {ln}: expected a string, got {v!r}")
        return v

    def ident(self):
        k, v, ln = self.next()
        if k != "id":
            raise RcError(f"line {ln}: expected an identifier, got {v!r}")
        return v.upper()

    def expect_id(self, name):
        v = self.ident()
        if v != name:
            raise RcError(f"expected {name}, got {v!r}")

    def comma(self):
        k, v, ln = self.next()
        if (k, v) != ("punct", ","):
            raise RcError(f"line {ln}: expected ','")

    def accept_punct(self, c):
        if self.more() and self.t[self.i][:2] == ("punct", c):
            self.i += 1
            return True
        return False

    def accept_id(self, name):
        if self.more() and self.t[self.i][0] == "id" and \
                self.t[self.i][1].upper() == name:
            self.i += 1
            return True
        return False

    def at_id(self, name):
        return self.more() and self.t[self.i][0] == "id" and \
            self.t[self.i][1].upper() == name

    def skip_memflags(self):
        while self.more() and self.t[self.i][0] == "id" and \
                self.t[self.i][1].upper() in MEMFLAG_KWS:
            self.i += 1

    def res_name(self):
        k, v, ln = self.next()
        if k == "num":
            return v[0]
        if k in ("id", "str"):
            return v.upper()
        raise RcError(f"line {ln}: expected a resource name")

    def style_expr(self):
        """`term (| term)*`, term = [NOT] number -> (or_bits, not_bits)."""
        orv = notv = 0
        while True:
            neg = self.accept_id("NOT")
            v = self.num() & 0xFFFFFFFF
            if neg:
                notv |= v
            else:
                orv |= v
            if not self.accept_punct("|"):
                return orv, notv


def _parse_accel(p, name, lang):
    p.skip_memflags()
    p.expect_id("BEGIN")
    rows = []
    while not p.at_id("END"):
        k, v, ln = p.next()
        if k == "str":
            ev = v
        elif k == "num":
            ev = v[0]
        elif k == "id":
            ev = NAME_VK.get(v.upper())
            if ev is None:
                raise RcError(f"line {ln}: unknown key name {v}")
        else:
            raise RcError(f"line {ln}: expected an accelerator event")
        p.comma()
        ident = p.num()
        fl = 0
        while p.accept_punct(","):
            kw = p.ident()
            if kw not in ACCEL_KW:
                raise RcError(f"unknown accelerator option {kw}")
            fl |= ACCEL_KW[kw]
        if isinstance(ev, str):
            if len(ev) != 1:
                raise RcError(f"line {ln}: accelerator event must be one char")
            key = ord(ev.upper()) if fl & 0x01 else ord(ev)
        else:
            key = ev
        rows.append([fl, key, ident])
    p.next()
    if not rows:
        raise RcError("empty ACCELERATORS table")
    rows[-1][0] |= 0x80          # the compiled table marks its last entry
    return 9, name, lang, enc_accel([tuple(r) for r in rows])


def _parse_dialog(p, name, lang, ex):
    p.skip_memflags()
    x = p.num()
    p.comma()
    y = p.num()
    p.comma()
    cx = p.num()
    p.comma()
    cy = p.num()
    style = None
    exstyle = 0
    caption = ""
    font = None
    while not p.at_id("BEGIN"):
        kw = p.ident()
        if kw == "STYLE":
            orv, notv = p.style_expr()
            style = orv & ~notv
        elif kw == "EXSTYLE":
            orv, notv = p.style_expr()
            exstyle = orv & ~notv
        elif kw == "CAPTION":
            caption = p.str_()
        elif kw == "FONT":
            pt = p.num()
            p.comma()
            fname = p.str_()
            if ex:
                p.comma()
                wt = p.num()
                p.comma()
                it = p.num()
                p.comma()
                chs = p.num()
                font = (pt, fname, wt, it, chs)
            else:
                font = (pt, fname, None, None, None)
        else:
            raise RcError(f"unsupported dialog statement {kw}")
    if style is None:
        raise RcError(f"dialog {name!r} lacks a STYLE statement")
    if bool(font) != bool(style & DS_SETFONT):
        raise RcError(f"dialog {name!r}: FONT and DS_SETFONT (0x40) must agree")
    p.expect_id("BEGIN")
    idmask = 0xFFFFFFFF if ex else 0xFFFF
    ctrls = []
    while not p.at_id("END"):
        p.expect_id("CONTROL")
        k, v, ln = p.next()
        if k == "str":
            txt = v
        elif k == "num":
            txt = v[0]
        else:
            raise RcError(f"line {ln}: control text must be a string or ordinal")
        p.comma()
        cid = p.num() & idmask
        p.comma()
        k, v, ln = p.next()
        if k == "id":
            cn = CLASS_ORD.get(v.upper())
            if cn is None:
                raise RcError(f"line {ln}: unknown control class {v}")
        elif k == "str":
            cn = v
        else:
            raise RcError(f"line {ln}: control class must be a name or string")
        p.comma()
        orv, notv = p.style_expr()
        cst = (WS_CHILD_VISIBLE | orv) & ~notv    # real rc.exe's generic-CONTROL rule
        p.comma()
        cx0 = p.num()
        p.comma()
        cy0 = p.num()
        p.comma()
        ccx = p.num()
        p.comma()
        ccy = p.num()
        cex = 0
        if p.accept_punct(","):
            orv, notv = p.style_expr()
            cex = orv & ~notv
        ctrls.append((0, cex, cst, cx0, cy0, ccx, ccy, cid, cn, txt, b"", True))
    p.next()
    m = dict(ex=ex, helpid=0, style=style, exstyle=exstyle, x=x, y=y, cx=cx,
             cy=cy, menu="", cls="", title=caption, font=font, ctrls=ctrls,
             tail=b"")
    return 5, name, lang, enc_dialog(m)


def _parse_vnode(p):
    kw = p.ident()
    if kw == "BLOCK":
        nm = p.str_()
        p.expect_id("BEGIN")
        kids = []
        while not p.at_id("END"):
            kids.append(_parse_vnode(p))
        p.next()
        return (nm, b"", 1, kids)
    if kw == "VALUE":
        key = p.str_()
        p.comma()
        if p.peek()[0] == "str":
            s = p.str_()
            return (key, (s + "\0").encode("utf-16-le"), 1, [])
        words = [p.num()]
        while p.accept_punct(","):
            words.append(p.num())
        return (key, b"".join(struct.pack("<H", w & 0xFFFF) for w in words), 0, [])
    raise RcError(f"expected BLOCK or VALUE, got {kw}")


def _parse_version(p, name, lang):
    p.skip_memflags()
    fx = {"FILEVERSION": (0, 0, 0, 0), "PRODUCTVERSION": (0, 0, 0, 0),
          "FILEFLAGSMASK": 0, "FILEFLAGS": 0, "FILEOS": 0, "FILETYPE": 0,
          "FILESUBTYPE": 0}
    while not p.at_id("BEGIN"):
        kw = p.ident()
        if kw in ("FILEVERSION", "PRODUCTVERSION"):
            a = p.num()
            p.comma()
            b = p.num()
            p.comma()
            c = p.num()
            p.comma()
            d = p.num()
            fx[kw] = (a, b, c, d)
        elif kw in fx:
            fx[kw] = p.num()
        else:
            raise RcError(f"unsupported VERSIONINFO statement {kw}")
    p.expect_id("BEGIN")
    kids = []
    while not p.at_id("END"):
        kids.append(_parse_vnode(p))
    p.next()
    a, b, c, d = fx["FILEVERSION"]
    e, f, g, h = fx["PRODUCTVERSION"]
    val = struct.pack("<13I", 0xFEEF04BD, 0x00010000,
                      (a << 16) | b, (c << 16) | d,
                      (e << 16) | f, (g << 16) | h,
                      fx["FILEFLAGSMASK"], fx["FILEFLAGS"], fx["FILEOS"],
                      fx["FILETYPE"], fx["FILESUBTYPE"], 0, 0)
    return 16, name, lang, enc_version(("VS_VERSION_INFO", val, 0, kids))


def _parse_dlginit(p, name, lang):
    # An MFC DLGINIT block is RAW DATA in the text: numbers are WORDs (DWORDs
    # with the L suffix), strings are cp1252 bytes. The stream IS the payload.
    p.skip_memflags()
    p.expect_id("BEGIN")
    buf = bytearray()
    while not p.at_id("END"):
        k, v, ln = p.next()
        if k == "num":
            val, islong = v
            buf += struct.pack("<I", val & 0xFFFFFFFF) if islong else \
                struct.pack("<H", val & 0xFFFF)
        elif k == "str":
            buf += v.encode("cp1252")
        elif (k, v) == ("punct", ","):
            continue
        else:
            raise RcError(f"line {ln}: unexpected token in DLGINIT data")
    p.next()
    return 240, name, lang, bytes(buf)


def parse_rc(text):
    """.rc text -> [(rtype, name, lang, payload)] in rc.exe emission order.

    STRINGTABLE entries bundle into 16-string blocks keyed id>>4; bundles land
    after the other statements in first-appearance order, which is rc.exe's own
    behavior (and, retail's stringtables being last anyway, also file order).
    """
    p = _P(_lex(text))
    lang = 0x409
    items = []
    st = []
    while p.more():
        k, v, ln = p.peek()
        if k == "id" and v.upper() == "LANGUAGE":
            p.next()
            prim = p.num()
            p.comma()
            sub = p.num()
            lang = ((sub & 0x3F) << 10) | (prim & 0x3FF)
        elif k == "id" and v.upper() == "STRINGTABLE":
            p.next()
            p.skip_memflags()
            p.expect_id("BEGIN")
            while not p.at_id("END"):
                sid = p.num()
                p.accept_punct(",")
                st.append((sid, p.str_(), lang))
            p.next()
        else:
            name = p.res_name()
            kw = p.ident()
            if kw == "ACCELERATORS":
                items.append(_parse_accel(p, name, lang))
            elif kw == "VERSIONINFO":
                items.append(_parse_version(p, name, lang))
            elif kw in ("DIALOG", "DIALOGEX"):
                items.append(_parse_dialog(p, name, lang, kw == "DIALOGEX"))
            elif kw == "DLGINIT":
                items.append(_parse_dlginit(p, name, lang))
            else:
                raise RcError(f"line {ln}: unsupported resource type {kw}")
    order, blocks, blang = [], {}, {}
    for sid, s, lg in st:
        b = sid // 16 + 1
        if b not in blocks:
            blocks[b], blang[b] = [""] * 16, lg
            order.append(b)
        if blocks[b][sid % 16]:
            raise RcError(f"duplicate STRINGTABLE id {sid}")
        blocks[b][sid % 16] = s
    for b in order:
        items.append((6, b, blang[b], enc_stringtable(blocks[b])))
    return items


def compile_rc(path=None):
    path = Path(path) if path else RC_FILE
    if not path.is_file():
        raise RcError(f"{path} is missing - the authorable resources have no source")
    return parse_rc(path.read_text(encoding="ascii"))


# ---------------------------------------------------------------- commands
def cmd_extract(_a):
    pe = _pe()
    rows, rs = read_rsrc(pe)
    if not rows:
        sys.exit("retail has no .rsrc?!")
    write_manifest(rows)
    payload = sum(len(r[4]) for r in rows)
    auth = sum(len(r[4]) for r in rows if r[0] in AUTHORABLE)
    print(f"[rescomp] extracted {len(rows)} resources ({payload:,} B payload) "
          f"-> {MANIFEST.relative_to(REPO)}")
    print(f"[rescomp]   authorable {auth:,} B ({100.0*auth/payload:.1f}%) compile "
          f"from {RC_FILE.relative_to(REPO)} (regen: rescomp rc --emit)   "
          f"copied {payload-auth:,} B art -> data/*.bin")


def cmd_build(a):
    rows = read_manifest()
    img = res_file([(t, n, lang, d) for t, n, lang, d in rows])
    out = Path(a.out) if a.out else RES_OUT
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(img)
    ncomp = sum(1 for t, _n, _l, _d in rows if t in AUTHORABLE)
    print(f"[rescomp] {ncomp} resources compiled from {RC_FILE.name} + "
          f"{len(rows) - ncomp} carried art blobs -> {out} ({len(img):,} B)")


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
    d, base, off0 = pe.data, rs["rva"], rs["raw_offset"]
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


def cmd_rc(a):
    """Decompile RETAIL's authorable resources to .rc text (the ground truth)."""
    rows = [(t, n, l, d) for t, n, l, _cp, d in read_rsrc(_pe())[0]]
    text = emit_rc(rows)
    if a.roundtrip:
        arows = [(t, n, l, d) for t, n, l, d in rows if t in AUTHORABLE]
        entries = parse_rc(text)
        emap = {(t, n): (l, d) for t, n, l, d in entries}
        ok = okb = badb = 0
        bad = []
        for t, n, l, d in arows:
            if emap.get((t, n)) == (l, d):
                ok += 1
                okb += len(d)
            else:
                bad.append((t, n))
                badb += len(d)
        print(f"// text round-trip: {ok}/{len(arows)} authorable resources "
              f"re-encode BYTE-IDENTICAL from parsed .rc text "
              f"({okb:,} of {okb + badb:,} B)")
        rcode = 0
        if [e[:2] for e in entries] != [r[:2] for r in arows]:
            print("// ORDER DIVERGES between .rc statements and retail payload order")
            rcode = 1
        for t, n in bad:
            print(f"//   MISMATCH {RT_NAME.get(t, t)} {n!r}")
        return 1 if bad else rcode
    if a.emit:
        Path(a.emit).write_text(text, encoding="ascii")
        print(f"[rescomp] wrote {a.emit} ({len(text.splitlines())} lines)")
        return 0
    print(text, end="")
    return 0


def cmd_check(_a):
    """The build gate: src .rc + carried art == retail's payload bytes, in order."""
    retail, _rs = read_rsrc(_pe())
    mrows = _manifest_rows()
    problems = []
    if len(mrows) != len(retail):
        problems.append(f"manifest has {len(mrows)} rows, retail "
                        f"{len(retail)} resources")
    try:
        entries = compile_rc()
    except RcError as e:
        print(f"[rescomp] check FAILED: {e}", file=sys.stderr)
        return 1
    emap = {}
    for t, n, l, d in entries:
        if (t, n) in emap:
            problems.append(f"duplicate resource in .rc: {RT_NAME.get(t, t)} {n!r}")
        emap[(t, n)] = (l, d)
    exp = [(t, n) for t, n, _l, _s, _f, prov in mrows if prov == "authorable"]
    if [(t, n) for t, n, _l, _d in entries] != exp:
        problems.append(".rc statement order diverges from retail payload order")
    ncomp = nblob = 0
    for (rtype, name, lang, _size, fn, prov), (t, nm, lg, _cp, data) in \
            zip(mrows, retail):
        if (rtype, name, lang) != (t, nm, lg):
            problems.append(f"manifest row {RT_NAME.get(rtype, rtype)} {name!r} != "
                            f"retail {RT_NAME.get(t, t)} {nm!r}")
            continue
        if prov == "authorable":
            got = emap.get((t, nm))
            if got is None:
                problems.append(f"{RC_FILE.name} lacks {RT_NAME.get(t, t)} {nm!r}")
            elif got != (lg, data):
                gl, gd = got
                if gl != lg:
                    problems.append(f"{RT_NAME.get(t, t)} {nm!r}: lang {gl} != {lg}")
                elif len(gd) != len(data):
                    problems.append(f"{RT_NAME.get(t, t)} {nm!r}: compiled "
                                    f"{len(gd)} B, retail {len(data)} B")
                else:
                    at = next(i for i in range(len(gd)) if gd[i] != data[i])
                    problems.append(f"{RT_NAME.get(t, t)} {nm!r}: byte mismatch at "
                                    f"+{at:#x} ({gd[at]:#04x} != {data[at]:#04x})")
            else:
                ncomp += 1
        else:
            if (PAYLOADS / fn).read_bytes() != data:
                problems.append(f"carried art blob {fn} != retail payload")
            else:
                nblob += 1
    if problems:
        for pr in problems:
            print(f"[rescomp] check: {pr}", file=sys.stderr)
        return 1
    tot = sum(len(r[4]) for r in retail)
    print(f"[rescomp] check OK: {ncomp} resources compiled from "
          f"{RC_FILE.relative_to(REPO)} + {nblob} carried art blobs == retail's "
          f".rsrc payloads ({tot:,} B), statement order preserved")
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("extract").set_defaults(fn=cmd_extract)
    r = sub.add_parser("rc", help="decompile retail's authorable resources to .rc")
    r.add_argument("--roundtrip", action="store_true",
                   help="retail -> text -> parse -> bytes, byte-compared")
    r.add_argument("--emit", metavar="PATH",
                   help="write the .rc (bootstrap/regen src/Gruntz/Gruntz.rc)")
    r.set_defaults(fn=cmd_rc)
    ck = sub.add_parser("check",
                        help="gate: .rc + art blobs reproduce retail's payloads")
    ck.set_defaults(fn=cmd_check)
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
