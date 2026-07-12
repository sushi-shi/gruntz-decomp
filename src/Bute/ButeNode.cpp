#include <rva.h>
// ButeNode.cpp - the .bute config-tree node TU: CButeNode (the concrete config node),
// the keyed store's destructor at 0x174d70, and the store's __cdecl per-value
// teardown callback at 0x174df0.
//
// RTTI-PROVEN NAME: the whole-object vtable 0x5e94ac carries the Complete-Object-
// Locator type descriptor `.?AVzPTree@@` (gruntz.analysis.vtable_hierarchy --class
// zPTree). zPTree's RTTI base-class-array names its primary base `zErrHandling` with a
// SECOND base (MI, secondary vtable @+8) - exactly our modeled shape (zErrHandling
// @+0, CButeNodeEntry @+8). zPTree's ctors (0x16df70 / 0x16dff0) live in their retail
// TU, the merged container obj src/Bute/TypeKeyColl.cpp; only the shapes are shared,
// via <Bute/PTreeNode.h>, so sibling TUs derive zPTree too.
//
// This unit carries C++ /GX EH frames (push -1 / push handler / fs:0 save) for the
// multiply-derived construction/teardown; flags="eh".
//
// Field names are placeholders; only OFFSETS + code bytes are load-bearing.

// The zErrHandling / CButeNodeEntry / zPTree / CButeNode family shapes.
#include <Bute/PTreeNode.h>
// CButeValue + ButeType - the typed value record the per-value teardown frees. The one
// canonical shape, shared with <Bute/ButeMgr.h> (which is NOT pulled in here: its
// <Bute/ButeStore.h> CButeStore would clash with this TU's own store class below).
#include <Bute/ButeValue.h>
// The REAL container-error base: its destructor IS the 0x16da60 this store's dtor
// chains to. zBitVec.h's `CContainerErr` and PTreeNode.h's `zErrHandling` are the SAME
// engine class under two reconstruction names, but only CContainerErr's dtor is
// DEFINED (in typekeycoll), so the store below derives THAT one and its primary
// base-dtor call name-binds instead of dangling on the undefined ??1zErrHandling.
// (@identity-TODO: the 3-way CContainerErr / zErrHandling / GameText.h-view split is a
//  real fold, but it is blocked outside this lane - see the ~CButeStore note below.)
#include <Wap32/zBitVec.h>
#include <Gruntz/String.h> // CString - the kButeString payload the teardown destructs

// ===========================================================================
// ButeValueTeardown (0x174df0) - the keyed store's __cdecl per-value teardown
// callback.
//
// RECOVERED FROM .text: 0x174df0 is a CODE address - a real function, with a 9-entry
// jump table at 0x174e48 - NOT a datum. The former `g_node174df0Tag` (u8) and
// `g_nodeDescriptor` (i32) externs were phantoms for this function's ADDRESS, which is
// the "descriptor" every CButeNode is constructed with (`new CButeNode(&ButeValueTeardown,
// 2)`). The address lands in the node's +0x0c callback slot; CButeStore::ClearRecursive
// fires it on each node's value and then frees the value cell itself
// (`m_cb(n->m_val); ::operator delete(n->m_val);`) - which is exactly why this frees
// only the PAYLOAD (v->pValue) and never `v`.
//
// The body is CButeValue's teardown switch (the same one ~CButeValue @0x172160 carries
// in butemgr - the logic is emitted once per TU that needs it): a dense 0..8 ButeType
// jump table; the kButeString case destructs the boxed CString first, every other
// type-group is a plain `operator delete(pValue)`.
//
// @early-stop
// per-arm scratch-register wall (~69%) - the twin of ~CButeValue (0x172160, ~69% for the
// same reason). BYTE-PROVEN with a raw COMDAT-vs-retail compare: of 0x7c bytes, every
// difference is either a reloc-masked field (the two call rel32s, the jmp's table DIR32,
// the 9 jump-table entries) or ONE byte per delete-arm - the arm's scratch register.
// Structure is byte-exact: same prologue, same `cmp ecx,8 / ja +0x48`, same four arms at
// the same offsets (0x13 string / 0x2c / 0x3a / 0x48), same shared `pop esi; ret` tail,
// same `8b ff` align pad. Retail colours the three identical `mov reg,[eax+4]; push reg;
// call ??3` arms edx / eax / ecx; cl colours them eax / ecx / edx.
// NOT source-steerable: the permuter found no lever (68.8, no change), and reordering the
// case groups to chase the colouring MOVES the blocks (cl lays arms out in source order),
// which craters it to 5.4%. So the arm ORDER is right and only the colouring is off.
// ===========================================================================
RVA(0x00174df0, 0x7c)
void __cdecl ButeValueTeardown(void* pValue) {
    CButeValue* v = (CButeValue*)pValue;
    switch (v->type) {
        case kButeString:
            delete (CString*)v->pValue;
            break;
        case kButeDouble:
        case kButeRef6:
            delete (double*)v->pValue;
            break;
        case kButeInt:
        case kButeFloat:
        case kButeRef7:
            delete (i32*)v->pValue;
            break;
        case kButeDword:
        case kButeRef5:
        case kButeRef8:
            delete (u32*)v->pValue;
            break;
    }
}

// ===========================================================================
// CButeNode::CButeNode(kind) (0x174d00) - the out-of-line 1-arg config-node ctor: pass
// the fixed per-value teardown callback as the node's descriptor, forward the kind.
// REAL POLYMORPHIC (ALL-VTABLES): derives zPTree, so cl auto-stamps the two
// most-derived vftables (primary 0x5f051c @+0x00, second base 0x5f0518 @+0x08) after
// the external base ctor runs (== the old manual double stamp). FOLDED with butemgr's
// CButeNode - the same class (identical vtable pair), so the fabricated
// `CButeCfgNode174d` name is gone and both TUs now emit the same ??_7CButeNode symbols.
// (The old "vptr-schedule wall" @early-stop here is RESOLVED: it was never a vptr
//  problem - the descriptor was bound to a phantom `g_node174df0Tag` u8. Passing the
//  real function address took this ctor to 100% WITH the real polymorphic vtables.)
//
// The ctor's +0x00 primary vptr-store: cl names the class's OWN primary vtable through
// the ultimate polymorphic base (zErrHandling), not the simple ??_7...@@6B@ that VTBL()
// emits, so the reloc needs the through-base alias (same 0x1f051c datum; the through-base
// name sorts last and wins the per-rva dedup). BOTH TUs that build a CButeNode - this
// one and butemgr's ParseTagLine (which inlines the ctor) - now emit these same two
// names, so both bind. (@data-symbol is read from the .cpp only, never from a header.)
// @data-symbol: ??_7CButeNode@@6BzErrHandling@@@ 0x001f051c
// The +0x08 second-base-in-derived vtable @0x5f0518 (the `mov [esi+0x8],0x5f0518` stamp).
// @data-symbol: ??_7CButeNode@@6BCButeNodeEntry@@@ 0x001f0518 0x4
// ===========================================================================
RVA(0x00174d00, 0x25)
CButeNode::CButeNode(i32 kind) : zPTree((void*)&ButeValueTeardown, kind) {}

// ---------------------------------------------------------------------------
// CButeStore @0x174d70 - the keyed store's multiple-inheritance /GX destructor. The
// most-derived class re-stamps its primary vptr (+0, 0x5e94ac) and its secondary base
// vptr (+8, 0x5e949c) at entry, runs ClearRecursive(0) (0x16e070), then destructs its
// bases in reverse declaration order: the secondary base @+8 (~CButeNodeEntry 0x16dfc0,
// with the `this ? this+8 : 0` MI adjust) then the primary base @+0 (~CContainerErr
// 0x16da60). Real-polymorphic MI model, so cl emits both implicit vptr stamps and both
// base-dtor CALLs (all reloc-masked). The two non-trivial bases earn the /GX frame.
//
// PROVEN bases (sema disasm): primary @+0 = the container-error base (its dtor 0x16da60
// re-stamps its own vtable 0x5f04cc); secondary @+8 = CButeNodeEntry (its dtor 0x16dfc0
// is a one-instruction `mov [ecx],offset ??_7CButeNodeEntry(0x5f04d8); ret` - now
// DEFINED in typekeycoll, so this call binds).
//
// @identity-TODO - THE STORE/zPTree CONFLATION. This dtor and its two byte-identical
// twins in butemgr (0x21310 / 0x21570) ALL stamp the SAME vtable pair (0x1e94ac /
// 0x1e949c), which RTTI names ??_7zPTree. Three destructors sharing one vtable pair
// means the three are ONE retail class (the store IS zPTree), duplicated across objs.
// Our tree must give them three distinct C++ names (one symbol cannot own three RVAs),
// so per rva only ONE of the vptr-stamp relocs can name-bind (one-name-per-rva). The
// clean fix is to rename the whole store family to `zPTree` and move ClearRecursive +
// the vtables onto it - but ?ClearRecursive@CButeStore@@ has 7 external callers
// (chatboxowner / rezsync / demo / ...) outside this lane, so it is reported as
// cross-lane deferred work rather than half-done here.
// ---------------------------------------------------------------------------
struct CButeStore : CContainerErr, CButeNodeEntry {
    virtual ~CButeStore() OVERRIDE;              // 0x174d70
    void ClearRecursive(struct CButeStoreNode*); // 0x16e070 (0 = null -> the root)
    i32 m_child18;                               // +0x18  tree root / child link
    char m_pad1c[0x28 - 0x1c];
    i32 m_child28; // +0x28  child link
};
SIZE(CButeStore, 0x2c); // MI: CContainerErr @0 (8B) + CButeNodeEntry @8 (0x10B) + child links
// The dtor's +0x08 secondary vptr-store: cl names CButeStore's OWN CButeNodeEntry-base
// vtable ??_7CButeStore@@6BCButeNodeEntry@@@ (0x1e949c; the through-base name sorts last
// and wins the per-rva dedup). The +0x00 primary stamp @0x1e94ac is the shared
// (conflated) zPTree vtable and stays bound under ??_7zPTree@@6B@ - see the TODO above.
// @data-symbol: ??_7CButeStore@@6BCButeNodeEntry@@@ 0x001e949c
RVA(0x00174d70, 0x70)
CButeStore::~CButeStore() {
    ClearRecursive(0);
}
