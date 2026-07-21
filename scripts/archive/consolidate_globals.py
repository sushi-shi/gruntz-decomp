#!/usr/bin/env python3
"""consolidate_globals.py - de-proliferate matched-global DATA() bindings.

A matched GLOBAL is bound to its retail address by `DATA(0xADDR)` (include/rva.h)
sitting above its declaration. Over the campaign matchers RE-DECLARED the same
global in many TUs, so a single RVA's DATA() can appear in dozens of files
(measured: 118 RVAs in >1 TU). labels.py keeps only the keep-last winner and WARNs
on the rest; the address binding is silently duplicated.

=============================================================================
THE DEFECT THIS TOOL USED TO CREATE  (fixed 2026-07-13 - do not reintroduce)
=============================================================================
This tool used to "consolidate" a global by moving its binding into src/Globals.cpp
as `DATA(0xADDR)` + `extern T g_name;` - a DECLARATION. Its own banner sold that as
a feature:

    "Unused externs emit no symbols, so the base obj is empty -> matching-neutral."

That sentence IS the bug. An `extern` emits no STORAGE, so NOTHING in the whole tree
defined the global: the label pass happily binds name->rva (so the symbol looks
"done" in symbol_names.csv) while every obj carries it as UNDEF. The link can never
succeed. This single generator shape is why the UNDEFINED-DATA link-defect bucket
(282 symbols) sat flat for the entire campaign - a data symbol is NEVER produced by
reconstructing a function, so it could not resolve on its own.

THE RIGHT END STATE: src/Globals.cpp should not exist. Every global's DATA() binding
belongs on its DEFINITION, in its OWNER TU; every other TU keeps a plain local
`extern`. This tool now homes bindings that way and REFUSES to create a
declaration-only pen. `--audit` fails on any `DATA()`-on-`extern` left in the tree,
so the hole cannot silently reopen.

WHY NOT `#include <Globals.h>` everywhere? MEASURED: under MSVC 5.0 adding ~270
extern declarations to a TU (via one shared header) PERTURBS the optimizer's
register allocation for unrelated functions - objdiff regressions. So the fix is
DISPERSAL to owner TUs, never centralisation: TUs keep their own local externs, and
Globals.h stays a documentation-only reference that is never force-included.

SAFETY GATES on synthesising a definition (each one paid for in real damage):
  * NEVER a `char[]` / unsized array. Its extent can only be guessed from the gap to
    the next NAMED address, which SWALLOWS the unnamed $SG string literals in
    between (measured: g_msgCaption came out 2752 bytes; s_out_of_memory 180 bytes
    spanning four unrelated strings). Those are compiler-emitted literals, not
    globals - write the literal at its use site. A wrong-sized array WOULD LINK,
    which makes it the worst possible defect: a fabrication that passes every gate.
  * NEVER an INITIALIZED datum whose bytes we cannot transcribe from the retail EXE.
    A zero-init definition of initialized data is simply wrong.
  * NEVER a class-typed value (its ctor form is unknown here; e.g. zDArray has no
    default ctor - retail's static-init thunk calls the 4-arg 0x16de30 ctor).
  * The owner TU is the UNIQUE referencing TU. If several TUs reference the global,
    the owner must be settled by xref - "the first file that happens to extern it"
    is exactly how g_buteMgr would land in BootyCheatState.cpp. Ambiguous -> LEFT IN
    PLACE and reported, never moved.
Anything refused is REPORTED, never guessed.

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
    # A DEFINITION is a decl with an initialiser OR - the case that used to be missed -
    # one that simply LACKS `extern` (`char g_cheatTable[0xfa0];` is a tentative
    # definition: it emits storage). Missing that made the tool copy definitions
    # VERBATIM into Globals.h, so every includer defined the global (C2086 redefinition)
    # and Globals.cpp emitted a second one. A definition is never moved and never
    # re-homed.
    body = re.sub(r'^extern\s+"C"\s*\{?', "", decl).strip()
    is_def = ("=" in decl) or ("{" in raw) or not re.match(r"extern\b", body or decl)
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
// plain `extern T g_name;`, purely as DOCUMENTATION of its one true type.
//
// This header is NEVER force-included: under MSVC 5.0 pulling ~270 extra extern
// declarations into a TU perturbs the optimizer's register allocation (measured
// objdiff regressions). Matched TUs keep their own in-place externs. A global's
// DATA(0xADDR) binding lives on its DEFINITION in its OWNER TU - not here, and not
// in a central pen (see src/Globals.cpp).
"""

CPP_BANNER = """// Globals.cpp - THE UNHOMED-GLOBAL BACKLOG. Every entry below is a LINK DEFECT.
//
// GENERATED by `python3 -m gruntz.analysis.consolidate_globals --apply`; do not
// hand-edit (re-run the tool).
//
// READ THIS BEFORE ADDING ANYTHING HERE. This file used to be described as "the
// single DATA() binding point ... unused externs emit no symbols, so the base obj is
// empty -> matching-neutral". That reassurance was the bug: an `extern` emits no
// STORAGE, so nothing in the tree defines these globals. The label pass still binds
// name->rva (they look "done" in symbol_names.csv) while every obj carries them
// UNDEF - they can NEVER link. A data symbol is never produced by reconstructing a
// function, so none of this resolves for free.
//
// Each `DATA(0xADDR) extern T g_name;` remaining here is therefore an OPEN
// UNDEFINED-DATA defect, kept in one place only so it is countable. The fix for each
// is the same: give it ONE real DEFINITION in its OWNER TU (owner = who xrefs it),
// move the DATA() binding onto that definition, and leave a plain `extern` in the
// TUs that merely use it. This file shrinks toward EMPTY, and empty is the goal -
// centralising definitions here instead would just move the lie (and force-including
// a fat globals header regresses matched TUs through regalloc: measured).
"""


def _fwd_set(consolidate):
    fwd = set()
    for rva, canon, sites, inc, fd in consolidate:
        fwd |= fd
    return fwd


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


def _as_extern(decl):
    """Render any decl as a DECLARATION: strip an initialiser and force `extern`.
    Globals.h is a header - emitting a DEFINITION here (a decl with no `extern`, e.g.
    `char g_cheatTable[0xfa0];`) defines the global in EVERY includer (C2086)."""
    d = normalise(decl)
    d = re.sub(r"\s*=.*$", "", d)                       # drop any initialiser
    if re.match(r'extern\s+"C"\b', d):
        return d
    if not re.match(r"extern\b", d):
        d = "extern " + d
    return d


def render_globals_h(consolidate, defs=()):
    lines = [HEADER_BANNER, "#ifndef GRUNTZ_GLOBALS_H", "#define GRUNTZ_GLOBALS_H", ""]
    lines += _preamble(consolidate)
    lines.append("")
    plain, clink = _split_linkage(consolidate)
    for rva, canon, sites, inc, fd in plain:
        lines.append(_as_extern(canon.decl) + ";")
    if clink:
        # C-linkage globals declared inside an `extern "C" {}` block in their TU:
        # keep them wrapped so the C-decorated mangled name is reproduced and the
        # member decl text matches the TU (idempotency; see from_linkage_block).
        lines += ["", 'extern "C" {']
        for rva, canon, sites, inc, fd in clink:
            lines.append(_as_extern(canon.decl) + ";")
        lines.append("}")
    if defs:
        # Globals.h must DECLARE every DATA-bound global - including the ones that are
        # now real DEFINITIONS in their owner TU. TUs `#include <Globals.h>` for the
        # declaration; dropping it once the global gets defined breaks them (and was
        # exactly the breakage the first homing run caused).
        fwd_names = {n for _, n in _fwd_set(consolidate)}
        lines += ["", "// Globals DEFINED in their owner TU (this is the reference decl)."]
        seen = set()
        for b in sorted(defs, key=lambda b: b.var):
            if b.var in seen:
                continue
            # Globals.h is deliberately include-light (<rva.h> only). A class/struct-typed
            # VALUE cannot be declared here without dragging its header into every
            # includer - those stay declared in their owner TU / the class's real header.
            ty = re.sub(r"\[[^\]]*\]", " ", b.type_str)
            ty = re.sub(r"\b" + re.escape(b.var) + r"\b", " ", ty)
            ids = [x for x in re.findall(r"[A-Za-z_]\w*", ty)
                   if x not in BUILTINS and x not in CC_KW]
            # ...and a POINTER global is only declarable here if its pointee is among
            # the forward-decls the preamble already emits.
            if any(x not in fwd_names for x in ids):
                continue
            seen.add(b.var)
            lines.append(_as_extern(b.decl) + ";")
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


# ---------------------------------------------------------------------------
# HOMING: a global's DATA() binding belongs on its DEFINITION in its OWNER TU.
# ---------------------------------------------------------------------------

def _pe_sections():
    """(name, va, vsize, rawsize) per PE section of the retail EXE, or [] if the EXE
    is not available (then we simply cannot prove .bss-ness and home nothing)."""
    import struct
    exe_path = os.environ.get("GRUNTZ_EXE")
    if not exe_path or not Path(exe_path).exists():
        return []
    data = Path(exe_path).read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    optsz = struct.unpack_from("<H", data, pe + 20)[0]
    out, off = [], pe + 24 + optsz
    for _ in range(nsec):
        nm = data[off:off + 8].rstrip(b"\x00").decode(errors="ignore")
        vsz, va, rsz, _ro = struct.unpack_from("<IIII", data, off + 8)
        out.append((nm, va, vsz, rsz))
        off += 40
    return out


def _is_zero_init_data(rva, secs):
    """True iff `rva` lands in the UNINITIALIZED tail of a data section (retail .bss):
    the loader zeroes it, so a bare definition reproduces it exactly. Initialized data
    is refused - a zero-init definition of initialized data is simply wrong."""
    for nm, va, vsz, rsz in secs:
        if va <= rva < va + vsz:
            return nm == ".data" and rva >= va + rsz
    return False


def definable(block, rva, secs):
    """Can we synthesise an HONEST definition for this global? (ok, reason).

    Every `False` below is a gate paid for in real damage - see the module docstring.
    """
    if block.is_def:
        return False, "already a definition"
    if not secs:
        return False, "$GRUNTZ_EXE unavailable - cannot prove the datum is zero-init"
    if not _is_zero_init_data(rva, secs):
        return False, ("not zero-init .data - an initialized/.rdata datum needs its "
                       "RETAIL BYTES transcribed, never a zero-init guess")
    ty = block.type_str
    # unsized arrays: the extent can only be guessed from the next NAMED address,
    # which swallows the unnamed $SG literals in between. Never.
    if re.search(r"\[\s*\]", ty) or re.search(r"\[\s*\]", block.decl):
        return False, ("unsized array - its extent would have to be guessed and would "
                       "swallow the unnamed $SG literals in between")
    if re.search(r"\bchar\b", ty) and "[" in block.decl:
        return False, "char[] datum - this is a STRING literal, not a global"
    core = re.sub(r"\[[^\]]*\]", " ", ty)
    core = re.sub(r"\b" + re.escape(block.var) + r"\b", " ", core)   # drop the var name
    core = re.sub(r"\b(extern|const|volatile|static|struct|class|union)\b", " ", core)
    core = core.replace("*", " ").strip()
    ids = [x for x in re.findall(r"[A-Za-z_]\w*", core) if x not in CC_KW]
    is_ptr = "*" in ty
    if not is_ptr and ids and any(x not in BUILTINS for x in ids):
        return False, (f"value of a class/struct type ({' '.join(ids)}) - its ctor form "
                       "is unknown here (e.g. zDArray has no default ctor)")
    return True, ""


def owner_tu(sites):
    """The TU that OWNS the global = the unique TU that declares/uses it. Several
    referencing TUs -> the owner must be settled by xref, not by 'first extern'."""
    files = {s.file.resolve() for s in sites
             if s.file.resolve() != GLOBALS_CPP.resolve()}
    return next(iter(files)) if len(files) == 1 else None


def audit_tree(blocks):
    """Every `DATA()`-on-`extern` in the tree is an OPEN undefined-data defect: the
    name is bound, no storage is emitted, the link can never succeed. Returns the
    list so the hole cannot silently reopen."""
    return [b for b in blocks if not b.is_def]


def home_definitions(consolidate, do_apply):
    """Turn each consolidatable global's annotated DECLARATION into a real DEFINITION
    in its OWNER TU (and drop it from the central pen). Refuses anything it cannot
    define honestly - see `definable`. Returns (homed, refused)."""
    secs = _pe_sections()
    homed, refused = [], []
    edits = {}   # file -> [(start, end, newtext)]
    keep = []
    for entry in consolidate:
        rva, canon, sites, inc, fd = entry
        own = owner_tu(sites)
        if own is None:
            refused.append((rva, canon.var, "several TUs reference it - the owner must "
                            "be settled by xref, not by first-extern"))
            keep.append(entry)
            continue
        ok, why = definable(canon, int(rva, 16), secs)
        if not ok:
            refused.append((rva, canon.var, why))
            keep.append(entry)
            continue
        site = next(s for s in sites if s.file.resolve() == own)
        decl = normalise(site.decl)
        # strip the leading `extern` (keep an `extern "C"` linkage-spec: with an
        # initializer-less class/POD it must become a definition, so use the block form)
        if re.match(r'extern\s+"C"\s', decl):
            body = re.sub(r'^extern\s+"C"\s+', "", decl)
            newtext = f'DATA({rva})\nextern "C" {{\n{body} = 0;\n}}'
            if "[" in body:                       # arrays: aggregate zero-init
                newtext = f'DATA({rva})\nextern "C" {{\n{body} = {{0}};\n}}'
        else:
            body = re.sub(r"^extern\s+", "", decl)
            newtext = f"DATA({rva})\n{body};"
        cmt = f"  // {site.comment.lstrip('/ ')}" if site.comment else ""
        newtext += cmt
        edits.setdefault(site.file, []).append((site.start, site.end, newtext))
        homed.append((rva, canon.var, str(site.file.relative_to(REPO))))
    if do_apply:
        for f, es in edits.items():
            text = f.read_text()
            for start, end, new in sorted(es, key=lambda e: e[0], reverse=True):
                text = text[:start] + new + text[end:]
            f.write_text(text)
    return homed, refused, keep


def apply_changes(consolidate, skipped, do_apply, defs=()):
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
    h = render_globals_h(consolidate, defs)
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
    ap.add_argument("--audit", action="store_true",
                    help="FAIL (rc=1) on every DATA()-on-`extern` left in the tree - "
                         "each one is an open UNDEFINED-DATA defect (the name binds, no "
                         "storage is emitted, the link can never succeed).")
    ap.add_argument("--home", action="store_true",
                    help="home each global's DATA() binding onto a real DEFINITION in "
                         "its OWNER TU (refuses anything it cannot define honestly).")
    args = ap.parse_args()

    blocks = scan_tree()

    if args.audit:
        bad = audit_tree(blocks)
        if not bad:
            log("audit OK - no DATA()-on-`extern` left; every binding sits on a definition")
            return 0
        log(f"AUDIT FAIL: {len(bad)} DATA()-on-`extern` binding(s) - each is an OPEN")
        log("UNDEFINED-DATA defect: the name binds, NO storage is emitted, the link")
        log("can never succeed. Fix = one real definition in the owner TU.")
        from collections import Counter
        for f, n in Counter(str(b.file.relative_to(REPO)) for b in bad).most_common(20):
            log(f"  {n:4d}  {f}")
        return 1

    consolidate, skipped = plan(blocks, only_dups=args.only_dups)

    if args.home:
        homed, refused, keep = home_definitions(consolidate, args.apply)
        log(f"homed {len(homed)} global(s) onto a DEFINITION in their owner TU")
        for rva, var, f in sorted(homed, key=lambda x: x[2]):
            log(f"  {var:<28} {rva}  -> {f}")
        log(f"refused {len(refused)} (reported, never guessed):")
        from collections import Counter
        for why, n in Counter(w for _, _, w in refused).most_common():
            log(f"  {n:4d}  {why}")
        consolidate = keep
        if not args.apply:
            log("dry-run; pass --apply to write.")
            return 0
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
    defs = [b for b in blocks
            if b.is_def and b.file.resolve() != GLOBALS_CPP.resolve()]
    h, cpp, file_edits = apply_changes(consolidate, skipped, args.apply, defs)
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
