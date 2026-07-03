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
#include <Bute/ButeMgr.h> // shared CButeStore / CButeStoreNode
#include <rva.h>

// The engine __cdecl deallocator (reloc-masked rel32). _RezFree @0x1b9b82.
extern "C" void RezFree(void* p); // 0x1b9b82

RVA(0x0016e070, 0x7b)
void CButeStore::ClearRecursive(CButeStoreNode* node) {
    CButeStoreNode* n = node;
    if (n == 0) {
        n = m_root18;
        if (n == 0) {
            return;
        }
    }
    if (n->m_left != 0 && n->m_left->m_key > n->m_key) {
        ClearRecursive(n->m_left);
    }
    if (n->m_right != 0 && n->m_right->m_key > n->m_key) {
        ClearRecursive(n->m_right);
    }
    RezFree(n->m_str);
    if (m_flags & 2) {
        m_cb(n->m_val);
        RezFree(n->m_val);
    }
    RezFree(n);
}
