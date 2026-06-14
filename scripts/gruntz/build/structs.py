#!/usr/bin/env python3
"""gen_structs.py - derive Ghidra struct + enum definitions from the source.

The single source of truth for a class layout is its *compilable* declaration in
`src/**/*.h` (matched, authoritative) and the converted `structure/**/*.h`
(unmatched comprehension) - both written in the placeholder style
(`int`/`void*`/`char[]` members, explicit padding, real bases + `virtual`). There
are NO `@offset` annotations: clang computes every field offset from the
declaration. This script runs clang as a *reader* and emits the JSON that
`apply_ghidra_enrichment.py` consumes, replacing its hand-written `STRUCTS`/`ENUMS`
literals (docs/source-consolidation-investigation.md, step 1).

For each translation unit:
  * `clang ... -Xclang -fdump-record-layouts-complete -fsyntax-only`
        -> the absolute offset + type + name of every field, flattened across
           bases, plus the total `sizeof` (the MS-ABI layout, target
           i686-pc-windows-msvc / -fms-compatibility-version=1100 = MSVC 5.0).
  * `clang ... -Xclang -ast-dump=json -fsyntax-only`
        -> enum definitions (name + members) and, when present, the source path
           of each record so `src/` can win over `structure/` on overlap.

Outputs (default --out-dir build/gen):
    structs.json : [{name, size, fields:[{offset,type,name}], source}]
    enums.json   : [{name, members:[{name,value}], source}]

Flags come from the clangd compilation database (build/clangd/compile_commands.json,
the clang-friendly clang-cl DB from gen_clangd.py) when available; those entries are
clang-cl form (/imsvc, /D...), so they run under clang's `--driver-mode=cl`. The
`--tu`/`--header` paths carry clang-style MS_FLAGS and use the plain driver. Pass
`--tu FILE` + `--flag ...` directly for self-contained TUs that parse with only the
target/ms flags, no toolchain headers.
"""

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO = next((p for p in SCRIPT_DIR.parents if (p / "flake.nix").exists()), SCRIPT_DIR)

# Mirror gen_clangd.py so clang emulates the real compiler as closely as it can.
TARGET = "i686-pc-windows-msvc"
MSC_COMPAT = "1100"  # MSVC 5.0 == cl 11.00 == _MSC_VER 1100
MS_FLAGS = [f"--target={TARGET}", f"-fms-compatibility-version={MSC_COMPAT}",
            "-fms-extensions"]

# Record-layout markers that are NOT data fields (skip them; keep their offset
# only to recognise the line). Examples from -fdump-record-layouts:
#   "0 | class CFileIO"                 record header
#   "0 |   class CObject (primary base)" base subobject
#   "0 |     (CObject vftable pointer)"  vptr slot
# Base subobjects / vptr slots are not data fields. Their *fields* (deeper
# nesting) ARE flattened into the derived class; an embedded record-typed MEMBER,
# by contrast, is one field whose interior we must skip.
_LAYOUT_NONFIELD = re.compile(
    r"\b(primary base|virtual base|\(base\)|vftable pointer|vbtable pointer)\b")
# capture (offset, indent-width, decl) so we can track nesting depth.
_LAYOUT_FIELD = re.compile(r"^\s*(\d+)\s*\|(\s+)(\S.*\S|\S)\s*$")
_SIZEOF = re.compile(r"\[sizeof=(\d+)")
_RECORD_TYPE = re.compile(r"^(?:class|struct|union)\s+")
# compiler-synthesised records we never want to emit.
_BUILTIN_RE = re.compile(r"^(__|_GUID$|type_info$)")


def log(msg):
    print(f"[gen_structs] {msg}", file=sys.stderr)


def split_field(decl):
    """'void * m_handle' -> ('void *','m_handle'); 'int m_open' -> ('int','m_open').

    The field name is the trailing identifier; the type is everything before it.
    clang prints array fields as e.g. 'char[64] _pad' -> ('char[64]','_pad').
    """
    m = re.match(r"^(.*?)([A-Za-z_]\w*)\s*$", decl)
    if not m:
        return (decl, None)
    typ = m.group(1).strip()
    name = m.group(2)
    if not typ:                       # a bare type with no member name -> skip
        return (decl, None)
    return (typ, name)


def parse_record_layouts(text):
    """Parse `-fdump-record-layouts(-complete)` text into {name: (size, fields)}.

    fields is a list of (offset, type, name) at ABSOLUTE offsets. Base subobject
    fields are flattened in (they nest under a "(... base)" line but carry their
    real offset); an embedded record-typed *member* is kept as a single field and
    its interior is skipped (tracked by indentation depth).
    """
    records = {}
    cur_name = None
    cur_fields = []
    in_block = False
    skip_below = None     # indent depth: skip lines nested deeper than this
    for line in text.splitlines():
        if "Dumping AST Record Layout" in line:
            cur_name, cur_fields, in_block, skip_below = None, [], True, None
            continue
        if not in_block:
            continue
        sz = _SIZEOF.search(line)
        if sz and cur_name is not None:
            records.setdefault(cur_name, (int(sz.group(1)), cur_fields))
            cur_name, cur_fields, in_block, skip_below = None, [], False, None
            continue
        m = _LAYOUT_FIELD.match(line)
        if not m:
            continue
        off, indent, decl = int(m.group(1)), len(m.group(2)), m.group(3)
        if skip_below is not None:
            if indent > skip_below:
                continue          # inside an embedded member's interior
            skip_below = None
        # record header line: "class/struct/union NAME" with no member name.
        if cur_name is None:
            hm = re.match(r"^(?:class|struct|union)\s+([\w:<>]+)\s*$", decl)
            if hm:
                cur_name = hm.group(1)
            continue
        if _LAYOUT_NONFIELD.search(decl):
            continue              # base subobject / vptr: flatten its fields in
        typ, name = split_field(decl)
        if name is None:
            continue
        # embedded record-typed member -> one field; skip its nested interior.
        if _RECORD_TYPE.match(typ):
            typ = _RECORD_TYPE.sub("", typ).strip()
            skip_below = indent
        cur_fields.append((off, typ, name))
    return records


def walk_ast(node, want, out):
    """Depth-first walk of clang -ast-dump=json collecting nodes of `want` kinds."""
    if isinstance(node, dict):
        if node.get("kind") in want:
            out.append(node)
        for child in node.get("inner", []) or []:
            walk_ast(child, want, out)


def parse_enums(ast_json):
    """Return [{name, members:[{name,value}]}] from an -ast-dump=json tree."""
    enums = []
    nodes = []
    walk_ast(ast_json, {"EnumDecl"}, nodes)
    for en in nodes:
        name = en.get("name")
        if not name:
            continue
        members = []
        val = 0
        for c in en.get("inner", []) or []:
            if c.get("kind") != "EnumConstantDecl":
                continue
            # clang emits an explicit initializer child when present; otherwise
            # the value is previous+1 (C enum rule).
            init = [k for k in (c.get("inner") or [])
                    if k.get("kind") not in ("FullComment",)]
            ev = None
            for k in init:
                t = (k.get("value") or k.get("valueCategory"))
                if k.get("kind") in ("ConstantExpr", "IntegerLiteral") and "value" in k:
                    ev = int(k["value"])
                    break
            if ev is None:
                ev = val
            members.append({"name": c["name"], "value": ev})
            val = ev + 1
        enums.append({"name": name, "members": members})
    return enums


def run_clang(clang, args, tu, extra, driver="plain"):
    # compdb flags are clang-cl form (/imsvc, /D...): the plain clang driver
    # treats "/imsvc" as an input file ("no such file or directory: '/imsvc'"),
    # so those units must run in cl driver mode. The structure/ header wrappers
    # carry clang-style MS_FLAGS and use the plain driver.
    pre = ["--driver-mode=cl"] if driver == "cl" else []
    cmd = [clang, *pre, *args, tu, "-fsyntax-only", *extra]
    res = subprocess.run(cmd, capture_output=True, text=True)
    # Layout dump goes to stdout; diagnostics to stderr. Return both so a parse
    # over stdout works and we can surface hard errors.
    return res.stdout, res.stderr, res.returncode


def compdb_entries(path):
    db = json.loads(Path(path).read_text())
    out = []
    for e in db:
        args = e.get("arguments") or []
        # drop the driver (clang-cl) and the /c + source; we re-add our own.
        flags = [a for a in args[1:] if a not in ("/c",) and a != e.get("file")]
        out.append((e["file"], flags))
    return out


def header_units(globs):
    """Wrap each .h under `globs` (dirs or globs) in a one-line .cpp TU.

    gen_structs needs C++ TUs (a bare .h parses as C and `extern "C"` fails), and
    a single combined TU clashes on the placeholder MFC types each comprehension
    header defines locally - so each header gets its OWN wrapper .cpp, processed
    as a separate clang invocation. Returns (wrapper_cpp, MS_FLAGS, header) triples.
    """
    import glob as _glob
    import tempfile
    headers = []
    for g in globs:
        p = Path(g)
        headers += (sorted(str(h) for h in p.rglob("*.h")) if p.is_dir()
                    else sorted(_glob.glob(g, recursive=True)))
    if not headers:
        return []
    tmp = Path(tempfile.mkdtemp(prefix="gruntz-structwrap-"))
    out = []
    for i, h in enumerate(headers):
        hp = Path(h).resolve()
        w = tmp / ("%03d_%s.cpp" % (i, hp.stem))
        w.write_text('#include "%s"\n' % hp)
        out.append((str(w), list(MS_FLAGS), h))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--clang", default=os.environ.get("GRUNTZ_CLANG") or "clang",
                    help="UNWRAPPED clang driver ($GRUNTZ_CLANG in the dev shell).")
    ap.add_argument("--compdb", default=str(REPO / "build/clangd/compile_commands.json"))
    ap.add_argument("--tu", action="append", default=[],
                    help="explicit TU(s) to read (bypasses --compdb).")
    ap.add_argument("--flag", action="append", default=[],
                    help="extra clang flag(s) used with --tu (repeatable).")
    ap.add_argument("--header", action="append", default=[],
                    help="comprehension header dir/glob (e.g. structure/): each .h "
                         "is wrapped in a one-line .cpp TU and laid out with MS flags. "
                         "Combine with --compdb/--tu; src/ wins on overlapping names.")
    ap.add_argument("--out-dir", default=str(REPO / "build/gen"))
    args = ap.parse_args()

    # units: (tu_path, clang_flags, source_label, driver). compdb TUs carry
    # clang-cl flags -> "cl" driver; --tu and structure/ header wrappers carry
    # clang-style MS_FLAGS -> "plain" driver.
    units = []
    if args.tu:
        units += [(tu, [*MS_FLAGS, *args.flag], tu, "plain") for tu in args.tu]
    elif Path(args.compdb).exists():
        units += [(tu, flags, tu, "cl") for (tu, flags) in compdb_entries(args.compdb)]
    units += [(w, flags, h, "plain") for (w, flags, h) in header_units(args.header)]
    if not units:
        ap.error(f"no --tu/--header given and no compdb at {args.compdb}")

    structs = {}   # name -> {name,size,fields,source}; src/ wins over structure/
    enums = {}
    for tu, flags, source, driver in units:
        src_priority = 0 if "/src/" in source or source.startswith("src/") else 1
        lo_out, lo_err, lo_rc = run_clang(
            args.clang, flags, tu, ["-Xclang", "-fdump-record-layouts-complete"], driver)
        if lo_rc != 0 and not lo_out:
            log(f"WARN {source}: layout dump failed rc={lo_rc}\n{lo_err.strip()[:400]}")
        for name, (size, fields) in parse_record_layouts(lo_out).items():
            if _BUILTIN_RE.match(name):
                continue
            rec = {"name": name, "size": size,
                   "fields": [{"offset": o, "type": t, "name": n}
                              for (o, t, n) in fields],
                   "source": source}
            prev = structs.get(name)
            # keep the higher-priority (src/) definition; else the larger one.
            if prev is None or src_priority < prev["_p"] or (
                    src_priority == prev["_p"] and size >= prev["size"]):
                rec["_p"] = src_priority
                structs[name] = rec

        ast_out, ast_err, ast_rc = run_clang(
            args.clang, flags, tu, ["-Xclang", "-ast-dump=json"], driver)
        if ast_out.strip():
            try:
                tree = json.loads(ast_out)
            except json.JSONDecodeError:
                tree = None
            if tree:
                for en in parse_enums(tree):
                    en["source"] = source
                    enums.setdefault(en["name"], en)

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    slist = sorted(({k: v for k, v in r.items() if k != "_p"}
                    for r in structs.values()), key=lambda r: r["name"])
    elist = sorted(enums.values(), key=lambda r: r["name"])
    (out_dir / "structs.json").write_text(json.dumps(slist, indent=2))
    (out_dir / "enums.json").write_text(json.dumps(elist, indent=2))
    log(f"wrote {len(slist)} struct(s), {len(elist)} enum(s) -> {out_dir}")
    for r in slist:
        log(f"  struct {r['name']}  size=0x{r['size']:x}  fields={len(r['fields'])}")


if __name__ == "__main__":
    main()
