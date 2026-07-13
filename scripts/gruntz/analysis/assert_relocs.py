#!/usr/bin/env python3
"""assert_relocs.py - OPT-IN reloc-TARGET audit. Ported from the sibling homm2-decomp project
(scripts/homm2/build/assert_relocs.py); same toolchain, same delink+objdiff pipeline.

WHY THIS EXISTS (and why reloc_fidelity.py is not enough):

objdiff MASKS relocations when scoring, so it never checks a reloc's TARGET. A "100% match" can
silently reference the WRONG global/const/field/function - or CALL A FABRICATED FUNCTION that is
declared, never defined, and (because we compile-but-don't-link) never caught as an unresolved
external. A wrong reloc costs ~0.005%, which rounds to "100.00%" in the display.

This complements the two tools we already have:
  * reloc_fidelity.py  - offset-EXACT, per-site, and only over byte-EXACT functions. Precise, but
                         blind to everything below 100% and brittle under instruction reordering.
  * link_defects.py    - "can the linker resolve this symbol at all" (reads the real .LIB tables).
  * assert_relocs.py   - "does this function point at the addresses RETAIL points at" - ORDER-
                         INDEPENDENT, so it works on NEAR-exact (>=99.5%) functions too, which is
                         most of the tree. Plus a FAKE check for unresolvable '?'-mangled symbols.

Order-INDEPENDENT: resolve each side's relocs to a MULTISET of final RVAs (symbol RVA + instruction
addend; REL32 -> the symbol's own RVA) and flag any address the BASE references that retail never
does (or references fewer times). A multiset - not a set - so an injected wrong ref is still caught
when that address is ALSO legitimately referenced elsewhere in the same function.

Data offsets come from symbol_names.csv (authoritative CodeView RVAs); globals with no CodeView
symbol are read from their DATA(0x..) annotation in src/ or include/.

OPT-IN, not a hard build gate: it also surfaces unreproducible link artifacts (chiefly the
delinker's COMDAT-folded empty stubs), which are a delinker-side concern, not a source bug.

    python -m gruntz.analysis.assert_relocs                 # audit every near-exact function
    python -m gruntz.analysis.assert_relocs --unit butemgr  # one unit
    python -m gruntz.analysis.assert_relocs 0x0008dc60      # review ONE function (works at a wall)

Exits 1 on any wrong/fabricated reloc target.
"""
import sys, os, re, csv, json, glob, struct, argparse, subprocess
from collections import Counter
from pathlib import Path


# Resolve REPO from the CWD first, not __file__: in a worktree the shell's PYTHONPATH can point at
# MAIN's scripts/, so `python -m ...` loads main's module and __file__ would mis-resolve to main.
def _find_repo():
    for base in (Path.cwd(), Path(__file__).resolve().parent):
        for p in (base, *base.parents):
            if (p / "flake.nix").exists() and (p / "build" / "objdiff").exists():
                return p
    return next(
        (p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
        Path(__file__).resolve().parents[3],
    )


REPO = str(_find_repo())
IMAGE_BASE = 0x400000
THRESHOLD = 99.5  # a wrong reloc costs ~0.005% -> audit NEAR-exact, not just ==100

# ---------------------------------------------------------------------------
# ILT jmp-thunk canonicalisation. THE dominant false-positive class if omitted.
#
# The retail EXE was linked INCREMENTALLY, so essentially every cross-TU call goes
# through a 5-byte ILT jmp-thunk (`jmp rel32`) in the 0x1000..0x5000 band. The
# delinker therefore names the TARGET-side rel32 after the THUNK ("ClearFrame2",
# "DtorStatus" - Ghidra's thunk labels), while cl gives our BASE-side rel32 the real
# mangled body symbol (?ClearFrame2@CSBI_MenuItem@@QAEXXZ -> the body's rva). Two
# different RVAs for ONE call that both route to the same body.
#
# Comparing raw RVAs flags every such call as "WRONG: base references <body> - retail
# never does". It is not wrong: it is the same callee, and at link time our direct
# call resolves to the same symbol (the linker re-creates the thunk itself). So
# canonicalise BOTH sides by chasing jmp-thunks to the ultimate body before comparing
# - exactly what reloc_fidelity.py already does (its resolve_thunk); this tool was
# ported without that step. resolve_thunk is the identity for data addresses and for
# a real function start, so it is safe to apply to every resolved reloc target.
# ---------------------------------------------------------------------------
EXE = os.environ.get("GRUNTZ_EXE", "")
TEXT = (0x1000, 0x1E626B)
_D = open(EXE, "rb").read() if EXE and os.path.isfile(EXE) else None


def _fo(r):  # RVA -> file offset (same section map as reloc_fidelity.fo)
    if r < 0x1E6800:
        return r - 0x1000 + 0x400
    if r < 0x208000:
        return 0x1E5800 + (r - 0x1E7000)
    return 0x206800 + (r - 0x208000)


def resolve_thunk(t, d=0):
    """Chase `jmp rel32` ILT thunks to the ultimate body. Identity for data / real starts."""
    if _D is None:
        return t
    while d < 4 and TEXT[0] <= t < TEXT[1] and _D[_fo(t)] == 0xE9:
        t = t + 5 + struct.unpack("<i", _D[_fo(t) + 1 : _fo(t) + 5])[0]
        d += 1
    return t


def _norm(s):
    # llvm-objdump renders each non-printable mangled byte as one '_'; symbol_names.csv stores it
    # as the UTF-8 replacement char.
    s = s.replace("\xef\xbf\xbd", "_")
    return "".join(c if 0x20 <= ord(c) < 0x7F else "_" for c in s)


def _key(s):
    # encoding-stable key for ??_C@ string names: the ASCII "??_C@_<len>@<hash>@" prefix
    m = re.match(r"(\?\?_C@_\w+@\w+@)", s)
    return m.group(1) if m else _norm(s)


def load_symbols():
    sym, dups = {}, {}
    csv_path = os.path.join(REPO, "build/gen/symbol_names.csv")
    for r in csv.reader(open(csv_path, encoding="latin-1")):
        if len(r) >= 2:
            try:
                v = int(r[0], 16)
            except ValueError:
                continue
            for k in (r[1], _norm(r[1]), _key(r[1])):
                # content-hashed string names (??_C@) COLLIDE - two distinct addresses can share one
                # name (e.g. two "!" literals). Record every address per name so an ambiguous target
                # reloc may match ANY of them (they are value-identical), not just the last loaded.
                sym.setdefault(k, v)
                dups.setdefault(k, set()).add(v)
    data = {}
    for pat in ("src/**/*.cpp", "include/**/*.h"):
        for f in glob.glob(os.path.join(REPO, pat), recursive=True):
            for ln in open(f, encoding="latin-1"):
                m = re.search(r"DATA\(0x([0-9a-fA-F]+)\).*?\b([A-Za-z_]\w*)\s*(?:\[|;|=)", ln)
                if m:
                    data[m.group(2)] = int(m.group(1), 16) - IMAGE_BASE
    # The TARGET side's symbol names are the delinker's, i.e. the fake PDB's, i.e. GHIDRA's
    # labels - NOT cl's mangled names and NOT symbol_names.csv. Without this table a target
    # reloc naming a Ghidra-labelled ILT thunk ("ClearFrame2", "DtorStatus") resolves to
    # None and is SILENTLY DROPPED from the retail multiset, so every such call reports as
    # "base references <body> - retail never does". Ghidra gives the thunk and the body the
    # SAME label (0x3fd5 ClearFrame2 = thunk, 0xe81a0 ClearFrame2 = body), so both land in
    # `dups` and resolve_thunk() collapses them onto the one body. This was the dominant
    # false-positive class in the port (~1400 of the 1547 WRONG).
    # (functions.csv: rva,size,name)  (symbols.csv: rva,name,is_primary - the DATA labels,
    # e.g. `0x21aef4,DAT_0061aef4`, which is how the delinker names an unlabelled global's
    # ELEMENT: base says `?g_bfP@@3PAIA + 0x44`, retail says `DAT_0061aef4` - one address.)
    for fname, ncol in (("functions.csv", 2), ("symbols.csv", 1)):
        gpath = os.path.join(REPO, "build/ghidra-enrich/exports", fname)
        if not os.path.exists(gpath):
            continue
        for r in csv.reader(open(gpath, encoding="latin-1")):
            if len(r) <= ncol or not r[0].startswith("0x"):
                continue
            try:
                v = int(r[0], 16)
            except ValueError:
                continue  # the export carries a few negative/bogus rvas
            if v < 0:
                continue
            for k in (r[ncol], _norm(r[ncol]), _key(r[ncol])):
                dups.setdefault(k, set()).add(v)  # dups ONLY: never shadow cl's own symbols
    return sym, data, dups


# Ghidra auto-labels embed the VA: DAT_0061aef4 / FUN_00401000 / LAB_ / PTR_ / UNK_ ...
_AUTO = re.compile(r"^(?:DAT|FUN|LAB|SUB|UNK|PTR|s|u)_(?:[A-Za-z]+_)*([0-9a-fA-F]{8})$")

# --- MSVC mangled string constants -----------------------------------------------------
# The delinker INVENTS the name for a string literal, using MSVC's content-hash mangling
# (??_C@_0BF@JCCE@GAME_TELEPORTERCLOSE?$AA@). That name is in NO table - not
# symbol_names.csv, not Ghidra - so it can never resolve to an RVA, and any base reference
# to the same string reports WRONG. But the name ENCODES THE STRING'S BYTES, so we can
# decode it and check it against the bytes at the address our base actually references.
# That is an exact content check, not a guess: if retail references a literal "FOO" and we
# reference an address that holds "FOO", it is the same datum. (Our src often gives such a
# literal a real NAME - `char g_teleporterCloseKey[] = "GAME_TELEPORTERCLOSE";` - which is
# what makes the two sides' symbols differ in the first place.)
_STR_SPECIAL = ",/\\:. \n\t'-"  # MSVC's ?0..?9 table


def _demangle_str(s):
    """??_C@_0<len>@<hash>@<encoded>@ -> the raw bytes, or None if anything is unrecognised."""
    m = re.match(r"^\?\?_C@_\w+?@\w+?@(.*)@$", s)
    if not m:
        return None
    enc, out, i = m.group(1), bytearray(), 0
    while i < len(enc):
        c = enc[i]
        if c != "?":
            out.append(ord(c))
            i += 1
        elif enc[i : i + 2] == "??":
            out.append(0x3F)
            i += 2
        elif i + 1 < len(enc) and enc[i + 1] == "$":
            if i + 3 >= len(enc):
                return None
            hi, lo = enc[i + 2], enc[i + 3]
            if not ("A" <= hi <= "P" and "A" <= lo <= "P"):
                return None
            out.append((ord(hi) - 65) * 16 + (ord(lo) - 65))
            i += 4
        elif i + 1 < len(enc) and enc[i + 1].isdigit():
            out.append(ord(_STR_SPECIAL[int(enc[i + 1])]))
            i += 2
        else:
            return None  # unknown escape -> refuse to guess
    return bytes(out).rstrip(b"\x00")


def _str_at(rva):
    """The NUL-terminated bytes the retail image holds at `rva` (or None)."""
    if _D is None:
        return None
    try:
        o = _fo(rva)
        if not (0 <= o < len(_D)):
            return None
        e = _D.index(b"\x00", o, min(o + 512, len(_D)))
        return _D[o:e]
    except (ValueError, IndexError):
        return None


def resolve(sym, data, typ, s, add):
    mc = re.match(r"const_([0-9a-fA-F]+)$", s)  # delinker names unlabeled data addrs const_<RVA>
    if mc:
        b = int(mc.group(1), 16)
    else:
        b = sym.get(s) or sym.get(_norm(s)) or sym.get(_key(s))
        if b is None:
            ma = _AUTO.match(s)  # a Ghidra auto-label carries its own VA
            if ma:
                b = int(ma.group(1), 16)
                b -= IMAGE_BASE if b >= IMAGE_BASE else 0
        if b is None:
            mm = re.match(r"\?(\w+)@@", s)  # ?<name>@@... -> a DATA()-pinned global
            if mm:
                b = data.get(mm.group(1))
    if b is None:
        return None
    v = b if typ == "REL32" else (b + add) & 0xFFFFFFFF
    return resolve_thunk(v)  # canonicalise ILT jmp-thunks (see the note at the top)


def _lib_symbols():
    """Every symbol the real toolchain archives (CRT, MFC, Win32, DirectX) can supply. Without this
    the FAKE check false-positives on every statically-linked MFC/CRT call - `CString::GetBuffer`,
    `operator new`, ... - which resolve at link perfectly well. link_defects.py reads the .LIB symbol
    tables (the linker's own answer to "can this resolve"); reuse it rather than re-deriving."""
    try:
        from gruntz.analysis.link_defects import lib_symbols

        return lib_symbols()
    except Exception:
        return set()


LIBS = None


def is_fake(sym, data, s, defined=()):
    """A '?'-mangled symbol that resolves to NEITHER CodeView, NOR a DATA() global, NOR any
    toolchain .LIB is FABRICATED: declared, never defined. We compile but never link, so nothing
    else in the pipeline catches it - it is a guaranteed `unresolved external symbol`."""
    if s.startswith("_") or s.startswith("??_C@") or s.startswith("$SG") or not s.startswith("?"):
        return False  # CRT / import / string / local -> outside CodeView, fine
    if s in defined:
        return False  # the obj defines it itself (a cl-emitted vftable) -> it links
    if resolve(sym, data, "DIR32", s, 0) is not None:
        return False
    global LIBS
    if LIBS is None:
        LIBS = _lib_symbols()
    return not (s in LIBS or s.lstrip("_") in LIBS)  # a real CRT/MFC body is not fabricated


def _addend(insn):
    insn = insn.split("#")[0]  # drop llvm-objdump's "# imm = 0xNN" annotation
    m = re.search(r"\[(0x[0-9a-f]+)\]", insn)  # absolute-memory operand [0xN] -> addend N
    if m:
        return int(m.group(1), 16)
    imms = re.findall(r"(?<![\w\]])(-?0x[0-9a-f]+)", insn)  # last standalone SIGNED imm/disp
    return int(imms[-1], 16) & 0xFFFFFFFF if imms else 0  # -0x4c disp -> 0xffffffb4 addend


IMAGE_END = IMAGE_BASE + 0x400000  # generous upper bound on the mapped image


def defined_syms(obj):
    """Every symbol the object FILE ITSELF defines (COFF symbol table, section > 0).

    A reloc onto one of these resolves at link time no matter what it is called: cl emits a
    class's ??_7 vftable straight into the obj that needs it, so a vftable reference is
    never an unresolved external even when its name is not one we bound to a retail RVA.
    Without this, the FAKE check reports every compiler-emitted vftable as a guaranteed link
    break - the opposite of true. (Read from the COFF directly; llvm-objdump -t does not
    render these symbol tables.)"""
    b = open(obj, "rb").read()
    symptr = struct.unpack_from("<I", b, 8)[0]
    nsym = struct.unpack_from("<I", b, 12)[0]
    strtab = symptr + nsym * 18
    out, i = set(), 0
    while i < nsym:
        base = symptr + i * 18
        if struct.unpack_from("<I", b, base)[0] == 0:
            off = struct.unpack_from("<I", b, base + 4)[0]
            e = b.index(b"\0", strtab + off)
            nm = b[strtab + off:e].decode("latin1")
        else:
            nm = b[base:base + 8].split(b"\0")[0].decode("latin1")
        sec = struct.unpack_from("<h", b, base + 12)[0]
        if sec > 0:
            out.add(nm)
        i += 1 + b[base + 17]
    return out


def parse_obj(obj):
    """llvm-objdump -dr -> ({func: [(type, symbol, addend), ...]}, {func: [rva, ...]}).

    The 2nd dict is the RAW-ABSOLUTE set: addresses an instruction references *in its own bytes*
    with NO relocation attached. The delinked TARGET obj is carved from a LINKED image, and the
    delinker does not emit a reloc for every absolute reference it leaves behind - notably the
    import table: retail's `call dword ptr [0x6c44c8]` (an IAT slot) arrives with the address
    baked in and no reloc. Our base models that slot as a DATA()-bound fn-pointer and DOES emit a
    DIR32, so a naive reloc-vs-reloc diff reports "base references 0x2c44c8 - retail never does".
    Retail plainly DOES reference it; it just is not expressed as a reloc. Feeding these into the
    retail multiset removes that whole false-positive class without weakening the check (an
    address retail truly references is not a defect by definition)."""
    out = subprocess.run(["llvm-objdump", "-dr", obj], capture_output=True, text=True).stdout
    funcs, raw, cur, prev, had = {}, {}, None, "", True
    for ln in out.splitlines():
        m = re.match(r"^[0-9a-f]+ <(.+?)>:", ln)
        if m:
            cur = m.group(1)
            funcs.setdefault(cur, [])
            raw.setdefault(cur, [])
            prev, had = "", True
            continue
        if cur is None:
            continue
        mr = re.search(r"IMAGE_REL_I386_(\w+)\s+(\S+)", ln)
        if mr:
            s = mr.group(2)
            if not s.startswith("__imp__"):
                funcs[cur].append((mr.group(1), s, _addend(prev)))
            had = True
            continue
        mi = re.match(r"^\s*[0-9a-f]+:\s+(?:[0-9a-f]{2}\s+)+(.+)$", ln)
        if mi:
            if not had and prev:
                _raw_refs(prev, raw[cur])
            prev = mi.group(1).replace("\t", " ").strip()
            had = False
    if cur is not None and not had and prev:
        _raw_refs(prev, raw[cur])
    return funcs, raw


def _raw_refs(insn, out):
    """Every address/symbol an UNRELOCATED instruction still references.

    Two shapes, both delinker gaps rather than source bugs:
      (a) absolute operand baked in - `call dword ptr [0x6c44c8]` (an IAT slot). Yield its RVA.
      (b) an intra-object relative call/jmp - the linker already resolved the displacement, so no
          reloc survives; llvm-objdump still annotates the destination symbol. The classic case is
          RECURSION: `calll 0x8e0 <?Load@CRezDirNode@@QAEHH@Z>` inside ?Load itself. cl must emit a
          REL32 for that call (separate COMDAT); the delinked image must not. Yield the symbol name.
    """
    body = insn.split("#")[0]
    for h in re.findall(r"0x([0-9a-f]{6,8})\b", body):
        v = int(h, 16)
        if IMAGE_BASE <= v < IMAGE_END:
            out.append(v - IMAGE_BASE)
    if re.match(r"^(?:call|jmp)", body):
        m = re.search(r"<([^>]+)>", body)
        if m:
            out.append(re.sub(r"\+0x[0-9a-f]+$", "", m.group(1)))  # name, resolved by _tvas
    return out


# Pre-existing discrepancies to triage, keyed (unit, function, base-symbol). Keep EMPTY if possible:
# an entry here is a known-wrong reloc we have chosen not to fix yet, not an exemption on principle.
ALLOWLIST = set()


def _cands(dups, typ, s, add):
    """Every address a target reloc name could denote, thunk-canonicalised. TWO sources of
    one-name-many-addresses: (a) content-hashed ??_C@ string names collide (one name, several
    value-identical addresses); (b) Ghidra labels the ILT jmp-thunk AND the body it jumps to with
    the SAME name. In both cases the reloc may mean ANY of them, so accept all. REL32 takes no
    addend (the displacement is relative to the next insn, not an offset into the symbol)."""
    c = dups.get(s) or dups.get(_norm(s)) or dups.get(_key(s)) or set()
    if typ == "REL32":
        return {resolve_thunk(a) for a in c}
    return {resolve_thunk((a + add) & 0xFFFFFFFF) for a in c}


def _tvas(sym, data, dups, tgt_relocs, tgt_raw=()):
    """Multiset of every RVA retail references. An ambiguous name contributes ALL its candidate
    addrs per occurrence, so a base ref to any one of them clears. Candidates are consulted FIRST:
    a target symbol may be a Ghidra-only label (a thunk) that resolve() - which reads cl's
    symbol_names.csv - cannot see at all. Dropping those silently is what made the retail multiset
    incomplete and manufactured ~1400 phantom WRONGs."""
    tvas, tstrs = Counter(), Counter()
    for r in tgt_relocs:
        cs = _cands(dups, *r)
        if len(cs) > 1:
            for c in cs:
                tvas[c] += 1
            continue
        if cs:
            tvas[next(iter(cs))] += 1
            continue
        v = resolve(sym, data, *r)
        if v is not None:
            tvas[v] += 1
        else:
            b = _demangle_str(r[1])  # an unresolvable delinker-invented string-literal name:
            if b:  # remember its CONTENT so a base ref to that same string clears
                tstrs[b] += 1
    for v in tgt_raw:  # references retail makes with NO reloc attached (IAT slot / intra-object call)
        if isinstance(v, str):
            for c in _cands(dups, "REL32", v, 0) or {resolve(sym, data, "REL32", v, 0)}:
                if c is not None:
                    tvas[c] += 1
        else:
            tvas[resolve_thunk(v)] += 1
    return tvas, tstrs


def check_fn(sym, data, dups, unit, name, base_relocs, tgt_relocs, tgt_raw=(), defined=()):
    probs = []
    for r in base_relocs:  # (1) fabricated function/data symbol -> would be an unresolved external
        if is_fake(sym, data, r[1], defined) and (unit, name, r[1]) not in ALLOWLIST:
            probs.append("FAKE ref '%s' - in neither CodeView nor a DATA() global" % r[1])
    tvas, tstrs = _tvas(sym, data, dups, tgt_relocs, tgt_raw)
    bvas, va_sym = Counter(), {}
    for r in base_relocs:
        v = resolve(sym, data, *r)
        if v is not None:
            bvas[v] += 1
            va_sym.setdefault(v, r[1])
    for v, n in (bvas - tvas).items():  # (2) base references an addr retail never does (or fewer)
        bs = va_sym[v]
        if "_00A@" in bs or (unit, name, bs) in ALLOWLIST:
            continue
        # Retail may name that datum as a mangled STRING LITERAL the delinker invented, which
        # resolves to no address at all. Cancel by CONTENT: if the bytes at the address we
        # reference are exactly a string retail references, it is the same datum.
        s = _str_at(v)
        if s and tstrs.get(s, 0) >= n:
            tstrs[s] -= n
            continue
        probs.append(
            "WRONG: base references 0x%x (%s) x%d - retail never does (or fewer)" % (v, bs, n)
        )
    return probs


def _objs(unit):
    return (
        os.path.join(REPO, "build/objdiff/base/%s.obj" % unit),
        os.path.join(REPO, "build/objdiff/target/%s.c.obj" % unit),
    )


def review(rva):
    """Single-function review: find the unit+name owning `rva`, then diff its two reloc multisets."""
    sym, data, dups = load_symbols()
    want = int(rva, 16) if isinstance(rva, str) else rva
    report = json.load(open(os.path.join(REPO, "build/objdiff/report.json")))
    for u in report["units"]:
        base_obj, tgt_obj = _objs(u["name"])
        if not (os.path.exists(base_obj) and os.path.exists(tgt_obj)):
            continue
        for f in u.get("functions", []):
            if f.get("address") not in (want, want + IMAGE_BASE):
                continue
            nm = f["name"]
            (bf, _br), (tf, tr) = parse_obj(base_obj), parse_obj(tgt_obj)
            if nm not in bf or nm not in tf:
                print("  %s: no COMDAT on one side" % nm)
                return 0
            print("%s  %s  (fuzzy %.2f%%)" % (u["name"], nm, f.get("fuzzy_match_percent", 0)))
            for p in check_fn(sym, data, dups, u["name"], nm, bf[nm], tf[nm],
                              tr.get(nm, []), defined_syms(base_obj)):
                print("  !! " + p)
            print("  base relocs=%d  target relocs=%d" % (len(bf[nm]), len(tf[nm])))
            return 0
    print("no near-exact function found at %s" % rva)
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rva", nargs="?", help="review ONE function at this RVA")
    ap.add_argument("--unit", help="audit only this unit")
    a = ap.parse_args()
    if a.rva:
        return review(a.rva)

    sym, data, dups = load_symbols()
    report = json.load(open(os.path.join(REPO, "build/objdiff/report.json")))
    bad, seen = [], 0
    for u in report["units"]:
        unit = u["name"]
        if a.unit and unit != a.unit:
            continue
        near = {
            f["name"]
            for f in u.get("functions", [])
            if f.get("fuzzy_match_percent", 0) >= THRESHOLD
        }
        if not near:
            continue
        base_obj, tgt_obj = _objs(unit)
        if not (os.path.exists(base_obj) and os.path.exists(tgt_obj)):
            continue
        (bf, _br), (tf, tr) = parse_obj(base_obj), parse_obj(tgt_obj)
        bdef = defined_syms(base_obj)
        for name in sorted(near):
            if name not in bf or name not in tf:
                continue
            seen += 1
            for p in check_fn(sym, data, dups, unit, name, bf[name], tf[name],
                              tr.get(name, []), bdef):
                bad.append((unit, name, p))
    for unit, name, p in bad:
        print("  %-22s %-46s %s" % (unit, name[:46], p))
    fake = sum(1 for _, _, p in bad if p.startswith("FAKE"))
    print(
        "\nassert_relocs: %d near-exact fn(s) audited (>=%.1f%%), %d defect(s) "
        "[%d FAKE, %d WRONG]" % (seen, THRESHOLD, len(bad), fake, len(bad) - fake)
    )
    if bad:
        print("objdiff MASKS relocs, so none of these cost %. They WILL break the link.")
        return 1
    print("relocs OK: every near-exact function's reloc targets resolve to the retail address.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
