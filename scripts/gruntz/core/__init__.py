"""gruntz.core - the shared binary-side library (the "lib crate").

    pe.py       the retail EXE parsed once: sections, relocs, ILT band, the
                whole-.text call index, the string table
    symbols.py  the rva<->name db: symbol_names + ghidra + demangled aliases,
                size-bounded owner attribution, name resolution
    report.py   the objdiff report.json accessor

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
