# -*- coding: utf-8 -*-
# apply_ghidra_enrichment.py - maximal comprehension enrichment for build/ghidra-named.
#
#   Extends (does NOT clobber) the prior name+plate enrichment with:
#     1. FUNCTION NAMES   - build/gen/symbol_names.csv (src-derived matched
#                           symbols) and config/library_labels.csv
#                           (HIGH/MED/AMBIG). engine_labels.csv is advisory only.
#                           Functions are created when Ghidra has none at the RVA.
#     2. PROTOTYPES + PARAM NAMES - source-derived prototypes are the intended
#                           authority. engine_labels.csv prototypes are kept as
#                           advisory comments and do not update function
#                           signatures.
#     3. STRUCTS (field names @ offsets) - from build/gen/structs.json (clang record
#                           layouts over src/ + structure/), defined in the DTM for
#                           source-derived/manual signatures to reference.
#     4. ENUMS            - from build/gen/enums.json (clang over structure/),
#                           defined in the DTM.
#
#   Reproducible from build/gen/symbol_names.csv + generated JSON +
#   config/library_labels.csv. config/engine_labels.csv is optional advisory
#   extension metadata: it may seed candidate functions/comments when present,
#   but source-derived symbols keep canonical naming/type authority.
#
#   Run as a GhidraScript under PyGhidra (CPython3 + JPype), driven by
#   scripts/gruntz/ghidra/ghidra_metadata_apply.py (which boots PyGhidra, imports/analyzes
#   GRUNTZ.EXE, then runs this + export.py). Invoked via `gruntz init` /
#   `gruntz ghidra-refresh`. The flat-API globals (currentProgram, monitor, ...)
#   are injected by pyghidra.ghidra_script.
#
#   Writes build/ghidra-named/exports/enrichment_apply_report.txt
#@category Gruntz
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
CSV_ENGINE = ROOT + "/config/engine_labels.csv"
# This script is STATELESS - no embedded layouts/enums. All data comes from these
# generated files (+ the config CSVs):
#   build/gen/symbol_names.csv  <- gen_labels.py   (rva -> mangled name, unit)
#   build/gen/structs.json      <- ghidra_metadata_generate.py  (clang record layouts of src/ + structure/)
#   build/gen/enums.json        <- ghidra_metadata_generate.py  (clang over structure/)
CSV_SYMBOL   = ROOT + "/build/gen/symbol_names.csv"
STRUCTS_JSON = ROOT + "/build/gen/structs.json"
ENUMS_JSON   = ROOT + "/build/gen/enums.json"
UNITS_TOML   = ROOT + "/config/units.toml"
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
    s = re.sub(r'\bunsigned\s+char\b', 'uchar', s)
    s = re.sub(r'\bunsigned\s+short\b', 'ushort', s)
    s = re.sub(r'\bunsigned\s+int\b', 'uint', s)
    s = re.sub(r'\bunsigned\s+long\b', 'ulong', s)
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
    if base in CUSTOM_TYPE_NAMES:
        dt = _find_named(base)
    elif leaf in CUSTOM_TYPE_NAMES:
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
    t = re.sub(r'/\*.*?\*/', ' ', ptxt).strip()
    t = re.sub(r'\b(_In_opt_|_In_|_Out_opt_|_Out_|_Inout_)\b', ' ', t).strip()
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

PROTO_RE = re.compile(r'^(?:(.*?)\s+)?([~A-Za-z_][\w:~]*)\s*\((.*)\)\s*$')

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
    pre = (m.group(1) or "").strip()    # return type + maybe CALLBACK/WINAPI
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
# 4b. SOURCE SYMBOL + PROTOTYPE HELPERS
# =====================================================================
ADDR_RE = re.compile(r"@address:\s*(0x[0-9a-fA-F]+)")
SYMBOL_RE = re.compile(r"@symbol:\s*(\S+)")

def parse_units_toml(path):
    """Return unit-name -> source-path from config/units.toml.

    The file shape is intentionally simple and stable; a tiny parser avoids
    requiring tomllib inside the Ghidra Python runtime.
    """
    out = {}
    if not os.path.exists(path):
        return out
    cur_unit = None
    cur_source = None
    try:
        for raw in open(path):
            line = raw.split("#", 1)[0].strip()
            if line == "[[unit]]":
                if cur_unit and cur_source:
                    out[cur_unit] = cur_source
                cur_unit = None
                cur_source = None
                continue
            m = re.match(r'unit\s*=\s*"([^"]+)"', line)
            if m:
                cur_unit = m.group(1)
                continue
            m = re.match(r'source\s*=\s*"([^"]+)"', line)
            if m:
                cur_source = m.group(1)
        if cur_unit and cur_source:
            out[cur_unit] = cur_source
    except Exception:
        return out
    return out

def split_line_comment(line):
    """Strip // comments when they are not part of a string literal."""
    out = ""
    quote = None
    i = 0
    while i < len(line):
        ch = line[i]
        if quote:
            out += ch
            if ch == "\\":
                if i + 1 < len(line):
                    out += line[i + 1]
                    i += 2
                    continue
            elif ch == quote:
                quote = None
            i += 1
            continue
        if ch in ("'", '"'):
            quote = ch
            out += ch
            i += 1
            continue
        if ch == "/" and i + 1 < len(line) and line[i + 1] == "/":
            break
        out += ch
        i += 1
    return out

def _source_header_after(lines, start_idx):
    """Find the next function definition header after an @address block.

    If another @address appears before code, this annotation describes a symbol
    without a source body (for example a scalar deleting destructor thunk), so
    no source prototype is returned.
    """
    buf = []
    parens = 0
    saw_paren = False
    for i in range(start_idx, len(lines)):
        raw = lines[i]
        if ADDR_RE.search(raw):
            return None
        stripped = raw.strip()
        if not buf and (not stripped or stripped.startswith("//") or stripped.startswith("#")):
            continue
        if not buf and stripped.startswith(("template", "extern \"C\"")):
            continue
        line = split_line_comment(raw)
        if not buf and not line.strip():
            continue
        for ch in line:
            if ch == "(":
                parens += 1
                saw_paren = True
            elif ch == ")" and parens > 0:
                parens -= 1
        cut = None
        for mark in ("{", ";"):
            pos = line.find(mark)
            if pos != -1 and parens == 0:
                cut = pos if cut is None else min(cut, pos)
        buf.append(line if cut is None else line[:cut])
        if cut is not None:
            break
    if not saw_paren or not buf:
        return None
    header = " ".join(x.strip() for x in buf)
    header = re.sub(r"/\*.*?\*/", " ", header)
    header = re.sub(r"\s+", " ", header).strip()
    idx = header.find(") :")
    if idx != -1:
        header = header[:idx + 1]
    return header if header else None

def source_prototypes_by_rva(unit_to_source):
    """Return rva -> source definition prototype text for annotated src rows."""
    out = {}
    seen_paths = set()
    for _unit, rel in unit_to_source.items():
        if not rel.startswith("src/"):
            continue
        path = os.path.join(ROOT, rel)
        if path in seen_paths or not os.path.exists(path):
            continue
        seen_paths.add(path)
        try:
            lines = open(path).read().splitlines()
        except Exception:
            continue
        i = 0
        while i < len(lines):
            ma = ADDR_RE.search(lines[i])
            if not ma:
                i += 1
                continue
            rva = int(ma.group(1), 16)
            block_has_symbol_override = bool(SYMBOL_RE.search(lines[i]))
            j = i + 1
            while j < len(lines) and lines[j].lstrip().startswith("//"):
                if SYMBOL_RE.search(lines[j]):
                    block_has_symbol_override = True
                j += 1
            # Explicit @symbol rows often bind compiler-generated symbols with no
            # source definition. Avoid stealing the next annotated function body.
            if not block_has_symbol_override:
                hdr = _source_header_after(lines, j)
                if hdr:
                    out[rva] = hdr
            i = j
    return out

def parse_msvc_function_symbol(sym):
    """Small MSVC decorated-name parser for source function authority.

    It intentionally extracts only what task 2 needs: scoped class name and
    whether the symbol is an instance member. Full argument types come from the
    source prototype where available.
    """
    if not sym or not sym.startswith("?"):
        return None
    end = sym.find("@@")
    if end == -1:
        return None
    method = None
    scopes = []
    if sym.startswith("??"):
        if len(sym) >= 4 and sym[2] == "_":
            method = sym[:4]
            rest = sym[4:end]
        else:
            method = sym[:3]
            rest = sym[3:end]
        scopes = [p for p in rest.split("@") if p]
    else:
        rest = sym[1:end]
        parts = [p for p in rest.split("@") if p]
        if not parts:
            return None
        method = parts[0]
        scopes = parts[1:]
    cls = "::".join(reversed(scopes)) if scopes else None
    decor = sym[end + 2:]
    first = decor[0] if decor else ""
    is_static = bool(cls and first == "S")
    is_instance = bool(cls and first in ("A", "E", "I", "M", "Q", "U"))
    return {
        "method": method,
        "class": cls,
        "is_member": bool(cls),
        "is_static": is_static,
        "is_instance": is_instance,
    }

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

def append_plate_once(addr, marker, text):
    """Append a plate comment when this exact advisory/source marker is absent."""
    old = listing.getComment(PLATE, addr) or ""
    if marker in old:
        return False
    listing.setComment(addr, PLATE, (old + "\n" if old else "") + text)
    return True

# leaf-name -> struct DataType for this-type application
def class_struct(cls):
    if not cls:
        return None
    candidates = [cls, cls.split("::")[-1]]
    for name in candidates:
        if name in CUSTOM_TYPE_NAMES:    # only classes with a generated struct
            dt = dtm.getDataType(GRUNTZ_CAT, name)
            if dt is not None:
                return dt
    return None

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

    # ---- (C) load CSVs ----
    eng_rows = load_csv_rows(CSV_ENGINE) if os.path.exists(CSV_ENGINE) else []  # rva,name,class,prototype,kind,source,confidence
    # filter to data rows (7 cols, header skipped by content)
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
    syms = []           # functions (kind=func / legacy): (rva, mangled name, unit)
    src_func_rvas = set()
    data_syms = []      # global data (kind=data): mangled name, Ghidra demangles for display
    for r in sym_rows:
        if len(r) < 2: continue
        if r[0] == "rva": continue
        try: rva = int(r[0], 16)
        except Exception: continue
        kind = r[4] if len(r) > 4 else "func"
        if kind == "data":
            data_syms.append((rva, r[1]))
        else:
            unit = r[2] if len(r) > 2 else ""
            syms.append((rva, r[1], unit))
            src_func_rvas.add(rva)

    fid_rows = load_csv_rows(CSV_FID) if os.path.exists(CSV_FID) else []  # rva,name,lib,confidence,source
    fids = []
    for r in fid_rows:
        if len(r) < 4: continue
        if r[0] == "rva": continue
        try: rva = int(r[0], 16)
        except Exception: continue
        fids.append((rva, r[1], r[2], r[3]))

    # ---- (D) FID/library + source-owned symbol names ----
    source_func_rvas = src_func_rvas
    source_data_rvas = set(rva for (rva, _name) in data_syms)

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

    n_sym_named = 0; n_sym_nofunc = 0; n_sym_kept = 0
    for (rva, name, _unit) in syms:
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
        else:
            n_sym_kept += 1
    R("source function symbols applied: renamed/reconciled=%d kept=%d no-func=%d total=%d" %
      (n_sym_named, n_sym_kept, n_sym_nofunc, len(syms)))

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
    R("source global data symbols applied: labeled=%d total=%d" %
      (n_data_named, len(data_syms)))

    # ---- (E) source-owned signatures from src/ + compiled symbols ----
    #
    # `symbol_names.csv` is generated from `src/` @address annotations and the
    # compiled/base .obj symbol table, so it is the authority for which target
    # functions are source-owned. The source definition gives return/argument
    # types and names; the MSVC decorated symbol decides whether a scoped symbol
    # is an instance member that must receive a typed `this`.
    unit_to_source = parse_units_toml(UNITS_TOML)
    src_proto_by_rva = source_prototypes_by_rva(unit_to_source)

    n_src_sig_applied = 0; n_src_sig_fail = 0; n_src_sig_nofunc = 0
    n_src_this_applied = 0; n_src_this_only = 0; n_src_this_missing_struct = 0
    n_src_no_proto = 0; n_src_proto_parse_fail = 0; n_src_non_src_unit = 0
    n_src_arg_params = 0
    src_sig_examples = []
    src_this_by_class = {}

    for (rva, name, unit) in syms:
        relsrc = unit_to_source.get(unit, "")
        if not relsrc.startswith("src/"):
            n_src_non_src_unit += 1
            continue
        addr = toaddr(rva)
        fn = ensure_function(addr)
        if fn is None:
            n_src_sig_nofunc += 1
            continue
        sym_info = parse_msvc_function_symbol(name)
        instance_cls = sym_info.get("class") if sym_info and sym_info.get("is_instance") else None
        proto = src_proto_by_rva.get(rva)
        parsed = parse_prototype(proto, "method" if instance_cls else "") if proto else None

        if proto and parsed is None:
            n_src_proto_parse_fail += 1

        if parsed is not None:
            ret_ct, cc, params, _is_member = parsed
            if sym_info is not None:
                if sym_info.get("is_instance"):
                    cc = CC_THISCALL
                elif sym_info.get("is_static") and cc == CC_THISCALL:
                    cc = CC_CDECL
            pis = []
            try:
                arg_count = 0
                if cc == CC_THISCALL:
                    cdt = class_struct(instance_cls)
                    if cdt is None:
                        n_src_this_missing_struct += 1
                        continue
                    pis.append(ParameterImpl("this", PointerDataType(cdt), prog))
                    src_this_by_class[instance_cls] = src_this_by_class.get(instance_cls, 0) + 1
                    n_src_this_applied += 1
                for (pct, pname) in params:
                    pis.append(ParameterImpl(pname, resolve_type(pct), prog))
                    arg_count += 1
                retparam = ReturnParameterImpl(resolve_type(ret_ct), prog)
                fn.updateFunction(cc, retparam, pis,
                                  FunctionUpdateType.DYNAMIC_STORAGE_FORMAL_PARAMS,
                                  True, US)
                n_src_sig_applied += 1
                n_src_arg_params += arg_count
                if len(src_sig_examples) < 15:
                    src_sig_examples.append("0x%06x %s [%s]" % (rva, proto, cc))
            except Exception:
                n_src_sig_fail += 1
            continue

        if instance_cls:
            # Source-owned compiler-generated members may have a decorated symbol
            # but no source definition. Apply the high-confidence `this` type
            # while preserving any explicit parameters Ghidra already has.
            cdt = class_struct(instance_cls)
            if cdt is None:
                n_src_this_missing_struct += 1
                continue
            try:
                new_params = [ParameterImpl("this", PointerDataType(cdt), prog)]
                for p in fn.getParameters():
                    if p.isAutoParameter():
                        continue
                    new_params.append(ParameterImpl(p.getName(), p.getDataType(), prog))
                retparam = ReturnParameterImpl(fn.getReturnType(), prog)
                fn.updateFunction(CC_THISCALL, retparam, new_params,
                                  FunctionUpdateType.DYNAMIC_STORAGE_FORMAL_PARAMS,
                                  True, US)
                src_this_by_class[instance_cls] = src_this_by_class.get(instance_cls, 0) + 1
                n_src_this_applied += 1
                n_src_this_only += 1
            except Exception:
                n_src_sig_fail += 1
        else:
            n_src_no_proto += 1

    R("source signatures applied: full=%d this-only=%d arg-params=%d failed=%d no-func=%d" %
      (n_src_sig_applied, n_src_this_only, n_src_arg_params,
       n_src_sig_fail, n_src_sig_nofunc))
    R("source signature misses: non-src-units=%d no-proto=%d proto-parse-fail=%d missing-this-struct=%d" %
      (n_src_non_src_unit, n_src_no_proto, n_src_proto_parse_fail,
       n_src_this_missing_struct))
    R("source this-type applications: %d across %d classes" %
      (n_src_this_applied, len(src_this_by_class)))
    for c in sorted(src_this_by_class.keys()):
        R("    source this->%s : %d methods" % (c, src_this_by_class[c]))
    for e in src_sig_examples:
        R("    source sig: " + e)

    # ---- (F) engine_labels advisory comments only ----
    #
    # engine_labels.csv is useful backlog/extension metadata, but it is optional
    # and is not an authority for canonical names, namespaces, prototypes, or
    # this-types. Source-owned RVAs from symbol_names.csv keep precedence.
    # Engine rows may still seed a candidate Function object and advisory plate
    # comment; nothing should depend on this file existing.
    n_engine_adv_plate = 0; n_engine_adv_existing = 0
    n_engine_adv_source_overlap = 0; n_engine_adv_non_source = 0
    n_engine_adv_nofunc = 0; n_engine_adv_created = 0; n_engine_adv_import = 0
    engine_examples = []

    for (rva, name, cls, proto, kind, source, conf) in eng:
        addr = toaddr(rva)
        fn = fm.getFunctionAt(addr)
        if fn is None:
            fn = fm.getFunctionContaining(addr)
        if fn is None:
            fn = ensure_function(addr)
            if fn is not None:
                n_engine_adv_created += 1
            else:
                n_engine_adv_nofunc += 1

        is_source_owned = rva in source_func_rvas or rva in source_data_rvas
        if is_source_owned:
            n_engine_adv_source_overlap += 1
        else:
            n_engine_adv_non_source += 1
        if kind == "import-caller":
            n_engine_adv_import += 1

        # Advisory plate comment (provenance) - keep prior, append if new. These
        # intentionally do not rename the function, move it into a namespace, or
        # update its prototype/this-type.
        if kind == "import-caller":
            detail = "import-caller; class=%s; %s" % ((cls or "?"), name or "")
        else:
            detail = proto if proto else name
        marker = "[ENGINE-LABEL %06x]" % rva
        scope = "source-overlap" if is_source_owned else "advisory"
        plate = "%s %s/%s/%s %s" % (marker, source, conf, scope, detail)
        try:
            if append_plate_once(addr, marker, plate):
                n_engine_adv_plate += 1
                if len(engine_examples) < 10:
                    engine_examples.append("0x%06x %s" % (rva, detail))
            else:
                n_engine_adv_existing += 1
        except Exception:
            pass

    R("engine_labels advisory rows: total=%d non-source=%d source-overlap=%d import-callers=%d created-func=%d no-func=%d" %
      (len(eng), n_engine_adv_non_source, n_engine_adv_source_overlap,
       n_engine_adv_import, n_engine_adv_created, n_engine_adv_nofunc))
    R("engine_labels advisory plates: added=%d already-present=%d canonical-updates=0" %
      (n_engine_adv_plate, n_engine_adv_existing))
    for e in engine_examples:
        R("    advisory: " + e)

    # struct availability tally
    n_structs_applied = sum(1 for (nm,(dt,ap)) in struct_dt_by_name.items() if ap)
    R("structs available for source/manual signatures: %d / %d defined" %
      (n_structs_applied, len(struct_dt_by_name)))

    # ---- (G) backfill the leading incremental-linker JMP-thunk table ----
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
