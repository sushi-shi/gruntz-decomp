#ifndef SRC_BUTE_BUTEVALUE_H
#define SRC_BUTE_BUTEVALUE_H

#include <Ints.h>
#include <rva.h>

class CString;

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

struct CButeValue {
    i32 type;     // +0x00  ButeType discriminant (kept i32-width for the ABI)
    void* pValue; // +0x04

    // The "boxed value" 2-arg ctor (@0x1741b0, butemgr): tag `this` with `type`,
    // op-new an 8-byte CButeValue, copy `src`'s {type, pValue} into it, and stow the
    // boxed copy in this->pValue (0 on alloc failure). Used by the attribute-file
    // store builder to wrap a parsed value.
    CButeValue(i32 type, CButeValue* src);

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
    CButeValue* SetRef5(i32 type, const struct ButeRefSmall* src);
    CButeValue* SetRef7(i32 type, const struct ButeRefLarge* src);
    CButeValue* SetRef8(i32 type, const struct ButeRefSmall* src);

    // CopyValue (@0x172040): copy `other`'s payload into this value's storage,
    // sized by THIS value's type-tag (a jump-table switch over ButeType). Returns this.
    CButeValue* CopyValue(CButeValue* other);
};
SIZE(0x8); // { type @0, pValue @4 }

struct ButeRefSmall {
    i32 w[4];
};
SIZE(0x10); // 16-byte kButeRef5/kButeRef8 payload

struct ButeRefLarge {
    i32 w[6];
};
SIZE(0x18); // 24-byte kButeRef7 payload

void __cdecl ButeValueTeardown(void* pValue); // 0x174df0

#endif // SRC_BUTE_BUTEVALUE_H
