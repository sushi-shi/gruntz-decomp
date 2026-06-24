// ButeMgr.h - CButeMgr, the engine's `.att`/`.bute` attribute config-parser.
// This is the layer the whole game reads entity stats through: a two-level
// keyed store (tag-group -> key -> typed value) populated by a recursive-descent
// lexer/parser over the .att text, queried by a family of typed getters.
//
// Minimal Monolith-faithful reconstruction sufficient to byte-match the leaf
// getters + parser. Field names are placeholders (m_<hexoffset>); only the
// OFFSETS + code bytes are load-bearing (campaign doctrine). Recovered from the
// getter/parser bodies (python -m gruntz.analysis.dump_target):
//
//   +0x08  m_lineNo  : int     - current source line (the `%d` in error msgs).
//   +0x18  m_tree    : the parsed store root (CButeTree). The getters do
//                      `lea ecx,[this+0x18]; Find(tag)` to get the tag-group,
//                      then `Find(key)` on it to get the typed value record.
//   +0x44  m_pNode   : void*   - last-created store node (set by ParseTagLine).
//   +0xa4  m_pText   : ptr to a CString-ish text accumulator; the parser appends
//                      the current char at +0xc inside it (CString::operator+=).
//   +0xa8  m_curChar : char    - the lexer's current character.
//   +0xaa  m_tokType : short   - the token type returned by the tokenizer.
//   +0xae  m_token[] : char    - the current token text buffer (indexed by the
//                      file-scope token-length counter).
//   +0x100 m_tagName : CString - the active tag name (ParseTagLine copies the
//                      token buffer here).
//   +0x10c m_10c     : char    - parser flag byte.
//   +0x10d m_10d     : char    - "skip duplicate-tag check" flag byte.
#ifndef SRC_BUTE_BUTEMGR_H
#define SRC_BUTE_BUTEMGR_H

// CString (+ CObject etc.) and the Win32 DWORD come from <Mfc.h>; pulled up here
// so the class below can use both. (afx.h is the period-correct windows.h path.)
#include <Gruntz/CString.h>

// ---------------------------------------------------------------------------
// CButeTree - the keyed store node. The getters/parser reach it through a single
// __thiscall lookup helper that, given a key string, returns
// the matching child record (or null). The store is two-level: the outer tree
// (CButeMgr::m_tree) maps a tag name to a per-tag sub-tree; the sub-tree maps a
// key name to a typed value record. Find() is modeled returning the record, and
// the typed getters interpret it:
//   +0x00  type   : int   - the value's type-tag (0=int, 1=dword, 2=double,
//                           3=float, 4=string, ...).
//   +0x04  pValue : void* - pointer to the stored value (the int/dword/float/
//                           double sits at [pValue]; for strings the char* IS
//                           [pValue] -- GetString returns it directly).
// Insert adds a (key,node) pair; the node ctor + its two
// vtable stores are modeled as external/no-body calls (reloc-masked).
// ---------------------------------------------------------------------------
struct CButeValue {
    int type;     // +0x00
    void* pValue; // +0x04

    // Value constructors: allocate storage, store the value, return `this`.
    CButeValue* SetInt(int type, int val);
    CButeValue* SetDword(int type, unsigned long val);
    CButeValue* SetFloat(int type, float val);
    CButeValue* SetDouble(int type, double val);
};

// Minimal engine helper embedded at CButeMgr+0x14; the cleanup pair operate on
// it (reloc-masked __thiscall externs, no body).
class CButeMgrHelper {
public:
    void FuncA();
    void FuncB();
};

class CButeTree {
public:
    // The shared find-by-key helper (__thiscall).
    void* Find(const char* key);
    // Insert a key/value node (__thiscall).
    void Insert(const char* key, void* pNode);
    // Apply a callback to each matching node (__thiscall: push flag/ctx/fn,
    // callee-cleanup). Reloc-masked external/no-body.
    void Walk(void (*fn)(), void* ctx, int flag);
};

// ---------------------------------------------------------------------------
// CButeNode - a per-tag store node allocated by ParseTagLine (0x2c bytes). Built
// via `new CButeNode(2, descriptor)`: the engine ctor runs, then the
// derived class's two vtable pointers are written inline at +0x00 / +0x08
// (a multiply-derived node with two vptrs). Modeled
// with an external (no-body) ctor; the vtable stores are emitted by the source
// `new` expression and are reloc-masked.
// ---------------------------------------------------------------------------
// The two derived vtables the node carries. External/
// reloc-masked file-scope addresses.
extern void* g_nodeVtblA;
extern void* g_nodeVtblB;

// CButeNodeBase - the engine base subobject ctor (__thiscall(this,
// desc, n)). Declared external/no-body so the `mov ecx,this; call` __thiscall
// shape (callee-cleanup) falls out reloc-masked.
class CButeNodeBase {
public:
    CButeNodeBase(void* desc, int n);
};

class CButeNode : public CButeNodeBase {
public:
    // Inline derived ctor: run the engine base ctor, then write the two derived
    // vtable pointers at +0x00 / +0x08 (reproduces ParseTagLine's inline
    // `call ctor; mov [node],vtblA; mov [node+8],vtblB`).
    CButeNode(void* desc, int n) : CButeNodeBase(desc, n) {
        m_vtblA = &g_nodeVtblA;
        m_vtblB = &g_nodeVtblB;
    }
    void* m_vtblA;            // +0x00
    char m_pad04[4];          // +0x04
    void* m_vtblB;            // +0x08
    char m_pad0c[0x2c - 0xc]; // pad to 0x2c bytes total
};

// CString::operator+= one char. Appends to the accumulator.
extern "C" void AfxString_AppendChar(void* pStr, char c);

// ---------------------------------------------------------------------------
// The typed-reference value structs (CButeMgr's GetTypedRef tag5-8 family).
// These getters mirror GetString: a function-local `static T s_default;` is
// returned by address on every error path, and `(T*)rec->pValue` on a type hit.
// Each T's only load-bearing properties are its SIZE (the inline zero-init of
// the static, one `mov [field],0` per DWORD) and that it carries a NON-TRIVIAL
// destructor (so the magic-static emits the atexit dtor-thunk register, exactly
// like GetString's empty CString). Field names are placeholders; the byte count
// is recovered from the static's zero-init store list:
//   tag5 -> 16 bytes (4 DWORDs), tag7 -> 24 bytes (6 DWORDs),
//   tag6 ->  8 bytes (2 DWORDs), tag8 -> 16 bytes (4 DWORDs).
// The default ctor zero-inits inline; the dtor is external/no-body so its thunk
// + the `push thunk; call atexit` shape fall out reloc-masked.
struct CButeRef5 { // 16 bytes
    CButeRef5() : a(0), b(0), c(0), d(0) {}
    ~CButeRef5();
    DWORD a, b, c, d;
};
struct CButeRef6 { // 8 bytes
    CButeRef6() : a(0), b(0) {}
    ~CButeRef6();
    DWORD a, b;
};
struct CButeRef7 { // 24 bytes
    CButeRef7() : a(0), b(0), c(0), d(0), e(0), f(0) {}
    ~CButeRef7();
    DWORD a, b, c, d, e, f;
};
struct CButeRef8 { // 16 bytes
    CButeRef8() : a(0), b(0), c(0), d(0) {}
    ~CButeRef8();
    DWORD a, b, c, d;
};

// ---------------------------------------------------------------------------
// CString member helper - an MFC-style CString (a single char* @+0). Only the
// engine library calls the getters/parser actually emit are modeled, with NO
// body so their `call rel32` displacements are reloc-masked in objdiff:
//   - operator=  (CString::operator=, NAFXCW)
//   - the literal-ctor (CString::CString(const char*)) used by the
//     one-shot default-string init in GetString.
// (CString itself comes from <Gruntz/CString.h>, included at the top.)
// ---------------------------------------------------------------------------

// CRT helpers (minimal external decls; reloc-masked engine CRT thunks).
extern "C" int atexit(void (*func)(void));

// The optional error callback CButeMgr::ReportError fires after formatting the
// message (cdecl, takes the formatted C string).
typedef void(__cdecl* ErrCallback)(const char*);

// ---------------------------------------------------------------------------
// CButeMgr - the attribute manager.
// ---------------------------------------------------------------------------
class CButeMgr {
public:
    int GetIntDef(char* tag, char* key, int def);
    int GetInt(char* tag, char* key);
    DWORD GetDwordDef(char* tag, char* key, DWORD def);
    DWORD GetDword(char* tag, char* key);
    float GetFloat(char* tag, char* key);
    double GetDouble(char* tag, char* key);
    CString* GetStringDef(char* tag, char* key, CString* def);
    char* GetString(char* tag, char* key);

    bool ScanToken(int expectType);
    bool ParseTagLine();
    bool Parse();

    // The variadic error reporter: format `fmt` + varargs into m_errStr, then
    // fire the optional m_errCallback with the message. A variadic member is
    // __cdecl with `this` as the hidden first stack arg (QAA mangling) - exactly
    // the retail ABI (this pushed last at each call site).
    void ReportError(const char* fmt, ...);

    // Callback trampoline + sub-object cleanup.
    void* InvokeCallback(void* (*fn)(CButeMgr*));
    void ClearHelper();

    // Lexer sub-helpers (engine functions, reloc-masked external/no-body).
    // PeekClass classifies the current char (returns a token-class word);
    // ReadValue/ReadIdent scan a value/identifier token (return the next kind);
    // All __thiscall on CButeMgr.
    short PeekClass(int kind, char c);
    int ReadValue(int kind, char c);
    int ReadIdent(int kind, char c);

    // ------------------------------------------------------------------
    // Lexer cluster (0x170330-0x170460, 0x171160-0x171a60). Recovered as
    // real methods on this class; the externals they reach (CButeTree::Find,
    // the recursive tree-walks, the static class/transition tables) are
    // reloc-masked.
    // ------------------------------------------------------------------
    // Reset the line/position counters + clear the two scratch CStrings.
    void Init();
    // Store the optional error callback (+0x14).
    void SetErrCallback(ErrCallback cb);
    // Advance the input one char: pull the next byte from the source stream
    // (+0xa0), update line/position, set m_curChar (+0xa8).
    void NextChar();
    // Map a raw char to its lexer character-class index (g_charClass - 1).
    short CharClass(char c);
    // Two adjacent columns of the lexer transition table (state x class):
    // PeekState classifies the current state/char; ScanState writes the token
    // type (+0xaa) and the secondary state (+0xac).
    short PeekState(short state, char c);
    short PeekState2(short state, char c);
    void ScanState(short state, char c);
    // Outer tag-skip loop: re-lex until a tag/group token (1/2), or fail.
    bool SkipToTag();
    // Recursive group parser (the per-tag descent).
    bool ParseGroup();
    // Key existence probe: Find(tag); if no key requested, hit on the group;
    // else require the key under it.
    bool Exists(char* tag, char* key);

    // Accessor for the +0x18 store tree (CButeTree is data-less; address it by
    // offset so its `this` resolves to `this+0x18` -> `lea ecx,[esi+0x18]`).
    CButeTree* Tree() {
        return reinterpret_cast<CButeTree*>(m_treeRaw);
    }
    // The second keyed sub-tree at +0x48 (ParseGroup reaches it).
    CButeTree* Tree48() {
        return reinterpret_cast<CButeTree*>(m_tree48);
    }

    int m_00;                    // +0x00  source base offset
    int m_04;                    // +0x04  current char position
    int m_lineNo;                // +0x08
    char m_0c;                   // +0x0c  "count this line" flag
    char m_0d;                   // +0x0d
    char m_pad0e[0x10 - 0xe];    // +0x0e
    CString m_errStr;            // +0x10  scratch the error reporter formats into
    ErrCallback m_errCallback;   // +0x14  optional error-callback fn-ptr
    char m_treeRaw[0x44 - 0x18]; // +0x18  the CButeTree store root
    void* m_pNode;               // +0x44
    char m_tree48[0x74 - 0x48];  // +0x48  second store sub-tree
    char m_tree74[0xa0 - 0x74];  // +0x74  third store sub-tree
    void* m_stream;              // +0xa0  the input source stream object
    void* m_pText;               // +0xa4
    char m_curChar;              // +0xa8
    char m_pada9;                // +0xa9
    short m_tokType;             // +0xaa
    short m_ac;                  // +0xac  secondary lexer state
    char m_token[0x100 - 0xae];  // +0xae
    CString m_tagName;           // +0x100
    CString m_str104;            // +0x104  second scratch string
    CString m_str108;            // +0x108  third scratch string
    char m_10c;                  // +0x10c
    char m_10d;                  // +0x10d
    char m_10e;                  // +0x10e
    char m_10f;                  // +0x10f  1-byte embedded object (trivial dtor)

    // The typed-reference getters (tag5-8). Each returns a pointer to the typed
    // value record's storage on a type hit, or a shared zero-default static on
    // any miss (no tag / no key / type mismatch), reporting the specific failure.
    CButeRef5* GetRef5(char* tag, char* key);
    CButeRef6* GetRef6(char* tag, char* key);
    CButeRef7* GetRef7(char* tag, char* key);
    CButeRef8* GetRef8(char* tag, char* key);
};

#endif // SRC_BUTE_BUTEMGR_H
