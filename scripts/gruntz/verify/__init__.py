"""gruntz.verify - the VERIFY/BANK slice: MAX-ledger banking + the regression
gate + the README score block.

    python3 -m gruntz.verify status        current summary + the regression
                                           report (rva-keyed), bankable
                                           improvements, renames, losses -
                                           exit 0 always (it reports)
    python3 -m gruntz.verify check         same computation; exit nonzero on a
                                           REAL regression (a fresh below-bank
                                           dip, an unbanked loss, or a hard
                                           report failure) - the MAX gate
    python3 -m gruntz.verify bank          preconditions (bankable tree), then
                                           update config/match_baseline.tsv
                                           under the src_hash rules and refresh
                                           the README score block. A MANUAL
                                           act: nothing regenerates the
                                           baseline automatically.
    python3 -m gruntz.verify fingerprints  refresh the per-function source
                                           fingerprint cache (clangd range
                                           hashes; bank runs this itself)
    python3 -m gruntz.verify selftest      NEGATIVE CONTROLS: feed every
                                           ported gate a known violation and
                                           assert it FAILS (a gate nobody has
                                           seen fail is a green light, not a
                                           check), then clean input passes
    python3 -m gruntz.verify <gate>        run one ported gate/audit module
                                           (see the list below); `check
                                           --tier fast|normal|full|link`
                                           runs them in tiers (default
                                           fast,normal - the graph's edge)

Ported doctrine (from the frozen gruntz-old/match/status.py, never imported):
the retail RVA is the BODY's identity (a vanished row whose rva is still
occupied is a rename/move, not a loss; the high-water travels by rva);
best_pct is gated by src_hash (same hash + a different % banks the high mark -
TU composition moved, not the source; a CHANGED hash resets best to cur - the
old peak belonged to source that no longer exists); hist_pct never resets
except when the rva moves under a name.

Input surface: build/objdiff/compare-new/report.json (falling back to
build/objdiff/report.json), config/match_baseline.tsv, the Model
(gruntz.model.resolve) for rva/unit joins, config/units.toml for the module
rollup, and git for the bankable-tree precondition only. Writes:
config/match_baseline.tsv (bank only), README.md's marked block (bank only),
and build/gen/ scratch (the fingerprint cache).
"""

from __future__ import annotations

_SUBS = ("status", "check", "bank", "fingerprints", "selftest")

#: the ported gate/audit modules, runnable as `gruntz verify <name>` (each is
#: also a tier member of `check --tier`; see gruntz.verify.tiers)
_GATES = {"board": "gruntz.verify.board", "bans": "gruntz.verify.bans",
          "casts": "gruntz.verify.casts",
          "enum-domains": "gruntz.verify.enum_domains",
          "label-style": "gruntz.verify.label_style",
          "include-order": "gruntz.verify.include_order",
          "unique-names": "gruntz.verify.unique_names",
          "library-overlap": "gruntz.verify.library_overlap",
          "tu-order": "gruntz.verify.tu_order",
          "data-tu-order": "gruntz.verify.data_tu_order",
          "undefined-closure": "gruntz.verify.undefined_closure",
          "vtables": "gruntz.verify.vtables",
          "vtable-scan": "gruntz.verify.vtable_scan",
          "alloc-size": "gruntz.verify.alloc_size",
          "assert-relocs": "gruntz.verify.assert_relocs",
          "data-relocs": "gruntz.verify.data_relocs",
          "caller-callee": "gruntz.verify.caller_callee",
          "link-tier": "gruntz.verify.link_tier"}


def main(argv=None) -> int:
    import sys
    argv = list(sys.argv[1:] if argv is None else argv)
    known = _SUBS + tuple(_GATES)
    if not argv or argv[0] in ("-h", "--help") or argv[0] not in known:
        print(__doc__.strip())
        print("\ngate modules (also run by `check --tier`): "
              + ", ".join(sorted(_GATES)))
        return 0 if argv and argv[0] in ("-h", "--help") else 2
    sub, rest = argv[0], argv[1:]
    if sub == "fingerprints":
        from gruntz.verify.fingerprints import main as fp_main
        return fp_main(rest)
    if sub == "selftest":
        from gruntz.verify.selftest import main as st_main
        return st_main(rest)
    if sub in _GATES:
        import importlib
        mod = importlib.import_module(_GATES[sub])
        sys.argv = [f"gruntz verify {sub}", *rest]
        return mod.main(rest)
    from gruntz.verify import verbs
    sys.argv = [f"gruntz verify {sub}", *rest]
    return {"status": verbs.cmd_status, "check": verbs.cmd_check,
            "bank": verbs.cmd_bank}[sub](rest)
