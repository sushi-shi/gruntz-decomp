"""gruntz.cleanliness - the drive-to-0 quality board and its gates, one module
per concern (all run in the `gruntz build` gate tail; each also `python -m`):

    board.py               the scoreboard: cast/placeholder/view/extern counts,
                           the down-only RATCHET + config/cleanliness-baseline.tsv
    view_debt.py           the UNGAMEABLE fake-view metric (phantom classes,
                           declared-only methods)
    foldable_views.py      which pure-phantom classes fold cleanly (worklist)
    vtable_bans.py         hard-fail: the four manual-vtable idioms
    vtable_coverage.py     every vtable in GRUNTZ.EXE must be covered
    vtable_virtuality.py   every vtable's slots must be real virtuals
    vtable_slot_binding.py every slot wired to a real virtual (frozen backlog
                           in config/vtable-slot-binding-baseline.tsv)
    class_sizes.py         every class carries a correct SIZE/SIZE_UNKNOWN
    class_vtables.py       every virtual-owning class carries its VTBL()

(The shared class-definition scanner these build on is gruntz.core.class_meta.)

Match scoring lives in gruntz/match (status, fingerprints, high_water,
residual_queue + the measurement-integrity verify_* gates); the negative-control
suite gruntz.match.gate_selftest covers both packages. Metric doctrine:
docs/cast-metric-policy.md + docs/cleanliness-metrics.md.
"""
