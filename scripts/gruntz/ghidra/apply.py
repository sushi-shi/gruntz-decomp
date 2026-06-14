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
#     3. STRUCTS (field names @ offsets) - the tomalla/RTTI-ported layouts that have
#                           CONFIRMED offsets, defined in the DTM and then APPLIED as
#                           the `this` (param-0) type on every method of that class
#                           (so member access decodes as this->m_health, not ecx+4).
#                           Layouts with @todo offsets (WwdObject, RezDirEntry) are
#                           DEFINED for reference but NOT applied as this-types.
#     4. ENUMS            - the taxonomy + real-value enums from structure/enums.h,
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
# Function names + struct/enum layouts now come GENERATED from src/ (+ converted
# structure/) rather than the hand CSV / the hardcoded STRUCTS|ENUMS below:
#   build/gen/symbol_names.csv  <- gen_labels.py   (rva -> mangled name, unit)
#   build/gen/structs.json      <- gen_structs.py  (clang record layouts)
#   build/gen/enums.json        <- gen_structs.py
# The hardcoded STRUCTS/ENUMS remain only as a FALLBACK for comprehension classes
# whose structure/ header is not yet compilable (graduate-on-match; shrinks to 0).
CSV_SYMBOL   = ROOT + "/build/gen/symbol_names.csv"
STRUCTS_JSON = ROOT + "/build/gen/structs.json"
ENUMS_JSON   = ROOT + "/build/gen/enums.json"
CSV_FID    = ROOT + "/config/library_labels.csv"   # tracked FID output (survives `git clean`)
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
    "GameInfo",  # GameInfo is defined as struct below; pointer handled via custom
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
# names of structs/classes we DEFINE (so resolve_type can find them)
CUSTOM_TYPE_NAMES = set([
    "RegistryHelper", "Font", "Pair", "MemoryPool_Pair",
    "CGameMgr", "CGameWnd", "CGameApp", "GameInfo",
    "CGruntzMgr", "CGruntzApp", "CGruntzWnd",
    "UnknownClassArrays", "UnknownClassInCGruntzMgr",
    "UnknownClassCGruntzMgrHarryPotter", "UnknownCGruntzMgrHogwarts",
    "UnknownCGruntzMgrLucius", "UnknownDraco", "UnknownHermiona",
    "UnknownHagrid", "UnknownSeverus", "UnknownSirius", "UnknownAlbus",
    "UnknownRemus", "UnknownMinerva", "UnknownPettigrew", "UnknownFilch",
    "UnknownSalazar", "UnknownVoldemort", "UnknownDirectDrawStructure",
    "WwdRect", "WwdObject", "RezDirEntry",
    "CNetMgr",
])

# (offset, c-type, name) tuples. Offsets in decimal. Gaps -> undefined.
# size in bytes; apply_as_this bool; comment carried as struct description.
STRUCTS = [
  ("RegistryHelper", 0x21C, True,
   "Utils::RegistryHelper - HKLM\\Software\\Monolith Productions\\Gruntz\\1.0 wrapper. @approx tomalla 1.0.1.77 (offsets version-independent)",
   [(0x0,"bool","m_isInitialized"),(0x4,"HKEY","m_hKey"),(0x8,"HKEY","m_hKey2"),
    (0xc,"HKEY","m_hKey3"),(0x10,"HKEY","m_hKey4"),(0x14,"HKEY","m_hKey5"),
    (0x18,"HKEY","m_hKey6"),(0x1c,"char[256]","m_szParam2"),(0x11c,"char[256]","m_szParam4")]),

  ("Font", 0x14, True,
   "Font - bitmap-font loader (large/medium/small/tiny). @approx tomalla 1.0.1.77",
   [(0x0,"bool","m_isMemoryAllocated"),(0x4,"int","m_lettersCount"),
    (0x8,"char**","m_pPixelData"),(0xc,"void*","m_pLettersSize"),
    (0x10,"int","m_fontHeight")]),

  ("Pair", 0x8, False,
   "Pair {int a; int b} - element type of CGruntzMgr::memory_pool. @approx tomalla 1.0.1.77",
   [(0x0,"int","a"),(0x4,"int","b")]),

  ("MemoryPool_Pair", 0x10, True,
   "Utils::MemoryPool<Pair> - fixed-block free-list allocator. @approx tomalla 1.0.1.77",
   [(0x0,"char*","m_pBlock"),(0x4,"void*","m_pNextFreeNode"),
    (0x8,"uint","m_nodesCount"),(0xc,"uint","m_dataOffset")]),

  # ---- WAP32 engine bases (tomalla layouts) ----
  ("CGameMgr", 0x2c, True,
   "WAP32::CGameMgr - engine game-manager base. @approx tomalla 1.0.1.77",
   [(0x0,"void*","__vftable"),(0x4,"CGameWnd*","m_pGameWnd"),(0x8,"CGameApp*","m_pGameApp"),
    (0xc,"int","fieldUnknown00C"),(0x10,"int","m_isSoundEnabled"),(0x14,"int","m_isMusicEnabled"),
    (0x18,"int","fieldUnknown018"),(0x1c,"int","fieldUnknown01C"),(0x20,"int","fieldUnknown020"),
    (0x24,"int","fieldUnknown024")]),

  ("CGameWnd", 0x10, True,
   "WAP32::CGameWnd - engine main-window base. @approx tomalla 1.0.1.77",
   [(0x0,"void*","__vftable"),(0x4,"HWND","m_hWnd"),(0x8,"CGameApp*","m_pGameApp"),
    (0xc,"int","fieldUnknown00C")]),

  ("GameInfo", 0x1d4, False,
   "WAP32::GameInfo - window/launch descriptor. @approx tomalla 1.0.1.77",
   [(0x0,"int","size"),(0x4,"char","windowClassFlags"),(0x8,"HINSTANCE","hInstance"),
    (0xc,"char[128]","szCmdLine"),(0x8c,"char[64]","szGameIdentifier"),
    (0xcc,"char[64]","szWindowName"),(0x14c,"char[128]","szWindowClassName"),
    (0x1cc,"int","windowWidth"),(0x1d0,"int","windowHeight")]),

  ("CGameApp", 0x254, True,
   "WAP32::CGameApp - engine application base. @approx tomalla 1.0.1.77",
   [(0x0,"void*","__vftable"),(0x4,"CGameWnd*","m_pGameWnd"),(0x8,"CGameMgr*","m_pGameMgr"),
    (0xc,"HINSTANCE","m_hInstance"),(0x10,"HACCEL","m_hAccelerators"),
    (0x14,"GameInfo","m_gameInfo"),
    (0x240,"uint","fieldUnknown240"),(0x244,"bool","fieldUnknown244_errorRelatedFlag"),
    (0x248,"bool","m_isError"),(0x24c,"uint","m_errorMessageId"),(0x250,"uint","m_errorCode")]),

  # CGruntzMgr full layout (derives from CGameMgr; base flattened at offset 0).
  ("CGruntzMgr", 0xa30, True,
   "CGruntzMgr - central Gruntz game manager (CGameMgr subclass). @approx tomalla 1.0.1.77; field NAMES past config block are placeholders",
   [(0x0,"void*","__vftable"),(0x4,"CGameWnd*","m_pGameWnd"),(0x8,"CGameApp*","m_pGameApp"),
    (0x10,"int","m_isSoundEnabled"),(0x14,"int","m_isMusicEnabled"),
    (0x2c,"int","fieldUnknown02C_pCurrentState"),
    (0x30,"UnknownClassCGruntzMgrHarryPotter*","fieldUnknown030_maybeSurfaceRestoreHandler"),
    (0x34,"int","fieldUnknown034"),(0x38,"RegistryHelper*","m_pRegistryHelper"),
    (0x80,"int","m_numRuns"),(0x84,"int","m_numMovies"),
    (0x94,"int","m_resolutionWidth"),(0x98,"int","m_resolutionHeight"),
    (0x9c,"bool","m_unknownIsLobbyConnectionSettingsInitialized"),
    (0xa0,"bool","m_unknownIsLobbyConnectionSettingsAttempted"),
    (0xb8,"int","m_isCheckpointPrompts"),
    (0xc0,"void*","m_pDirectPlayLobby"),(0xc4,"void*","m_pDirectPlayConnection"),
    (0xd0,"char","m_driveLetter"),(0xd4,"bool","m_isDriveLetterLoaded"),
    (0x100,"int","m_isVoiceEnabled"),(0x104,"int","m_isAmbientEnabled"),
    (0x108,"int","m_isInterlaced"),(0x10c,"int","m_isHighDetail"),
    (0x110,"int","m_unknownSecondIsHighDetail"),(0x118,"int","m_isEasyMode"),
    (0x11c,"int","m_soundVolume"),(0x120,"int","m_voiceVolume"),(0x124,"int","m_scrollSpeed")]),

  ("CGruntzApp", 0x254, True,
   "CGruntzApp - the app object (CGameApp subclass). @approx tomalla 1.0.1.77 (base layout)",
   [(0x0,"void*","__vftable"),(0x4,"CGameWnd*","m_pGameWnd"),(0x8,"CGameMgr*","m_pGameMgr"),
    (0xc,"HINSTANCE","m_hInstance"),(0x10,"HACCEL","m_hAccelerators")]),

  ("CGruntzWnd", 0x10, True,
   "CGruntzWnd - the main window (CGameWnd subclass). @approx tomalla 1.0.1.77 (base layout)",
   [(0x0,"void*","__vftable"),(0x4,"HWND","m_hWnd"),(0x8,"CGameApp*","m_pGameApp")]),

  # ---- CGruntzMgr nested helper objects (tomalla) ----
  ("UnknownClassArrays", 0x144, True,
   "UnknownClassArrays - nested array bundle in UnknownClassInCGruntzMgr. @approx tomalla 1.0.1.77 (mostly @todo)",
   [(0x18,"int","fieldUnknown018"),(0xdc,"int[5]","unknownMemoryPoolPointers1"),
    (0xf0,"int[5]","unknownMemoryPoolPointers2"),(0x104,"int[5]","unknownDwordArray1"),
    (0x118,"int[5]","unknownDwordArray2"),(0x13c,"int","fieldUnknown13C"),(0x140,"int","fieldUnknown140")]),

  ("UnknownClassInCGruntzMgr", 0x238, True,
   "UnknownClassInCGruntzMgr - 0x238 sub-object, CGruntzMgr holds an array of 4. @approx tomalla 1.0.1.77",
   [(0x0,"int","fieldUnknown000"),(0x4,"int","strUnknownString004"),(0x8,"int","fieldUnknown008"),
    (0x20,"int","fieldUnknown020"),(0x38,"UnknownClassArrays","unknownObjectArrays"),
    (0x220,"int","fieldUnknown220")]),

  # ---- DDrawMgr "harry_potter" family (HYPOTHESIS: identity=CDirectDrawMgr) ----
  ("UnknownCGruntzMgrHogwarts", 0x8, False,
   "DDrawMgr-family common base (CObject). @approx tomalla 1.0.1.77. HYPOTHESIS: CDirectDrawMgr family; names = tomalla placeholders",
   [(0x0,"void*","__vftable"),(0x4,"int","m_fieldBaseUnknown")]),

  ("UnknownCGruntzMgrLucius", 0x10, True,
   "DDrawMgr-family shared polymorphic sub-manager base. @approx tomalla 1.0.1.77. HYPOTHESIS: CDirectDrawMgr family",
   [(0x0,"void*","__vftable"),(0x4,"int","m_fieldBaseUnknown"),(0x8,"int","fieldUnknown8"),
    (0xc,"UnknownClassCGruntzMgrHarryPotter*","m_pUnknownHarryPotter")]),

  ("UnknownDraco", 0x1c, True,
   "DDrawMgr-family sub-manager. @approx tomalla 1.0.1.77. HYPOTHESIS",
   [(0x0,"void*","__vftable"),(0x8,"int","fieldUnknown8"),(0xc,"UnknownClassCGruntzMgrHarryPotter*","m_pUnknownHarryPotter"),
    (0x10,"int","fieldUnknown10"),(0x14,"int","fieldUnknown14"),(0x18,"int","fieldUnknown18")]),

  ("UnknownHermiona", 0x6c, True,
   "DDrawMgr-family sub-manager (CObList + 2x CMapPtrToPtr). @approx tomalla 1.0.1.77. HYPOTHESIS",
   [(0x0,"void*","__vftable"),(0x8,"int","fieldUnknown8"),(0xc,"UnknownClassCGruntzMgrHarryPotter*","m_pUnknownHarryPotter"),
    (0x64,"int","fieldUnknown64"),(0x68,"int","fieldUnknown68")]),

  ("UnknownHagrid", 0x2c, True,
   "DDrawMgr-family sub-manager (CObList). @approx tomalla 1.0.1.77. HYPOTHESIS",
   [(0x0,"void*","__vftable"),(0x8,"int","fieldUnknown8"),(0xc,"UnknownClassCGruntzMgrHarryPotter*","m_pUnknownHarryPotter")]),

  ("UnknownDirectDrawStructure", 0x64, False,
   "DDSURFACEDESC-shaped static struct held by UnknownSeverus (strongest DDraw evidence). @approx tomalla 1.0.1.77",
   [(0x0,"DWORD","dwSize")]),

  ("UnknownSeverus", 0x2c, True,
   "DDrawMgr-family sub-manager - holds static DDSURFACEDESC struct (widest vtable, 18 slots). @approx tomalla 1.0.1.77. HYPOTHESIS",
   [(0x0,"void*","__vftable"),(0x8,"int","fieldUnknown8"),(0xc,"UnknownClassCGruntzMgrHarryPotter*","m_pUnknownHarryPotter")]),

  ("UnknownSirius", 0x2c, True,
   "DDrawMgr-family sub-manager (CMapStringToOb). @approx tomalla 1.0.1.77. HYPOTHESIS",
   [(0x0,"void*","__vftable"),(0x8,"int","fieldUnknown8"),(0xc,"UnknownClassCGruntzMgrHarryPotter*","m_pUnknownHarryPotter")]),

  ("UnknownAlbus", 0x68, True,
   "DDrawMgr-family sub-manager (3x CMapStringToOb). @approx tomalla 1.0.1.77. HYPOTHESIS",
   [(0x0,"void*","__vftable"),(0x8,"int","fieldUnknown8"),(0xc,"UnknownClassCGruntzMgrHarryPotter*","m_pUnknownHarryPotter"),
    (0x64,"int","fieldUnknown64")]),

  ("UnknownRemus", 0x6d4, True,
   "DDrawMgr-family sub-manager - seeds resolution/scaling ladder; 3x CObArray + ~1.5KB buffer. @approx tomalla 1.0.1.77. HYPOTHESIS",
   [(0x0,"void*","__vftable"),(0x8,"int","fieldUnknown8"),(0xc,"UnknownClassCGruntzMgrHarryPotter*","m_pUnknownHarryPotter"),
    (0x10,"int","fieldUnknown10"),(0x5c,"int","fieldUnknown5C"),(0x60,"int","fieldUnknown60"),
    (0x64,"int","fieldUnknown64"),(0x68,"int","fieldUnknown68")]),

  ("UnknownMinerva", 0x38, True,
   "DDrawMgr-family sub-manager (CMapStringToPtr). @approx tomalla 1.0.1.77. HYPOTHESIS",
   [(0x0,"void*","__vftable"),(0x8,"int","fieldUnknown8"),(0xc,"UnknownClassCGruntzMgrHarryPotter*","m_pUnknownHarryPotter"),
    (0x2c,"int","fieldUnknown2C"),(0x34,"int","fieldUnknown34")]),

  ("UnknownPettigrew", 0x2c, True,
   "DDrawMgr-family sub-manager (CMapStringToPtr). @approx tomalla 1.0.1.77. HYPOTHESIS",
   [(0x0,"void*","__vftable"),(0x8,"int","fieldUnknown8"),(0xc,"UnknownClassCGruntzMgrHarryPotter*","m_pUnknownHarryPotter")]),

  ("UnknownFilch", 0x948, True,
   "DDrawMgr-family standalone (2x CPtrList + CPtrArray). @approx tomalla 1.0.1.77. HYPOTHESIS",
   [(0x0,"int","fieldUnknown000"),(0x4,"int","fieldUnknown004"),
    (0x534,"int","fieldUnknown534"),(0x538,"int","fieldUnknown538"),
    (0x93c,"int","fieldUnknown93C"),(0x940,"int","fieldUnknown940"),(0x944,"int","fieldUnknown944")]),

  ("UnknownSalazar", 0x94, True,
   "DDrawMgr-family - holds 101-entry volume->attenuation lookup table. @approx tomalla 1.0.1.77. HYPOTHESIS",
   [(0x0,"void*","__vftable"),(0x4,"int","fieldUnknown04"),(0x8,"int","fieldUnknown08"),
    (0xc,"int","fieldUnknown0C"),(0x10,"int","fieldUnknown10"),(0x78,"int","fieldUnknown78"),
    (0x80,"int","fieldUnknown80"),(0x84,"int","fieldUnknown84"),(0x88,"int","fieldUnknown88"),
    (0x8c,"int","fieldUnknown8C"),(0x90,"int","fieldUnknown90")]),

  ("UnknownVoldemort", 0x9c, True,
   "DDrawMgr-family (UnknownSalazar subclass). @approx tomalla 1.0.1.77. HYPOTHESIS",
   [(0x0,"void*","__vftable"),(0x4,"int","fieldUnknown04"),(0x94,"int","fieldUnknown94"),(0x98,"int","fieldUnknown98")]),

  ("UnknownClassCGruntzMgrHarryPotter", 0x40, True,
   "DDrawMgr-family MANAGER (HYPOTHESIZED CDirectDrawMgr); stored in CGruntzMgr@0x30. Owns 11 sub-managers. @approx tomalla 1.0.1.77",
   [(0x0,"void*","__vftable"),
    (0x4,"UnknownCGruntzMgrLucius*","fieldUnknownDraco"),(0x8,"UnknownCGruntzMgrLucius*","fieldUnknownHermiona"),
    (0xc,"UnknownCGruntzMgrLucius*","fieldUnknownHagrid"),(0x10,"UnknownCGruntzMgrLucius*","fieldUnknownSeverus"),
    (0x14,"UnknownCGruntzMgrLucius*","fieldUnknownSirius"),(0x18,"UnknownCGruntzMgrLucius*","fieldUnknownAlbus"),
    (0x1c,"UnknownFilch*","fieldUnknownFilch"),(0x20,"UnknownVoldemort*","fieldUnknownVoldemort"),
    (0x24,"UnknownCGruntzMgrLucius*","fieldUnknownRemus"),(0x28,"UnknownMinerva*","fieldUnknownMinerva"),
    (0x2c,"UnknownCGruntzMgrLucius*","fieldUnknownPettigrew"),
    (0x30,"HWND","m_hWnd"),(0x34,"int","m_flagsUnknown"),
    (0x38,"int","fieldUnknown38"),(0x3c,"int","fieldUnknown3C")]),

  # ---- CNetMgr (partial; used for ReportError this-type) ----
  ("CNetMgr", 0x4, True,
   "CNetMgr - networking manager (C:\\Proj\\NetMgr). Layout partial. @approx tomalla 1.0.1.77",
   [(0x0,"void*","__vftable")]),

  # ---- On-disk formats: DEFINE for reference, NOT applied as this-type ----
  ("WwdRect", 0x10, False,
   "WWD inclusive rectangle {left,top,right,bottom} (editor labels). @todo: layout not confirmed",
   [(0x0,"int","left"),(0x4,"int","top"),(0x8,"int","right"),(0xc,"int","bottom")]),
]

# WwdObject is field-set-known but offsets @todo: define as a flat sequential
# reference struct (NOT applied as this). Built separately below.

# RezDirEntry similarly.

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
def seq(names, start=0):
    return [(n, start + i) for (i, n) in enumerate(names)]

ENUMS = [
  ("GruntType", 4, "36 grunt types (GRUNTZ_<TYPE>). Values 0..N sequential, @todo unverified",
   seq(["GRUNT_NORMAL","GRUNT_WAND","GRUNT_CLUB","GRUNT_SWORD","GRUNT_GLOVEZ",
        "GRUNT_GAUNTLETZ","GRUNT_SHOVEL","GRUNT_SPRING","GRUNT_GOOBER","GRUNT_BOMB",
        "GRUNT_TIMEBOMB","GRUNT_ROCK","GRUNT_BRICK","GRUNT_BOOMERANG","GRUNT_NERFGUN",
        "GRUNT_GUNHAT","GRUNT_SHIELD","GRUNT_SPY","GRUNT_TOOB","GRUNT_TOOBWATER",
        "GRUNT_WELDER","GRUNT_WINGZ","GRUNT_GRAVITYBOOTZ","GRUNT_WARPSTONE","GRUNT_SCROLL",
        "GRUNT_REAPER","GRUNT_POGOSTICK","GRUNT_YOYO","GRUNT_JUMPROPE","GRUNT_BEACHBALL",
        "GRUNT_BIGWHEEL","GRUNT_BABYWALKER","GRUNT_GOKART","GRUNT_JACKINTHEBOX",
        "GRUNT_SQUEAKTOY","GRUNT_HAREKRISHNA"])),

  ("Tool", 4, "22 Toolz (TOOLZ_<NAME>). @todo unverified",
   seq(["TOOL_BOMBZ","TOOL_BOOMERANGZ","TOOL_BRICKZ","TOOL_CLUBZ","TOOL_GAUNTLETZ",
        "TOOL_GLOVEZ","TOOL_GOOBERZ","TOOL_GRAVITYBOOTZ","TOOL_GUNHATZ","TOOL_NERFGUNZ",
        "TOOL_ROCKZ","TOOL_SHIELDZ","TOOL_SHOVELZ","TOOL_SPRINGZ","TOOL_SPYZ","TOOL_SWORDZ",
        "TOOL_TIMEBOMBZ","TOOL_TOOBZ","TOOL_WANDZ","TOOL_WARPSTONEZ1","TOOL_WELDERZ","TOOL_WINGZ"])),

  ("Toy", 4, "10 Toyz (TOYZ_<NAME>). @todo unverified",
   seq(["TOY_BABYWALKERZ","TOY_BEACHBALLZ","TOY_BIGWHEELZ","TOY_GOKARTZ","TOY_JACKINTHEBOXZ",
        "TOY_JUMPROPEZ","TOY_POGOSTICKZ","TOY_SCROLLZ","TOY_SQUEAKTOYZ","TOY_YOYOZ"])),

  ("Warlord", 4, "4 enemy bosses (WARLORDZ_<NAME>). @todo unverified",
   seq(["WARLORD_KING","WARLORD_NAPOLEAN","WARLORD_PATTON","WARLORD_VIKING"])),

  ("Powerup", 4, "5 powerupz (POWERUPZ_<NAME>). @todo unverified",
   seq(["POWERUP_COIN","POWERUP_GHOST","POWERUP_MINICAM","POWERUP_ROIDZ","POWERUP_CONVERSION"])),

  ("ColorTint", 4, "17 ownership-tint colors. @todo unverified",
   seq(["TINT_BLACK","TINT_BLUE","TINT_CYAN","TINT_DKBLUE","TINT_DKGREEN","TINT_DKRED",
        "TINT_DKYELLOW","TINT_GREEN","TINT_GREY","TINT_HOTPINK","TINT_ORANGE","TINT_PINK",
        "TINT_PURPLE","TINT_RED","TINT_TURQ","TINT_WHITE","TINT_YELLOW"])),

  ("Direction", 4, "8-way movement (clockwise from North, GUESS). @todo unverified",
   seq(["DIR_NORTH","DIR_NORTHEAST","DIR_EAST","DIR_SOUTHEAST","DIR_SOUTH","DIR_SOUTHWEST",
        "DIR_WEST","DIR_NORTHWEST"])),

  ("Statez", 4, "game state-machine state ids (STATEZ_*). @todo unverified",
   seq(["STATEZ_SPLASH","STATEZ_MENU","STATEZ_HELP","STATEZ_CREDITZ","STATEZ_ATTRACT",
        "STATEZ_PREVIEW","STATEZ_BOOTY","STATEZ_MULTI"])),

  ("LaunchMode", 4, "WinMain command-line dispatch tokens (string mapping). @note flags not strict enum",
   seq(["LAUNCH_PLAY","LAUNCH_MULTI","LAUNCH_DEMO","LAUNCH_ATTRACT","LAUNCH_SELECT",
        "LAUNCH_EDIT","LAUNCH_HOST","LAUNCH_JOIN","LAUNCH_LOAD","LAUNCH_LOADGAME",
        "LAUNCH_NOLOGO","LAUNCH_NOMOVIES","LAUNCH_LOBBYLAUNCH","LAUNCH_QUICKSTART"])),

  ("LaunchModeCode", 4, "REAL internal launch mode codes tomalla recovered. @approx tomalla 1.0.1.77",
   [("LAUNCHCODE_DEFAULT",2),("LAUNCHCODE_PLAY",3),("LAUNCHCODE_DEMO",7),
    ("LAUNCHCODE_SELECT",16),("LAUNCHCODE_MULTI",17)]),

  ("Resolution", 4, "Resolution registry value -> WxH. Default 1 (640x480). @approx tomalla 1.0.1.77",
   [("RES_640x480",1),("RES_800x600",2),("RES_1024x768",3)]),

  ("Commands", 4, "WM_COMMAND notification codes. @approx tomalla 1.0.1.77",
   [("LOBBYLAUNCH",0x80B7)]),

  ("WwdObjectFlags", 4, "WWD Object Flags bits (editor labels, label order). @todo bit values unverified",
   seq(["WWD_OBJ_NO_HIT","WWD_OBJ_ALWAYS_ACTIVE","WWD_OBJ_SAFE","WWD_OBJ_AUTO_HIT_DAMAGE",
        "WWD_OBJ_DIFFICULT","WWD_OBJ_EYE_CANDY","WWD_OBJ_HIGH_DETAIL","WWD_OBJ_MULTIPLAYER",
        "WWD_OBJ_EXTRA_MEMORY","WWD_OBJ_FAST_CPU","WWD_OBJ_NO_DRAW","WWD_OBJ_MIRROR",
        "WWD_OBJ_INVERT","WWD_OBJ_FLASH"])),

  ("WwdHitTypeFlags", 4, "WWD Hit Type / Object Type bits (editor labels). @todo bit values unverified",
   seq(["WWD_HIT_GENERIC","WWD_HIT_PLAYER","WWD_HIT_ENEMY","WWD_HIT_POWERUP","WWD_HIT_SHOT",
        "WWD_HIT_PSHOT","WWD_HIT_ESHOT","WWD_HIT_SPECIAL","WWD_HIT_USER1","WWD_HIT_USER2",
        "WWD_HIT_USER3","WWD_HIT_USER4"])),
]

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
    # map class name -> our defined struct name
    mapping = {
        "RegistryHelper": "RegistryHelper", "Font": "Font",
        "CGruntzMgr": "CGruntzMgr", "CGruntzApp": "CGruntzApp", "CGruntzWnd": "CGruntzWnd",
        "CGameMgr": "CGameMgr", "CGameApp": "CGameApp", "CGameWnd": "CGameWnd",
        "UnknownClassArrays": "UnknownClassArrays",
        "UnknownClassInCGruntzMgr": "UnknownClassInCGruntzMgr",
        "UnknownClassCGruntzMgrHarryPotter": "UnknownClassCGruntzMgrHarryPotter",
        "UnknownCGruntzMgrLucius": "UnknownCGruntzMgrLucius",
        "UnknownDraco": "UnknownDraco", "UnknownHermiona": "UnknownHermiona",
        "UnknownHagrid": "UnknownHagrid", "UnknownSeverus": "UnknownSeverus",
        "UnknownSirius": "UnknownSirius", "UnknownAlbus": "UnknownAlbus",
        "UnknownRemus": "UnknownRemus", "UnknownMinerva": "UnknownMinerva",
        "UnknownPettigrew": "UnknownPettigrew", "UnknownFilch": "UnknownFilch",
        "UnknownSalazar": "UnknownSalazar", "UnknownVoldemort": "UnknownVoldemort",
        "CNetMgr": "CNetMgr",
    }
    sname = mapping.get(leaf, None)
    if sname is None:
        return None
    return dtm.getDataType(GRUNTZ_CAT, sname)

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
    effective_structs = gen_structs_list + [s for s in STRUCTS if s[0] not in gen_names]
    R("structs: %d generated + %d hardcoded fallback (graduate-on-match)" %
      (len(gen_structs_list), len(effective_structs) - len(gen_structs_list)))
    n_structs = 0
    struct_dt_by_name = {}
    for (name, size, apply_this, desc, fields) in effective_structs:
        dt = get_or_create_struct(name, size, desc, fields)
        struct_dt_by_name[name] = (dt, apply_this)
        n_structs += 1
    # WwdObject + RezDirEntry: field-set reference structs (sequential, @todo offsets)
    wwd_fields_seq = [
        ("name","int"),("logic","int"),("imageSet","int"),("score","int"),("points","int"),
        ("powerup","int"),("damage","int"),("smarts","int"),("health","int"),
        ("speedX","int"),("speedY","int"),("speed","int"),("faceDir","int"),("direction","int"),
        ("counter","int"),("width","int"),("height","int"),("timeDelay","int"),("frameDelay","int"),
        ("zCoord","int"),("xCoord","int"),("yCoord","int"),
        ("xMin","int"),("xMax","int"),("yMin","int"),("yMax","int"),
        ("xMoveRes","int"),("yMoveRes","int"),("xTweak","int"),("yTweak","int"),
        ("userValue","int"),
        ("drawFlags","uint"),("dynamicFlags","uint"),("userFlags","uint"),
        ("hitTypeFlags","uint"),("objectFlags","uint"),("autoHitDamage","int"),
    ]
    wwd = StructureDataType(GRUNTZ_CAT, "WwdObject", 0)
    wwd.setDescription("WWD world-object record. Field SET high confidence (editor labels); ORDER/OFFSETS @todo (sequential placeholder). NOT applied as this-type.")
    for (fn_, ct_) in wwd_fields_seq:
        wwd.add(resolve_type(ct_), 4, fn_, None)
    wwdrect = dtm.getDataType(GRUNTZ_CAT, "WwdRect")
    if wwdrect is not None:
        for rn in ["hitRect","attackRect","clipRect","moveRect","user1Rect","user2Rect"]:
            wwd.add(wwdrect, wwdrect.getLength(), rn, None)
    for i in range(1,9):
        wwd.add(resolve_type("int"), 4, "user%d" % i, None)
    dtm.addDataType(wwd, DataTypeConflictHandler.REPLACE_HANDLER); n_structs += 1

    rez = StructureDataType(GRUNTZ_CAT, "RezDirEntry", 0)
    rez.setDescription("REZ/VRZ archive directory entry {Type,Name,Size,ID}. OFFSETS @todo. NOT applied as this-type.")
    arr4 = ArrayDataType(resolve_type("char"), 4, 1)
    rez.add(arr4, 4, "type", "FOURCC")
    rez.add(resolve_type("int"), 4, "name", "CString ptr")
    rez.add(resolve_type("int"), 4, "size", None)
    rez.add(resolve_type("int"), 4, "id", None)
    dtm.addDataType(rez, DataTypeConflictHandler.REPLACE_HANDLER); n_structs += 1

    R("structs defined: %d" % n_structs)

    # ---- (B) define enums (generated JSON preferred; hardcoded = fallback) ----
    gen_enums_list = _load_gen(ENUMS_JSON, lambda o: (
        o["name"], 4, "generated by gen_structs.py from %s" % o.get("source", "src"),
        [(m["name"], m["value"]) for m in o["members"]]))
    gen_enum_names = set(e[0] for e in gen_enums_list)
    effective_enums = gen_enums_list + [e for e in ENUMS if e[0] not in gen_enum_names]
    n_enums = 0
    for (name, size, desc, members) in effective_enums:
        define_enum(name, size, desc, members)
        n_enums += 1
    R("enums: %d generated + %d hardcoded fallback" %
      (len(gen_enums_list), len(effective_enums) - len(gen_enums_list)))

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
