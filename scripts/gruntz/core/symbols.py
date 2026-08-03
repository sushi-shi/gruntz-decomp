"""gruntz.core.symbols - the rva<->name database, loaded once.

Merges build/gen/symbol_names.csv (matched src/ names + units + sizes - the
reconstructed authority) with the Ghidra exports (functions.csv boundaries,
symbols.csv labels), plus demangled aliases (`CClass::Member`, bare `Member`,
ctor/dtor spellings). Owns size-bounded owner attribution and name->rva
resolution. Get one via gruntz.core.get_context().symbols.
"""
import bisect
import csv
import sys

from gruntz.core.pe import ILT_HI, ILT_LO, REPO

SYMCSV = REPO / "build/gen/symbol_names.csv"
FUNCS = REPO / "build/ghidra-enrich/exports/functions.csv"
GSYMS = REPO / "build/ghidra-enrich/exports/symbols.csv"


# special member codes we can still attribute to a class (ctor/dtor/deleting
# dtors/operators). `?_7`/`?_8`/`?_R*` are vftable/RTTI DATA - skip.
_CTORDTOR = {"?0": "ctor", "?1": "dtor", "?_G": "vec-dtor", "?_E": "scalar-dtor"}


def parse_mangled(m):
    """(class_or_None, member_or_None, access_char_or_None) for a MSVC function
    mangling. No templates in this binary, so `find('@@')` reliably terminates
    the qualified name. Returns None for non-C++ / data-ish symbols."""
    if not m.startswith("?"):
        return None                        # extern "C" (_foo, _foo@N)
    b = m[1:]
    special = None
    if b.startswith("?"):
        b = b[1:]
        if b.startswith("_"):
            special = "?_" + b[1:2]
            b = b[2:]
        else:
            special = "?" + b[:1]
            b = b[1:]
        if special.startswith("?_") and special not in _CTORDTOR:
            return None                    # ?_7 vftable, ?_R0 RTTI, ... = data
    end = b.find("@@")
    if end < 0:
        return None
    tokens = b[:end].split("@")
    rest = b[end + 2:]
    access = rest[0] if rest else None
    if special is not None:
        cls = tokens[0] if tokens else None
        member = _CTORDTOR.get(special, "op")
        return (cls, member, access)
    member = tokens[0] if tokens else None
    cls = tokens[1] if len(tokens) > 1 else None   # None => free function
    return (cls, member, access)


def load_names():
    """(names, byname, fstarts, fsize) from the process-wide cached SymbolDb
    (the ex-xref._names shape)."""
    from gruntz.core import get_context
    db = get_context().symbols
    return db.names, db.byname, db.fstarts, db.fsize


def owner(rva, fstarts, fsize):
    """The recovered function CONTAINING `rva`, or None if `rva` is in an
    unrecovered gap (size-bounded; the ex-xref._owner shape)."""
    k = bisect.bisect_right(fstarts, rva) - 1
    if k < 0:
        return None
    start = fstarts[k]
    sz = fsize.get(start)
    if sz and rva >= start + sz:
        return None
    return start


def _psize(x):
    """Parse a size cell -> int bytes or None. symbol_names is hex (0x2e),
    functions.csv is decimal (46); int(_, 0) reads both. 0/blank -> None."""
    x = str(x).strip()
    if not x:
        return None
    try:
        return int(x, 0) or None
    except ValueError:
        return None


class SymbolDb:
    """names: rva -> (name, unit); kind: rva -> symbol_names kind; byname: exact
    names -> rva (int) and demangled aliases -> {rva,...}; fstarts/fsize: recovered
    function starts + byte sizes (symbol_names sizes win); gsyms: ghidra symbol
    labels (secondary, for data/table naming)."""

    def __init__(self):
        names, kind, fsize, starts, byname, gsyms = {}, {}, {}, set(), {}, {}
        if SYMCSV.exists():
            with open(SYMCSV) as f:
                for r in csv.DictReader(f):
                    try:
                        rva = int(r["rva"], 16)
                    except Exception:
                        continue
                    names[rva] = (r["name"], r.get("unit", ""))
                    kind[rva] = r.get("kind", "")
                    if (r.get("kind") or "func") == "func":
                        starts.add(rva)
                        sz = _psize(r.get("size", ""))
                        if sz:
                            fsize[rva] = sz
        if FUNCS.exists():
            with open(FUNCS) as f:
                for r in csv.DictReader(f):
                    try:
                        rva = int(r["entry_rva"], 16)
                    except Exception:
                        continue
                    starts.add(rva)
                    if rva not in fsize:   # symbol_names size wins
                        sz = _psize(r.get("byte_size", ""))
                        if sz:
                            fsize[rva] = sz
                    names.setdefault(rva, (r["name"], "ghidra"))
                    byname.setdefault(r["name"], rva)
        if GSYMS.exists():
            with open(GSYMS) as f:
                for r in csv.DictReader(f):
                    a = r.get("address_rva", "")
                    try:
                        rva = int(a, 16) if not a.startswith("0x-") else -int(a[3:], 16)
                    except Exception:
                        continue
                    if rva not in gsyms or r.get("is_primary") == "1":
                        gsyms[rva] = r["name"]
                    byname.setdefault(r["name"], rva)
        for rva, (nm, _u) in names.items():
            byname.setdefault(nm, rva)
        # demangled aliases: `CClass::Member` and bare `Member` resolve too. An alias
        # shared by several fns maps to the SET of rvas; resolve() prints candidates.
        for rva, (nm, _u) in names.items():
            if not nm.startswith("?"):
                continue
            pm = parse_mangled(nm)
            if not pm:
                continue
            cls, member, _access = pm
            if member == "ctor":
                member = cls
            elif member == "dtor":
                member = "~" + cls
            elif member in ("vec-dtor", "scalar-dtor", "op"):
                continue                   # no stable spelled name to alias
            for alias in ({f"{cls}::{member}", member} if cls else {member}):
                cur = byname.get(alias)
                if cur is None:
                    byname[alias] = {rva}
                elif isinstance(cur, set):
                    cur.add(rva)
                elif not alias.startswith("?"):
                    byname[alias] = {cur, rva}  # ghidra flat name (often the ILT
                    #                     thunk) + the body: surface both as candidates
        self.names, self.kind, self.byname = names, kind, byname
        self.fstarts, self.fsize, self.gsyms = sorted(starts), fsize, gsyms

    # --- lookups ------------------------------------------------------------
    def name_of(self, rva, default_unit="?"):
        return self.names.get(rva, (f"FUN_{rva:x}", default_unit))

    def label(self, rva):
        nm, unit = self.name_of(rva)
        return f"0x{rva:08x} {nm} [{unit}]"

    def resolve(self, arg):
        """RVA for a hex string / exact mangled / ghidra / `CClass::Member` / bare
        `Member` name. Ambiguity or a miss exits with the candidate list (rc=2)."""
        try:
            return int(arg, 16)
        except ValueError:
            pass
        hit = self.byname.get(arg)
        if hit is None:
            print(f"[symbols] '{arg}' not an RVA and not found in symbol_names/"
                  f"functions.csv (exact mangled, ghidra, `CClass::Member` and bare "
                  f"`Member` names resolve)", file=sys.stderr)
            sys.exit(2)
        if isinstance(hit, int):
            return hit
        if len(hit) == 1:
            return next(iter(hit))
        lines = [f"[symbols] '{arg}' is ambiguous ({len(hit)} functions) - pick one:"]
        for rva in sorted(hit):
            lines.append(f"  {self.label(rva)}")
        print("\n".join(lines), file=sys.stderr)
        sys.exit(2)

    # --- attribution --------------------------------------------------------
    def owner(self, rva):
        """The recovered function CONTAINING `rva`, or None if `rva` is in an
        unrecovered gap (size-bounded so a site past start+size is never pinned
        on the previous fn - that manufactured phantom caller edges)."""
        return owner(rva, self.fstarts, self.fsize)

    def gap_start(self, site):
        """The likely START of the uncarved function containing `site` (which
        owner() couldn't place): the end of the nearest preceding carved function,
        or that function's start if its size is unknown."""
        k = bisect.bisect_right(self.fstarts, site) - 1
        if k < 0:
            return site
        st = self.fstarts[k]
        sz = self.fsize.get(st)
        return st + sz if sz else st

    def is_thunk(self, rva):
        """A pass-through forwarder, not real code: an ILT jump-table entry or a
        Ghidra thunk_* single-jmp forwarder."""
        if ILT_LO <= rva < ILT_HI:
            return True
        return self.name_of(rva)[0].startswith("thunk_")

    def is_matched(self, rva):
        """Reconstructed in src/ (a real symbol_names unit), vs a ghidra-only /
        FUN_ body not matched yet (the attribution frontier)."""
        nm, unit = self.name_of(rva)
        return unit not in ("", "?", "ghidra") and not nm.startswith("FUN_")
