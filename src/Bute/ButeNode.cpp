#include <rva.h>
// ButeNode.cpp - zPTree, the ButeMgr config-tree node base (dedicated unit).
//
// RTTI-PROVEN NAME: the whole-object vtable 0x5e94ac carries the Complete-Object-
// Locator type descriptor `.?AVzPTree@@` (gruntz.analysis.vtable_hierarchy --class
// zPTree, aka the fabricated CButeNodeBase). zPTree's RTTI base-class-array names
// its primary base `zErrHandling` with a SECOND base (MI, secondary vtable @+8) -
// which is exactly our modeled shape (zErrHandling @+0, CButeNodeEntry @+8). The
// former fabricated name "CButeNodeBase" is replaced by the real library name
// zPTree here; its primary base is the RTTI-real `zErrHandling` (the same engine
// class is modeled per-TU elsewhere under placeholder names - EngStr/GameText/
// ProjActCache).
//
// This is its own self-contained class model (zErrHandling base, CButeNodeEntry
// subobject); it cannot fold into ButeMgr.cpp, which carries a different minimal
// `class zPTree` decl (ButeMgr.h, the empty call-site stand-in) for the ParseTagLine
// `new CButeNode` path -> ODR conflict. So it lives in its own unit, flags="eh".
//
// zPTree derives from zErrHandling (the container-library exception base, ctor
// @0x16d9c0, modeled in GameText) and embeds a small node subobject
// at +0x8 (the keyed-store entry: a ptr + a 16-bit kind + a vtbl). The ctor
// runs the zErrHandling base ctor (passing the node's error-message global),
// constructs the +0x8 subobject from (n, desc), then re-stamps the two derived
// vtables (the subobject at +0x8 and the whole object at +0x0) and zeroes the
// two child-link fields at +0x18 / +0x28.
//
// This ctor carries a C++ /GX EH frame (push -1 / push handler / fs:0 save) for
// the subobject construction; this unit is flags="eh" so the frame is present.
//
// Field names are placeholders; only OFFSETS + code bytes are load-bearing.

// zErrHandling - the container-library exception base (vptr@0, msg@4), RTTI-real
// name (zPTree's RTTI base-class-array names its primary base `zErrHandling`). Its
// ctor (RVA 0x16d9c0, real body in GameText) is __thiscall(this, msg); declared
// no-body here so the `push msg; call` shape is reloc-masked. The same engine class
// is modeled per-TU elsewhere under placeholder names (EngStr/GameText/ProjActCache);
// this unit carries its own self-contained decl. REAL POLYMORPHIC (ALL-VTABLES phase):
// the vtbl@0 field is the implicit vptr (virtual dtor); the derived zPTree ctor
// auto-stamps ??_7zPTree @+0 after the external base ctor returns (== the old manual
// whole-object restamp). The base stays destructible so the derived ctor keeps its
// /GX unwind frame.
extern void* g_buteNodeErrMsg; // DAT_006bf480 - the node's error-message global

class zErrHandling {
public:
    zErrHandling(void* msg);
    virtual ~zErrHandling(); // +0x00 vptr; external no-body dtor (real body in GameText)

    void* m_msg; // +0x04
};

// The node subobject at zPTree+0x8 (a small keyed-store entry): the SECOND
// polymorphic base of zPTree. REAL POLYMORPHIC: its ctor (0x16df70) auto-
// stamps ??_7CButeNodeEntry (retail 0x5f04d8) at +0x0, then stores desc@+4,
// (WORD)n@+8, 0@+0xc. As zPTree's second base it lands at +0x08 and its vptr
// is promoted to the second-base-in-derived vtable (0x5e949c) automatically by cl
// (== the old manual raw sub-vtable write).
class CButeNodeEntry {
public:
    CButeNodeEntry(i32 n, void* desc);
    virtual ~CButeNodeEntry(); // +0x00 vptr; external no-body scalar-deleting dtor

    void* m_desc; // +0x04  kind descriptor (opaque; a tag pointer)
    i16 m_kind;   // +0x08  (WORD)n
    char m_pada[2];
    i32 m_0c; // +0x0c  zero-init
};
SIZE(CButeNodeEntry, 0x10);       // { vptr, desc, kind, 0 }
VTBL(CButeNodeEntry, 0x001f04d8); // the entry member's own (base) vtable

// zPTree layout (multiply-derived, two vptrs):
//   +0x00  zErrHandling base    (vptr, msg)
//   +0x08  CButeNodeEntry base  (vptr, desc, kind, 0) - spans +0x08..+0x18
//   +0x18  m_child18 : child link, zeroed
//   +0x28  m_child28 : child link, zeroed
class zPTree : public zErrHandling, public CButeNodeEntry {
public:
    zPTree(void* desc, i32 n);

    // CButeNodeEntry base occupies +0x08..+0x18
    i32 m_child18; // +0x18  child link (zeroed)
    char m_pad1c[0x28 - 0x1c];
    i32 m_child28; // +0x28  child link (zeroed)
};
SIZE(zPTree, 0x2c); // measured: new(0x2c) -> ctor 0x16dff0; matches the layout above

// CButeNodeEntry ctor (0x16df70): __thiscall(this, n, desc). cl auto-stamps the
// ??_7CButeNodeEntry vptr@+0, then stores desc@+4, (WORD)n@+8, 0@+0xc. Clean leaf
// ctor. Called out-of-line by the zPTree ctor's m_entry member-init.
// @early-stop
// vptr-position wall (~82.9%, was 100% hand-rolled): real polymorphism sinks the
// implicit ??_7 vptr stamp to FIRST; the hand-rolled last-store cannot be sunk in
// MSVC5 (same mechanism as CZArray2D). Converted per the ALL-VTABLES mandate.
RVA(0x0016df70, 0x22)
CButeNodeEntry::CButeNodeEntry(i32 n, void* desc) {
    m_desc = desc;
    m_kind = (i16)n;
    m_0c = 0;
}

// zPTree ctor (0x16dff0): run the zErrHandling primary base ctor + the
// CButeNodeEntry second-base ctor, then cl auto-stamps the two most-derived vptrs
// (??_7zPTree @+0 = 0x5e94ac, and the second-base-in-derived vtable @+8 =
// 0x5e949c) and zeroes the two child links. /GX unwind frame from the two
// destructible base sub-objects. (Was 100% hand-rolled; the auto-vptr stamps are
// accepted per the ALL-VTABLES mandate.)
// @early-stop
// vptr-position wall (~96.1%, was 100%): real MI polymorphism auto-stamps both
// vptrs FIRST, shifting the stamp schedule vs the hand-rolled last-stores. Logic
// byte-faithful; converted per the ALL-VTABLES mandate.
RVA(0x0016dff0, 0x73)
zPTree::zPTree(void* desc, i32 n) : zErrHandling(&g_buteNodeErrMsg), CButeNodeEntry(n, desc) {
    m_child18 = 0;
    m_child28 = 0;
}

// ===========================================================================
// CButeCfgNode174d - a concrete zPTree-derived config-tree node ctor
// (0x174d00), re-homed from src/Stub/MallocConstructors (was "Node174d00"). A
// zPTree-family node. REAL POLYMORPHIC (ALL-VTABLES): derives from zPTree so
// cl auto-stamps its two most-derived vftables (primary @0x5f051c at +0x00 and the
// second-base-in-derived vtable @0x5f0518 at +0x08) after the external base ctor
// runs (== the old manual double stamp). The concrete class has no RTTI
// type-descriptor in retail so the name stays a placeholder; base + module
// (zPTree, the .bute config tree) are proven.
extern u8 g_node174df0Tag; // 0x574df0  kind descriptor (in .text)

VTBL(CButeCfgNode174d, 0x001f051c); // node primary (most-derived) vtable @+0x00
class CButeCfgNode174d : public zPTree {
public:
    CButeCfgNode174d(i32 kind);           // 0x174d00
    virtual ~CButeCfgNode174d() OVERRIDE; // slot 0 (zPTree dtor)
};
SIZE(CButeCfgNode174d, 0x2c); // derives zPTree (no new fields)

// @early-stop
// vptr-schedule wall (ALL-VTABLES): the base ctor is called first, then cl stamps
// the two most-derived vptrs, but the compiler's vptr-first schedule + the base's
// own vptr-position wall shift the store order vs the hand-rolled double-stamp. Was
// 100% hand-rolled; converted per the ALL-VTABLES mandate (regression OK).
RVA(0x00174d00, 0x25)
CButeCfgNode174d::CButeCfgNode174d(i32 kind) : zPTree(&g_node174df0Tag, kind) {}
