#include <rva.h>

#include <Bute/PTreeNode.h>
#include <Bute/ButeValue.h>
#include <Bute/ButeStore.h> // the canonical CButeStore (real bases; INLINE dtor)
#include <Gruntz/String.h>  // CString - the kButeString payload the teardown destructs

VTBL(CButeNode, 0x001f051c); // node primary (most-derived) vtable @+0x00 (this TU emits it)

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

// zPTree's OWN two most-derived vtables (== the store's): the pair every copy of the
// destructor stamps, and which zPTree's ctor (0x16dff0) stamps too. cl spells them through
// the ultimate polymorphic base.
// @data-symbol: ??_7zPTree@@6BzErrHandling@@@ 0x001e94ac 0x4
// @data-symbol: ??_7zPTree@@6BCButeNodeEntry@@@ 0x001e949c 0x4
RVA(0x00174d70, 0x70)
CButeNode::~CButeNode() {}

RVA(0x00174de0, 0x9)
void ButeStoreFreeAdapter(void* p) {
    (static_cast<CButeNode*>(p))->CButeNode::~CButeNode();
}
