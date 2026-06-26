// HelperHost.cpp - two scattered __thiscall leaf helpers Ghidra grouped under the
// placeholder class "HelperHost" (0x164790 / 0x166040). Both hang off an owner
// context at +0x0c. Self-contained except the MFC CMapStringToOb::Lookup
// (0x1b8008) named-object lookup in 0x166040. Names are placeholders; offsets +
// code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>

// MFC CMapStringToOb::Lookup (NAFXCW 0x1b8008). Minimal decl - the mangled name
// ?Lookup@CMapStringToOb@@QBEHPBDAAPAVCObject@@@Z reloc-masks against the library
// symbol, so no <Mfc.h> is needed (keeps this a /O2 /MT `base` TU).
class CObject;
class CMapStringToOb {
public:
    i32 Lookup(const char* key, CObject*& rValue) const;
};

// The owner context at HelperHost+0x0c: a sub-manager ptr at +0x10 (its +0x10 is
// the string->object map) and an int at +0x24.
struct HelperHostMap {
    char pad_00[0x10];
    CMapStringToOb m_10; // +0x10  named-object map
};
struct HelperHostCtx {
    char pad_00[0x10];
    HelperHostMap* m_10; // +0x10
    char pad_14[0x24 - 0x14];
    i32 m_24;            // +0x24
};

// The object Lookup yields, viewed as a bounded element array.
struct HelperHostObj {
    char pad_00[0x14];
    void** m_14;             // +0x14  element array
    char pad_18[0x64 - 0x18];
    i32 m_64;                // +0x64  lo index
    i32 m_68;                // +0x68  hi index
};

class HelperHost {
public:
    i32 Helper_164790(i32 a, i32 b);
    i32 Helper_166040(i32 key, i32 idx);

    char pad_00[0x0c];
    HelperHostCtx* m_0c; // +0x0c  owner context
    i32 m_10;            // +0x10
    i32 m_14;            // +0x14
    char pad_18[0x3c - 0x18];
    i32 m_3c;            // +0x3c
    i32 m_40;            // +0x40
    i32 m_44;            // +0x44
    i32 m_48;            // +0x48
    i32 m_4c;            // +0x4c
    i32 m_50;            // +0x50
    char pad_54[0x58 - 0x54];
    i32 m_58;            // +0x58
    i32 m_5c;            // +0x5c
    i32 m_60;            // +0x60
    char pad_64[0x78 - 0x64];
    i32 m_78;            // +0x78
};

// ===========================================================================
// 0x164790 - reset/arm the helper from (a, b); seeds m_3c off the owner context.
// ===========================================================================
// @early-stop
// regalloc/scheduling wall (topic:regalloc): logic + every member store are
// byte-exact, but retail parks the m_0c context ptr in eax (immediate stores for
// m_48/m_50, a late `mov eax,1` for the return) while cl parks it in edx and
// reuses one `mov eax,1` for both m_50 and the return. A pure eax<->edx coin-flip
// over the trailing constant; no source lever flips it (hoisting the ctx read,
// reordering the stores - all no-change at the same plateau). ~90%.
RVA(0x00164790, 0x41)
i32 HelperHost::Helper_164790(i32 a, i32 b) {
    m_5c = a;
    m_10 = 0;
    m_14 = 0;
    m_40 = 0;
    m_44 = 0;
    m_4c = 0;
    m_58 = 0;
    m_60 = b;
    m_48 = 0x32;
    m_50 = 1;
    m_3c = m_0c->m_24;
    return 1;
}

// ===========================================================================
// 0x166040 - look up a named object in the owner's map, then fetch element[idx]
// when in range; cache it at +0x78 and return whether it is non-null.
// ===========================================================================
// @early-stop
// scheduling wall (topic:regalloc): body byte-exact (same no-edi regalloc, the
// tail-duplicated v!=0 epilogue, the bounds dispatch). Only residue is WHERE the
// Lookup out-param zero-init lands - retail emits it after both arg pushes, cl
// emits it between them (a 1-instruction reorder). Not source-steerable. ~95%.
RVA(0x00166040, 0x66)
i32 HelperHost::Helper_166040(i32 key, i32 idx) {
    CObject* obj = 0;
    m_0c->m_10->m_10.Lookup((const char*)key, obj);
    HelperHostObj* p = (HelperHostObj*)obj;
    i32 v;
    if (p != 0 && idx >= p->m_64 && idx <= p->m_68)
        v = (i32)p->m_14[idx];
    else
        v = 0;
    m_78 = v;
    return v != 0;
}
