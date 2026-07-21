#!/usr/bin/env python3
"""reorder_tu.py - sort a TU's function DEFINITIONS to ascending-RVA order.

Retail intra-TU function order == the devs' in-`.cpp` source-definition order
(MSVC /O2 emits each function as its own COMDAT, laid out in source order - see
`docs/link-order-investigation.md`). Reordering a TU's function definitions to
ascending RVA is:

  * **objdiff-NEUTRAL** - objdiff pairs functions by symbol name and scores each
    independently; the *order/set* of definitions in a `.cpp` does not touch any
    function's bytes (docs/patterns/within-tu-order-vs-field-order.md). A reorder
    must leave `gruntz build` GREEN with a 0-delta objdiff report.
  * the fix for the whole-EXE layout number `exe-layout-order%`
    (`gruntz exe-diff`): each reordered `.cpp` flips its TU to "intra-TU order
    correct".
  * the devs' real source order.

This is the mechanical reorder engine used by the tree-wide reorder campaign.

MODEL - dependency-aware placement
----------------------------------
The file is split into  PREAMBLE | <region> | TRAILER.

  * PREAMBLE - everything above the first function's leading content (includes,
    `using`, file-scope globals / DATA externs / forward decls / types defined
    before the first function, a namespace-open, the file-header comment). FIXED.
  * TRAILER  - everything after the last function's body (a namespace-close,
    trailing helpers). FIXED.

The region is a sequence of ITEMs: FUNCTION blocks (an RVA-anchored definition +
its tight leading comment/annotation) and DECLARATION groups (any real code
between two functions: a `struct`/`class`/`typedef`/`enum`/`union`/global/
`extern`/`#define`/forward-decl/helper, with its own comment). Blank/comment-only
gaps are trailed onto the preceding function (deterministic; keeps orphan comments
put). Then:

  * each function's sort key is its RVA;
  * each declaration group is placed just before the LOWEST-RVA function that
    references any NAME it defines (a declaration with no in-TU user floats to the
    top of the region); this is what keeps a *shared* local type ahead of ALL its
    users. e.g. `struct W {...}` used by `W::AddItem` (0x181660) and `W::DropItem`
    (0x1816a0) is placed before `AddItem`, so both methods still follow it;
  * items are stable-sorted by (key, decl-before-func, original index) and emitted.

Because declarations carry no objdiff-scored code (types emit nothing; static
data / helper COMDATs are order-neutral), moving them is byte-neutral, and placing
each ahead of its lowest-RVA user keeps every use-before-decl satisfied - the
build stays green while functions reach ascending-RVA order.

Handled edge cases:
  * functions nested one level in `namespace { ... }` (depth-relative brace
    tracking; the namespace open/close land in preamble/trailer).
  * one-line bodies (`void f() {}`), /GX EH blocks, multi-line signatures.
  * a block carrying TWO RVA lines for one function -> key = the lowest RVA.
  * braces/quotes inside strings, char literals, `//` and `/* */` comments, and
    `#define` macro bodies are not miscounted.
  * a declaration shared by several sibling methods stays ahead of all of them.

A use-before-definition GUARD (`_use_before_def`) runs after every reorder: if the
new order would place a free function / macro / top-level forward declaration
below a use of it, the file is left UNTOUCHED and reported as skipped. So a
tree-wide run only rewrites files it can prove sort cleanly.

Idempotent: on the reordered file, declarations that landed before the first
function are absorbed into the preamble and the functions are already ascending,
so a re-run rewrites byte-for-byte.

SAFETY / LIMITATIONS - **run per file (or small batch) with a `gruntz build`
check; revert any file that fails.** The guard + this dependency model make the
common case (functions, comments, local single-use types, per-function
`#define/#undef`, ILT `__asm` thunks) reorder cleanly and byte-neutrally, but a
STATIC textual tool cannot see every C++ dependency:
  * a local type used only IMPLICITLY - via a helper's return type
    (`ResolveItem(id)->m_field`), a typedef alias, or `auto` - is not textually
    mentioned by its user, so the tool may place the type after that use. Seen in
    the big src/Stub aggregate (ApiCallers) with many nested local structs; the
    guard does not catch it (it is a type, not a free symbol). BUILD CATCHES IT.
  * a declaration that DEPENDS on another mid-region declaration defined later in
    the file (rare - base types usually precede) could invert; `#if 0` dead code
    with unbalanced braces is not modelled.
Measured on this tree: of 147 files with inversions, ~139 reorder green +
objdiff-0-delta, 8 are auto-skipped by the guard, and a couple of decl-heavy
aggregates need the build backstop.

Usage:
    python -m gruntz.audit.reorder_tu <file.cpp> [<file2.cpp> ...]
    python -m gruntz.audit.reorder_tu --check <file.cpp> ...   # would-change? (exit 1)
    python -m gruntz.audit.reorder_tu --dry-run <file.cpp> ... # report, no write
"""

import argparse
import re
import sys
from pathlib import Path

RVA_RE = re.compile(r"^\s*(RVA|RVAU)\(\s*(0x[0-9a-fA-F]+)")
ANNOT_RE = re.compile(r"^\s*(RVA|RVAU|SYMBOL)\(")


def analyze(text):
    """Per-line scan. Returns lines and parallel arrays:
      depth_start[i]   - brace depth at the start of line i
      comment_start[i] - line i starts inside a /* */ block comment
      net[i]           - net code brace delta on line i
      opens[i]         - count of code '{' on line i
      blank[i]         - line i is whitespace only
      comment[i]       - line i is comment-only (// , /* , or inside a block comment)
      annot[i]         - line i is an RVA/RVAU/SYMBOL annotation (not in a comment)
    Braces/quotes inside strings, chars, comments and #directives are ignored."""
    lines = text.splitlines(keepends=True)
    n = len(lines)
    depth_start = [0] * n
    comment_start = [False] * n
    net = [0] * n
    opens = [0] * n
    blank = [False] * n
    comment = [False] * n
    annot = [False] * n

    depth = 0
    in_block = False  # inside /* */, persists across lines
    pp_cont = False  # previous physical line was a continued #directive

    for i, line in enumerate(lines):
        depth_start[i] = depth
        comment_start[i] = in_block
        stripped = line.lstrip()
        blank[i] = stripped == ""

        this_pp = False
        if not in_block:
            this_pp = pp_cont or stripped.startswith("#")

        line_net = 0
        line_open = 0
        j = 0
        L = line
        ln = len(L)
        in_str = False
        in_chr = False
        while j < ln:
            c = L[j]
            two = L[j:j + 2]
            if in_block:
                if two == "*/":
                    in_block = False
                    j += 2
                    continue
                j += 1
                continue
            if in_str:
                if c == "\\":
                    j += 2
                    continue
                if c == '"':
                    in_str = False
                j += 1
                continue
            if in_chr:
                if c == "\\":
                    j += 2
                    continue
                if c == "'":
                    in_chr = False
                j += 1
                continue
            if two == "//":
                break
            if two == "/*":
                in_block = True
                j += 2
                continue
            if c == '"':
                in_str = True
                j += 1
                continue
            if c == "'":
                in_chr = True
                j += 1
                continue
            if not this_pp:
                if c == "{":
                    line_net += 1
                    line_open += 1
                elif c == "}":
                    line_net -= 1
            j += 1

        net[i] = line_net
        opens[i] = line_open
        depth += line_net

        comment[i] = comment_start[i] or (not blank[i] and (
            stripped.startswith("//") or stripped.startswith("/*")))
        annot[i] = (not comment_start[i]) and bool(ANNOT_RE.match(line))

        raw = line.rstrip("\n").rstrip("\r")
        pp_cont = this_pp and raw.endswith("\\")

    return dict(lines=lines, n=n, depth_start=depth_start, comment_start=comment_start,
                net=net, opens=opens, blank=blank, comment=comment, annot=annot)


def find_anchors(a):
    """Locate every RVA/RVAU-governed function: list of (rva, sig_line, body_end).
    Adjacent RVA lines governing the SAME function merge (key = lowest RVA)."""
    lines, n = a["lines"], a["n"]
    ds, cs, net, opens = a["depth_start"], a["comment_start"], a["net"], a["opens"]
    blank, comment, annot = a["blank"], a["comment"], a["annot"]

    raw = []
    for i in range(n):
        if cs[i]:
            continue
        m = RVA_RE.match(lines[i])
        if not m:
            continue
        rva = int(m.group(2), 16)

        sig = i + 1
        while sig < n and (blank[sig] or comment[sig] or annot[sig]):
            sig += 1
        if sig >= n:
            continue

        base = ds[sig]
        opened = False
        e = sig
        body_end = None
        while e < n:
            if opens[e] > 0:
                opened = True
            d_after = ds[e] + net[e]
            if opened and d_after <= base:
                body_end = e
                break
            if not opened and e > sig and annot[e]:
                body_end = e - 1
                break
            e += 1
        if body_end is None:
            body_end = min(e, n - 1)
        raw.append((rva, sig, body_end))

    anchors = []
    for rva, sig, end in raw:
        if anchors and anchors[-1][1] == sig:
            prev = anchors[-1]
            anchors[-1] = (min(prev[0], rva), sig, end)
        else:
            anchors.append((rva, sig, end))
    return anchors


def tight_top(a, sig, lo):
    """First line of a function's TIGHT leading content: skip its annotation
    line(s), any blank lines between annotation and comment, then the ONE comment
    paragraph directly above (stop at the first blank line above the paragraph).
    A blank-separated comment above is NOT grabbed - so comment attribution does
    not depend on which function ends up above after sorting (idempotency)."""
    blank, comment, annot = a["blank"], a["comment"], a["annot"]
    i = sig - 1
    while i >= lo and annot[i]:
        i -= 1
    while i >= lo and blank[i]:
        i -= 1
    while i >= lo and comment[i]:
        i -= 1
    return i + 1


# tokens that are never a user-defined name a function "depends on" (keywords,
# builtins, the project's int aliases, common Win32 typedefs). Excluding them
# keeps a declaration that merely *mentions* `int`/`DWORD`/... from being pulled
# to the top by every function.
_STOP = set("""
struct class union enum typedef static extern const volatile inline virtual mutable
register auto friend explicit namespace using template typename operator return if
else for while do switch case default break continue goto sizeof new delete this
true false void int char short long unsigned signed float double bool wchar_t
public private protected __cdecl __stdcall __thiscall __fastcall __declspec
dllimport dllexport i8 u8 i16 u16 i32 u32 i64 u64 f32 f64 BOOL DWORD WORD BYTE
UINT INT LONG ULONG SHORT USHORT CHAR UCHAR VOID PVOID LPVOID HWND HANDLE HDC
HRESULT LPARAM WPARAM LRESULT UINT_PTR INT_PTR DWORD_PTR SIZE_T LPCSTR LPSTR
LPCTSTR LPTSTR
""".split())
_IDENT = re.compile(r"[A-Za-z_]\w*")
# closing preprocessor directives belong AFTER the function they follow (e.g. a
# `#undef` that ends a per-function macro), not before its user.
_CLOSING_PP = re.compile(r"^\s*#\s*(?:undef|endif|else|elif|pragma)\b")


def _strip_comments(line):
    """Crudely drop // and /* */ comment text from a single physical line."""
    line = re.sub(r"/\*.*?\*/", " ", line)
    line = re.sub(r"//.*", " ", line)
    return line


def _decl_idents(line):
    """Identifiers a declaration line could DEFINE - comments and string/char
    literals stripped (so `extern \"C\"` never yields a bogus name `C`)."""
    line = _strip_comments(line)
    line = re.sub(r'"(?:\\.|[^"\\])*"', " ", line)
    line = re.sub(r"'(?:\\.|[^'\\])*'", " ", line)
    return _IDENT.findall(line)


# region-level free-symbol definitions/declarations, for the use-before-def guard
_FREE_DEF_RE = re.compile(r"^\s*(?:extern\s+\"C\"\s+)?[\w:<>*&\s]*?\b(\w+)\s*\([^;{]*\)\s*[;{]")
_MACRO_DEF_RE = re.compile(r"^\s*#\s*define\s+(\w+)")


def _use_before_def(new_text):
    """Return the name of a FREE function / macro / forward-decl whose definition
    the reorder pushed BELOW a use of it (and which is not declared in the
    preamble). METHODS (Class::m) are prototyped by their class header, so they
    are exempt. A non-None result means the reorder would not compile - skip it."""
    a = analyze(new_text)
    lines = a["lines"]
    blank, comment, ds = a["blank"], a["comment"], a["depth_start"]
    anchors = find_anchors(a)
    if len(anchors) < 2:
        return None
    region_lo = tight_top(a, anchors[0][1], 0)
    base = ds[region_lo]  # region top-level brace depth (0, or 1 inside a namespace)

    pre = set()
    for i in range(region_lo):
        pre.update(_IDENT.findall(_strip_comments(lines[i])))

    defs = {}  # free-symbol name -> EARLIEST def/decl line in region

    def note(name, line):
        if name not in defs or line < defs[name]:
            defs[name] = line

    for rva, sig, end in anchors:  # free-function definitions (no `::`)
        if "::" in lines[sig].split("(", 1)[0]:
            continue
        m = re.search(r"(\w+)\s*\(", lines[sig])
        if m and m.group(1) not in _STOP:
            note(m.group(1), sig)
    for i in range(region_lo, len(lines)):
        if ds[i] != base or blank[i] or comment[i]:  # region top level only
            continue
        code = _strip_comments(lines[i])
        mm = _MACRO_DEF_RE.match(code)
        if mm:
            note(mm.group(1), i)
        fd = _FREE_DEF_RE.match(code)
        if fd and fd.group(1) not in _STOP and "::" not in code.split("(", 1)[0]:
            note(fd.group(1), i)

    for name, defline in defs.items():
        if name in pre:
            continue
        pat = re.compile(r"\b" + re.escape(name) + r"\b")
        for i in range(region_lo, defline):
            if blank[i] or comment[i]:
                continue
            if pat.search(_strip_comments(lines[i])):
                return name
    return None


def reorder_text(text):
    """Return (new_text, info) where info={'anchors','inversions','changed'}."""
    a = analyze(text)
    lines = a["lines"]
    ds, blank, comment, annot = a["depth_start"], a["blank"], a["comment"], a["annot"]
    anchors = find_anchors(a)

    info = {"anchors": len(anchors), "inversions": 0, "changed": False, "skipped": None}
    if len(anchors) < 2:
        return text, info

    region_lo = tight_top(a, anchors[0][1], 0)
    region_hi = anchors[-1][2]

    def is_code(i):
        return not (blank[i] or comment[i] or annot[i])

    # split the region into ordered items: functions and declaration groups
    funcs = []   # {rva, top, end, orig}
    decls = []   # {start, end, orig, names}
    order = 0
    prev_end = region_lo - 1  # body_end of previous function in file order
    for idx, (rva, sig, end) in enumerate(anchors):
        top = region_lo if idx == 0 else tight_top(a, sig, prev_end + 1)
        g0, g1 = prev_end + 1, top - 1  # inclusive gap between prev func and this one
        if idx > 0 and g1 >= g0:
            # peel a leading run of blanks + closing directives (e.g. a `#undef`
            # ending the previous function's macro) - those trail the PREVIOUS
            # function; the remainder (a declaration + its comment) leads the next.
            t = g0
            while t <= g1 and (blank[t] or (
                    is_code(t) and _CLOSING_PP.match(_strip_comments(lines[t])))):
                t += 1
            if any(is_code(i) for i in range(t, g1 + 1)):
                if funcs and t > g0:
                    funcs[-1]["end"] = t - 1
                base = ds[t]
                names = set()
                for i in range(t, g1 + 1):
                    if ds[i] != base or blank[i] or comment[i]:
                        continue
                    for tok in _decl_idents(lines[i]):
                        if tok not in _STOP:
                            names.add(tok)
                decls.append({"start": t, "end": g1, "orig": order, "names": names})
                order += 1
            elif funcs:  # blank/comment/closing-directive-only gap -> trail prev func
                funcs[-1]["end"] = g1
        funcs.append({"rva": rva, "top": top, "end": end, "orig": order})
        order += 1
        prev_end = end

    info["inversions"] = sum(
        1 for x, y in zip(funcs, funcs[1:]) if x["rva"] > y["rva"])

    # each declaration group -> the lowest RVA of a function that references any
    # name it defines (else a sentinel below every RVA, i.e. top of the region)
    SENTINEL = -1
    fident = []
    for f in funcs:
        toks = set()
        for i in range(f["top"], f["end"] + 1):
            toks.update(_IDENT.findall(lines[i]))
        fident.append(toks)
    for d in decls:
        users = [f["rva"] for f, ids in zip(funcs, fident) if d["names"] & ids]
        d["key"] = min(users) if users else SENTINEL

    # stable-sort all items by (key, decl-before-func, original index)
    items = [(f["rva"], 1, f["orig"], ("f", f)) for f in funcs]
    items += [(d["key"], 0, d["orig"], ("d", d)) for d in decls]
    items.sort(key=lambda t: (t[0], t[1], t[2]))

    out = list(lines[:region_lo])
    for _key, _kind, _orig, (tag, it) in items:
        out.extend(lines[it["start" if tag == "d" else "top"]:it["end"] + 1])
    out.extend(lines[region_hi + 1:])
    new = "".join(out)

    if new != text:
        hazard = _use_before_def(new)
        if hazard:
            info["skipped"] = hazard  # would not compile - leave the file untouched
            return text, info
    info["changed"] = new != text
    return new, info


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("files", nargs="+", help=".cpp files to reorder in place")
    ap.add_argument("--check", action="store_true",
                    help="report which files would change; write nothing (exit 1 if any).")
    ap.add_argument("--dry-run", action="store_true", help="report the reorder; write nothing.")
    ap.add_argument("-q", "--quiet", action="store_true")
    args = ap.parse_args()

    would_change = False
    for f in args.files:
        p = Path(f)
        text = p.read_text()
        new, info = reorder_text(text)
        tag = f"{p.name:28} anchors={info['anchors']:3d} inversions={info['inversions']:2d}"
        if info.get("skipped"):
            if not args.quiet:
                print(f"SKIP unsafe    {tag}  (use-before-def of '{info['skipped']}')")
        elif info["changed"]:
            would_change = True
            if args.check or args.dry_run:
                if not args.quiet:
                    print(f"WOULD REORDER  {tag}")
            else:
                if sorted(new) != sorted(text):  # pure line permutation guard
                    print(f"ABORT (byte-set changed) {p}", file=sys.stderr)
                    sys.exit(2)
                p.write_text(new)
                if not args.quiet:
                    print(f"REORDERED      {tag}")
        elif not args.quiet:
            print(f"ok (sorted)    {tag}")

    if args.check and would_change:
        sys.exit(1)


if __name__ == "__main__":
    main()
