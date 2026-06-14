# -*- coding: utf-8 -*-
# apply_ghidra_enrichment.py - maximal comprehension enrichment for build/ghidra-named.
#
#   Extends (does NOT clobber) the prior name+plate enrichment with:
#     1. FUNCTION NAMES   - engine_labels.csv (incl. +37 import-caller rows),
#                           config/library_labels.csv (HIGH/MED/AMBIG),
#                           config/symbol_names.csv (zlib + ctors).
#                           Functions are created when Ghidra has none at the RVA.
#     2. PROTOTYPES + PARAM NAMES - parsed from engine_labels' `prototype` field
#                           (tomalla rows). Sets return type, params (name+type),
#                           and calling convention (__thiscall members / __cdecl
#                           free funcs / __stdcall callbacks). Win32 types resolve
#                           against the windows_vs12_32 archive already in the DTM;
#                           custom struct types resolve against the structs defined
#                           below; anything unresolved falls back to void*/int.
#     3. STRUCTS (field names @ offsets) - from build/gen/structs.json (clang record
#                           layouts over src/ + structure/), defined in the DTM and
#                           APPLIED as the `this` (param-0) type on every method of
#                           that class (so member access decodes as this->m_health).
#     4. ENUMS            - from build/gen/enums.json (clang over structure/),
#                           defined in the DTM.
#
#   Reproducible from config/engine_labels.csv + config/symbol_names.csv +
#   config/library_labels.csv + structure/. Idempotent: re-runnable, never
#   downgrades a better existing name, keeps prior [LABEL] plate comments.
#
#   Run (Ghidra 11.4.2 headless Jython):
#     analyzeHeadless build/ghidra-named gruntz -process GRUNTZ.EXE -noanalysis \
#         -scriptPath build/ghidra-named/scripts -postScript apply_ghidra_enrichment.py
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
#   build/gen/structs.json      <- gen_structs.py  (clang record layouts of src/ + structure/)
#   build/gen/enums.json        <- gen_structs.py  (clang over structure/)
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

# =====================================================================
# 1. TYPE RESOLUTION
# =====================================================================
GRUNTZ_CAT = CategoryPath("/Gruntz")

# canonical builtin lookups, cached
_type_cache = {}

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
    global PTR_FALLBACK, INT_FALLBACK
    voidt = _find_named("void")
    PTR_FALLBACK = PointerDataType(voidt)
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
def is_default(nm): return nm.startswith("FUN_") or nm.startswith("thunk_FUN_")
def is_bogus(nm):   return any(p.search(nm) for p in BOGUS)

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
    # gen_structs.py JSON: {name,size,fields:[{offset,type,name}],source} ->
    # the (name,size,apply_this,desc,fields) shape this script already consumes.
    gen_structs_list = _load_gen(STRUCTS_JSON, lambda o: (
        o["name"], o["size"], True,
        "generated by gen_structs.py from %s" % o.get("source", "src"),
        [(f["offset"], f["type"], f["name"]) for f in o["fields"]]))
    gen_names = set(s[0] for s in gen_structs_list)
    CUSTOM_TYPE_NAMES.update(gen_names)   # stateless: custom types = the generated structs
    effective_structs = gen_structs_list
    R("structs: %d generated (from src/ + structure/ via clang)" % len(gen_structs_list))
    n_structs = 0
    struct_dt_by_name = {}
    for (name, size, apply_this, desc, fields) in effective_structs:
        dt = get_or_create_struct(name, size, desc, fields)
        struct_dt_by_name[name] = (dt, apply_this)
        n_structs += 1
    # WwdObject + RezDirEntry now come from structure/ headers via gen_structs (clang).

    R("structs defined: %d" % n_structs)

    # ---- (B) define enums (generated JSON preferred; hardcoded = fallback) ----
    gen_enums_list = _load_gen(ENUMS_JSON, lambda o: (
        o["name"], 4, "generated by gen_structs.py from %s" % o.get("source", "src"),
        [(m["name"], m["value"]) for m in o["members"]]))
    effective_enums = gen_enums_list
    n_enums = 0
    for (name, size, desc, members) in effective_enums:
        define_enum(name, size, desc, members)
        n_enums += 1
    R("enums: %d generated (from structure/ via clang)" % len(gen_enums_list))

    # ---- (C) load CSVs ----
    eng_rows = load_csv_rows(CSV_ENGINE)   # rva,name,class,prototype,kind,source,confidence
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

    sym_rows = load_csv_rows(CSV_SYMBOL)    # rva,name,unit
    syms = []
    for r in sym_rows:
        if len(r) < 2: continue
        if r[0] == "rva": continue
        try: rva = int(r[0], 16)
        except Exception: continue
        syms.append((rva, r[1]))

    fid_rows = load_csv_rows(CSV_FID) if os.path.exists(CSV_FID) else []  # rva,name,lib,confidence,source
    fids = []
    for r in fid_rows:
        if len(r) < 4: continue
        if r[0] == "rva": continue
        try: rva = int(r[0], 16)
        except Exception: continue
        fids.append((rva, r[1], r[2], r[3]))

    # ---- (D) FID + zlib names (apply first; engine_labels overrides where overlapping) ----
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
        cur = fn.getName()
        if is_default(cur) or is_bogus(cur) or cur != name:
            try:
                fn.setName(name, US); n_sym_named += 1
            except Exception:
                pass
    R("symbol_names (zlib+ctors) reconciled: %d  (no-func: %d)" % (n_sym_named, n_sym_nofunc))

    # ---- (E) engine_labels: names + namespace + plate + PROTOTYPES + this-type ----
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
                if pn is None or pn.getName() != ns.getName():
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

    R("engine_labels: renamed=%d bogus-overwrote=%d kept=%d ns-set=%d plate-added=%d created-func=%d no-func=%d"
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

    # ---- coverage ----
    from ghidra.program.model.data import Pointer as _Ptr
    named = 0; funn = 0; thiscall = 0; typed_this_now = 0; named_param = 0
    bare_regions = {}
    for fn in fm.getFunctions(True):
        nm = fn.getName()
        frva = fn.getEntryPoint().getOffset() - IMAGE_BASE
        if nm.startswith("FUN_"):
            funn += 1
            b = frva & ~0xFFFF
            bare_regions[b] = bare_regions.get(b, 0) + 1
        else:
            named += 1
        if fn.getCallingConventionName() == "__thiscall":
            thiscall += 1
        ps = fn.getParameters()
        sawnamed = False
        for i, p in enumerate(ps):
            pn = p.getName()
            if pn and not pn.startswith("param_") and not pn.startswith("in_") and pn != "this":
                sawnamed = True
            if i == 0:
                d = p.getDataType()
                base = d.getDataType() if isinstance(d, _Ptr) else d
                if base is not None:
                    cp = base.getCategoryPath()
                    if cp is not None and cp.getPath().startswith("/Gruntz"):
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
fh = open(REPORT, "w")
try:
    for ln in report: fh.write(ln + "\n")
finally:
    fh.close()
print("[apply_ghidra_enrichment] report -> %s" % REPORT)
