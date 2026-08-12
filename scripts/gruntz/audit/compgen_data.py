#!/usr/bin/env python3
"""compgen_data.py - the compiler-generated DATA pin ratchet.

`config/retail/compiler-generated-data.tsv` is the DATA analog of the RVA_COMPGEN
source pin: it names a datum cl.exe emits from a definition that already exists in
the tree, but that neither source-side data device can reach.

  * `DATA(rva)` binds to an AST VarDecl in the MAIN file, so it is ignored inside a
    header - and a function-local static in a header inline lives exactly there;
  * `DATA_COMPGEN(rva, value)` wraps a compiler-generated allocation at its
    USE SITE, so it needs a value expression - and a `??_B` dynamic-init guard byte
    has no source spelling at all (cl assigns it a counter).

It is a MANIFEST and not a macro because these data have no owning TU: cl emits each
as a COFF COMMON (a tentative definition) into every TU that instantiates the inline
and the linker merges the copies into one bss slot, so any source position would
fabricate an owner.

WHAT THIS GATE PROVES - the reason the manifest is not the retired DATA_SYMBOL in a
new coat. DATA_SYMBOL was a source DECLARATION that let a datum exist as a name-only
pin instead of a real C++ definition. Here the definition is real and the only fact
stated is the retail ADDRESS, so the gate re-derives everything else from the objs
cl.exe just produced:

  1. SPELLING   - canonical and machine-checkable, like every other label:
                  4 tab-separated columns, rva zero-padded to 8 lowercase hex
                  digits, size unpadded lowercase hex, rows ascending by rva, no
                  duplicate rva or symbol, and the `emitter` path must exist.
  2. AUTHORITY  - each pinned symbol must be a COMMON in at least one base obj, and
                  every copy's cl-computed size must equal the pinned extent. An
                  invented row therefore binds nothing and fails here.
  3. BINDING    - the pin must have reached build/gen/symbol_names.csv under that
                  exact name. Catches a pin that parses but that the label pass
                  silently dropped.
  4. COMPLETENESS - every COMMON in every base obj must be pinned. Without this the
                  class silently regrows: a COMMON costs 0% (objdiff masks relocs)
                  and links fine, so nothing else in the build would ever notice.

    python -m gruntz.audit.compgen_data           # report
    python -m gruntz.audit.compgen_data --gate    # exit 1 on any violation
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
sys.path.insert(0, str(REPO / "scripts"))

MANIFEST = REPO / "config/retail/compiler-generated-data.tsv"
BASE_DIR = REPO / "build/objdiff/base"
SYMBOLS = REPO / "build/gen/symbol_names.csv"

RVA_RE = re.compile(r"^0x[0-9a-f]{8}$")
SIZE_RE = re.compile(r"^0x(?:0|[1-9a-f][0-9a-f]*)$")


def parse(path=MANIFEST):
    """([(lineno, rva, size, symbol, emitter)], [spelling error]) - never raises."""
    rows, errs = [], []
    if not Path(path).exists():
        return rows, ["%s: missing" % path]
    rel = Path(path).relative_to(REPO)
    for i, ln in enumerate(Path(path).read_text().splitlines(), 1):
        if not ln.strip() or ln.lstrip().startswith("#"):
            continue
        parts = ln.split("\t")
        if len(parts) != 4:
            errs.append("%s:%d: expected 4 tab-separated columns, got %d"
                        % (rel, i, len(parts)))
            continue
        rva_s, size_s, sym, emitter = parts
        if not RVA_RE.match(rva_s):
            errs.append("%s:%d: rva %r must be 0x + 8 lowercase hex digits"
                        % (rel, i, rva_s))
            continue
        if not SIZE_RE.match(size_s) or int(size_s, 16) == 0:
            errs.append("%s:%d: size %r must be unpadded lowercase hex and non-zero"
                        % (rel, i, size_s))
            continue
        if not sym.strip() or sym != sym.strip():
            errs.append("%s:%d: symbol %r is empty or padded" % (rel, i, sym))
            continue
        if not (REPO / emitter.split(":")[0]).exists():
            errs.append("%s:%d: emitter %r does not name an existing file"
                        % (rel, i, emitter))
            continue
        rows.append((i, int(rva_s, 16), int(size_s, 16), sym, emitter))

    for a, b in zip(rows, rows[1:]):
        if b[1] <= a[1]:
            errs.append("%s:%d: rva 0x%08x is not above the previous row's 0x%08x "
                        "(rows sort ascending by rva)" % (rel, b[0], b[1], a[1]))
    for key, what in ((1, "rva"), (3, "symbol")):
        seen = {}
        for r in rows:
            if r[key] in seen:
                errs.append("%s:%d: duplicate %s (also line %d)"
                            % (rel, r[0], what, seen[r[key]]))
            seen[r[key]] = r[0]
    return rows, errs


def base_commons(base_dir=BASE_DIR):
    """{symbol: {unit: size}} for every COFF COMMON in the base objs.

    A COMMON is section 0 with a NON-ZERO Value holding the size; section 0 with a
    zero Value is an ordinary undefined external. That one bit is the whole
    difference between "cl defined this and the linker will merge the copies" and
    "this is an unresolved reference".
    """
    from gruntz.build.coff_oracle import _Coff

    out = {}
    if not Path(base_dir).is_dir():
        return out
    for obj in sorted(Path(base_dir).glob("*.obj")):
        try:
            c = _Coff(obj)
        except Exception:
            continue
        for idx, value, secnum in c.iter_symbols():
            if secnum == 0 and value:
                out.setdefault(c.sym_name(idx), {})[obj.stem] = value
    return out


def bound_names(symbols=SYMBOLS):
    """{rva: name} for the kind=data rows the label pass actually emitted."""
    out = {}
    if not Path(symbols).is_file():
        return out
    with Path(symbols).open(newline="") as f:
        for r in csv.DictReader(l for l in f if not l.lstrip().startswith("#")):
            if (r.get("kind") or "") == "data":
                out[int(r["rva"], 16)] = r["name"]
    return out


def violations():
    rows, errs = parse()
    commons = base_commons()
    if not commons:                      # no base objs yet - spelling only
        return errs, rows, 0
    bound = bound_names()
    pinned = {r[3] for r in rows}
    for _ln, rva, size, sym, _emitter in rows:
        owners = commons.get(sym)
        if not owners:
            errs.append("0x%08x %s: pinned but NO base obj emits it as a COMMON - a "
                        "stale or invented pin binds nothing" % (rva, sym))
            continue
        wrong = {u: n for u, n in owners.items() if n != size}
        if wrong:
            errs.append("0x%08x %s: pinned extent 0x%x but cl emitted %s"
                        % (rva, sym, size,
                           ", ".join("0x%x in %s" % (n, u)
                                     for u, n in sorted(wrong.items()))))
            continue
        if bound and bound.get(rva) != sym:
            errs.append("0x%08x %s: not bound in symbol_names.csv (found %r) - the "
                        "label pass dropped the pin"
                        % (rva, sym, bound.get(rva)))
    for sym in sorted(commons):
        if sym not in pinned:
            errs.append("%s: a COMMON in [%s] with no pin in %s - add its retail rva "
                        "or the datum stays unnamed (which costs 0%% and so nothing "
                        "else will ever report it)"
                        % (sym, ", ".join(sorted(commons[sym])),
                           MANIFEST.relative_to(REPO)))
    return errs, rows, len(commons)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 on any violation (build-tail ratchet)")
    args = ap.parse_args()
    errs, rows, n_common = violations()
    for e in errs:
        print(e)
    if errs:
        print("compgen-data: %d violation(s)" % len(errs))
        return 1 if args.gate else 0
    print("compgen-data: OK - %d pin(s) cover all %d compiler-generated COMMON "
          "symbol(s), each authority-checked against the base objs"
          % (len(rows), n_common))
    return 0


if __name__ == "__main__":
    sys.exit(main())
