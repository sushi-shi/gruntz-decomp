#include <rva.h>

#include <Bute/ButeTree.h>

#include <Ints.h>

#include <stddef.h>

RVA(0x00193340, 0x61)
void zPTree::Walk(
    void(__cdecl* fn)(char* key, void* value, void* ctx),
    void* ctx,
    CButeTreeNode* node
) {
    while (1) {
        if (node == NULL) {
            node = m_root;
            if (node == NULL) {
                return;
            }
        }
        fn(node->m_key, node->m_value, ctx);
        CButeTreeNode* l = node->m_child[0];
        if (l != NULL && l->m_bit > node->m_bit) {
            Walk(fn, ctx, l);
        }
        CButeTreeNode* r = node->m_child[1];
        if (r == NULL || r->m_bit <= node->m_bit) {
            return;
        }
        node = r;
    }
}
