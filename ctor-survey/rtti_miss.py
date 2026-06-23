#!/usr/bin/env python3
"""How many RTTI (polymorphic) classes still lack a size? Denominator = RTTI
classes with a vftable (vtables.csv); split game/engine vs MFC/CRT library."""
import csv, re
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
VTABS = REPO / "ctor-survey/vtables.csv"
NEW = REPO / "ctor-survey/new_sites.csv"

LIB = set("""CWinApp CWinThread CCmdTarget CCmdUI CTestCmdUI CCommandLineInfo
CRecentFileList CNoTrackObject CObject CWnd CFrameWnd CDialog CView CCtrlView
CScrollView CDocument CDC CClientDC CPaintDC CWindowDC CGdiObject CBrush CPen CRgn
CMenu CImageList CImage CButton CComboBox CEdit CStatic CListBox CDragListBox
CScrollBar CSliderCtrl CSpinButtonCtrl CProgressCtrl CHeaderCtrl CHotKeyCtrl
CListCtrl CTabCtrl CTreeCtrl CRichEditCtrl CAnimateCtrl CStatusBarCtrl CToolBarCtrl
CFile CMemFile CMirrorFile CArchiveStream CByteArray CDWordArray CStringArray
CPtrArray CObArray CStringList CPtrList CObList CMapStringToOb CMapStringToPtr
CMapPtrToPtr CException CArchiveException CFileException CMemoryException
CNotSupportedException CResourceException CSimpleException CUserException CThreadData
_AFX_BASE_MODULE_STATE _AFX_CTL3D_STATE _AFX_CTL3D_THREAD AFX_MODULE_STATE
AFX_MODULE_THREAD_STATE _AFX_THREAD_STATE _AFX_WIN_STATE type_info ios iostream
istream ostream istream_withassign ostream_withassign istrstream ostrstream strstream
filebuf fstream ifstream ofstream streambuf strstreambuf IUnknown IStream
ISequentialStream""".split())
BASES = {"CUserBase", "CUserLogic", "CState", "CObject", "CWapObj", "CWapX"}

rtti = {r["class"] for r in csv.DictReader(open(VTABS)) if "+" not in r["class"]
        and not r["class"].startswith(".?A")}      # drop template-mangled
game = sorted(c for c in rtti if c not in LIB)

clean, tokens = set(), set()
for r in csv.DictReader(open(NEW)):
    if r["method"] in ("vtable", "ctor") and r["size_kind"] == "const" and r["class"]:
        parts = r["class"].split("+")
        tokens.update(parts)
        if len(parts) == 1:
            clean.add(parts[0])

game_clean = [c for c in game if c in clean]
game_composite = [c for c in game if c not in clean and c in tokens]
game_missing = [c for c in game if c not in tokens]

print(f"RTTI classes with a vftable        : {len(rtti)}")
print(f"  game/engine                       : {len(game)}   (library excluded: {len(rtti)-len(game)})")
print(f"    sized cleanly                    : {len(game_clean)}")
print(f"    only via composite (base-ish)    : {len(game_composite)}  {game_composite}")
print(f"    NO size at all (still missing)   : {len(game_missing)}")
print()
print("game/engine RTTI classes still missing a size:")
for c in game_missing:
    print(f"  {c}")
