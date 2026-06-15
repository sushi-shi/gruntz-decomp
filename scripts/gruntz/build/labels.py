#!/usr/bin/env python3
"""gen_labels.py - derive the rva -> mangled-name, unit label map from source.

Replaces the hand-maintained config/symbol_names.csv. The only manual input is
the `// @address: 0x....` comment above each matched function; everything else is
derived (docs/source-consolidation-investigation.md, §4 / step 2):

  1. clang -ast-dump=json --target=i686-pc-windows-msvc -fms-compatibility-version=1100
     -> every function DEFINITION (a decl with a body) and its MS-ABI mangledName,
        with its source byte-offset (-> line).
  2. scan the .cpp for `@address` (and optional `@symbol`) comments + their lines.
  3. JOIN: the @address comment at line L belongs to the nearest definition whose
     line is just below L. -> (rva, candidate mangledName).
  4. AUTHORITY CHECK: keep the mangledName iff it is a symbol in the base
     <unit>.obj (llvm-nm). A member is, by construction, the unique symbol for
     that function; a miss falls back to an explicit `@symbol` or is reported.
        (clang's MS mangling reproduced the real VC5 symbols in the spike, but is
         not contractually VC5 - the nm membership check is what makes it safe.)
  5. unit comes from config/units.toml via the source path.

Output: build/gen/symbol_names.csv  (rva,name,unit,size) - drop-in for synth_pdb.
The `size` column is the `// @size: 0xNN` byte extent from the same comment block
(empty if absent); synth_pdb uses it as the authoritative boundary for matched
functions Ghidra's auto-analysis never recovered, so a single build is
self-contained (no ghidra-refresh round-trip needed to delink them).

Without --obj the authority check is skipped (candidate names emitted with a
WARN) - useful for inspecting clang's mangling against config/symbol_names.csv
before base objs exist.
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

TARGET = "i686-pc-windows-msvc"
MSC_COMPAT = "1100"
MS_FLAGS = [f"--target={TARGET}", f"-fms-compatibility-version={MSC_COMPAT}",
            "-fms-extensions"]

ADDR_RE = re.compile(r"@address:\s*(0x[0-9a-fA-F]+)")
SYM_RE = re.compile(r"@symbol:\s*(\S+)")
SIZE_RE = re.compile(r"@size:\s*(0x[0-9a-fA-F]+|\d+)")
FUNC_KINDS = {"FunctionDecl", "CXXMethodDecl", "CXXConstructorDecl",
              "CXXDestructorDecl", "CXXConversionDecl"}


def log(msg):
    print(f"[gen_labels] {msg}", file=sys.stderr)


def line_index(text):
    """offsets -> 1-based line. Returns a function offset->line."""
    starts = [0]
    for i, ch in enumerate(text):
        if ch == "\n":
            starts.append(i + 1)
    import bisect
    return lambda off: bisect.bisect_right(starts, off)


def collect_defs(ast, want=FUNC_KINDS):
    """Yield (mangledName, offset) for every function DEFINITION (has a body).

    A definition's AST node has a CompoundStmt among its children. We take the
    node's loc.offset (the name location) to place it against @address comments.
    """
    out = []

    def visit(node):
        if isinstance(node, dict):
            # Skip compiler-synthesised members (implicit copy/default ctors of
            # helper structs) - clang gives them an implicit CompoundStmt body
            # that would otherwise be mistaken for a real source definition.
            if (node.get("kind") in want and "mangledName" in node
                    and not node.get("isImplicit")):
                inner = node.get("inner") or []
                has_body = any(isinstance(c, dict) and c.get("kind") == "CompoundStmt"
                               for c in inner)
                loc = node.get("loc") or {}
                off = loc.get("offset")
                if has_body and off is not None:
                    out.append((node["mangledName"], off))
            for c in node.get("inner", []) or []:
                visit(c)
    visit(ast)
    return out


def scan_annotations(text):
    """Return [(rva, line, symbol_override_or_None, size_or_None)].

    `@address` and the optional `@symbol`/`@size` associate only when they share
    the same contiguous `//` comment block (so one block's @symbol/@size never
    leaks onto the next block's @address across an intervening code line). `@size`
    is the exact byte extent of the matched function (the authoritative boundary
    the synth-PDB falls back on for functions Ghidra never recovered as objects).
    """
    out = []
    lines = text.splitlines()
    n = len(lines)
    i = 0
    while i < n:
        if not lines[i].lstrip().startswith("//"):
            i += 1
            continue
        j = i
        block_addrs = []
        block_sym = None
        block_size = None
        while j < n and lines[j].lstrip().startswith("//"):
            ma = ADDR_RE.search(lines[j])
            if ma:
                block_addrs.append((int(ma.group(1), 16), j + 1))
            ms = SYM_RE.search(lines[j])
            if ms:
                block_sym = ms.group(1)
            mz = SIZE_RE.search(lines[j])
            if mz:
                s = mz.group(1)
                block_size = int(s, 16) if s.lower().startswith("0x") else int(s)
            j += 1
        for rva, ln in block_addrs:
            out.append((rva, ln, block_sym, block_size))
        i = j
    return out


def nm_symbols(obj, nm="llvm-nm"):
    """Defined CODE symbols (functions) in the obj.

    Restricted to text/code symbol types (T/t/W/w) so data artifacts - notably
    clang's `@4HA` static-init guards that demangle to the same `~Class` identity
    as the real destructor - don't pollute the function-symbol set or make the
    dtor undname-fallback ambiguous. .rdata/.data symbols are a separate concern
    (symbols.csv), not function labels.
    """
    res = subprocess.run([nm, "--defined-only", obj],
                         capture_output=True, text=True)
    syms = set()
    for line in res.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 2 and len(parts[-2]) == 1 and parts[-2] in "TtWw":
            syms.add(parts[-1])
    return syms


# A destructor-variant operator code: ??_D (vbase), ??_E (vector deleting),
# ??_G (scalar deleting), ??1 (the real one). clang's AST reports ??_D for an
# out-of-line dtor definition, so the candidate misses the obj.
_DTOR_CAND = re.compile(r"^\?\?(?:_[DEG]|1)([\w@?$]+?@@)")


def plain_dtor_symbol(candidate, obj_syms):
    """clang's dtor candidate (e.g. `??_DCFileIO@@QAEXXZ`) misses the real `??1`.

    Resolve it by matching the CANONICAL plain-destructor mangling
    `??1<class>@@<quals>@XZ` directly in the obj's code symbols. Matching the
    mangled string (not the demangling) excludes clang's EH funclet artifacts
    (`?dtor$5@?0???1...@4HA`, `___ehhandler$??1...`) and the deleting-dtor thunks,
    which all *demangle* to the same `~Class` identity but are not `??1…@XZ`.
    Returns the unique symbol or None.
    """
    m = _DTOR_CAND.match(candidate)
    if not m:
        return None
    classpart = m.group(1)                      # e.g. "CFileIO@@"
    pat = re.compile(r"^\?\?1" + re.escape(classpart) + r"[A-Z]+@XZ$")
    hits = [s for s in obj_syms if pat.match(s)]
    return hits[0] if len(hits) == 1 else None


def units_from_toml(path):
    """source-path (repo-relative) -> unit stem."""
    import tomllib
    with open(path, "rb") as f:
        data = tomllib.load(f)
    return {u["source"]: u["unit"] for u in data.get("unit", [])}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--clang", default=os.environ.get("GRUNTZ_CLANG") or "clang")
    ap.add_argument("--nm", default="llvm-nm")
    ap.add_argument("--tu", action="append", default=[], required=True,
                    help="source TU(s) to read @address from.")
    ap.add_argument("--flag", action="append", default=[])
    ap.add_argument("--obj", action="append", default=[],
                    help="base <unit>.obj for the authority check (same order/count "
                         "as --tu, or omit to skip).")
    ap.add_argument("--unit", action="append", default=[],
                    help="unit stem per --tu (else derived from units.toml).")
    ap.add_argument("--units-toml", default=str(REPO / "config/units.toml"))
    ap.add_argument("--out", default=str(REPO / "build/gen/symbol_names.csv"))
    args = ap.parse_args()

    unit_map = {}
    if not args.unit and Path(args.units_toml).exists():
        unit_map = units_from_toml(args.units_toml)

    rows = []          # (rva, name, unit, size)
    misses = []        # (rva, candidate, unit, reason)
    for i, tu in enumerate(args.tu):
        text = Path(tu).read_text()
        off2line = line_index(text)
        # unit resolution
        if args.unit:
            unit = args.unit[i]
        else:
            rel = str(Path(tu))
            unit = unit_map.get(rel) or unit_map.get("./" + rel) or Path(tu).stem
        # clang AST
        cmd = [args.clang, *MS_FLAGS, *args.flag, tu, "-fsyntax-only",
               "-Xclang", "-ast-dump=json"]
        res = subprocess.run(cmd, capture_output=True, text=True)
        try:
            ast = json.loads(res.stdout)
        except json.JSONDecodeError:
            log(f"ERROR {tu}: clang produced no JSON AST\n{res.stderr[:400]}")
            continue
        defs = [(mn, off2line(off)) for (mn, off) in collect_defs(ast)]
        defs.sort(key=lambda d: d[1])
        addrs = scan_annotations(text)
        obj_syms = nm_symbols(args.obj[i], args.nm) if i < len(args.obj) else None

        for rva, line, override, size in addrs:
            # nearest definition strictly below the @address comment line
            cand = next((mn for (mn, dl) in defs if dl >= line), None)
            name = override or cand
            if name is None:
                misses.append((rva, None, unit, "no definition below @address"))
                continue
            if obj_syms is None:                  # no authority check (inspection)
                rows.append((rva, name, unit, size))
                continue
            if name in obj_syms:                  # clang candidate confirmed
                rows.append((rva, name, unit, size))
                continue
            if override:                          # trust explicit @symbol
                rows.append((rva, name, unit, size))
                continue
            # clang's AST mangledName misses the destructor (it emits the
            # `??_D vbase dtor` variant, not the real `??1`). Resolve the canonical
            # `??1<class>@@...@XZ` directly from the obj's code symbols.
            resolved = plain_dtor_symbol(name, obj_syms)
            if resolved:
                rows.append((rva, resolved, unit, size))
            else:
                misses.append((rva, name, unit, "candidate not in base obj"))

    rows.sort()
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w") as f:
        # `size` is the @size byte extent (hex), the authoritative boundary the
        # synth-PDB uses for matched functions Ghidra's auto-analysis never
        # recovered as objects (so a single build is self-contained). Empty when
        # a row carries no @size (then the existing functions.csv boundary wins).
        f.write("rva,name,unit,size\n")
        for rva, name, unit, size in rows:
            size_s = f"0x{size:x}" if size else ""
            f.write(f"0x{rva:06x},{name},{unit},{size_s}\n")
    log(f"wrote {len(rows)} label(s) -> {out}")
    for rva, cand, unit, why in misses:
        log(f"  MISS 0x{rva:x} [{unit}] {why}" + (f" (cand {cand})" if cand else ""))


if __name__ == "__main__":
    main()
