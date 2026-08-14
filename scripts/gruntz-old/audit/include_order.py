#!/usr/bin/env python3
"""include_order.py - the canonical #include block: no duplicates, one order.

The tree was reconstructed function-by-function, so includes accreted wherever a
type was first needed: appended mid-file, duplicated, unordered. The 2026-08-02
sweep hoisted every mid-file include (provably matching-neutral - 0 of 4275
functions moved) and folded every <afx*.h> behind the Mfc*.h wrappers. This tool
owns what is left - DUPLICATES, SELF-SUFFICIENCY and ORDER - and gates them.

THE ORDER. Groups are separated by one blank line; within a group includes sort
case-insensitively, except group 2 which has an explicit rank (below).

  0  file-local configuration #defines   e.g. `#define DIRECTINPUT_VERSION 0x0500`
  1  <rva.h>                             the label-macro header, always first
  2  the TU's OWN header                 X.cpp -> <Dir/X.h>   (.cpp only)
  3  platform preludes                   <Mfc.h> <MfcNoInline.h> <MfcWin.h>
                                         <Win32.h>
  4  project / engine headers            everything else under include/
  5  library headers                     CRT, DirectX, 3rd-party, raw <afx*.h>

The own header leads its dependencies rather than sitting between two of their
groups: everything below group 2 is a dependency, in widening order. Parsing the
TU's own header first is also what keeps it honest - a header that needs <Mfc.h>
has to say so rather than inheriting it from whatever its .cpp happened to
include earlier.

WHY GROUP 3 IS RANKED, NOT SORTED. The preludes are not ordinary libraries -
they configure how every header parsed after them is seen. <Mfc.h> defines
VC_EXTRALEAN before <afx.h>; <MfcNoInline.h> undefines _AFX_ENABLE_INLINES so
the rest of the TU parses MFC's accessors out-of-line; <MfcWin.h> suppresses the
same inlines for clang only, before <afxwin.h>. Alphabetical order would land a
device after the header it is meant to configure, so group 3 carries the
dependency order explicitly (PRELUDE_RANK). Each device header pulls <Mfc.h>
itself, so it is atomic - it cannot be defeated by where it lands.

Only a prelude that carries LOGIC earns a wrapper. <MfcWin.h> holds the clang
inline suppression and <MfcNoInline.h> the per-TU codegen device; <afxtempl.h>
and <afxcmn.h> need no logic, so they stay raw and ride in group 5, where the
group-2 preludes still precede them.

SELF-SUFFICIENCY. Group 5 can only be last because no header leans on its
includer: a header that names CString includes <Mfc.h> itself. `--fix-prelude`
establishes that, `--gate` keeps it.

Anything the parser does not recognise inside the include block - a stray
#pragma, an #if that guards includes - makes the file MANUAL: reported, and left
untouched rather than mangled.

  python -m gruntz.audit.include_order                # report
  python -m gruntz.audit.include_order --gate         # exit 1 on any violation
  python -m gruntz.audit.include_order --fix-dupes    # drop duplicate includes
  python -m gruntz.audit.include_order --fix-prelude  # headers pull their own MFC/Win32
  python -m gruntz.audit.include_order --fix          # apply the canonical order
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])

SRC_DIRS = ("src", "include")
EXTS = (".h", ".cpp", ".hpp", ".inl", ".c")

INC_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
IFNDEF_RE = re.compile(r'^\s*#\s*ifndef\s+(\w+)\s*$')
DEFINE_RE = re.compile(r'^\s*#\s*define\s+(\w+)\s*$')
PP_RE = re.compile(r'^\s*#\s*(\w+)')

RVA_H = "rva.h"

# group 2, in dependency order - NOT alphabetical (see the module docstring)
PRELUDE_RANK = {
    "Mfc.h": 0,           # VC_EXTRALEAN + <afx.h> + <afxcoll.h>
    "MfcNoInline.h": 1,   # #undef _AFX_ENABLE_INLINES  (per-TU codegen device)
    "MfcWin.h": 2,        # clang-only inline suppression + <afxwin.h>
    "Win32.h": 3,         # WIN32_LEAN_AND_MEAN + <windows.h>
}

G_RVA, G_OWN, G_PRELUDE, G_PROJECT, G_LIBRARY = 1, 2, 3, 4, 5
GROUPS = (G_RVA, G_OWN, G_PRELUDE, G_PROJECT, G_LIBRARY)

# Spellings that oblige a header to pull its own prelude. Split by which MFC
# header actually declares them, so a header asking for CString does not drag in
# the whole <afxwin.h> window/GDI surface.
AFXWIN_TOKENS = re.compile(
    r'\b(CWnd|CDialog|CDC|CClientDC|CPaintDC|CWindowDC|CRgn|CBitmap|CPalette|'
    r'CFont|CBrush|CPen|CGdiObject|CWinApp|CWinThread|CFrameWnd|CView|CDocument|'
    r'CMenu|CButton|CEdit|CListBox|CComboBox|CStatic|CScrollBar|'
    r'CRect|CPoint|CSize|'
    r'DECLARE_MESSAGE_MAP|BEGIN_MESSAGE_MAP)\b')
AFX_TOKENS = re.compile(
    r'\b(CString|CObject|CFile|CArchive|CException|CMemFile|CRuntimeClass|'
    r'CPtrArray|CPtrList|CObList|CObArray|CStringList|CStringArray|CByteArray|'
    r'CWordArray|CDWordArray|CUIntArray|CMapPtrToPtr|CMapPtrToWord|'
    r'CMapStringToPtr|CMapStringToOb|CMapStringToString|CMapWordToPtr|'
    r'CMapWordToOb|CTime|CTimeSpan|POSITION|DECLARE_DYNAMIC|DECLARE_DYNCREATE|'
    r'DECLARE_SERIAL|IMPLEMENT_DYNAMIC|IMPLEMENT_DYNCREATE|IMPLEMENT_SERIAL)\b')
WIN_TOKENS = re.compile(
    r'\b(HWND|HDC|HINSTANCE|HBITMAP|HPALETTE|HMODULE|HRESULT|HGLOBAL|LPARAM|'
    r'WPARAM|LRESULT|tagRECT|tagPOINT|PALETTEENTRY|WINAPI|CALLBACK|IUnknown|'
    r'CRITICAL_SECTION|LARGE_INTEGER|WNDPROC|COLORREF|LPDIRECT\w+)\b')
# every prelude that already supplies the MFC/Win32 surface
MFC_SUPPLIERS = {"Mfc.h", "MfcWin.h", "MfcNoInline.h", "afx.h", "afxwin.h",
                 "afxtempl.h", "afxcmn.h"}
AFXWIN_SUPPLIERS = {"MfcWin.h", "afxwin.h", "afxcmn.h"}
WIN_SUPPLIERS = MFC_SUPPLIERS | {"Win32.h", "windows.h"}

# Vendored SDK headers that pull <windows.h> themselves (e.g. SFMAN.H) supply
# the Win32 surface to whoever includes them - computed, not hand-listed.
def _vendor_win_suppliers():
    out = set()
    vendor = REPO / "vendor"
    if vendor.is_dir():
        for p in vendor.rglob("*"):
            if p.suffix.lower() == ".h":
                try:
                    txt = p.read_text(encoding="utf-8", errors="replace")
                except OSError:
                    continue
                if re.search(r'^\s*#\s*include\s*[<"]windows\.h[>"]', txt, re.M):
                    out.add(p.name)
    return out


FWD_RE = re.compile(r'^\s*(?:class|struct|union)\s+(\w+)\s*;', re.M)
ELAB_RE = re.compile(r'\b(?:class|struct|union)\s+(\w+)')


def repo_files():
    for d in SRC_DIRS:
        for p in sorted((REPO / d).rglob("*")):
            if p.suffix in EXTS:
                yield p


PROJECT_HEADERS = {p.relative_to(REPO / "include").as_posix()
                   for p in (REPO / "include").rglob("*.h")}


def own_header(path: Path):
    if path.suffix != ".cpp":
        return None
    rel = path.relative_to(REPO / "src")
    cand = rel.with_suffix(".h").as_posix()
    if cand in PROJECT_HEADERS:
        return cand
    bare = rel.name[:-4] + ".h"
    return bare if bare in PROJECT_HEADERS else None


def classify(header: str, own: str | None) -> int:
    if header == RVA_H:
        return G_RVA
    if header in PRELUDE_RANK:
        return G_PRELUDE
    if own and header == own:
        return G_OWN
    if header in PROJECT_HEADERS:
        return G_PROJECT
    return G_LIBRARY


def sort_key(group: int, header: str):
    if group == G_PRELUDE:
        return (PRELUDE_RANK[header],)
    return (header.lower(),)


class Manual(Exception):
    """The include block holds something this tool will not rewrite."""


def parse(path: Path):
    """-> (head, entries, tail).

    head    guard + group-0 config defines
    entries [(comment_lines, header)] for the leading include block. A comment
            inside the block belongs to the include it introduces and travels
            with it when the block is sorted; a comment with no include after it
            (before any code) is NOT part of the block - it introduces the code
            below, so the block ends there and the comment starts the tail. That
            distinction is what keeps `// @early-stop` and `// clang-format off`
            attached to the thing they annotate.
    tail    the rest of the file, verbatim
    """
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    i, n, head = 0, len(lines), []

    def trivia():
        nonlocal i
        while i < n and (not lines[i].strip()
                         or lines[i].lstrip().startswith(("//", "/*", "*"))):
            head.append(lines[i])
            i += 1

    trivia()
    if i + 1 < n and (m := IFNDEF_RE.match(lines[i])):
        m2 = DEFINE_RE.match(lines[i + 1])
        if m2 and m2.group(1) == m.group(1):
            head.extend(lines[i:i + 2])
            i += 2
    trivia()
    while i < n and DEFINE_RE.match(lines[i]):
        head.append(lines[i])
        i += 1
        trivia()

    entries, start = [], i
    pending: list[str] = []      # comment lines seen since the last include
    pending_at = i               # where those comments began
    while i < n:
        s = lines[i].strip()
        if not s:
            i += 1
            continue
        if s.startswith(("//", "/*", "*")):
            if not pending:
                pending_at = i
            pending.append(lines[i])
            i += 1
            continue
        if m := INC_RE.match(lines[i]):
            entries.append((pending, m.group(1)))
            pending = []
            i += 1
            pending_at = i
            continue
        break

    # trailing comments introduce the CODE below, not an include - give them back
    if pending:
        i = pending_at

    if not entries:
        return head, [], lines[start:]

    depth = 0
    for j, line in enumerate(lines[i:]):
        s = line.strip()
        if m := PP_RE.match(s):
            d = m.group(1)
            if d in ("if", "ifdef", "ifndef"):
                depth += 1
            elif d == "endif":
                depth -= 1
            elif d == "include":
                raise Manual(f"include below the block (+{j})")
        elif s and not s.startswith(("//", "/*", "*")):
            break
    return head, entries, lines[i:]


def render(head, entries, tail, own):
    groups: dict[int, list] = {}
    seen: set[str] = set()
    for comments, h in entries:
        if h in seen:                      # duplicate: keep any comment it carried
            for g in groups.values():
                for k, (c, hh) in enumerate(g):
                    if hh == h:
                        g[k] = (c + comments, hh)
            continue
        seen.add(h)
        groups.setdefault(classify(h, own), []).append((comments, h))
    out = list(head)
    while out and not out[-1].strip():
        out.pop()
    if out:
        out.append("")
    first = True
    for g in GROUPS:
        if g not in groups:
            continue
        if not first:
            out.append("")
        first = False
        for comments, h in sorted(groups[g], key=lambda e: sort_key(g, e[1])):
            out.extend(comments)
            out.append(f"#include <{h}>")
    body = list(tail)
    while body and not body[0].strip():
        body.pop(0)
    if body:
        out.append("")
        out.extend(body)
    return out


def assert_conserved(path: Path, before: list[str], after: list[str], dropped: list[str]):
    """Nothing but blank lines and duplicate includes may change.

    The reorder is a permutation, not an edit: every non-blank line in the input
    must survive into the output, minus exactly the duplicate #include lines we
    meant to drop. A silent loss here once ate 38 `// @early-stop` markers, so
    this runs on EVERY rewrite rather than being a test.
    """
    from collections import Counter
    b = Counter(l.strip() for l in before if l.strip())
    a = Counter(l.strip() for l in after if l.strip())
    for h in dropped:
        for cand in (f"#include <{h}>", f'#include "{h}"'):
            if b[cand]:
                b[cand] -= 1
                if not b[cand]:
                    del b[cand]
                break
    if a != b:
        lost = (b - a)
        gained = (a - b)
        raise SystemExit(
            f"[include-order] ABORT: rewrite of {path} is not line-conserving\n"
            f"   lost:   {list(lost.elements())[:8]}\n"
            f"   gained: {list(gained.elements())[:8]}")


_SUPPLY_CACHE: dict[str, tuple[bool, bool, bool]] = {}
_VENDOR_WIN: set[str] | None = None


def _header_text(name: str) -> str:
    try:
        return (REPO / "include" / name).read_text(encoding="utf-8", errors="replace")
    except OSError:
        return ""


def _supply(name: str, stack=()) -> tuple[bool, bool, bool]:
    """(afx, afxwin, win): does `name` reach each platform surface through its
    OWN includes? Leaning on your own dependencies is ordinary composition; only
    leaning on your INCLUDER is the self-sufficiency defect."""
    global _VENDOR_WIN
    if _VENDOR_WIN is None:
        _VENDOR_WIN = _vendor_win_suppliers()
    if name in AFXWIN_SUPPLIERS:
        return True, True, True
    if name in MFC_SUPPLIERS:
        return True, False, True
    if name in WIN_SUPPLIERS or name in _VENDOR_WIN:
        return False, False, True
    if name not in PROJECT_HEADERS or name in stack:
        return False, False, False
    if name in _SUPPLY_CACHE:
        return _SUPPLY_CACHE[name]
    afx = afxwin = win = False
    for line in _header_text(name).splitlines():
        if m := INC_RE.match(line):
            a, aw, w = _supply(m.group(1), stack + (name,))
            afx, afxwin, win = afx or a, afxwin or aw, win or w
    _SUPPLY_CACHE[name] = (afx, afxwin, win)
    return afx, afxwin, win


def missing_prelude(path: Path, headers) -> list[str]:
    """Preludes a HEADER must pull for itself so group 5 can be last.

    Verified against a standalone-compile sweep of every header (MSVC 5.0,
    2026-08-02): a token satisfied by a forward declaration or an
    elaborated-type-specifier (`class CString* p`) needs no prelude, and supply
    through the header's OWN includes (transitive) is self-sufficiency, not
    leaning. Both are honored below so a flagged header is a REAL defect.
    """
    if path.suffix != ".h" or path.name in PRELUDE_RANK:
        return []
    txt = path.read_text(encoding="utf-8", errors="replace")
    incs = [m.group(1) for m in (INC_RE.match(l) for l in txt.splitlines()) if m]
    afx_ok = any(_supply(h)[0] or h in MFC_SUPPLIERS for h in incs)
    afxwin_ok = any(_supply(h)[1] or h in AFXWIN_SUPPLIERS for h in incs)
    win_ok = afx_ok or any(_supply(h)[2] or h in WIN_SUPPLIERS for h in incs)
    # COMMENTS ARE STRIPPED FIRST. A header that merely NAMES a type in prose -
    # "written as PALETTEENTRY entries[256]", "the CString's initialiser" - does
    # not need that type declared, and scanning comment text for tokens reported
    # exactly those two as missing preludes.
    body = "\n".join(l for l in txt.splitlines() if not INC_RE.match(l))
    body = re.sub(r"/\*.*?\*/", " ", body, flags=re.S)
    body = re.sub(r"//[^\n]*", "", body)
    # names satisfied without any prelude: fwd decls in this file or one hop
    # down its project includes, plus elaborated uses (`struct tagRECT* r`)
    declared = set(FWD_RE.findall(body)) | set(ELAB_RE.findall(body))
    for h in incs:
        if h in PROJECT_HEADERS:
            declared |= set(FWD_RE.findall(_header_text(h)))
    want = []
    afxwin_hits = set(AFXWIN_TOKENS.findall(body)) - declared
    if afxwin_hits and not afxwin_ok:
        want.append("MfcWin.h")
    afx_hits = set(AFX_TOKENS.findall(body)) - declared
    if not want and afx_hits and not afx_ok:
        want.append("Mfc.h")
    # Win32 types come from <Mfc.h>, not <Win32.h>, for MFC-side headers: Mfc.h
    # is a superset (afx.h pulls windows.h) and cannot trip MFC's C1189. A
    # header whose includers are all pure-Win32 TUs keeps <Win32.h> instead
    # (ProcAddr.h and SFSelectDevice.h) - dragging afx into those TUs would
    # change their codegen.
    win_hits = set(WIN_TOKENS.findall(body)) - declared
    if not want and win_hits and not win_ok:
        want.append("Mfc.h")
    return want


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gate", action="store_true")
    ap.add_argument("--fix-dupes", action="store_true")
    ap.add_argument("--fix-prelude", action="store_true")
    ap.add_argument("--fix", action="store_true")
    ap.add_argument("--verbose", "-v", action="store_true")
    args = ap.parse_args(argv)

    dupes, preludes, unordered, manual = {}, {}, [], {}
    changed = 0

    for path in repo_files():
        rel = path.relative_to(REPO).as_posix()
        try:
            head, entries, tail = parse(path)
        except Manual as e:
            manual[rel] = str(e)
            continue
        if not entries:
            continue
        own = own_header(path)
        headers = [h for _, h in entries]

        dropped = [h for i, h in enumerate(headers) if h in headers[:i]]
        if dropped:
            dupes[rel] = sorted(set(dropped))
        want_add = missing_prelude(path, headers)
        if want_add:
            preludes[rel] = want_add

        work = list(entries)
        if args.fix_prelude:
            work.extend(([], h) for h in want_add)
        want = render(head, work, tail, own)
        have = path.read_text(encoding="utf-8", errors="replace").splitlines()
        if want != have:
            if not dropped and not want_add:
                unordered.append(rel)
            do = args.fix or (args.fix_dupes and dropped) or (args.fix_prelude and want_add)
            if do:
                assert_conserved(path, have,
                                 want if not args.fix_prelude else
                                 [l for l in want
                                  if l.strip() not in {f"#include <{h}>" for h in want_add}],
                                 dropped)
                path.write_text("\n".join(want) + "\n", encoding="utf-8")
                changed += 1

    ndupe = sum(len(v) for v in dupes.values())
    print(f"[include-order] duplicate includes:      {ndupe} in {len(dupes)} file(s)")
    print(f"[include-order] headers missing prelude: {len(preludes)}")
    print(f"[include-order] files out of order:      {len(unordered)}")
    print(f"[include-order] MANUAL (untouched):      {len(manual)}")
    if args.verbose:
        for rel, d in sorted(dupes.items()):
            print(f"   dup  {rel}: {', '.join(d)}")
        for rel, w in sorted(preludes.items()):
            print(f"   pre  {rel}: {', '.join(w)}")
        for rel in unordered:
            print(f"   ord  {rel}")
    for rel, why in sorted(manual.items()):
        print(f"   MANUAL {rel}: {why}")

    if changed:
        print(f"[include-order] rewrote {changed} file(s)")
        return 0
    # Preludes are GATED since the 2026-08-02 standalone-compile sweep proved
    # every header self-sufficient: a flagged header is a real leaner (evidence
    # honors fwd decls, elaborated uses and transitive supply. The fix is one
    # verified prelude: <MfcWin.h> for afxwin value types, <Mfc.h> for base MFC
    # types, and <Win32.h> for pure-Win32-side headers).
    if args.gate and (ndupe or unordered or preludes):
        print("[include-order] FATAL: include block is not canonical - fix with "
              "`python -m gruntz.audit.include_order --fix-dupes --fix` "
              "(preludes: add the includer-side prelude by hand)")
        return 1
    if not args.gate:
        return 0
    print("[include-order] OK - deduped, canonical order, every header self-sufficient")
    return 0


if __name__ == "__main__":
    sys.exit(main())
