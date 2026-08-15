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
| board.py | VERIFY (fast) | the cleanliness scoreboard + baseline delta; drop counters whose subject retired (single-view, view-typedef) |
| caller_callee.py | VERIFY (full) | retail↔reconstruction call-graph reconciliation; found ~13x more defects than reloc fidelity; rebuild on sema's reloc map + extraction IR |
| class_vtables.py | SUPERSEDED (mostly) | catalog integrity now = data_vtables channel + model violations; fold residue into the vtable tier |
| declared_only.py | MERGE → undefined-closure | same mechanism as view_debt: an undefined external defined nowhere in the corpus; one verify check covers both |
| foldable_views.py | DEAD | view-folding campaign helper; views folded |
| view_debt.py | MERGE → undefined-closure | see declared_only |
| vtable_bans.py | VERIFY (fast) | cheap regex ban on the four manual-vtable idioms; keeps a drove-to-0 class at 0 |
| vtable_coverage.py | VERIFY (full) | every image-scanned vtable covered; needs core/vtable_scan ported |
| vtable_owner.py | SEMA (backlog) | dtor-fingerprint ownership oracle — investigation, not a gate |
| vtable_secondary.py | SUPERSEDED (mostly) | MI names now bind through the model/delink; residue folds into the vtable tier |
| vtable_slot_binding.py | VERIFY (full) | slots wired to real virtuals (clang side) |
| vtable_virtuality.py | VERIFY (full) | class declares >= N virtuals per vtable |

## audit/

| module | verdict | rationale |
|---|---|---|
| aggregate_copies.py | WALLS (sieve) | rep-movs aggregate-copy modelling sieve — a diagnose-adjacent lever |
| alloc_size.py | VERIFY (full) | `push n; call ??2` sizeof oracle vs clang layout — strong class-layout gate |
| assert_relocs.py | VERIFY (full) | independent reloc-target multiset audit (aliases, final placement) |
| bare_constants.py | PARK | enum campaign instrument; resurrect with the campaign |
| cast_ledger.py | VERIFY (fast) | casts→0 is a ruling; the ledger keeps it at 0 |
| channels.py | SUPERSEDED | the model IS the base-superset invariant (unadmitted claim = violation) |
| compgen_data.py | SUPERSEDED | src_data_compgen channel + extraction authority replaced it |
| compgen_order.py | MERGE → label_style residue | pin-placement style; one cheap text check |
| compgen_pins.py | SUPERSEDED | extraction's obj-authority drop/FATAL is exactly this |
| data_access_map.py | VERIFY (full) | the retail-side access map — caught real undercount mismodels; the big port |
| data_coverage.py | VERIFY (full) | rides on the access map |
| data_denominator.py | SUPERSEDED | census complete + model claimed/unclaimed summary |
| data_integrity.py | SUPERSEDED (mostly) | holes = census; referents = delink withholding + objdiff scoring |
| data_layout.py | PARK | the MSVC5 data-layout oracle (docs/compiler-data-layout.md) — instrument |
| data_relocs.py | VERIFY (full) | data-side wrong-referent sieve; independent of objdiff's masking |
| data_tu_order.py | VERIFY (normal) | DATA contiguity per TU band (model x link_order join) |
| eh_band.py | SUPERSEDED | delink/eh_band + compare close over EH |
| eh_frame.py | WALLS (sieve) | /GX frame-presence source sieve — per-function evidence |
| enum_case_labels.py | PARK | enum campaign rewriter |
| enum_domains.py | VERIFY (fast) | GZ_ENUM device invariants — the macros are live in the tree |
| function_census.py | SUPERSEDED | the completeness sweep FATALs a lost label |
| global_refs.py | WALLS (sieve) | global read-count sieve — the cached-global bug class; a diagnose lever |
| image_diff.py | VERIFY (link) | candidate-EXE per-segment diff |
| include_order.py | VERIFY (fast) | include block duplicates/order/self-sufficiency |
| init_funclets.py | PARK | XCU-walk oracle; dyninit channel carries the result |
| insn_count.py | SUPERSEDED | walls diagnose counts insns; inventory can grow the column |
| jump_tables.py | SUPERSEDED (mostly) | sema disasm --switch + census extents; extent-includes-table residue → rva-extent check |
| label_style.py | MERGE → verify fast | macro spelling; the sweep already catches vanishing invocations |
| link_defects.py | VERIFY (link) | unlinkable-defect hunt |
| link_line.py | PARK | evidence generator (produced link-order doctrine) |
| link_order.py | PARK | same family |
| link_sections.py | VERIFY (link) | candidate section census (graph CR-7) |
| mask_immediates.py | PARK | objdiff scores DIR32 addends now; REL32/imm residue is a walls sieve later |
| max_divergence.py | SUPERSEDED | walls inventory is exactly this |
| mfc_class.py | SEMA (backlog) | MFC container identity oracle |
| nested_static_casts.py | MERGE → cast ledger | same campaign, one check |
| rename_member.py | TOOL (backlog) | clangd LSP renamer → tool/clangd + `gruntz lsp` (sema CR-5) |
| rva_size.py | SUPERSEDED | model size authority + crossing violation |
| self_recursion.py | MERGE → cast ledger | seam-extraction guard |
| single_view.py | SUPERSEDED | ratchet retired; model data-name uniqueness |
| stale_markers.py | WALLS | @early-stop on a 100% fn = stale; an inventory flag |
| stale_walls.py | DEAD | wall prose was stripped; markers-only doctrine |
| strip_wall_prose.py | DEAD | one-shot, executed |
| thunk_oracle.py | PARK | incremental-thunk TU/library oracle (evidence generator) |
| tu_layout.py | DEAD | exploratory analysis |
| tu_order_check.py | VERIFY (normal) | the contiguous-ascending-band linker invariant |
| unmatched_attribute.py | MERGE → walls inventory | add an --unclaimed mode (census rows with no claim) |
| view_typedef.py | SUPERSEDED | reached 0, ratchet retired; regex ban foldable into board |
| wall_reasons.py | DEAD | reason prose is gone |

## match/ (status.py is the in-flight verify/bank port)

| module | verdict | rationale |
|---|---|---|
| fingerprints.py | VERIFY | the src_hash mechanism — the bank's identity gate |
| gate_selftest.py | VERIFY | negative controls; every ported gate gets its failure case (+ the DATA_COMPGEN negctrl set) |
| residual_queue.py | SUPERSEDED | walls inventory (weighting is an easy column) |
| verify_library_overlap.py | MERGE → model/verify | src claim on a static-libs rva — precedence aliases it today; make it loud |
| verify_stubs.py | DEAD | zero `@stub` sites remain in src/ - the convention died with its campaign |
| verify_unique_names.py | VERIFY (normal) | function-side name uniqueness — the model only checks data (sema finding #4) |

## init/ + core/ stragglers

| module | verdict | rationale |
|---|---|---|
| init/clangd.py | GRAPH | compdb generation (two agents' CRs) |
| init/toolchain.py | SUPERSEDED | tool/wine init_prefix + create-toolchain-release.py |
| core/exe_map.py, report.py | PARK | the docs exe-map site generator; port when the site next rebuilds |
| core/library_labels.py | PARK | FLIRT label refresh (evidence generator) |
| core/vtable_scan.py, vtable_catalog.py, vtable_hierarchy.py, class_meta.py | VERIFY (with the vtable tier) | the scan feeds coverage; hierarchy feeds `sema class` derivations |
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
