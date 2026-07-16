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
// The REAL zPTree config-tree hierarchy (zErrHandling @0, zPtrColl/CButeNodeEntry @8;
// zPTree : those; CButeTree/CButeStore/CButeNode : zPTree). RTTI-proven single model -
// the former ButeMgr.h stand-in zErrHandling/zPTree/CButeNodeSub views are dissolved
// onto it (structure-recovery: CButeTree/CButeStore/CButeNode all derive zPTree).
#include <Bute/PTreeNode.h>

// CString (+ CObject etc.) and the Win32 DWORD come from <Mfc.h>; pulled up here
// so the class below can use both. (afx.h is the period-correct windows.h path.)
#include <Gruntz/String.h>

// ButeType / CButeValue / ButeRefSmall / ButeRefLarge - the typed value record every key
// maps to. Canonical (one shape) in <Bute/ButeValue.h>, shared with src/Bute/ButeNode.cpp
// (which reproduces the store's __cdecl per-value teardown callback over the same type).
#include <Bute/ButeValue.h>

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
// m_tree74). The family (CButeStoreBase2/Node/Primary/Second/CButeStore) lives
// verbatim in <Bute/ButeStore.h> (wave2-H extraction) so ClearRecursive's
// defining TU can include it without this header's zPTree stand-in.
#include <Bute/ButeStore.h>

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

// (CButeNode - the per-tag store node ParseTagLine allocates - now lives in the
//  SHARED <Bute/PTreeNode.h>, folded with butenode's former `CButeCfgNode174d`: the
//  two were one class all along (same pair of most-derived vtables, 0x1f051c @+0 and
//  0x1f0518 @+8 - see the fold note there). Both TUs now emit the same ??_7CButeNode
//  names, so the two runtime vtable data cells bind.)

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

// The MSVC 5.0 <iostream.h> input stream (LIBCP/LIBCMT). Forward-declared for the
// m_stream pointer member below; the complete type (via <fstream.h>) is only needed in
// the defining TU (ButeMgr.cpp), which calls istream::get()/ios::eof() on it.
class istream;

// ---------------------------------------------------------------------------
// CButeMgr - the attribute manager.
// ---------------------------------------------------------------------------
class CButeMgr {
public:
    i32 GetIntDef(const char* tag, const char* key, i32 def);
    i32 GetInt(const char* tag, const char* key);
    DWORD GetDwordDef(const char* tag, const char* key, DWORD def);
    DWORD GetDword(const char* tag, const char* key);
    float GetFloatDef(const char* tag, const char* key, float def);
    float GetFloat(const char* tag, const char* key);
    double GetDoubleDef(const char* tag, const char* key, double def);
    double GetDouble(const char* tag, const char* key);
    CString* GetStringDef(const char* tag, const char* key, CString* def);
    char* GetString(const char* tag, const char* key);
    // The typed-reference getters (value type-tags 5..8): return the stored ref
    // payload pointer on a type match, else report (type mismatch) and return def.
    struct CButeRef5* GetRef5(const char* tag, const char* key, struct CButeRef5* def);
    struct CButeRef6* GetRef6(const char* tag, const char* key, struct CButeRef6* def);
    struct CButeRef7* GetRef7(const char* tag, const char* key, struct CButeRef7* def);
    struct CButeRef8* GetRef8(const char* tag, const char* key, struct CButeRef8* def);

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
    void SetErrCallback(ErrCallback cb); // 0x170380
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
    istream* m_stream;            // +0xa0  input source stream: a real CRT istream* (the
                                  //         common base of the concrete streams stored -
                                  //         Parse's `new ifstream(...)` and the .rez path's
                                  //         custom istream-derived decode stream both adjust
                                  //         to their istream subobject on store, which is
                                  //         exactly what this base pointer holds). NextChar
                                  //         reads it via istream::get()/ios::eof().
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

// The big attribute-file line driver (0x170750) is the ONLY method retail mangles under
// `ButeMgr@@` (every sibling is `CButeMgr@@`), so ParseAttributeFile lives on a real `ButeMgr`
// class that single-inherits CButeMgr at offset 0 (CButeMgr is non-polymorphic -> no vptr), so
// `this` is a ButeMgr* == its CButeMgr* sub-object and every member is reached through
// inheritance with no cast. Body in ButeMgr.cpp. (Was a .cpp-local decl.)
class ButeMgr : public CButeMgr {
public:
    bool ParseAttributeFile(); // 0x170750
};
SIZE(ButeMgr, 0x110); // == sizeof(CButeMgr): the single base, no added members

// (The store's two vtables are zPTree's - CButeStore IS zPTree, see <Bute/ButeStore.h> -
//  and they are pinned on their real emitted names in src/Bute/ButeNode.cpp. The old
//  VTBL(CButeStore, 0x1e949c) here bound them to a class that no longer exists, and the
//  0x5f04dc pin named a fabricated CButeStoreSecond base; 0x1f04dc is really an
//  unrecovered container class, now catalogued in config/library_vtables.csv.)

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

// The global bute store singleton (?g_buteMgr@@3VCButeMgr@@A @0x6453d8); its getters
// (GetInt/GetString/...) reloc-mask. Declared here so consumers include the header.
extern CButeMgr g_buteMgr;

#endif // SRC_BUTE_BUTEMGR_H
