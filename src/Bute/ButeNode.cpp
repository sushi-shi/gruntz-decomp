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
// canonical shape, shared with <Bute/ButeMgr.h> (not pulled in here - it is simply not
// needed). STALE CLAIM REMOVED (2026-07-13): the old reason ("its <Bute/ButeStore.h>
// CButeStore would clash with this TU's own store class below") is imaginary - ButeStore.h
// is included RIGHT BELOW, and the local CButeStoreDtorCopyNode DERIVES from that same canonical
// CButeStore. Adding #include <Bute/ButeMgr.h> here compiles clean under the real MSVC 5.0.
#include <Bute/ButeValue.h>
#include <Bute/ButeStore.h>           // the canonical CButeStore (real bases; INLINE dtor)
#include <Bute/ButeStoreDtorCopies.h> // CButeStoreDtorCopyNode anchor (0x174d70 ~ copy)
#include <Gruntz/String.h>            // CString - the kButeString payload the teardown destructs

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
    CButeValue* v = static_cast<CButeValue*>(pValue);
    switch (v->type) {
        case kButeString:
            delete static_cast<CString*>(v->pValue);
            break;
        case kButeDouble:
        case kButeRef6:
            delete static_cast<double*>(v->pValue);
            break;
        case kButeInt:
        case kButeFloat:
        case kButeRef7:
            delete static_cast<i32*>(v->pValue);
            break;
        case kButeDword:
        case kButeRef5:
        case kButeRef8:
            delete static_cast<u32*>(v->pValue);
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
// @data-symbol: ??_7CButeNode@@6BCContainerErr@@@ 0x001f051c
// The +0x08 second-base-in-derived vtable @0x5f0518 (the `mov [esi+0x8],0x5f0518` stamp).
// @data-symbol: ??_7CButeNode@@6BCButeNodeEntry@@@ 0x001f0518 0x4
// ===========================================================================
RVA(0x00174d00, 0x25)
CButeNode::CButeNode(i32 kind) : zPTree(&ButeValueTeardown, kind) {}

// ---------------------------------------------------------------------------
// CButeStore @0x174d70 - ONE OF THE THREE COPIES of the store's inline destructor.
//
// The class is now the canonical <Bute/ButeStore.h> CButeStore (CContainerErr @0 +
// CButeNodeEntry @8), whose destructor is INLINE. Retail carries three byte-identical
// copies of it - 0x174d70 here, 0x21310 and 0x21570 in butemgr - because MSVC5 (no /Gy)
// emits an inline member as a per-object-file static while folding the vftable COMDAT,
// which is exactly why all three copies stamp the ONE vtable pair (0x1e94ac / 0x1e949c).
// C++ cannot give one symbol three RVAs, so each copy is anchored on its own thin
// subclass; the inline ~CButeStore expands into each, reproducing the retail body:
// stamp both vptrs, ClearRecursive(0) (now the REAL ?ClearRecursive@CButeStore@@ at
// 0x16e070 - it used to be a per-copy fake declared nowhere), then fold the +0x08 base
// (~CButeNodeEntry 0x16dfc0) and the +0x00 base (~CContainerErr 0x16da60).
// (CButeStoreDtorCopyNode - the 0x174d70 anchor - is declared in the shared
//  <Bute/ButeStoreDtorCopies.h>; its ~ body + the free-adapter below are this TU's.)

// zPTree's OWN two most-derived vtables (== the store's): the pair every copy of the
// destructor stamps, and which zPTree's ctor (0x16dff0) stamps too. cl spells them through
// the ultimate polymorphic base.
// @data-symbol: ??_7zPTree@@6BCContainerErr@@@ 0x001e94ac
// @data-symbol: ??_7zPTree@@6BCButeNodeEntry@@@ 0x001e949c 0x4
RVA(0x00174d70, 0x70)
CButeStoreDtorCopyNode::~CButeStoreDtorCopyNode() {}

// The __cdecl -> __thiscall per-value teardown adapter the CBSecStream ctor passes
// as zPTree's free-callback (retail 0x174de0: `mov ecx,[esp+4]; jmp 0x174d70`, laid
// directly after the dtor above - this TU's code). It used to be mis-modeled as a
// DATA global (`u8 g_streamTag`): a FUNCTION, not a datum. The qualified dtor call
// keeps the dispatch direct (the tail-jmp), matching retail.
RVA(0x00174de0, 0x9)
void ButeStoreFreeAdapter(void* p) {
    (static_cast<CButeStoreDtorCopyNode*>(p))->CButeStoreDtorCopyNode::~CButeStoreDtorCopyNode();
}
