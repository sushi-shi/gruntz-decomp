# -*- coding: utf-8 -*-
# apply_ghidra_enrichment.py - maximal comprehension enrichment for build/ghidra-named.
#
#   STATELESS: every fact comes from a generated, source-derived file (or the
#   tracked config CSVs), so nothing important lives only in the .gpr blob:
#     1. FUNCTION NAMES   - build/gen/symbol_names.csv (rva -> mangled name; from
#                           src/ RVA() macros via labels.py) + config/library_labels.csv
#                           (FID HIGH/MED/AMBIG). Functions are created when Ghidra
#                           has none at the RVA.
#     2. PROTOTYPES + PARAM NAMES - build/gen/functions.json (labels.py): per-RVA
#                           class / return / calling convention / named params,
#                           from the IR rva-map joined with llvm-undname + the clang
#                           AST. Applied as a typed prototype (+ a struct* `this`).
#     3. LOCALS           - build/gen/locals.json (harvest_locals.py): named stack
#                           variables for BYTE-EXACT functions, read from CodeView in
#                           a /Z7 debug build. Applied as named Ghidra stack vars.
#     4. GLOBAL TYPES     - build/gen/globals.json (labels.py): the declared type of
#                           each named global; laid as typed data (g_buteMgr : CButeMgr).
#     5. STRUCTS / ENUMS  - build/gen/structs.json + enums.json (clang record layouts
#                           over src/ + structure/), defined in the DTM; each struct
#                           is applied as the `this` type on its class's methods.
#   Win32/CRT types resolve against the windows_vs12_32 archive in the DTM; custom
#   types against the generated structs; anything unresolved falls back to void*/int.
#
#   Idempotent: re-runnable, never downgrades a better existing name, keeps prior
#   [LABEL] plate comments. Run as a GhidraScript under PyGhidra (CPython3 + JPype),
#   driven by scripts/gruntz/ghidra/ghidra_metadata_apply.py; invoked via `gruntz
#   init` / `gruntz ghidra-refresh`. Flat-API globals (currentProgram, ...) are
#   injected by pyghidra.ghidra_script.
#
#   Writes build/ghidra-named/exports/enrichment_apply_report.txt
#@category Gruntz
import json
import re
from ghidra.program.model.symbol import SourceType
from ghidra.program.model.listing import CodeUnit, GhidraClass, CommentType
from ghidra.program.model.listing import Function, ParameterImpl, ReturnParameterImpl
from ghidra.program.model.listing.Function import FunctionUpdateType
from ghidra.program.model.data import (
    StructureDataType, EnumDataType, CategoryPath, PointerDataType,
    DataTypeConflictHandler, ArrayDataType, Undefined1DataType, Undefined4DataType)
from ghidra.program.model.address import AddressSet
from java.util import ArrayList

PLATE = CommentType.PLATE
US = SourceType.USER_DEFINED
IMAGE_BASE = 0x400000

import os
ROOT = os.environ.get("GRUNTZ_DIR", "/home/sheep/Projects/gruntz")
# This script is STATELESS - no embedded layouts/enums. All data comes from these
# generated files (+ the config CSVs):
#   build/gen/symbol_names.csv  <- labels.py   (rva -> mangled name, unit)
#   build/gen/functions.json    <- labels.py   (rva -> class/return/cc/named params)
#   build/gen/structs.json      <- ghidra_metadata_generate.py  (clang record layouts of src/ + structure/)
#   build/gen/enums.json        <- ghidra_metadata_generate.py  (clang over structure/)
CSV_SYMBOL   = ROOT + "/build/gen/symbol_names.csv"
FUNCTIONS_JSON = ROOT + "/build/gen/functions.json"
LOCALS_JSON  = ROOT + "/build/gen/locals.json"
GLOBALS_JSON = ROOT + "/build/gen/globals.json"
STRUCTS_JSON = ROOT + "/build/gen/structs.json"
ENUMS_JSON   = ROOT + "/build/gen/enums.json"
CSV_FID    = ROOT + "/config/library_labels.csv"
REPORT     = ROOT + "/build/ghidra-named/exports/enrichment_apply_report.txt"

prog     = currentProgram
st       = prog.getSymbolTable()
fm       = prog.getFunctionManager()
listing  = prog.getListing()
gns      = prog.getGlobalNamespace()
af       = prog.getAddressFactory()
dtm      = prog.getDataTypeManager()
space    = af.getDefaultAddressSpace()

report = []
def R(s):
    report.append(s)
    print(s)

def toaddr(rva):
    return space.getAddress(IMAGE_BASE + rva)


def demangle_apply(addr, mangled):
    """Demangle `mangled` and apply the readable name + namespace + signature at
    addr. Returns True on success.

    Ghidra's Demangler runs as an ANALYSIS pass over imported/auto-named symbols;
    a name set programmatically as USER_DEFINED after analysis is treated as a
    literal and never demangled (so a FID label like `??1CString@@QAE@XZ` would
    otherwise show verbatim with Ghidra's __fastcall guess). We DEMOTE any existing
    USER_DEFINED name to DEFAULT (else the demangler refuses to override it), then
    DemanglerCmd applies the real name (CString::~CString), class namespace, and
    __thiscall(void) signature. Names that aren't mangled (zlib `_adler32`) just
    fail here; the caller restores the literal on a False return."""
    try:
        from ghidra.app.cmd.label import DemanglerCmd
        from ghidra.util.task import TaskMonitor
        prim = st.getPrimarySymbol(addr)
        if prim is not None and prim.getSource() == US:
            try:
                prim.setName(None, SourceType.DEFAULT)  # let the demangler win
            except Exception:
                pass
        return bool(DemanglerCmd(addr, mangled).applyTo(prog, TaskMonitor.DUMMY))
    except Exception:
        return False

# =====================================================================
# 0. CSV loader (respects quoted prototype field)
# =====================================================================
def load_csv_rows(path):
    rows = []
    fh = open(path)
    try:
        for line in fh:
            line = line.rstrip("\n").rstrip("\r")
            if not line or line.startswith("#"):
                continue
            parts = []
            cur = ""
            inq = False
            for ch in line:
                if ch == '"':
                    inq = not inq
                elif ch == "," and not inq:
                    parts.append(cur); cur = ""
                else:
                    cur += ch
            parts.append(cur)
            rows.append(parts)
    finally:
        fh.close()
    return rows

def load_functions_json(path):
    """build/gen/functions.json -> [dict(rva,name,cls,kind,ret,cc,params)].

    labels.py derives each signature from the clang mangledName (undname gives
    return type / calling convention / class / parameter types) overlaid with the
    source AST's parameter names. This is the structured replacement for the
    removed `// engine-label:` JSON `prototype` string.
    """
    if not os.path.exists(path):
        return []
    fh = open(path)
    try:
        data = json.load(fh)
    except Exception:
        return []
    finally:
        fh.close()
    out = []
    for d in data:
        try:
            rva = int(d["rva"], 16)
        except Exception:
            continue
        params = [(p.get("type") or "", p.get("name"))
                  for p in (d.get("params") or [])]
        out.append({
            "rva": rva,
            "name": d.get("name") or "",
            "cls": d.get("class") or None,
            "kind": d.get("kind") or "method",
            "ret": d.get("ret") or "void",
            "cc": d.get("cc") or "__cdecl",
            "params": params,
        })
    return out


def load_locals_json(path):
    """build/gen/locals.json -> {rva: [{name, kind, offset|reg, type}]}.

    harvest_locals.py reads CodeView frame-relative locals from a /Z7 debug build
    of each BYTE-EXACT function; the frame layout there equals the retail one, so
    these stack offsets are valid to inject (the decompiler honours committed
    stack variables on its next on-demand decompile).
    """
    if not os.path.exists(path):
        return {}
    fh = open(path)
    try:
        data = json.load(fh)
    except Exception:
        return {}
    finally:
        fh.close()
    out = {}
    for d in data:
        try:
            rva = int(d["rva"], 16)
        except Exception:
            continue
        out[rva] = d.get("locals") or []
    return out


def load_globals_json(path):
    """build/gen/globals.json -> {rva: ctype-string}. labels.py records the
    declared C/C++ type of each named global; apply.py lays typed data at the
    address so the global decodes as its real type (e.g. g_buteMgr : CButeMgr)."""
    if not os.path.exists(path):
        return {}
    fh = open(path)
    try:
        data = json.load(fh)
    except Exception:
        return {}
    finally:
        fh.close()
    out = {}
    for d in data:
        t = (d.get("type") or "").strip()
        if not t:
            continue
        try:
            out[int(d["rva"], 16)] = t
        except Exception:
            pass
    return out

# =====================================================================
# 1. TYPE RESOLUTION
# =====================================================================
GRUNTZ_CAT = CategoryPath("/Gruntz")

# canonical builtin lookups, cached
_type_cache = {}

# Ghidra built-in primitive singletons, used as a last resort when no type
# archive (windows_vs12_32) is attached so a name like "void"/"int" still
# resolves. On GRUNTZ.EXE the archive provides them and this never fires.
_BUILTIN_PRIMS = None
def _builtin_prim(name):
    global _BUILTIN_PRIMS
    if _BUILTIN_PRIMS is None:
        from ghidra.program.model.data import (
            VoidDataType, IntegerDataType, CharDataType, UnsignedIntegerDataType,
            ShortDataType, LongDataType, FloatDataType, DoubleDataType,
            BooleanDataType, UnsignedCharDataType, UnsignedShortDataType,
            UnsignedLongDataType)
        _BUILTIN_PRIMS = {
            "void": VoidDataType.dataType, "int": IntegerDataType.dataType,
            "char": CharDataType.dataType, "uint": UnsignedIntegerDataType.dataType,
            "short": ShortDataType.dataType, "long": LongDataType.dataType,
            "float": FloatDataType.dataType, "double": DoubleDataType.dataType,
            "bool": BooleanDataType.dataType, "uchar": UnsignedCharDataType.dataType,
            "ushort": UnsignedShortDataType.dataType, "ulong": UnsignedLongDataType.dataType,
        }
    return _BUILTIN_PRIMS.get(name)

def _find_named(name):
    """Resolve a bare datatype name to a DataType (no pointer), or None."""
    if name in _type_cache:
        return _type_cache[name]
    dt = dtm.getDataType("/" + name)
    if dt is None:
        # search anywhere (windows archive categories, /Gruntz, etc.)
        lst = ArrayList()
        dtm.findDataTypes(name, lst)
        if lst.size() > 0:
            dt = lst.get(0)
    if dt is None:
        # last resort: Ghidra's built-in primitive (so void*/int fallbacks and
        # struct fields still resolve when no Windows type archive is attached)
        dt = _builtin_prim(name)
    _type_cache[name] = dt
    return dt

# Win32 / CRT type aliases -> a name we know resolves, else a fallback builtin.
TYPE_ALIASES = {
    "BOOL": "BOOL", "bool": "bool", "void": "void",
    "int": "int", "char": "char", "short": "short", "long": "long",
    "unsigned": "uint", "float": "float", "double": "double",
    "DWORD": "DWORD", "WORD": "ushort", "BYTE": "uchar",
    "UINT": "UINT", "LONG": "long", "ULONG": "ulong",
    "HWND": "HWND", "HKEY": "HKEY", "PHKEY": "PHKEY", "HANDLE": "HANDLE",
    "HINSTANCE": "HINSTANCE", "HACCEL": "HACCEL", "HMODULE": "HMODULE",
    "LRESULT": "LRESULT", "WPARAM": "WPARAM", "LPARAM": "LPARAM",
    "INT_PTR": "INT_PTR", "LPSTR": "LPSTR", "LPCSTR": "LPCSTR",
    "HRESULT": "HRESULT", "WNDCLASSA": "WNDCLASSA", "size_t": "size_t",
    "LPDIRECTPLAYLOBBYA": None, "DPLCONNECTION": None,
}
# Missing-from-archive types -> use a plain pointer-to-void or stub.
MISSING_AS_VOIDP = set([
    "CREATESTRUCTA", "PMODULEENTRY32", "MODULEENTRY32", "LPMSG", "MSG",
])

PTR_FALLBACK = None  # void*
INT_FALLBACK = None

def init_fallbacks():
    # _find_named falls back to Ghidra built-in primitives, so void/int are
    # never None here (avoids JPype's PointerDataType(None) ambiguous overload,
    # which Jython tolerated).
    global PTR_FALLBACK, INT_FALLBACK
    PTR_FALLBACK = PointerDataType(_find_named("void"))
    INT_FALLBACK = _find_named("int")

def resolve_type(ctype):
    """Resolve a C/C++ type string (may have *, &, namespaces) -> DataType."""
    s = ctype.strip()
    if not s:
        return INT_FALLBACK
    # strip leading SAL/MSVC annotations
    s = re.sub(r'\b(_In_opt_|_In_|_Out_opt_|_Out_|_Inout_|const|CALLBACK|WINAPI|__stdcall|__cdecl|__thiscall|struct|class|enum|unsigned\s+(?=char|int|short|long))\b', ' ', s)
    s = s.strip()
    # count pointer / reference depth
    nptr = s.count("*") + s.count("&")
    base = s.replace("*", " ").replace("&", " ").strip()
    base = re.sub(r'\s+', ' ', base)
    # namespace-qualified custom type -> leaf
    leaf = base.split("::")[-1].strip()

    dt = None
    # custom struct?
    if leaf in CUSTOM_TYPE_NAMES:
        dt = _find_named(leaf)
    if dt is None and base in MISSING_AS_VOIDP:
        # treat as opaque pointer target; if no ptr requested make it int-sized stub
        if nptr == 0:
            return INT_FALLBACK
        # one ptr already accounted by treating base as void
        dt = _find_named("void")
        for _ in range(nptr):
            dt = PointerDataType(dt)
        return dt
    if dt is None and leaf in MISSING_AS_VOIDP:
        if nptr == 0:
            return INT_FALLBACK
        dt = _find_named("void")
        for _ in range(nptr):
            dt = PointerDataType(dt)
        return dt
    if dt is None:
        # alias table
        alias = TYPE_ALIASES.get(leaf, None)
        if alias is None and leaf in TYPE_ALIASES:
            # explicitly unmodeled (e.g. LPDIRECTPLAYLOBBYA) -> void* / int
            if nptr > 0:
                dt = _find_named("void")
                for _ in range(nptr):
                    dt = PointerDataType(dt)
                return dt
            return INT_FALLBACK
        if alias is not None:
            dt = _find_named(alias)
        else:
            dt = _find_named(leaf)
    if dt is None:
        # unknown -> void* if pointer, else int
        if nptr > 0:
            dt = _find_named("void")
            for _ in range(nptr):
                dt = PointerDataType(dt)
            return dt
        return INT_FALLBACK
    for _ in range(nptr):
        dt = PointerDataType(dt)
    return dt

# =====================================================================
# 2. STRUCT DEFINITIONS  (from structure/ ; confirmed offsets only for apply)
#    Each: (name, size, [(offset, fieldtype, fieldname), ...], apply_as_this)
#    fieldtype is a C-string resolved via resolve_type at build time.
#    A pad gap left between explicit offsets stays as undefined bytes.
# =====================================================================
# names of structs/classes we DEFINE (so resolve_type can find them) - populated
# in RUN from the generated struct names (build/gen/structs.json), not hardcoded.
CUSTOM_TYPE_NAMES = set()

def get_or_create_struct(name, size, desc, fields):
    """Define (or replace) a StructureDataType in /Gruntz with named fields at offsets."""
    existing = dtm.getDataType(GRUNTZ_CAT, name)
    sdt = StructureDataType(GRUNTZ_CAT, name, size)
    if desc:
        sdt.setDescription(desc)
    for (off, ctype, fname) in fields:
        # array type?  "T[N]"
        m = re.match(r'^(.*)\[(\d+)\]$', ctype.strip())
        if m:
            base = resolve_type(m.group(1).strip())
            n = int(m.group(2))
            try:
                dt = ArrayDataType(base, n, base.getLength())
            except Exception:
                dt = base
        else:
            dt = resolve_type(ctype)
        if dt is None:
            continue
        try:
            sdt.replaceAtOffset(off, dt, dt.getLength(), fname, None)
        except Exception as e:
            # field overruns size or overlaps; skip but keep going
            pass
    added = dtm.addDataType(sdt, DataTypeConflictHandler.REPLACE_HANDLER)
    return added

# =====================================================================
# 3. ENUM DEFINITIONS  (from structure/enums.h)
#    (name, size_bytes, [(enumname, value), ...], description)
#    For taxonomy enums with unverified values, use sequential 0..N (as in header).
# =====================================================================
def define_enum(name, size, desc, members):
    edt = EnumDataType(GRUNTZ_CAT, name, size)
    if desc:
        edt.setDescription(desc)
    for (mn, mv) in members:
        try:
            edt.add(mn, mv)
        except Exception:
            pass
    return dtm.addDataType(edt, DataTypeConflictHandler.REPLACE_HANDLER)

# =====================================================================
# 4. CALLING CONVENTIONS  (structured params come from functions.json)
# =====================================================================
CC_THISCALL = "__thiscall"
CC_CDECL = "__cdecl"
CC_STDCALL = "__stdcall"
_VALID_CC = (CC_THISCALL, CC_CDECL, CC_STDCALL, "__fastcall")

# =====================================================================
# 5. NAMESPACE + NAME HELPERS  (mirrors prior ApplyEngineLabels)
# =====================================================================
ns_cache = {}
def get_class_ns(name):
    if not name:
        return None
    if name in ns_cache:
        return ns_cache[name]
    parent = gns
    for part in name.split("::"):
        existing = st.getNamespace(part, parent)
        if existing is None:
            try:
                existing = st.createClass(parent, part, US)
            except Exception:
                existing = st.createNameSpace(parent, part, US)
        parent = existing
    ns_cache[name] = parent
    return parent

BOGUS = [
    re.compile(r"^\?\?0CMetaFileDC@@"),
    re.compile(r"^\?\?_?[G0]?__non_rtti_object@@"),
    re.compile(r"^\?\?0__non_rtti_object@@"),
    re.compile(r"^opt_get_"), re.compile(r"^opt_set_"),
]
def is_default(nm):
    nm = str(nm)
    return nm.startswith("FUN_") or nm.startswith("thunk_FUN_")
def is_bogus(nm):   return any(p.search(str(nm)) for p in BOGUS)

def ensure_function(addr):
    fn = fm.getFunctionAt(addr)
    if fn is not None:
        return fn
    fn = fm.getFunctionContaining(addr)
    if fn is not None and fn.getEntryPoint().equals(addr):
        return fn
    # create one-instruction function (best effort)
    try:
        from ghidra.app.cmd.function import CreateFunctionCmd
        cmd = CreateFunctionCmd(addr)
        cmd.applyTo(prog)
        fn = fm.getFunctionAt(addr)
    except Exception:
        fn = None
    return fn

# leaf-name -> struct DataType for this-type application
def class_struct(cls):
    if not cls:
        return None
    leaf = cls.split("::")[-1]
    if leaf not in CUSTOM_TYPE_NAMES:    # only classes with a generated struct
        return None
    return dtm.getDataType(GRUNTZ_CAT, leaf)

# =====================================================================
# RUN
# =====================================================================
R("=== Gruntz comprehension enrichment ===")

tx = prog.startTransaction("gruntz-enrichment")
ok = False
try:
    init_fallbacks()

    # ---- (A) define structs (generated JSON preferred; hardcoded = fallback) ----
    import json as _json
    def _load_gen(path, conv):
        try:
            with open(path) as _f:
                data = _json.load(_f)
        except Exception:
            return []
        return [conv(o) for o in data]
    # ghidra_metadata_generate.py JSON: {name,size,fields:[{offset,type,name}],source} ->
    # the (name,size,apply_this,desc,fields) shape this script already consumes.
    ghidra_metadata_generate_list = _load_gen(STRUCTS_JSON, lambda o: (
        o["name"], o["size"], True,
        "generated by ghidra_metadata_generate.py from %s" % o.get("source", "src"),
        [(f["offset"], f["type"], f["name"]) for f in o["fields"]]))
    gen_names = set(s[0] for s in ghidra_metadata_generate_list)
    CUSTOM_TYPE_NAMES.update(gen_names)   # stateless: custom types = the generated structs
    effective_structs = ghidra_metadata_generate_list
    R("structs: %d generated (from src/ + structure/ via clang)" % len(ghidra_metadata_generate_list))
    n_structs = 0
    struct_dt_by_name = {}
    for (name, size, apply_this, desc, fields) in effective_structs:
        dt = get_or_create_struct(name, size, desc, fields)
        struct_dt_by_name[name] = (dt, apply_this)
        n_structs += 1
    # WwdObject + RezDirEntry now come from structure/ headers via ghidra_metadata_generate (clang).

    R("structs defined: %d" % n_structs)

    # ---- (B) define enums (generated JSON preferred; hardcoded = fallback) ----
    gen_enums_list = _load_gen(ENUMS_JSON, lambda o: (
        o["name"], 4, "generated by ghidra_metadata_generate.py from %s" % o.get("source", "src"),
        [(m["name"], m["value"]) for m in o["members"]]))
    effective_enums = gen_enums_list
    n_enums = 0
    for (name, size, desc, members) in effective_enums:
        define_enum(name, size, desc, members)
        n_enums += 1
    R("enums: %d generated (from structure/ via clang)" % len(gen_enums_list))

    # ---- (C) load metadata ----
    eng = load_functions_json(FUNCTIONS_JSON)   # [dict(rva,name,cls,kind,ret,cc,params)]
    R("function signatures loaded: %d (from build/gen/functions.json)" % len(eng))
    locals_map = load_locals_json(LOCALS_JSON)  # {rva: [local,...]} for byte-exact funcs
    R("local-variable sets loaded: %d (from build/gen/locals.json)" % len(locals_map))
    globals_map = load_globals_json(GLOBALS_JSON)  # {rva: ctype} declared global types
    R("global types loaded: %d (from build/gen/globals.json)" % len(globals_map))

    sym_rows = load_csv_rows(CSV_SYMBOL) if os.path.exists(CSV_SYMBOL) else []  # rva,name,unit,size,kind
    syms = []           # functions (kind=func / legacy)
    data_syms = []      # global data (kind=data): mangled name, Ghidra demangles for display
    for r in sym_rows:
        if len(r) < 2: continue
        if r[0] == "rva": continue
        try: rva = int(r[0], 16)
        except Exception: continue
        kind = r[4] if len(r) > 4 else "func"
        (data_syms if kind == "data" else syms).append((rva, r[1]))

    fid_rows = load_csv_rows(CSV_FID) if os.path.exists(CSV_FID) else []  # rva,name,lib,confidence,source
    fids = []
    for r in fid_rows:
        if len(r) < 4: continue
        if r[0] == "rva": continue
        try: rva = int(r[0], 16)
        except Exception: continue
        fids.append((rva, r[1], r[2], r[3]))

    # ---- (D) FID + zlib names (apply first; source stub labels override where overlapping) ----
    # Each mangled name is DEMANGLED + applied (readable name + namespace +
    # signature), falling back to the literal name when it isn't manglable (zlib).
    n_fid_named = 0; n_fid_skip_low = 0; n_fid_nofunc = 0; n_fid_demangled = 0
    for (rva, name, lib, conf) in fids:
        if conf == "LOW":
            n_fid_skip_low += 1
            continue
        addr = toaddr(rva)
        fn = ensure_function(addr)
        if fn is None:
            n_fid_nofunc += 1
            continue
        cur = str(fn.getName())
        # name it when unnamed/bogus OR still showing a mangled string (a prior
        # run set the raw name); a successful demangle leaves a readable name with
        # no '?'/'@', so the next run skips it (idempotent).
        if is_default(cur) or is_bogus(cur) or "?" in cur or "@" in cur:
            if demangle_apply(addr, name):
                n_fid_named += 1; n_fid_demangled += 1
            else:
                try:
                    fn.setName(name, US); n_fid_named += 1
                except Exception:
                    pass
    R("FID/library funcs named (HIGH/MED/AMBIG): %d  (%d demangled, LOW skipped: %d, no-func: %d)" %
      (n_fid_named, n_fid_demangled, n_fid_skip_low, n_fid_nofunc))

    n_sym_named = 0; n_sym_nofunc = 0; n_sym_demangled = 0
    for (rva, name) in syms:
        addr = toaddr(rva)
        fn = ensure_function(addr)
        if fn is None:
            n_sym_nofunc += 1
            continue
        cur = str(fn.getName())
        if is_default(cur) or is_bogus(cur) or "?" in cur or "@" in cur:
            if demangle_apply(addr, name):
                n_sym_named += 1; n_sym_demangled += 1
            else:
                try:
                    fn.setName(name, US); n_sym_named += 1
                except Exception:
                    pass
    R("symbol_names (zlib+ctors) reconciled: %d  (%d demangled, no-func: %d)"
      % (n_sym_named, n_sym_demangled, n_sym_nofunc))

    # Global DATA symbols a matched global is referenced through (labels.py @data
    # rows). The name is the clang MS-ABI mangling (?g_foo@@3.. / _g_foo); Ghidra
    # demangles it for a readable display while the export keeps it for matching.
    n_data_named = 0
    for (rva, name) in data_syms:
        addr = toaddr(rva)
        try:
            prim = st.getPrimarySymbol(addr)
            if prim is not None:
                cur = str(prim.getName())
                if is_default(cur) or cur != name:
                    prim.setName(name, US); n_data_named += 1
            else:
                st.createLabel(addr, name, US); n_data_named += 1
        except Exception:
            pass
    R("global data symbols labeled (Ghidra demangles): %d" % n_data_named)

    # Apply each named global's DECLARED type (labels.py globals.json) so it
    # decodes as its real type (g_buteMgr : CButeMgr, g_gameReg : CGameReg*)
    # instead of raw bytes. resolve_type maps the C/C++ type onto the /Gruntz
    # structs + Win32/CRT archive; CLEAR_ALL_CONFLICT_DATA overwrites the
    # placeholder data Ghidra laid down.
    from ghidra.program.model.data import DataUtilities
    n_data_typed = 0
    for rva, ctype in sorted(globals_map.items()):
        dt = resolve_type(ctype)
        if dt is None:
            continue
        try:
            DataUtilities.createData(prog, toaddr(rva), dt, -1, False,
                                     DataUtilities.ClearDataMode.CLEAR_ALL_CONFLICT_DATA)
            n_data_typed += 1
        except Exception:
            pass
    R("global data symbols typed (declared type applied): %d / %d"
      % (n_data_typed, len(globals_map)))

    # ---- (E) function signatures: name + namespace + plate + PROTOTYPE + this-type ----
    # Each row is a structured signature from build/gen/functions.json (matched
    # functions AND backlog stubs). The mangled symbol_names name set in (D) gives
    # Ghidra the matching name; here we overlay the readable leaf, the class
    # namespace, and a typed/named prototype (with a struct* `this`).
    n_renamed = 0; n_kept = 0; n_ns = 0; n_plate = 0
    n_nofunc = 0; n_created = 0
    n_proto = 0; n_proto_fail = 0
    n_this_applied = 0
    n_local_vars = 0; n_local_funcs = 0
    this_methods_by_class = {}
    proto_examples = []
    _ident = re.compile(r"^[A-Za-z_]\w*$")

    def fmt_proto(ret, qual, params):
        ps = ", ".join((("%s %s" % (t, n)) if n else t) for (t, n) in params) or "void"
        return ("%s %s(%s)" % (ret, qual, ps)).strip()

    for d in eng:
        rva = d["rva"]; name = d["name"]; cls = d["cls"]; kind = d["kind"]
        ret = d["ret"]; cc = d["cc"]; params = d["params"]
        addr = toaddr(rva)
        fn = fm.getFunctionAt(addr)
        if fn is None:
            fn = fm.getFunctionContaining(addr)
        if fn is None:
            fn = ensure_function(addr)
            if fn is not None:
                n_created += 1
        if fn is None:
            n_nofunc += 1
            continue

        # name action: set the readable leaf (the symbol_names mangled name set in
        # (D) is kept for matching via the delinker, not Ghidra display). Skip
        # leaves that aren't plain identifiers (dtors `~Foo`, operators) - those
        # keep the mangled name Ghidra demangles on its own.
        leaf = name.rsplit("::", 1)[-1] if name else ""
        cur = str(fn.getName())
        if leaf and _ident.match(leaf) and cur != leaf:
            try:
                fn.setName(leaf, US); n_renamed += 1
            except Exception:
                try: fn.setName("%s_%06x" % (leaf, rva), US); n_renamed += 1
                except Exception: pass
        else:
            n_kept += 1

        # namespace from the class
        ns = get_class_ns(cls)
        if ns is not None:
            try:
                pn = fn.getParentNamespace()
                if pn is None or str(pn.getName()) != str(ns.getName()):
                    fn.setParentNamespace(ns); n_ns += 1
            except Exception:
                pass

        # plate comment (provenance) - keep prior, append if new
        plate = "[LABEL src] " + fmt_proto(ret, name or "?", params)
        old = listing.getComment(PLATE, addr) or ""
        if "[LABEL " not in old:
            listing.setComment(addr, PLATE, (old + "\n" if old else "") + plate)
            n_plate += 1

        # ---- PROTOTYPE application (typed return + named params + struct* this) ----
        applied_proto = False
        use_cc = cc if cc in _VALID_CC else CC_CDECL
        member_cls = cls if cls else (name.rsplit("::", 1)[0] if "::" in name else None)
        try:
            retparam = ReturnParameterImpl(resolve_type(ret), prog)
            pis = ArrayList()    # JPype matches the List overload only on a real java List
            if use_cc == CC_THISCALL:
                cdt = class_struct(member_cls)
                if cdt is not None:
                    pis.add(ParameterImpl("this", PointerDataType(cdt), prog))
                    this_methods_by_class[member_cls] = this_methods_by_class.get(member_cls, 0) + 1
                    n_this_applied += 1
            for (pct, pname) in params:
                pis.add(ParameterImpl(pname or None, resolve_type(pct), prog))
            fn.setCallingConvention(use_cc)
            fn.updateFunction(use_cc, retparam, pis,
                              FunctionUpdateType.DYNAMIC_STORAGE_FORMAL_PARAMS,
                              True, US)
            n_proto += 1; applied_proto = True
            if len(proto_examples) < 15:
                proto_examples.append("0x%06x %s [%s]" % (rva, fmt_proto(ret, name, params), use_cc))
        except Exception as e:
            n_proto_fail += 1
            if n_proto_fail <= 5:
                R("    proto FAIL 0x%06x %s: %r" % (rva, name, e))

        # ---- this-type fallback for member funcs whose prototype failed ----
        if (not applied_proto) and cls and kind in ("ctor", "dtor", "method", "vfunc"):
            cdt = class_struct(cls)
            if cdt is not None:
                try:
                    new_params = ArrayList()
                    new_params.add(ParameterImpl("this", PointerDataType(cdt), prog))
                    for p in fn.getParameters():
                        if p.isAutoParameter():
                            continue  # the old auto-`this` is replaced by ours
                        new_params.add(
                            ParameterImpl(p.getName(), p.getDataType(), prog))
                    retparam = ReturnParameterImpl(fn.getReturnType(), prog)
                    fn.updateFunction(CC_THISCALL, retparam, new_params,
                                      FunctionUpdateType.DYNAMIC_STORAGE_FORMAL_PARAMS,
                                      True, US)
                    this_methods_by_class[cls] = this_methods_by_class.get(cls, 0) + 1
                    n_this_applied += 1
                except Exception:
                    pass

        # ---- LOCAL VARIABLES (byte-exact funcs only) - name the frame slots ----
        # harvest_locals.py gives the CodeView frame offset of each named local;
        # MSVC's frame base matches Ghidra's (return addr @ 0, params +, locals -),
        # so the offset maps directly. The decompiler shows these on next decompile.
        locs = locals_map.get(rva)
        if locs:
            sf = fn.getStackFrame()
            set_here = 0
            for L in locs:
                if L.get("kind") != "stack":
                    continue
                off, nm = L.get("offset"), L.get("name")
                if off is None or not nm:
                    continue
                try:
                    sf.createVariable(nm, off, Undefined4DataType.dataType, US)
                    set_here += 1
                except Exception:
                    # a Ghidra-inferred variable already covers this slot: just
                    # rename it (keep Ghidra's recovered type).
                    try:
                        v = sf.getVariableContaining(off)
                        if v is not None and not v.isParameter():
                            v.setName(nm, US); set_here += 1
                    except Exception:
                        pass
            if set_here:
                n_local_vars += set_here
                n_local_funcs += 1

    R("function signatures: renamed=%d kept=%d ns-set=%d plate-added=%d created-func=%d no-func=%d"
      % (n_renamed, n_kept, n_ns, n_plate, n_created, n_nofunc))
    R("prototypes applied (typed return + named params): %d  (failed: %d)" % (n_proto, n_proto_fail))
    R("local variables named (byte-exact funcs): %d across %d function(s)" % (n_local_vars, n_local_funcs))
    for e in proto_examples: R("    proto: " + e)
    R("this-type (struct*) applications: %d  across %d classes" %
      (n_this_applied, len(this_methods_by_class)))
    applied_struct_names = sorted(this_methods_by_class.keys())
    for c in applied_struct_names:
        R("    this->%s : %d methods" % (c, this_methods_by_class[c]))

    # struct apply tally
    n_structs_applied = sum(1 for (nm,(dt,ap)) in struct_dt_by_name.items() if ap)
    R("structs flagged apply-as-this: %d / %d defined" % (n_structs_applied, len(struct_dt_by_name)))

    # ---- (F) backfill the leading incremental-linker JMP-thunk table ----
    # MSVC lays a run of 5-byte `jmp target` thunks at the very start of .text
    # (RVA 0x1000), preceded by 0xCC padding. Ghidra 12.0.4's auto-analysis
    # disassembles these jmps but does NOT wrap the first few in Function objects
    # (the older 11.4.2 analysis did). vostok-delinker needs a named function
    # at-or-before EVERY .text reloc target (relocs.rs: range(..=rva).next_back()),
    # so a reloc into an unwrapped leading thunk panics ("all function relocs to
    # be named"). Create a function at each instruction in .text that has no
    # containing function, up to the first already-existing function entry, so the
    # thunk table is fully covered. (Ghidra named the wrapped ones thunk_FUN_*;
    # the new ones get default FUN_* names, which is what the delinker needs.)
    n_thunk_backfill = 0
    try:
        text_block = None
        for blk in prog.getMemory().getBlocks():
            if str(blk.getName()) == ".text":
                text_block = blk
                break
        if text_block is not None:
            first_fn = None
            for f in fm.getFunctions(True):
                first_fn = f
                break
            stop = first_fn.getEntryPoint() if first_fn is not None else text_block.getEnd()
            it = listing.getInstructions(text_block.getStart(), True)
            while it.hasNext():
                inst = it.next()
                ia = inst.getAddress()
                if ia.compareTo(stop) >= 0:
                    break
                if fm.getFunctionContaining(ia) is None:
                    if ensure_function(ia) is not None:
                        n_thunk_backfill += 1
    except Exception as e:
        R("thunk backfill skipped: %s" % e)
    R("leading-thunk functions backfilled: %d" % n_thunk_backfill)

    # ---- coverage ----
    from ghidra.program.model.data import Pointer as _Ptr
    named = 0; funn = 0; thiscall = 0; typed_this_now = 0; named_param = 0
    bare_regions = {}
    for fn in fm.getFunctions(True):
        nm = str(fn.getName())
        frva = fn.getEntryPoint().getOffset() - IMAGE_BASE
        if nm.startswith("FUN_"):
            funn += 1
            b = frva & ~0xFFFF
            bare_regions[b] = bare_regions.get(b, 0) + 1
        else:
            named += 1
        if str(fn.getCallingConventionName()) == "__thiscall":
            thiscall += 1
        ps = fn.getParameters()
        sawnamed = False
        for i, p in enumerate(ps):
            pn = str(p.getName()) if p.getName() is not None else ""
            if pn and not pn.startswith("param_") and not pn.startswith("in_") and pn != "this":
                sawnamed = True
            if i == 0:
                d = p.getDataType()
                base = d.getDataType() if isinstance(d, _Ptr) else d
                if base is not None:
                    cp = base.getCategoryPath()
                    if cp is not None and str(cp.getPath()).startswith("/Gruntz"):
                        typed_this_now += 1
        if sawnamed:
            named_param += 1
    R("[after] total named=%d  FUN_ defaults=%d  __thiscall=%d" % (named, funn, thiscall))
    R("[after] functions w/ typed this (/Gruntz struct*): %d" % typed_this_now)
    R("[after] functions w/ named param(s) (excl this):   %d" % named_param)
    R("=== dominant still-bare FUN_ regions (64KB buckets, top 15) ===")
    for (b, c) in sorted(bare_regions.items(), key=lambda kv: -kv[1])[:15]:
        R("  RVA 0x%06x-0x%06x : %5d bare FUN_" % (b, b + 0xFFFF, c))

    ok = True
finally:
    prog.endTransaction(tx, ok)

# write report
_rep_dir = os.path.dirname(REPORT)
if _rep_dir and not os.path.isdir(_rep_dir):
    os.makedirs(_rep_dir)
fh = open(REPORT, "w")
try:
    for ln in report: fh.write(ln + "\n")
finally:
    fh.close()
print("[apply_ghidra_enrichment] report -> %s" % REPORT)
