// ButeTree.cpp - CButeTree::Walk, the string-keyed crit-bit (PATRICIA) trie's
// recursive traversal.
//
// The trie's core (Find 0x16d190 / Insert 0x16db90 / FirstDiffBit 0x16e480)
// lives in its retail TU, the merged container/type-registry obj at 0x16d000
// (src/Gruntz/TypeKeyColl.cpp, wave2-H). Walk (0x193340) is retail-sited far
// away (just before the projactcache interval @0x1933b0) - it stays here as the
// unit's one function until that region's own TU resolution.
//
// Field names are placeholders; only the OFFSETS + emitted bytes are load-bearing.
#include <Bute/ButeTree.h> // canonical CButeTree / CVariantSlot / CButeTreeNode (one shape)
#include <Ints.h>
#include <rva.h>

// ===========================================================================
// CButeTree::Walk (0x193340) - recursive crit-bit trie traversal: for each node,
// invoke fn(node->key, node->value, ctx), recurse on the left child (child[0]) then
// iterate down the right child (child[1]) as long as the child's crit-bit index still
// increases (a proper crit-bit descent - stops when it loops back up). A null start
// node begins from m_root. Re-homed from ReconBatch2 (was the Tree_193340 placeholder
// view; CButeMgr::ParseGroup calls it on a CButeTree sub-tree; node layout == CButeTreeNode).
// ===========================================================================
RVA(0x00193340, 0x61)
void CButeTree::Walk(
    void(__cdecl* fn)(char* key, void* value, void* ctx),
    void* ctx,
    CButeTreeNode* node
) {
    while (1) {
        if (node == 0) {
            node = m_root;
            if (node == 0) {
                return;
            }
        }
        fn(node->m_key, node->m_value, ctx);
        CButeTreeNode* l = node->m_child[0];
        if (l != 0 && l->m_bit > node->m_bit) {
            Walk(fn, ctx, l);
        }
        CButeTreeNode* r = node->m_child[1];
        if (r == 0 || r->m_bit <= node->m_bit) {
            return;
        }
        node = r;
    }
}
