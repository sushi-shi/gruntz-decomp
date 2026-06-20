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
void *operator new(unsigned int n);

// The node-ctor descriptor. Reloc-masked file-scope address.
extern int g_nodeDescriptor;

// CString::operator+= one char (__thiscall(receiver, char)):
// appends the char to the value-text accumulator (m_pText + 0xc). Modeled as an
// external (no-body) __thiscall on a tiny receiver class so the `mov ecx,recv;
// call` shape falls out reloc-masked.
class CButeText {
public:
    void AppendChar(char c);
};

// The token-length counter (file-scope signed WORD, read with movsx).
static short g_tokenLen;

// Error-reporter format strings (reloc-masked file-scope literals).
static const char s_fmtFormatError[]  = "ButeMgr (%d): A formatting error";
static const char s_fmtBadSymbol[]    = "ButeMgr (%d): Bad symbol encountered";
static const char s_fmtDupTag[]       = "ButeMgr: duplicate tag encountered";
static const char s_fmtTypeMismatch[] = "ButeMgr: Type mismatch - [%s]:%s";
static const char s_fmtInvalidTag[]   = "ButeMgr: Invalid tag specified - ";
static const char s_fmtNotFound[]     = "ButeMgr: Symbol not found - [%s]";

// Float/double zero-on-error constants (reloc-masked file-scope).
static const float  s_floatZero  = 0.0f;
static const double s_doubleZero = 0.0;

// ---------------------------------------------------------------------------
// CButeMgr::ScanToken
// Parse() (re)lexes one token, then ScanToken verifies it is the expected type
// (m_tokType == expectType). On mismatch it reports a formatting error (with the
// current line m_lineNo) and returns false; else true.
RVA(0x170710, 0x3b)
bool CButeMgr::ScanToken(int expectType)
{
    if (!Parse())
        return false;

    if (m_tokType != expectType) {
        CButeMgr_ReportError(this, s_fmtFormatError, m_lineNo);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetIntDef
// type-0 (int) getter with a caller default: Find(tag).Find(key); on a type-0
// hit return *(int*)rec->pValue, on type mismatch report + fall through, on any
// miss return def.
RVA(0x171aa0, 0x50)
int CButeMgr::GetIntDef(char *tag, char *key, int def)
{
    void *grp = Tree()->Find(tag);
    if (grp) {
        CButeValue *rec = (CButeValue *)((CButeTree *)grp)->Find(key);
        if (rec) {
            if (rec->type == 0)
                return *(int *)rec->pValue;
            CButeMgr_ReportError(this, s_fmtTypeMismatch, tag, key);
        }
    }
    return def;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetInt
// type-0 getter, no default: returns 0x80000000 on any miss (and reports the
// specific failure - type mismatch / symbol-not-found / invalid-tag).
RVA(0x171af0, 0x86)
int CButeMgr::GetInt(char *tag, char *key)
{
    void *grp = Tree()->Find(tag);
    if (grp) {
        CButeValue *rec = (CButeValue *)((CButeTree *)grp)->Find(key);
        if (rec) {
            if (rec->type == 0)
                return *(int *)rec->pValue;
            CButeMgr_ReportError(this, s_fmtTypeMismatch, tag, key);
            return (int)0x80000000;
        }
        CButeMgr_ReportError(this, s_fmtNotFound, tag, key);
        return (int)0x80000000;
    }
    CButeMgr_ReportError(this, s_fmtInvalidTag, tag);
    return (int)0x80000000;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetDwordDef
// type-1 (dword) getter with default. The type check is `if (--type == 0)` i.e.
// type == 1 (the disasm `mov ecx,[eax]; dec ecx; je`).
RVA(0x1721e0, 0x5a)
DWORD CButeMgr::GetDwordDef(char *tag, char *key, DWORD def)
{
    void *grp = Tree()->Find(tag);
    if (grp) {
        CButeValue *rec = (CButeValue *)((CButeTree *)grp)->Find(key);
        if (rec) {
            switch (rec->type) {
            case 1:
                return *(DWORD *)rec->pValue;
            }
            CButeMgr_ReportError(this, s_fmtTypeMismatch, tag, key);
        }
    }
    return def;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetDword
// type-1 getter, no default: returns 0 on any miss.
RVA(0x172240, 0x7d)
DWORD CButeMgr::GetDword(char *tag, char *key)
{
    void *grp = Tree()->Find(tag);
    if (grp) {
        CButeValue *rec = (CButeValue *)((CButeTree *)grp)->Find(key);
        if (rec) {
            switch (rec->type) {
            case 1:
                return *(DWORD *)rec->pValue;
            }
            CButeMgr_ReportError(this, s_fmtTypeMismatch, tag, key);
            return 0;
        }
        CButeMgr_ReportError(this, s_fmtNotFound, tag, key);
        return 0;
    }
    CButeMgr_ReportError(this, s_fmtInvalidTag, tag);
    return 0;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetFloat
// type-3 (float) getter, no default. Accepts type 0 (int) too: `fild` the int
// when type==0, `fld` the float when type==3, else report + return 0.0f. Returns
// in st(0) (an x87 float return).
RVA(0x172730, 0x9a)
float CButeMgr::GetFloat(char *tag, char *key)
{
    void *grp = Tree()->Find(tag);
    if (grp) {
        CButeValue *rec = (CButeValue *)((CButeTree *)grp)->Find(key);
        if (rec) {
            switch (rec->type) {
            case 0:
                return (float)*(int *)rec->pValue;
            case 3:
                return *(float *)rec->pValue;
            }
            CButeMgr_ReportError(this, s_fmtTypeMismatch, tag, key);
            return s_floatZero;
        }
        CButeMgr_ReportError(this, s_fmtNotFound, tag, key);
        return s_floatZero;
    }
    CButeMgr_ReportError(this, s_fmtInvalidTag, tag);
    return s_floatZero;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetDouble
// type-2 (double) getter, no default. Accepts type 0 (int): `fild` when type==0,
// `fld qword` when type==2, else report + return 0.0. (The disasm tests with
// `sub ecx,0; je` then `sub ecx,2; je` -> the type==0 and type==2 branches.)
RVA(0x172c40, 0x9b)
double CButeMgr::GetDouble(char *tag, char *key)
{
    void *grp = Tree()->Find(tag);
    if (grp) {
        CButeValue *rec = (CButeValue *)((CButeTree *)grp)->Find(key);
        if (rec) {
            switch (rec->type) {
            case 0:
                return (double)*(int *)rec->pValue;
            case 2:
                return *(double *)rec->pValue;
            }
            CButeMgr_ReportError(this, s_fmtTypeMismatch, tag, key);
            return s_doubleZero;
        }
        CButeMgr_ReportError(this, s_fmtNotFound, tag, key);
        return s_doubleZero;
    }
    CButeMgr_ReportError(this, s_fmtInvalidTag, tag);
    return s_doubleZero;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetStringDef
// type-4 (string) getter with default: returns rec->pValue (the char*) on a
// type-4 hit, reports a type mismatch otherwise, returns def on any miss.
RVA(0x173180, 0x4e)
CString *CButeMgr::GetStringDef(char *tag, char *key, CString *def)
{
    void *grp = Tree()->Find(tag);
    if (grp) {
        CButeValue *rec = (CButeValue *)((CButeTree *)grp)->Find(key);
        if (rec) {
            if (rec->type == 4)
                return (CString *)rec->pValue;
            CButeMgr_ReportError(this, s_fmtTypeMismatch, tag, key);
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
RVA(0x1731d0, 0xb6)
char *CButeMgr::GetString(char *tag, char *key)
{
    static CString s_empty("");

    void *grp = Tree()->Find(tag);
    if (grp) {
        CButeValue *rec = (CButeValue *)((CButeTree *)grp)->Find(key);
        if (rec) {
            if (rec->type == 4)
                return (char *)rec->pValue;
            CButeMgr_ReportError(this, s_fmtTypeMismatch, tag, key);
            return (char *)&s_empty;
        }
        CButeMgr_ReportError(this, s_fmtNotFound, tag, key);
        return (char *)&s_empty;
    }
    CButeMgr_ReportError(this, s_fmtInvalidTag, tag);
    return (char *)&s_empty;
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
RVA(0x1711b0, 0xf5)
bool CButeMgr::ParseTagLine()
{
    if (!ScanToken(4))
        return false;

    char *tok = m_token;
    m_tagName = tok;

    if (!m_10d) {
        CButeTree *t = Tree();
        if (t->Find(tok)) {
            CButeMgr_ReportError(this, s_fmtDupTag, tok);
            return false;
        }
        CButeNode *node = new CButeNode(&g_nodeDescriptor, 2);
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
RVA(0x1704c0, 0x1e3)
bool CButeMgr::Parse()
{
    int kind = 0x11;
    g_tokenLen = 0;

    for (;;) {
        short cls = PeekClass(kind, m_curChar);
        switch (cls) {
        case 0:  // bad symbol
            CButeMgr_ReportError(this, s_fmtBadSymbol, m_lineNo);
            return false;

        case 1:  // value char: scan, store to token buffer, echo, advance, loop
            kind = ReadValue(kind, m_curChar);
            m_token[g_tokenLen++] = m_curChar;
            if (m_10c != 0 && m_curChar != 0)
                ((CButeText *)((char *)m_pText + 0xc))->AppendChar(m_curChar);
            NextChar();
            break;

        case 2:  // value char: scan, echo only, advance, loop
            kind = ReadValue(kind, m_curChar);
            if (m_10c != 0 && m_curChar != 0)
                ((CButeText *)((char *)m_pText + 0xc))->AppendChar(m_curChar);
            NextChar();
            break;

        case 3:  // identifier: scan, store, echo, advance, recurse, terminate
            ReadIdent(kind, m_curChar);
            m_token[g_tokenLen++] = m_curChar;
            if (m_10c != 0 && m_curChar != 0)
                ((CButeText *)((char *)m_pText + 0xc))->AppendChar(m_curChar);
            NextChar();
            if (m_tokType == 0)
                Parse();
            m_token[g_tokenLen] = 0;
            return true;

        case 4:  // identifier: scan, echo only, advance, recurse, terminate
            ReadIdent(kind, m_curChar);
            if (m_10c != 0 && m_curChar != 0)
                ((CButeText *)((char *)m_pText + 0xc))->AppendChar(m_curChar);
            NextChar();
            if (m_tokType == 0)
                Parse();
            m_token[g_tokenLen] = 0;
            return true;

        case 5:  // identifier: scan, recurse, terminate (no echo, no advance)
            ReadIdent(kind, m_curChar);
            if (m_tokType == 0)
                Parse();
            m_token[g_tokenLen] = 0;
            return true;
        }
    }
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
RVA(0x173770, 0xc6)
CButeRef5 *CButeMgr::GetRef5(char *tag, char *key)
{
    static CButeRef5 s_default;

    void *grp = Tree()->Find(tag);
    if (grp) {
        CButeValue *rec = (CButeValue *)((CButeTree *)grp)->Find(key);
        if (rec) {
            if (rec->type == 5)
                return (CButeRef5 *)rec->pValue;
            CButeMgr_ReportError(this, s_fmtTypeMismatch, tag, key);
            return &s_default;
        }
        CButeMgr_ReportError(this, s_fmtNotFound, tag, key);
        return &s_default;
    }
    CButeMgr_ReportError(this, s_fmtInvalidTag, tag);
    return &s_default;
}

RVA(0x173d00, 0xbb)
CButeRef6 *CButeMgr::GetRef6(char *tag, char *key)
{
    static CButeRef6 s_default;

    void *grp = Tree()->Find(tag);
    if (grp) {
        CButeValue *rec = (CButeValue *)((CButeTree *)grp)->Find(key);
        if (rec) {
            if (rec->type == 6)
                return (CButeRef6 *)rec->pValue;
            CButeMgr_ReportError(this, s_fmtTypeMismatch, tag, key);
            return &s_default;
        }
        CButeMgr_ReportError(this, s_fmtNotFound, tag, key);
        return &s_default;
    }
    CButeMgr_ReportError(this, s_fmtInvalidTag, tag);
    return &s_default;
}

RVA(0x174240, 0xe3)
CButeRef7 *CButeMgr::GetRef7(char *tag, char *key)
{
    static CButeRef7 s_default;

    void *grp = Tree()->Find(tag);
    if (grp) {
        CButeValue *rec = (CButeValue *)((CButeTree *)grp)->Find(key);
        if (rec) {
            if (rec->type == 7)
                return (CButeRef7 *)rec->pValue;
            CButeMgr_ReportError(this, s_fmtTypeMismatch, tag, key);
            return &s_default;
        }
        CButeMgr_ReportError(this, s_fmtNotFound, tag, key);
        return &s_default;
    }
    CButeMgr_ReportError(this, s_fmtInvalidTag, tag);
    return &s_default;
}

RVA(0x1747c0, 0xcf)
CButeRef8 *CButeMgr::GetRef8(char *tag, char *key)
{
    static CButeRef8 s_default;

    void *grp = Tree()->Find(tag);
    if (grp) {
        CButeValue *rec = (CButeValue *)((CButeTree *)grp)->Find(key);
        if (rec) {
            if (rec->type == 8)
                return (CButeRef8 *)rec->pValue;
            CButeMgr_ReportError(this, s_fmtTypeMismatch, tag, key);
            return &s_default;
        }
        CButeMgr_ReportError(this, s_fmtNotFound, tag, key);
        return &s_default;
    }
    CButeMgr_ReportError(this, s_fmtInvalidTag, tag);
    return &s_default;
}

// ===========================================================================
// CButeMgr::InvokeCallback
// ===========================================================================
// Simple trampoline: takes a function pointer, calls it with `this`, returns `this`.
RVA(0x171550, 0x11)
void *CButeMgr::InvokeCallback(void *(*fn)(CButeMgr *))
{
    fn(this);
    return this;
}

// ===========================================================================
// CButeMgr::ClearHelper
// ===========================================================================
// Calls two cleanup methods on the engine helper object at this+0x14.
RVA(0x171a40, 0x14)
void CButeMgr::ClearHelper()
{
    CButeMgrHelper *h = (CButeMgrHelper *)((char *)this + 0x14);
    h->FuncA();
    h->FuncB();
}

// ===========================================================================
// CButeValue::SetDword
// ===========================================================================
// Allocates 4-byte storage, stores the value, sets the type field.
// Returns `this` (or NULL on alloc failure, though the target code always
// returns `this` with pValue reset to NULL).
RVA(0x172000, 0x31)
CButeValue *CButeValue::SetDword(int type, unsigned long val)
{
    this->type = type;
    unsigned long *p = new unsigned long;
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
RVA(0x172680, 0x31)
CButeValue *CButeValue::SetFloat(int type, float val)
{
    this->type = type;
    float *p = new float;
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
RVA(0x172b90, 0x31)
CButeValue *CButeValue::SetInt(int type, int val)
{
    this->type = type;
    int *p = new int;
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
RVA(0x173140, 0x38)
CButeValue *CButeValue::SetDouble(int type, double val)
{
    this->type = type;
    double *p = new double;
    if (p) {
        *p = val;
        this->pValue = p;
    } else {
        this->pValue = 0;
    }
    return this;
}
