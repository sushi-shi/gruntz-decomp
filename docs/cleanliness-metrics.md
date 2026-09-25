# Cleanliness checks

Run inside `nix develop`:

```sh
gruntz verify board
gruntz verify board --semantic
```

The [board implementation](../scripts/gruntz/verify/board.py) defines the measured
rows and gate behavior. Baselines live in
[config/cleanliness](../config/cleanliness/); derived reports belong in `build/`.
The normal build checks the configured gate tiers. Read their current composition
in [tiers.py](../scripts/gruntz/verify/tiers.py), not a copied count table.

`--update` explicitly writes the measured floors; it is not a fix for a failure.
An unmeasured metric must not silently disappear. Review the underlying modeling
change before accepting a new baseline.

Counters identify candidates, not source truth. Preserve authentic SDK/ABI types,
source-backed ownership, and necessary reviewed casts. Do not replace a correct
type with a layout view, hide a cast in a helper, or invent names to lower a count.
Use [source markers](comment-markers.md) for genuinely unresolved identity.

For enum-domain reuse, `gruntz verify enum-reuse` compares current declarations
with [the review ledger](../config/reviews/enum-reuse.tsv). Equal numeric values
alone do not establish one semantic domain. Keep reports generated; do not add a
second hand-maintained inventory to this document.
