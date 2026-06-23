#!/usr/bin/env python3
"""Set SIZE() for every vtable-clean game-class size not yet set, compile-driven.

  pass 1: append SIZE(X, N) to X's host .cpp (where its ctor lives / its stub).
  pass 2: compile; for each failing SIZE read clang's actual size A, pad X's def by
          the shortfall - full N for an empty stub (A==1), remainder N-A otherwise.
  pass 3: re-verify.

list mode (default) prints the plan; --write applies it.
"""
import csv, re, json, subprocess, shlex, sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
SRC = REPO / "src"
NEW = REPO / "ctor-survey/new_sites.csv"
CCMAP = {Path(e["file"]).resolve(): e for e in json.load(open(REPO / "build/clangd/compile_commands.json"))}
WRITE = "--write" in sys.argv

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
type_info ios iostream istream ostream istream_withassign ostream_withassign
istrstream ostrstream strstream filebuf fstream ifstream ofstream streambuf
strstreambuf IUnknown IStream ISequentialStream zPTree""".split())

sizes = {}
for r in csv.DictReader(open(NEW)):
    if r["method"] in ("vtable", "ctor") and r["size_kind"] == "const" and r["class"] and "+" not in r["class"]:
        sizes.setdefault(r["class"], set()).add(int(r["sizeof"], 16))
targets = {c: next(iter(s)) for c, s in sizes.items() if len(s) == 1 and c not in LIB}

have = {m.group(1) for p in SRC.rglob("*.cpp")
        for m in re.finditer(r"SIZE\(\s*(?:\w+::)?(\w+)\s*,", p.read_text(errors="ignore"))}

rp, dp = re.compile(r"RVAU?\(\s*0x[0-9a-fA-F]+"), re.compile(r"\b([A-Za-z_]\w*)\s*::\s*~?\w+\s*\(")
host_by_class = {}
for p in SRC.rglob("*.cpp"):
    L = p.read_text(errors="ignore").splitlines()
    for i, ln in enumerate(L):
        if rp.search(ln):
            for j in range(i, min(i + 4, len(L))):
                d = dp.search(L[j])
                if d: host_by_class.setdefault(d.group(1), p); break

ALLH = list(SRC.rglob("*.cpp")) + list(SRC.rglob("*.h")) + list((REPO / "include").rglob("*.h"))
def find_def(cls):
    for p in ALLH:
        if re.search(r"\bclass\s+" + re.escape(cls) + r"\b[^{;]*\{", p.read_text(errors="ignore")):
            return p
    return None

def host_of(cls):
    if cls in host_by_class: return host_by_class[cls]
    stub = SRC / "Stub" / f"{cls}.cpp"
    return stub if stub.exists() else None

def class_close(text, cls):
    m = re.search(r"\bclass\s+" + re.escape(cls) + r"\b[^{;]*\{", text)
    if not m: return None
    i = text.index("{", m.start()); depth = 0
    while i < len(text):
        depth += (text[i] == "{") - (text[i] == "}")
        if depth == 0: return i
        i += 1
    return None

def compile_tu(f):
    e = CCMAP.get(f.resolve())
    if not e: return (None, "")
    args = e.get("arguments") or shlex.split(e["command"])
    out = [args[0], "-fsyntax-only", f"-I{REPO}/include"]
    skip = False
    for a in args[1:]:
        if skip: skip = False; continue
        if a in ("-o", "-c"): skip = (a == "-o"); continue
        if a.endswith(".cpp"): continue
        out.append(a)
    out.append(str(f))
    r = subprocess.run(out, cwd=e.get("directory", str(REPO)), capture_output=True, text=True)
    return (r.returncode == 0, r.stderr)

def is_empty(text, cls):
    """True if class X has no data members (only method decls) -> sizeof 1."""
    m = re.search(r"\bclass\s+" + re.escape(cls) + r"\b[^{;]*\{", text)
    if not m: return True
    i = text.index("{", m.start()); depth = 0; j = i
    while j < len(text):
        depth += (text[j] == "{") - (text[j] == "}")
        if depth == 0: break
        j += 1
    for ln in text[i+1:j].splitlines():
        s = ln.split("//")[0].strip()
        if not s or s.endswith(":") or "(" in s or "{" in s or "}" in s: continue
        if s.endswith(";") and not s.startswith(("using", "typedef", "friend", "static", "public", "private", "protected")):
            return False
    return True

PAD_RE = re.compile(r"[ \t]*char m_size_pad\[0x[0-9a-f]+\]; // pad to size 0x[0-9a-f]+ \(gruntz\.analysis\.news\); layout TBD\n")
settable = [(c, targets[c], host_of(c), find_def(c)) for c in sorted(targets)]
ok = [(c, n, h, d) for c, n, h, d in settable if h and d]
bad = [(c, n, h, d) for c, n, h, d in settable if not (h and d)]

if not WRITE:
    for c, n, h, d in settable:
        st = "ok" if h and d else ("no-host" if not h else "no-def")
        print(f"{c:30} 0x{n:<5x} {st}")
    print(f"\n{len(ok)} settable, {len(bad)} need attention, {len(targets)-len(settable)} already set.")
    sys.exit(0)

allcpp = SRC / "Stub/All.cpp"
# idempotent: strip any pad this script added before, and (re)append SIZE
for p in set(d for _, _, _, d in ok):
    t = p.read_text(); t2 = PAD_RE.sub("", t)
    if t2 != t: p.write_text(t2)
tus = set()
for c, n, h, d in ok:
    t = h.read_text()
    if not re.search(r"SIZE\(\s*" + re.escape(c) + r"\s*,", t):
        h.write_text((t if t.endswith("\n") else t + "\n") +
                     f"// size 0x{n:x} from operator-new vtable attribution (gruntz.analysis.news)\nSIZE({c}, 0x{n:x});\n")
    tus.add(allcpp if "src/Stub/" in str(h) else h)

# empties pad to full N deterministically; has-fields need the compiled actual
empties = [(c, n, d) for c, n, h, d in ok if is_empty(d.read_text(), c)]
filled = [(c, n, h, d) for c, n, h, d in ok if not is_empty(d.read_text(), c)]

# actual sizes for has-fields classes: compile their TUs, pair class<->size by the
# nearest preceding requirement to each 'evaluates to' note (robust to multi-fail).
actual, ne_tus = {}, set(allcpp if "src/Stub/" in str(h) else h for c, n, h, d in filled)
for tu in ne_tus:
    _, err = compile_tu(tu)
    reqs = [(m.end(), m.group(1), int(m.group(2))) for m in re.finditer(r"sizeof\((\w+)\) == \((\d+)\)", err)]
    for m in re.finditer(r"evaluates to '(\d+) == (\d+)'", err):
        a, nn, ep = int(m.group(1)), int(m.group(2)), m.start()
        cand = [r for r in reqs if r[0] < ep and r[2] == nn]
        if cand: actual[max(cand)[1]] = a

padded = 0
for c, n, d in empties:
    t = d.read_text(); cc = class_close(t, c)
    if cc is None: continue
    d.write_text(t[:cc] + f"    char m_size_pad[0x{n:x}]; // pad to size 0x{n:x} (gruntz.analysis.news); layout TBD\n" + t[cc:])
    padded += 1
for c, n, h, d in filled:
    a = actual.get(c)
    if a is None or a >= n: continue                  # passed as-is, or already >= target
    t = d.read_text(); cc = class_close(t, c)
    if cc is None: continue
    d.write_text(t[:cc] + f"    char m_size_pad[0x{n-a:x}]; // pad to size 0x{n:x} (gruntz.analysis.news); layout TBD\n" + t[cc:])
    padded += 1

print(f"set {len(ok)} sizes ({len(empties)} empty-stub, {len(filled)} has-fields); padded {padded}; "
      f"skipped {len(bad)} (no host/def: {[b[0] for b in bad]})")
fails = 0
for tu in sorted(tus):
    okc, err = compile_tu(tu)
    if not okc:
        fails += 1
        print(f"FAIL {tu.relative_to(REPO)}")
        for e1 in re.findall(r"error: .*", err)[:3]:
            print("   ", e1[:150])
print(f"\nre-verify: {len(tus)-fails}/{len(tus)} TUs PASS")
