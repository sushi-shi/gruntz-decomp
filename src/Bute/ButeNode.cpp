#include <rva.h>
// ButeNode.cpp - CButeNodeBase, a ButeMgr config-tree node base (dedicated unit).
//
// Re-homed from src/Stub/CButeNodeBase.cpp: its own self-contained class model
// (CContainerErr base, CButeNodeEntry subobject) cannot fold into ButeMgr.cpp,
// which carries a different minimal `class CButeNodeBase` decl (ButeMgr.h) for the
// ParseTagLine `new CButeNode` path -> ODR conflict. So it lives in its own unit,
// flags="eh" (== the engine_label_stubs base+/GX it came from; matching-neutral).
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

// CContainerErr - the container-library exception base (vtbl@0, msg@4). Its
// ctor (RVA 0x16d9c0, real body in GameText) is __thiscall(this, msg); declared
// no-body here so the `push msg; call` shape is reloc-masked.
extern void* g_buteNodeErrMsg; // DAT_006bf480 - the node's error-message global

class CContainerErr {
public:
    CContainerErr(void* msg);
    // Non-virtual external (no-body) dtor: the base IS destructible in retail. This
    // is what forces the /GX unwind frame in the derived ctor - if the m_entry
    // member ctor throws (constructed after the base), the base must be unwound, so
    // cl sets trylevel 0 right before the m_entry call (verified: llvm-objdump
    // base-vs-target). Non-virtual, so the ctor-call bytes are unchanged.
    ~CContainerErr();

    void* m_vtbl; // +0x00
    void* m_msg;  // +0x04
};

// The two derived vtables re-stamped by the ctor. Reloc-masked file-scope
// addresses (the most-derived vtbl for the +0x8 subobject, and for the whole
// object). External, no-body.
extern void* g_buteNodeSubVtbl; // 0x5e949c -> stamped at this+0x8
extern void* g_buteNodeVtbl;    // 0x5e94ac -> stamped at this+0x0

// The embedded node subobject at CButeNodeBase+0x8: a small keyed-store entry.
// Its ctor (RVA 0x16df70) is __thiscall(this, int n, void* desc): stores desc at
// +0x4, (WORD)n at +0x8, 0 at +0xc, and its own vtbl at +0x0. This is a WAP-engine
// `z*`-container node with a HAND-ROLLED vtable, NOT a C++ polymorphic class: the
// vtbl pointer is stored LAST (after the member stores). Modeling it with a real
// `virtual` dtor makes cl emit the auto-vptr stamp FIRST (before the members),
// which REGRESSES this ctor 100%->82.9% (proven: llvm-objdump base-vs-target -- the
// implicit ??_7 stamp cannot be sunk; the MSVC5 vptr-position wall, same mechanism
// as CZArray2D). So the manual vtbl-field store stays -- it is the devs' real shape.
class CButeNodeEntry {
public:
    CButeNodeEntry(i32 n, void* desc);
    // Non-virtual external (no-body) dtor: the node IS destructible in retail (its
    // hand-rolled scalar-deleting dtor). Declaring it makes the owning CButeNodeBase
    // ctor emit the /GX unwind frame around the m_entry member-init (the member has a
    // non-trivial dtor to run on a throw); non-virtual, so the entry ctor's vptr
    // position (stamped LAST) is unchanged.
    ~CButeNodeEntry();

    void* m_vtbl; // +0x00
    void* m_4;    // +0x04  desc
    i16 m_8;      // +0x08  (WORD)n
    char m_pada[2];
    i32 m_c; // +0x0c
};

// CButeNodeBase layout:
//   +0x00  CContainerErr base (vtbl, msg)
//   +0x08  CButeNodeEntry subobject (vtbl, desc, kind, 0) - spans +0x08..+0x18
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

// The base vtable the CButeNodeEntry subobject stamps from its own ctor (later
// restamped to g_buteNodeSubVtbl by the owning CButeNodeBase ctor). Reloc-masked.
DATA(0x005f04d8)
void* g_buteNodeEntryVtbl;

// CButeNodeEntry ctor (0x16df70): __thiscall(this, n, desc). Stores desc@+4,
// (WORD)n@+8, 0@+0xc, then its own base vtable@+0 (stamped LAST -- written in
// source order; a hand-rolled vtbl, not a C++ virtual -- see the note above). Clean
// leaf ctor, no EH frame. Called out-of-line by the CButeNodeBase ctor's m_entry
// member-init. (Trace-discovered; was the ClassUnknown_7 stub.)
RVA(0x0016df70, 0x22)
CButeNodeEntry::CButeNodeEntry(i32 n, void* desc) {
    m_4 = desc;
    m_8 = (i16)n;
    m_c = 0;
    m_vtbl = &g_buteNodeEntryVtbl;
}

// CButeNodeBase scalar-deleting dtor (??_G, vtbl slot 0): restore the two vftables,
// run the 3 sub-object dtors, then (flags & 1) -> operator delete(this), return this.
// The ??_G thunk is compiler-generated, but this class is modeled with a MANUAL vtbl
// pointer (the m_vtbl field + a hand store), not C++ virtuals - so MSVC emits no
// ??_GCButeNodeBase for an @rva-symbol pin to match (it would be a MISS). Pinning it
// would require a real `virtual ~CButeNodeBase()`, i.e. a compiler vptr at +0 that
// collides with the manually-modeled vtbl and breaks the matched ctor layout. So the
// dtor is documented here, not symbol-pinned; defer to a real vtable reconstruction.
// Retail RTTI names the class zPTree; CButeNodeBase is this codebase's placeholder.
//
// 100% byte-exact: the /GX unwind frame retail emits here is triggered by the two
// destructible sub-objects (CContainerErr base + CButeNodeEntry member, both given
// non-virtual external dtors above) - cl sets trylevel 0 before the m_entry ctor so
// a throw unwinds the base. Was @early-stop at 59.58% while the frame was elided
// (no destructible sub-object was modeled). docs/patterns/gx-frame-destructible-local.md.
RVA(0x0016dff0, 0x73)
CButeNodeBase::CButeNodeBase(void* desc, i32 n)
    : CContainerErr(&g_buteNodeErrMsg), m_entry(n, desc) {
    m_entry.m_vtbl = &g_buteNodeSubVtbl;
    m_18 = 0;
    m_28 = 0;
    m_vtbl = &g_buteNodeVtbl;
}

// --- class-metadata sweep (Bute module): SIZE at this .cpp EOF (SIZE_UNKNOWN).
SIZE_UNKNOWN(CButeNodeEntry);
