"""gruntz.sema.diagnose - `gruntz sema diagnose`: classify a matching wall.

One command from a plateaued function to a routed answer. Reads the same
base/target object pair objdiff scores (no recompile) and classifies the
residual in doctrine order:

    inline/call-set -> control flow -> register/schedule -> masked/referent

The order is load-bearing: an inlining difference perturbs branches and
registers downstream, and a CFG difference perturbs registers, so a later
class is only diagnosable once every earlier one compares clean. See the
`wall-identifier` skill for the levers per class.

rc: 0 = exact already (nothing to diagnose), 1 = classified, 2 = error.
"""
import sys

from gruntz.sema._common import GEN_NAMES, REPO, csv_find, die


def _calls(insns, stop):
    """Ordered `call` instructions before `stop`: [(offset, operand)]."""
    return [(off, op) for off, mn, op in insns
            if mn == "call" and (stop is None or off < stop)]


def run(args) -> None:
    from gruntz.core import branches as B
    from gruntz.core.report import Report

    try:
        n = int(args.rva, 16)
    except ValueError:
        die(f"'{args.rva}' is not a hex RVA")
    claim = csv_find(GEN_NAMES, n)
    if not claim:
        die("no src claim at this RVA - diagnose compares the objdiff object "
            "pair, so it needs a reconstructed fn (check `gruntz sema rva`)")
    unit, name = claim["unit"], claim["name"]

    pct = Report().fn_pct(name, unit)
    head = f"[diagnose @ {args.rva} - {name} [{unit}]"
    print(head + (f" - {pct:.2f}%]" if pct is not None else " - not in report]"))
    if pct is not None and pct >= 100.0:
        print("  EXACT - nothing to diagnose.")
        sys.exit(0)

    bobj, tobj = B.obj_paths(unit)
    for o in (bobj, tobj):
        if not o.is_file():
            die(f"{o.relative_to(REPO)} missing - run `gruntz build` first")
    bi = B.decode(bobj, name).get(name)
    ti = B.decode(tobj, name).get(name)
    if not bi:
        die(f"{name} not in {bobj.name}")
    if not ti:
        die(f"{name} not in {tobj.name} (delinked target absent - nothing to "
            "compare; check the retail claim)")
    bstop, tstop = B.code_stop(bi), B.code_stop(ti)

    # 1 - inline / call-set: the out-of-line CALL sequence must agree first.
    cb, ct = _calls(bi, bstop), _calls(ti, tstop)
    if len(cb) != len(ct):
        print(f"  CLASS: INLINE / CALL-SET - base makes {len(cb)} call(s), "
              f"target {len(ct)}.")
        which = ("we call something retail expanded (or a body is missing a "
                 "statement that carries a call)" if len(cb) > len(ct) else
                 "retail calls something we expanded or never wrote")
        print(f"  {which}.")
        print(f"  evidence: `gruntz sema disasm {args.rva} --diff` (the call "
              f"rows), `gruntz sema xref --callees {args.rva}`,")
        print("  and `llvm-nm build/objdiff/base/*.obj | grep <callee>` for "
              "COMDAT emission.")
        print("  levers: body completeness first; the cl 5.0 inline-budget "
              "rule is docs/patterns/inline-budget-emits-ool-comdat.md.")
        sys.exit(1)

    # 2 - control flow: branch counts, then the symbolic sequence.
    res = B.compare(bi, ti)
    if res["status"] == "struct":
        # A mid-function jump table truncates the symbol-scoped decode on ONE
        # side (gruntz.core.branches.table_stop), which fakes a huge branch
        # deficit. The rendered block view survives tables, so sanity-check the
        # struct verdict against block counts before routing.
        from gruntz.sema.disasm import _cfg, base_text, target_text
        nb, nt = len(_cfg(base_text(args.rva))), len(_cfg(target_text(args.rva)))
        if nt and abs(nb - nt) / nt < 0.1:
            print(f"  branch decode disagrees (base {res['nbr']} vs target "
                  f"{res['nbr_t']}) but block skeletons nearly agree "
                  f"({nb} vs {nt}) - a jump table truncated one side's decode.")
            print(f"  trust the blocks: `gruntz sema disasm {args.rva} "
                  "--blocks --diff --lite` and read the first !! row.")
            print("  CLASS: CONTROL FLOW (local) or finer - classify from the "
                  "block diff, not the branch count.")
            sys.exit(1)
    if res["status"] not in ("clean", "no-branches"):
        br, tr = res["rets"]
        print(f"  CLASS: CONTROL FLOW - base {res['nbr']} branch(es)/{br} "
              f"ret(s) vs target {res['nbr_t']}/{tr} "
              f"[{res.get('kind') or res['status']}].")
        print(f"  structural reconstruction work, NOT a permute target.")
        print(f"  evidence: `gruntz sema disasm {args.rva} --branches --diff` "
              f"(per-row verdict + pattern pointer),")
        print(f"  `gruntz sema disasm {args.rva} --blocks --diff --lite` "
              "(where the skeleton splits).")
        if br > tr:
            print("  DUP-EXIT: we duplicate an exit retail merges - "
                  "docs/patterns/goto-fail-shares-one-exit-block.md.")
        sys.exit(1)

    # 3/4 - split by whether the masked byte-shape already agrees.
    from gruntz.sema.disasm import base_text, norm, target_text
    masked_equal = norm(base_text(args.rva)) == norm(target_text(args.rva))
    if masked_equal:
        print("  CLASS: MASKED / REFERENT - the masked asm agrees but the "
              "score does not, and the masked view")
        print("  cannot show a wrong referent by construction. This is "
              "labeling/identity work, not codegen.")
        print(f"  evidence: `python -m gruntz.audit.assert_relocs {args.rva}` "
              "(the unmasked referent multiset).")
        sys.exit(1)
    print("  CLASS: REGISTER / SCHEDULE - call set and branch sequence agree; "
          "the residue is operand order,")
    print("  spill/materialization, coloring, or scheduling. This is the "
          "`permute` skill's domain (banked by MAX).")
    print("  TU-global state can reach residue no body edit can: "
          "docs/patterns/tu-state-probe-family-decides-reachability.md.")
    print("  first re-check the TYPE model - one misplaced register op often "
          "means a mis-modeled aggregate.")
    sys.exit(1)


if __name__ == "__main__":
    import sys
    sys.exit("%s is a `gruntz sema` implementation, not a standalone entry point - "
             "run `gruntz sema %s ...`."
             % (__name__, __name__.rsplit(".", 1)[-1]))
