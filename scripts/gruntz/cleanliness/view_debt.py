#!/usr/bin/env python3
"""gruntz.cleanliness.view_debt - the UNGAMEABLE fake-view metric.

objdiff's match % is gameable by a fake VIEW: declaring a placeholder class with
reloc-masked (declared-only) methods makes a call compile, and reloc-masking hides
the call target from the diff - so % never penalizes it. But the phantom method's
SYMBOL is still there: the base .obj carries an UNDEFINED external for it that is
DEFINED NOWHERE in the corpus (the real function has a different mangled name).

So we read the compiled output directly: llvm-nm every base .obj, and a "phantom"
is a `?Method@CClass@@` symbol undefined in every obj and defined in none. A
PURE-PHANTOM CLASS - >=1 phantom method, ZERO defined method bodies anywhere, no RTTI
vtable, not an MFC/CRT/std library class - is a fake view by construction (a real
class always has some reconstructed body or RTTI). This count CANNOT be gamed:
adding a fake view raises it. Driving it to 0 == removing every fake view.

Real-but-unreconstructed methods on real classes (CGrunt, CPlay, ...) are NOT counted:
those classes have defined bodies and/or RTTI, so only their yet-to-do methods are
undefined - legitimate reloc-masked callees, not fakes.

    python -m gruntz.cleanliness.view_debt            # headline: phantom classes/methods
    python -m gruntz.cleanliness.view_debt --list     # every pure-phantom class + its methods
    python -m gruntz.cleanliness.view_debt --fatal     # exit 1 if any exist (gate)
"""
import glob
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent)
OBJS = REPO / "build/objdiff/base"

# MFC / CRT / std / COM classes whose methods are legitimately external (static-linked
# libraries), never defined in our objs. A declared-only method on these is NOT a fake.
LIBRARY_CLASSES = {
    "CString", "CObject", "CWnd", "CDialog", "CDC", "CGdiObject", "CFile", "CArchive",
    "CException", "CMemoryException", "CFileException", "CNotSupportedException",
    "CPtrList", "CObList", "CObArray", "CStringList", "CPtrArray", "CByteArray", "CDWordArray",
    "CMapStringToOb", "CMapStringToPtr", "CMapPtrToPtr", "CList", "CMap", "CArray",
    "CRgn", "CBitmap", "CFont", "CPen", "CBrush", "CPalette", "CMenu", "CImageList",
    "CTime", "CTimeSpan", "CPoint", "CRect", "CSize", "CRuntimeClass", "CCmdTarget",
    "CWinApp", "CWinThread", "CFrameWnd", "CView", "CDocument", "CControlBar",
    "CStatic", "CButton", "CEdit", "CListBox", "CComboBox", "CScrollBar",
}


def _class_of(sym):
    """The owning class of an MSVC-mangled MEMBER FUNCTION: ?Method@CClass@@Q... -> CClass.
    Requires the `@@` that ends the qualified name AND a member-function code after it
    ([A-Z], e.g. Q/A/I/U/M/E) - NOT a digit. This excludes:
      - global operators whose "class" is a type-signature fragment (`??2@YAPAXI@Z` = operator
        new -> the YAPAXI is the PAX/I return+arg mangle, not a class);
      - DATA symbols (`?g_x@NS@@3PAX...` = a variable, `@@3`) - a phantom is an untied METHOD,
        not a data extern.
    None for non-member / C / data / operator-fragment symbols."""
    # Anchor on the qualified name: ?MethodName (@Component)+ @@ <storage-code>. The code
    # RIGHT AFTER the owning class-chain's `@@` must be a member-function access letter
    # ([A-Z]: Q/A/I/U/M/E...), NOT a digit (`@@3` = a DATA member/global, not a method) and
    # not absent (a global operator like `??2@YAPAXI@Z` - the `@YAPAXI@` is a return-type
    # mangle, no `@@`). Returns the INNERMOST (owning) class for nested names.
    m = re.match(r"\?[^@]+((?:@[A-Za-z_]\w*)+)@@([A-Za-z])", sym)
    return m.group(1).split("@")[1] if m else None


def _current_objs():
    """Base objs the CURRENT build actually produces. Orphan objs left on disk by a
    renamed/merged unit carry STALE symbols that would inflate the count (and break the
    hard-assert), so read the produced set from build.ninja and intersect with disk."""
    ninja = REPO / "build.ninja"
    on_disk = sorted(glob.glob(str(OBJS / "*.obj")))
    if not ninja.exists():
        return on_disk
    produced = set(re.findall(r"build (build/objdiff/base/\S+\.obj):", ninja.read_text()))
    cur = [o for o in on_disk if str(Path(o).relative_to(REPO)) in produced]
    return cur or on_disk


def _nm_symbols():
    """(defined, undefined) sets of function symbols across all current base objs."""
    defined, undefined = set(), set()
    for o in _current_objs():
        out = subprocess.run(["llvm-nm", o], capture_output=True, text=True).stdout
        for ln in out.splitlines():
            p = ln.split()
            if len(p) == 2 and p[0] == "U":
                undefined.add(p[1])
            elif len(p) == 3 and p[1] in "Tt":
                defined.add(p[2])
    return defined, undefined


def _rtti_classes():
    from gruntz.analysis import vtable_scan as vs
    out = set()
    for v in vs.VTABLES:
        m = re.search(r"V([A-Za-z_]\w*)@", v.get("decorated") or "")
        if m:
            out.add(m.group(1))
    return out


def pure_phantom_classes():
    """{class -> [phantom method symbols]} for fake-view classes (0 defined, no RTTI)."""
    defined, undefined = _nm_symbols()
    never = undefined - defined
    rtti = _rtti_classes()
    defined_cls = {c for c in (_class_of(s) for s in defined if s.startswith("?")) if c}
    phantom = defaultdict(list)
    for s in never:
        if not s.startswith("?"):
            continue                       # C symbol / __imp_ / global - not a method
        c = _class_of(s)
        if not c or c in LIBRARY_CLASSES or c.startswith("std"):
            continue
        if c in defined_cls or c in rtti:  # real class: has a body or RTTI somewhere
            continue
        phantom[c].append(s)
    return phantom


def main():
    args = sys.argv[1:]
    if not OBJS.is_dir() or not any(OBJS.glob("*.obj")):
        print("view-debt: no base objs (run `gruntz build` first)")
        return 0
    phantom = pure_phantom_classes()
    nclass = len(phantom)
    nmeth = sum(len(v) for v in phantom.values())
    print(f"view debt: {nclass} pure-phantom class(es), {nmeth} declared-only method(s) "
          f"defined nowhere (fake views - ungameable metric; drive to 0)")
    if "--list" in args:
        for c in sorted(phantom, key=lambda c: -len(phantom[c])):
            print(f"  {c}  ({len(phantom[c])})")
            for s in sorted(phantom[c]):
                print(f"      {s}")
    if "--fatal" in args and nclass:
        print("view-debt: FAIL (--fatal) - fake-view classes present", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
