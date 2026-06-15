# match-status — queriable matching progress & regressions

`scripts/match_status.py` makes one question cheap to answer:

> **did anything regress?** → `scripts/match_status.py check`

It is a small, PDB-free match tracker. We have no target PDBs and only a few
dozen units, so instead of a full database with interning + declaration records
we keep a single git-tracked text file and diff a fresh build against it. Git is
the history store.

## The model

objdiff already emits a fuzzy% per function in `build/objdiff/report.json`
(`units[].functions[].fuzzy_match_percent`). `match_status.py` remembers the
**best-ever** fuzzy% per function in `config/match_baseline.tsv` and reports any
function whose freshly-built fuzzy% sits below its recorded best.

Two ideas, a best-ever high-water mark gated by a source fingerprint:

- **Keep the max, not the last value.** `best_pct` only ever rises on `update`;
  it is never silently lowered. So a drop caused by something *unrelated to the
  current edit* — a shared header, a flag tweak, a target-side delink change —
  stays visible until a human looks at it.
- **Gate the max by a source fingerprint.** Each function carries `src_hash`. On
  `update`, if a function's fingerprint changed, it resets `best ← current` (the
  old peak belonged to *different* source, so it isn't a real regression); if
  unchanged, `best ← max(best, current)`. Deliberately rewriting a function
  clears its stale peak; everything you did *not* touch keeps guarding its
  high-water mark.

### Per-function fingerprints (clangd)

The fingerprint is **per function**, not per `.cpp`. `scripts/func_fingerprints.py`
asks clangd (`textDocument/documentSymbol`, hierarchical) for each function's
source extent and hashes that range's text. Whole-`.cpp` hashing was too coarse:
editing one function would reset the high-water mark of every *sibling* in the
unit, hiding any collateral regression they suffered. Per-function gating keeps
siblings independent — edit `SaveOption`, and a drop in `SetDefaults` two
functions down still surfaces as a regression.

The mangled→source bridge: C++ names go through `llvm-undname` to a
`Class::Method` key matched against clangd's qualified symbol; C names strip the
cdecl/stdcall decoration (`_init_block`→`init_block`, `_Foo@12`→`Foo`). clangd
emits both the zlib forward-declaration prototypes and the real definitions, so
we keep the multi-line body range(s) and drop single-line prototypes.

This is a **derived cache** at `build/clangd/func_fingerprints.tsv` (gitignored),
regenerated incrementally — only TUs whose `.cpp` content hash changed get
re-parsed. `gruntz build` refreshes it before `check`. A function clangd can't
resolve (a compiler-generated `` `scalar deleting dtor' ``, a WIP unit, a name
clangd spells differently) simply gets no entry, and `match_status` falls back to
the unit's whole-`.cpp` hash for it — coarser but always available (e.g. in a
fresh worktree with no clangd compile DB). Current coverage: ~99/102 functions
fingerprinted, the rest on the safe fallback.

## The baseline file

`config/match_baseline.tsv` — git-tracked, sorted, deterministic; two
TAB-separated sections, hand-greppable and trivial to parse from py/awk:

```
# [units]      unit  n_functions  matched          (matched = functions at 100% now)
adler32        1  1
# [functions]  unit  function  best_pct  cur_pct  tries  src_hash
adler32        _adler32  100.0000  100.0000  1  7d212c481a3f
```

Per function it carries three numbers, each answering a different question:

- **`best_pct`** — best-ever (max) fuzzy%. The **regression gate**: a working-tree
  build below this (with `src_hash` unchanged) is a regression. May sit at 100%
  even while the function currently scores lower.
- **`cur_pct`** — fuzzy% at *this commit*. Diff two commits' baselines to see the
  actual moves (a function `10%→40%`, a unit `5→10` functions) while `best_pct`
  holds the high-water mark.
- **`tries`** — how many times this function's `src_hash` changed across commits,
  i.e. how much it's been worked on. High = hard to match.

The `[units]` section records each unit's function count and how many are matched,
so a diff shows units growing. `git diff config/match_baseline.tsv` after an
`update` is itself a readable "what changed" view; cross-commit comparison is the
`diff` command below.

## Commands

Run inside `nix develop` (the banner goes to stderr, so `--json` pipes cleanly):

```
scripts/match_status.py check                 # regressions vs baseline (non-fatal)
scripts/match_status.py check --all           # also show improve/touched/new
scripts/match_status.py check --strict         # exit 1 if regressions (CI gate)
scripts/match_status.py status --below 99      # per-function current %, worst first
scripts/match_status.py status --by-tries      # most-worked-on functions first
scripts/match_status.py status --unit rezmgr   # filter by unit / --grep <name>
scripts/match_status.py summary                # 3-metric report vs the full engine
scripts/match_status.py summary --write-readme  # refresh the README score block
scripts/match_status.py update                 # recompute best/cur/tries, write baseline
scripts/match_status.py update --accept-regressions  # bless current as the new floor
scripts/match_status.py diff <revA> [<revB>]   # what moved between two commits' baselines
scripts/match_status.py diff HEAD~5 --all      # ... incl. TOUCHED; revB defaults to working tree

scripts/func_fingerprints.py [--all] [-v]      # refresh the per-function cache (needs .#build)
```

`diff` reads each side from `git show <rev>:config/match_baseline.tsv` and reports
per-unit count moves and per-function `cur%` moves (`10% → 40%`), with each
function's `max` and `tries` alongside — so you can see both progress and which
functions have been ground on the most.

`--report PATH` overrides the report location (default
`build/objdiff/report.json`).

## Progress report (the README score block)

`summary` renders a score block (and `--write-readme` writes it into `README.md`
between `<!-- match-score:start -->` / `<!-- match-score:end -->`, refreshed by
`gruntz build`). Three metrics per started module:

- **Functions exact** — `matched_functions / total_functions` (objdiff 100%s).
- **Fuzzy** — code-weighted partial-credit % (how close, includes <100%).
- **Code matched** — `matched_code / total_code`, byte-exact only.

The important part: **the totals are weighed against the whole engine, not just
the units we've started.** objdiff's `report.json` only counts
functions already pulled into units — measuring 62/113 there reads as "99%
fuzzy" when we've barely begun. So the denominator is the full reversing target:
every in-`.text` non-thunk function minus FID-identified CRT/MFC library code
(`build/ghidra/exports/functions.csv` minus `build/fid/library_labels.csv`). The
bulk we have not started shows up as the `(unmatched)` row at 0%, and the
headline reads honestly — e.g. `62 / 9,083 functions exact (0.68%)`. A second
line keeps the started-unit view for context. If the Ghidra/FID exports aren't
present (fresh worktree), it falls back to started-unit totals and says so.

## Non-fatal by design

`check` does **not** fail the build. In binary matching many fuzzy% drops are
not under the matcher's control — matching one function to 100% can shift a
shared TU's codegen and nudge a sibling down 0.1%, the delinked target side can
move, etc. Blocking the loop on that noise would stall real progress. So `check`
is a review signal that exits 0 by default, and `gruntz build` only prints it.
`--strict` opts into a non-zero exit for anyone who wants a CI/pre-commit gate.

## Workflow

1. Edit `src/`, then `gruntz build`. The build refreshes the fingerprint cache
   and prints regressions vs the baseline at the end (non-fatal).
2. `check` flags any function below its best. Either fix the regression, or —
   if the drop is intentional/uncontrollable — `update --accept-regressions` to
   set the new floor, then commit the baseline.
3. When you land an improvement, `scripts/match_status.py update` raises the
   recorded best and you commit `config/match_baseline.tsv` alongside the code.
   Reviewers see the % movement in the diff.

## What we deliberately left out (and why)

No database, no symbol/unit/file interning, no `frameless`/prologue inspection, no
declaration records or `base_only` taxonomy, no statement-level structure
classification. All of those need target PDBs or rich symbol info we don't have,
and pay off only at a scale of thousands of TUs. The useful core —
best-% high-water mark, per-function source-fingerprint gating, and the
regress/improve/new/lost categorisation — survives in a couple of text-file
scripts.
