// ButeMgr.h - CButeMgr, the engine's `.att`/`.bute` attribute config-parser.
// This is the layer the whole game reads entity stats through: a two-level
// keyed store (tag-group -> key -> typed value) populated by a recursive-descent
// lexer/parser over the .att text, queried by a family of typed getters.
//
// Minimal Monolith-faithful reconstruction sufficient to byte-match the leaf
// getters + parser. Field names recovered from how every getter/parser body
// reads/writes them; only the OFFSETS + code bytes are load-bearing (campaign
// doctrine). Fields whose role stays unprovable keep the m_<hexoffset> form.
//
//   +0x00  m_streamBase : int  - stream base offset NextChar subtracts.
//   +0x04  m_pos        : int  - running parse cursor position.
//   +0x08  m_lineNo     : int  - current source line (the `%d` in error msgs).
//   +0x0c  m_countLine  : char - bump m_lineNo on the next char (set after '\n').
//   +0x18  m_tree       : the parsed store root (CButeTree). The getters do
//                         `lea ecx,[this+0x18]; Find(tag)` for the tag-group,
//                         then `Find(key)` on it for the typed value record.
//   +0x44  m_pNode      : void* - last-created store node (set by ParseTagLine).
//   +0xa4  m_pText      : ptr to a value-text accumulator host; the parser
//                         appends to its +0xc CString (CString::operator+=).
//   +0xa8  m_curChar    : char  - the lexer's current character.
//   +0xaa  m_tokType    : short - the token type returned by the tokenizer.
//   +0xac  m_lexState   : short - transition-table secondary state output.
//   +0xae  m_token[]    : char  - the current token text buffer (indexed by the
//                         file-scope token-length counter).
//   +0x100 m_tagName    : CString - the active tag name (ParseTagLine copies the
//                         token buffer here).
//   +0x10c m_captureText: char  - gate appending value text to the accumulator.
#ifndef SRC_BUTE_BUTEMGR_H
#define SRC_BUTE_BUTEMGR_H

#include <Ints.h>

// CString (+ CObject etc.) and the Win32 DWORD come from <Mfc.h>; pulled up here
// so the class below can use both. (afx.h is the period-correct windows.h path.)
#include <Gruntz/CString.h>

// The CButeValue::type discriminant. Recovered from the type-tag each getter
// compares against (GetInt==0, GetDword==1, GetDouble==2, GetFloat==3,
// GetString==4) and the typed-reference getters (GetRef5..8). The numeric values
// are load-bearing (the exact immediates the getters cmp); the enum only names
// them. Kept as an `int`-width enum so it is interchangeable with the `i32 type`
// field/params at /O2.
enum ButeType {
    kButeInt = 0,    // stored int      (GetInt/GetIntDef)
    kButeDword = 1,  // stored DWORD    (GetDword/GetDwordDef)
    kButeDouble = 2, // stored double   (GetDouble)
    kButeFloat = 3,  // stored float    (GetFloat)
    kButeString = 4, // stored char*    (GetString/GetStringDef)
    kButeRef5 = 5,   // 16-byte struct  (GetRef5)
    kButeRef6 = 6,   // 8-byte struct   (GetRef6)
    kButeRef7 = 7,   // 24-byte struct  (GetRef7)
    kButeRef8 = 8,   // 16-byte struct  (GetRef8)
};

// ---------------------------------------------------------------------------
// CButeTree - the keyed store node. The getters/parser reach it through a single
// __thiscall lookup helper that, given a key string, returns
// the matching child record (or null). The store is two-level: the outer tree
// (CButeMgr::m_tree) maps a tag name to a per-tag sub-tree; the sub-tree maps a
// key name to a typed value record. Find() is modeled returning the record, and
// the typed getters interpret it:
//   +0x00  type   : ButeType - the value's type-tag (see enum above).
//   +0x04  pValue : void* - pointer to the stored value (the int/dword/float/
//                           double sits at [pValue]; for strings the char* IS
//                           [pValue] -- GetString returns it directly).
// Insert adds a (key,node) pair; the node ctor + its two
// vtable stores are modeled as external/no-body calls (reloc-masked).
// ---------------------------------------------------------------------------
struct CButeValue {
    i32 type;     // +0x00  ButeType discriminant (kept i32-width for the ABI)
    void* pValue; // +0x04

    // Value constructors: allocate storage, store the value, return `this`.
    CButeValue* SetInt(i32 type, i32 val);
    CButeValue* SetDword(i32 type, u32 val);
    CButeValue* SetFloat(i32 type, float val);
    CButeValue* SetDouble(i32 type, double val);

    // CopyValue (@0x172040): copy `other`'s payload into this value's storage,
    // sized by THIS value's type-tag (a jump-table switch over ButeType). Returns
    // this.
    CButeValue* CopyValue(CButeValue* other);
};

// The 24-byte (kButeRef7) payload, copied as a struct so MSVC lowers it to the
// retail `rep movsd` (6 dwords).
struct ButeRef24 {
    i32 w[6];
};

// The engine helper embedded at CButeMgr+0x14 (the .bute compiler/registry
// sub-object). FuncA/FuncB are the cleanup pair ClearHelper drives (reloc-masked
// __thiscall externs, no body). The rest is the constructor + the sub-object
// setter (SetSub) + the virtual-base vtable-init thunks reconstructed below.
//
// Layout recovered from the ctor's field-init list (0x169c00):
//   +0x00  m_vptr    : vptr (0x5f03bc, shared with the dtor's vtable restore)
//   +0x04  m_pSub    : void* - a sub-object pointer (the dtor/Set delete-and-
//                      replace target; its slot-0 is a scalar deleting dtor)
//   +0x08  m_flags   : int   - flag word (Set toggles bit 0x4 from sub nullness)
//   +0x0c  m_0c      : int   - zero-init
//   +0x10  m_10      : int   - zero-init
//   +0x14  m_14[8]   : padding to +0x1c
//   +0x1c  m_ownsSub : int   - "owns sub-object" guard (Set checks it)
//   +0x20  m_20      : int   - zero-init
//   +0x24  m_24      : int   - zero-init
//   +0x28  m_28      : int   - 6
//   +0x2c  m_2c      : char  - 0x20
//   +0x30  m_30      : int   - zero-init
//   +0x34  m_34      : int   - -1
//   +0x38  m_cs      : CRITICAL_SECTION (per-instance, Initialize/Delete'd)
class CButeMgrHelper {
public:
    void FuncA();
    void FuncB();

    // The constructor: zero-init the fields, stamp the vptr, set the constants,
    // init the per-instance critical section, and one-time-init the shared
    // critical section under a ref-count guard.
    CButeMgrHelper* Construct();
    // Replace the sub-object at +0x4 (delete the old one through its vtable when
    // owned), then toggle bit 0x4 of m_flags from the new pointer's nullness.
    void SetSub(void* p);
    // The virtual-base vtable-init thunks (compiler vbase ctor closures): each
    // stamps a virtual-base subobject's vptr through the this-relative vbtable;
    // the A/B pair tail into the C/D ret-only thunks.
    void InitVbaseA();
    void InitVbaseB();
    void InitVbaseC();
    void InitVbaseD();

    void* m_vptr;              // +0x00
    void* m_pSub;              // +0x04
    i32 m_flags;               // +0x08  flag word (SetSub toggles bit 0x4)
    i32 m_0c;                  // +0x0c
    i32 m_10;                  // +0x10
    char m_pad14[0x1c - 0x14]; // +0x14
    i32 m_ownsSub;             // +0x1c  "owns sub-object" guard (SetSub checks it)
    i32 m_20;                  // +0x20
    i32 m_24;                  // +0x24
    i32 m_28;                  // +0x28
    char m_2c;                 // +0x2c
    char m_pad2d[0x30 - 0x2d]; // +0x2d
    i32 m_30;                  // +0x30
    i32 m_34;                  // +0x34
    CRITICAL_SECTION m_cs;     // +0x38
};

class CButeTree {
public:
    // The shared find-by-key helper (__thiscall).
    void* Find(const char* key);
    // Insert a key/value node (__thiscall).
    void Insert(const char* key, void* pNode);
    // Apply a callback to each matching node (__thiscall: push flag/ctx/fn,
    // callee-cleanup). Reloc-masked external/no-body.
    void Walk(void (*fn)(), void* ctx, i32 flag);
};

// ---------------------------------------------------------------------------
// CButeStore - one owned keyed-store sub-tree (CButeMgr's m_tree / m_tree48 /
// m_tree74). 0x2c bytes, multiply-derived: a base store at +0 plus a second base
// at +8, both polymorphic (two vptrs). The mgr's /GX destructor tears each one
// down inline at its own trylevel; the member destructor reproduces that
// multiply-derived teardown:
//   (1) re-stamp the two most-derived vptrs (g_storeVtblA @+0, g_storeVtblB @+8),
//   (2) run the derived clear body (recursive node-free, __thiscall(this, 0)),
//   (3) restore the second base's vptr (__thiscall on the masked `this+8`,
//       `neg ecx; sbb ecx,ecx; and ecx,ebx` second-base-this adjust),
//   (4) run the primary-base destructor (__thiscall(this)).
// Only the SIZE + the two-vptr teardown are load-bearing; the interior is opaque.
// The two vptr values are reloc-masked externals; the three teardown callees are
// __thiscall engine functions (no body) so `mov ecx,this; call` falls out masked.
extern "C" void g_storeVtblA(); // 0x5e94ac  most-derived (scalar-deleting) vptr
extern "C" void g_storeVtblB(); // 0x5e949c  second-base vptr

// The second-base subobject at +0x8: its vptr restore (0x16dfc0) is a __thiscall
// `mov [ecx],<vtbl>; ret`. Modeled as a tiny receiver so the call lands the
// masked `this+8` pointer in ecx with no caller-side cleanup.
struct CButeStoreBase2 {
    void RestoreVptr(); // 0x16dfc0
};

struct CButeStore {
    void* m_vptrA;            // +0x00  most-derived vptr
    char m_pad04[4];          // +0x04
    void* m_vptrB;            // +0x08  second-base vptr
    char m_pad0c[0x2c - 0xc]; // +0x0c  opaque keyed-store interior
    // The derived clear body (0x16e070): recursively frees the keyed nodes.
    // __thiscall(this, recurse); callee-cleans 4.
    void ClearRecursive(i32 recurse);
    // The primary-base destructor (0x16da60): restore vptr + tear down [this+4].
    void BaseDtor(); // __thiscall(this)
    ~CButeStore() {
        m_vptrA = (void*)&g_storeVtblA;
        m_vptrB = (void*)&g_storeVtblB;
        ClearRecursive(0);
        // The second base lives at this+8; the compiler null-masks the adjust
        // (`neg ecx; sbb ecx,ecx; and ecx,ebx`) -> RestoreVptr on (this?this+8:0).
        CButeStoreBase2* b2 = this ? (CButeStoreBase2*)((char*)this + 8) : 0;
        b2->RestoreVptr();
        BaseDtor();
    }
};

// The 1-byte embedded object at CButeMgr+0x10f. Its destructor (0x16f6b0) is a
// bare `ret` (trivial teardown), but it is a NON-trivial member of the mgr (the
// dtor schedules a trylevel slot for it). Modeled as a value member with an
// external (no-body) member dtor so the mgr's /GX teardown emits its slot.
struct CButeTail {
    char m_00;   // +0x00
    void Dtor(); // 0x16f6b0  (ret)
    ~CButeTail() {
        Dtor();
    }
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
    CButeNodeBase(void* desc, i32 n);
};

class CButeNode : public CButeNodeBase {
public:
    // Inline derived ctor: run the engine base ctor, then write the two derived
    // vtable pointers at +0x00 / +0x08 (reproduces ParseTagLine's inline
    // `call ctor; mov [node],vtblA; mov [node+8],vtblB`).
    CButeNode(void* desc, i32 n) : CButeNodeBase(desc, n) {
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
extern "C" i32 atexit(void (*func)(void));

// The optional error callback CButeMgr::ReportError fires after formatting the
// message (cdecl, takes the formatted C string).
typedef void(__cdecl* ErrCallback)(const char*);

// ---------------------------------------------------------------------------
// CButeMgr - the attribute manager.
// ---------------------------------------------------------------------------
class CButeMgr {
public:
    i32 GetIntDef(char* tag, char* key, i32 def);
    i32 GetInt(char* tag, char* key);
    DWORD GetDwordDef(char* tag, char* key, DWORD def);
    DWORD GetDword(char* tag, char* key);
    float GetFloat(char* tag, char* key);
    double GetDouble(char* tag, char* key);
    CString* GetStringDef(char* tag, char* key, CString* def);
    char* GetString(char* tag, char* key);

    bool ScanToken(i32 expectType);
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
    i16 PeekClass(i32 kind, char c);
    i32 ReadValue(i32 kind, char c);
    i32 ReadIdent(i32 kind, char c);

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
    i16 CharClass(char c);
    // Two adjacent columns of the lexer transition table (state x class):
    // PeekState classifies the current state/char; ScanState writes the token
    // type (+0xaa) and the secondary state (+0xac).
    i16 PeekState(i16 state, char c);
    i16 PeekState2(i16 state, char c);
    void ScanState(i16 state, char c);
    // Outer tag-skip loop: re-lex until a tag/group token (1/2), or fail.
    bool SkipToTag();
    // Recursive group parser (the per-tag descent).
    bool ParseGroup();
    // Key existence probe: Find(tag); if no key requested, hit on the group;
    // else require the key under it.
    bool Exists(char* tag, char* key);

    // The /GX (EH-frame) scalar destructor (0x213c0): tears down the three owned
    // CButeStore sub-trees + the five CStrings + the +0x10f tail object, each at
    // its own descending trylevel (reverse declaration order).
    ~CButeMgr();

    // Accessor for the +0x18 store tree (CButeTree is data-less; address the store
    // member so its `this` resolves to `this+0x18` -> `lea ecx,[esi+0x18]`).
    CButeTree* Tree() {
        return reinterpret_cast<CButeTree*>(&m_tree);
    }
    // The second keyed sub-tree at +0x48 (ParseGroup reaches it).
    CButeTree* Tree48() {
        return reinterpret_cast<CButeTree*>(&m_tree48);
    }

    i32 m_streamBase;             // +0x00  stream base offset (NextChar's `- m_00`)
    i32 m_pos;                    // +0x04  running parse cursor position
    i32 m_lineNo;                 // +0x08
    char m_countLine;             // +0x0c  bump m_lineNo on the next char (set after \n)
    char m_0d;                    // +0x0d  (role unproven - cleared by Init only)
    char m_pad0e[0x10 - 0xe];     // +0x0e
    CString m_errStr;             // +0x10  scratch the error reporter formats into
    ErrCallback m_errCallback;    // +0x14  optional error-callback fn-ptr
    CButeStore m_tree;            // +0x18  the keyed store root (0x2c bytes)
    void* m_pNode;                // +0x44
    CButeStore m_tree48;          // +0x48  second store sub-tree
    CButeStore m_tree74;          // +0x74  third store sub-tree
    void* m_stream;               // +0xa0  the input source stream object
    struct CButeTextBuf* m_pText; // +0xa4  -> value-text accumulator host (+0xc)
    char m_curChar;               // +0xa8
    char m_pada9;                 // +0xa9
    i16 m_tokType;                // +0xaa
    i16 m_lexState;               // +0xac  transition-table secondary state output
    char m_token[0x100 - 0xae];   // +0xae
    CString m_tagName;            // +0x100
    CString m_str104;             // +0x104  second scratch string
    CString m_str108;             // +0x108  third scratch string
    char m_captureText;           // +0x10c  capture value text into m_pText accumulator
    char m_10d;                   // +0x10d  build-suppress / group-walk mode (role unproven)
    char m_10e;                   // +0x10e
    CButeTail m_10f;              // +0x10f  1-byte embedded object (non-trivial dtor)

    // The typed-reference getters (tag5-8). Each returns a pointer to the typed
    // value record's storage on a type hit, or a shared zero-default static on
    // any miss (no tag / no key / type mismatch), reporting the specific failure.
    CButeRef5* GetRef5(char* tag, char* key);
    CButeRef6* GetRef6(char* tag, char* key);
    CButeRef7* GetRef7(char* tag, char* key);
    CButeRef8* GetRef8(char* tag, char* key);
};

#endif // SRC_BUTE_BUTEMGR_H
