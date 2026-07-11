// ButeStore.h - the CButeStore keyed-store family (extracted verbatim from
// <Bute/ButeMgr.h>, wave2-H) so ClearRecursive's defining TU (the merged
// TypeKeyColl.cpp, its retail home) can model the store WITHOUT pulling in
// ButeMgr.h's zPTree stand-in (which would clash with <Bute/PTreeNode.h>'s real
// zPTree in that TU). ButeMgr.h includes this header, so its many includers see
// the identical shapes.
//
// CButeStore - one owned keyed-store sub-tree (CButeMgr's m_tree / m_tree48 /
// m_tree74). 0x2c bytes, multiply-derived: a base store at +0 plus a second base
// at +8, both polymorphic (two vptrs). The mgr's /GX destructor tears each one
// down inline at its own trylevel; the member destructor reproduces that
// multiply-derived teardown:
//   (1) re-stamp the two most-derived vptrs (0x5e94ac @+0, 0x5e949c @+8),
//   (2) run the derived clear body (recursive node-free, __thiscall(this, 0)),
//   (3) restore the second base's vptr (__thiscall on the masked `this+8`,
//       `neg ecx; sbb ecx,ecx; and ecx,ebx` second-base-this adjust),
//   (4) run the primary-base destructor (__thiscall(this)).
// Only the SIZE + the two-vptr teardown are load-bearing; the interior is opaque.
// REAL POLYMORPHIC (ALL-VTABLES): CButeStore is modeled as a real multiply-derived
// class (a primary store base at +0x00 == 0x5e94ac, a second base at +0x08 ==
// 0x5e949c), so the /GX dtor auto-stamps both most-derived vptrs (== the old manual
// store-vtable re-stamps), reloc-masked; the three teardown callees are __thiscall
// engine functions (no body) so `mov ecx,this; call` falls out masked.
#ifndef SRC_BUTE_BUTESTORE_H
#define SRC_BUTE_BUTESTORE_H

#include <rva.h>
#include <Ints.h>

// The second-base subobject at +0x8: its vptr restore (0x16dfc0) is a __thiscall
// `mov [ecx],<vtbl>; ret`. Modeled as a tiny receiver so the call lands the
// masked `this+8` pointer in ecx with no caller-side cleanup.
struct CButeStoreBase2 {
    void RestoreVptr(); // 0x16dfc0
};
SIZE(CButeStoreBase2, 0x24); // receiver view of the +0x08 second base

// A keyed-store tree node the recursive clear (CButeStore::ClearRecursive)
// walks: left/right children, the heap-order key, the owned name string, and
// the value the store's per-value callback consumes+frees.
struct CButeStoreNode {
    CButeStoreNode* m_left;  // +0x00
    CButeStoreNode* m_right; // +0x04
    i32 m_key;               // +0x08  order key
    char* m_str;             // +0x0c  owned name string (freed)
    void* m_val;             // +0x10  value the per-value callback consumes (freed);
                             //        genuinely heterogeneous, so void*
};
SIZE(CButeStoreNode, 0x14);

// The primary store base at +0x00 (most-derived scalar-deleting vptr 0x5e94ac).
struct CButeStorePrimary {
    virtual void P0(); // +0x00  most-derived (scalar-deleting) vptr
    char m_pad04[4];   // +0x04
};
SIZE(CButeStorePrimary, 0x8); // { vptr, pad }

// The second base at +0x08 (second-base vptr 0x5e949c); carries the keyed-store
// interior fields (offsets shown relative to the enclosing CButeStore).
struct CButeStoreSecond {
    virtual void S0();          // +0x00 (this+0x08)  second-base vptr
    void(__cdecl* m_cb)(void*); // +0x04 (+0x0c)  per-value callback (ClearRecursive fires)
    char m_flags;               // +0x08 (+0x10)  flag byte (bit 2 gates the callback)
    char m_pad09[0x0c - 0x09];  // +0x09 (+0x11)  opaque keyed-store interior
    i32 m_14;                   // +0x0c (+0x14)  reset-to-empty field (Parse zeros it)
    CButeStoreNode* m_root18;   // +0x10 (+0x18)  tree root (Parse zeros it)
    char m_pad14[0x20 - 0x14];  // +0x14 (+0x1c)  opaque
    i32 m_28;                   // +0x20 (+0x28)  reset-to-empty field (Parse zeros it)
};
SIZE(CButeStoreSecond, 0x24); // second base (spans +0x08..+0x2c of CButeStore)

struct CButeStore : public CButeStorePrimary, public CButeStoreSecond {
    // The derived clear body (0x16e070): recursively frees the keyed nodes from
    // `node` (or the tree root when null). __thiscall(this, node); callee-cleans 4.
    void ClearRecursive(CButeStoreNode* node);
    // Reset-to-empty: free the nodes, then zero the root + the two reset fields.
    // Inlined into CButeMgr::Parse (the tree-base pointer lands in a register).
    void Reset() {
        ClearRecursive(0);
        m_root18 = 0;
        m_28 = 0;
        m_14 = 0;
    }
    // The primary-base destructor (0x16da60): restore vptr + tear down [this+4].
    void BaseDtor(); // __thiscall(this)
    ~CButeStore() {
        // cl auto-stamps the two most-derived vptrs (0x5e94ac @+0, 0x5e949c @+8) at
        // dtor entry (== the old manual re-stamps), reloc-masked.
        ClearRecursive(0);
        // The second base lives at this+8; the compiler null-masks the adjust
        // (`neg ecx; sbb ecx,ecx; and ecx,ebx`) -> RestoreVptr on (this?this+8:0).
        CButeStoreBase2* b2 = this ? (CButeStoreBase2*)((char*)this + 8) : 0;
        b2->RestoreVptr();
        BaseDtor();
    }
};
SIZE(CButeStore, 0x2c); // MI: CButeStorePrimary @0 + CButeStoreSecond @8

#endif // SRC_BUTE_BUTESTORE_H
