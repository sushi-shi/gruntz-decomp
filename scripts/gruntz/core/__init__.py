"""gruntz.core - the shared engine library (the "lib crate"): everything
imported by more than one package lives here.

    pe.py            the retail EXE parsed once: sections, relocs, ILT band,
                     the whole-.text call index, the string table
    symbols.py       the rva<->name db: symbol_names + ghidra + demangled
                     aliases, size-bounded owner attribution, name resolution
    report.py        the objdiff report.json accessor
    vtable_scan.py   recover every vtable in the EXE (gates/sema/build import it)
    vtable_hierarchy.py  per-class vtable topology (sema class + build gate)
    class_meta.py    src/+include/ class-definition scanner (cleanliness gates
                     + vtable_hierarchy)
    ir.py            clang MSVC-compat flags + emit_ir/load_compdb (labels,
                     caller_callee)
    cc_wrap.py       the `wine cl` compiler wrapper (ninja's cl rule, permute,
                     sema disasm --rich)
    codeview.py      MSVC 5.0 CodeView reader: /Z7 locals + line tables
                     (harvest_locals, sema disasm --rich)
    exe_map.py       queryable .text space map (sema map + the docs/exe-map suite)
    library_labels.py shared HIGH/MED/AMBIG library-carve contract
    clangd_query.py  the clangd LSP client (sema refs/hover/rename, fingerprints)
    data_audit.py    data-section attribution engine (`gruntz data-audit` + build)

One lazily-populated Context per process; every query tool takes it as its
first argument. No on-disk cache - staleness traps (docs/gotchas.md); batch
mode (`gruntz sema -`) is how N queries share one load.
"""


class Context:
    """Lazy handles to the loaded-once state. Cheap to construct."""

    def __init__(self):
        self._pe = None
        self._symbols = None
        self._report = None

    @property
    def pe(self):
        if self._pe is None:
            from gruntz.core.pe import PE
            self._pe = PE()
        return self._pe

    @property
    def symbols(self):
        if self._symbols is None:
            from gruntz.core.symbols import SymbolDb
            self._symbols = SymbolDb()
        return self._symbols

    @property
    def report(self):
        if self._report is None:
            from gruntz.core.report import Report
            self._report = Report()
        return self._report


_CTX = None


def get_context() -> Context:
    """The process-wide Context (one EXE/symbol load per process)."""
    global _CTX
    if _CTX is None:
        _CTX = Context()
    return _CTX


def call_main(module: str, argv: list) -> int:
    """Run a gruntz module's main() IN-PROCESS with a patched sys.argv; returns
    its rc. The single-process replacement for python-subprocess delegation -
    module state (loaded EXE, symbol dbs) caches in sys.modules."""
    import importlib
    import sys
    mod = importlib.import_module(module)
    old = sys.argv
    sys.argv = [module.rsplit(".", 1)[-1], *map(str, argv)]
    try:
        rc = mod.main()
        return rc if isinstance(rc, int) else 0
    except SystemExit as e:
        return e.code if isinstance(e.code, int) else (0 if e.code is None else 1)
    finally:
        sys.argv = old
