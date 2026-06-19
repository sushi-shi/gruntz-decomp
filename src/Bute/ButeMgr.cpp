// ButeMgr.cpp - CButeMgr, the engine's `.att`/`.bute` attribute config-parser.
// The attribute layer the whole game reads entity stats through: a two-level
// keyed store (tag-group -> key -> typed value) built by a recursive-descent
// lexer/parser and queried by a family of typed getters.
//
// Functions matched in this TU (RVAs in GRUNTZ.EXE):
//   CButeMgr::ScanToken    @ 0x170710  (59 B,  thiscall ret 4)  - token-expect
//   CButeMgr::GetIntDef    @ 0x171aa0  (80 B,  thiscall ret c)  - type 0, default
//   CButeMgr::GetInt       @ 0x171af0  (134 B, thiscall ret 8)  - type 0
//   CButeMgr::GetDwordDef  @ 0x1721e0  (90 B,  thiscall ret c)  - type 1, default
//   CButeMgr::GetDword     @ 0x172240  (125 B, thiscall ret 8)  - type 1
//   CButeMgr::GetFloat     @ 0x172730  (154 B, thiscall ret 8)  - type 3 (or int)
//   CButeMgr::GetDouble    @ 0x172c40  (155 B, thiscall ret 8)  - type 2 (or int)
//   CButeMgr::GetStringDef @ 0x173180  (78 B,  thiscall ret c)  - type 4, default
//   CButeMgr::GetString    @ 0x1731d0  (182 B, thiscall ret 8)  - type 4
//   CButeMgr::ParseTagLine @ 0x1711b0  (245 B, thiscall ret)    - one tag=value line
//   CButeMgr::Parse        @ 0x1704c0  (483 B, thiscall ret)    - the .att parser
//
// The getters funnel through one __thiscall find-by-key helper (engine @0x16d190,
// CButeTree::Find): outer Find(tag) on m_tree (+0x18) yields the tag sub-tree;
// inner Find(key) yields the typed value record { int type; void* pValue; }. Each
// getter checks the record's type then reads the value through pValue. On any miss
// (no tag / no key / type mismatch) it calls the variadic error reporter (@0x1706c0)
// and returns the default (or 0 / 0x80000000 / an empty CString).
//
// The error-string format literals are reloc-masked file-scope constants. The CRT
// tokenizer helpers + the tree Find/Insert + the node ctor are external/no-body
// engine calls (reloc-masked). ParseTagLine constructs a store node + carries a
// C++ EH frame (the CString copy + the node ctor under unwind) -> /GX.
#include "ButeMgr.h"

// Global operator new (engine NAFXCW @0x1b9b46); external/no-body so the
// `push 0x2c; call ??2; add esp,4` shape falls out reloc-masked.
void *operator new(unsigned int n);

// The node-ctor descriptor (@0x574df0). Reloc-masked file-scope address.
extern int g_nodeDescriptor;

// CString::operator+= one char (engine @0x192060, __thiscall(receiver, char)):
// appends the char to the value-text accumulator (m_pText + 0xc). Modeled as an
// external (no-body) __thiscall on a tiny receiver class so the `mov ecx,recv;
// call` shape falls out reloc-masked.
class CButeText {
public:
    void AppendChar(char c);   // @0x192060
};

// The token-length counter (file-scope signed WORD @0x6bf678; read with movsx).
static short g_tokenLen;

// Error-reporter format strings (reloc-masked file-scope literals).
static const char s_fmtFormatError[]  = "ButeMgr (%d): A formatting error";
static const char s_fmtBadSymbol[]    = "ButeMgr (%d): Bad symbol encountered";
static const char s_fmtDupTag[]       = "ButeMgr: duplicate tag encountered";
static const char s_fmtTypeMismatch[] = "ButeMgr: Type mismatch - [%s]:%s";
static const char s_fmtInvalidTag[]   = "ButeMgr: Invalid tag specified - ";
static const char s_fmtNotFound[]     = "ButeMgr: Symbol not found - [%s]";

// Float/double zero-on-error constants (reloc-masked file-scope @0x5f0520/0x5f0528).
static const float  s_floatZero  = 0.0f;
static const double s_doubleZero = 0.0;

// ---------------------------------------------------------------------------
// CButeMgr::ScanToken  @ 0x170710 (59 B, ret 4).
// Parse() (re)lexes one token, then ScanToken verifies it is the expected type
// (m_tokType == expectType). On mismatch it reports a formatting error (with the
// current line m_lineNo) and returns false; else true.
// ---------------------------------------------------------------------------
// @address: 0x170710
// @size:    0x3b
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
// CButeMgr::GetIntDef  @ 0x171aa0 (80 B, ret c).
// type-0 (int) getter with a caller default: Find(tag).Find(key); on a type-0
// hit return *(int*)rec->pValue, on type mismatch report + fall through, on any
// miss return def.
// ---------------------------------------------------------------------------
// @address: 0x171aa0
// @size:    0x50
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
// CButeMgr::GetInt  @ 0x171af0 (134 B, ret 8).
// type-0 getter, no default: returns 0x80000000 on any miss (and reports the
// specific failure - type mismatch / symbol-not-found / invalid-tag).
// ---------------------------------------------------------------------------
// @address: 0x171af0
// @size:    0x86
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
// CButeMgr::GetDwordDef  @ 0x1721e0 (90 B, ret c).
// type-1 (dword) getter with default. The type check is `if (--type == 0)` i.e.
// type == 1 (the disasm `mov ecx,[eax]; dec ecx; je`).
// ---------------------------------------------------------------------------
// @address: 0x1721e0
// @size:    0x5a
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
// CButeMgr::GetDword  @ 0x172240 (125 B, ret 8).
// type-1 getter, no default: returns 0 on any miss.
// ---------------------------------------------------------------------------
// @address: 0x172240
// @size:    0x7d
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
// CButeMgr::GetFloat  @ 0x172730 (154 B, ret 8).
// type-3 (float) getter, no default. Accepts type 0 (int) too: `fild` the int
// when type==0, `fld` the float when type==3, else report + return 0.0f. Returns
// in st(0) (an x87 float return).
// ---------------------------------------------------------------------------
// @address: 0x172730
// @size:    0x9a
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
// CButeMgr::GetDouble  @ 0x172c40 (155 B, ret 8).
// type-2 (double) getter, no default. Accepts type 0 (int): `fild` when type==0,
// `fld qword` when type==2, else report + return 0.0. (The disasm tests with
// `sub ecx,0; je` then `sub ecx,2; je` -> the type==0 and type==2 branches.)
// ---------------------------------------------------------------------------
// @address: 0x172c40
// @size:    0x9b
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
// CButeMgr::GetStringDef  @ 0x173180 (78 B, ret c).
// type-4 (string) getter with default: returns rec->pValue (the char*) on a
// type-4 hit, reports a type mismatch otherwise, returns def on any miss.
// ---------------------------------------------------------------------------
// @address: 0x173180
// @size:    0x4e
char *CButeMgr::GetStringDef(char *tag, char *key, char *def)
{
    void *grp = Tree()->Find(tag);
    if (grp) {
        CButeValue *rec = (CButeValue *)((CButeTree *)grp)->Find(key);
        if (rec) {
            if (rec->type == 4)
                return (char *)rec->pValue;
            CButeMgr_ReportError(this, s_fmtTypeMismatch, tag, key);
        }
    }
    return def;
}

// ---------------------------------------------------------------------------
// CButeMgr::GetString  @ 0x1731d0 (182 B, ret 8).
// type-4 getter, no default: returns rec->pValue on a hit, else reports the
// specific failure and returns a shared empty CString. The empty string is a
// function-local static CString (MFC magic-static: one-shot guarded ctor +
// atexit-registered dtor) returned by address on every error path.
// ---------------------------------------------------------------------------
// @address: 0x1731d0
// @size:    0xb6
char *CButeMgr::GetString(char *tag, char *key)
{
    static AfxString s_empty("");

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
// CButeMgr::ParseTagLine  @ 0x1711b0 (245 B, ret).
// Reads one "tag = value" line: ScanToken(4) for the tag name, copies it into the
// active-tag CString (m_tagName), and -- unless m_10d suppresses it -- checks for a
// duplicate tag in the store (reporting + bailing if found), else allocates a new
// store node, constructs it, wires its two vtables, records it (m_pNode) and
// inserts it under the tag name; finally ScanToken(3) consumes the value. The new
// node lives under a C++ EH frame (freed on unwind if the ctor throws) -> /GX.
// ---------------------------------------------------------------------------
// @address: 0x1711b0
// @size:    0xf5
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

    return ScanToken(3);
}

// ---------------------------------------------------------------------------
// CButeMgr::Parse  @ 0x1704c0 (483 B, ret).
// The recursive-descent token lexer: resets the token-length counter, then loops
// classifying the current character (ButeLex_PeekClass) -- skipping any class > 5
// -- and dispatching a 6-way jump table on the token class to read identifiers/
// values/punctuation (ButeLex_ReadValue/ReadIdent), append chars to the token
// buffer, advance the lexer (ButeLex_NextChar), and recurse for nested groups.
// Reports "Bad symbol encountered" (with m_lineNo) on the error class.
// ---------------------------------------------------------------------------
// @address: 0x1704c0
// @size:    0x1e3
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

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: low
// @source: decomp-xref
// @address: 0x173770
// @size:    0xc6
// @stub
void CButeMgr::Stub_173770() {}

// @confidence: low
// @source: decomp-xref
// @address: 0x173d00
// @size:    0xbb
// @stub
void CButeMgr::Stub_173d00() {}

// @confidence: low
// @source: decomp-xref
// @address: 0x174240
// @size:    0xe3
// @stub
void CButeMgr::Stub_174240() {}

// @confidence: low
// @source: decomp-xref
// @address: 0x1747c0
// @size:    0xcf
// @stub
void CButeMgr::Stub_1747c0() {}
