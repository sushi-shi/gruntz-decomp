// ButeMgr.cpp - CButeMgr, the engine's `.att`/`.bute` attribute config-parser.
// The attribute layer the whole game reads entity stats through: a two-level
// keyed store (tag-group -> key -> typed value) built by a recursive-descent
// lexer/parser and queried by a family of typed getters.
//
// Functions matched in this TU (RVAs in GRUNTZ.EXE):
//   CButeMgr::Parse              @ 0x1704c0  (483 B, thiscall ret)
//   CButeMgr_ReportError         @ 0x1706c0  (75 B,  cdecl)
//   CButeMgr::ScanToken          @ 0x170710  (59 B,  thiscall ret 4)
//   CButeMgr::ParseAttributeFile @ 0x170750  (2520 B, thiscall, /GX)
//   CButeMgr::ParseBody          @ 0x171160  (69 B,  thiscall ret)
//   CButeMgr::ParseTagLine       @ 0x1711b0  (245 B, thiscall, /GX)
//   CButeMgr::InvokeCallback     @ 0x171550  (17 B,  thiscall ret 4)
//   CButeMgr::ParseAttributeBlock@ 0x171580  (186 B, thiscall ret)
//   CButeMgr::ClearHelper        @ 0x171a40  (20 B,  thiscall ret)
//   CButeMgr::IsKey              @ 0x171a60  (52 B,  thiscall ret 8)
//   CButeMgr::GetIntDef          @ 0x171aa0  (80 B,  thiscall ret c)
//   CButeMgr::GetInt             @ 0x171af0  (134 B, thiscall ret 8)
//   CButeValue::SetDword         @ 0x172000  (49 B,  thiscall ret 8)
//   CButeValue::Assign           @ 0x172040  (252 B, thiscall ret 4)
//   CButeValue::Free             @ 0x172160  (82 B,  thiscall ret)
//   CButeMgr::GetDwordDef        @ 0x1721e0  (90 B,  thiscall ret c)
//   CButeMgr::GetDword           @ 0x172240  (125 B, thiscall ret 8)
//   CButeValue::SetFloat         @ 0x172680  (49 B,  thiscall ret 8)
//   CButeMgr::GetFloat           @ 0x172730  (154 B, thiscall ret 8)
//   CButeValue::SetInt           @ 0x172b90  (49 B,  thiscall ret 8)
//   CButeMgr::GetDouble          @ 0x172c40  (155 B, thiscall ret 8)
//   CButeValue::SetDouble        @ 0x173140  (56 B,  thiscall ret c)
//   CButeMgr::GetStringDef       @ 0x173180  (78 B,  thiscall ret c)
//   CButeMgr::GetString          @ 0x1731d0  (182 B, thiscall ret 8)
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
// engine calls (reloc-masked). ParseAttributeFile carries a C++ EH frame
// (/GX, needed for the local CString temp in the string-value case).
#include "ButeMgr.h"
#include <stdarg.h>
#include <stdlib.h>

// Global operator new (engine NAFXCW @0x1b9b46); external/no-body so the
// `push 0x2c; call ??2; add esp,4` shape falls out reloc-masked.
void *operator new(unsigned int n);
void  operator delete(void *p);

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
static const char s_fmtDupSymbol[]    = "ButeMgr: duplicate symbol encountered";
static const char s_fmtTypeMismatch[] = "ButeMgr: Type mismatch - [%s]:%s";
static const char s_fmtInvalidTag[]   = "ButeMgr: Invalid tag specified - ";
static const char s_fmtNotFound[]     = "ButeMgr: Symbol not found - [%s]";
static const char s_fmtInvalidToken[] = "ButeMgr (%d): Invalid token encountered";

// Float/double zero-on-error constants (reloc-masked file-scope @0x5f0520/0x5f0528).
static const float  s_floatZero  = 0.0f;
static const double s_doubleZero = 0.0;

// Engine function declarations for external calls used by ParseAttributeFile.
extern "C" int engine_OutputString(const char *s);   // @0x121770
extern "C" double engine_atof(const char *s);        // @0x18d220
extern "C" unsigned long engine_strtoul(const char *s, char **end, int base); // @0x1240b0
extern "C" int engine_sscanf(const char *s, const char *fmt, ...);            // @0x120900
extern "C" int engine_sprintf(char *buf, const char *fmt, ...);               // @0x16be60
extern "C" int engine_appendString(void *pStr, const char *s);                // @0x191d20
extern "C" int engine_appendInt(void *pStr, int val);                         // @0x1921e0
extern "C" int engine_appendDouble(void *pStr, double d);                     // @0x191df0
extern "C" int engine_appendFormat(void *pStr, const char *fmt, ...);         // @0x192120

// CString scratch buffer methods used by ReportError (engine, no body).
class CStringScratch {
public:
    char *GetBuffer(int n);   // @0x1ba11c  (format + get buffer, takes 3 args)
    void  ReleaseBuffer(int n); // @0x1ba16b
};

// ===========================================================================
// CButeMgr::Parse  @ 0x1704c0 (483 B, ret).
// ===========================================================================
// The recursive-descent token lexer: resets the token-length counter, then loops
// classifying the current character (ButeLex_PeekClass) -- skipping any class > 5
// -- and dispatching a 6-way jump table on the token class to read identifiers/
// values/punctuation (ButeLex_ReadValue/ReadIdent), append chars to the token
// buffer, advance the lexer (ButeLex_NextChar), and recurse for nested groups.
// Reports "Bad symbol encountered" (with m_lineNo) on the error class.
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

// ===========================================================================
// CButeMgr_ReportError  @ 0x1706c0 (75 B, cdecl).
// ===========================================================================
// The variadic error reporter. Formats the message into a CString scratch buffer
// at self+0x10 (m_errBuf), calls the engine's OutputString, then (if a callback
// is registered at self+0x14) formats the message again and calls the callback.
// @address: 0x1706c0
// @size:    0x4b
int __cdecl CButeMgr_ReportError(CButeMgr *self, const char *fmt, ...)
{
    CStringScratch *buf = (CStringScratch *)((char *)self + 0x10);
    char *result;
    va_list va;

    va = (va_list)((char *)&fmt + 4);
    result = buf->GetBuffer(0x100);
    engine_OutputString(result);
    buf->ReleaseBuffer(-1);

    if (self->m_pErrCb) {
        result = buf->GetBuffer(0);
        ((void (__cdecl *)(const char *))self->m_pErrCb)(result);
    }
    return 0;
}

// ===========================================================================
// CButeMgr::ScanToken  @ 0x170710 (59 B, ret 4).
// ===========================================================================
// Parse() (re)lexes one token, then ScanToken verifies it is the expected type
// (m_tokType == expectType). On mismatch it reports a formatting error (with the
// current line m_lineNo) and returns false; else true.
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

// ===========================================================================
// CButeMgr::ParseAttributeFile  @ 0x170750 (2520 B, /GX).
// ===========================================================================
// Attribute-file section parser. Called with m_token holding a section-header
// token (like "[Section]").  Assigns m_curTag from m_token, checks for duplicate
// tags (via m_pNode's store tree), then reads section-body tokens via
// ScanToken(5) -> Parse() and dispatches token types 6..16:
//
//   6  = int            (type 0, 4B)
//   7  = dword          (type 1, 4B)
//   8  = float          (type 3, 4B)
//   9  = float variant  (type 3, 4B)
//   10 = double         (type 2, 8B)
//   11 = rect (4 ints)  (type 5, 16B)
//   12 = point (2 ints) (type 6, 8B)
//   13 = vector3        (type 7, 24B)
//   14 = vector2        (type 8, 16B)
//   15 = string         (type 4, CString)
//   16 = string variant (type 4, CString)
//
// On a non-duplicate / non-write-back parse, each case allocates a CButeValue
// node + value storage and inserts it into m_pNode's tree.
//
// Has C++ EH frame (/GX) from the local AfxString in the string case.
// @address: 0x170750
// @size:    0x9d8
bool CButeMgr::ParseAttributeFile()
{
    bool dupTag = false;

    // Copy the current token buffer into m_curTag (the active tag).
    // m_token holds the section-header text at this point.
    m_curTag = m_token;

    if (!m_10d) {
        // Check if this tag already exists in the store (via m_pNode's tree).
        if (((CButeTree *)m_pNode)->Find((const char *)m_curTag)) {
            CButeMgr_ReportError(this, s_fmtDupSymbol, (const char *)m_curTag);
            dupTag = true;
        }
    }

    // ScanToken(5) verifies the section-header token and moves to next.
    if (!ScanToken(5))
        return false;

    if (m_10d) {
        ((CButeText *)((char *)m_pText + 0xc))->AppendChar(' ');
        m_10c = 0;
    }

    // Parse the first token inside the section
    if (!Parse())
        return false;

    // Dispatch on token type (m_tokType values 6..16)
    switch (m_tokType) {
    case 6: {
        // --- INT (type 0) ---
        int val = atoi(m_token);
        if (!m_10d && !dupTag) {
            CButeValue *node = (CButeValue *)operator new(8);
            if (node) {
                node->type = 0;
                int *p = (int *)operator new(4);
                if (p) {
                    *p = val;
                    node->pValue = p;
                } else {
                    node->pValue = NULL;
                }
            }
            ((CButeTree *)m_pNode)->Insert((const char *)m_curTag, node);
        }
        break;
    }
    case 7: {
        // --- DWORD (type 1) ---
        if (!ScanToken(6))
            return false;
        DWORD dwVal = engine_strtoul(m_token, NULL, 10);
        if (!m_10d && !dupTag) {
            CButeValue *node = (CButeValue *)operator new(8);
            if (node) {
                node->type = 1;
                DWORD *p = (DWORD *)operator new(4);
                if (p) {
                    *p = dwVal;
                    node->pValue = p;
                } else {
                    node->pValue = NULL;
                }
            }
            ((CButeTree *)m_pNode)->Insert((const char *)m_curTag, node);
        }
        break;
    }
    case 8: {
        // --- FLOAT (type 3), token type 8 ---
        if (!ScanToken(8))
            return false;
        float fVal = (float)engine_atof(m_token);
        if (!m_10d && !dupTag) {
            CButeValue *node = (CButeValue *)operator new(8);
            if (node) {
                node->type = 3;
                float *p = (float *)operator new(4);
                if (p) {
                    *p = fVal;
                    node->pValue = p;
                } else {
                    node->pValue = NULL;
                }
            }
            ((CButeTree *)m_pNode)->Insert((const char *)m_curTag, node);
        }
        break;
    }
    case 9: {
        // --- FLOAT variant (type 3), token type 9 ---
        float fVal = (float)engine_atof(m_token);
        if (!m_10d && !dupTag) {
            CButeValue *node = (CButeValue *)operator new(8);
            if (node) {
                node->type = 3;
                float *p = (float *)operator new(4);
                if (p) {
                    *p = fVal;
                    node->pValue = p;
                } else {
                    node->pValue = NULL;
                }
            }
            ((CButeTree *)m_pNode)->Insert((const char *)m_curTag, node);
        }
        break;
    }
    case 10: {
        // --- DOUBLE (type 2) ---
        double dVal = engine_atof(m_token);
        if (!m_10d && !dupTag) {
            CButeValue *node = (CButeValue *)operator new(8);
            if (node) {
                node->type = 2;
                double *p = (double *)operator new(8);
                if (p) {
                    *p = dVal;
                    node->pValue = p;
                } else {
                    node->pValue = NULL;
                }
            }
            ((CButeTree *)m_pNode)->Insert((const char *)m_curTag, node);
        }
        break;
    }
    case 11: {
        // --- RECT (type 5), 4 ints ---
        int a = 0, b = 0, c = 0, d = 0;
        engine_sscanf(m_token, "(%d %d %d %d)", &a, &b, &c, &d);
        if (!m_10d && !dupTag) {
            CButeValue *node = (CButeValue *)operator new(8);
            if (node) {
                node->type = 5;
                int *p = (int *)operator new(16);
                if (p) {
                    p[0] = a; p[1] = b; p[2] = c; p[3] = d;
                    node->pValue = p;
                } else {
                    node->pValue = NULL;
                }
            }
            ((CButeTree *)m_pNode)->Insert((const char *)m_curTag, node);
        }
        break;
    }
    case 12: {
        // --- POINT (type 6), 2 ints ---
        int x = 0, y = 0;
        engine_sscanf(m_token, "(%d %d)", &x, &y);
        if (!m_10d && !dupTag) {
            CButeValue *node = (CButeValue *)operator new(8);
            if (node) {
                node->type = 6;
                int *p = (int *)operator new(8);
                if (p) {
                    p[0] = x; p[1] = y;
                    node->pValue = p;
                } else {
                    node->pValue = NULL;
                }
            }
            ((CButeTree *)m_pNode)->Insert((const char *)m_curTag, node);
        }
        break;
    }
    case 13: {
        // --- VECTOR3 (type 7), 3 doubles ---
        double vx = 0.0, vy = 0.0, vz = 0.0;
        engine_sscanf(m_token, "<%lf %lf %lf>", &vx, &vy, &vz);
        if (!m_10d && !dupTag) {
            CButeValue *node = (CButeValue *)operator new(8);
            if (node) {
                node->type = 7;
                double *p = (double *)operator new(24);
                if (p) {
                    p[0] = vx; p[1] = vy; p[2] = vz;
                    node->pValue = p;
                } else {
                    node->pValue = NULL;
                }
            }
            ((CButeTree *)m_pNode)->Insert((const char *)m_curTag, node);
        }
        break;
    }
    case 14: {
        // --- VECTOR2 (type 8), 2 doubles ---
        double rx = 0.0, ry = 0.0;
        engine_sscanf(m_token, "[%lf %lf]", &rx, &ry);
        if (!m_10d && !dupTag) {
            CButeValue *node = (CButeValue *)operator new(8);
            if (node) {
                node->type = 8;
                double *p = (double *)operator new(16);
                if (p) {
                    p[0] = rx; p[1] = ry;
                    node->pValue = p;
                } else {
                    node->pValue = NULL;
                }
            }
            ((CButeTree *)m_pNode)->Insert((const char *)m_curTag, node);
        }
        break;
    }
    case 15:
    case 16: {
        // --- STRING (type 4) ---
        if (!m_10d && !dupTag) {
            // Local AfxString temp to trigger the /GX EH frame
            AfxString localStr(m_token);
            CButeValue *node = (CButeValue *)operator new(8);
            if (node) {
                node->type = 4;
                // Allocate 4 bytes for the CString pointer
                AfxString *pStr = (AfxString *)operator new(4);
                if (pStr) {
                    *pStr = (const char *)localStr;
                    node->pValue = pStr;
                } else {
                    node->pValue = NULL;
                }
            }
            ((CButeTree *)m_pNode)->Insert((const char *)m_curTag, node);
        }
        break;
    }
    default:
        CButeMgr_ReportError(this, s_fmtInvalidToken, m_lineNo);
        return false;
    }

    if (m_10d)
        m_10c = 1;

    return true;
}

// ===========================================================================
// CButeMgr::ParseBody  @ 0x171160 (69 B, ret).
// ===========================================================================
// Section-body loop.  Calls ParseAttributeFile to consume one key=value entry,
// then loops calling Parse() for subsequent tokens until it hits a value token
// (type 1 or 2, signifying end-of-section) or a sub-section (type 4, handled
// recursively).  Returns true on section end, false on error.
// @address: 0x171160
// @size:    0x45
bool CButeMgr::ParseBody()
{
    if (!ParseAttributeFile())
        return false;

    for (;;) {
        if (!Parse())
            return false;

        if (m_tokType == 1 || m_tokType == 2)
            return true;

        if (m_tokType != 4)
            return false;

        if (!ParseAttributeFile())
            return false;
    }
}

// ===========================================================================
// CButeMgr::ParseTagLine  @ 0x1711b0 (245 B, /GX).
// ===========================================================================
// Reads one "tag = value" line: ScanToken(4) for the tag name, copies it into the
// active-tag CString (m_tagName), and -- unless m_10d suppresses it -- checks for a
// duplicate tag in the store (reporting + bailing if found), else allocates a new
// store node, constructs it, wires its two vtables, records it (m_pNode) and
// inserts it under the tag name; finally ScanToken(3) consumes the value. The new
// node lives under a C++ EH frame (freed on unwind if the ctor throws) -> /GX.
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

// ===========================================================================
// CButeMgr::InvokeCallback  @ 0x171550 (17 B, ret 4).
// ===========================================================================
// Simple trampoline: takes a function pointer, calls it with `this`, returns `this`.
// @address: 0x171550
// @size:    0x11
void *CButeMgr::InvokeCallback(void *(*fn)(CButeMgr *))
{
    fn(this);
    return this;
}

// ===========================================================================
// CButeMgr::ParseAttributeBlock  @ 0x171580 (186 B, ret).
// ===========================================================================
// Reads attribute blocks: NextChar + Parse, then loops through tokens:
//   tokType 1 -> return true (value/end-of-block)
//   tokType 2 -> parse tag=value via ParseTagLine (with optional write-back
//                to the +0x48 sub-tree when m_10d is set)
//   tokType 4 -> recurse via ParseBody
//   otherwise -> return false
// @address: 0x171580
// @size:    0xba
bool CButeMgr::ParseAttributeBlock()
{
    NextChar();
    if (!Parse())
        return false;

    if (m_tokType == 1)
        return true;

    if (m_tokType != 2)
        return false;

    for (;;) {
        if (!ParseTagLine())
            return false;

        if (m_10d) {
            CButeTree *sub = SubTree();
            void *found = sub->Find((const char *)m_tagName);
            if (found) {
                // The target has an engine call here (@0x193340) that inserts
                // text-buffer content into the found node in write-back mode.
            }
        }

        if (!Parse())
            return false;

        if (m_tokType == 1)
            return true;

        if (m_tokType == 2)
            continue;

        if (m_tokType != 4)
            return false;

        if (!ParseBody())
            return false;

        if (m_tokType == 1)
            return true;
    }
}

// ===========================================================================
// CButeMgr::ClearHelper  @ 0x171a40 (20 B, ret).
// ===========================================================================
// Calls two cleanup methods on the engine helper object at this+0x14.
// @address: 0x171a40
// @size:    0x14
void CButeMgr::ClearHelper()
{
    CButeMgrHelper *h = (CButeMgrHelper *)((char *)this + 0x14);
    h->FuncA();
    h->FuncB();
}

// ===========================================================================
// CButeMgr::IsKey  @ 0x171a60 (52 B, ret 8).
// ===========================================================================
// Two-level key existence check.  Tree()->Find(tag); if found and key
// is NULL, returns true.  If key is non-NULL, Find(key) on the tag sub-tree
// and returns true if key exists.  Manually offset this to force add ecx,0x18.
// @address: 0x171a60
// @size:    0x34
bool CButeMgr::IsKey(char *tag, char *key)
{
    void *grp;
    grp = ((CButeTree *)((char *)this + 0x18))->Find(tag);
    if (grp) {
        if (key) {
            if (((CButeTree *)grp)->Find(key))
                return true;
        } else {
            return true;
        }
    }
    return false;
}

// ===========================================================================
// CButeMgr::GetIntDef  @ 0x171aa0 (80 B, ret c).
// ===========================================================================
// type-0 (int) getter with a caller default: Find(tag).Find(key); on a type-0
// hit return *(int*)rec->pValue, on type mismatch report + fall through, on any
// miss return def.
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

// ===========================================================================
// CButeMgr::GetInt  @ 0x171af0 (134 B, ret 8).
// ===========================================================================
// type-0 getter, no default: returns 0x80000000 on any miss (and reports the
// specific failure - type mismatch / symbol-not-found / invalid-tag).
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

// ===========================================================================
// CButeValue::SetDword  @ 0x172000 (49 B, ret 8).
// ===========================================================================
// Allocates 4-byte storage, stores the value, sets the type field.
// Returns `this` (or NULL on alloc failure, though the target code always
// returns `this` with pValue reset to NULL).
// @address: 0x172000
// @size:    0x31
CButeValue *CButeValue::SetDword(int type, unsigned long val)
{
    this->type = type;
    unsigned long *p = new unsigned long;
    if (p) {
        *p = val;
        this->pValue = p;
    } else {
        this->pValue = NULL;
    }
    return this;
}

// ===========================================================================
// CButeValue::Assign  @ 0x172040 (252 B, ret 4).
// ===========================================================================
// Copies the stored value from another CButeValue.  Dispatches on this->type:
//   type 0: 4-byte scalar (int)
//   type 1: 8-byte scalar (2 dwords)
//   type 2: 4-byte scalar (float)
//   type 3: CString (operator=)
//   type 4: 16-byte rect (4 ints)
//   type 5: 8-byte point (2 ints)
//   type 6: 24-byte vector3
//   type 7: 16-byte vector2
//   type 8+: no-op
// @address: 0x172040
// @size:    0xfc
void CButeValue::Assign(CButeValue *other)
{
    switch (type) {
    case 0:
        *(int *)pValue = *(int *)other->pValue;
        break;
    case 1:
        ((int *)pValue)[0] = ((int *)other->pValue)[0];
        ((int *)pValue)[1] = ((int *)other->pValue)[1];
        break;
    case 2:
        *(int *)pValue = *(int *)other->pValue;
        break;
    case 3:
        ((AfxString *)pValue)->operator=((const char *)other->pValue);
        break;
    case 4:
        ((int *)pValue)[0] = ((int *)other->pValue)[0];
        ((int *)pValue)[1] = ((int *)other->pValue)[1];
        ((int *)pValue)[2] = ((int *)other->pValue)[2];
        ((int *)pValue)[3] = ((int *)other->pValue)[3];
        break;
    case 5:
        ((int *)pValue)[0] = ((int *)other->pValue)[0];
        ((int *)pValue)[1] = ((int *)other->pValue)[1];
        break;
    case 6: {
        int *d = (int *)pValue;
        int *s = (int *)other->pValue;
        d[0] = s[0]; d[1] = s[1]; d[2] = s[2];
        d[3] = s[3]; d[4] = s[4]; d[5] = s[5];
        break;
    }
    case 7:
        ((int *)pValue)[0] = ((int *)other->pValue)[0];
        ((int *)pValue)[1] = ((int *)other->pValue)[1];
        ((int *)pValue)[2] = ((int *)other->pValue)[2];
        ((int *)pValue)[3] = ((int *)other->pValue)[3];
        break;
    }
}

// ===========================================================================
// CButeValue::Free  @ 0x172160 (82 B, ret).
// ===========================================================================
// Frees the stored value.  Type 0 (string/CString) calls the CString
// destructor before freeing.  Types 1..8 just free the raw allocation.
// Types > 8 (or out of range) are no-ops.
// @address: 0x172160
// @size:    0x52
void CButeValue::Free()
{
    switch (type) {
    case 0:
        if (pValue) {
            ((AfxString *)pValue)->~AfxString();
            operator delete(pValue);
        }
        break;
    case 1:
        operator delete(pValue);
        break;
    case 2:
        operator delete(pValue);
        break;
    case 3:
        operator delete(pValue);
        break;
    case 4:
        operator delete(pValue);
        break;
    case 5:
        operator delete(pValue);
        break;
    case 6:
        operator delete(pValue);
        break;
    case 7:
        operator delete(pValue);
        break;
    case 8:
        operator delete(pValue);
        break;
    }
}

// ===========================================================================
// CButeMgr::GetDwordDef  @ 0x1721e0 (90 B, ret c).
// ===========================================================================
// type-1 (dword) getter with default. The type check is `if (--type == 0)` i.e.
// type == 1 (the disasm `mov ecx,[eax]; dec ecx; je`).
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

// ===========================================================================
// CButeMgr::GetDword  @ 0x172240 (125 B, ret 8).
// ===========================================================================
// type-1 getter, no default: returns 0 on any miss.
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

// ===========================================================================
// CButeValue::SetFloat  @ 0x172680 (49 B, ret 8).
// ===========================================================================
// Allocates 4-byte float storage, stores the value.
// @address: 0x172680
// @size:    0x31
CButeValue *CButeValue::SetFloat(int type, float val)
{
    this->type = type;
    float *p = new float;
    if (p) {
        *p = val;
        this->pValue = p;
    } else {
        this->pValue = NULL;
    }
    return this;
}

// ===========================================================================
// CButeMgr::GetFloat  @ 0x172730 (154 B, ret 8).
// ===========================================================================
// type-3 (float) getter, no default. Accepts type 0 (int) too: `fild` the int
// when type==0, `fld` the float when type==3, else report + return 0.0f. Returns
// in st(0) (an x87 float return).
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

// ===========================================================================
// CButeValue::SetInt  @ 0x172b90 (49 B, ret 8).
// ===========================================================================
// Allocates 4-byte int storage, stores the value.
// @address: 0x172b90
// @size:    0x31
CButeValue *CButeValue::SetInt(int type, int val)
{
    this->type = type;
    int *p = new int;
    if (p) {
        *p = val;
        this->pValue = p;
    } else {
        this->pValue = NULL;
    }
    return this;
}

// ===========================================================================
// CButeMgr::GetDouble  @ 0x172c40 (155 B, ret 8).
// ===========================================================================
// type-2 (double) getter, no default. Accepts type 0 (int): `fild` when type==0,
// `fld qword` when type==2, else report + return 0.0. (The disasm tests with
// `sub ecx,0; je` then `sub ecx,2; je` -> the type==0 and type==2 branches.)
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

// ===========================================================================
// CButeValue::SetDouble  @ 0x173140 (56 B, ret c).
// ===========================================================================
// Allocates 8-byte double storage, stores the value.  Returns `this`.
// @address: 0x173140
// @size:    0x38
CButeValue *CButeValue::SetDouble(int type, double val)
{
    this->type = type;
    double *p = new double;
    if (p) {
        *p = val;
        this->pValue = p;
    } else {
        this->pValue = NULL;
    }
    return this;
}

// ===========================================================================
// CButeMgr::GetStringDef  @ 0x173180 (78 B, ret c).
// ===========================================================================
// type-4 (string) getter with default: returns rec->pValue (the char*) on a
// type-4 hit, reports a type mismatch otherwise, returns def on any miss.
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

// ===========================================================================
// CButeMgr::GetString  @ 0x1731d0 (182 B, ret 8).
// ===========================================================================
// type-4 getter, no default: returns rec->pValue on a hit, else reports the
// specific failure and returns a shared empty CString. The empty string is a
// function-local static CString (MFC magic-static: one-shot guarded ctor +
// atexit-registered dtor) returned by address on every error path.
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
