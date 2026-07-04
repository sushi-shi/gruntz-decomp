#!/usr/bin/env python3
"""consolidate_globals.py - de-proliferate matched-global DATA() bindings.

A matched GLOBAL is bound to its retail address by `DATA(0xADDR)` (include/rva.h)
sitting above an `extern T g_name;`. Over the campaign matchers RE-DECLARED the
same global in many TUs, so a single RVA's DATA() can appear in dozens of files
(measured: 118 RVAs in >1 TU). labels.py keeps only the keep-last winner and WARNs
on the rest; the address binding is silently duplicated.

This tool makes each global's DATA() binding live in ONE place:

  * src/Globals.cpp - the single DATA() binding point: `DATA(0xADDR) extern T
    g_name;` exactly once per consolidated global, in the `globals` unit
    (config/units.toml). labels.py treats `globals` as TRUSTED (the name was
    authority-checked in the matched TU it came from before being moved here).
  * include/Globals.h - a plain `extern T g_name;` per global (the canonical
    reference point for NEW code to include instead of re-declaring).

Each using TU keeps its `extern T g_name;` declaration IN PLACE and loses only the
`DATA(0xADDR)` macro line above it.

WHY NOT replace the TUs' externs with `#include <Globals.h>`? MEASURED: under
MSVC 5.0 adding ~270 extern declarations to a TU (via one shared header) PERTURBS
the optimizer's register allocation for unrelated functions (a DATA() macro emits
no code, but the extra declarations still shift codegen) - causing objdiff
regressions. So the externs stay where the matcher wrote them; only the no-op
`DATA()` macro (which expands to NOTHING under MSVC) is removed - byte-neutral for
every base obj. Globals.h exists as the documented single reference but is not
forced into the already-matched TUs.

WHAT IS SAFE TO CONSOLIDATE (everything else is left in place + reported):
  * the RVA carries exactly ONE authority-confirmed name in the current build
    (build/gen/labels/*.csv) and it is THIS global's - so the keep-last winner
    cannot flip and the trusted bypass cannot resurrect a dropped name.
  * all the RVA's DATA() sites carry a byte-identical extern; the name is not
    reused for another RVA; the decl is file-scope (a function-local extern's type
    may be local); the type is reproducible in Globals.cpp (builtin / int-alias /
    void(*) / function-pointer / pointer-to-project-type forward-declared with the
    source tag). A Win32/MFC-typed or value-of-project-class global is SKIPPED.
  * a DEFINITION (`= ...` / `{ ... }`) is never moved (it emits data).

MATCHING-NEUTRAL: only the no-op DATA() macro is removed from a using TU (its base
obj is byte-identical), and `apply_named_data` in synth_pdb keys data symbols by
RVA->name only (the unit column is provenance, never reaching the delinker), so
the rva->name map - hence delink+objdiff - is unchanged. Verify with:
  build/gen/symbol_names.csv (rva,name,kind projection) identical before/after,
  and `gruntz build` green with no config/match_baseline.tsv regression.

IDEMPOTENT: re-running on an already-migrated tree is a no-op (Globals.cpp is
re-scanned as the canonical home of the moved bindings). Run as
  python3 -m gruntz.analysis.consolidate_globals [--apply] [--only-dups]
(default is a dry-run report). REQUIRES a prior `gruntz build` (reads the label
fragments for the rva->name preservation gate).
"""

import argparse
import os
import re
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO = next((p for p in SCRIPT_DIR.parents if (p / "flake.nix").exists()), SCRIPT_DIR)

GLOBALS_H = REPO / "include" / "Globals.h"
GLOBALS_CPP = REPO / "src" / "Globals.cpp"
GLOBALS_CPP_REL = "src/Globals.cpp"
UNITS_TOML = REPO / "config" / "units.toml"
GLOBALS_UNIT = "globals"

DATA_RE = re.compile(r"\bDATA\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")

# Int aliases (include/Ints.h) + C/C++ builtins: need no type context.
BUILTINS = {
    "void", "char", "short", "int", "long", "unsigned", "signed", "float",
    "double", "bool", "wchar_t", "const", "volatile", "__int64", "__int32",
    "__int16", "__int8",
    "i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64",
}
# Calling-convention keywords that appear inside function-pointer types.
CC_KW = {"__stdcall", "__cdecl", "__thiscall", "__fastcall"}

# Win32/MFC types -> the wrapper header that defines them. Only an explicit
# allowlist (a project type also starts with C, so we cannot map C* blindly).
WIN32_TYPES = {
    "WORD", "DWORD", "BYTE", "UINT", "INT", "LONG", "ULONG", "SHORT", "USHORT",
    "BOOL", "BOOLEAN", "CHAR", "WCHAR", "TCHAR", "FLOAT", "DWORD_PTR",
    "HINSTANCE", "HMODULE", "HWND", "HDC", "HBITMAP", "HMENU", "HANDLE",
    "HCURSOR", "HGDIOBJ", "HICON", "HBRUSH", "HPEN", "HPALETTE", "HFONT",
    "HKEY", "HRESULT", "ATOM", "COLORREF", "LPARAM", "WPARAM", "LRESULT",
    "LPVOID", "LPSTR", "LPCSTR", "LPCTSTR", "LPTSTR", "LPDWORD", "LPRECT",
    "RECT", "POINT", "SIZE", "PALETTEENTRY", "RGBQUAD", "LOGPALETTE",
    "CRITICAL_SECTION", "GUID", "IID", "CLSID", "LARGE_INTEGER", "FILETIME",
    "SYSTEMTIME", "MSG", "WNDCLASS", "PAINTSTRUCT", "OVERLAPPED",
    "SECURITY_ATTRIBUTES", "OSVERSIONINFOA", "VOID",
}
MFC_TYPES = {
    "CString", "CObject", "CObList", "CPtrList", "CPtrArray", "CObArray",
    "CRuntimeClass", "CWnd", "CDialog", "CArchive", "CDC", "CRect", "CPoint",
    "CSize", "CFile", "CException", "CMapStringToOb", "CStringArray",
    "CByteArray", "CWordArray", "CDWordArray", "CTime", "CTimeSpan",
}

WIN32_HDR = "Win32.h"
MFC_HDR = "Mfc.h"


def log(msg):
    print(f"[consolidate] {msg}", file=sys.stderr)


def units_source_map():
    """source-path (repo-relative, forward slash) -> unit stem, from units.toml."""
    import tomllib
    with open(UNITS_TOML, "rb") as f:
        data = tomllib.load(f)
    return {u["source"]: u["unit"] for u in data.get("unit", [])}


class Block:
    """One DATA(rva) + its extern declaration, with the exact source span."""

    __slots__ = ("file", "rva", "start", "end", "decl", "var", "type_str",
                 "is_def", "extern_c", "comment", "depth")

    def __init__(self, file, rva, start, end, decl, var, type_str, is_def,
                 extern_c, comment):
        self.file = file          # Path
        self.rva = rva            # "0x........" lowercased, zero-padded as written
        self.start = start        # char offset of the DATA( in the file text
        self.end = end            # char offset just past the closing ';' (+ comment)
        self.decl = decl          # normalised `extern ... ;` text (one line)
        self.var = var            # variable name
        self.type_str = type_str  # type portion (between extern[ "C"] and var)
        self.is_def = is_def      # has an initialiser -> a definition, never moved
        self.extern_c = extern_c
        self.comment = comment    # trailing `// ...` on the decl's last line, or ""


def _blank_noncode(text):
    """Copy of `text` with comment + string/char-literal CONTENT replaced by spaces
    (newlines preserved, so char offsets are unchanged). Lets a regex find macros /
    keywords only in real code - e.g. a `DATA(0x..)` written inside a comment must
    not be mistaken for a real binding."""
    out = list(text)
    n, i, st = len(text), 0, "code"
    while i < n:
        c = text[i]
        if st == "code":
            if c == "/" and i + 1 < n and text[i + 1] == "/":
                while i < n and text[i] != "\n":
                    out[i] = " "; i += 1
                continue
            if c == "/" and i + 1 < n and text[i + 1] == "*":
                while i < n and not (text[i] == "*" and i + 1 < n and text[i + 1] == "/"):
                    if text[i] != "\n":
                        out[i] = " "
                    i += 1
                continue
            if c == '"':
                st = '"'
            elif c == "'":
                st = "'"
        else:
            if c == "\\":
                out[i] = " "
                if i + 1 < n and text[i + 1] != "\n":
                    out[i + 1] = " "
                i += 2
                continue
            if c == st:
                st = "code"
            elif c != "\n":
                out[i] = " "
        i += 1
    return "".join(out)


def _scan_blocks_in_text(path, text):
    """Yield Block for every DATA(rva)+extern in one file's text."""
    out = []
    spans = _linkage_spans(text)
    for m in DATA_RE.finditer(_blank_noncode(text)):
        rva = m.group(1).lower()
        in_linkage = any(o < m.start() < c for o, c in spans)
        # The declaration starts right after DATA(...) - possibly same line
        # (one-line form) or on a following line. Skip whitespace + line-comments.
        i = m.end()
        n = len(text)
        # gather declaration text up to the first ';' at brace/paren depth 0.
        # (a funcptr extern has balanced parens and exactly one terminating ';';
        # a definition's '{...}=...' is detected and skipped, not moved.)
        j = i
        depth = 0
        seen_nonspace = False
        # We must not run past into the next statement; find the ';'.
        decl_end = None
        while j < n:
            c = text[j]
            if c == "/" and j + 1 < n and text[j + 1] == "/":
                # skip a line comment within the (rare) multi-line decl
                k = text.find("\n", j)
                j = n if k < 0 else k
                continue
            if c == "/" and j + 1 < n and text[j + 1] == "*":
                k = text.find("*/", j)
                j = n if k < 0 else k + 2
                continue
            if c in "([{":
                depth += 1
            elif c in ")]}":
                depth -= 1
            elif c == ";" and depth == 0:
                decl_end = j
                break
            if not c.isspace():
                seen_nonspace = True
            j += 1
        if decl_end is None:
            continue
        raw = text[i:decl_end]
        # trailing same-line comment after the ';'
        line_end = text.find("\n", decl_end)
        if line_end < 0:
            line_end = n
        tail = text[decl_end + 1:line_end]
        comment = ""
        mt = re.match(r"\s*(//.*)$", tail)
        if mt:
            comment = mt.group(1).strip()
            end = line_end
        else:
            end = decl_end + 1
        out.append(_parse_block(path, rva, m.start(), end, raw, comment,
                                in_linkage))
    return out


def _parse_block(path, rva, start, end, raw, comment, in_linkage=False):
    # strip block/line comments inside raw, collapse whitespace
    raw_nc = re.sub(r"/\*.*?\*/", " ", raw, flags=re.S)
    raw_nc = re.sub(r"//[^\n]*", " ", raw_nc)
    decl = re.sub(r"\s+", " ", raw_nc).strip()
    is_def = ("=" in decl) or ("{" in raw)
    # C linkage comes either inline (`extern "C" T g;`) or from an enclosing
    # `extern "C" { ... }` block (in_linkage): both give the C-decorated mangled
    # name, so a member of a linkage block must be re-wrapped when rendered alone.
    extern_c = bool(re.match(r'extern\s+"C"\b', decl)) or in_linkage
    body = re.sub(r'^extern\s+("C"\s+)?', "", decl).strip().rstrip(";").strip()
    # var name = last identifier before a trailing [..] / ( for funcptr / end.
    # funcptr: `T(__stdcall* name)(args)` -> name is inside the first (...).
    var = None
    mfp = re.search(r"\(\s*[A-Za-z_:<>* ]*?\*\s*([A-Za-z_]\w*)\s*\)", body)
    if mfp and "(" in body and body.index("(") < (body.index(var) if var else len(body)):
        var = mfp.group(1)
    if var is None:
        mm = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*$", body)
        var = mm.group(1) if mm else "?"
    # type portion: body with the var (and any array/func suffix) removed
    type_str = body
    b = Block(path, rva, start, end, decl, var, type_str, is_def, extern_c,
              comment)
    b.depth = 0
    return b


def _linkage_spans(text):
    """[(open_pos, close_pos)] for every `extern "C" { ... }` (or `extern "C++"`)
    LINKAGE block. A linkage-specification block does NOT introduce a new scope:
    the externs inside it are file scope (just C-decorated). The plain brace-depth
    heuristic below would otherwise count the `{` and flag such a global as
    block-scope (not file scope), needlessly skipping it; and its members' decl
    text lacks the `extern "C"` (that sits on the block), so a member consolidated
    into Globals.cpp must be re-wrapped in `extern "C"` to keep its C-decorated
    mangled name. Braces inside comments / string-char literals are blanked first,
    so only real code braces are matched."""
    nc = _blank_noncode(text)
    spans = []
    for m in re.finditer(r'\bextern\s*"[^"]*"\s*\{', nc):
        bpos = m.end() - 1                      # the opening '{'
        d, i, n = 0, bpos, len(nc)
        while i < n:
            ch = nc[i]
            if ch == "{":
                d += 1
            elif ch == "}":
                d -= 1
                if d == 0:
                    spans.append((bpos, i))     # ('{', matching '}')
                    break
            i += 1
    return spans


def _linkage_brace_positions(text):
    """The `{` / `}` char offsets of every linkage block (see _linkage_spans)."""
    skip = set()
    for o, c in _linkage_spans(text):
        skip.add(o)
        skip.add(c)
    return skip


def _brace_depth_at(text, positions):
    """{pos: net-brace-depth at pos}, ignoring braces in comments / string-char
    literals AND the braces of `extern "C" {}` linkage blocks (those are file
    scope) - so a DATA() inside a function body is detected (depth > 0) but one in
    a file-scope linkage block is not."""
    want = sorted(set(positions))
    skip = _linkage_brace_positions(text)
    out, wi, n = {}, 0, len(text)
    d, i, st = 0, 0, "code"
    while i < n:
        while wi < len(want) and want[wi] <= i:
            out[want[wi]] = d
            wi += 1
        c = text[i]
        if st == "code":
            if c == "/" and i + 1 < n and text[i + 1] == "/":
                i = text.find("\n", i)
                i = n if i < 0 else i
                continue
            if c == "/" and i + 1 < n and text[i + 1] == "*":
                k = text.find("*/", i)
                i = n if k < 0 else k + 2
                continue
            if c == '"':
                st = "str"
            elif c == "'":
                st = "chr"
            elif c == "{":
                if i not in skip:
                    d += 1
            elif c == "}":
                if i not in skip:
                    d -= 1
        elif st == "str":
            if c == "\\":
                i += 2
                continue
            if c == '"':
                st = "code"
        elif st == "chr":
            if c == "\\":
                i += 2
                continue
            if c == "'":
                st = "code"
        i += 1
    while wi < len(want):
        out[want[wi]] = d
        wi += 1
    return out


def scan_tree():
    """All DATA blocks across src/ + include/ (excluding the generated files)."""
    files = sorted(set(
        list((REPO / "src").rglob("*.cpp")) + list((REPO / "src").rglob("*.h"))
        + list((REPO / "include").rglob("*.h"))))
    blocks = []
    # Globals.cpp IS scanned: after the first pass each consolidated global lives
    # there (as its canonical, self-contained DATA block), so re-scanning it makes
    # the tool idempotent (it re-discovers and re-emits them). Globals.h carries no
    # DATA() and is skipped.
    for f in files:
        if f.resolve() == GLOBALS_H.resolve():
            continue
        text = f.read_text()
        if "DATA(" not in text:
            continue
        fb = _scan_blocks_in_text(f, text)
        depths = _brace_depth_at(text, [b.start for b in fb])
        for b in fb:
            b.depth = depths.get(b.start, 0)
        blocks.extend(fb)
    return blocks


def scan_all_externs():
    """var-name -> set(normalised `extern ... ;` decl) for EVERY extern variable
    declaration in the tree (DATA-bound or not, file- or block-scope).

    Used to detect a name reused with a DIFFERENT type somewhere (e.g. one TU's
    block-scope `extern i32 g_x;` vs another's `extern u32 g_x;`): consolidating
    such a name into a file-scope header would be a C2371 redefinition.
    """
    files = sorted(set(
        list((REPO / "src").rglob("*.cpp")) + list((REPO / "src").rglob("*.h"))
        + list((REPO / "include").rglob("*.h"))))
    out, block_scope = {}, set()
    for f in files:
        if f.resolve() in (GLOBALS_H.resolve(), GLOBALS_CPP.resolve()):
            continue
        text = f.read_text()
        matches = list(re.finditer(r"\bextern\b", _blank_noncode(text)))
        depths = _brace_depth_at(text, [m.start() for m in matches])
        for m in matches:
            j, n, depth = m.end(), len(text), 0
            end = None
            while j < n:
                c = text[j]
                if c == "/" and j + 1 < n and text[j + 1] == "/":
                    k = text.find("\n", j); j = n if k < 0 else k; continue
                if c == "/" and j + 1 < n and text[j + 1] == "*":
                    k = text.find("*/", j); j = n if k < 0 else k + 2; continue
                if c in "([{":
                    depth += 1
                elif c in ")]}":
                    depth -= 1
                elif c == ";" and depth == 0:
                    end = j; break
                elif c in "{}" and depth <= 0:
                    break          # an `extern "C" { ... }` linkage block, not a var
                j += 1
            if end is None:
                continue
            raw = text[m.start():end]
            if "{" in raw or "=" in raw:
                continue
            b = _parse_block(f, "0x0", m.start(), end, raw, "")
            out.setdefault(b.var, set()).add(normalise(b.decl))
            # a block-scope extern (inside a function) may carry a different
            # language linkage (inside an `extern "C" {}`) than a file-scope decl,
            # so its name cannot be safely hoisted to a file-scope header.
            if depths.get(m.start(), 0) > 0:
                block_scope.add(b.var)
    return out, block_scope


def load_confirmed_data():
    """int(rva) -> set(mangled data names) from the LAST build's label fragments
    (build/gen/labels/*.csv, kind=data). These are the names that PASSED the
    per-TU authority check (or are trusted in the `globals` unit). None if no
    build has run.

    Used to keep consolidation rva->name-PRESERVING: a global is moved only when
    its address carries exactly ONE confirmed name (so the keep-last winner cannot
    flip) and that name is this global's (not, e.g., an auto-emitted ??_7 vtable
    that shares the address with a stale placeholder extern). This also stops the
    trusted-globals bypass from RESURRECTING a name the authority check had been
    dropping."""
    d = REPO / "build" / "gen" / "labels"
    if not d.exists():
        return None
    import csv
    # Only read fragments for units CURRENTLY in units.toml: ninja leaves a stale
    # <unit>.csv on disk when a unit is removed (e.g. a previous build's trusted
    # `globals.csv`), and globbing those would re-confirm dropped names.
    current = set(units_source_map().values())
    out = {}
    for unit in current:
        f = d / f"{unit}.csv"
        if not f.exists():
            continue
        with open(f, newline="") as fh:
            for r in csv.DictReader(fh):
                if (r.get("kind") or "") == "data" and r.get("rva"):
                    out.setdefault(int(r["rva"], 16), set()).add(r["name"].strip())
    return out


def mangled_ident(name):
    """g_foo identifier from a data symbol: `?g_foo@@3..` / `_g_foo` -> g_foo;
    a `??_7Class@@6B@` vtable / other compound symbol -> None."""
    if name.startswith("??"):
        return None
    if name.startswith("?"):
        return name[1:].split("@@", 1)[0]
    if name.startswith("_"):
        return name[1:]
    return name


def find_tag(name, files):
    """struct/class tag for a project type `name`, parsed from the using TUs.

    Returns 'struct' or 'class' if a forward-decl/definition is found and the tag
    is unambiguous across the given files; None if unknown or conflicting.
    """
    pat = re.compile(r"\b(struct|class)\s+" + re.escape(name) + r"\b")
    tags = set()
    for f in files:
        for mm in pat.finditer(f.read_text()):
            tags.add(mm.group(1))
    if len(tags) == 1:
        return next(iter(tags))
    return None


_ELAB_KW = {"struct", "class", "union", "enum"}


def classify(block, sites):
    """Decide how to reproduce block's type in the header.

    Returns (ok, includes:set, forward_decls:set, reason).
    forward_decls is a set of (tag, name). ok=False -> SKIP with `reason`.
    """
    if block.is_def:
        return False, set(), set(), "is a definition (emits data)"
    # type portion: drop array subscripts ([0x800]) and the variable name so an
    # array-size hex literal is not mis-read as a type identifier.
    body = re.sub(r"\[[^\]]*\]", " ", block.type_str)
    body = re.sub(r"\b" + re.escape(block.var) + r"\b", " ", body)
    # names introduced by an elaborated-type-specifier (`struct Foo*`) are
    # self-declared by the declaration itself -> need no header context.
    elaborated = {m.group(2) for m in
                  re.finditer(r"\b(struct|class|union|enum)\s+([A-Za-z_]\w*)", body)}
    ids = {x for x in re.findall(r"[A-Za-z_]\w*", body)
           if x not in BUILTINS and x not in CC_KW and x not in _ELAB_KW}
    includes, fwd = set(), set()
    for t in sorted(ids):
        if t in elaborated:
            continue                            # self-declared by `struct T`
        if t in WIN32_TYPES or t in MFC_TYPES:
            # A Win32/MFC type would force <Win32.h>/<Mfc.h> into Globals.h, which
            # every using TU then pulls in - and MFC TUs forbid <windows.h>. Keep
            # Globals.h include-light (rva.h only): leave these in their TUs.
            return False, set(), set(), f"needs Win32/MFC header for {t}"
        else:
            # a project type. Safe only as a POINTER (forward-decl); a value of a
            # project class type needs its full (match-load-bearing) definition.
            ptr_use = re.search(r"\b" + re.escape(t) + r"\b[^,;()]*\*", body)
            if not ptr_use:
                return False, set(), set(), f"value of project type {t}"
            tag = find_tag(t, [s.file for s in sites])
            if tag is None:
                return False, set(), set(), f"unknown/ambiguous tag for {t}"
            fwd.add((tag, t))
    return True, includes, fwd, ""


def normalise(decl):
    return re.sub(r"\s+", " ", decl.strip()).rstrip(";").strip()


def from_linkage_block(block):
    """True if `block` is a member of an `extern "C" { ... }` linkage block: its
    extern_c flag is set (C linkage) yet its OWN decl text lacks an inline
    `extern "C"` (that sat on the enclosing block). Such a global must be rendered
    back inside an `extern "C" { }` wrapper - re-wrapping it as a standalone
    `extern "C" T g;` would make its decl text DIVERGE from the consuming TU's
    linkage-block member (`extern T g;`), so name_decls would see two spellings,
    flag the name polluted, and DROP it on the next run (non-idempotent + lost
    binding). Wrapping preserves the raw decl text AND the C-decorated mangling."""
    return block.extern_c and not re.match(r'extern\s+"C"\b', normalise(block.decl))


def plan(blocks, only_dups=False):
    """Group blocks by rva and decide consolidatable vs skipped.

    Returns (consolidate:list[(rva, canonical_block, sites, includes, fwd)],
             skipped:list[(rva, reason, sites)]).
    """
    by_rva = {}
    for b in blocks:
        by_rva.setdefault(b.rva, []).append(b)
    # NAME-UNIQUENESS: the same C identifier is reused for DIFFERENT globals (e.g.
    # `g_gameReg` names CGameReg* at one rva and WwdGameReg* at another). A shared
    # header can declare a name only once, so a name bound to >1 rva (or >1 type)
    # cannot be consolidated - doing so would collide with every TU that declares
    # the name for the other address. Such names are skipped wholesale.
    name_rvas, name_decls = {}, {}
    for b in blocks:
        name_rvas.setdefault(b.var, set()).add(b.rva)
        name_decls.setdefault(b.var, set()).add(normalise(b.decl))
    # fold in EVERY extern decl tree-wide (incl. non-DATA / block-scope), so a name
    # declared with a different type (or a different language linkage in a
    # block-scope `extern "C"`) anywhere is treated as polluted.
    all_externs, block_scope_names = scan_all_externs()
    for name, decls in all_externs.items():
        name_decls.setdefault(name, set()).update(decls)
    polluted = {n for n in name_rvas
                if len(name_rvas[n]) > 1 or len(name_decls[n]) > 1}
    polluted |= (set(name_rvas) & block_scope_names)
    confirmed = load_confirmed_data()
    consolidate, skipped = [], []
    # type-name -> tag, to catch a forward-decl tag conflict across DIFFERENT rvas
    global_tags = {}
    for rva, sites in sorted(by_rva.items()):
        # Vtable placeholder externs (g_*Vtbl / *vtable*) are NOT real globals -
        # they are transitional decomp artifacts that vtable MODELLING removes
        # (virtual-attaching each slot's fn to the class so cl emits the real
        # ??_7). Keep them in their TUs, out of Globals.cpp.
        if re.search(r"[Vv]tbl|[Vv]table", sites[0].var):
            skipped.append((rva, "vtable placeholder (handled by vtable modelling)",
                            sites))
            continue
        norms = {normalise(s.decl) for s in sites}
        if len(norms) != 1:
            skipped.append((rva, "divergent extern across sites", sites))
            continue
        # rva->name PRESERVATION gate (needs a prior build's label fragments):
        # consolidate only when this address has exactly ONE authority-confirmed
        # name and it is THIS global's. Skips names the authority check drops
        # (would be resurrected by the trusted bypass) and vtable/placeholder
        # address clashes (would flip the keep-last winner).
        if confirmed is None:
            skipped.append((rva, "no build fragments (run gruntz build first)",
                            sites))
            continue
        names_here = confirmed.get(int(rva, 16), set())
        if len(names_here) != 1:
            why = ("not authority-confirmed in build" if not names_here
                   else f"address has competing names {sorted(names_here)}")
            skipped.append((rva, why, sites))
            continue
        if mangled_ident(next(iter(names_here))) != sites[0].var:
            skipped.append((rva, f"confirmed name {next(iter(names_here))} "
                            f"!= {sites[0].var}", sites))
            continue
        if {s.var for s in sites} & polluted:
            skipped.append((rva, f"name reused for another global ({sites[0].var})",
                            sites))
            continue
        if any(s.depth > 0 for s in sites):
            # a block-scope `extern` (inside a function); its type may be a
            # function-local struct, so it cannot be hoisted to a file-scope header.
            skipped.append((rva, "block-scope extern (not file scope)", sites))
            continue
        if only_dups and len(sites) == 1:
            continue
        canon = sites[0]
        ok, inc, fwd, reason = classify(canon, sites)
        if not ok:
            skipped.append((rva, reason, sites))
            continue
        consolidate.append([rva, canon, sites, inc, fwd])
    # resolve cross-rva forward-decl tag conflicts -> skip offenders
    for tag, name in (fd for c in consolidate for fd in c[4]):
        global_tags.setdefault(name, set()).add(tag)
    conflicted = {n for n, ts in global_tags.items() if len(ts) > 1}
    if conflicted:
        keep = []
        for c in consolidate:
            bad = {n for _, n in c[4]} & conflicted
            if bad:
                skipped.append((c[0], f"forward-decl tag conflict {sorted(bad)}",
                                c[2]))
            else:
                keep.append(c)
        consolidate = keep
    return consolidate, skipped


HEADER_BANNER = """// Globals.h - the canonical reference declaration of consolidated engine globals.
//
// GENERATED by `python3 -m gruntz.analysis.consolidate_globals --apply`; do not
// hand-edit (re-run the tool). Each consolidated global is declared once here as a
// plain `extern T g_name;`; its DATA(0xADDR) address binding lives once in
// src/Globals.cpp (the `globals` unit). NEW code should `#include <Globals.h>`
// instead of re-declaring a global.
//
// NOTE: already-matched TUs deliberately KEEP their in-place externs and are NOT
// switched to this header - under MSVC 5.0 pulling ~270 extra extern declarations
// into a TU perturbs the optimizer's register allocation (objdiff regressions),
// even though a DATA() macro itself emits no code. So this header is the reference,
// not a forced include. Emits no code -> matching-neutral.
"""

CPP_BANNER = """// Globals.cpp - the single DATA() binding point for matched engine globals.
//
// GENERATED by `python3 -m gruntz.analysis.consolidate_globals --apply`; do not
// hand-edit (re-run the tool). One `DATA(0xADDR)\\nextern T g_name;` per global,
// authority-trusted by labels.py for the `globals` unit. Unused externs emit no
// symbols, so the base obj is empty -> matching-neutral.
"""


def _preamble(consolidate):
    """Shared header context (forward-decls) for both generated files.

    Globals.h is deliberately include-light: only <rva.h> (int aliases) plus
    forward-decls for pointer-to-engine-object globals. Win32/MFC-typed globals
    are skipped (classify), so no heavyweight wrapper header leaks into every TU.
    """
    fwd = set()
    for rva, canon, sites, inc, fd in consolidate:
        fwd |= fd
    lines = ["#include <rva.h> // int aliases (i8..u64)"]
    if fwd:
        lines.append("")
        lines.append("// Forward declarations for pointer-to-engine-object globals.")
        for tag, name in sorted(fwd, key=lambda x: x[1]):
            lines.append(f"{tag} {name};")
    return lines


def _split_linkage(consolidate):
    """(plain, clink) - the consolidated globals partitioned into file-scope decls
    and members of an `extern "C" {}` linkage block, each sorted by rva."""
    key = lambda c: int(c[0], 16)
    plain = sorted((c for c in consolidate if not from_linkage_block(c[1])), key=key)
    clink = sorted((c for c in consolidate if from_linkage_block(c[1])), key=key)
    return plain, clink


def render_globals_h(consolidate):
    lines = [HEADER_BANNER, "#ifndef GRUNTZ_GLOBALS_H", "#define GRUNTZ_GLOBALS_H", ""]
    lines += _preamble(consolidate)
    lines.append("")
    plain, clink = _split_linkage(consolidate)
    for rva, canon, sites, inc, fd in plain:
        lines.append(normalise(canon.decl) + ";")
    if clink:
        # C-linkage globals declared inside an `extern "C" {}` block in their TU:
        # keep them wrapped so the C-decorated mangled name is reproduced and the
        # member decl text matches the TU (idempotency; see from_linkage_block).
        lines += ["", 'extern "C" {']
        for rva, canon, sites, inc, fd in clink:
            lines.append(normalise(canon.decl) + ";")
        lines.append("}")
    lines += ["", "#endif // GRUNTZ_GLOBALS_H", ""]
    return "\n".join(lines)


def render_globals_cpp(consolidate):
    # SELF-CONTAINED (does not include Globals.h): carries its own forward-decls so
    # the tool re-derives every pointer type's tag from Globals.cpp on a re-run
    # (idempotency), and so the DATA externs' clang VarDecls sit unambiguously in
    # this main file (no header redeclaration to mis-join).
    lines = [CPP_BANNER]
    lines += _preamble(consolidate)
    lines.append("")
    plain, clink = _split_linkage(consolidate)
    for rva, canon, sites, inc, fd in plain:
        cmt = f"  {canon.comment}" if canon.comment else ""
        lines.append(f"DATA({rva}) {normalise(canon.decl)};{cmt}")
    if clink:
        lines += ["", 'extern "C" {']
        for rva, canon, sites, inc, fd in clink:
            cmt = f"  {canon.comment}" if canon.comment else ""
            lines.append(f"DATA({rva}) {normalise(canon.decl)};{cmt}")
        lines.append("}")
    lines.append("")
    return "\n".join(lines)


def remove_data_macro(text, block):
    """Delete ONLY the `DATA(0xADDR)` macro of a block, keeping its `extern ...;`
    declaration exactly where it is (the extern is matching-load-bearing in its TU;
    the DATA() macro expands to nothing under MSVC, so removing it is byte-neutral).

    Two-line form (DATA alone on its line) -> drop the whole DATA line. One-line form
    (`DATA(0x..) extern T g;`) -> drop the macro + the following blank, keep `extern`.
    """
    m = DATA_RE.match(text, block.start)
    if not m:                                    # be defensive; leave untouched
        return text
    after = m.end()
    k = after
    while k < len(text) and text[k] in " \t":
        k += 1
    ls = text.rfind("\n", 0, block.start) + 1
    pre = text[ls:block.start]                   # indentation before DATA(
    if k < len(text) and text[k] == "\n":
        # DATA() alone on its line -> remove the entire line including its newline
        return text[:ls] + text[k + 1:]
    # one-line form -> keep the indentation, drop `DATA(..) ` up to the extern
    return text[:ls] + pre + text[k:]


def apply_changes(consolidate, skipped, do_apply):
    # Drop ONLY the DATA() macro from each using TU site (the extern stays in place);
    # Globals.cpp is regenerated wholesale (never edited in place).
    drop = {}   # file -> [blocks]
    for rva, canon, sites, inc, fd in consolidate:
        for b in sites:
            if b.file.resolve() == GLOBALS_CPP.resolve():
                continue
            drop.setdefault(b.file, []).append(b)
    file_edits = {}
    for f, blocks in drop.items():
        text = f.read_text()
        for b in sorted(blocks, key=lambda b: b.start, reverse=True):   # bottom-up
            text = remove_data_macro(text, b)
        file_edits[f] = text
    h = render_globals_h(consolidate)
    cpp = render_globals_cpp(consolidate)
    if not do_apply:
        return h, cpp, file_edits
    for f, text in file_edits.items():
        f.write_text(text)
    GLOBALS_H.write_text(h)
    GLOBALS_CPP.write_text(cpp)
    ensure_units_entry()
    return h, cpp, file_edits


def ensure_units_entry():
    text = UNITS_TOML.read_text()
    if re.search(r'unit\s*=\s*"%s"' % re.escape(GLOBALS_UNIT), text):
        return False
    entry = (f'\n[[unit]]\nunit = "{GLOBALS_UNIT}"\n'
             f'source = "{GLOBALS_CPP_REL}"\nflags = "base"\n')
    if not text.endswith("\n"):
        text += "\n"
    UNITS_TOML.write_text(text + entry)
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--apply", action="store_true",
                    help="write the changes (default: dry-run report).")
    ap.add_argument("--only-dups", action="store_true",
                    help="consolidate only RVAs declared in >1 site.")
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    blocks = scan_tree()
    consolidate, skipped = plan(blocks, only_dups=args.only_dups)
    n_sites = sum(len(c[2]) for c in consolidate)
    log(f"scanned {len(blocks)} DATA block(s) across the tree")
    log(f"consolidating {len(consolidate)} rva(s) from {n_sites} site(s) "
        f"-> {GLOBALS_H.name}/{GLOBALS_CPP.name}")
    log(f"skipped {len(skipped)} rva(s) (left in place)")
    if args.verbose:
        from collections import Counter
        reasons = Counter(r for _, r, _ in skipped)
        for r, n in reasons.most_common():
            log(f"  SKIP {n:4d}  {r}")
    h, cpp, file_edits = apply_changes(consolidate, skipped, args.apply)
    if args.apply:
        log(f"wrote {GLOBALS_H} ({h.count(chr(10))} lines), "
            f"{GLOBALS_CPP} ({cpp.count(chr(10))} lines)")
        log(f"dropped DATA() macro from {len(file_edits)} using TU(s) "
            f"(externs kept in place)")
    else:
        log("dry-run; pass --apply to write. Sample Globals.h head:")
        print("\n".join(h.splitlines()[:30]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
