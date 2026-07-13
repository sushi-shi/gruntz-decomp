// ButeValue.h - CButeValue, the typed value record every .att/.bute key maps to,
// plus its ButeType discriminant and the two raw payload shapes.
//
// Extracted verbatim from <Bute/ButeMgr.h> so the OTHER TU that reproduces the
// value teardown - src/Bute/ButeNode.cpp, home of the store's __cdecl per-value
// teardown callback at 0x174df0 - models the value WITHOUT pulling ButeMgr.h.
// ButeMgr.h includes this header, so its many includers see the identical shapes;
// there is exactly ONE CButeValue in the tree.
//
// STALE CLAIM REMOVED (2026-07-13): the reason given used to be "(whose
// <Bute/ButeStore.h> CButeStore would clash with butenode's own store class)". That clash is
// imaginary - ButeNode.cpp ALREADY includes <Bute/ButeStore.h> directly, and its "own store
// class" (CButeStoreCopy174d) DERIVES from that same canonical CButeStore. Verified: adding
// #include <Bute/ButeMgr.h> to ButeNode.cpp compiles clean under the real MSVC 5.0. Keeping
// this header separate is fine (it is a shared canonical header, not a view) - but it is a
// convenience, NOT a wall, so do not cite a C2011 to justify anything else.
#ifndef SRC_BUTE_BUTEVALUE_H
#define SRC_BUTE_BUTEVALUE_H

#include <Ints.h>
#include <rva.h>

class CString;

// The CButeValue::type discriminant. Recovered from the type-tag each getter
// compares against (GetInt==0, GetDword==1, GetDouble==2, GetFloat==3,
// GetString==4) and the typed-reference getters (GetRef5..8). The numeric values
// are load-bearing (the exact immediates the getters cmp, and the dense 0..8 jump
// table both value-teardown paths switch on); the enum only names them. Kept as an
// `int`-width enum so it is interchangeable with the `i32 type` field/params at /O2.
enum ButeType {
    kButeInt = 0,    // stored int      (GetInt/GetIntDef)
    kButeDword = 1,  // stored DWORD    (GetDword/GetDwordDef)
    kButeDouble = 2, // stored double   (GetDouble)
    kButeFloat = 3,  // stored float    (GetFloat)
    kButeString = 4, // stored CString  (GetString/GetStringDef)
    kButeRef5 = 5,   // 16-byte struct  (GetRef5)
    kButeRef6 = 6,   // 8-byte struct   (GetRef6)
    kButeRef7 = 7,   // 24-byte struct  (GetRef7)
    kButeRef8 = 8,   // 16-byte struct  (GetRef8)
};

// The typed value record a key resolves to:
//   +0x00  type   : ButeType - the value's type-tag (see enum above).
//   +0x04  pValue : void* - the heap payload (the int/dword/float/double sits at
//                           [pValue]; kButeString boxes a CString there).
struct CButeValue {
    i32 type;     // +0x00  ButeType discriminant (kept i32-width for the ABI)
    void* pValue; // +0x04

    // Destructor: free the owned pValue storage, sized/typed by the type-tag via a
    // dense ButeType jump table (string destructs the CString first; all others are
    // a plain operator delete). The store fires the SAME teardown through its
    // __cdecl per-value callback slot (ButeValueTeardown, 0x174df0, butenode).
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
    // struct (kButeRef7).
    CButeValue* SetString(i32 type, const CString& src);
    CButeValue* SetRef5(i32 type, const struct ButeRef16* src);
    CButeValue* SetRef7(i32 type, const struct ButeRef24* src);
    CButeValue* SetRef8(i32 type, const struct ButeRef16* src);

    // CopyValue (@0x172040): copy `other`'s payload into this value's storage,
    // sized by THIS value's type-tag (a jump-table switch over ButeType). Returns this.
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

// The keyed store's per-value teardown callback (0x174df0, butenode): __cdecl, so it
// fits the store's generic `void(__cdecl*)(void*)` callback slot, which
// CButeStore::ClearRecursive fires on each node's value before freeing the value cell
// itself. Its ADDRESS is the "descriptor" every config node is constructed with
// (retail: `new CButeNode(&ButeValueTeardown, 2)`). Declared here so both the node
// ctor (butenode) and ParseTagLine (butemgr) reference the one real function.
void __cdecl ButeValueTeardown(void* pValue); // 0x174df0

#endif // SRC_BUTE_BUTEVALUE_H
