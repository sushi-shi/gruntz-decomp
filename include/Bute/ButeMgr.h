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
//   +0x44  m_pNode      : CButeTree* - last-created store node (a CButeNode, which
//                         is-a keyed store; ParseAttributeFile Find/Inserts through it).
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

#include <rva.h>
#include <Wap32/ZVec.h>

// CString (+ CObject etc.) and the Win32 DWORD come from <Mfc.h>; pulled up here
// so the class below can use both. (afx.h is the period-correct windows.h path.)
#include <Gruntz/String.h>

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

    // Destructor: free the owned pValue storage, sized/typed by the type-tag via a
    // dense ButeType jump table (string frees the CString first; all others are a
    // plain operator delete).
    ~CButeValue();

    // Value constructors: allocate storage, store the value, return `this`.
    CButeValue* SetInt(i32 type, i32 val);
    CButeValue* SetDword(i32 type, u32 val);
    CButeValue* SetFloat(i32 type, float val);
    CButeValue* SetDouble(i32 type, double val);
    // The struct/string value setters: op-new a payload block, copy-construct the
    // source into it, store the type-tag (+0x00) + payload ptr (+0x04). SetString
    // boxes a CString (kButeString, /GX unwind on the throwing copy-ctor),
    // SetRef5/SetRef8 a 16-byte struct (kButeRef5/kButeRef8), SetRef7 a 24-byte
    // struct (kButeRef7). Re-homed from src/Stub/MallocConstructors (were
    // BoxedStr/Boxed16a/Boxed16b/Boxed24).
    CButeValue* SetString(i32 type, const CString& src);
    CButeValue* SetRef5(i32 type, const struct ButeRef16* src);
    CButeValue* SetRef7(i32 type, const struct ButeRef24* src);
    CButeValue* SetRef8(i32 type, const struct ButeRef16* src);

    // CopyValue (@0x172040): copy `other`'s payload into this value's storage,
    // sized by THIS value's type-tag (a jump-table switch over ButeType). Returns
    // this.
    CButeValue* CopyValue(CButeValue* other);
};
SIZE(CButeValue, 0x8); // { type @0, pValue @4 }

// The 16-byte (kButeRef5 / kButeRef8) payload, copied as a struct so MSVC lowers
// it to four memberwise dword stores (SetRef5/SetRef8's op-new'd copy).
struct ButeRef16 {
    i32 w[4];
};
SIZE(ButeRef16, 0x10); // 16-byte kButeRef5/kButeRef8 payload

// The 24-byte (kButeRef7) payload, copied as a struct so MSVC lowers it to the
// retail `rep movsd` (6 dwords).
struct ButeRef24 {
    i32 w[6];
};
SIZE(ButeRef24, 0x18); // 24-byte kButeRef7 payload

// The statically-linked MSVC 5.0 <iostream.h> `ios` base (RTTI `.?AVios@@`, vtable
// 0x5f03bc) that CButeMgr embeds at +0x14 is LIBRARY code (LIBCP.LIB / LIBCMT), not
// game scope: its ctor/dtor/streambuf-setter + the virtual-base construction thunks
// are carved out in config/library_labels.csv and are NOT reconstructed here. The
// parser's own ios view lives in ButeMgrParse.cpp (struct ButeIos); the one game
// consumer, CButeMgr::ClearHelper, calls the two library entry points it needs
// through a tiny caller-side receiver defined locally in ButeMgr.cpp.

// CButeTree - the shared crit-bit trie store (Find/Insert/Walk). CButeMgr's owned
// sub-trees (m_tree/m_tree48/m_tree74) are instances of it, addressed through the
// Tree()/Tree48() accessors below. Canonical definition (one shape) lives in
// <Bute/ButeTree.h>.
#include <Bute/ButeTree.h>

// ---------------------------------------------------------------------------
// CButeStore - one owned keyed-store sub-tree (CButeMgr's m_tree / m_tree48 /
// m_tree74). 0x2c bytes, multiply-derived: a base store at +0 plus a second base
// at +8, both polymorphic (two vptrs). The mgr's /GX destructor tears each one
// down inline at its own trylevel; the member destructor reproduces that
// multiply-derived teardown:
//   (1) re-stamp the two most-derived vptrs (0x5e94ac @+0, 0x5e949c @+8),
//   (2) run the derived clear body (recursive node-free, __thiscall(this, 0)),
//   (3) restore the second base's vptr (__thiscall on the masked `this+8`,
//       `neg ecx; sbb ecx,ecx; and ecx,ebx` second-base-this adjust),
//   (4) run the primary-base destructor (__thiscall(this)).
// Only the SIZE + the two-vptr teardown are load-bearing; the interior is opaque.
// REAL POLYMORPHIC (ALL-VTABLES): CButeStore is modeled as a real multiply-derived
// class (a primary store base at +0x00 == 0x5e94ac, a second base at +0x08 ==
// 0x5e949c), so the /GX dtor auto-stamps both most-derived vptrs (== the old manual
// store-vtable re-stamps), reloc-masked; the three teardown callees are __thiscall
// engine functions (no body) so `mov ecx,this; call` falls out masked.

// The second-base subobject at +0x8: its vptr restore (0x16dfc0) is a __thiscall
// `mov [ecx],<vtbl>; ret`. Modeled as a tiny receiver so the call lands the
// masked `this+8` pointer in ecx with no caller-side cleanup.
struct CButeStoreBase2 {
    void RestoreVptr(); // 0x16dfc0
};
SIZE(CButeStoreBase2, 0x24); // receiver view of the +0x08 second base

// A keyed-store tree node the recursive clear (CButeStore::ClearRecursive,
// ButeStoreClear.cpp) walks: left/right children, the heap-order key, the owned
// name string, and the value the store's per-value callback consumes+frees.
struct CButeStoreNode {
    CButeStoreNode* m_left;  // +0x00
    CButeStoreNode* m_right; // +0x04
    i32 m_key;               // +0x08  order key
    char* m_str;             // +0x0c  owned name string (freed)
    void* m_val;             // +0x10  value the per-value callback consumes (freed);
                             //        genuinely heterogeneous, so void*
};
SIZE(CButeStoreNode, 0x14);

// The primary store base at +0x00 (most-derived scalar-deleting vptr 0x5e94ac).
struct CButeStorePrimary {
    virtual void P0(); // +0x00  most-derived (scalar-deleting) vptr
    char m_pad04[4];   // +0x04
};
SIZE(CButeStorePrimary, 0x8); // { vptr, pad }

// The second base at +0x08 (second-base vptr 0x5e949c); carries the keyed-store
// interior fields (offsets shown relative to the enclosing CButeStore).
struct CButeStoreSecond {
    virtual void S0();          // +0x00 (this+0x08)  second-base vptr
    void(__cdecl* m_cb)(void*); // +0x04 (+0x0c)  per-value callback (ClearRecursive fires)
    char m_flags;               // +0x08 (+0x10)  flag byte (bit 2 gates the callback)
    char m_pad09[0x0c - 0x09];  // +0x09 (+0x11)  opaque keyed-store interior
    i32 m_14;                   // +0x0c (+0x14)  reset-to-empty field (Parse zeros it)
    CButeStoreNode* m_root18;   // +0x10 (+0x18)  tree root (Parse zeros it)
    char m_pad14[0x20 - 0x14];  // +0x14 (+0x1c)  opaque
    i32 m_28;                   // +0x20 (+0x28)  reset-to-empty field (Parse zeros it)
};
SIZE(CButeStoreSecond, 0x24); // second base (spans +0x08..+0x2c of CButeStore)

struct CButeStore : public CButeStorePrimary, public CButeStoreSecond {
    // The derived clear body (0x16e070): recursively frees the keyed nodes from
    // `node` (or the tree root when null). __thiscall(this, node); callee-cleans 4.
    void ClearRecursive(CButeStoreNode* node);
    // Reset-to-empty: free the nodes, then zero the root + the two reset fields.
    // Inlined into CButeMgr::Parse (the tree-base pointer lands in a register).
    void Reset() {
        ClearRecursive(0);
        m_root18 = 0;
        m_28 = 0;
        m_14 = 0;
    }
    // The primary-base destructor (0x16da60): restore vptr + tear down [this+4].
    void BaseDtor(); // __thiscall(this)
    ~CButeStore() {
        // cl auto-stamps the two most-derived vptrs (0x5e94ac @+0, 0x5e949c @+8) at
        // dtor entry (== the old manual re-stamps), reloc-masked.
        ClearRecursive(0);
        // The second base lives at this+8; the compiler null-masks the adjust
        // (`neg ecx; sbb ecx,ecx; and ecx,ebx`) -> RestoreVptr on (this?this+8:0).
        CButeStoreBase2* b2 = this ? (CButeStoreBase2*)((char*)this + 8) : 0;
        b2->RestoreVptr();
        BaseDtor();
    }
};
SIZE(CButeStore, 0x2c); // MI: CButeStorePrimary @0 + CButeStoreSecond @8

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
SIZE(CButeTail, 0x1); // 1-byte embedded tail object

// ---------------------------------------------------------------------------
// CButeNode - a per-tag store node allocated by ParseTagLine (0x2c bytes). Built
// via `new CButeNode(2, descriptor)`: the engine base ctor runs, then the derived
// class's two vtable pointers are written at +0x00 / +0x08 (a multiply-derived node
// with two vptrs). REAL POLYMORPHIC (ALL-VTABLES): the derived ctor auto-stamps
// ??_7CButeNode @+0x00 and the second sub-object's vptr @+0x08.

// zPTree - the engine base subobject ctor (__thiscall(this, desc, n), retail
// 0x16dff0). RTTI-real name (was fabricated "CButeNodeBase"; see ButeNode.cpp for
// the full multiply-derived zPTree model + the vtable_hierarchy evidence). Here it
// is only the call-site stand-in for the `new CButeNode` path (empty external base,
// ctor reloc-masked), a separate model from ButeNode.cpp's to avoid an ODR conflict.
//
// zPTree's RTTI-real primary base is zErrHandling (the container-library exception
// base; full model in ButeNode.cpp, where the real zPTree derives it). This stand-in
// is intentionally kept base-less: any structural change here (even an EBO-neutral
// empty base) perturbs MSVC5's /O2 regalloc in this hot header's many includers
// (measured: gruntcombatanim / grunt / cplay fuzzy-% regressed). The vtable_hierarchy
// INHERIT audit therefore still flags this stand-in decl; the substantive fix lives
// in ButeNode.cpp's real model.
SIZE_UNKNOWN(zPTree);
VTBL(zPTree, 0x001e94ac);
class zPTree : public zErrHandling {
public:
    zPTree(void* desc, i32 n);
};

// The +0x08 second sub-object: a small polymorphic node subobject whose implicit
// vptr the CButeNode ctor auto-stamps (== the old raw second-vtable store). Modeled
// as an embedded polymorphic member so the second vptr lands at +0x08, reloc-masked.
class CButeNodeSub {
public:
    virtual void Slot0(); // +0x00 (this+0x08): second sub-object vptr
};
SIZE(CButeNodeSub, 0x4); // second sub-object (vptr only)
RELOC_VTBL(CButeNodeSub, 0x001e94ac); // aliases zPTree (doc-comment vtable, in-family)

// CButeNode - REAL POLYMORPHIC (ALL-VTABLES phase): the derived ctor runs the
// engine base ctor, then cl auto-stamps ??_7CButeNode @+0x00 and the m_sub member's
// implicit vptr @+0x08 (== the old two hand-rolled node vtable stores).
class CButeNode : public zPTree {
public:
    virtual ~CButeNode(); // +0x00 vptr; external no-body dtor

    CButeNode(void* desc, i32 n) : zPTree(desc, n) {}
    // vptr implicit @ +0x00 (??_7CButeNode@@6B@)
    char m_pad04[4];          // +0x04
    CButeNodeSub m_sub;       // +0x08  second sub-object (implicit vptr)
    char m_pad0c[0x2c - 0xc]; // pad to 0x2c bytes total
};
SIZE(CButeNode, 0x2c); // new CButeNode(0x2c); zPTree base + m_sub @0x08

// A fabricated-name placeholder for CString::operator+=(char) (a real NAFXCW
// method). Currently UNREFERENCED (no call sites), so it cannot be re-expressed as
// `str += c` via <Mfc.h> CString. KEPT (not removed) because deleting it from this
// widely-included header perturbs an unrelated fragile neighbor - it shaves 0.12%
// off grunt::CGrunt_SegBoxOverlap (a cross-TU codegen leak; unit-neutral). See P0b.
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
SIZE(CButeRef5, 0x10);
struct CButeRef6 { // 8 bytes
    CButeRef6() : a(0), b(0) {}
    ~CButeRef6();
    DWORD a, b;
};
SIZE(CButeRef6, 0x8);
struct CButeRef7 { // 24 bytes
    CButeRef7() : a(0), b(0), c(0), d(0), e(0), f(0) {}
    ~CButeRef7();
    DWORD a, b, c, d, e, f;
};
SIZE(CButeRef7, 0x18);
struct CButeRef8 { // 16 bytes
    CButeRef8() : a(0), b(0), c(0), d(0) {}
    ~CButeRef8();
    DWORD a, b, c, d;
};
SIZE(CButeRef8, 0x10);

// ---------------------------------------------------------------------------
// CString member helper - an MFC-style CString (a single char* @+0). Only the
// engine library calls the getters/parser actually emit are modeled, with NO
// body so their `call rel32` displacements are reloc-masked in objdiff:
//   - operator=  (CString::operator=, NAFXCW)
//   - the literal-ctor (CString::CString(const char*)) used by the
//     one-shot default-string init in GetString.
// (CString itself comes from <Gruntz/String.h>, included at the top.)
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
    i32 GetIntDef(const char* tag, const char* key, i32 def);
    i32 GetInt(const char* tag, const char* key);
    DWORD GetDwordDef(const char* tag, const char* key, DWORD def);
    DWORD GetDword(const char* tag, const char* key);
    float GetFloat(const char* tag, const char* key);
    double GetDouble(const char* tag, const char* key);
    CString* GetStringDef(const char* tag, const char* key, CString* def);
    char* GetString(const char* tag, const char* key);

    bool ScanToken(i32 expectType);
    bool ParseTagLine();
    bool Parse();
    // Open the named .att stream, reset the three stores, run the recursive
    // group parse, then close+delete the stream. Returns parse success.
    // (0x3cc20, defined in ButeMgrParse.cpp.)
    bool Parse(CString filename, int streamBase);

    // The variadic error reporter: format `fmt` + varargs into m_errStr, then
    // fire the optional m_errCallback with the message. A variadic member is
    // __cdecl with `this` as the hidden first stack arg (QAA mangling) - exactly
    // the retail ABI (this pushed last at each call site).
    void ReportError(const char* fmt, ...);

    // Callback trampoline + sub-object cleanup.
    void* InvokeCallback(void* (*fn)(CButeMgr*));
    void ClearHelper();

    // Reset/teardown of the manager's stores (0x170210, 280 B): restamps the store
    // vtables and tears the sub-trees down through the scalar-deleting-destructor
    // path. No-body extern (reloc-masked); the low-RVA thunk pool tail-forwards it.
    void Term();

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
    bool Exists(const char* tag, const char* key);

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
    CButeTree* m_pNode;           // +0x44  active store node (a CButeNode used as a keyed tree)
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
    char m_writeMode;             // +0x10d  0 = STORE (parse text -> build tree); non-zero =
                                  //         WRITE-BACK (read tree via getters -> reconstruct
                                  //         text into m_pText). Gates dup-check + node-alloc off
                                  //         and the getter/accumulator readback on (ParseTagLine,
                                  //         ParseAttributeFile, ParseGroup).
    char m_10e;                   // +0x10e
    CButeTail m_10f;              // +0x10f  1-byte embedded object (non-trivial dtor)

    // The typed-reference getters (tag5-8). Each returns a pointer to the typed
    // value record's storage on a type hit, or a shared zero-default static on
    // any miss (no tag / no key / type mismatch), reporting the specific failure.
    CButeRef5* GetRef5(const char* tag, const char* key);
    CButeRef6* GetRef6(const char* tag, const char* key);
    CButeRef7* GetRef7(const char* tag, const char* key);
    CButeRef8* GetRef8(const char* tag, const char* key);
};
SIZE(CButeMgr, 0x110); // fields through the +0x10f embedded tail object

// --- vtable catalog (reduced-view classes share their base vtable rva) ---
VTBL(CButeStore, 0x001e949c);
// (g_buteTree's runtime +0x08 secondary @0x5f04dc is bound to this same class's
// emitted ??_7CButeStore@@6BCButeStoreSecond@@ via @data-symbol in ButeMgr.cpp.)

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // SRC_BUTE_BUTEMGR_H
