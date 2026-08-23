"""gruntz.walls - the wall-breaking slice: the remaining matching campaign.

    gruntz walls inventory        the derived worklist (report x Model x
                                  match_baseline) - ascending historical MAX
    gruntz walls diagnose <fn>    classify one wall from the normalized pair:
                                  referent -> inline/call-set -> cfg -> regalloc
    gruntz walls inline-model     the cl 5.0 inline-budget model (--gap
                                  quantifies the finish-the-caller lever;
                                  --measure-cb titrates with the real compiler)
    gruntz walls aggregate-copies rep-movs count sieve; a source/CFG lead,
                                  never proof until block merging is excluded
    gruntz walls framescan        stack-frame-size sieve: our `sub esp,N`
                                  against retail's, ranked by what survives
                                  masking the displacements a frame shift moves
    gruntz walls jccscan          CONDITION-CODE sieve: the branch mnemonic IS
                                  the source comparison operator and objdiff
                                  never masks it, so a differing multiset is a
                                  different comparison (SIGNED = a signedness
                                  defect, OPERATOR = switch-vs-|| chain,
                                  POLARITY = arm order)
    gruntz walls loopscan         loop-BODY-SIZE sieve: an instruction inside
                                  the loop on one side and outside it on the
                                  other runs N times instead of once, and a
                                  masked diff reads that as a schedule coin
    gruntz walls retscan          one-sided calling-convention sieve: retail's
                                  own `ret N` against our mangled name's
                                  stack-argument bytes. The stack complement of
                                  thisscan, which owns ECX: a dropped RECEIVER
                                  is invisible here and a dropped ARGUMENT is
                                  invisible there. --cdecl is the half `ret 0`
                                  cannot do - our declared __cdecl argument
                                  bytes against RETAIL's caller cleanup, still
                                  one-sided. --virtual runs the vtable-slot
                                  census over the same names
    gruntz walls signscan         ARITHMETIC signedness sieve, the complement of
                                  jccscan: cl lowers a division, modulo, shift
                                  or narrow load through different instructions
                                  per declared signedness, so `cdq`/`idiv`/`div`
                                  /one-operand `imul`/`mul` counts and a
                                  sar<->shr or movsx<->movzx swap name a TYPE.
                                  The only wall class that is a CORRECTNESS
                                  difference (--control fires it on a positive)
    gruntz walls escapescan       ADDRESS-ESCAPE sieve (declined enregistration):
                                  retail materializing a frame address INTO a
                                  call we feed from a register says the source
                                  is missing an `&` or a whole local object.
                                  Keyed on the callee referent, because the raw
                                  `lea [esp+N]` count is rematerialization, not
                                  source
    gruntz walls reloadscan       the three DECLINED memory optimizations, one
                                  machine: a load retail repeats ACROSS a call
                                  (CSE declined - the source re-reads it), a
                                  load retail repeats INSIDE a loop (hoisting
                                  declined - aliasing or an escaped address),
                                  and an INDEX vs pointer-walk loop body
                                  (strength reduction declined - the induction
                                  variable is live after the loop)
    gruntz walls valuetemp        by-value struct temp sieve: retail's inlined
                                  accessor returns a pair BY VALUE and leaves the
                                  UNREAD half's store dead in the frame
                                  (--control re-proves the detector fires)
    gruntz walls residue          what the masked residual IS, once position
                                  and register-name differences are cancelled
                                  (arm-result temps, wrong constants/offsets)
    gruntz walls storescan        permuted-member-store-run sieve: the two
                                  sides store the same fields in a different
                                  ORDER, so the source transcribed C2's output
                                  (--values screens for a swapped CONSTANT)
    gruntz walls thisscan         dropped-receiver sieve: a member modelled as
                                  a free function has IDENTICAL callee bytes,
                                  so only a caller shows it - as a dead ECX
                                  load retail emits before the call and we do
                                  not (--inverse for the mirror). --retail is
                                  the stronger form: our side carries no
                                  information here, so retail's own call sites
                                  decide the row with no score and no pairing
    gruntz walls eh-frame         /GX frame-presence + unwind-state sieve,
                                  cause-tagged (inline/merge/state-flow/object)
    gruntz walls global-refs      global read-COUNT sieve (the cached-global
                                  bug class; --calibrate = detector-bug rate)
    gruntz walls semdiff <fn>     OPERAND-LEVEL adjudication of one pair:
                                  exclusive fp/disp/store/imm keys, plus the
                                  ordered referent sequence a masked diff
                                  structurally cannot show
    gruntz walls semsweep <tsv>   the same screen over a worklist range - one
                                  line per clean row, the exclusive keys and
                                  FP deltas for the rest
    gruntz walls ehactions <fn>   the /GX unwind ACTION sequence (object slot
                                  + dtor identity, in order) of one parent -
                                  a funclet COUNT delta is the ctor-inlining
                                  boundary, a differing action is a defect.
                                  --census does the whole sub-100 EH band,
                                  grouped by parent: the funclets pair BY
                                  CONSTRUCTION through the normalizer's
                                  canonical names, and most of the band is a
                                  second readout of its parent's frame.
                                  --census classifies the band; --shift reads
                                  the slot-shift group's displacement deltas
                                  and says whether each parent's objects moved
                                  as a unit or relative to each other
    gruntz walls calibrate        the REFLEXIVITY control the paired sieves
                                  lacked: framescan/loopscan/jccscan/storescan/
                                  residue over the EXACT rows, where every cell
                                  must be 0. It tests the REFERENT filters (the
                                  two objects are cl's and the delinker's, and
                                  their relocation tables differ even at
                                  100.00); it structurally CANNOT test whether
                                  a byte-keyed quantity is comparable between
                                  two different builds - that is equal here by
                                  construction
    gruntz walls stale-markers    @early-stop markers sitting on 100% bodies
    gruntz walls review           Codex's source-hash-scoped personal reviews
    gruntz walls priors           BOTH prior-verdict stores for a worklist -
                                  the comment above the RVA() pin AND the
                                  review ledger row - screened before any A/B

The easy matches are drained; what remains of the matching objective IS the
walls. This package holds the instruments a matcher points at a classified
wall. What does NOT live here: blind permutation search (removed by ruling -
walls are broken by understood levers, not ground).  The worklist is derived
from the compare report every time.  The optional Codex review ledger records
only reviewer progress and invalidates each row when its source hash changes;
it is not evidence that a reconstruction is correct.

Input surface: the Model, the compare out-dir (report.json + normalized
objs), config/match_baseline.tsv, tool.objdump/tool.cl, delink.coffx (the
shared COFF topology reader). Read-only except inline-model's scratch
harness compiles under build/inline-model/.
"""

from __future__ import annotations

_SUBS = {"calibrate": "gruntz.walls.calibrate",
         "inventory": "gruntz.walls.inventory",
         "diagnose": "gruntz.walls.diagnose",
         "inline-model": "gruntz.walls.inline_model",
         "aggregate-copies": "gruntz.walls.aggregate_copies",
         "eh-frame": "gruntz.walls.eh_frame",
         "framescan": "gruntz.walls.framescan",
         "jccscan": "gruntz.walls.jccscan",
         "loopscan": "gruntz.walls.loopscan",
         "signscan": "gruntz.walls.signscan",
         "escapescan": "gruntz.walls.escapescan",
         "reloadscan": "gruntz.walls.reloadscan",
         "valuetemp": "gruntz.walls.valuetemp",
         "residue": "gruntz.walls.residue",
         "retscan": "gruntz.walls.retscan",
         "storescan": "gruntz.walls.storescan",
         "thisscan": "gruntz.walls.thisscan",
         "global-refs": "gruntz.walls.global_refs",
         "ehactions": "gruntz.walls.ehactions",
         "semdiff": "gruntz.walls.semdiff",
         "semsweep": "gruntz.walls.semdiff",
         "stale-markers": "gruntz.walls.stale_markers",
         "priors": "gruntz.walls.priors",
         "review": "gruntz.walls.reviews"}


def check_unit(unit: str | None) -> str | None:
    """`--unit` filters answer 0/none for a name nobody has - which reads as a
    clean result rather than a typo. Reject an unknown unit here instead."""
    if unit is None:
        return None
    from gruntz.manifest import units as manifest_units
    known = {u["unit"] for u in manifest_units()}
    if unit in known:
        return unit
    import difflib
    import sys
    near = difflib.get_close_matches(unit, sorted(known), n=3)
    print(f"[walls] unknown unit {unit!r} - not in config/units.toml"
          + (f" (did you mean: {', '.join(near)}?)" if near else "")
          + "\n        `gruntz sema map units` lists the units that claim rows",
          file=sys.stderr)
    raise SystemExit(2)


def main(argv=None) -> int:
    import importlib
    import sys
    argv = list(sys.argv[1:] if argv is None else argv)
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__.strip())
        return 0 if argv else 2
    if argv[0] not in _SUBS:
        print(f"gruntz walls: unknown verb {argv[0]!r} (have: "
              f"{', '.join(_SUBS)})", file=sys.stderr)
        return 2
    mod = importlib.import_module(_SUBS[argv[0]])
    sys.argv = [f"gruntz walls {argv[0]}", *argv[1:]]
    entry = mod.sweep_main if argv[0] == "semsweep" else mod.main
    return entry(argv[1:])
