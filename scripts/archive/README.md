# scripts/archive — retired tooling (do NOT resurrect)

These scripts served **completed** or **superseded** campaigns. They are kept only for
historical reference / reproducing old audit-ledger commands. **Do not move them back into
the active tree, and do not re-create equivalents.** A new session that "notices a tool is
missing" should read this file first.

**The METRICS stay — only the TOOLS are archived.** The cast / placeholder / fake-view /
fake-virtual counts these tools helped drive to 0 REMAIN LIVE in `gruntz.cleanliness.board`
(and the vtable/view build gates) as **regression guards**: they must hold at 0, and a
non-zero count means a cast or fake-virtual was **reintroduced** — fix it at the source
(re-type the member, dissolve the view, make the virtual real). Do NOT remove those metric
checks; that's the whole point of keeping them. What's gone is only the one-shot *fixer*
tools, because there's nothing left for them to fix.

## One-shot cast converters — cast metrics are all 0 (DONE)
The whole cast-cleanliness campaign is complete: `)this`, `)m_`, `(char*)`, `(const char*)`,
C-style numeric, and offset-cast-macro counts are all **0** in `config/cleanliness-baseline.tsv`.
- `cast_ptr_to_named.py`, `cast_to_static.py`, `cast_str_to_named.py` — AST C-style-cast →
  named-cast converters (byte-neutral, build-verified). Their job is finished.
- `cast_drivers.py` — ranked offset-cast *drivers* (not files); the banned `(char*)x+N` pattern
  is gone, so there are no drivers left to rank.

## Superseded by a better tool
- `reloc_fidelity.py` → **`gruntz.analysis.assert_relocs`** (order-independent, covers
  NEAR-exact functions, ~13× more defects). `assert_relocs.py`'s docstring explains the delta.
- `gen_match_queue.py` (built `config/match-queue.md`) → **`gruntz.match.residual_queue`**
  (the live persistent non-exact queue, `build/gen/residual_function_queue.tsv`, best-first).
- `caller_audit.py` (two-sided "WS5" call-graph audit) → **`gruntz.analysis.caller_callee`**
  (the tracked caller-callee reconciliation metric).

## Completed one-shot sweeps
- `hex_globals.py` — the `g_<hex>` global defect finder; the `g_<hex> globals` metric is **0**.
- `hoist_forward_decls.py` — one-shot "group forward declarations at the top" tidy sweep; done.
- `thunk_alias_dups.py` — thunk-alias duplicate finder; the defect it hunted is resolved.

## 2026-07-21 audit round — zero-reference one-shots (campaigns done)
Retired after a tree-wide reference audit (imports, cli.py, build graph, docs, agent
briefs — all empty for these; audit method in `docs/sema-greenfield.md`'s evidence style):
- `discover_gaps.py` — found+stubbed functions Ghidra never carved; the match queue is
  EMPTY (0 candidates), every function is carved and homed. Done.
- `consolidate_globals.py` — the globals-consolidation campaign concluded in a design
  decision (a fat force-included Globals.h REGRESSES matched TUs); one-shot spent.
- `cleanliness_ast.py` — the libclang AST cast metrics; the whole C-style-cast board is
  **0** and the live regex ratchet in `gruntz.cleanliness.board` guards it.
- `vtable_audit.py` — per-slot virtuality census; superseded by the LIVE build gates
  (`cleanliness/vtable_virtuality.py`, `vtable_coverage.py`, `vtable_slot_binding.py`).
- `vtable_slot_identity.py` — placeholder-slot resolver; the `placeholder vtable slots`
  metric is **0** and every analysed vtable is covered (333/333).

The live tooling that replaces the *purpose* of these lives in `scripts/gruntz/cleanliness/` (the board + the vtable/view gates), `scripts/gruntz/match/`
(`status.py`, `residual_queue.py`), `scripts/gruntz/sema/`
(the navigation surface) and `scripts/gruntz/analysis/` (the still-active campaign
tools). See `docs/gotchas.md` for what's current.
