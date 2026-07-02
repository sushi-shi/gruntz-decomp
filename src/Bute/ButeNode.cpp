#include <rva.h>
// ButeNode.cpp - CButeNodeBase, a ButeMgr config-tree node base (dedicated unit).
//
// This is its own self-contained class model (CContainerErr base, CButeNodeEntry
// subobject); it cannot fold into ButeMgr.cpp, which carries a different minimal
// `class CButeNodeBase` decl (ButeMgr.h) for the ParseTagLine `new CButeNode` path
// -> ODR conflict. So it lives in its own unit, flags="eh".
//
// CButeNodeBase derives from CContainerErr (the container-library exception
// base, ctor @0x16d9c0, modeled in GameText) and embeds a small node subobject
// at +0x8 (the keyed-store entry: a ptr + a 16-bit kind + a vtbl). The ctor
// runs the CContainerErr base ctor (passing the node's error-message global),
// constructs the +0x8 subobject from (n, desc), then re-stamps the two derived
// vtables (the subobject at +0x8 and the whole object at +0x0) and zeroes the
// two child-link fields at +0x18 / +0x28.
//
// This ctor carries a C++ /GX EH frame (push -1 / push handler / fs:0 save) for
// the subobject construction; this unit is flags="eh" so the frame is present.
//
// Field names are placeholders; only OFFSETS + code bytes are load-bearing.

// CContainerErr - the container-library exception base (vptr@0, msg@4). Its
// ctor (RVA 0x16d9c0, real body in GameText) is __thiscall(this, msg); declared
// no-body here so the `push msg; call` shape is reloc-masked. REAL POLYMORPHIC
// (ALL-VTABLES phase): the vtbl@0 field is now the implicit vptr (virtual dtor);
// the derived CButeNodeBase ctor auto-stamps ??_7CButeNodeBase @+0 after the
// external base ctor returns (== the old manual g_buteNodeVtbl restamp). The base
// stays destructible so the derived ctor keeps its /GX unwind frame.
extern void* g_buteNodeErrMsg; // DAT_006bf480 - the node's error-message global

class CContainerErr {
public:
    CContainerErr(void* msg);
    virtual ~CContainerErr(); // +0x00 vptr; external no-body dtor (real body in GameText)

    void* m_msg; // +0x04
};

// The +0x8 member's promoted "sub" vtable, re-stamped into the CButeNodeEntry
// member's vptr by the CButeNodeBase ctor (the "incorrect load into struct" the
// ALL-VTABLES phase keeps as a raw vptr write). Reloc-masked file-scope address.
extern void* g_buteNodeSubVtbl; // 0x5e949c -> stamped at this+0x8

// The embedded node subobject at CButeNodeBase+0x8: a small keyed-store entry.
// REAL POLYMORPHIC: its ctor (0x16df70) auto-stamps ??_7CButeNodeEntry (retail
// 0x5f04d8) at +0x0, then stores desc@+4, (WORD)n@+8, 0@+0xc. (Making it a real
// virtual sinks the auto-vptr stamp to FIRST rather than the old hand-rolled
// last-store; the vptr-position shift is accepted per the ALL-VTABLES mandate.)
class CButeNodeEntry {
public:
    CButeNodeEntry(i32 n, void* desc);
    virtual ~CButeNodeEntry(); // +0x00 vptr; external no-body scalar-deleting dtor

    void* m_4; // +0x04  desc
    i16 m_8;   // +0x08  (WORD)n
    char m_pada[2];
    i32 m_c; // +0x0c
};
SIZE_UNKNOWN(CButeNodeEntry);
VTBL(CButeNodeEntry, 0x005f04d8); // the entry member's own (base) vtable

// CButeNodeBase layout:
//   +0x00  CContainerErr base (vptr, msg)
//   +0x08  CButeNodeEntry subobject (vptr, desc, kind, 0) - spans +0x08..+0x18
//   +0x18  m_18 : child link, zeroed
//   +0x28  m_28 : child link, zeroed
class CButeNodeBase : public CContainerErr {
public:
    CButeNodeBase(void* desc, i32 n);

    CButeNodeEntry m_entry; // +0x08 (0x10 bytes -> ends at +0x18)
    i32 m_18;               // +0x18
    char m_pad1c[0x28 - 0x1c];
    i32 m_28; // +0x28
};
SIZE(CButeNodeBase, 0x2c); // measured: new(0x2c) -> ctor 0x16dff0; matches the layout above
VTBL(CButeNodeBase, 0x005e94ac); // most-derived (whole-object) vtable @+0

// CButeNodeEntry ctor (0x16df70): __thiscall(this, n, desc). cl auto-stamps the
// ??_7CButeNodeEntry vptr@+0, then stores desc@+4, (WORD)n@+8, 0@+0xc. Clean leaf
// ctor. Called out-of-line by the CButeNodeBase ctor's m_entry member-init.
// @early-stop
// vptr-position wall (~82.9%, was 100% hand-rolled): real polymorphism sinks the
// implicit ??_7 vptr stamp to FIRST; the hand-rolled last-store cannot be sunk in
// MSVC5 (same mechanism as CZArray2D). Converted per the ALL-VTABLES mandate.
RVA(0x0016df70, 0x22)
CButeNodeEntry::CButeNodeEntry(i32 n, void* desc) {
    m_4 = desc;
    m_8 = (i16)n;
    m_c = 0;
}

// CButeNodeBase ctor (0x16dff0): run the CContainerErr base ctor + the m_entry
// member ctor, promote the m_entry member's vptr to the "sub" vtable 0x5e949c
// (raw vptr write - the promoted second vtable can't be a C++-nameable ??_7), zero
// the two child links, then cl auto-stamps ??_7CButeNodeBase @+0 (== the old manual
// g_buteNodeVtbl restamp). /GX unwind frame from the two destructible sub-objects.
// Was 100% with the hand-rolled manual stamps; the auto-vptr-first stamps are
// accepted per the ALL-VTABLES mandate (regression OK).
// @early-stop
// vptr-position wall (~96.1%, was 100%): real polymorphism auto-stamps ??_7 @+0
// and the member's implicit vptr, shifting the stamp schedule vs the hand-rolled
// stores. Logic byte-faithful; converted per the ALL-VTABLES mandate.
RVA(0x0016dff0, 0x73)
CButeNodeBase::CButeNodeBase(void* desc, i32 n)
    : CContainerErr(&g_buteNodeErrMsg), m_entry(n, desc) {
    *(void**)&m_entry = &g_buteNodeSubVtbl;
    m_18 = 0;
    m_28 = 0;
}
