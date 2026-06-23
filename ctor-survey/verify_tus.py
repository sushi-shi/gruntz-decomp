#!/usr/bin/env python3
"""Compile (clang -fsyntax-only) every TU that now carries a SIZE() we added,
forcing the worktree include/ to win. Report PASS / FAIL, and for FAILs pull the
'evaluates to A == B' note so we see actual-vs-expected size."""
import json, subprocess, shlex, re
from pathlib import Path

WT = Path.cwd()
cc = {Path(e["file"]).resolve(): e for e in json.load(open("build/clangd/compile_commands.json"))}

# TUs to check: every src file containing a SIZE( ... ) plus the stubs aggregator
files = set()
for p in (WT / "src").rglob("*.cpp"):
    t = p.read_text(errors="ignore")
    if re.search(r"\bSIZE\(", t):
        files.add(p.resolve())
# stubs compile via All.cpp, not individually
allcpp = (WT / "src/Stub/All.cpp").resolve()
files = {f for f in files if "src/Stub/" not in str(f)} | {allcpp}

def compile_one(f):
    e = cc.get(f)
    if not e:
        return ("NOCC", "")
    args = e.get("arguments") or shlex.split(e["command"])
    out = [args[0], "-fsyntax-only", f"-I{WT}/include"]
    skip = False
    for a in args[1:]:
        if skip: skip = False; continue
        if a in ("-o", "-c"): skip = (a == "-o"); continue
        if a.endswith(".cpp"): continue
        out.append(a)
    out.append(str(f))
    r = subprocess.run(out, cwd=e.get("directory", str(WT)), capture_output=True, text=True)
    return ("PASS" if r.returncode == 0 else "FAIL", r.stderr)

for f in sorted(files):
    status, err = compile_one(f)
    rel = f.relative_to(WT) if str(f).startswith(str(WT)) else f
    print(f"{status:5} {rel}")
    if status == "FAIL":
        for m in re.finditer(r"static assertion failed.*?'sizeof\(([^)]+)\) == \((\w+)\)'", err):
            print(f"        FAILED SIZE: {m.group(1)} expected 0x... ")
        for m in re.finditer(r"expression evaluates to '(\d+) == (\d+)'", err):
            a, b = int(m.group(1)), int(m.group(2))
            print(f"        actual 0x{a:x} vs expected 0x{b:x}  (short by 0x{b-a:x})" if b > a else f"        actual 0x{a:x} vs expected 0x{b:x}")
        # show first hard error if not a SIZE assert
        e1 = re.search(r"error: (?!static assertion).*", err)
        if e1: print("        ERR:", e1.group(0)[:140])
