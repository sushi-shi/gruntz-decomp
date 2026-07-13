"""Census the `g_<hex>` globals: where declared, whether DATA()-bound, and to what.

THE DEFECT FAMILY
-----------------
A global named for its own address (`g_5e8fe8` == VA 0x5e8fe8 == RVA 0x1e8fe8) carries
its ground truth in its name - but the tree does three wrong things with them:

  1. NOT HOMED: the declaration lives in the CONSUMER TUs, not in the owner. (Do NOT
     "fix" this by centralising into a fat globals header - force-including one
     REGRESSES matched TUs through regalloc. Home the DEFINITION in the owner TU with
     the DATA() on it, and leave ONE extern in a header.)
  2. NO DATA() ANYWHERE: the symbol is bound to no retail address, so the delinker
     cannot resolve it. It is reloc-masked, so it costs 0% and objdiff shows nothing -
     only this check finds it.
  3. CONFLICTING BINDINGS: the same name carrying DATA() at two DIFFERENT rvas, or a
     DATA() that disagrees with the rva the NAME asserts. Those are flagged `!!`.

NOT a defect (measured 2026-07-13, and worth stating because it looks like one): a global
with ONE DATA() binding plus plain `extern` decls in the consumer TUs. That is exactly
correct - the binding is a property of the SYMBOL, not of each declaration, and one
DATA() per name is all the delinker needs. 35 of the 56 look like that and all 35 are
fine: every one binds to exactly one rva, and that rva matches its own name.

Usage:  python -m gruntz.analysis.hex_globals            # the table
        python -m gruntz.analysis.hex_globals --unbound  # only the ones with no DATA()
Exit 0 always (a report, not a gate).
"""

import argparse
import os
import re
import sys
from collections import defaultdict
from pathlib import Path

REPO = Path(os.environ.get("REPO", Path(__file__).resolve().parents[3]))

# `g_<hex>` with >= 4 hex digits (so g_1 / g_ab are not swept in).
NAME_RE = re.compile(r"\bg_([0-9a-f]{4,8})\b")
DATA_RE = re.compile(r"\bDATA\(\s*(0x[0-9a-fA-F]+)\s*\)")
# A comment line - the census counts LIVE CODE only.
COMMENT_RE = re.compile(r"^\s*(//|\*|/\*)")


def scan():
    """name -> {'decl': [(file, line, rva_or_None)], 'use': [(file, line)]}"""
    info = defaultdict(lambda: {"decl": [], "use": []})
    for root in ("src", "include"):
        for p in sorted((REPO / root).rglob("*")):
            if p.suffix not in (".cpp", ".h"):
                continue
            lines = p.read_text(errors="replace").splitlines()
            pending_rva = None  # a DATA() on the previous line binds the next decl
            for i, ln in enumerate(lines, 1):
                if COMMENT_RE.match(ln):
                    continue
                # Strip the TRAILING comment. A rename note like `// (was g_6452a4)` is a
                # dead reference, not a live one, and counting it INFLATES the census -
                # measured: it fabricated four phantom "unbound" globals in GruntzMgr.cpp
                # alone, which are in fact already renamed AND bound.
                ln = ln.split("//")[0]
                if not ln.strip():
                    continue
                m_data = DATA_RE.search(ln)
                names = NAME_RE.findall(ln)
                if m_data and not names:
                    pending_rva = m_data.group(1)
                    continue
                if not names:
                    if ln.strip():
                        pending_rva = None
                    continue
                rel = str(p.relative_to(REPO))
                is_decl = bool(re.search(r"\bextern\b|^\s*(static\s+)?[A-Za-z_][\w:*<>& ]*\bg_[0-9a-f]{4,8}\s*(=|;|\[)", ln))
                rva = m_data.group(1) if m_data else pending_rva
                for nm in set(names):
                    if is_decl:
                        info["g_" + nm]["decl"].append((rel, i, rva))
                    else:
                        info["g_" + nm]["use"].append((rel, i))
                pending_rva = None
    return info


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--unbound", action="store_true", help="only globals with NO DATA() anywhere")
    args = ap.parse_args()

    info = scan()
    bound_only, unbound_only, both, undeclared = [], [], [], []
    for nm, d in sorted(info.items()):
        if not d["decl"]:
            undeclared.append(nm)
            continue
        rvas = {r for (_f, _l, r) in d["decl"] if r}
        n_unbound = sum(1 for (_f, _l, r) in d["decl"] if not r)
        if rvas and n_unbound:
            both.append(nm)
        elif rvas:
            bound_only.append(nm)
        else:
            unbound_only.append(nm)

    def expected(nm):
        """The RVA the NAME asserts: VA 0x5e8fe8 -> RVA 0x1e8fe8 (image base 0x400000)."""
        va = int(nm[2:], 16)
        return va - 0x400000 if va >= 0x400000 else va

    print(f"g_<hex> globals: {len(info)} distinct\n"
          f"  DATA()-bound only      : {len(bound_only)}\n"
          f"  NO DATA() anywhere     : {len(unbound_only)}   <- bound to no retail address\n"
          f"  bound + plain externs  : {len(both)}   (NOT a defect - one DATA() per symbol is all that is needed)\n"
          f"  used, never declared   : {len(undeclared)}\n")

    show = unbound_only if args.unbound else (unbound_only + both + bound_only)
    for nm in show:
        d = info[nm]
        rvas = sorted({r for (_f, _l, r) in d["decl"] if r})
        state = "UNBOUND" if not rvas else ("DUP" if any(not r for (_f, _l, r) in d["decl"]) else "bound")
        exp = expected(nm)
        note = ""
        if rvas:
            got = {int(r, 16) for r in rvas}
            if len(got) > 1:
                note = "  !! bound to MORE THAN ONE rva"
            elif exp not in got:
                note = f"  !! name asserts 0x{exp:08x}, DATA() says {rvas[0]}"
        print(f"  {nm:<12s} {state:<8s} name->rva 0x{exp:08x}  DATA={','.join(rvas) or '-':<12s}{note}")
        for f, l, r in d["decl"]:
            print(f"       decl {f}:{l}{'  DATA ' + r if r else '   (no DATA)'}")
        if d["use"]:
            files = sorted({f for f, _ in d["use"]})
            print(f"       used in: {', '.join(files[:6])}{' ...' if len(files) > 6 else ''}")
    if undeclared:
        print("\n  used but NEVER declared:", ", ".join(undeclared))
    return 0


if __name__ == "__main__":
    sys.exit(main())
