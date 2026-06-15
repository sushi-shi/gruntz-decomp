# Review findings — PR #5 `regressions-queriable`

Adversarial review of branch `regressions-queriable` (head `5b8579a`) vs base `6ea7f69`.
Static review only — gitignored `build/` is absent in the review worktree, so the full
`gruntz build` path was reasoned about, not executed. Items needing a build to confirm
are flagged **[needs-build]**.

Verdict: **merge-with-fixes**. The core model (best-ever high-water mark + per-function
source-fingerprint gating, source-driven so reloc-masking can't affect it) is sound and
well-documented. But two correctness issues let `check` report a false "no regressions",
which is exactly what this PR exists to prevent. The blocker has a small mitigation
applied here (a degraded-gate warning); the underlying design choice still wants a fix.

---

## BLOCKER

### B1. `check` silently hides ALL regressions when the clangd fingerprint cache is missing/stale
`scripts/match_status.py:190-196` (`fingerprinter.fp`), consumed at `:343` (`classify`) and `:160` (`gruntz build` calls `check`).

The committed baseline `config/match_baseline.tsv` stores **per-function** `src_hash`
values (verified: within `filestream` the rows carry distinct hashes — they were produced
with a live clangd cache). But the cache `build/clangd/func_fingerprints.tsv` is
**gitignored** (`.gitignore:12 /build/`) and therefore absent on any fresh worktree / CI /
machine where clangd hasn't run.

When the cache is absent or its recorded `cpp_hash` for a unit no longer matches the
working tree, `fp(unit, mangled)` returns the unit's **whole-`.cpp` hash** for every
function (`:195`). That value can never equal a stored per-function hash, so in `classify`
(`:343`) `prev["fp"] != fp(*key)` is True for **every** baseline row → every function is
classified **TOUCHED**, and the `REGRESS`/`LOST` buckets stay empty. `check` then prints
"no regressions vs baseline" even if the build genuinely regressed — a false all-clear.

This is not a corner case: `gruntz build` only runs `func_fingerprints.py` when
`build/clangd/compile_commands.json` exists (`scripts/gruntz.py:152`). If clangd setup
failed, was skipped, or the developer is on a fresh checkout, every post-build `check`
reads green regardless of reality. The docstring frames the cpp-hash fallback as "the safe
(touched) direction" (`:178-179`), but for a regression *detector* TOUCHED = "do not
flag", i.e. the **unsafe** direction (silent false negatives).

Also note the per-unit granularity makes the milder form common even with a cache present:
editing one function in a unit changes that unit's `cpp_hash`; until `func_fingerprints.py`
re-parses it, *all* functions in that unit fall back to TOUCHED — so a collateral
regression in an unedited sibling is hidden in exactly the window the per-function design
was meant to cover.

**Applied mitigation (this PR branch):** `cmd_check` now prints a loud WARNING to stderr
when `build/clangd/func_fingerprints.tsv` is absent, so a degraded run can't be mistaken
for a real all-clear (`scripts/match_status.py`, the `FP_CACHE.is_file()` guard).

**Recommended real fix (not applied — needs a design call):** make the degradation
*visible in the result*, not just a stderr line. Options, best first:
  1. In `classify`, only emit `TOUCHED` when the unit's cache is actually fresh; when the
     unit fell back to cpp-hash, emit a distinct `UNGATED`/`STALE` kind and, if `pct <
     best - EPS`, still surface it as a (lower-confidence) regression rather than swallowing
     it. Then "no regressions" means *gate was live*.
  2. Have `gruntz build` always refresh the cache (drop the `compile_commands.json`
     precondition, or fail loudly if it can't), so `check` after a build is always gated.
  3. Record in the baseline whether each `src_hash` is per-function or unit-level, and on a
     cache-less `check` compare like-for-like (cpp-hash vs cpp-hash) instead of guaranteeing
     a mismatch.

---

## MAJOR

### M1. Engine-universe filter is a dead no-op → headline denominator is wrong/inflated
`scripts/match_status.py:96-113` (`engine_universe`), surfaced in the README block at `:527-538`.

`functions.csv` is `entry_rva,byte_size,name` only (see `scripts/gruntz/ghidra/export.py:38`,
which writes `entry_rva,byte_size,name` and emits **every** `getFunctions(True)` — not
restricted to `.text`, not thunk-filtered). But `engine_universe` filters with
`r.get("in_text", "1") == "1" and r.get("is_thunk", "0") != "1"` (`:103`). Those columns
do **not exist**, so `.get(..., default)` makes the predicate always-true: every row is
counted. The only filter that actually runs is the FID subtraction (`:105-112`).

Consequence: the README headline ("vs full engine = every in-`.text` non-thunk function
minus FID library code", `:531-533`) is **not** what is computed — thunks and any non-`.text`
Ghidra functions inflate the 7,440-function / 957,867-byte denominator, so the reported
0.75%/1.07%/2.62% are understated against their own stated definition. The number is a
deliberately fuzzy progress metric, so this is not a crash, but it misrepresents the
denominator the prose promises.

This bug is **copied verbatim** from the pre-existing `scripts/analysis/gen_match_queue.py:58`
(same dead `in_text`/`is_thunk` defaults on the same CSV), so it is a latent repo bug
inherited, not newly invented — but the README now publishes a number built on it.

Fix: either add real `in_text`/`is_thunk` columns to `export.py`'s `functions.csv` (and to
the consumers), or drop the misleading filter + reword the README to "every Ghidra function
minus FID library code". **[needs-build to see the magnitude of the inflation.]**

### M2. Overloaded functions (constructors) share one fingerprint → per-function gating fails for them
`scripts/func_fingerprints.py:159-187` (`body_ranges`).

`acc.setdefault(prefix + nm, []).append(...)` keys by clangd's name. The two `CFileIO`
constructors both clangd-name `CFileIO`, so their ranges are **unioned** under one key and
both get the same hash. Verified in the committed baseline: `??0CFileIO@@QAE@PAX@Z` and
`??0CFileIO@@QAE@XZ` both carry `7e74e5908d69` (the only intra-unit hash collision in the
file).

Effect: editing one overload changes the union hash → BOTH overloads are flagged TOUCHED,
so a real regression introduced in the *other* overload while editing the first is hidden.
This defeats the PR's stated "siblings stay independent" guarantee for any overload set.
The comment at `:163-166` calls the union intentional, but it is unsound for the regression
gate. Today the only affected set is the 2 `CFileIO` ctors; it will recur for every
overloaded method/ctor pulled into a unit.

Fix: disambiguate overloads (e.g. key by `name@startline`, or hash each multi-line def
separately and map each mangled name to the nearest/matching range) instead of unioning.

---

## MINOR

- **m1. No `## ` heading → crash on first README install.** `write_readme` (`:141-143`)
  does `text.index("\n## ")` when the markers are absent; `.index` raises `ValueError` on a
  README with no `## ` heading. Dormant (markers now exist + README has headings). Use
  `find` + guard, or append the block if no heading is found.

- **m2. Units with 0 functions vanish from the `[units]` section.** `unit_counts` (`:237-245`)
  only records a unit when iterating its functions, so a started-but-empty unit never
  appears. Harmless for counts, but a `diff` won't show such a unit appearing/disappearing.

- **m3. Two unrelated `status` concepts.** `gruntz status` (`scripts/gruntz.py:411`,
  `cmd_status`) is the old objdiff summary; `match_status.py status` is the new per-function
  view. `match_status.py` is invoked by path, never wired as a `gruntz` subcommand, so the
  PR title/docstring mention of a "status subcommand" can confuse. Consider a `gruntz
  match-status` passthrough or a doc note.

- **m4. C-name candidate stripping can theoretically collide.** `_candidates`
  (`func_fingerprints.py:195-203`) strips leading `_`/`__` and trailing `@N`, so `_foo`
  and `__foo` both reduce to `foo`. Matching is gated on existence in the unit's clangd
  symbol set and first-match-wins, so a real collision needs two distinct C funcs with the
  same stripped name in one unit — unlikely here, but a latent false-identity path.

- **m5. Doc/data drift.** `docs/match-status.md:54` says "~99/102 functions"; the committed
  baseline has **101** functions (README headline also says 101). Cosmetic.

---

## What's sound

- **Reloc-masking robustness:** fingerprints hash clangd **source ranges**, never objdiff
  bytes, so reloc-name differences in operands cannot move a fingerprint. The
  fuzzy% itself (which reloc-masking lowers) is gated by the best-ever high-water mark, so a
  reloc-masked ~99% function that holds steady is not flagged. Correct.
- **Best-ever high-water mark:** `update` default keeps `max(best, cur)` when the
  fingerprint is unchanged and resets `best ← cur` when it changed (`:305-314`). This is the
  right way to keep an entropy dip from lowering the floor while letting a deliberate rewrite
  clear a stale peak. `--keep-max` / `--accept-regressions` are sensible escape hatches.
- **Entropy doctrine:** `EPS = 0.01%` is correctly scoped to float slop, not entropy
  tolerance; the real entropy defense is the high-water mark + fingerprint gate. The tool
  honestly classifies an unrelated-edit entropy dip (fp unchanged) as REGRESS and documents
  it as a non-fatal review signal — a defensible, honest choice, not a bug.
- **No silent baseline mutation by builds:** `gruntz build` runs `check`/`summary`/cache-refresh
  but **never** `update`, so a build cannot absorb a regression into the committed baseline.
  Good for honesty.
- **Identity join is stable:** unit names line up across `report.json` unit `name`,
  `units.toml` `unit`, `symbol_names.csv`, and the baseline; the `(unit, mangled)` key is
  consistent. Mangled names are objdiff-stable across builds.
- **flake.nix change is correct & minimal:** echo→stderr only, so `--json`/`status` pipes
  cleanly. `llvm-undname` (needed by `func_fingerprints.py`) is already provided by
  `pkgs.llvm` (flake `:181`) — no new/unpinned dependency was needed or added.
- **clangd_query.py change is correct:** opting into `hierarchicalDocumentSymbolSupport`
  and the new `document_symbols` helper are exactly what `body_ranges` needs.
- **Baseline file is internally self-consistent:** 101 function rows, 56 at ≥99.995
  (matches the README "56/101"), `[units]` n_functions sums to 101; deterministic & sorted.
- **Edge handling:** missing report exits with a clear message (`:206`); null
  `fuzzy_match_percent` coerces to 0.0 (`:214`); `_pct` guards div-by-zero.

---

## Overall verdict: **merge-with-fixes**

The architecture is the right one and most of it is correct. Before merge, address **B1**
(make a degraded fingerprint gate impossible to mistake for an all-clear — the stderr
warning added here is a floor, not the fix) and **M2** (overload fingerprint collision).
**M1** should be fixed or the README prose corrected so the published headline matches what
is actually computed. The minors are cleanup.
