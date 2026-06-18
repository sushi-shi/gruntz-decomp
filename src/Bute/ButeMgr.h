// ButeMgr.h - CButeMgr, the engine's `.att`/`.bute` attribute config-parser.
// This is the layer the whole game reads entity stats through: a two-level
// keyed store (tag-group -> key -> typed value) populated by a recursive-descent
// lexer/parser over the .att text, queried by a family of typed getters.
//
// Minimal Monolith-faithful reconstruction sufficient to byte-match the leaf
// getters + parser. Field names are placeholders (m_<hexoffset>); only the
// OFFSETS + code bytes are load-bearing (campaign doctrine). Recovered from the
// getter/parser bodies (scripts/dump_target.py):
//
//   +0x08  m_lineNo  : int     - current source line (the `%d` in error msgs).
//   +0x10  m_errBuf  : CString - error-message buffer (used by ReportError).
//   +0x14  m_pErrCb  : void*   - error callback (ReportError calls through it).
//   +0x18  m_tree    : the parsed store root (CButeTree). The getters do
//                        `lea ecx,[this+0x18]; Find(tag)` to get the tag-group,
//                        then `Find(key)` on it to get the typed value record.
//   +0x44  m_pNode   : void*   - last-created store node (set by ParseTagLine).
//   +0xa4  m_pText   : ptr to a CString-ish text accumulator; the parser appends
//                        the current char at +0xc inside it (CString::operator+=).
//   +0xa8  m_curChar : char    - the lexer's current character.
//   +0xaa  m_tokType : short   - the token type returned by the tokenizer.
//   +0xae  m_token[] : char    - the current token text buffer (indexed by the
//                        file-scope token-length counter).
//   +0x100 m_tagName : CString - the active tag name (ParseTagLine copies the
//                        token buffer here).
//   +0x104 m_curTag  : CString - the current tag being processed (used by
//                        ParseAttributeFile as the tree Insertion key).
//   +0x10c m_10c     : char    - parser flag byte.
//   +0x10d m_10d     : char    - "skip duplicate-tag check" flag byte.
#ifndef SRC_BUTE_BUTEMGR_H
#define SRC_BUTE_BUTEMGR_H

typedef unsigned long DWORD;

// ---------------------------------------------------------------------------
// CButeTree - the keyed store node. The getters/parser reach it through a single
// __thiscall lookup helper (engine @0x16d190) that, given a key string, returns
// the matching child record (or null). The store is two-level: the outer tree
// (CButeMgr::m_tree) maps a tag name to a per-tag sub-tree; the sub-tree maps a
// key name to a typed value record. Find() is modeled returning the record, and
// the typed getters interpret it:
//   +0x00  type   : int   - the value's type-tag (0=int, 1=dword, 2=double,
//                           3=float, 4=string, ...).
//   +0x04  pValue : void* - pointer to the stored value (the int/dword/float/
//                           double sits at [pValue]; for strings the char* IS
//                           [pValue] -- GetString returns it directly).
// Insert (@0x16db90) adds a (key,node) pair; the node ctor (@0x16dff0) + its two
// vtable stores are modeled as external/no-body calls (reloc-masked).
// ---------------------------------------------------------------------------
struct CButeValue {
    int   type;     // +0x00
    void *pValue;   // +0x04

    // Value constructors (allocate storage and store value).
    // Each returns `this` (for chaining). The 4-byte variants are three identical
    // overloads (int/DWORD/float all take 4 bytes on stack).
    CButeValue *SetInt(int type, int val);
    CButeValue *SetDword(int type, unsigned long val);
    CButeValue *SetFloat(int type, float val);
    CButeValue *SetDouble(int type, double val);

    // Copy the stored value from another CButeValue (dispatches on this->type).
    void Assign(CButeValue *other);

    // Free the stored value (type-dependent: calls CString dtor for type 0 values,
    // plain delete for all others).
    void Free();
};

class CButeTree {
public:
    // The shared find-by-key helper (@0x16d190, __thiscall ret 4).
    void *Find(const char *key);
    // Insert a key/value node (@0x16db90, __thiscall).
    void  Insert(const char *key, void *pNode);
};

// ---------------------------------------------------------------------------
// CButeNode - a per-tag store node allocated by ParseTagLine (0x2c bytes). Built
// via `new CButeNode(2, descriptor)`: the engine ctor (@0x16dff0) runs, then the
// derived class's two vtable pointers are written inline at +0x00 / +0x08 (the
// 0x5f051c / 0x5f0518 vtables -- a multiply-derived node with two vptrs). Modeled
// with an external (no-body) ctor; the vtable stores are emitted by the source
// `new` expression and are reloc-masked.
// ---------------------------------------------------------------------------
// The two derived vtables the node carries (@0x5f051c / @0x5f0518). External/
// reloc-masked file-scope addresses.
extern void *g_nodeVtblA;   // 0x5f051c
extern void *g_nodeVtblB;   // 0x5f0518

// CButeNodeBase - the engine base subobject ctor (@0x16dff0, __thiscall(this,
// desc, n)). Declared external/no-body so the `mov ecx,this; call` __thiscall
// shape (callee-cleanup) falls out reloc-masked.
class CButeNodeBase {
public:
    CButeNodeBase(void *desc, int n);   // @0x16dff0
};

class CButeNode : public CButeNodeBase {
public:
    // Inline derived ctor: run the engine base ctor, then write the two derived
    // vtable pointers at +0x00 / +0x08 (reproduces ParseTagLine's inline
    // `call ctor; mov [node],vtblA; mov [node+8],vtblB`).
    CButeNode(void *desc, int n) : CButeNodeBase(desc, n) {
        m_vtblA = &g_nodeVtblA;
        m_vtblB = &g_nodeVtblB;
    }
    void *m_vtblA;       // +0x00  (0x5f051c)
    char  m_pad04[4];    // +0x04
    void *m_vtblB;       // +0x08  (0x5f0518)
    char  m_pad0c[0x2c - 0xc]; // pad to 0x2c bytes total
};

// CString::operator+= one char (engine @0x192060). Appends to the accumulator.
extern "C" void AfxString_AppendChar(void *pStr, char c);

// ---------------------------------------------------------------------------
// CString member helper - an MFC-style CString (a single char* @+0). Only the
// engine library calls the getters/parser actually emit are modeled, with NO
// body so their `call rel32` displacements are reloc-masked in objdiff:
//   - operator=  (CString::operator=, NAFXCW @0x1b9e74)
//   - the literal-ctor (CString::CString(const char*), @0x1b9d4c) used by the
//     one-shot default-string init in GetString.
// ---------------------------------------------------------------------------
class AfxString {
public:
    AfxString();
    AfxString(const char *src);          // @0x1b9d4c
    const AfxString &operator=(const char *src);  // @0x1b9e74
    operator const char *() const { return m_pchData; }
private:
    char *m_pchData;
};

// CRT helpers (minimal external decls; reloc-masked engine CRT thunks).
extern "C" int atexit(void (*func)(void));

// ---------------------------------------------------------------------------
// Minimal class for the engine helper object embedded at this+0x14.
// The error callback & a cleanup pair operate on it.
// ---------------------------------------------------------------------------
class CButeMgrHelper {
public:
    void FuncA();  // @0x169be0  (engine __thiscall, no body)
    void FuncB();  // @0x169d70  (engine __thiscall, no body)
};

// ---------------------------------------------------------------------------
// CButeMgr - the attribute manager.
// ---------------------------------------------------------------------------
class CButeMgr {
public:
    // --- Getters ---
    int   GetIntDef(char *tag, char *key, int def);
    int   GetInt(char *tag, char *key);
    DWORD GetDwordDef(char *tag, char *key, DWORD def);
    DWORD GetDword(char *tag, char *key);
    float GetFloat(char *tag, char *key);
    double GetDouble(char *tag, char *key);
    char *GetStringDef(char *tag, char *key, char *def);
    char *GetString(char *tag, char *key);

    // --- Parser/lexer members ---
    bool  ScanToken(int expectType);
    bool  ParseTagLine();
    bool  Parse();

    // --- Attribute-file parser (the big one) ---
    bool  ParseAttributeFile();

    // --- Body/block parsers (internal helpers) ---
    bool  ParseBody();           // ParseAttributeFile loop until tokType 1|2|4
    bool  ParseAttributeBlock(); // Read tag=value lines under a section

    // --- Error helper ---
    void  ReportError(const char *fmt, ...);  // member variant (for body impl)

    // --- Key existence ---
    bool  IsKey(char *tag, char *key);

    // --- Callback trampoline ---
    void *InvokeCallback(void *(*fn)(CButeMgr *));

    // --- Sub-object cleanup ---
    void  ClearHelper();

    // Lexer sub-helpers (engine functions, reloc-masked external/no-body).
    // PeekClass classifies the current char (returns a token-class word);
    // ReadValue/ReadIdent scan a value/identifier token (return the next kind);
    // NextChar advances the input. All __thiscall on CButeMgr.
    short PeekClass(int kind, char c);   // @0x170400
    int   ReadValue(int kind, char c);   // @0x170430
    int   ReadIdent(int kind, char c);   // @0x170460
    void  NextChar();                    // @0x170390

    // Accessor for the +0x18 store tree (CButeTree is data-less; address it by
    // offset so its `this` resolves to `this+0x18` -> `lea ecx,[esi+0x18]`).
    CButeTree *Tree() { return reinterpret_cast<CButeTree *>(m_treeRaw); }

    // Some attribute-file-read helpers reach the +0x48 sub-tree:
    CButeTree *SubTree() { return reinterpret_cast<CButeTree *>(m_subTreeRaw); }

    char  m_pad00[0x8];            // +0x00
    int   m_lineNo;                // +0x08
    char  m_pad0c[0x10 - 0xc];    // +0x0c
    char  m_errBuf[0x14 - 0x10];  // +0x10  CString buffer (ReportError)
    void *m_pErrCb;               // +0x14  error callback
    char  m_treeRaw[0x44 - 0x18]; // +0x18  the CButeTree store root
    void *m_pNode;                // +0x44
    char  m_subTreeRaw[0xa4 - 0x48]; // +0x48  another CButeTree (for ParseAttributeBlock)
    void *m_pText;                // +0xa4
    char  m_curChar;              // +0xa8
    char  m_pada9;                // +0xa9
    short m_tokType;              // +0xaa
    char  m_padac[0xae - 0xac];   // +0xac
    char  m_token[0x100 - 0xae];  // +0xae
    AfxString m_tagName;          // +0x100
    AfxString m_curTag;           // +0x104  current tag key during ParseAttributeFile
    char  m_10c;                  // +0x10c
    char  m_10d;                  // +0x10d
};

// The variadic error reporter free function (the real mangled entry point at 0x1706c0).
int __cdecl CButeMgr_ReportError(CButeMgr *self, const char *fmt, ...);

#endif // SRC_BUTE_BUTEMGR_H
