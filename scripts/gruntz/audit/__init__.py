"""gruntz.audit - `gruntz audit <tool>` - campaign audits, run on demand as `python -m
gruntz.audit.<name>`. MOST are on-demand only, but NINE are wired as build gates in
cli.py (compgen_order, data_symbol, data_tu_order, label_style, rva_size,
self_recursion, single_view, tu_order_check, view_typedef); the rest of the always-on
gates live in gruntz/cleanliness/ + gruntz/match/:

  reloc      assert_relocs (+ link_defects, its lib) - reloc-target fidelity
  data       data_home / data_tu_order / globals_attribute / shredded -
             the DATA-definition + attribution campaign
  layout     exe_diff (`gruntz audit exe-diff`) / link_order (`gruntz link`) /
             tu_layout + tu_order_check (the .text linker-order invariant) /
             interleavers (COMDAT methods inside foreign units)
  quality    reinterpret_census (cross-cast inventory) / stale_walls
             (re-derive @early-stop blockers) / tidy_audit (`gruntz audit tidy`) /
             extern_harvest (unresolved-referent inventory)
  labeling   fid/ + fid_generate (library_labels.csv regeneration) /
             mfc_class (MFC container-band arbiter) / unmatched_attribute
  rename     rename_member (the m_<hex> endgame bulk renamer) / retype_ints /
             reorder_tu (tu_order_check's mechanical fixer)

Retired audits live in scripts/archive/ (read its README before resurrecting
anything). Shared engines these import live in gruntz/core/.
"""
