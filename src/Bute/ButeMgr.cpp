// ButeMgr.cpp - CButeMgr, the engine's `.att`/`.bute` attribute config-parser.
// The attribute layer the whole game reads entity stats through: a two-level
// keyed store (tag-group -> key -> typed value) built by a recursive-descent
// lexer/parser and queried by a family of typed getters.
//
// Functions matched in this TU:
//   CButeMgr::ScanToken    - token-expect
//   CButeMgr::GetIntDef    - type 0, default
//   CButeMgr::GetInt       - type 0
//   CButeMgr::GetDwordDef  - type 1, default
//   CButeMgr::GetDword     - type 1
//   CButeMgr::GetFloat     - type 3 (or int)
//   CButeMgr::GetDouble    - type 2 (or int)
//   CButeMgr::GetStringDef - type 4, default
//   CButeMgr::GetString    - type 4
//   CButeMgr::GetRef5..8   - typed-reference getters (value type-tags 5..8;
//                            return a pointer to the stored struct, or a shared
//                            zero-default static on any miss)
//   CButeMgr::ParseTagLine - one tag=value line
//   CButeMgr::Parse        - the .att parser
//
// Lexer cluster (migrated from src/Stub/Discovered.cpp, the trace-discovered
// CButeMgr set 0x170330-0x171a60):
//   CButeMgr::Init           - reset counters + the +0x100/+0x104 scratch strings
//   CButeMgr::SetErrCallback - store the +0x14 error callback
//   CButeMgr::NextChar       - advance the input one char (the lexer's getch)
//   CButeMgr::CharClass      - char -> lexer character-class index
//   CButeMgr::PeekState/2    - read a transition-table column (state x class)
//   CButeMgr::ScanState      - write m_tokType (+0xaa) + m_lexState (+0xac)
//   CButeMgr::SkipToTag      - re-lex until a tag/group token
//   CButeMgr::ParseGroup     - the recursive per-tag descent
//   CButeMgr::Exists         - tag/key existence probe
//   CButeMgr::~CButeMgr    - the EH-frame (/GX) scalar destructor (0x0213c0);
//                            tears down the 3 CButeStore sub-trees + 5 CStrings +
//                            the +0x10f tail object (@early-stop on the EH-region
//                            granularity wall; see its definition below).
//
// The getters funnel through one __thiscall find-by-key helper (CButeTree::Find):
// outer Find(tag) on m_tree (+0x18) yields the tag sub-tree;
// inner Find(key) yields the typed value record { int type; void* pValue; }. Each
// getter checks the record's type then reads the value through pValue. On any miss
// (no tag / no key / type mismatch) it calls the variadic error reporter
// and returns the default (or 0 / 0x80000000 / an empty CString).
//
// The error-string format literals are reloc-masked file-scope constants. The CRT
// tokenizer helpers + the tree Find/Insert + the node ctor are external/no-body
// engine calls (reloc-masked). ParseTagLine constructs a store node + carries a
// C++ EH frame (the CString copy + the node ctor under unwind) -> /GX.
#include <Bute/ButeMgr.h>
#include <rva.h>

// Global operator new (engine NAFXCW); external/no-body so the
// `push 0x2c; call ??2; add esp,4` shape falls out reloc-masked.
void* operator new(u32 n);

// The node-ctor descriptor. Reloc-masked file-scope address.
extern i32 g_nodeDescriptor;

// ---------------------------------------------------------------------------
// Lexer static tables (read-only engine data, reloc-masked DATA externs).
//   g_charClass[256]  - per-char lexer character-class word (the getter returns
//                       this - 1). One WORD per char.
//   g_transTable      - the 2D state x class transition table (row stride 147
//                       WORDs); three adjacent columns are read off the same
//                       base (g_transTable[i], [i+1], [i+2] -> base +0/+2/+4).
// ---------------------------------------------------------------------------
DATA(0x0021cf40)
extern "C" i16 g_charClass[]; // 0x61cf40
DATA(0x0021d140)
extern "C" i16 g_transTable[]; // 0x61d140  (state x class table, row stride 147)
// The token-type column is read off the table's second WORD (0x61d142): retail
// folds the +1 into the data symbol's address, so it is its own DATA extern.
DATA(0x0021d142)
extern "C" i16 g_transTable142[][147]; // 0x61d142

// The shared empty C string (0x6293f4); Init assigns it into the two scratch
// CStrings.
extern "C" char g_emptyString[]; // 0x6293f4

// The input source stream embedded at CButeMgr+0xa0. ReadByte pulls the next
// byte position (an engine __thiscall, reloc-masked external/no-body).
class CButeStream {
public:
    i32 ReadByte();
};

// The big attribute-file line driver at 0x170750 (the same class as CButeMgr;
// its retail symbol is ?ParseAttributeFile@ButeMgr@@QAE_NXZ, stubbed in
// src/Stub/Backlog.cpp). The lexer cluster reaches it through this `ButeMgr`
// view so the call pairs by name; `this` (a CButeMgr*) is the receiver.
class ButeMgr {
public:
    bool ParseAttributeFile();
};

// ParseGroup's recursive node-walk callback (the engine apply-fn passed to the
// tree walker at 0x193340). Reloc-masked file-scope address.
extern "C" void ButeGroup_Apply();

// The token-value scanners ParseAttributeFile feeds the lexed token buffer
// through (all __cdecl CRT-style, reloc-masked external/no-body):
//   ReadInt(tok)            atoi-like, the type-0 int value
//   ReadDword(tok, end, b)  strtoul-like (base b), the type-1 dword value
//   ReadFloat(tok)          atof-like, returns the value in st(0)
// plus the CRT variadic sscanf the point/rect cases drive.
extern "C" i32 ButeRead_Int(char* tok);                           // 0x11ffb0
extern "C" DWORD ButeRead_Dword(char* tok, char** end, i32 base); // 0x1240b0
extern "C" double ButeRead_Float(char* tok);                      // 0x18d220
extern "C" i32 sscanf(const char* buf, const char* fmt, ...);     // 0x120900

// The stored value record ParseAttributeFile builds + inserts: a tagged head
// `{ i32 type; void* pValue }` (CButeValue), pValue pointing at a heap copy of
// the parsed value. For the int/dword/float cases pValue is `new`-d 4-byte
// storage; double is 8-byte; the point/rect refs are 8/16/24-byte blocks. The
// node is allocated via the global operator new, so the `push N; call ??2;
// add esp,4; test eax,eax` shape reloc-masks. Modeled with the field offsets
// the stores hit (type @+0, pValue @+4).
struct CButeValueNode {
    i32 type;     // +0x00
    void* pValue; // +0x04
};

// CString::operator+= one char (__thiscall(receiver, char)):
// appends the char to the value-text accumulator. Modeled as an external
// (no-body) __thiscall on a tiny receiver class so the `mov ecx,recv; call`
// shape falls out reloc-masked.
//
// The other overloads ParseAttributeFile's write-back path drives off the same
// accumulator (each __thiscall, reloc-masked external/no-body): the value-text
// reconstruction `accum += "<" += d0 += ", " += d1 += ">"` chains them. The
// `operator+=(const char*)` returns CButeText& so the chains fold to the
// `call; mov ecx,eax; call` shape; the typed appends consume the chain result.
class CButeText {
public:
    void AppendChar(char c);
    // 0x16be60  CString::operator+=(const char*) -> CString&
    CButeText& operator+=(const char* s);
    // Each formatted-append returns &accum so the write-back value reconstruction
    // chains `accum += "(" .AppendElem(a) += ", " .AppendElem(b) ...`.
    // 0x191d20  formatted-append signed int
    CButeText& AppendInt(i32 v);
    // 0x1921e0  formatted-append DWORD
    CButeText& AppendDword(DWORD v);
    // 0x191df0  formatted-append double
    CButeText& AppendDouble(double v);
    // 0x192120  formatted-append signed int (the point/rect element appender)
    CButeText& AppendElem(i32 v);
    // 0x192060  truncate/release helper (takes a char count: the quote-strip in
    // the string write-back path passes 0x22 = '"').
    void Trim(i32 n);
};

// The object m_pText points at: the value-text accumulator (a CButeText / MFC
// CString) lives at +0xc inside it. Parse reaches it as m_pText->accum so the
// `mov ecx,[esi+0xa4]; add ecx,0xc; call` shape falls out (no raw-offset cast).
struct CButeTextBuf {
    char m_pad00[0xc]; // +0x00
    CButeText accum;   // +0x0c  the appended value text
};

// The token-length counter (file-scope signed WORD, read with movsx).
static i16 g_tokenLen;

// Error-reporter format strings (reloc-masked file-scope literals).
static const char s_fmtFormatError[] = "ButeMgr (%d): A formatting error";
static const char s_fmtBadSymbol[] = "ButeMgr (%d): Bad symbol encountered";
static const char s_fmtDupTag[] = "ButeMgr: duplicate tag encountered";
static const char s_fmtTypeMismatch[] = "ButeMgr: Type mismatch - [%s]:%s";
static const char s_fmtInvalidTag[] = "ButeMgr: Invalid tag specified - ";
static const char s_fmtNotFound[] = "ButeMgr: Symbol not found - [%s]";

// Float/double zero-on-error constants (reloc-masked file-scope).
static const float s_floatZero = 0.0f;
static const double s_doubleZero = 0.0;

// ParseAttributeFile write-back decorations + sscanf format strings (reloc-masked
// file-scope literals, matched against their delinked $SG addresses).
static const char s_fmtInvalidToken[] = "ButeMgr (%d):  Invalid token encountered.";
static const char s_strDword[] = "(DWORD)";
static const char s_strFloat[] = "(FLOAT)";
static const char s_strFloatSuffix[] = "f";
static const char s_fmtPoint4[] = "(%d, %d, %d, %d)";
static const char s_strOpen[] = "(";
static const char s_strClose[] = ")";
static const char s_strComma[] = ", ";
static const char s_fmtPoint2[] = "(%d, %d)";
static const char s_fmtRect3[] = "<%lf, %lf, %lf>";
static const char s_strLt[] = "<";
static const char s_strGt[] = ">";
static const char s_fmtRect2[] = "[%lf, %lf]";
static const char s_strLBrack[] = "[";
static const char s_strRBrack[] = "]";

// CRT varargs formatter (engine NAFXCW vsprintf, reloc-masked external/no-body).
extern "C" i32 vsprintf(char* buf, const char* fmt, char* va);

// ---------------------------------------------------------------------------
// CButeMgr::~CButeMgr
// The /GX (EH-frame) scalar destructor: tears down the three owned CButeStore
// sub-trees (+0x18 / +0x48 / +0x74), the five CStrings (m_errStr @+0x10, m_tagName
// @+0x100, m_str104 @+0x104, m_str108 @+0x108) and the +0x10f tail object, each at
// its own descending trylevel in reverse declaration order. The body is empty:
// every store/string/tail member is a value member with a non-trivial dtor, so the
// compiler emits the full /GX teardown chain (the `push -1 / mov fs:0,esp` frame,
// the `[esp+N]=state` trylevel writes, and the per-member `lea ecx,[this+off]; call
// ~Member` invocations) automatically. The three CButeStore members each expand to
// the inline multiply-derived teardown (two vptr re-stamps + the recursive clear +
// the masked second-base restore + the primary-base dtor).
// @early-stop
// 68.6% inline-member EH-region-granularity wall (docs/patterns/
// eh-dtor-inline-member-vtable-stamp-thisadjust.md). The teardown is byte-correct:
// the /GX frame, the reverse-declaration member order (10f,108,104,100,74,48,18,10),
// each CString dtor with its trylevel, AND each CButeStore's inline multiply-derived
// teardown -- the two vptr re-stamps + `mov ecx,edi; call ClearRecursive` + the
// masked `mov ecx,edi; neg ecx; sbb ecx,ecx; and ecx,ebx; call RestoreVptr`
// second-base adjust + `mov ecx,edi; call BaseDtor` -- all match retail instruction
// for instruction. The residual is the EH-state MACHINE only: retail destructs each
// store member as a NESTED region using TWO state slots ([esp+0x2c] outer state
// 8/a/c, [esp+0x28] inner 7/9/b + 2/1/0) and stores the member-`this` cleanup
// pointer (`mov [esp+0x1c],edi`); the inline form collapses each store to a single
// trylevel (2/1/0) with no member-`this` re-point. That coupling also makes retail
// reserve a 0x10-larger frame (`sub esp,0x14`/`add esp,0x20` vs our `push ecx`/
// `add esp,0x10`), shifting every `[esp+N]` operand by 0x10. Out-of-lining
// ~CButeStore would emit the nested states but then ~CButeMgr would CALL it (retail
// inlines), diverging worse. No source lever produces the nested two-slot EH region
// from an inlined manual-vtable member dtor; deferred to the final sweep.
RVA(0x000213c0, 0x14c)
CButeMgr::~CButeMgr() {}

// ---------------------------------------------------------------------------
// CButeMgr::ReportError
// The variadic error reporter the getters/parser funnel every failure through.
// Formats `fmt` + the varargs into the m_errStr scratch buffer (CString
// GetBuffer/vsprintf/ReleaseBuffer), then - if an error callback is registered -
// fires it with the formatted message. A variadic member compiles __cdecl with
// `this` as the hidden first stack arg (the retail ABI: callers push `this`
// last).
RVA(0x001706c0, 0x4b)
void CButeMgr::ReportError(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vsprintf(m_errStr.GetBuffer(0x100), fmt, args);
    m_errStr.ReleaseBuffer(-1);

    if (m_errCallback != 0) {
        m_errCallback(m_errStr.GetBuffer(0));
    }

    va_end(args);
}

// ---------------------------------------------------------------------------
// CButeMgr::ScanToken
// Parse() (re)lexes one token, then ScanToken verifies it is the expected type
// (m_tokType == expectType). On mismatch it reports a formatting error (with the
// current line m_lineNo) and returns false; else true.
RVA(0x00170710, 0x3b)
bool CButeMgr::ScanToken(i32 expectType) {
    if (!Parse()) {
        return false;
    }

    if (m_tokType != expectType) {
        ReportError(s_fmtFormatError, m_lineNo);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetIntDef
// type-0 (int) getter with a caller default: Find(tag).Find(key); on a type-0
// hit return *(int*)rec->pValue, on type mismatch report + fall through, on any
// miss return def.
RVA(0x00171aa0, 0x50)
i32 CButeMgr::GetIntDef(char* tag, char* key, i32 def) {
    void* grp = Tree()->Find(tag);
    if (grp) {
        CButeValue* rec = (CButeValue*)((CButeTree*)grp)->Find(key);
        if (rec) {
            if (rec->type == kButeInt) {
                return *(i32*)rec->pValue;
            }
            ReportError(s_fmtTypeMismatch, tag, key);
        }
    }
    return def;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetInt
// type-0 getter, no default: returns 0x80000000 on any miss (and reports the
// specific failure - type mismatch / symbol-not-found / invalid-tag).
RVA(0x00171af0, 0x86)
i32 CButeMgr::GetInt(char* tag, char* key) {
    void* grp = Tree()->Find(tag);
    if (grp) {
        CButeValue* rec = (CButeValue*)((CButeTree*)grp)->Find(key);
        if (rec) {
            if (rec->type == kButeInt) {
                return *(i32*)rec->pValue;
            }
            ReportError(s_fmtTypeMismatch, tag, key);
            return (i32)0x80000000;
        }
        ReportError(s_fmtNotFound, tag, key);
        return (i32)0x80000000;
    }
    ReportError(s_fmtInvalidTag, tag);
    return (i32)0x80000000;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetDwordDef
// type-1 (dword) getter with default. The type check is `if (--type == 0)` i.e.
// type == 1 (the disasm `mov ecx,[eax]; dec ecx; je`).
RVA(0x001721e0, 0x5a)
DWORD CButeMgr::GetDwordDef(char* tag, char* key, DWORD def) {
    void* grp = Tree()->Find(tag);
    if (grp) {
        CButeValue* rec = (CButeValue*)((CButeTree*)grp)->Find(key);
        if (rec) {
            switch (rec->type) {
                case kButeDword:
                    return *(DWORD*)rec->pValue;
            }
            ReportError(s_fmtTypeMismatch, tag, key);
        }
    }
    return def;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetDword
// type-1 getter, no default: returns 0 on any miss.
RVA(0x00172240, 0x7d)
DWORD CButeMgr::GetDword(char* tag, char* key) {
    void* grp = Tree()->Find(tag);
    if (grp) {
        CButeValue* rec = (CButeValue*)((CButeTree*)grp)->Find(key);
        if (rec) {
            switch (rec->type) {
                case kButeDword:
                    return *(DWORD*)rec->pValue;
            }
            ReportError(s_fmtTypeMismatch, tag, key);
            return 0;
        }
        ReportError(s_fmtNotFound, tag, key);
        return 0;
    }
    ReportError(s_fmtInvalidTag, tag);
    return 0;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetFloat
// type-3 (float) getter, no default. Accepts type 0 (int) too: `fild` the int
// when type==0, `fld` the float when type==3, else report + return 0.0f. Returns
// in st(0) (an x87 float return).
RVA(0x00172730, 0x9a)
float CButeMgr::GetFloat(char* tag, char* key) {
    void* grp = Tree()->Find(tag);
    if (grp) {
        CButeValue* rec = (CButeValue*)((CButeTree*)grp)->Find(key);
        if (rec) {
            switch (rec->type) {
                case kButeInt:
                    return (float)*(i32*)rec->pValue;
                case kButeFloat:
                    return *(float*)rec->pValue;
            }
            ReportError(s_fmtTypeMismatch, tag, key);
            return s_floatZero;
        }
        ReportError(s_fmtNotFound, tag, key);
        return s_floatZero;
    }
    ReportError(s_fmtInvalidTag, tag);
    return s_floatZero;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetDouble
// type-2 (double) getter, no default. Accepts type 0 (int): `fild` when type==0,
// `fld qword` when type==2, else report + return 0.0. (The disasm tests with
// `sub ecx,0; je` then `sub ecx,2; je` -> the type==0 and type==2 branches.)
RVA(0x00172c40, 0x9b)
double CButeMgr::GetDouble(char* tag, char* key) {
    void* grp = Tree()->Find(tag);
    if (grp) {
        CButeValue* rec = (CButeValue*)((CButeTree*)grp)->Find(key);
        if (rec) {
            switch (rec->type) {
                case kButeInt:
                    return (double)*(i32*)rec->pValue;
                case kButeDouble:
                    return *(double*)rec->pValue;
            }
            ReportError(s_fmtTypeMismatch, tag, key);
            return s_doubleZero;
        }
        ReportError(s_fmtNotFound, tag, key);
        return s_doubleZero;
    }
    ReportError(s_fmtInvalidTag, tag);
    return s_doubleZero;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetStringDef
// type-4 (string) getter with default: returns rec->pValue (the char*) on a
// type-4 hit, reports a type mismatch otherwise, returns def on any miss.
RVA(0x00173180, 0x4e)
CString* CButeMgr::GetStringDef(char* tag, char* key, CString* def) {
    void* grp = Tree()->Find(tag);
    if (grp) {
        CButeValue* rec = (CButeValue*)((CButeTree*)grp)->Find(key);
        if (rec) {
            if (rec->type == kButeString) {
                return (CString*)rec->pValue;
            }
            ReportError(s_fmtTypeMismatch, tag, key);
        }
    }
    return def;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetString
// type-4 getter, no default: returns rec->pValue on a hit, else reports the
// specific failure and returns a shared empty CString. The empty string is a
// function-local static CString (MFC magic-static: one-shot guarded ctor +
// atexit-registered dtor) returned by address on every error path.
RVA(0x001731d0, 0xb6)
char* CButeMgr::GetString(char* tag, char* key) {
    static CString s_empty("");

    void* grp = Tree()->Find(tag);
    if (grp) {
        CButeValue* rec = (CButeValue*)((CButeTree*)grp)->Find(key);
        if (rec) {
            if (rec->type == kButeString) {
                return (char*)rec->pValue;
            }
            ReportError(s_fmtTypeMismatch, key, tag);
            return (char*)&s_empty;
        }
        ReportError(s_fmtNotFound, key, tag);
        return (char*)&s_empty;
    }
    ReportError(s_fmtInvalidTag, tag);
    return (char*)&s_empty;
}

// ---------------------------------------------------------------------------
// CButeMgr::ParseTagLine
// Reads one "tag = value" line: ScanToken(4) for the tag name, copies it into the
// active-tag CString (m_tagName), and -- unless m_10d suppresses it -- checks for a
// duplicate tag in the store (reporting + bailing if found), else allocates a new
// store node, constructs it, wires its two vtables, records it (m_pNode) and
// inserts it under the tag name; finally ScanToken(3) consumes the value. The new
// node lives under a C++ EH frame (freed on unwind if the ctor throws) -> /GX.
//
// The final return explicitly normalizes the ScanToken result (`? true : false`):
// retail emits `test al,al; setne al` after the SEH-frame teardown. Returning the
// bool directly (`return ScanToken(3)`) elides that normalization; the explicit
// canonicalization reproduces the retail tail byte-for-byte (the SEH cleanup spans
// the value, so MSVC re-canonicalizes the bool after restoring fs:[0]).
RVA(0x001711b0, 0xf5)
bool CButeMgr::ParseTagLine() {
    if (!ScanToken(4)) {
        return false;
    }

    char* tok = m_token;
    m_tagName = tok;

    if (!m_10d) {
        CButeTree* t = Tree();
        if (t->Find(tok)) {
            ReportError(s_fmtDupTag, tok);
            return false;
        }
        CButeNode* node = new CButeNode(&g_nodeDescriptor, 2);
        m_pNode = node;
        t->Insert(tok, node);
    }

    return ScanToken(3) ? true : false;
}

// ---------------------------------------------------------------------------
// CButeMgr::Parse
// The recursive-descent token lexer: resets the token-length counter, then loops
// classifying the current character (ButeLex_PeekClass) -- skipping any class > 5
// -- and dispatching a 6-way jump table on the token class to read identifiers/
// values/punctuation (ButeLex_ReadValue/ReadIdent), append chars to the token
// buffer, advance the lexer (ButeLex_NextChar), and recurse for nested groups.
// Reports "Bad symbol encountered" (with m_lineNo) on the error class.
RVA(0x001704c0, 0x1e3)
bool CButeMgr::Parse() {
    const i32 kLexStartState = 0x11; // initial lexer state seeded into PeekClass
    i32 kind = kLexStartState;
    g_tokenLen = 0;

    for (;;) {
        i16 cls = PeekClass(kind, m_curChar);
        switch (cls) {
            case 0: // bad symbol
                ReportError(s_fmtBadSymbol, m_lineNo);
                return false;

            case 1: // value char: scan, store to token buffer, echo, advance, loop
                kind = ReadValue(kind, m_curChar);
                m_token[g_tokenLen++] = m_curChar;
                if (m_captureText != 0 && m_curChar != 0) {
                    m_pText->accum.AppendChar(m_curChar);
                }
                NextChar();
                break;

            case 2: // value char: scan, echo only, advance, loop
                kind = ReadValue(kind, m_curChar);
                if (m_captureText != 0 && m_curChar != 0) {
                    m_pText->accum.AppendChar(m_curChar);
                }
                NextChar();
                break;

            case 3: // identifier: scan, store, echo, advance, recurse, terminate
                ReadIdent(kind, m_curChar);
                m_token[g_tokenLen++] = m_curChar;
                if (m_captureText != 0 && m_curChar != 0) {
                    m_pText->accum.AppendChar(m_curChar);
                }
                NextChar();
                if (m_tokType == 0) {
                    Parse();
                }
                m_token[g_tokenLen] = 0;
                return true;

            case 4: // identifier: scan, echo only, advance, recurse, terminate
                ReadIdent(kind, m_curChar);
                if (m_captureText != 0 && m_curChar != 0) {
                    m_pText->accum.AppendChar(m_curChar);
                }
                NextChar();
                if (m_tokType == 0) {
                    Parse();
                }
                m_token[g_tokenLen] = 0;
                return true;

            case 5: // identifier: scan, recurse, terminate (no echo, no advance)
                ReadIdent(kind, m_curChar);
                if (m_tokType == 0) {
                    Parse();
                }
                m_token[g_tokenLen] = 0;
                return true;
        }
    }
}

// ---------------------------------------------------------------------------
// ButeMgr::ParseAttributeFile
// The "key = value" value-line driver (?ParseAttributeFile@ButeMgr@@QAE_NXZ).
// Reads the key name (already lexed into m_token) into the scratch CString
// m_str104, optionally probes the active store node (m_pNode) for a duplicate
// key (reporting + flagging it), lexes the value-type token, then dispatches an
// 11-way switch over the value-type token (m_tokType in 5..15) to either:
//   - STORE mode (m_10d == 0, no duplicate): allocate a CButeValueNode, tag it,
//     `new` a heap copy of the parsed value, and Insert it under the key; or
//   - WRITE-BACK mode (m_10d != 0): read the existing typed value back through
//     the matching getter and append a formatted representation of it to the
//     value-text accumulator (m_pText->accum), using the per-type literal
//     decorations ("(DWORD)", "(FLOAT)", "(a, b)", "<x, y, z>", "[x, y]", ...).
// The token scanners + the heap value copies live under a C++ EH frame (each
// inline value object + the CString string temp are destructible on unwind) ->
// /GX. `this` is a CButeMgr (the ButeMgr view only fixes the retail mangling).
// @early-stop
// 40.9% this-register-pin + EH-frame-layout regalloc wall. Logic/CFG are complete
// and correct: all 11 value-type cases (int/dword/float/double/string + the
// point2/4 + rect2/3 refs), both STORE (new+Insert) and WRITE-BACK (getter +
// formatted accum append) modes, with byte-correct case bodies (verified
// per-case against the disasm). The residual is two coupled allocator coin-flips
// that cascade through every `this`-relative operand: (1) retail pins `this` in
// EBP (`mov ebp,ecx`), the recompile picks ESI -- so every `[this+N]` access
// encodes a different mod/rm byte fn-wide; (2) retail reserves a 0x54 frame
// (double-staging the rect/point value copies through `[esp+0x34..0x60]` +
// `rep movs`), the recompile reserves 0x28 (field-store, no staging buffer).
// Modeling the refs as named value-structs copied to the heap (which reproduces
// the staging + rep-movs) regressed the whole fn 41%->36% by perturbing the /GX
// EH-state of the destructible locals -- a net-negative lever. The case bodies
// otherwise match; deferred to the final sweep (see docs/patterns/
// stack-buffer-size-drives-frame.md + o2-optimizer-bailout-framed.md).
RVA(0x00170750, 0x9d8)
bool ButeMgr::ParseAttributeFile() {
    CButeMgr* self = (CButeMgr*)this;
    CButeText* accum = &self->m_pText->accum;

    // The shared 4-byte scalar value slot ([esp+0x10] in retail), zero-init at
    // entry (`mov [esp+0x10],0`) and reused by the int/dword/float store paths.
    i32 v = 0;

    self->m_str104 = self->m_token;

    bool bDup = false;
    if (!self->m_10d) {
        if (((CButeTree*)self->m_pNode)->Find((const char*)self->m_str104)) {
            self->ReportError(s_fmtDupTag, self->m_str104.GetBuffer(0));
            bDup = true;
        }
    }

    if (!self->ScanToken(5)) {
        return false;
    }
    if (self->m_10d) {
        accum->Trim(0x20);
        self->m_captureText = 0;
    }
    if (!self->Parse()) {
        return false;
    }

    switch (self->m_tokType) {
        case 5:
        case 6: { // signed int -> type 0
            v = ButeRead_Int(self->m_token);
            if (self->m_10d) {
                accum->AppendInt(self->GetInt(
                    (char*)(const char*)self->m_tagName,
                    (char*)(const char*)self->m_str104
                ));
            } else if (!bDup) {
                CButeValueNode* n = (CButeValueNode*)operator new(8);
                if (n) {
                    n->type = 0;
                    i32* p = (i32*)operator new(4);
                    if (p) {
                        *p = v;
                        n->pValue = p;
                    } else {
                        n->pValue = 0;
                    }
                }
                ((CButeTree*)self->m_pNode)->Insert((const char*)self->m_str104, n);
            }
            break;
        }
        case 13: { // dword -> type 1
            if (!self->ScanToken(6)) {
                return false;
            }
            *(DWORD*)&v = ButeRead_Dword(self->m_token, 0, 10);
            if (self->m_10d) {
                *accum += s_strDword;
                accum->AppendDword(self->GetDword(
                    (char*)(const char*)self->m_tagName,
                    (char*)(const char*)self->m_str104
                ));
            } else if (!bDup) {
                CButeValueNode* n = (CButeValueNode*)operator new(8);
                if (n) {
                    n->type = 1;
                    DWORD* p = (DWORD*)operator new(4);
                    if (p) {
                        *p = v;
                        n->pValue = p;
                    } else {
                        n->pValue = 0;
                    }
                }
                ((CButeTree*)self->m_pNode)->Insert((const char*)self->m_str104, n);
            }
            break;
        }
        case 14: { // float (FLOAT-tagged) -> type 3
            if (!self->ScanToken(8)) {
                return false;
            }
            *(float*)&v = (float)ButeRead_Float(self->m_token);
            if (self->m_10d) {
                (*accum += s_strFloat)
                    .AppendDouble(self->GetFloat(
                        (char*)(const char*)self->m_tagName,
                        (char*)(const char*)self->m_str104
                    ));
            } else if (!bDup) {
                CButeValueNode* n = (CButeValueNode*)operator new(8);
                if (n) {
                    n->type = 3;
                    i32* p = (i32*)operator new(4);
                    if (p) {
                        *p = v;
                        n->pValue = p;
                    } else {
                        n->pValue = 0;
                    }
                }
                ((CButeTree*)self->m_pNode)->Insert((const char*)self->m_str104, n);
            }
            break;
        }
        case 15: { // float ('f'-tagged) -> type 3
            *(float*)&v = (float)ButeRead_Float(self->m_token);
            if (self->m_10d) {
                (*accum).AppendDouble(self->GetFloat(
                    (char*)(const char*)self->m_tagName,
                    (char*)(const char*)self->m_str104
                ));
                *accum += s_strFloatSuffix;
            } else if (!bDup) {
                CButeValueNode* n = (CButeValueNode*)operator new(8);
                if (n) {
                    n->type = 3;
                    i32* p = (i32*)operator new(4);
                    if (p) {
                        *p = v;
                        n->pValue = p;
                    } else {
                        n->pValue = 0;
                    }
                }
                ((CButeTree*)self->m_pNode)->Insert((const char*)self->m_str104, n);
            }
            break;
        }
        case 7: { // double -> type 2
            double dv = ButeRead_Float(self->m_token);
            if (self->m_10d) {
                accum->AppendDouble(self->GetDouble(
                    (char*)(const char*)self->m_tagName,
                    (char*)(const char*)self->m_str104
                ));
            } else if (!bDup) {
                CButeValueNode* n = (CButeValueNode*)operator new(8);
                if (n) {
                    n->type = 2;
                    double* p = (double*)operator new(8);
                    if (p) {
                        *p = dv;
                        n->pValue = p;
                    } else {
                        n->pValue = 0;
                    }
                }
                ((CButeTree*)self->m_pNode)->Insert((const char*)self->m_str104, n);
            }
            break;
        }
        case 9: { // (a, b, c, d) point4 -> type 5
            i32 a, b, c, d;
            sscanf(self->m_token, s_fmtPoint4, &a, &b, &c, &d);
            if (self->m_10d) {
                CButeRef5* r = self->GetRef5(
                    (char*)(const char*)self->m_tagName,
                    (char*)(const char*)self->m_str104
                );
                (*accum += s_strOpen).AppendElem((i32)r->a);
                (*accum += s_strComma).AppendElem((i32)r->b);
                (*accum += s_strComma).AppendElem((i32)r->c);
                (*accum += s_strComma).AppendElem((i32)r->d);
                *accum += s_strClose;
            } else if (!bDup) {
                CButeValueNode* n = (CButeValueNode*)operator new(8);
                if (n) {
                    n->type = 5;
                    i32* p = (i32*)operator new(0x10);
                    if (p) {
                        p[0] = a;
                        p[1] = b;
                        p[2] = c;
                        p[3] = d;
                        n->pValue = p;
                    } else {
                        n->pValue = 0;
                    }
                }
                ((CButeTree*)self->m_pNode)->Insert((const char*)self->m_str104, n);
            }
            break;
        }
        case 10: { // (a, b) point -> type 6
            i32 a, b;
            sscanf(self->m_token, s_fmtPoint2, &a, &b);
            if (self->m_10d) {
                CButeRef6* r = self->GetRef6(
                    (char*)(const char*)self->m_tagName,
                    (char*)(const char*)self->m_str104
                );
                (*accum += s_strOpen).AppendElem((i32)r->a);
                (*accum += s_strComma).AppendElem((i32)r->b);
                *accum += s_strClose;
            } else if (!bDup) {
                CButeValueNode* n = (CButeValueNode*)operator new(8);
                if (n) {
                    n->type = 6;
                    i32* p = (i32*)operator new(8);
                    if (p) {
                        p[0] = a;
                        p[1] = b;
                        n->pValue = p;
                    } else {
                        n->pValue = 0;
                    }
                }
                ((CButeTree*)self->m_pNode)->Insert((const char*)self->m_str104, n);
            }
            break;
        }
        case 11: { // <x, y, z> rect3 -> type 7
            double x, y, z;
            sscanf(self->m_token, s_fmtRect3, &x, &y, &z);
            if (self->m_10d) {
                CButeRef7* r = self->GetRef7(
                    (char*)(const char*)self->m_tagName,
                    (char*)(const char*)self->m_str104
                );
                double dx = *(double*)&r->a;
                double dy = *(double*)&r->c;
                double dz = *(double*)&r->e;
                (*accum += s_strLt).AppendDouble(dx);
                (*accum += s_strComma).AppendDouble(dy);
                (*accum += s_strComma).AppendDouble(dz);
                *accum += s_strGt;
            } else if (!bDup) {
                CButeValueNode* n = (CButeValueNode*)operator new(8);
                if (n) {
                    n->type = 7;
                    double* p = (double*)operator new(0x18);
                    if (p) {
                        p[0] = x;
                        p[1] = y;
                        p[2] = z;
                        n->pValue = p;
                    } else {
                        n->pValue = 0;
                    }
                }
                ((CButeTree*)self->m_pNode)->Insert((const char*)self->m_str104, n);
            }
            break;
        }
        case 12: { // [x, y] rect2 -> type 8
            double x, y;
            sscanf(self->m_token, s_fmtRect2, &x, &y);
            if (self->m_10d) {
                CButeRef8* r = self->GetRef8(
                    (char*)(const char*)self->m_tagName,
                    (char*)(const char*)self->m_str104
                );
                double dx = *(double*)&r->a;
                double dy = *(double*)&r->c;
                (*accum += s_strLBrack).AppendDouble(dx);
                (*accum += s_strComma).AppendDouble(dy);
                *accum += s_strRBrack;
            } else if (!bDup) {
                CButeValueNode* n = (CButeValueNode*)operator new(8);
                if (n) {
                    n->type = 8;
                    double* p = (double*)operator new(0x10);
                    if (p) {
                        p[0] = x;
                        p[1] = y;
                        n->pValue = p;
                    } else {
                        n->pValue = 0;
                    }
                }
                ((CButeTree*)self->m_pNode)->Insert((const char*)self->m_str104, n);
            }
            break;
        }
        case 8: { // quoted string -> type 4
            if (self->m_10d) {
                CString tmp(*(CString*)self->GetString(
                    (char*)(const char*)self->m_tagName,
                    (char*)(const char*)self->m_str104
                ));
                accum->Trim(0x22);
                *accum += tmp.GetBuffer(0);
                accum->Trim(0x22);
            } else if (!bDup) {
                CString s(self->m_token);
                CButeValueNode* n = (CButeValueNode*)operator new(8);
                if (n) {
                    n->type = 4;
                    n->pValue = new CString(s);
                }
                ((CButeTree*)self->m_pNode)->Insert((const char*)self->m_str104, n);
            }
            break;
        }
        default:
            self->ReportError(s_fmtInvalidToken, self->m_lineNo);
            return false;
    }

    if (self->m_10d) {
        self->m_captureText = 1;
    }
    return true;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetRef5 / GetRef6 / GetRef7 / GetRef8
// The typed-reference getter family (value type-tags 5..8). Each mirrors
// GetString: a function-local `static T s_default;` (MFC magic-static: a guard
// byte + inline zero-init + an atexit-registered dtor thunk) is returned by
// address on every error path, and `(T*)rec->pValue` on the matching type hit.
// The type check is the cmp-mem `if (rec->type == N)` form (success inline, the
// three failure blocks float to the tail as the nested-else cascade). The
// default struct's only role is its SIZE + non-trivial dtor (-> the magic-static
// atexit shape); its fields are zero on every miss.
//
// MATCH STATUS: GetRef5/GetRef6 are byte-exact (100% fuzzy). GetRef7/GetRef8 sit
// at the INVERSE zero-register-pinning wall (84-85%): retail uses immediate zero
// stores (`mov [field],0`) + `test eax,eax` null tests, while MSVC here pins ebp=0
// (`xor ebp,ebp` + `mov [field],ebp` + `cmp ebp,eax`) for the >=4-field zero-init.
// GetRef5 (also 4 fields) DOES pin ebp in retail, so this is an isolated allocator
// coin-flip - logic/CFG/offsets are byte-exact, only the zero-materialization +
// null-test register differ. Init-list / assignment-body / reversed-order ctor
// forms all produce identical (ebp-pinned) MSVC codegen; no source lever flips it
// (see docs/patterns/zero-register-pinning.md - the regalloc wall).
RVA(0x00173770, 0xc6)
CButeRef5* CButeMgr::GetRef5(char* tag, char* key) {
    static CButeRef5 s_default;

    void* grp = Tree()->Find(tag);
    if (grp) {
        CButeValue* rec = (CButeValue*)((CButeTree*)grp)->Find(key);
        if (rec) {
            if (rec->type == kButeRef5) {
                return (CButeRef5*)rec->pValue;
            }
            ReportError(s_fmtTypeMismatch, tag, key);
            return &s_default;
        }
        ReportError(s_fmtNotFound, tag, key);
        return &s_default;
    }
    ReportError(s_fmtInvalidTag, tag);
    return &s_default;
}

RVA(0x00173d00, 0xbb)
CButeRef6* CButeMgr::GetRef6(char* tag, char* key) {
    static CButeRef6 s_default;

    void* grp = Tree()->Find(tag);
    if (grp) {
        CButeValue* rec = (CButeValue*)((CButeTree*)grp)->Find(key);
        if (rec) {
            if (rec->type == kButeRef6) {
                return (CButeRef6*)rec->pValue;
            }
            ReportError(s_fmtTypeMismatch, tag, key);
            return &s_default;
        }
        ReportError(s_fmtNotFound, tag, key);
        return &s_default;
    }
    ReportError(s_fmtInvalidTag, tag);
    return &s_default;
}

RVA(0x00174240, 0xe3)
CButeRef7* CButeMgr::GetRef7(char* tag, char* key) {
    static CButeRef7 s_default;

    void* grp = Tree()->Find(tag);
    if (grp) {
        CButeValue* rec = (CButeValue*)((CButeTree*)grp)->Find(key);
        if (rec) {
            if (rec->type == kButeRef7) {
                return (CButeRef7*)rec->pValue;
            }
            ReportError(s_fmtTypeMismatch, tag, key);
            return &s_default;
        }
        ReportError(s_fmtNotFound, tag, key);
        return &s_default;
    }
    ReportError(s_fmtInvalidTag, tag);
    return &s_default;
}

RVA(0x001747c0, 0xcf)
CButeRef8* CButeMgr::GetRef8(char* tag, char* key) {
    static CButeRef8 s_default;

    void* grp = Tree()->Find(tag);
    if (grp) {
        CButeValue* rec = (CButeValue*)((CButeTree*)grp)->Find(key);
        if (rec) {
            if (rec->type == kButeRef8) {
                return (CButeRef8*)rec->pValue;
            }
            ReportError(s_fmtTypeMismatch, tag, key);
            return &s_default;
        }
        ReportError(s_fmtNotFound, tag, key);
        return &s_default;
    }
    ReportError(s_fmtInvalidTag, tag);
    return &s_default;
}

// ===========================================================================
// CButeMgr::InvokeCallback
// ===========================================================================
// Simple trampoline: takes a function pointer, calls it with `this`, returns `this`.
RVA(0x00171550, 0x11)
void* CButeMgr::InvokeCallback(void* (*fn)(CButeMgr*)) {
    fn(this);
    return this;
}

// ===========================================================================
// CButeMgr::ClearHelper
// ===========================================================================
// Calls two cleanup methods on the engine helper sub-object at this+0x14.
// FLAG (raw-offset, not cleanly removable): +0x14 is also where SetErrCallback
// stores m_errCallback and ReportError reads it. Modeling the helper as a real
// embedded member at +0x14 would collide with that already-matched field, so the
// char-offset view stays a transitional device until the helper-vs-callback
// overlap is reconciled (a follow-up matcher: is m_errCallback the helper's vptr
// slot, or does the helper start past +0x14?).
RVA(0x00171a40, 0x14)
void CButeMgr::ClearHelper() {
    CButeMgrHelper* h = (CButeMgrHelper*)((char*)this + 0x14);
    h->FuncA();
    h->FuncB();
}

// ===========================================================================
// CButeValue::SetDword
// ===========================================================================
// Allocates 4-byte storage, stores the value, sets the type field.
// Returns `this` (or NULL on alloc failure, though the target code always
// returns `this` with pValue reset to NULL).
RVA(0x00172000, 0x31)
CButeValue* CButeValue::SetDword(i32 type, u32 val) {
    this->type = type;
    u32* p = new u32;
    if (p) {
        *p = val;
        this->pValue = p;
    } else {
        this->pValue = 0;
    }
    return this;
}

// ===========================================================================
// CButeValue::SetFloat
// ===========================================================================
// Allocates 4-byte float storage, stores the value.
RVA(0x00172680, 0x31)
CButeValue* CButeValue::SetFloat(i32 type, float val) {
    this->type = type;
    float* p = new float;
    if (p) {
        *p = val;
        this->pValue = p;
    } else {
        this->pValue = 0;
    }
    return this;
}

// ===========================================================================
// CButeValue::SetInt
// ===========================================================================
// Allocates 4-byte int storage, stores the value.
RVA(0x00172b90, 0x31)
CButeValue* CButeValue::SetInt(i32 type, i32 val) {
    this->type = type;
    i32* p = new i32;
    if (p) {
        *p = val;
        this->pValue = p;
    } else {
        this->pValue = 0;
    }
    return this;
}

// ===========================================================================
// CButeValue::SetDouble
// ===========================================================================
// Allocates 8-byte double storage, stores the value.  Returns `this`.
RVA(0x00173140, 0x38)
CButeValue* CButeValue::SetDouble(i32 type, double val) {
    this->type = type;
    double* p = new double;
    if (p) {
        *p = val;
        this->pValue = p;
    } else {
        this->pValue = 0;
    }
    return this;
}

// ===========================================================================
// Lexer cluster (CButeMgr 0x170330-0x171a60) + the destructor at 0x0213c0.
// All __thiscall. The CButeTree sub-objects, the recursive node-walks, the
// stream reader, and the static lexer tables are reloc-masked externals.
// ===========================================================================

// ---------------------------------------------------------------------------
// CButeMgr::Init
// Reset the position/line counters and the two scratch CStrings to empty.
RVA(0x00170330, 0x34)
void CButeMgr::Init() {
    m_pos = 0;
    m_lineNo = 0;
    m_countLine = 1;
    m_0d = 0;
    m_tagName = g_emptyString;
    m_str104 = g_emptyString;
}

// ---------------------------------------------------------------------------
// CButeMgr::SetErrCallback
// Store the optional error callback at +0x14.
RVA(0x00170380, 0xa)
void CButeMgr::SetErrCallback(ErrCallback cb) {
    m_errCallback = cb;
}

// ---------------------------------------------------------------------------
// CButeMgr::NextChar
// Advance the input one char: pull the next byte position from the source
// stream (+0xa0), compute the delta from the base (m_streamBase), and -- unless
// the stream signals EOF (a self-indexed bitmap bit) -- record it as m_curChar,
// bump the line counter on the count-flag, track the newline flag, and advance
// the position (m_pos).
// @early-stop
// 88.67% scheduling/materialization wall: body/CFG/EOF-test/offsets are
// byte-identical; only the tail differs -- retail interleaves `mov m_curChar,al`
// between cmp and `sete cl` and stores the bool without pre-zeroing ecx, while
// MSVC here emits `xor ecx,ecx` + floats the m_curChar store past the sete. An
// identical instruction multiset, permuted (statement-schedule-faithful /
// outparam-zeroinit-scheduling family); no source spelling flips it.
RVA(0x00170390, 0x50)
void CButeMgr::NextChar() {
    i32 delta = ((CButeStream*)m_stream)->ReadByte() - m_streamBase;
    char* bitmap = *(char**)((char*)*(void**)m_stream + 4);
    if (bitmap[(i32)m_stream + 8] & 1) {
        m_curChar = 0;
        return;
    }
    if (m_countLine) {
        m_lineNo++;
    }
    m_curChar = (char)delta;
    m_countLine = delta == 0xa;
    m_pos += delta;
}

// ---------------------------------------------------------------------------
// CButeMgr::CharClass
// Map a raw char to its lexer character-class index (g_charClass[uc] - 1).
RVA(0x001703e0, 0x15)
i16 CButeMgr::CharClass(char c) {
    return (i16)(g_charClass[(u8)c] - 1);
}

// ---------------------------------------------------------------------------
// CButeMgr::PeekState / PeekState2
// Read one column of the 2D lexer transition table at row `state` (stride 147
// WORDs) / column CharClass(c). The two variants read adjacent columns
// (g_transTable[idx] vs [idx+1]).
RVA(0x00170400, 0x2f)
i16 CButeMgr::PeekState(i16 state, char c) {
    return ((i16(*)[147])g_transTable)[state][CharClass(c) * 3];
}

RVA(0x00170430, 0x2f)
i16 CButeMgr::PeekState2(i16 state, char c) {
    return g_transTable142[state][CharClass(c) * 3];
}

// ---------------------------------------------------------------------------
// CButeMgr::ScanState
// Write the token type (+0xaa) and the secondary lexer state (+0xac) from two
// adjacent columns of the transition table.
RVA(0x00170460, 0x58)
void CButeMgr::ScanState(i16 state, char c) {
    i32 base = state * 49;
    m_tokType = g_transTable[(base + CharClass(c)) * 3 + 1];
    m_lexState = g_transTable[(base + CharClass(c)) * 3 + 2];
}

// ---------------------------------------------------------------------------
// CButeMgr::SkipToTag
// Re-lex tokens until a tag/group token (type 1 or 2) is reached (return true),
// failing on a parse error or any non-continuation token. Type 4 continues the
// loop (re-running the attribute-file driver).
// @early-stop
// 78% block-layout wall: the body is byte-identical (the shared `je fail` and
// the `jne loop`-to-Parse loop-back all match) -- the ONLY divergence is the two
// 3-byte cold exit blocks `xor al,al`(false) / `mov al,1`(true) emitted in
// swapped .text order vs retail (false-first), which shifts the branch
// displacements that target them. A block-placement coin-flip; while/break,
// nested success-deepest, and continue forms all leave the order fixed.
RVA(0x00171160, 0x45)
bool CButeMgr::SkipToTag() {
    while (((ButeMgr*)this)->ParseAttributeFile()) {
        if (!Parse()) {
            break;
        }
        i16 t = m_tokType;
        if (t == 1 || t == 2) {
            return true;
        }
        if (t != 4) {
            break;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// CButeMgr::ParseGroup
// The recursive per-tag descent: advance, lex, and -- depending on the token
// type -- either accept the group (1), parse a tag line (2) and walk its
// matching nodes, or recurse. Loops while the current token stays a group.
// @early-stop
// 94.6%: two residuals, both walls. (1) The node-walk callback push references
// the static apply-fn at 0x1712b0 (an un-named recovery-gap function) so the
// reloc is masked (push $0 vs retail's section-offset push); naming it is a
// separate reconstruction. (2) A loop-rotation difference at the bottom: retail
// loops back with a bare `jne loopTop`, MSVC duplicates the ParseTagLine call
// onto the fall-through edge. CFG/offsets/the shared token-classify tail are
// otherwise byte-identical.
RVA(0x00171580, 0xba)
bool CButeMgr::ParseGroup() {
    NextChar();
    if (!Parse()) {
        return false;
    }
    i16 t = m_tokType;
    if (t == 1) {
        return true;
    }
    if (t != 2) {
        return false;
    }
    for (;;) {
        if (!ParseTagLine()) {
            return false;
        }
        if (m_10d) {
            CButeTree* grp = (CButeTree*)Tree48()->Find((char*)(const char*)m_tagName);
            if (grp) {
                grp->Walk(&ButeGroup_Apply, m_pText, 0);
            }
        }
        if (!Parse()) {
            return false;
        }
        t = m_tokType;
        if (t == 1) {
            return true;
        }
        if (t != 2) {
            if (t != 4) {
                return false;
            }
            if (!SkipToTag()) {
                return false;
            }
        }
        if (m_tokType == 1) {
            return true;
        }
    }
}

// ---------------------------------------------------------------------------
// CButeMgr::Exists
// Probe for a tag (and optionally a key under it): Tree()->Find(tag); on a hit
// with no key requested return true, else require the key to exist under it.
RVA(0x00171a60, 0x34)
bool CButeMgr::Exists(char* tag, char* key) {
    void* grp = Tree()->Find(tag);
    if (grp) {
        if (key == 0) {
            return true;
        }
        if (((CButeTree*)grp)->Find(key)) {
            return true;
        }
    }
    return false;
}

// NOTE: the CButeMgr scalar destructor at 0x0213c0 is now reconstructed above (the
// `CButeMgr::~CButeMgr()` near the top, in retail-RVA order); the @early-stop wall is
// documented there. The three sub-trees were retyped to the CButeStore value member
// (matching-neutral: same +0x18/+0x48/+0x74 offsets + 0x2c size), which left all 14
// already-matched getters/parser at 100%.

// ===========================================================================
// CButeMgrHelper cluster (0x1697c0-0x16c0c0). The helper sub-object embedded at
// CButeMgr+0x14 (a .bute registry/compiler object). All __thiscall.
//   0x1697c0 / 0x1699c0  - virtual-base vtable-init thunks (set one vbase vptr,
//                          then jmp the matching ret-only thunk to set another)
//   0x169c00 Construct   - the constructor (field-init + per-instance crit-sec +
//                          one-time shared crit-sec under a ref-count guard)
//   0x169dd0 SetSub      - replace the +0x4 sub-object (delete-through-vtable +
//                          flag toggle)
//   0x16b650 / 0x16c0c0  - ret-only vbase vtable-init thunks
// ===========================================================================

// The per-instance + shared critical-section init/delete go through engine
// thunks (0x16c9c0 / 0x16c9d0) over the Win32 imports; modeled as __cdecl
// externals (no body) so the `push p; call; add esp,4` shape reloc-masks.
extern "C" void Helper_InitCriticalSection(void* cs);

// The shared one-time-init guard + the shared critical section (reloc-masked
// file-scope DATA externs at 0x6bf400 / 0x6bf3c8).
DATA(0x006bf400)
extern "C" i32 g_helperRefCount; // 0x6bf400
DATA(0x006bf3c8)
extern "C" CRITICAL_SECTION g_helperSharedCS; // 0x6bf3c8

// The helper's primary vtable (shared with the cleanup dtor's restore); the four
// virtual-base vtables the init thunks stamp. Reloc-masked file-scope addresses.
DATA(0x005f03bc)
extern "C" void* g_helperVtbl; // 0x5f03bc
DATA(0x005f0374)
extern "C" void* g_helperVbaseVtblA; // 0x5f0374
DATA(0x005f0384)
extern "C" void* g_helperVbaseVtblB; // 0x5f0384
DATA(0x005f045c)
extern "C" void* g_helperVbaseVtblC; // 0x5f045c
DATA(0x005f047c)
extern "C" void* g_helperVbaseVtblD; // 0x5f047c

// The sub-object at +0x4. Its slot-0 virtual is a __thiscall scalar-deleting
// dtor (`mov eax,[ecx]; push 1; call [eax]`): modeled polymorphically so the
// receiver lands in ecx and the call carries no caller-side cleanup.
struct CButeSub {
    virtual void* ScalarDtor(i32 flags);
};

// ---------------------------------------------------------------------------
// CButeMgrHelper::InitVbaseA (0x1697c0)
// Virtual-base vtable-init thunk: read the vbtable pointer at this-0xc, follow
// its [+4] displacement to the virtual base subobject, stamp its vptr, then tail
// (jmp) into InitVbaseC to stamp the next vbase's vptr. The this-relative offset
// arithmetic reproduces the `mov eax,[ecx-0xc]; mov edx,[eax+4]; mov
// [edx+ecx-0xc],vtbl` form exactly; the tail call folds to the `jmp`.
RVA(0x001697c0, 0x13)
void CButeMgrHelper::InitVbaseA() {
    i32* vbptr = *(i32**)((char*)this - 0xc);
    *(void**)((char*)this - 0xc + vbptr[1]) = &g_helperVbaseVtblA;
    InitVbaseC();
}

// ---------------------------------------------------------------------------
// CButeMgrHelper::InitVbaseB (0x1699c0)
// As InitVbaseA but for the vbtable at this-0x8, then tail into InitVbaseD.
RVA(0x001699c0, 0x13)
void CButeMgrHelper::InitVbaseB() {
    i32* vbptr = *(i32**)((char*)this - 0x8);
    *(void**)((char*)this - 0x8 + vbptr[1]) = &g_helperVbaseVtblB;
    InitVbaseD();
}

// ---------------------------------------------------------------------------
// CButeMgrHelper::Construct (0x169c00)
// Constructor: zero-init the data fields, stamp the vptr + the type constants,
// init the per-instance critical section, and one-time-init the shared critical
// section under a ref-count guard.
RVA(0x00169c00, 0x67)
CButeMgrHelper* CButeMgrHelper::Construct() {
    m_pSub = 0;
    m_0c = 0;
    m_10 = 0;
    m_20 = 0;
    m_24 = 0;
    m_30 = 0;
    m_ownsSub = 0;
    m_vptr = &g_helperVtbl;
    m_flags = 4;
    m_28 = 6;
    m_2c = 0x20;
    m_34 = -1;
    Helper_InitCriticalSection(&m_cs);
    if (g_helperRefCount++ == 0) {
        Helper_InitCriticalSection(&g_helperSharedCS);
    }
    return this;
}

// ---------------------------------------------------------------------------
// CButeMgrHelper::SetSub (0x169dd0)
// Replace the +0x4 sub-object: if it is owned (m_ownsSub) and present (m_pSub),
// delete it through its slot-0 scalar-deleting dtor; store the new pointer;
// toggle bit 0x4 of the flag word from the new pointer's nullness.
RVA(0x00169dd0, 0x37)
void CButeMgrHelper::SetSub(void* p) {
    if (m_ownsSub && m_pSub) {
        ((CButeSub*)m_pSub)->ScalarDtor(1);
    }
    m_pSub = p;
    if (p) {
        m_flags &= ~0x4;
    } else {
        m_flags |= 0x4;
    }
}

// ---------------------------------------------------------------------------
// CButeMgrHelper::InitVbaseC (0x16b650)
// Ret-only virtual-base vtable-init thunk: stamp the vbase at this-0xc.
RVA(0x0016b650, 0xf)
void CButeMgrHelper::InitVbaseC() {
    i32* vbptr = *(i32**)((char*)this - 0xc);
    *(void**)((char*)this - 0xc + vbptr[1]) = &g_helperVbaseVtblC;
}

// ---------------------------------------------------------------------------
// CButeMgrHelper::InitVbaseD (0x16c0c0)
// Ret-only virtual-base vtable-init thunk: stamp the vbase at this-0x8.
RVA(0x0016c0c0, 0xf)
void CButeMgrHelper::InitVbaseD() {
    i32* vbptr = *(i32**)((char*)this - 0x8);
    *(void**)((char*)this - 0x8 + vbptr[1]) = &g_helperVbaseVtblD;
}
