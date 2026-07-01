// ButeStoreClear.cpp - CButeStore::ClearRecursive (0x16e070), the derived keyed-
// store's recursive node-free (C:\Proj\Bute). The trace mislabeled it
// CButeTree_16e070 because the recursion runs on a CButeStore `this`; the class
// is the CButeStore modeled in <Bute/ButeMgr.h> (whose ~CButeStore already calls
// this as a no-body external). Kept in its own TU with a local class view so it
// stays frameless (base flags) and self-contained.
//
// The store holds a binary tree at +0x18 keyed by each node's +0x08. The walk
// post-order frees: it recurses into a child only when that child's key is
// GREATER than the current node's (the heap-ordered owned-subtree invariant),
// frees the node's name string (+0x0c), then - when the store's +0x10 flag has
// bit 2 - runs the store's per-value callback (+0x0c fn-ptr) on the node's value
// (+0x10) and frees it, and finally frees the node itself. Field names are
// placeholders; only offsets + code bytes are load-bearing.
#include <rva.h>

// The engine __cdecl deallocator (reloc-masked rel32). _RezFree @0x1b9b82.
extern "C" void RezFree(void* p); // 0x1b9b82

// A keyed-store tree node: left/right children, the order key, the owned name
// string, and the value the callback consumes.
struct CButeStoreNode {
    CButeStoreNode* m_left;  // +0x00
    CButeStoreNode* m_right; // +0x04
    i32 m_key;               // +0x08  order key
    void* m_str;             // +0x0c  owned name string (freed)
    void* m_val;             // +0x10  value (callback target, freed)
};

// Local CButeStore view: only the three fields ClearRecursive reads (the +0x0c
// callback, the +0x10 flag byte, the +0x18 tree root). The full class lives in
// <Bute/ButeMgr.h>; the mangled method name is layout-independent.
struct CButeStore {
    char m_pad0[0xc];
    void(__cdecl* m_cb)(void*); // +0x0c  per-value callback
    char m_flags;               // +0x10  flag byte (bit 2 gates the callback)
    char m_pad11[0x18 - 0x11];
    CButeStoreNode* m_root; // +0x18  tree root
    void ClearRecursive(i32 node);
};

RVA(0x0016e070, 0x7b)
void CButeStore::ClearRecursive(i32 node) {
    CButeStoreNode* n = (CButeStoreNode*)node;
    if (n == 0) {
        n = m_root;
        if (n == 0) {
            return;
        }
    }
    if (n->m_left != 0 && n->m_left->m_key > n->m_key) {
        ClearRecursive((i32)n->m_left);
    }
    if (n->m_right != 0 && n->m_right->m_key > n->m_key) {
        ClearRecursive((i32)n->m_right);
    }
    RezFree(n->m_str);
    if (m_flags & 2) {
        m_cb(n->m_val);
        RezFree(n->m_val);
    }
    RezFree(n);
}

// --- class-metadata sweep (Bute module): SIZE at this .cpp EOF (SIZE_UNKNOWN).
SIZE_UNKNOWN(CButeStoreNode);
