---
name: permute
description: THE way Gruntz breaks a codegen wall - forests x islands, banked by MAX. Use when a function is a COMPLETE, correct reconstruction (right types, right control-flow shape) but plateaus below 100% on MSVC 5.0 /O2 residue - operand-load order, a spill/materialization point, register coloring, instruction scheduling - AND whenever a sibling/neighbour function drops after a correct change (that dip is this skill's job to reclaim, never a reason to revert). Trigger phrases: "climb to 100%", "hit a wall / plateau", "permute", "source permutation", "objdiff won't reach 100", "sibling cratered", "@early-stop but structure is correct". NOT for wrong structure/types/control-flow - fix those by hand first.
version: 0.2.0
---

# Permuter — break the wall: FORESTS × ISLANDS, banked by MAX

**Walls get BROKEN, not documented.** A plateau on a correct reconstruction is a
search problem, and this is the search. `@early-stop` is the rare exception you reach
*after* the search stalls — never the opening move, and never a substitute for running
it. Likewise a **sibling drop is not a problem**: land the byte-evidenced shape and let
this skill reclaim the percent (see the MAX convention in `CLAUDE.md`).

The three pieces, crossed:

- **FORESTS** — the variant tree. Legitimate source *transformations* of the function
  (the AST families: relational/commutative order, statement reorder, decl
  split/merge/hoist, inline extraction, identifier renames — renames steer stack-slot
  and coloring order, automating the hand slot-guess). Each family is a tree of
  spellings; together they are the forest the search walks.
- **ISLANDS** — for EVERY variant, ~32–120 deterministic TU-state permutations
  (`--state-trials N --state-seed S`, families from `tu_state_noise.py`). Each island
  is a different *compiler-state context* for the same source.
- **PERMUTER** — the engine: recompile each cell with the REAL `wine cl`, score the
  COFF against the delinked retail target with `objdiff-cli`. Every mutation is
  semantics-preserving by construction, so a higher byte-score can never come from a
  wrong program.

Score every (transformation, island) cell — **FIRST EXACT WINS** — then **bank the MAX
and restore the cleanest source** (see PRIME SOURCE RULE below).

Why the cross matters: islands ALONE are structurally immune to intra-function
regalloc, and forests alone miss cross-function composition. Crossed, they reach both.

## The default shape of a run: ONE hand-authored Cartesian matrix

**Never test hypotheses as a ladder of one-off edit-compile cycles.** That is the single
biggest waste of a session. Localize the divergence first
(`gruntz sema disasm <rva> --blocks --diff`, then `--branches --diff` when the block view
is clean), then enumerate **every site you suspect** and **every legal spelling per
site**, put the whole family in ONE manifest, and let the engine compile and score the
**Cartesian product** in a single run. N sites × M spellings tests N^M ideas at once
instead of N×M sequential guesses — this is why the matrix beats hand iteration.

    python -m gruntz.permute.match_variants <src.cpp> <rva> --axes-from <axes.json> --run

The manifest is byte-exact substitution only — no regex rewrites — so every option is
authored and semantically reviewed before the run. Put the **full candidate family per
site in one file; never ladder it across runs**, or you lose the interaction effects
that are the whole point.

**Exact axes-file schema** (get this wrong and the run dies on validation):

```json
{ "schema": 1, "source": "src/Bute/ButeMgr.cpp", "rva": "0x171640",
  "axes": [ { "name": "outbuf_pos",
              "find": "<byte-exact span, must occur EXACTLY ONCE in the file>",
              "options": [ {"name": "keep"},                       // no "replace" = identity
                           {"name": "after_init", "replace": "..."} ] } ] }
```

Traps, all hit in one session: options are **objects** (`{"name","replace"}`), not bare
strings; `source`/`rva` must match the generated manifest or `--axes-from` refuses it;
`--max-depth 0` writes `"candidates": []` and the batch runner then rejects the file —
**delete the empty `candidates` key** before running; and `--axes-from` needs a separate
`batch_source_variants` invocation (passing `--run` with `--limit/--top` errors out):

    python -m gruntz.permute.match_variants <src> <rva> --max-depth 0 --axes-from a.json -o m.json
    python - <<'EOF'  # strip empty candidates
    import json; p='m.json'; d=json.load(open(p)); d.pop('candidates',None); json.dump(d,open(p,'w'))
    EOF
    python -m gruntz.permute.batch_source_variants m.json --limit <product> --top 8

**`--max-depth` defaults to 0: generate NO AST trees.** Two reasons, both measured:
at `/O2` a generated tree is rarely the answer (the wins come from spellings derived
from the *disassembly*), and the axes product is already exponential — a generated tree
is another multiplier on top of it. Opt in explicitly (`--max-depth 2`) when you have a
reason. Same for `--state-trials` (the declaration-forest islands): measured on this
project across three runs of 400 / 280 / 300 cells, the pure-island axis moved
**nothing**, while hand-authored forests off the disassembly produced every win. Islands
are for a structurally aligned residual with unchanged source, not a first resort.

Judge survivors by **block topology and ordered relocations**, not fuzzy score alone.

## When to use it (and when NOT)

USE it when a function is **already the right shape** — correct types (no casts/views
standing in for real classes), correct control flow, correct call/EH conventions —
and the only thing between it and 100% is codegen ordering the source can't directly
pin. This is the automated form of the by-hand spelling chase.

Do **NOT** reach for it to paper over a wrong reconstruction. It cannot fix:
- a **control-flow-shape** mismatch (that needs the real `for`/`goto`/`while`/early-exit
  restructuring done by hand — the permuter does not restructure control flow), or
- **wrong types / a cast-hacked view** (fix the class model first — matcher.md rule 0).

**`match_variants --state-trials` moves ONLY cross-function-dependent walls — it is
structurally immune to INTRA-function regalloc (empirically proven).** The exhaustive
engine's `--state-trials` perturbs the TU content emitted *before* the target
(declarations/includes), so it can only change codegen that depends on cross-function
composition: inlining budget, COMDAT/string ordering, cross-function scheduling. It
CANNOT move a wall that comes from the function's OWN dataflow — register *coloring*
(`ebx` vs `edi` for `this`), SIB base/index role, a spill decision, partial-register
width (`and al` vs `and eax`), callee-saved coalescing (frame `0x80` vs `0x70`). A
wall-breaker experiment ran 4 such families (one at 1024 variants) and moved **zero**.
So do NOT spend `--state-trials` ALONE on a documented intra-function regalloc/SIB/
spill/width wall; a pure island sweep of the unchanged source cannot re-color it. But
see the CROSSED search below — islands multiplied against source transformations are
a different animal from islands alone.

## Running the crossed search, step by step

The sibling homm2 campaign closed its residual tail with exactly this Cartesian
search (homm2/RECOVERY.md "THE METHOD"); its mechanics apply here verbatim.

1. **Forests (the axes)**: enumerate LEGITIMATE source transformations of the function
   (the AST families: relational/commutative order, statement reorder, decl
   split/merge/hoist, inline extraction, identifier renames — renames steer
   stack-slot/coloring order and are exactly the axis that automates hand slot-guessing).
2. **Islands**: for EVERY transformation, ~32-120 deterministic TU-state permutations
   (`--state-trials N --state-seed S`, families from `tu_state_noise.py`) — each island
   is a different compiler-state context for the SAME variant.
3. Score every (transformation, island) cell. FIRST EXACT WINS. Then:
   **bank the MAX and RESTORE THE CLEANEST SOURCE.** gruntz has no `--record-max`
   flag (homm2 grew one); the manual recipe: apply the audited `exact.cpp`,
   `gruntz build --fast` (the per-function MAX ledger banks the observed 100), then
   restore the clean spelling and rebuild. The exact was OBSERVED and is permanently
   credited; the committed source stays dev-plausible.
4. If the best ceiling stalls across all transformations, **the ceiling's diff EXPOSES
   the structural error** — a wrong type/layout/control shape in OUR reconstruction.
   Fix THAT and re-run. (This is the mislabeled-correctness-bug rule in search form.)
   **This is the normal outcome, not the exception** — measured on `CButeMgr::Save`
   (2026-08-02): a 192-cell hand matrix, a 180-cell rename forest and the generated
   AST tree moved the score by **1 point total**, while the divergences those flat
   runs pointed at carried it 46 → 87. The matrix's real product is a *ranked list of
   places the source is wrong*; read the survivors' diff, do not admire the score.
   Specific reads that paid there, all reusable:
   - **an inlined CRT idiom you mistook for a call you already have** — an
     `ios::clear()` shows up as a lock-enter / `state=0` / lock-leave triple, NOT as a
     call; `seekg(0)` was our wrong guess for it (+8.4).
   - **`neg`/`sbb`/`and` on an address = cl5's null-checked base-offset upcast**
     (`p ? p+delta : 0`). It only appears when the argument is a POINTER expression, so
     it tells you a local is `T*`, not `T&` — a 4-byte frame slot you were missing.
   - **`mov ecx, <lvalue>` before a "free function" call = it is a `__thiscall`
     member.** Check EVERY retail call site (`sema xref` the callee): three independent
     sites agreeing turns a guess into proof, and the delinker's own offset-derived
     member name in the target obj (`g_x.m_10f`) corroborates for free.
   - **a uniform N-byte shift across every frame offset** is one missing local, not
     entropy — but probing for it with unused decls fails (cl elides them); find the
     *use* that forces the slot.
5. **Hash-scoped completion**: never re-search byte-identical source — only a new
   transformation (new hash) buys new information. Log per-function results; dedupe
   the worklist against functions whose current hash was already searched.
6. **Epoch discipline**: a stale report breeds phantom residuals. Full `gruntz build`
   → fresh `residual_queue` → verify any surprising row with `objdiff-cli diff`
   before spending search time on it.

**PRIME SOURCE RULE (user directive, both campaigns): we write code the way the
original devs wrote it.** No contorted spellings, no steering macros, no index-flipped
subscripts in committed source. A grotesque spelling that scores 100 is WORSE than a
clean 99.9 — the search exists so the clean spelling gets the credit via MAX.

**The real high-yield move on a "regalloc wall" is to suspect a MISLABELED CORRECTNESS
BUG.** A large fraction of `@early-stop` "walls" are a hidden source bug the diff masks:
a signedness slip (`jl/jle` vs retail `jb/jbe` — cast the loop guard to `u32`), a wrong
magic constant (`objdiff --diff` masks large immediates as `<addr>` — verify with
`--base`; a `/9`-vs-`/30` divisor showed only as a downstream shift), a missed CSE, a
dropped member/vtable stamp. Those are hand-fixable and bank permanently (FadeRange
99.1→99.9 was a mislabeled signedness bug). Re-audit the disasm before believing "wall".

The fast `permute` pass (operand-order/reassoc/decl-split) still gives *incremental*
nudges on a genuinely-correct body; MAX fuzzy (best-ever) banks any 100% it reaches.

## THE methodology: top-down in source order

**MSVC /O2 codegen can interact across a translation unit.** Permute a TU's functions
**from the top of the source downwards** — lock in each function before the ones below
it. If you permute a lower function first and then change an upper one, the upper
change can shift the lower function's codegen and invalidate the win. `permute_sweep`
enforces this ordering; when driving `permute` by hand, honor it yourself.

## Tools

Run inside `nix develop .#build`, from the repo/worktree root, AFTER a `gruntz build`
(so `build/objdiff/{target/*.c.obj, report.json}` and `build/gen/symbol_names.csv`
are current). The `<unit>` is the `unit` stem from `config/units.toml`; the
`<mangled-sym>` is the exact mangled name (from the objdiff row / `llvm-nm` the base obj).

- **One function:**
  ```
  gruntz permute fn <src.cpp> <unit> <mangled-sym> [iters]
  # e.g.
  gruntz permute fn src/Gruntz/GameLevel.cpp gamelevel \
      '?ProbeFootBlocked@CGameLevel@@AAEHPAUCGameObject@@H@Z' 400
  ```
  Edits `<src.cpp>` IN PLACE, leaving the best-scoring variant (prints `start`/`FINAL`).
  Interrupt-safe: a `finally` always writes back the best-known variant (never worse
  than the original).

- **A whole unit, top-down (preferred):**
  ```
  gruntz permute sweep <unit> [iters]
  # e.g.
  gruntz permute sweep gamelevel 60
  ```
  Discovers the unit's `<100%` functions in SOURCE order and permutes each in turn,
  accumulating wins. Prints a per-function `before -> after` line and a WIN summary.

- **Exhaustive engine (escalation for a function `permute` can't move):**
  ```
  gruntz permute variants <src.cpp> <rva> \
      --max-depth 3 --limit 512 -o /tmp/m.json --run --top 12
  ```
  The fuller homm2 engine. Where `permute` is a greedy random-walk over a few mutation
  families, `match_variants` GENERATES an exhaustive, non-overlapping AST-mutation set
  (commutative/relational order, decl split/merge/hoist, expression / read-advance /
  nested / member-access inline extraction, identifier rename) with libclang, then
  compiles+scores the whole Cartesian product and reports the best, gated on an EXACT
  CLOSURE (objdiff score == 100.0 **and** size == retail **and** the ordered relocation
  stream == retail). It does NOT edit the source in place — it writes candidates to a
  manifest and an audited `exact.cpp` only on a real closure; apply that by hand. Add
  `--state-trials N` to also search TU-state (declarations/includes emitted before the
  target) when a function is stuck on cross-function codegen steering. Reach for it when
  `permute` plateaus but the reconstruction is provably correct.

## After a run

`git diff` the source, then **rebuild and confirm** (`gruntz build`) before committing —
the permuter's own score is a fast objdiff read, but the build is the source of truth
and re-checks the whole unit. Commit the wins; each is matching-positive and
semantics-preserving. If a function did not move, that residual is a genuine wall
(regalloc-coloring or a control-flow shape) — leave the `@early-stop` with its
byte-level reason.

## How it works (three seams that matter)

- **Mutations come from a real clang AST (libclang), so they are precedence-correct.**
  For a commutative binary operator the two operand SUB-EXPRESSIONS are read from the
  parse tree — the RHS of `a + b*c` is the whole `b*c` — so swapping is value-preserving
  by construction. (The earlier regex form was operator-precedence-blind and banked WRONG
  code, e.g. `cells + width*y + x` → `cells + y*x + width`; that is why it is AST-based.)
  Source is handled as BYTES to match clang's byte offsets (one non-ASCII byte would
  otherwise drift char offsets and splice wrong). Needs `ps.libclang` in the flake.
- **Scoring is per-symbol.** `objdiff-cli` ignores its `<symbol>` arg for JSON output
  and emits every symbol in the TU; the tool pulls the TARGET symbol's `match_percent`
  by exact mangled name. (A global max would saturate at 100 the instant any sibling
  is 100% — and every real Gruntz TU has 100% siblings.)
- **Mutations are scoped** to the target function's `RVA()`-marker span (via
  `build/gen/symbol_names.csv`), so a run is fast and **cannot regress a 100% sibling**
  (a stacked mutation otherwise could, since scoring only reads the target).

Ported from the HoMM2 sibling decomp (same units.toml + cc_wrap + objdiff-cli pipeline),
then hardened onto a clang AST. HoMM2's fuller multi-family generator + exact-closure
batch runner are also ported here as `gruntz permute variants` (module: gruntz/permute/match_variants.py) (see the escalation
above); `permute` remains the fast iterative first pass.
