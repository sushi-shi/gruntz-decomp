# match-status — queriable matching progress & regressions

`python -m gruntz.match.status` makes one question cheap to answer:

> **did anything regress?** → `python -m gruntz.match.status check`

It is a small, PDB-free match tracker. We have no target PDBs and only a few
dozen units, so instead of a full database with interning + declaration records
we keep a single git-tracked text file and diff a fresh build against it. Git is
the history store.

## The model

objdiff already emits a fuzzy% per function in `build/objdiff/report.json`
(`units[].functions[].fuzzy_match_percent`). `gruntz.match.status` remembers the
**best-ever** fuzzy% per function in `config/match_baseline.tsv` and reports any
function whose freshly-built fuzzy% sits below its recorded best.

Two ideas, a best-ever high-water mark keyed by the retail body:

- **Keep the max, not the last value.** `best_pct` only ever rises on `update`;
  it is never silently lowered. So a drop caused by something *unrelated to the
  current edit* — a shared header, a flag tweak, a target-side delink change —
  stays visible until a human looks at it.
- **Identify the body by RVA, not by source text or TU.** Editing a function can
  never lower its high-water mark. If an annotation moves to the real TU while
  retaining the same retail RVA, `update` carries its `best_pct` and `tries` to
  the new row. If the same name moves to a different RVA, it now denotes a
  different body and starts a new history. A retail body with no current source
  claim remains in the ledger and is reported `LOST`; removing an artificial
  emitter must not erase its historical MAX.

### Per-function fingerprints (clangd)

The fingerprint is **per function**, not per `.cpp`. `gruntz.match.fingerprints`
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
a **fallback-tagged** whole-`.cpp` hash (`cpp:<sha>`) for it — coarser but always
available (e.g. in a fresh worktree with no clangd compile DB).

The tag matters: a fallback fingerprint is *unknown*, not a real source change, so
`check`/`update` only use a fingerprint difference to classify a row as edited and
increment `tries` when **both** sides are real (non-fallback). Fingerprints never
reset `best_pct`. When a whole unit's cache is stale/absent, `check` still compares
each body against `best` and prints a loud `DEGRADED` warning (to stderr and the
summary) naming the units to refresh — the check degrades *visibly*, never silently.

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
  build below this is a regression regardless of source edits. May sit at 100%
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
python -m gruntz.match.status check            # regressions vs baseline (non-fatal)
python -m gruntz.match.status check --all       # also show improve/touched/new
python -m gruntz.match.status check --strict    # exit 1 if regressions (CI gate)
python -m gruntz.match.status status --below 99 # per-function current %, worst first
python -m gruntz.match.status status --by-tries # most-worked-on functions first
python -m gruntz.match.status status --unit rezmgr  # filter by unit / --grep <name>
python -m gruntz.match.status summary           # 3-metric report vs the full engine
python -m gruntz.match.status summary --write-readme  # refresh the README score block
python -m gruntz.match.status update            # recompute best/cur/tries, write baseline
python -m gruntz.match.status update --accept-regressions  # bless current as the new floor
python -m gruntz.match.status diff <revA> [<revB>]  # what moved between two commits' baselines
python -m gruntz.match.status diff HEAD~5 --all # ... incl. TOUCHED; revB defaults to working tree

python -m gruntz.match.fingerprints [--all] [-v]  # refresh the per-function cache (needs the dev shell)
```

The two writing commands (`update` and `summary --write-readme`) refuse to bank
scores while a build input has unstaged edits or is untracked. Stage the intended
source/config/tooling changes first, rebuild, then write the baseline and README;
the index is the explicit source snapshot that must be committed atomically with
those generated files. Staged changes are allowed. Read-only status/check/summary
commands remain available on a dirty tree. `gruntz build` treats this refusal as a
non-fatal feedback skip and leaves both tracked score files untouched.

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
(`config/retail/functions.tsv` classified with
`config/retail/library_labels.csv`). The bulk we have not started shows up as
the `(unmatched)` row at 0%, and the headline reads honestly — e.g.
`62 / 9,083 functions exact (0.68%)`. A second line keeps the started-unit view
for context. The tracked retail inventory is required build input, so a fresh
worktree has the same denominator without first creating a Ghidra project.

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
3. When you land an improvement, `python -m gruntz.match.status update` raises the
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

See [max-fuzzy-divergence.md](max-fuzzy-divergence.md) for why current fuzzy and MAX diverge,
and `python -m gruntz.audit.max_divergence --history` for the git-recovered peak set.
