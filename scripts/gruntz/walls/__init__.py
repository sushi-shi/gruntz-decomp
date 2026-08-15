"""gruntz.walls - the wall-breaking slice: the remaining matching campaign.

    gruntz walls inventory        the derived worklist (report x Model x
                                  match_baseline) - ascending historical MAX
    gruntz walls diagnose <fn>    classify one wall from the normalized pair:
                                  referent -> inline/call-set -> cfg -> regalloc
    gruntz walls inline-model     the cl 5.0 inline-budget model (--gap
                                  quantifies the finish-the-caller lever;
                                  --measure-cb titrates with the real compiler)
    gruntz walls aggregate-copies rep-movs count sieve (object copied vs
                                  spelled as fields) over sub-100% functions
    gruntz walls eh-frame         /GX frame-presence + unwind-state sieve,
                                  cause-tagged (INLINE_CUT/EXIT_MERGE/objects)
    gruntz walls global-refs      global read-COUNT sieve (the cached-global
                                  bug class; --calibrate = detector-bug rate)
    gruntz walls stale-markers    @early-stop markers sitting on 100% bodies

The easy matches are drained; what remains of the matching objective IS the
walls. This package holds the instruments a matcher points at a classified
wall. What does NOT live here: blind permutation search (removed by ruling -
walls are broken by understood levers, not ground), and any hand-kept wall
ledger (the worklist is derived from the compare report every time).

Input surface: the Model, the compare out-dir (report.json + normalized
objs), config/match_baseline.tsv, tool.objdump/tool.cl, delink.coffx (the
shared COFF topology reader). Read-only except inline-model's scratch
harness compiles under build/inline-model/.
"""

from __future__ import annotations

_SUBS = {"inventory": "gruntz.walls.inventory",
         "diagnose": "gruntz.walls.diagnose",
         "inline-model": "gruntz.walls.inline_model",
         "aggregate-copies": "gruntz.walls.aggregate_copies",
         "eh-frame": "gruntz.walls.eh_frame",
         "global-refs": "gruntz.walls.global_refs",
         "stale-markers": "gruntz.walls.stale_markers"}


def main(argv=None) -> int:
    import importlib
    import sys
    argv = list(sys.argv[1:] if argv is None else argv)
    if not argv or argv[0] in ("-h", "--help") or argv[0] not in _SUBS:
        print(__doc__.strip())
        return 0 if argv and argv[0] in ("-h", "--help") else 2
    mod = importlib.import_module(_SUBS[argv[0]])
    sys.argv = [f"gruntz walls {argv[0]}", *argv[1:]]
    return mod.main(argv[1:])
