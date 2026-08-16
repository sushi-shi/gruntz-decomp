# The match ledger — banked progress and the MAX gate

`gruntz verify` makes one question cheap to answer:

> **did anything regress?** → `gruntz verify check`

It is a small, PDB-free match tracker. We have no target PDBs, so instead of a
database with interning and declaration records we keep a single git-tracked
text file and diff a fresh build against it. Git is the history store.

```
gruntz verify status        # the summary + the rva-keyed regression report; always exit 0
gruntz verify check         # the same computation as a GATE (the graph runs this)
gruntz verify bank          # MANUAL: write config/match_baseline.tsv + the README block
gruntz verify fingerprints  # refresh the per-function source-fingerprint cache
```

## The model

objdiff emits a fuzzy% per function in the compare report
(`units[].functions[].fuzzy_match_percent`). The ledger remembers the
**best-ever** fuzzy% per function in `config/match_baseline.tsv` and reports any
function whose freshly-built fuzzy% sits below its recorded best.

Two ideas, and one high-water mark keyed by the retail body:

- **Keep the max, not the last value.** `best_pct` only ever rises on `bank`; it
  is never silently lowered. So a drop caused by something *unrelated to the
  current edit* — a shared header, a flag tweak, a target-side delink change —
  stays visible until a human looks at it.
- **Identify the body by RVA, not by source text or TU.** Editing a function can
  never lower its high-water mark. A vanished row whose rva is still occupied is
  a **rename or move**, not a loss, and the high-water travels with the rva; if
  the same name appears at a different rva it denotes a different body and starts
  a new history. A retail body with no current source claim keeps its row and is
  reported rather than erased — removing an artificial emitter must not erase a
  proven MAX.

## The two marks: `best_pct` and `hist_pct`

They answer opposite questions and both are load-bearing:

- **`best_pct`** — the best score *this implementation* has reached, and the
  **regression gate**. Same `src_hash` with a different % means TU composition
  moved, so the high mark is banked; a **changed** `src_hash` resets it to
  current, because the old peak belonged to source that no longer exists.
- **`hist_pct`** — the all-time peak any implementation reached. It **never**
  resets, and it is a ratchet per rva: only the rva moving under a name may lower
  it. `hist > best` means **known headroom** — we had a better implementation once
  and lost it — which is a worklist row, and it is the column
  `gruntz walls inventory` sorts the campaign by.

## Per-function fingerprints (clangd)

The fingerprint is **per function**, not per `.cpp`. `gruntz verify fingerprints`
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
the multi-line body range(s) are kept and single-line prototypes dropped.

This is a **derived cache** at `build/clangd/func_fingerprints.tsv` (gitignored),
regenerated incrementally — only TUs whose `.cpp` content hash changed get
re-parsed. It is an edge in the build graph, so a normal `gruntz build` refreshes
it before the gate runs. A function clangd cannot resolve (a compiler-generated
`` `scalar deleting dtor' ``, a WIP unit, a name clangd spells differently) gets
no entry and falls back to a **fallback-tagged** whole-`.cpp` hash (`cpp:<sha>`) —
coarser but always available.

The tag matters: a fallback fingerprint is *unknown*, not a real source change,
so a fingerprint difference only classifies a row as edited (and increments
`tries`) when **both** sides are real. Fingerprints never reset `hist_pct`.

## The baseline file

`config/match_baseline.tsv` — git-tracked, sorted, deterministic; two
TAB-separated sections, hand-greppable and trivial to parse from py/awk. Its own
header block is the schema of record:

```
# [units]      unit  n_functions  matched
# [functions]  unit  function  best_pct  cur_pct  tries  src_hash  rva  hist_pct  state
actionarea     ??0CActionArea@@QAE@PAUCGameObject@@@Z  94.4151  94.4151  1  e6ac9fe56326  0x7da0  94.4151
```

- **`cur_pct`** — fuzzy% at *this bank*. Diff two commits' baselines to see the
  actual moves while `best_pct` holds the high-water mark.
- **`tries`** — how many times this function's `src_hash` changed, i.e. how much
  it has been worked on. High = hard to match.
- **`state`** — empty when the row scored at the last bank; `absent` when it was
  preserved while unscored (no natural emitter in that build). The MAX is kept so
  a later emitter resumes from it instead of forgetting proven work.

`git diff config/match_baseline.tsv` after a bank is itself a readable "what
changed" view.

## Banking is a MANUAL act

`bank` is the only writer of `config/match_baseline.tsv` and of the README's
`match-score` block, and **nothing regenerates them automatically**. It refuses
to bank while a build input has unstaged edits or is untracked (`--dirty`
overrides, loudly): the index is the explicit source snapshot that must be
committed atomically with those generated files. Read-only `status`/`check`
remain available on a dirty tree.

## The gate

`gruntz verify check` is a real gate: it exits non-zero on a **fresh below-bank
dip, an unbanked loss, or a hard report failure**, and it is an edge in the
default build graph (followed by the `fast` and `normal` tiers). `--strict` also
fails on carried (inherited) regressions.

This is deliberate, and it is the doctrine in CLAUDE.md: *ordinary* current-score
movement caused by a correctness fix is expected and is not investigated — the
MAX gate is what decides whether something was actually lost. Matching one
function to 100% can shift a shared TU's codegen and nudge a sibling down 0.1%;
that is not a regression as long as the bank holds.

## The README score block

`bank` renders the score block into `README.md` between
`<!-- match-score:start -->` / `<!-- match-score:end -->`. The important part:
**the totals are weighed against the whole engine, not just the units we have
started.** The compare report only counts functions already pulled into units —
measuring 62/113 there reads as "99% fuzzy" when we have barely begun. So the
denominator is the full reversing target: every in-`.text` reconstruction-target
function, with the generated/library categories (EH funclets, `$E` dyninit
helpers, CRT/MFC static-lib labels, ILT thunks, linker pad) tabled separately and
excluded — see `gruntz.verify.universe` for the precedence that classifies each
census row. Whatever is not started shows up as the `(unmatched)` row at 0%, so
the headline reads honestly, and a second line keeps the started-unit view for
context.

## What is deliberately left out (and why)

No database, no symbol/unit/file interning, no `frameless`/prologue inspection,
no declaration records or `base_only` taxonomy, no statement-level structure
classification. All of those need target PDBs or rich symbol info we do not have,
and pay off only at a scale of thousands of TUs. The useful core — the two
high-water marks, per-function source-fingerprint gating, and the
regress/improve/new/lost categorisation — fits in a text file and a few modules.

See [max-fuzzy-divergence.md](max-fuzzy-divergence.md) for why current fuzzy and
MAX diverge, and `gruntz walls inventory` for the derived worklist ordered by
ascending historical MAX.
