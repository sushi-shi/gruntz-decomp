#!/usr/bin/env python3
"""Cleanliness scoreboard - cast / placeholder / view counts that should trend to
0 as the reconstruction's type/call/name layer is cleaned up, shown WITH a delta
vs the committed baseline so a matcher can steer on its own change.

Printed by ``gruntz build`` in the report block (below the match summary); runnable
as ``python -m gruntz.cleanliness.board`` (shows counts + delta),
``--update`` (bless: write the baseline), ``--csv``. See docs/cleanliness-metrics.md.

Fast metrics count occurrences over ``src/`` + ``include/`` C++ sources with
comments and string/char literals stripped first. The optional semantic metrics
consume build IR and the retail graph and are kept out of normal builds.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
ROOTS = ("src", "include")
EXTS = {".cpp", ".cc", ".cxx", ".h", ".hpp", ".inl"}
TEXT_BASELINE = REPO / "config" / "cleanliness" / "cleanliness-text-baseline.tsv"
SEMANTIC_BASELINE = REPO / "config" / "cleanliness" / "cleanliness-semantic-baseline.tsv"

_BLOCK = re.compile(r"/\*.*?\*/", re.DOTALL)
_LINE = re.compile(r"//[^\n]*")
_STR = re.compile(r'"(?:\\.|[^"\\\n])*"')
_CHR = re.compile(r"'(?:\\.|[^'\\\n])*'")


# `// @dead-code` (docs/comment-markers.md): a PROVEN-zero-ref function - no rel32
# caller, no data-slot, no .text address-taking anywhere in the image (verified with
# `gruntz sema xref`, the default --tree). Retail emitted it (no /OPT:REF) but nothing
# reaches it, so its identity is genuinely UNRECOVERABLE (no reference to trace) and
# recovering it has no value. A placeholder name on such a fn is a permanent, non-
# actionable artifact - NOT an open identity-TODO - so the metric blanks the whole
# marked function body (marker -> its closing brace), excluding it exactly like a
# library carve-out. The marker must state the zero-ref proof (as @identity-TODO must
# state what it tried); an unproven @dead-code is a lie the reviewer must reject.
_DEAD = re.compile(r"//\s*@dead-code\b")


def _blank_dead(text: str) -> str:
    """Blank each `// @dead-code`-marked function (marker .. matching `}`) so its
    placeholder name contributes to no metric. Brace-matched from the first `{`
    after the marker; leaves newlines so line numbers are unperturbed."""
    out = list(text)
    for m in _DEAD.finditer(text):
        b = text.find("{", m.start())
        if b < 0:
            continue
        depth, i, n = 0, b, len(text)
        while i < n:
            c = text[i]
            if c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        for k in range(m.start(), min(i + 1, n)):
            if out[k] != "\n":
                out[k] = " "
    return "".join(out)


def _strip(text: str) -> str:
    text = _blank_dead(text)   # drop proven-dead fns before any metric sees them
    text = _BLOCK.sub(" ", text)
    text = _LINE.sub("", text)
    text = _STR.sub(" ", text)
    text = _CHR.sub(" ", text)
    return text


# The "placeholder classes" metric: a type whose NAME carries an RVA-like hex run
# (4+ hex incl. a digit: L_fe90, S_104a0, CVtbl_1efc58, Method117e20, Sub150c30, ...)
# is an un-attributed placeholder shell, NOT a real engine class - a fake view. This
# is LOCATION-INDEPENDENT (src AND include): moving such a shell from a .cpp into a
# header does NOT resolve it, so the metric can't be gamed by relocation. Drive to 0
# by recovering each placeholder's REAL class identity (name + shape) or removing it.
_CPP = {".cpp", ".cc", ".cxx"}
_TYPEDEF = re.compile(r"\b(?:struct|class)\s+(\w+)")
_HEXRUN = re.compile(r"[0-9a-f]{4,}")


def _is_placeholder(name: str) -> bool:
    return any(any(c.isdigit() for c in run) for run in _HEXRUN.findall(name))


def _count_placeholders(code: str) -> int:
    return sum(1 for n in _TYPEDEF.findall(code) if _is_placeholder(n))


# The ".cpp-local views" metric enforces matcher.md rule 0: a struct/class DEFINITION inside
# a .cpp is a per-TU view of a class whose one true shape belongs in a header - regardless of
# its NAME (unlike "placeholder classes", which is name-based). Counts DEFINITIONS (name then
# a body brace, optional base-clause), NOT forward-decls / elaborated uses / anonymous aggs.
# cpp_only + scaffolding-excluded => only real main-tree .cpp files count; homing a view into
# one trips it. Drive to ~0 by moving the type to include/<Module>/.
_TYPEDEF_DEF = re.compile(r"\b(?:struct|class)\s+(\w+)\b(?:\s+final)?\s*(?::[^;{]*)?\{")


def _count_cpp_local_defs(code: str) -> int:
    return len(_TYPEDEF_DEF.findall(code))


# The "placeholder vtable slots" metric (GAMEABLE, but tracked per request): a
# virtual whose NAME is a placeholder for an unresolved vtable slot (dummyN / vNN /
# vfunc / SlotN) - a real virtual with an un-recovered identity. Counts the DECL
# sites (one per slot). Drive to 0 via the vtable_hierarchy TOPOLOGICAL override
# analysis (inherit the base's slots, name the rest from the slot RVA), NOT by
# hand-renaming - see `python -m gruntz.core.vtable_hierarchy --audit/--coverage`.
_VTSLOT = re.compile(
    r"virtual\b[^;{}\n]*\b(?:dummy[0-9]+|vfunc[0-9]*|[Ss]lot[0-9]+"
    r"|[sv][0-9a-f]{2,3}(?![0-9a-z])"  # v04 / s04 offset-named fake slots (both are placeholders)
    r"|[SsVv]f[0-9a-f]+)\s*\("          # Sf1 / vf10 slot-forwarders
)


# ")m_ casts" (a C-style cast applied to a member, `(CFoo*)m_54`) is a VECTOR of fake-view
# propagation - a caller casts an untyped/placeholder member to a fake view. RATCHETED down.
# EXCLUDES the legit string casts (char*)/(const char*)m_x (a member byte-buffer): those are the
# allowed exception and already counted under "(char*) casts". ")this casts" (casting `this` to a
# fake view) is ALWAYS the vector - ratcheted with no exception. Killing these dissolves the views.
_M_CAST = re.compile(r"\)m_[A-Za-z0-9_]")
_STR_M_CAST = re.compile(r"\((?:const |unsigned |signed )*char ?\*\)m_[A-Za-z0-9_]")


def _count_nonstring_m_casts(code: str) -> int:
    return len(_M_CAST.findall(code)) - len(_STR_M_CAST.findall(code))


# Offset-access cast-hiding macros: `#define F(p,o) (*(i32*)((char*)(p)+(o)))` (also P/PTR/I32/DBL/
# M/W/WTS_*/PB_*/GREG_*/FEC_*/I32AT). Each expands to the raw-offset cast the ratchet counts but
# HIDES every call-site (the define counts once as a `(char*)` cast; the N uses vanish at
# preprocessing). Count each such macro's def + every call-site -> the true hidden-cast footprint.
# Cast-ratchet EVASION; dissolve into real typed member access `p->m_field`.
_OFFSET_MACRO_DEF = re.compile(
    r"#define\s+(\w+)\s*\([^)]*\)\s*\(\s*\*\s*\(\s*\w[\w ]*\*\s*\)\s*\(\s*\(\s*char\s*\*\s*\)"
)


def _count_offset_macro_casts(code: str) -> int:
    total = 0
    for m in _OFFSET_MACRO_DEF.finditer(code):
        total += len(re.findall(r"\b" + re.escape(m.group(1)) + r"\s*\(", code))
    return total


# Per-TU declarations: a global/function re-declared in a consumer .cpp instead of living in its
# OWNER's header (which consumers #include). `extern "C"` DATA-array globals that need it for
# mangling are the residual; the rest belong in owner headers. A header `extern` IS the owner decl.
_CPP_EXTERN = re.compile(r"^\s*extern\b", re.MULTILINE)

# ... but ONLY the owner's. This metric read 0 for months while 45 globals were
# `extern`-declared in 2+ headers (g_frameTime in EIGHT, and GruntzMgr.h declared
# eight of its own globals TWICE) - the banned construct had simply moved out of the
# .cpp files the counter scans, and the counter went quiet instead of following it.
# So: count the REDUNDANT header declarations (occurrences - 1 per name), which is
# exactly the debt. One declaration per symbol, in its owner header, is 0 here.
#
# Why redundancy and not "header externs": a header extern IS the legitimate owner
# decl, so counting them all would ratchet against correct code. A SECOND one is
# always wrong - the two copies drift (type, linkage, array bound), and every TU
# that sees both pays for the duplicate in cl 5.0's cumulative declaration count,
# which steers /O2 register allocation (docs/patterns/declaration-count-window-
# steers-regalloc.md).
_HEADER_EXTS = {".h", ".hpp", ".inl"}
# `extern` up to the declarator: skip the optional language linkage and the type,
# then take every comma-separated declarator name before the `;`. A `{` means an
# `extern "C" { ... }` BLOCK, not a declaration - its members are matched on their
# own lines (they may or may not respell `extern`, so the block body is rescanned).
_EXTERN_DECL = re.compile(
    r'\bextern\b\s*(?:"[^"]*")?\s*(?![{;])([^;{}]*);',
    re.DOTALL,
)
# NOTE the optional linkage string: `_strip` has already blanked `"C"` to a space by
# the time these run, so a regex that REQUIRES the quotes matches nothing. That bug
# silently dropped every bare member of an `extern "C" { ... }` block.
_EXTERN_BLOCK = re.compile(r'\bextern\s*(?:"[^"]*")?\s*\{')
_IDENT = re.compile(r"[A-Za-z_]\w*")
# A `(` opens a prototype's parameter list (or a function-pointer declarator); the
# declared name is the last identifier BEFORE it. `[` opens an array bound.
_ARRAY_BOUND = re.compile(r"\[[^\]]*\]")
# ... except when the `(` belongs to a DECL-SPECIFIER, which is not the declarator:
# `extern "C" __declspec(dllimport) unsigned long WINAPI timeGetTime(void);` would
# otherwise report `__declspec` as the declared symbol (it did; two umbrella headers
# then read as a duplicate pair of it).
_DECLSPEC = re.compile(r"\b(?:__declspec|__attribute__)\s*\([^()]*(?:\([^()]*\)[^()]*)*\)")


def _declarator_name(part: str) -> str | None:
    """The symbol a single declarator declares - the last identifier in it once the
    decl-specifiers, parameter list and array bounds are cut. `const double g_max`
    -> g_max; `char g_slots[TINT_COUNT]` -> g_slots; `void Sink(const char* m)` ->
    Sink; `__declspec(dllimport) unsigned long WINAPI timeGetTime(void)` -> the
    function, not the attribute."""
    head = _DECLSPEC.sub(" ", part)           # cut __declspec(...)/__attribute__(...)
    head = head.split("(", 1)[0]              # cut a prototype's parameter list
    head = _ARRAY_BOUND.sub(" ", head)        # cut array bounds
    ids = _IDENT.findall(head)
    return ids[-1] if ids else None


def _header_extern_names(code: str) -> list[str]:
    """Every symbol an `extern` declaration in this header declares.

    Handles the four spellings this tree uses: plain `extern T g;`, language-linkage
    `extern "C" T g;`, comma lists (`extern i32 a, b, c;`) and `extern "C" { ... }`
    blocks (whose members are re-scanned, so a member that omits `extern` still
    counts). Array bounds and prototype parameter lists are stripped before the name
    is taken, so `g_slots[TINT_COUNT]` and `Sink(const char*)` both yield one name.
    """
    names: list[str] = []
    # An `extern "C" { ... }` block gives every declaration inside it external
    # linkage whether or not it respells `extern`; splice those bodies in so the
    # bare members are seen. (FrameClock.h respells it, NetLobby's blocks do not.)
    spliced = code
    for m in _EXTERN_BLOCK.finditer(code):
        depth, i, n = 0, m.end() - 1, len(code)
        while i < n:
            if code[i] == "{":
                depth += 1
            elif code[i] == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        body = code[m.end():i]
        spliced += "\n" + "\n".join(
            f"extern {stmt};" for stmt in body.split(";")
            if stmt.strip() and "extern" not in stmt
        )
    for m in _EXTERN_DECL.finditer(spliced):
        decl = m.group(1)
        if "=" in decl:          # a definition with an initialiser, not a declaration
            continue
        # split the declarator list at top level (commas inside (...) are parameters)
        parts, depth, cur = [], 0, ""
        for ch in decl:
            if ch in "([":
                depth += 1
            elif ch in ")]":
                depth -= 1
            if ch == "," and depth == 0:
                parts.append(cur)
                cur = ""
            else:
                cur += ch
        parts.append(cur)
        for part in parts:
            name = _declarator_name(part)
            if name:
                names.append(name)
    return names


# `<Mfc.h>` and `<Win32.h>` are the two MUTUALLY EXCLUSIVE platform umbrellas -
# Mfc.h is a superset of Win32.h and including both trips MFC's C1189 - so a symbol
# declared once in each is never declared twice in any TU. Their mirrored preludes
# (timeGetTime, INT_PTR) are that, not duplication. Any THIRD declaring header, or a
# repeat inside one umbrella, still counts.
_UMBRELLA_PAIR = {"include/Mfc.h", "include/Win32.h"}


def _duplicate_header_externs() -> dict[str, list[str]]:
    """symbol -> the header paths declaring it, for every symbol declared in 2+
    places (the same header twice counts: GruntzMgr.h had eight such pairs)."""
    import collections
    seen: dict[str, list[str]] = collections.defaultdict(list)
    for root in ROOTS:
        base = REPO / root
        if not base.is_dir():
            continue
        for path in sorted(base.rglob("*")):
            if path.suffix not in _HEADER_EXTS or not path.is_file():
                continue
            try:
                code = _strip(path.read_text(errors="ignore"))
            except OSError:
                continue
            rel = str(path.relative_to(REPO))
            for name in _header_extern_names(code):
                seen[name].append(rel)
    return {k: v for k, v in seen.items()
            if len(v) > 1 and set(v) != _UMBRELLA_PAIR}

# A function prototype has external linkage even when it does not spell `extern`:
#
#     void ChannelSlots_Set(i32, i32);
#     namespace NetLobby { void __stdcall AppendEditLine(HWND, char*); }
#
# `_CPP_EXTERN` misses both. Scan only file/namespace scope (not function bodies or
# class definitions), split it into semicolon-terminated declarations, and count
# function-shaped declarators. This intentionally treats a namespace-scope
# most-vexing-parse form as a prototype too; use a real initializer form for objects.
_CPP_PROTO = re.compile(
    r"^\s*"
    r"(?!typedef\b|using\b|return\b|if\b|for\b|while\b|switch\b)"
    r"(?:(?:extern|static|inline|constexpr|friend)\s+)*"
    r"(?:[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*(?:\s*<[^;{}()]*>)?"
    r"(?:\s+const)?\s*(?:[*&]\s*)?\s+)+"
    r"(?:__cdecl\s+|__stdcall\s+|WINAPI\s+|CALLBACK\s+)*"
    r"[A-Za-z_~]\w*(?:::[A-Za-z_~]\w*)*\s*"
    r"\([^;{}]*\)\s*"
    r"(?:(?:const|volatile|noexcept|OVERRIDE)\s*|=\s*0\s*)*$",
    re.DOTALL,
)
# A qualified static-data definition with a scalar direct initializer is not a
# prototype. This shape is required for non-copyable VC5 types such as
# `CPtrList Pool<T>::s_list(10)`, so copy-initialization is not an available
# spelling merely to appease the textual census.
# A qualified definition with a direct-initialiser - `CActReg
# CActRegPool<T>::s_table(2000, 2010);` - is a DEFINITION, not a prototype.
# The first argument may be a literal OR a SCREAMING_SNAKE constant: this
# campaign replaces those literals with named constants, and keying the
# exemption on the literal alone made all 51 CActRegPool definitions read as
# prototypes the moment they were named. No type in this tree is spelled
# SCREAMING_SNAKE, so a parameter declaration cannot match.
_CPP_QUALIFIED_DIRECT_INIT = re.compile(
    r"::[A-Za-z_]\w*\s*\(\s*(?:0[xX][0-9A-Fa-f]+|[0-9]+|[A-Z][A-Z0-9_]{2,})"
    r"(?:\s*[,)]|[uUlL]+\s*[,)])"
)
_NAMESPACE_OPEN = re.compile(r"(?:^|[;{}])\s*(?:inline\s+)?namespace(?:\s+\w+(?:::\w+)*)?\s*$")
_EXTERN_BLOCK_OPEN = re.compile(r"(?:^|[;{}])\s*extern\s*$")


def _count_cpp_external_prototypes(code: str) -> int:
    """Count external function prototypes at .cpp file/namespace scope."""
    # Preprocessor directives are not declarations and can otherwise become a
    # prefix of the next semicolon-terminated statement.
    code = re.sub(r"(?m)^\s*#.*$", "", code)
    contexts: list[bool] = []
    statement_start = 0
    total = 0
    for i, ch in enumerate(code):
        allowed = all(contexts)
        if ch == "{":
            prefix = code[statement_start:i]
            opens_decl_scope = bool(
                _NAMESPACE_OPEN.search(prefix) or _EXTERN_BLOCK_OPEN.search(prefix)
            )
            contexts.append(allowed and opens_decl_scope)
            statement_start = i + 1
        elif ch == "}":
            if contexts:
                contexts.pop()
            statement_start = i + 1
        elif ch == ";":
            statement = code[statement_start:i]
            if (
                allowed
                and _CPP_PROTO.match(statement)
                and not _CPP_QUALIFIED_DIRECT_INIT.search(statement)
            ):
                total += 1
            statement_start = i + 1
    return total


# --- cast ratchets ---
# Policy (docs/cast-metric-policy.md): C-style `(T)expr` casts are BANNED; every cast is a
# C++ NAMED cast (static_cast/reinterpret_cast/const_cast/dynamic_cast). Named casts remain
# visible debt: every reinterpret_cast is counted and ratcheted down because it is often the
# only lexical evidence of an integer carrier, fake class facet, or member-address re-view.
_C_THIS_CAST = re.compile(r"\)this\b")
_CHAR_STAR_CAST = re.compile(r"(?<![\w>)])\((?:const |unsigned |signed )*char ?\*\)")
_NUMERIC_CAST = re.compile(
    r"(?<![\w>)])\((?:i8|i16|i32|i64|u8|u16|u32|u64|float|double|char|short|int|long|unsigned)\)")
_REINTERPRET_CAST = re.compile(r"\breinterpret_cast\s*<")

# Artificial COMDAT emitters make the compiler materialize an otherwise-inline
# constructor/destructor solely so the matching pipeline can label it. They are
# reconstruction scaffolding, not original source structure: current emission must
# come from a real caller, and a retail-only copy remains an unmatched inventory row.
_FORCED_COMDAT_EMITTER = re.compile(r"\bForceEmit\w*\b|\b\w+_OOL_(?:CTOR|DTOR)\b")


# NOT A METRIC, deliberately - do not add `reinterpret_cast<char*>(x) + N` as a
# ratcheted row. It looks like the banned offset-cast, and it was added here once on
# that assumption, but all 13 tree-wide sites were checked and every one is byte
# arithmetic over data that has no member to name:
#
#   RIFF chunk walking (chunks are variable-length by definition) - DirectSoundMgr
#   BMP bfOffBits / DIB biSize + 0x400 palette                    - FileImage
#   PCX + PID trailing 0x300 palettes, fixed header skips         - FileImage
#   an RLE blob walked from its typed head                        - ImageOwned
#   MFC's PRIVATE AFX_MODULE_STATE +4 slot                        - CustomLevelDlg
#
# The ban (CLAUDE.md) is on `(char*)obj + N` used to reach a MEMBER of a class WE
# model - "named member &x->m_field" - because that is a mis-modeled declaration.
# Serialized file formats and foreign layouts have no such member. Every row on this
# board is contracted to reach 0, so a row that should NOT reach 0 would drive
# someone to break correct code. The C-style spelling and the offset-cast macros
# stay metered (both 0); they are the shapes that do indicate a mis-model.


def _count_c_style_casts(code: str) -> int:
    """Every C-style `(T)expr` cast, one number: `)this`, `(T)m_x`, `(char*)`, numeric
    `(i32)`, and the offset-cast-hiding macros. All must be C++ named casts instead."""
    return (len(_C_THIS_CAST.findall(code)) + _count_nonstring_m_casts(code)
            + len(_CHAR_STAR_CAST.findall(code)) + len(_NUMERIC_CAST.findall(code))
            + _count_offset_macro_casts(code))


# (label, matcher, cpp_only). matcher = compiled regex (findall count) OR a callable
# code->int for structural counts. Occurrences summed over stripped code.
def _count_unexplained_casts(code: str) -> int:
    """The reinterpret_casts the cast ledger cannot account for (see
    gruntz.audit.cast_ledger for what counts as explained).

    Takes RAW text (see `_NEEDS_RAW`): the reasons this metric credits are written in
    comments, so a stripped body can never match them. The per-line handling below is
    kept identical to the ledger's scan() - drop the `//` tail before looking for a
    cast, so a cast NAMED in prose is not counted as a site, but keep the comment in
    the context window, which is where the reason lives."""
    from gruntz.audit.cast_ledger import CAST, FORCED, REASON
    import re as _re
    lines = code.split("\n")
    n = 0
    for i, line in enumerate(lines):
        for _ in CAST.finditer(line.split("//", 1)[0]):
            ctx = " ".join(lines[max(0, i - 3):i + 2])
            if any(_re.search(pat, line) or _re.search(pat, ctx) for _, pat in FORCED):
                continue
            if REASON.search(ctx):
                continue
            n += 1
    return n


METRICS = (
    ("m_<hex> fields", re.compile(r"\bm_[0-9a-f]{2,}\b"), False),
    ("Unknown ids", re.compile(r"\b\w*[Uu]nknown\w*\b"), False),
    ("g_<hex> globals", re.compile(r"\bg_[0-9a-f]{4,}\b"), False),
    # Placeholder/orphan function names (identity not recovered). Covers BOTH the underscore
    # (Method_c2a50, Gap_184900, Sub_c3e30) and no-underscore (Method0a90) forms, and the
    # proximity-homed Gap_/Ghidra FUN_ orphans that the old regex missed entirely.
    ("Method/Stub/FUN/Gap",
     re.compile(r"\b(?:(?:Method|Gap|Sub|Stub|Fwd|Func|FUN|Nullsub|Handler|LogicHandler|winapi)_?[0-9a-f]{4,}|vfunc_[0-9]+)\b"), False),
    # Virtual vtable SLOTS named by index+RVA (role unrecovered): SlotNN_<hex>, Vfunc<hex>,
    # Vtbl_<hex>. Real correctly-modeled virtuals, placeholder-named -> vtable-slot naming backlog.
    # NOTE the bare `v<N>` form (`virtual void v0();`) is ALSO a synthetic slot name and was
    # evading every metric. Match it ONLY in the DECLARATION shape - bare v0/v1/v2 identifiers
    # are legit vertex locals in the raster/poly code, so a plain \bv[0-9]+\b would be wrong.
    ("virtual slot placeholders",
     re.compile(r"\b(?:Slot[0-9]{1,2}_[0-9a-f]{4,}|Vfunc[0-9a-f]+|Vtbl_[0-9a-f]{4,})\b"
                r"|\bvirtual\b[^;{\n]*?\bv[0-9]+\s*\("), False),
    # Positional PARAMETER placeholders: `i32 a3`, `i32* p4`, `void* arg2`. The digit IS the
    # argument index - the decompiler's default - so the name carries no information at all
    # and the reader has to re-derive the meaning from the body every time. This is the
    # parameter-side twin of `m_<hex> fields`, and the same rule applies: recover the meaning
    # from the call sites and the body, then name it.
    #
    # Deliberately EXCLUDED, because it is not dead weight:
    #   * bare `unused` (`i32 WriteSnapshot(CFileMemBase*, i32 unused)`) - that name is a real
    #     claim about the ABI, namely that retail ignores the slot. Naming it `unused` is the
    #     recovered meaning, not a placeholder for one.
    #   * `v[0-9]+` - already covered by "virtual slot placeholders", and bare v0/v1/v2 are
    #     legitimate vertex locals in the raster/poly code.
    # Matched only when preceded by a TYPE token, so an ordinary `a1`/`p2` expression variable
    # elsewhere cannot inflate the count.
    ("positional arg placeholders",
     re.compile(r"\b(?:i8|u8|i16|u16|i32|u32|i64|u64|float|double|bool|char|short|int|long|void"
                r"|[A-Z]\w*)\s*(?:\*\s*|&\s*)*\b(?:a|p|arg)[0-9]+\b"), False),
    ("placeholder classes", _count_placeholders, False),
    (".cpp-local views", _count_cpp_local_defs, True),
    # --- manual-vtable residue (the de-hack / vtable-review targets) ---
    ("placeholder vtable slots", _VTSLOT, False),
    ("*Vtbl structs", re.compile(r"\b(?:struct|class)\s+\w*Vtbl\w*"), False),
    ("->vtbl accesses", re.compile(r"->\s*\w*[Vv]tbl\w*"), False),
    ("g_*Vtbl globals", re.compile(r"\bg_\w*[Vv]tbl\w*"), False),
    ("m_vtbl/m_vptr members", re.compile(r"\bm_v(?:tbl|ptr)\w*"), False),
    # --- casts: C-style is forbidden; reinterpret_cast debt may only decrease ---
    # --- numeric-domain metrics (docs/enum-modeling-plan.md) ---------------------
    # A `case` label that is a bare number is an un-named member of some domain: the
    # reader has to re-derive what the number means every time, and nothing stops one
    # domain's value being switched on another's key. Naming it is matching-neutral
    # (docs/patterns/enum-domains.md: literal -> enumerator leaves .text identical),
    # so this is pure debt. Drive to 0 by declaring the domain (GZ_ENUM_*) and using
    # its enumerators. Deliberately NOT counted: `case 0:`/`case 1:` are excluded
    # nowhere - a two-valued switch is still a domain, and the bool-ish ones are rare
    # enough that exempting them would hide real cases.
    ("magic case labels", re.compile(r"^[ \t]*case[ \t]+(?:0[xX][0-9a-fA-F]+|-?[0-9]+)[ \t]*:", re.M), False),
    # The comparison twin: `x == 0x36` where 0x36 is a domain member. `== 0` and
    # `== 1` are EXCLUDED - those are overwhelmingly null/bool tests, not domain
    # membership, and counting them would swamp the signal with noise.
    ("unnamed domain compares",
     re.compile(r"[=!]=[ \t]*(?:0[xX](?!0\b|1\b)[0-9a-fA-F]+|(?!0\b|1\b)[0-9]+)\b"), False),
    # An enum defined inside a .cpp is fine when the domain is genuinely TU-private,
    # but the `.cpp-local views` metric only looks at struct/class, so enums were
    # invisible to it. A cross-TU domain stranded in one .cpp gets re-declared in the
    # next TU that needs it - which is how this tree ended up with three spellings of
    # the grunt/pickup id space. Ratcheted so no NEW ones appear.
    (".cpp-local enums", re.compile(r"\bGZ_ENUM_(?:BEGIN|BEGIN_SPLIT|CONST_BEGIN|FLAGS_BEGIN)\b|^[ \t]*(?:typedef[ \t]+)?enum[ \t]+\w*[ \t]*\{", re.M), True),
    ("C-style casts", _count_c_style_casts, False),
    ("reinterpret_casts", _REINTERPRET_CAST, False),
    # The count above is raw; THIS one is the campaign's real worklist - the casts
    # nobody has explained yet (gruntz.audit.cast_ledger sorts the forced/seamed ones
    # out). An unexplained cast is indistinguishable from un-started work.
    ("unexplained casts", _count_unexplained_casts, False),
    ("void* m_ members", re.compile(r"\bvoid ?\* m_"), False),
    # --- metric-evasion / placeholder hacks (2026-07-14 de-hack campaign; MAX-fuzzy gate) ---
    ("offset-cast macros", _count_offset_macro_casts, False),
    ("forced COMDAT emitters", _FORCED_COMDAT_EMITTER, False),
    ("cpp extern decls", _CPP_EXTERN, True),
    ("cpp external prototypes", _count_cpp_external_prototypes, True),
)


# The RATCHET set: metrics that only go DOWN (main-tree). The *Views.h view-scaffolding is
# reconstruction machinery, NOT main-tree code - its fake-view/placeholder shells (and their
# casts) are EXPECTED, so they don't count here. (The src/Stub/ backlog that this also used to
# exempt is gone.) A view
# (or a `)this`/`)m_` cast that props a view) only counts once it lands in a real main-tree TU,
# where it must resolve to a proper class. Includes the two cast VECTORS of fake-view propagation:
# `)this casts` (always) and `)m_ casts` (string-cast-excluded). Other metrics (m_<hex>, string
# casts, ...) still count everywhere and are tracked, not ratcheted.
_VIEW_METRICS = {"placeholder classes", ".cpp-local views", "placeholder vtable slots",
                 "*Vtbl structs", "->vtbl accesses", "g_*Vtbl globals", "m_vtbl/m_vptr members"}


def _is_scaffolding(path) -> bool:
    return path.name.endswith("Views.h")


# The caller_callee FAKE-VIEW metric: our source calls an ALREADY-RECONSTRUCTED callee through
# the WRONG class-view name (mangles to a name that doesn't resolve to the callee's RVA) -> retype
# the receiver to the real class and the edge reconciles (and the caller's % rises). This is the
# ONE genuinely drive-to-0 slice of caller_callee. The other unreconciled kinds (MISSING /
# MISSING-SPECIAL / UNANALYZED, ~2268) are NOT tracked here: they are dominated by FALSE POSITIVES
# - inline member ctor/dtor/operator ops, indirect/PMF/virtual calls the static tool can't resolve,
# and near-exact codegen residue (a 92%-matched fn shows "MISSING" edges for its inlined zBitVec
# member ops). Tracking that total as drive-to-0 was wrong. NOT a text scan (reads clang IR + the
# retail graph), so it needs a build; gracefully absent when no build IR.
_SEMANTIC_LABELS = ("caller-callee FAKE-VIEW", "nested static_casts")
_SEMANTIC_LABEL_SET = set(_SEMANTIC_LABELS)


def _caller_callee_counts() -> dict[str, int]:
    import subprocess
    try:
        proc = subprocess.run(
            [sys.executable, "-m", "gruntz.cleanliness.caller_callee", "--metric"],
            capture_output=True, text=True, timeout=600, cwd=str(REPO),
        )
        if proc.returncode != 0:
            # a crashed tool exits non-zero with empty stdout; without this check the
            # empty parse below returns {} with NO message - the silent-skip hole.
            tail = (proc.stderr or "").strip().splitlines()[-1:] or ["no stderr"]
            raise RuntimeError(f"exit {proc.returncode}: {tail[0]}")
        out = proc.stdout
    except Exception as exc:
        # Say so. Returning {} drops these rows from the scoreboard, and a silently
        # absent ratcheted metric is indistinguishable from a clean 0 (see
        # merge_baseline_downonly, which now preserves the floor rather than dropping it).
        print(f"  cleanliness: caller-callee metrics UNMEASURED ({type(exc).__name__}: "
              f"{exc}) - their baseline floors are carried forward, not re-blessed",
              file=sys.stderr)
        return {}
    res: dict[str, int] = {}
    for line in out.splitlines():
        m = re.search(r"UNRECONCILED.*:\s*(\d+)", line)  # "... drive to 0): 2366" -> 2366
        if m:
            res["caller-callee unreconciled"] = int(m.group(1))
        m = re.search(r"(\d+)\s+FAKE-VIEW", line)
        if m:
            res["caller-callee FAKE-VIEW"] = int(m.group(1))
    # FAKE-VIEW == 0 vanishes from the breakdown; if the tool ran (UNRECONCILED seen)
    # but printed no FAKE-VIEW line, that IS the achieved 0 -> keep the row tracked.
    if "caller-callee unreconciled" in res and "caller-callee FAKE-VIEW" not in res:
        res["caller-callee FAKE-VIEW"] = 0
    return res


def semantic_count() -> list[tuple[str, int]]:
    """Build/AST-derived metrics. This is intentionally full-tier work."""
    from gruntz.audit.nested_static_casts import scan as nested_static_casts

    values = {"nested static_casts": len(nested_static_casts())}
    values.update(_caller_callee_counts())
    return [(label, values[label]) for label in _SEMANTIC_LABELS if label in values]


# Ratchet set: metrics that only go DOWN. The view/vtable metrics + the caller_callee
# fake-view edge + cast counts + .cpp external declarations (owner headers only).
_RATCHET = _VIEW_METRICS | _SEMANTIC_LABEL_SET | {
    # Numeric-domain debt: every one of these is a name nobody wrote down yet, and
    # naming it cannot move a byte. Ratcheted at the standing count so the backlog
    # drains and no new magic number arrives.
    "magic case labels",
    "unnamed domain compares",
    ".cpp-local enums",
    "C-style casts",
    "reinterpret_casts",
    "unexplained casts",
    "forced COMDAT emitters",
    "cpp extern decls",
    "cpp external prototypes",
    # The header half of the same construct. `cpp extern decls` sat at 0 while 45
    # globals were declared in 2+ headers; ratcheted so it cannot regrow quietly.
    "duplicate header externs",
    # A newly reconstructed body must NOT arrive with `i32 a3`-style parameters. The meaning
    # is available at the moment of reconstruction - from the call sites and the body - and it
    # is far cheaper to name then than to re-derive later. Ratcheted at the standing count, so
    # existing debt is drained rather than forbidden.
    "positional arg placeholders",
    # Same argument for the field side. `m_<hex>` naming is deliberately LAST in the campaign
    # order (CLAUDE.md), which is why this sat un-ratcheted - but "last" meant it could rise
    # silently and the baseline would bless it: one merge of 14 new bodies took it 7483 -> 7581
    # with no gate saying anything. Ratcheted at the standing count for the same reason as the
    # arg metric: existing debt is drained on its own schedule, but NEW placeholder fields do
    # not get to arrive unnoticed.
    "m_<hex> fields",
}


# Metrics whose evidence lives in COMMENTS, so they must see the raw file. `_strip`
# removes every comment before a metric runs, which silently broke "unexplained casts":
# its REASON regex only ever matches reason text written in a comment, so after
# stripping it could never fire and the metric scored EVERY non-structurally-forced
# cast as unexplained. It read 410 while the cast ledger - same rule, unstripped
# source - read 122 OPEN; 577 total minus 167 structurally-forced is exactly 410, so
# the metric was giving zero credit for every reason ever written.
_NEEDS_RAW = {"unexplained casts"}


def count(*, include_semantic: bool = False) -> list[tuple[str, int]]:
    """Measure fast text metrics and, when requested, build-derived semantic metrics."""
    totals = {label: 0 for label, _, _ in METRICS}
    for root in ROOTS:
        base = REPO / root
        if not base.is_dir():
            continue
        for path in base.rglob("*"):
            if path.suffix not in EXTS or not path.is_file():
                continue
            is_cpp = path.suffix in _CPP
            scaffold = _is_scaffolding(path)   # stub/Views: excluded from the VIEW metrics
            try:
                raw = path.read_text(errors="ignore")
                code = _strip(raw)
            except OSError:
                continue
            for label, matcher, cpp_only in METRICS:
                if cpp_only and not is_cpp:
                    continue
                if scaffold and label in _VIEW_METRICS:
                    continue
                text = raw if label in _NEEDS_RAW else code
                totals[label] += matcher(text) if callable(matcher) else len(matcher.findall(text))
    rows = [(label, totals[label]) for label, _, _ in METRICS]
    # The *Views.h holding pens are EXCLUDED from the ratcheted view metrics above
    # (drain-campaign machinery), which made their resident view classes invisible
    # once the drain ended - a false green (user call-out 2026-07-19). Count them
    # as their own tracked (non-ratcheted) debt row: every class/struct DEFINITION
    # in a *Views.h is a view by construction; drive to 0 by identity folds.
    vh = 0
    for root in ROOTS:
        base = REPO / root
        if not base.is_dir():
            continue
        for path in base.rglob("*Views.h"):
            try:
                vh += len(_TYPEDEF_DEF.findall(_strip(path.read_text(errors="ignore"))))
            except OSError:
                pass
    rows.append(("view classes (*Views.h)", vh))
    # Cross-FILE, so it cannot be a per-file regex like the rows above: the same
    # symbol `extern`-declared in two headers (or twice in one) is the `cpp extern
    # decls` construct after it migrated out of the .cpp files that metric scans.
    # Counted as REDUNDANT declarations (occurrences - 1 per symbol).
    rows.append(("duplicate header externs",
                 sum(len(v) - 1 for v in _duplicate_header_externs().values())))
    if include_semantic:
        rows.extend(semantic_count())
    return rows


def _load_baseline_file(path: Path) -> dict[str, int]:
    out: dict[str, int] = {}
    if not path.is_file():
        return out
    for line in path.read_text().splitlines():
        if "\t" in line:
            lbl, n = line.rsplit("\t", 1)
            try:
                out[lbl] = int(n)
            except ValueError:
                pass
    return out


def load_baseline() -> dict[str, int]:
    """Load the text and semantic floors as one scoreboard namespace."""
    text = _load_baseline_file(TEXT_BASELINE)
    semantic = _load_baseline_file(SEMANTIC_BASELINE)
    duplicate = set(text) & set(semantic)
    if duplicate:
        raise ValueError("cleanliness metric occurs in both baselines: "
                         + ", ".join(sorted(duplicate)))
    return {**text, **semantic}


def save_baseline(rows: list[tuple[str, int]], *, include_semantic: bool = True) -> None:
    """Route floors by mechanism; normal builds leave the semantic file untouched."""
    text_rows = [(label, n) for label, n in rows if label not in _SEMANTIC_LABEL_SET]
    semantic_rows = [(label, n) for label, n in rows if label in _SEMANTIC_LABEL_SET]
    if text_rows or not TEXT_BASELINE.exists():
        TEXT_BASELINE.write_text("".join(f"{lbl}\t{n}\n" for lbl, n in text_rows))
    if include_semantic and (semantic_rows or not SEMANTIC_BASELINE.exists()):
        SEMANTIC_BASELINE.write_text(
            "".join(f"{lbl}\t{n}\n" for lbl, n in semantic_rows))


def merge_baseline_downonly(rows: list[tuple[str, int]]) -> list[tuple[str, int]]:
    """Rows to persist during a build's baseline roll: the RATCHETED metrics
    (_VIEW_METRICS - views + the fake-view-propagating casts) never RISE. Keep
    min(new count, committed floor) so a regression stays visible as debt on every
    later build instead of being silently blessed into the floor; other (tracked,
    non-ratcheted) metrics roll forward as before. Manual ``--update`` still writes
    the true counts (the one deliberate way to bless a floor - only ever lower).

    A label MISSING from `rows` keeps its committed floor instead of being dropped.
    An UNMEASURED metric is not a measurement of zero, and dropping its row deletes
    the floor: `count()` only appends the caller-callee rows when
    ``_caller_callee_counts()`` returns them, and that helper swallows EVERY exception
    (it shells out with a 600s timeout and returns {} on any failure). Since
    ``gruntz build --full`` measures both families; one flaky/timed-out
    caller_callee run used to rewrite the baseline
    without ``caller-callee FAKE-VIEW`` - erasing its 0 floor, after which any
    regression reads as a brand-new metric with no delta and no ratchet. Carry the
    floor forward; a metric we could not measure is reported as unmeasured, never
    blessed away.
    """
    base = load_baseline()
    out = []
    for label, n in rows:
        if label in _RATCHET and label in base:
            out.append((label, min(n, base[label])))
        else:
            out.append((label, n))
    measured = {label for label, _ in rows}
    for label, n in base.items():
        if label not in measured:
            out.append((label, n))
    return out


def report_lines(rows: list[tuple[str, int]] | None = None) -> list[str]:
    """Scoreboard for the build report: only the NON-ZERO metrics (with delta vs
    baseline, down = good). A metric already at 0 is clean, so it is omitted - just
    the count of zeroed metrics is noted - and the report shows what still needs
    work instead of a wall of zeros. A regression makes a metric non-zero (or raises
    it), so it reappears here; the hard ratchet gate in cli.py still fires on it."""
    rows = rows if rows is not None else count()
    base = load_baseline()
    nz = [(lbl, n) for lbl, n in rows if n]
    zeros = len(rows) - len(nz)
    if not nz:
        return [f"cleanliness: all {len(rows)} metrics at 0 (clean)."]
    cells = []
    for lbl, n in nz:
        d = n - base[lbl] if lbl in base else 0
        cells.append(f"{lbl} {n}" + (f" ({d:+d})" if d else ""))
    lines = [f"cleanliness (non-zero of {len(rows)}; {zeros} at 0; delta vs baseline, down = good):"]
    row = ""
    for c in cells:
        if row and len(row) + len(c) + 4 > 92:
            lines.append("    " + row.rstrip())
            row = ""
        row += c + "    "
    if row:
        lines.append("    " + row.rstrip())
    return lines


def main() -> int:
    include_semantic = "--semantic" in sys.argv
    if "--dup-externs" in sys.argv:
        dups = _duplicate_header_externs()
        for name in sorted(dups, key=lambda k: (-len(dups[k]), k)):
            print(f"{name}  ({len(dups[name])} decls)")
            for path in dups[name]:
                print(f"    {path}")
        print(f"# {len(dups)} symbol(s), "
              f"{sum(len(v) - 1 for v in dups.values())} redundant declaration(s)")
        return 0
    rows = count(include_semantic=include_semantic)
    if "--update" in sys.argv:
        save_baseline(rows, include_semantic=include_semantic)
        scope = "text + semantic" if include_semantic else "text"
        print(f"cleanliness {scope} baseline updated: {sum(n for _, n in rows)} "
              f"total across {len(rows)} measured metrics")
        return 0
    if "--csv" in sys.argv:
        base = load_baseline()
        for label, n in rows:
            print(f"{label},{n},{n - base.get(label, n)}")
        return 0
    for line in report_lines(rows):
        print(line)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
