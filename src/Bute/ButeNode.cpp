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

// The zErrHandling / CButeNodeEntry / zPTree family shapes are the shared
// <Bute/PTreeNode.h> (so sibling TUs derive zPTree too); ctor bodies below.
#include <Bute/PTreeNode.h>

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
// The +0x08 second-base-in-derived vtable @0x5f0518 (stamped by the ctor's
// `mov [esi+0x8],0x5f0518`). Being zPTree-MI-derived, cl emits this secondary itself
// as ??_7CButeCfgNode174d@@6BCButeNodeEntry@@ (verified in butenode.obj); bind the
// retail datum to that real emitted symbol instead of an AnalysisVtables placeholder.
// @data-symbol: ??_7CButeCfgNode174d@@6BCButeNodeEntry@@@ 0x001f0518 0x4
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

// ---------------------------------------------------------------------------
// CButeStore @0x174d70 - a multiple-inheritance /GX dtor (RVA-adjacent to the config
// nodes above). The most-derived class re-stamps its primary vptr (+0, 0x5e94ac) and
// its secondary base vptr (+8, 0x5e949c) at entry, runs ClearRecursive(0) (0x16e070),
// then destructs its bases in reverse declaration order: the secondary base @+8
// (~CObj50 0x16dfc0, with the `this ? this+8 : 0` adjust) then the primary base @0
// (~CContainerErr 0x16da60). Real-polymorphic MI model so cl emits both implicit vptr
// stamps + both base-dtor CALLs (all reloc-mask). The non-trivial bases earn the /GX
// frame (this TU is flags="eh"). This MI model cannot fold into ButeStoreClear.cpp's
// flat <Bute/ButeMgr.h> CButeStore without reconciling the two models (deferred).
// Re-homed from src/Stub/DiscoveredEh.cpp.
struct CContainerErr {
    virtual ~CContainerErr(); // 0x16da60 (external)
    char m_pad[8 - 4];        // vptr@+0; CObj50 begins at +8
};
SIZE_UNKNOWN(CContainerErr);
struct CObj50 {
    virtual ~CObj50(); // 0x16dfc0 (external; stamps 0x5f04d8 internally)
};
SIZE_UNKNOWN(CObj50);
struct CButeStore : CContainerErr, CObj50 {
    virtual ~CButeStore() OVERRIDE;              // 0x174d70
    void ClearRecursive(struct CButeStoreNode*); // 0x16e070 (real takes CButeStoreNode*; 0 = null)
};
SIZE_UNKNOWN(CButeStore);
RVA(0x00174d70, 0x70)
CButeStore::~CButeStore() {
    ClearRecursive(0);
}
