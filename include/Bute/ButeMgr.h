#ifndef SRC_BUTE_BUTEMGR_H
#define SRC_BUTE_BUTEMGR_H

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Bute/PTreeNode.h>

#include <Gruntz/String.h>

#include <Bute/ButeValue.h>

#include <Bute/ButeTree.h>

#include <Bute/ButeStore.h>

void ButeStoreFreeAdapter(void* p); // 0x174de0 (ButeNode.cpp)
struct CBSecStream : zPTree {
    CBSecStream() : zPTree(&ButeStoreFreeAdapter, 2) {}
    virtual ~CBSecStream() OVERRIDE {} // inline: ~CButeMgr inlines the two-layer expansion
};
SIZE(0x2c); // adds nothing to zPTree (m_tree +0x18 .. m_pNode +0x44)
// The +0x08 second-base-in-derived vtable @0x5f0514 (cl-emitted from the CButeNodeEntry
// base); the DATA_SYMBOL pins binding both retail datums live in ButeSectionCtor.cpp.

struct CButeTail {
    char m_00;   // +0x00
    CButeTail(); // 0x16f680  external ctor (`mov eax,ecx; ret`; BSecObj10fCtor.cpp)
    // The empty out-of-line dtor (0x16f6b0, one `ret`) - the +0x10f member teardown
    // ~CButeMgr + its unwind funclet call (was mislabeled __fpclear in the library
    // catalog, and faked here as a declared-only "Dtor()").
    ~CButeTail();
};
SIZE(0x1); // 1-byte embedded tail object

extern "C" void AfxString_AppendChar(void* pStr, char c);

struct CButeRef5 { // 16 bytes
    CButeRef5() : a(0), b(0), c(0), d(0) {}
    ~CButeRef5() {}
    DWORD a, b, c, d;
};
SIZE(0x10);
struct CButeRef6 { // 8 bytes
    CButeRef6() : a(0), b(0) {}
    ~CButeRef6() {}
    DWORD a, b;
};
SIZE(0x8);
// The <x,y,z> rect3 record: three doubles carried as their {lo,hi} DWORD halves.
// The ctor writes six SEPARATE dword zeroes (plain doubles change that store shape),
// while the writer reads each pair whole - both readings of the same 24 bytes are
// real, so the double arms are named here instead of punned at the reader.
struct CButeRef7 { // 24 bytes
    CButeRef7() : a(0), b(0), c(0), d(0), e(0), f(0) {}
    ~CButeRef7() {}
    union {
        struct {
            DWORD a, b;
        };
        double x;
    };
    union {
        struct {
            DWORD c, d;
        };
        double y;
    };
    union {
        struct {
            DWORD e, f;
        };
        double z;
    };
};
SIZE(0x18);
// The [x,y] rect2 record - same {lo,hi}-halves-of-a-double shape as CButeRef7.
struct CButeRef8 { // 16 bytes
    CButeRef8() : a(0), b(0), c(0), d(0) {}
    ~CButeRef8() {}
    union {
        struct {
            DWORD a, b;
        };
        double x;
    };
    union {
        struct {
            DWORD c, d;
        };
        double y;
    };
};
SIZE(0x10);

extern "C" i32 atexit(void (*func)(void));

typedef void(__cdecl* ErrCallback)(const char*);

class istream;

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

    // The default constructor (0x170210, 280 B): the 8-state /GX EH ctor that
    // default-constructs the five CStrings, the three CBSecStream sub-trees and the
    // +0x10f tail, zeroes the scalars and Empty()s m_str108/m_tagName. Body in
    // ButeSectionCtor.cpp. (Ex "CButeSection::CButeSection" - the CButeSection twin
    // class is DISSOLVED: it was this same 0x113-B object modeled a second time; the
    // phantom `void Term()` once claimed this RVA too, with zero callers.)
    CButeMgr();

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
    // The lexer transition table's three columns (state x class): PeekState
    // returns column 0, PeekState2 returns column 1, and ScanState writes
    // columns 1 and 2 to the token type (+0xaa) and lexer state (+0xac).
    i16 PeekState(i16 state, char c);
    i16 PeekState2(i16 state, char c);
    void ScanState(i16 state, char c);
    // Outer tag-skip loop: re-lex until a tag/group token (1/2), or fail.
    bool SkipToTag();
    // Recursive group parser (the per-tag descent).
    bool ParseGroup();
    // 0x173dd0: box `val` as a type-6 CButeValue and store it at (tag, key),
    // walking the three keyed stores. `this` is proven by the three zPTree
    // receivers the body uses - `lea ecx,[this+0x18]` / `+0x48` / `+0x74` are
    // exactly m_tree / m_tree48 / m_tree74, and the callees are zPTree::Find
    // (0x16d190), zPTree::Insert (0x16db90), CButeNode::CButeNode(i32)
    // (0x174d00), CButeValue::CButeValue(i32,CButeValue*) (0x1741b0),
    // CButeValue::CopyValue (0x172040) and ~CButeValue (0x172160). `ret 0xc`
    // pins the three-dword arity; every path leaves eax holding a callee's
    // leftover, so the return type is void. No caller in retail (dead code:
    // `sema xref` finds neither a call/jmp nor a data reference), which is why
    // it carried a fabricated `LoadProjActMap` name.
    void SetValue(const char* tag, const char* key, struct CButeValue* val);

    // Key existence probe: Find(tag); if no key requested, hit on the group;
    // else require the key under it.
    bool Exists(const char* tag, const char* key);

    // The /GX (EH-frame) scalar destructor (0x213c0): tears down the three owned
    // zPTree sub-trees + the five CStrings + the +0x10f tail object, each at
    // its own descending trylevel (reverse declaration order).
    ~CButeMgr();

    // Accessor for the +0x18 store tree. Find/Insert live on the zPTree base now, so
    // the store member is addressed at its OWN type (`lea ecx,[esi+0x18]`, unchanged) -
    // the ex sibling-to-sibling reinterpret to CButeTree is gone.
    CBSecStream* Tree() {
        return &m_tree;
    }
    // The second keyed sub-tree at +0x48 (ParseGroup reaches it).
    CBSecStream* Tree48() {
        return &m_tree48;
    }

    i32 m_streamBase;          // +0x00  stream base offset (NextChar's `- m_00`)
    i32 m_pos;                 // +0x04  running parse cursor position
    i32 m_lineNo;              // +0x08
    bool m_countLine;          // +0x0c  bump m_lineNo on the next char (set after \n).
                               //         bool, not char: `m_countLine = (delta == 0xa)`
                               //         stores the setcc byte directly (retail has no
                               //         `xor ecx,ecx` int-widening before the `sete cl`).
    char m_0d;                 // +0x0d  (role unproven - cleared by Init only)
    char m_pad0e[0x10 - 0xe];  // +0x0e
    CString m_errStr;          // +0x10  scratch the error reporter formats into
    ErrCallback m_errCallback; // +0x14  optional error-callback fn-ptr
    CBSecStream m_tree;        // +0x18  the keyed store root (0x2c bytes; the ctor's
                               //         0x1f0510 stamps prove the concrete type)
    // +0x44 the active store node. Declared at the zPTree base: ParseTagLine stores a
    // `new CButeNode` here and the getters use it as a keyed store, which only works
    // because the store operations are the BASE's (they were on CButeTree, forcing a
    // CButeNode* -> CButeTree* sibling reinterpret).
    zPTree* m_pNode;
    CBSecStream m_tree48;         // +0x48  second store sub-tree
    CBSecStream m_tree74;         // +0x74  third store sub-tree
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
SIZE(0x110); // fields through the +0x10f embedded tail object

class ButeMgr : public CButeMgr {
public:
    bool ParseAttributeFile(); // 0x170750
};
SIZE(0x110); // == sizeof(CButeMgr): the single base, no added members

extern CButeMgr g_buteMgr;

// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" double ButeRead_Float(char* tok); // 0x18d220

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" void ButeGroup_Apply(char* key, void* value, void* ctx);
extern "C" DWORD ButeRead_Dword(char* tok, char** end, i32 base); // 0x1240b0
extern "C" i32 sscanf(const char* buf, const char* fmt, ...);     // 0x120900

extern "C" i16 g_charClass[];
extern "C" i16 g_transTable[97][49][3];
#endif // SRC_BUTE_BUTEMGR_H
