# gruntz-old triage — the port-then-delete ledger

Every module still in `scripts/gruntz-old/` (after the slices already landed:
labels/model/delink/compare/graph/sema/walls, and verify/bank in flight),
classified. Verdicts: **SUPERSEDED** (the new pipeline enforces it
structurally — retire), **VERIFY** (a live gate/ratchet — port into the
verify tiers), **WALLS/SEMA/GRAPH/TOOL** (live, belongs to that package),
**MERGE** (fold into a named sibling check), **PARK** (evidence
generator / campaign instrument — keep the frozen source until needed),
**DEAD** (campaign closed, delete with gruntz-old).

This file dies with `gruntz-old` — it is a work order, not doctrine.

## cleanliness/

| module | verdict | rationale |
|---|---|---|
| board.py | VERIFY (fast) — **DONE** → verify/board.py | the cleanliness scoreboard + baseline delta; semantic rows measured by the full tier (truncated-masks floor carried, its instrument is PARK); gate never writes floors (--update is the bless) |
| caller_callee.py | VERIFY (full) — **DONE** → verify/caller_callee.py | rebuilt on sema image + tool.clang IR + the Model; FAKE-VIEW ratchets vs the board's committed floor (0); other causes reported, not gated |
| class_vtables.py | SUPERSEDED (mostly) | catalog integrity now = data_vtables channel + model violations; residue folded into verify/vtables.py |
| declared_only.py | MERGE → undefined-closure — **DONE** → verify/undefined_closure.py | one closure covers both; baseline mechanism kept (fail-closed when absent; current debt = 0, no file needed) |
| foldable_views.py | DEAD | view-folding campaign helper; views folded |
| view_debt.py | MERGE → undefined-closure — **DONE** | see declared_only; library-shadow check kept |
| vtable_bans.py | VERIFY (fast) — **DONE** → verify/bans.py | regexes pulled from verify/board METRICS so guard and score cannot drift |
| vtable_coverage.py | VERIFY (full) — **DONE** → verify/vtables.py | scan ported to verify/vtable_scan.py (sema.image plumbing); coverage = admitted vtable row OR interior to an admitted claim (closes the zlib config-table gap) |
| vtable_owner.py | SEMA (backlog) | dtor-fingerprint ownership oracle — investigation, not a gate |
| vtable_secondary.py | SUPERSEDED (mostly) | MI names bind through the model/delink; coverage requires MI secondaries admitted (verify/vtables.py) |
| vtable_slot_binding.py | VERIFY (full) — **DONE** → verify/vtables.py | mangled-name virtuality (the compiler's own word); pure fail-closed, the deleted baseline stays gone |
| vtable_virtuality.py | VERIFY (full) — **DONE** → verify/vtables.py | slot counts from the scan; unverifiable rows reported, never asserted |

## audit/

| module | verdict | rationale |
|---|---|---|
| aggregate_copies.py | WALLS (sieve) — **DONE** → walls/aggregate_copies.py | rep-movs aggregate-copy modelling sieve — a diagnose-adjacent lever |
| alloc_size.py | VERIFY (full) — **DONE** → verify/alloc_size.py | `push n; call ??2` sizeof oracle vs clang layout — strong class-layout gate |
| assert_relocs.py | VERIFY (full) — **DONE** → verify/assert_relocs.py | independent reloc-target multiset audit (aliases, final placement) |
| bare_constants.py | PARK | enum campaign instrument; resurrect with the campaign |
| cast_ledger.py | VERIFY (fast) — **DONE** → verify/casts.py | casts→0 is a ruling; the ledger keeps it at 0 |
| channels.py | SUPERSEDED | the model IS the base-superset invariant (unadmitted claim = violation) |
| compgen_data.py | SUPERSEDED | src_data_compgen channel + extraction authority replaced it |
| compgen_order.py | MERGE → label_style residue — **DONE** → verify/label_style.py | pin-placement style; one cheap text check |
| compgen_pins.py | SUPERSEDED | extraction's obj-authority drop/FATAL is exactly this |
| data_access_map.py | VERIFY (full) — **DONE** → verify/data_access.py (engine: verify/access_map.py; field-offset oracle: verify/layout.py) | the retail-side access map, rebuilt on sema.image + tool.objdump + the Model + a pylibclang record-layout oracle; every suppression re-proven site by site (`--suppressed`), gated on width/undercount/shortfall/adjacent, report-only on stride/unclaimed/unaccessed/import-slot |
| data_coverage.py | VERIFY (full) — **DONE** → verify/data_coverage.py | the claim-side gap census, joined with the access map's touched ranges; claim authority is the Model (the manifest-only reading predates the census and reported every unenrolled .bss global as uncovered) |
| data_denominator.py | SUPERSEDED | census complete + model claimed/unclaimed summary |
| data_integrity.py | SUPERSEDED (mostly) | holes = census; referents = delink withholding + objdiff scoring |
| data_layout.py | PARK | the MSVC5 data-layout oracle (docs/compiler-data-layout.md) — instrument |
| data_relocs.py | VERIFY (full) — **DONE** → verify/data_relocs.py | data-side wrong-referent sieve; independent of objdiff's masking |
| data_tu_order.py | VERIFY (normal) — **DONE** → verify/data_tu_order.py | DATA contiguity per TU band (model x link_order join) |
| eh_band.py | SUPERSEDED | delink/eh_band + compare close over EH |
| eh_frame.py | WALLS (sieve) — **DONE** → walls/eh_frame.py | /GX frame-presence source sieve — per-function evidence |
| enum_case_labels.py | PARK | enum campaign rewriter |
| enum_domains.py | VERIFY (fast) — **DONE** → verify/enum_domains.py | GZ_ENUM device invariants — the macros are live in the tree |
| function_census.py | SUPERSEDED | the completeness sweep FATALs a lost label |
| global_refs.py | WALLS (sieve) — **DONE** → walls/global_refs.py | global read-count sieve — the cached-global bug class; a diagnose lever |
| image_diff.py | VERIFY (link) — **DONE (scoped)** → verify/link_tier.py — reloc-masked linked-image diff of exact bodies + staleness guard; the old runtime-defect worklists stay in gruntz-old until needed | candidate-EXE per-segment diff |
| include_order.py | VERIFY (fast) — **DONE** → verify/include_order.py | include block duplicates/order/self-sufficiency |
| init_funclets.py | PARK | XCU-walk oracle; dyninit channel carries the result |
| insn_count.py | SUPERSEDED | walls diagnose counts insns; inventory can grow the column |
| jump_tables.py | SUPERSEDED (mostly) | sema disasm --switch + census extents; extent-includes-table residue → rva-extent check |
| label_style.py | MERGE → verify fast — **DONE** → verify/label_style.py (RE-VERDICT: the frozen canon predated template-id RVA_DYNINIT owners; the ported canon mirrors the live extraction regex) | macro spelling; the sweep already catches vanishing invocations |
| link_defects.py | VERIFY (link) — **DONE (scoped)** → verify/link_tier.py | unlinkable-defect hunt |
| link_line.py | PARK | evidence generator (produced link-order doctrine) |
| link_order.py | PARK | same family |
| link_sections.py | VERIFY (link) — **DONE (scoped)** → verify/link_tier.py (census + absent-section gate; the byte-budget partitions stay in gruntz-old) | candidate section census (graph CR-7) |
| mask_immediates.py | PARK | objdiff scores DIR32 addends now; REL32/imm residue is a walls sieve later |
| max_divergence.py | SUPERSEDED | walls inventory is exactly this |
| mfc_class.py | SEMA (backlog) | MFC container identity oracle |
| nested_static_casts.py | MERGE → cast ledger — **DONE** → verify/casts.py --nested | same campaign, one check |
| rename_member.py | TOOL (backlog) | clangd LSP renamer → tool/clangd + `gruntz lsp` (sema CR-5) |
| rva_size.py | SUPERSEDED | model size authority + crossing violation |
| self_recursion.py | MERGE → cast ledger — **DONE** → verify/casts.py | seam-extraction guard |
| single_view.py | SUPERSEDED | ratchet retired; model data-name uniqueness |
| stale_markers.py | WALLS — **DONE** → walls/stale_markers.py | @early-stop on a 100% fn = stale; an inventory flag |
| stale_walls.py | DEAD | wall prose was stripped; markers-only doctrine |
| strip_wall_prose.py | DEAD | one-shot, executed |
| thunk_oracle.py | PARK | incremental-thunk TU/library oracle (evidence generator) |
| tu_layout.py | DEAD | exploratory analysis |
| tu_order_check.py | VERIFY (normal) — **DONE** → verify/tu_order.py | the contiguous-ascending-band linker invariant |
| unmatched_attribute.py | MERGE → walls inventory — **OPEN (change request)**: inventory.py bodies were out of the port's ownership; add --unclaimed there | add an --unclaimed mode (census rows with no claim) |
| view_typedef.py | SUPERSEDED | reached 0, ratchet retired; regex ban foldable into board |
| wall_reasons.py | DEAD | reason prose is gone |

## match/ (status.py is the in-flight verify/bank port)

| module | verdict | rationale |
|---|---|---|
| fingerprints.py | VERIFY — **DONE** (landed with the verify/bank slice as verify/fingerprints.py) | the src_hash mechanism — the bank's identity gate |
| gate_selftest.py | VERIFY — **DONE** → verify/selftest.py (`gruntz verify selftest`; 56 controls incl. the 16 DATA_COMPGEN refusals) | negative controls; every ported gate gets its failure case (+ the DATA_COMPGEN negctrl set) |
| residual_queue.py | SUPERSEDED | walls inventory (weighting is an easy column) |
| verify_library_overlap.py | MERGE → model/verify — **DONE** → verify/library_overlap.py (RE-VERDICT: LOW-confidence static-lib rows are leads the model filters before resolution, so they no longer participate - deliberate narrowing) | src claim on a static-libs rva — precedence aliases it today; make it loud |
| verify_stubs.py | DEAD | zero `@stub` sites remain in src/ - the convention died with its campaign |
| verify_unique_names.py | VERIFY (normal) — **DONE** → verify/unique_names.py (scoped to src/src_compgen claims; extent overlap is structural in the model, whose violations the same gate FATALs) | function-side name uniqueness — the model only checks data (sema finding #4) |

## init/ + core/ stragglers

| module | verdict | rationale |
|---|---|---|
| init/clangd.py | GRAPH | compdb generation (two agents' CRs) |
| init/toolchain.py | SUPERSEDED | tool/wine init_prefix + create-toolchain-release.py |
| core/exe_map.py, report.py | PARK | the docs exe-map site generator; port when the site next rebuilds |
| core/library_labels.py | PARK | FLIRT label refresh (evidence generator) |
| core/vtable_scan.py, vtable_catalog.py, vtable_hierarchy.py, class_meta.py | VERIFY (with the vtable tier) — **DONE (scoped)**: scan → verify/vtable_scan.py; catalog reads → the Model's channels; class_meta → verify/srcscan.py; hierarchy → the RTTI ancestor walk in verify/alloc_size.py (sema.classof is the live derivation surface; propose sema/ as the scan's long-term home) | the scan feeds coverage; hierarchy feeds `sema class` derivations |
| core/{pe,coff,manifest,ir,dyninit,retail_*,symbols,function_universe,data_universe,codeview,branches,access_map,cc_wrap,clangd_query}.py | SUPERSEDED | absorbed by core/, retail_labels/, delink/, walls/, graph/ |

## The port plan

Tier shape for verify (matching the old fast/normal/full):
**fast** = text/ledger checks (board, cast ledger, bans, include order,
enum devices, label style residue); **normal** = model/layout joins
(unique names, tu order, data tu order); **full** = binary-evidence audits
(caller_callee, alloc_size, assert_relocs, data access map + coverage,
data_relocs, vtable tier); **link** = candidate-EXE audits (sections,
image diff, link defects). Every ported gate arrives WITH its negative
control (gate_selftest doctrine: a gate nobody has seen fail is a green
light, not a check).

Delete `scripts/gruntz-old/` when: verify/bank landed, the VERIFY rows
above are ported (or explicitly re-verdicted), the PARK rows are either
moved to an `attic/` or accepted as recoverable from git history, and the
stale doc references (data_denominator in CLAUDE.md is already fixed;
docs/build-system.md, docs/sema-greenfield.md) are updated.

## Port status (2026-08-15)

The VERIFY/MERGE/WALLS rows above are marked per row. The tier runner is
`gruntz verify check --tier fast|normal|full|link` (default fast,normal);
`gruntz verify selftest` is the negative-control harness (every ported gate
has a demonstrated failure case; 16 DATA_COMPGEN refusal controls folded
in). The graph gained two edges: `verify_fp` (the fingerprint cache, beside
the compare leg) and `verify_check` (MAX gate + fast+normal tiers, after
compare, in `all`). data_access_map + data_coverage LANDED (2026-08-16) as
verify/data_access.py + verify/data_coverage.py over verify/access_map.py
and the new verify/layout.py field-offset oracle; the full tier runs both
and the DEFERRED print is gone. REMAINDER:
unmatched_attribute's --unclaimed mode is an open change request against
walls/inventory.py; image_diff/link_sections are scoped ports (the
gate-bearing checks; the exploratory byte-budget/worklist modes remain in
the frozen tree).
