"""gruntz.audit - `gruntz audit <tool>`, run on demand as `python -m gruntz.audit.<name>`.

Three populations, one directory:

  GATES (wired into `gruntz build` by cli.py; the ratchets and FATAL checks):
    bare_constants channels compgen_data compgen_order compgen_pins
    data_access_map data_denominator data_integrity data_relocs data_tu_order
    eh_band enum_case_labels enum_domains function_census include_order
    label_style link_line rva_size self_recursion single_view tu_order_check
    view_typedef

  ENGINES (imported by the gates / cli / each other, not run directly):
    _textdisasm aggregate_copies cast_ledger data_coverage data_layout eh_frame
    global_refs image_diff insn_count link_defects link_order link_sections
    mask_immediates nested_static_casts rename_member thunk_oracle tu_layout

  HAND TOOLS (the kept on-demand set - each has a named, current consumer):
    stale_walls / stale_markers / wall_reasons / strip_wall_prose - wall
        hygiene: re-derive @early-stop blockers, clear satisfied markers
    assert_relocs   - reloc-target fidelity beyond objdiff's masking
    max_divergence  - historical-MAX vs current divergence (campaign navigation)
    unmatched_attribute - the unclaimed-row worklist with evidence
    init_funclets   - data_compgen class=copy candidate derivation
    jump_tables     - jump-table run classifier (future functions.tsv rows)
    alloc_size      - class sizes from `push <n>; call operator new` sites
    mfc_class       - the binary's own MFC class namer (CRuntimeClass chains)

Retired audits are DELETED, not archived (git history keeps them; do not
resurrect - the campaign conclusions live in ledgers, docs/ and wall-break.md).
"""
