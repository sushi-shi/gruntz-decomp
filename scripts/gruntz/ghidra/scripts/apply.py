# -*- coding: utf-8 -*-
# apply_ghidra_enrichment.py - maximal comprehension enrichment for build/ghidra-named.
#
#   Extends (does NOT clobber) the prior name+plate enrichment with:
#     1. FUNCTION NAMES   - src/**/*.cpp source stub metadata (incl. import-caller rows),
#                           config/library_labels.csv (HIGH/MED/AMBIG),
#                           config/symbol_names.csv (zlib + ctors).
#                           Functions are created when Ghidra has none at the RVA.
#     2. PROTOTYPES + PARAM NAMES - parsed from source stub `prototype` metadata
#                           (tomalla rows). Sets return type, params (name+type),
#                           and calling convention (__thiscall members / __cdecl
#                           free funcs / __stdcall callbacks). Win32 types resolve
#                           against the windows_vs12_32 archive already in the DTM;
#                           custom struct types resolve against the structs defined
#                           below; anything unresolved falls back to void*/int.
#     3. STRUCTS (field names @ offsets) - from build/gen/structs.json (clang record
#                           layouts over src/ + src/Stub/types/), defined in the DTM and
#                           APPLIED as the `this` (param-0) type on every method of
#                           that class (so member access decodes as this->m_health).
#     4. ENUMS            - from build/gen/enums.json (clang over src/Stub/types/),
#                           defined in the DTM.
#
#   Reproducible from src/**/*.cpp @stub metadata + config/symbol_names.csv +
#   config/library_labels.csv + src/Stub/types/. Idempotent: re-runnable, never
#   downgrades a better existing name, keeps prior [LABEL] plate comments.
#
#   Run as a GhidraScript under PyGhidra (CPython3 + JPype), driven by
#   scripts/gruntz/ghidra/ghidra_metadata_apply.py (which boots PyGhidra, imports/analyzes
#   GRUNTZ.EXE, then runs this + export.py). Invoked via `gruntz init` /
#   `gruntz ghidra-refresh`. The flat-API globals (currentProgram, monitor, ...)
#   are injected by pyghidra.ghidra_script.
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
    DataTypeConflictHandler, ArrayDataType, Undefined1DataType)
from ghidra.program.model.address import AddressSet
from java.util import ArrayList

PLATE = CommentType.PLATE
US = SourceType.USER_DEFINED
IMAGE_BASE = 0x400000

import os
ROOT = os.environ.get("GRUNTZ_DIR", "/home/sheep/Projects/gruntz")
STUB_LABEL_ROOT = ROOT + "/src"
# This script is STATELESS - no embedded layouts/enums. All data comes from these
# generated files (+ the config CSVs):
#   build/gen/symbol_names.csv  <- gen_labels.py   (rva -> mangled name, unit)
#   build/gen/structs.json      <- ghidra_metadata_generate.py  (clang record layouts of src/ + src/Stub/types/)
#   build/gen/enums.json        <- ghidra_metadata_generate.py  (clang over src/Stub/types/)
CSV_SYMBOL   = ROOT + "/build/gen/symbol_names.csv"
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

def load_stub_label_rows(path):
    rows = []
    marker = "// engine-label:"
    if os.path.isdir(path):
        files = []
        for base, _dirs, names in os.walk(path):
            for name in names:
                if name.endswith(".cpp"):
                    files.append(os.path.join(base, name))
        files.sort()
    else:
        files = [path]
    for file_path in files:
        fh = open(file_path)
        try:
            for line in fh:
                if marker not in line:
                    continue
                try:
                    row = json.loads(line.split(marker, 1)[1].strip())
                except Exception:
                    continue
                rows.append((
                    row.get("rva", ""),
                    row.get("name", ""),
                    row.get("class", ""),
                    row.get("prototype", ""),
                    row.get("kind", ""),
                    row.get("source", ""),
                    row.get("confidence", ""),
                ))
        finally:
            fh.close()
    return rows

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
# 2. STRUCT DEFINITIONS  (from src/Stub/types/ ; confirmed offsets only for apply)
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
# 3. ENUM DEFINITIONS  (from src/Stub/types/enums.h)
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
# 4. PROTOTYPE PARSING
# =====================================================================
# strip ICF-folded annotations + initializer lists
ICF_RE = re.compile(r'\s*\[ICF-folded:[^\]]*\]\s*$')
def clean_proto(p):
    p = ICF_RE.sub('', p).strip()
    # drop C++ ctor initializer list  ") : Base(...)"
    idx = p.find(') :')
    if idx != -1:
        p = p[:idx+1]
    return p

CC_THISCALL = "__thiscall"
CC_CDECL = "__cdecl"
CC_STDCALL = "__stdcall"

def split_params(argstr):
    """Split a parameter list on top-level commas."""
    out = []
    depth = 0
    cur = ""
    for ch in argstr:
        if ch in "<([":
            depth += 1; cur += ch
        elif ch in ">)]":
            depth -= 1; cur += ch
        elif ch == "," and depth == 0:
            out.append(cur); cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur)
    return [x.strip() for x in out]

def parse_param(ptxt):
    """Split a single 'type name' param into (ctype, name)."""
    t = re.sub(r'\b(_In_opt_|_In_|_Out_opt_|_Out_|_Inout_)\b', ' ', ptxt).strip()
    if t == "void" or t == "":
        return None
    # name = last identifier token not attached to *; type = rest
    m = re.match(r'^(.*?)([A-Za-z_]\w*)\s*$', t)
    if not m:
        return (t, None)
    base = m.group(1).strip()
    nm = m.group(2)
    if base == "":
        # only a type with no separate name (e.g. "HWND")
        return (nm, None)
    # if base ends without * and 'nm' is actually part of type (rare), still ok
    return (base, nm)

PROTO_RE = re.compile(r'^(.*?)\b([A-Za-z_][\w:]*)\s*\((.*)\)\s*$')

def parse_prototype(proto, kind):
    """Return (ret_ctype, cc, [(ctype,name),...]) or None."""
    p = clean_proto(proto)
    if not p or p.startswith("vftable slot"):
        return None
    if "(" not in p:
        return None
    m = PROTO_RE.match(p)
    if not m:
        return None
    pre = m.group(1).strip()    # return type + maybe CALLBACK/WINAPI
    funcname = m.group(2)
    args = m.group(3).strip()
    # calling convention
    cc = CC_CDECL
    if "CALLBACK" in pre or "WINAPI" in pre or "__stdcall" in proto:
        cc = CC_STDCALL
    # strip cc markers from return type
    ret = re.sub(r'\b(CALLBACK|WINAPI|__stdcall|__cdecl|__thiscall)\b', ' ', pre).strip()
    if ret == "":
        # ctor/dtor: no explicit return
        ret = "void"
    # member function? (qualified name with ::) and not callback -> thiscall
    is_member = "::" in funcname
    if kind in ("ctor", "dtor", "method", "vfunc") and cc == CC_CDECL:
        cc = CC_THISCALL
    elif is_member and cc == CC_CDECL:
        cc = CC_THISCALL
    # params
    params = []
    if args and args != "void":
        for ptxt in split_params(args):
            pr = parse_param(ptxt)
            if pr is None:
                continue
            params.append(pr)
    return (ret, cc, params, is_member)

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
    R("structs: %d generated (from src/ + src/Stub/types/ via clang)" % len(ghidra_metadata_generate_list))
    n_structs = 0
    struct_dt_by_name = {}
    for (name, size, apply_this, desc, fields) in effective_structs:
        dt = get_or_create_struct(name, size, desc, fields)
        struct_dt_by_name[name] = (dt, apply_this)
        n_structs += 1
    # WwdObject + RezDirEntry now come from src/Stub/types/ headers via ghidra_metadata_generate (clang).

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
    R("enums: %d generated (from src/Stub/types/ via clang)" % len(gen_enums_list))

    # ---- (C) load metadata ----
    eng_rows = load_stub_label_rows(STUB_LABEL_ROOT)   # rva,name,class,prototype,kind,source,confidence
    eng = []
    for r in eng_rows:
        if len(r) < 7: continue
        if r[0] == "rva": continue
        try:
            rva = int(r[0], 16)
        except Exception:
            continue
        eng.append((rva, r[1], r[2], r[3], r[4], r[5], r[6]))

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
    n_fid_named = 0; n_fid_skip_low = 0; n_fid_nofunc = 0
    for (rva, name, lib, conf) in fids:
        if conf == "LOW":
            n_fid_skip_low += 1
            continue
        addr = toaddr(rva)
        fn = ensure_function(addr)
        if fn is None:
            n_fid_nofunc += 1
            continue
        cur = fn.getName()
        if is_default(cur) or is_bogus(cur):
            try:
                fn.setName(name, US); n_fid_named += 1
            except Exception:
                pass
    R("FID/library funcs named (HIGH/MED/AMBIG): %d  (LOW skipped: %d, no-func: %d)" %
      (n_fid_named, n_fid_skip_low, n_fid_nofunc))

    n_sym_named = 0; n_sym_nofunc = 0
    for (rva, name) in syms:
        addr = toaddr(rva)
        fn = ensure_function(addr)
        if fn is None:
            n_sym_nofunc += 1
            continue
        cur = str(fn.getName())
        if is_default(cur) or is_bogus(cur) or cur != name:
            try:
                fn.setName(name, US); n_sym_named += 1
            except Exception:
                pass
    R("symbol_names (zlib+ctors) reconciled: %d  (no-func: %d)" % (n_sym_named, n_sym_nofunc))

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

    # ---- (E) source stub labels: names + namespace + plate + PROTOTYPES + this-type ----
    n_renamed = 0; n_bogus = 0; n_kept = 0; n_ns = 0; n_plate = 0
    n_nofunc = 0; n_created = 0
    n_proto = 0; n_proto_fail = 0
    n_this_applied = 0
    this_methods_by_class = {}
    proto_examples = []

    for (rva, name, cls, proto, kind, source, conf) in eng:
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

        leaf = name.split("::")[-1] if name else ""
        cur = fn.getName()

        # name action (skip if no leaf, e.g. import-caller rows)
        if leaf:
            if is_default(cur):
                try: fn.setName(leaf, US); n_renamed += 1
                except Exception:
                    try: fn.setName("%s_%06x" % (leaf, rva), US); n_renamed += 1
                    except Exception: pass
            elif is_bogus(cur):
                try: fn.setName(leaf, US); n_bogus += 1
                except Exception: pass
            else:
                n_kept += 1

        # namespace
        ns = get_class_ns(cls)
        if ns is not None:
            try:
                pn = fn.getParentNamespace()
                if pn is None or str(pn.getName()) != str(ns.getName()):
                    fn.setParentNamespace(ns); n_ns += 1
            except Exception:
                pass

        # plate comment (provenance) - keep prior, append if new
        if kind == "import-caller":
            plate = "[LABEL %s/%s] import-caller; class=%s; %s" % (source, conf, (cls or "?"), name or "")
        else:
            plate = "[LABEL %s/%s] %s" % (source, conf, (proto if proto else name))
        old = listing.getComment(PLATE, addr) or ""
        if "[LABEL " not in old:
            listing.setComment(addr, PLATE, (old + "\n" if old else "") + plate)
            n_plate += 1

        # ---- PROTOTYPE application (tomalla rows with a real signature) ----
        applied_proto = False
        if proto and source == "tomalla" and not proto.startswith("vftable slot"):
            parsed = parse_prototype(proto, kind)
            if parsed is not None:
                ret_ct, cc, params, is_member = parsed
                try:
                    ret_dt = resolve_type(ret_ct)
                    retparam = ReturnParameterImpl(ret_dt, prog)
                    pis = []
                    # this-type for member functions
                    member_cls = cls if cls else (name.rsplit("::",1)[0] if "::" in name else None)
                    if cc == CC_THISCALL:
                        cdt = class_struct(member_cls)
                        if cdt is not None:
                            this_t = PointerDataType(cdt)
                            pis.append(ParameterImpl("this", this_t, prog))
                            this_methods_by_class[member_cls] = this_methods_by_class.get(member_cls,0)+1
                            n_this_applied += 1
                    for (pct, pname) in params:
                        pdt = resolve_type(pct)
                        if pname is None:
                            pname = None
                        pis.append(ParameterImpl(pname, pdt, prog))
                    fn.setCallingConvention(cc)
                    fn.updateFunction(cc, retparam, pis,
                                      FunctionUpdateType.DYNAMIC_STORAGE_FORMAL_PARAMS,
                                      True, US)
                    n_proto += 1; applied_proto = True
                    if len(proto_examples) < 15:
                        proto_examples.append("0x%06x %s [%s]" % (rva, clean_proto(proto), cc))
                except Exception as e:
                    n_proto_fail += 1

        # ---- this-type for member funcs WITHOUT a full tomalla proto ----
        if (not applied_proto) and cls:
            cdt = class_struct(cls)
            if cdt is not None and kind in ("ctor","dtor","method","vfunc"):
                try:
                    this_t = PointerDataType(cdt)
                    # Rebuild formal params under __thiscall: typed `this` first,
                    # then the existing EXPLICIT (non-auto) params preserved.
                    new_params = [ParameterImpl("this", this_t, prog)]
                    for p in fn.getParameters():
                        if p.isAutoParameter():
                            continue  # the old auto-`this` is replaced by ours
                        new_params.append(
                            ParameterImpl(p.getName(), p.getDataType(), prog))
                    retparam = ReturnParameterImpl(fn.getReturnType(), prog)
                    fn.updateFunction(CC_THISCALL, retparam, new_params,
                                      FunctionUpdateType.DYNAMIC_STORAGE_FORMAL_PARAMS,
                                      True, US)
                    this_methods_by_class[cls] = this_methods_by_class.get(cls,0)+1
                    n_this_applied += 1
                except Exception:
                    pass

    R("source stub labels: renamed=%d bogus-overwrote=%d kept=%d ns-set=%d plate-added=%d created-func=%d no-func=%d"
      % (n_renamed, n_bogus, n_kept, n_ns, n_plate, n_created, n_nofunc))
    R("prototypes applied (typed sig+params): %d  (failed: %d)" % (n_proto, n_proto_fail))
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
