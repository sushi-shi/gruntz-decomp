#!/usr/bin/env python3
"""resolve_v3.py - per-fn reloc-SEQUENCE alignment (wildcard LCS) for the
declared-only rows whose callers are not fuzzy-100 (UNTRUSTED/NOSITE).

For each base fn F referencing a declared-only symbol S:
  base_seq = F's relocs in site order, resolved to RVAs (S and other unknowns -> None)
  tgt_seq  = the same fn in the delinked target obj (same mangled name, synth PDB):
             its reloc SITES mapped to retail addresses and READ from the retail
             image (ground truth; no name trust at all)
  Align base_seq to tgt_seq with a DP (exact-RVA match=3, wildcard=1, gap=-1);
  S's aligned target value votes for its true RVA.

Output TSV: symbol  class  voted_rva  real_name  votes  spread
Classes: ALIAS_DEF / ALIAS_CLAIM / UNRECON / DATA / UNALIGNED / NOSITE / CONFLICT.
"""
import glob
import json
import re
import struct
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path



from gruntz.audit.assert_relocs import (REPO, IMAGE_BASE, _D, _fo, target_at,
                                        load_symbols, resolve, resolve_thunk)
from gruntz.cleanliness.view_debt import _current_objs
from gruntz.audit.declared_classify import coff

TEXT = (0x1000, 0x1E626B)
BASELINE = Path(REPO) / "config/declared-only-baseline.tsv"
TARGET_DIR = Path(REPO) / "build/delink/named"
REL_REL32 = 20


def fn_relocs(obj_path):
    """{fn_name: (fn_sec_off, [(off, type, symname), ...site-ordered])} for one obj.
    $L jump-table labels are merged into the enclosing fn (their section's previous
    real symbol)."""
    sections, defined, relocs = coff(obj_path)
    per_sec = defaultdict(list)
    for nm, sec, val, idx in defined:
        if nm.startswith(("$L", ".", "$SG", "__ehhandler", "__unwindfunclet", "__catch")):
            continue
        if not sections.get(sec, "").startswith(".text"):
            continue
        per_sec[sec].append((val, nm))
    for s in per_sec:
        per_sec[s].sort()
    out = {}
    for sec_idx, off, ty, nm in sorted(relocs, key=lambda r: (r[0], r[1])):
        cands = [x for x in per_sec.get(sec_idx, []) if x[0] <= off]
        if not cands:
            continue
        fval, fname = cands[-1]
        out.setdefault(fname, (fval, []))[1].append((off, ty, nm))
    return out


def align(base_vals, tgt_vals):
    """DP alignment; returns list mapping base index -> tgt index (or None)."""
    n, m = len(base_vals), len(tgt_vals)
    NEG = -10**9
    dp = [[0] * (m + 1) for _ in range(n + 1)]
    bt = [[0] * (m + 1) for _ in range(n + 1)]  # 0 diag, 1 up(skip base), 2 left(skip tgt)
    for i in range(1, n + 1):
        dp[i][0] = dp[i - 1][0] - 1
        bt[i][0] = 1
    for j in range(1, m + 1):
        dp[0][j] = dp[0][j - 1] - 1
        bt[0][j] = 2
    for i in range(1, n + 1):
        b = base_vals[i - 1]
        for j in range(1, m + 1):
            t = tgt_vals[j - 1]
            if b is not None and t is not None and b == t:
                sc = 3
            elif b is None or t is None:
                sc = 1
            else:
                sc = -2
            best = dp[i - 1][j - 1] + sc
            k = 0
            if dp[i - 1][j] - 1 > best:
                best, k = dp[i - 1][j] - 1, 1
            if dp[i][j - 1] - 1 > best:
                best, k = dp[i][j - 1] - 1, 2
            dp[i][j], bt[i][j] = best, k
    # backtrack
    mapping = [None] * n
    i, j = n, m
    while i > 0 or j > 0:
        k = bt[i][j]
        if i > 0 and j > 0 and k == 0:
            mapping[i - 1] = j - 1
            i, j = i - 1, j - 1
        elif i > 0 and (k == 1 or j == 0):
            i -= 1
        else:
            j -= 1
    return mapping


def main():
    targets = {ln.split("\t")[0].strip()
               for ln in BASELINE.read_text().splitlines()
               if ln.strip() and not ln.startswith("#")}
    only = None
    if "--only" in sys.argv:
        only = sys.argv[sys.argv.index("--only") + 1]
        targets = {only}

    sym, data, dups = load_symbols()
    import csv as _csv
    rva2name = {}
    for r in _csv.reader(open(Path(REPO) / "build/gen/symbol_names.csv", encoding="latin-1")):
        if len(r) >= 2:
            try:
                v = int(r[0], 16)
            except ValueError:
                continue
            if v not in rva2name or (r[1].startswith("?") and not rva2name[v].startswith("?")):
                rva2name[v] = r[1]

    base_defined = set()
    objs = _current_objs()
    for o in objs:
        out = subprocess.run(["llvm-nm", str(o)], capture_output=True, text=True).stdout
        for ln in out.splitlines():
            p = ln.split()
            if len(p) == 3 and p[1] != "U":
                base_defined.add(p[2])

    votes = defaultdict(Counter)
    seen_sites = Counter()
    tgt_cache = {}
    for o in objs:
        try:
            fns = fn_relocs(o)
        except Exception as e:
            print(f"# base parse fail {o}: {e}", file=sys.stderr)
            continue
        hits = {f: rl for f, rl in fns.items() if any(r[2] in targets for r in rl[1])}
        if not hits:
            continue
        stem = Path(o).stem
        tpath = TARGET_DIR / f"{stem}.c.obj"
        if not tpath.exists():
            continue
        if stem not in tgt_cache:
            try:
                tgt_cache[stem] = fn_relocs(str(tpath))
            except Exception as e:
                print(f"# tgt parse fail {tpath}: {e}", file=sys.stderr)
                tgt_cache[stem] = {}
        tfns = tgt_cache[stem]
        for fname, (foff, rl) in hits.items():
            frva = sym.get(fname)
            if frva is None or fname not in tfns:
                for off, ty, nm in rl:
                    if nm in targets:
                        seen_sites[nm] += 1
                continue
            tfoff, trl = tfns[fname]
            base_vals = []
            for off, ty, nm in rl:
                typ = "REL32" if ty == REL_REL32 else "DIR32"
                if nm in targets:
                    base_vals.append(None)
                else:
                    base_vals.append(resolve(sym, data, typ, nm, 0))
            tgt_vals = []
            for off, ty, nm in trl:
                typ = "REL32" if ty == REL_REL32 else "DIR32"
                tgt_vals.append(target_at(frva + (off - tfoff), typ))
            mapping = align(base_vals, tgt_vals)
            for k, (off, ty, nm) in enumerate(rl):
                if nm not in targets:
                    continue
                seen_sites[nm] += 1
                j = mapping[k]
                if j is not None and tgt_vals[j] is not None:
                    votes[nm][tgt_vals[j]] += 1

    print("symbol\tclass\tvoted_rva\treal_name\tvotes\tspread")
    tally = Counter()
    for t in sorted(targets):
        vs = votes.get(t)
        if not vs:
            cls = "NOSITE" if seen_sites[t] == 0 else "UNALIGNED"
            tally[cls] += 1
            print(f"{t}\t{cls}\t-\t-\t0\t-")
            continue
        top, topn = vs.most_common(1)[0]
        total = sum(vs.values())
        spread = ",".join(f"0x{v:06x}x{c}" for v, c in vs.most_common(4))
        if len(vs) > 1 and topn < total * 0.7:
            tally["CONFLICT"] += 1
            print(f"{t}\tCONFLICT\t-\t-\t{total}\t{spread}")
            continue
        rva = top
        name = rva2name.get(rva, "")
        b12 = _D[_fo(rva):_fo(rva) + 12].hex() if _D is not None else ""
        if not (TEXT[0] <= rva < TEXT[1]):
            cls = "DATA"
        elif name and name in base_defined:
            cls = "ALIAS_DEF"
        elif name:
            cls = "ALIAS_CLAIM"
        else:
            cls = "UNRECON"
        tally[cls] += 1
        print(f"{t}\t{cls}\t0x{rva:06x}\t{name or b12}\t{total}\t{spread}")
    print("# " + "  ".join(f"{k}:{v}" for k, v in tally.most_common()), file=sys.stderr)


if __name__ == "__main__":
    main()
