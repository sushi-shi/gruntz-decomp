# Match tracking

Run inside `nix develop`:

```sh
gruntz build
gruntz verify status
gruntz walls inventory
```

[config/match_baseline.tsv](../config/match_baseline.tsv) records banked progress.
The current report is generated from a real build, not maintained in prose.

- `cur_pct`: score at the banked snapshot.
- `best_pct`: best observed score associated with that implementation fingerprint.
- `hist_pct`: historical peak across implementations, indicating known headroom.
- `src_hash`: source fingerprint; `cpp:` denotes a coarser fallback, not proof of a function edit.
- `state=absent`: a retained historical row currently unscored, not silently erased.

The [verifier](../scripts/gruntz/verify/verbs.py) separates fresh below-bank
regressions from carried ones, unbanked losses, and hard report failures.
`gruntz verify check` runs the gate; `--strict` also rejects carried
regressions. Use its actual output rather than inferring a verdict from the
aggregate exact count or fuzzy percentage.

The [fingerprinter](../scripts/gruntz/verify/fingerprints.py) writes
`build/gen/func_fingerprints.tsv`. Its old `build/clangd/` path is only a seed
for migration, not the active cache location.

`gruntz verify bank` is an explicit ledger-writing operation. Review and stage
the corresponding source snapshot first; do not bank an unexplained mismatch.
A normal build can refresh the README score block without banking the ledger.

An exact score is evidence about the compared body and referents, not proof of
all source identities, callers, or runtime behavior. A historical peak is not a
current-source proof. Keep per-function work state in the derived inventory and
the existing ledgers rather than adding another report to `docs/`.
